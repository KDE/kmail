// kmfolder.cpp
// Author: Stefan Taferner <taferner@alpin.or.at>

#include <qfileinfo.h>
#include <qsortedlist.h>

#include "kmmessage.h"
#include "kmfolderdir.h"
#include "kmfolderimap.h"
#include "kmundostack.h"
#include "kmmsgdict.h"
#include "kmidentity.h"
#include "kmfiltermgr.h"

#include <mimelib/mimepp.h>
#include <qregexp.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kcursor.h>

#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_BYTESWAP_H
#include <byteswap.h>
#endif

// We define functions as kmail_swap_NN so that we don't get compile errors
// on platforms where bswap_NN happens to be a function instead of a define.

/* Swap bytes in 32 bit value.  */
#ifdef bswap_32
#define kmail_swap_32(x) bswap_32(x)
#else
#define kmail_swap_32(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) |		      \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))
#endif

//#define HAVE_MMAP //need to get this into autoconf FIXME  --Sam
#ifdef HAVE_MMAP
#include <unistd.h>
#include <sys/mman.h>
#endif

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <klocale.h>
#include <kapplication.h>

#ifndef MAX_LINE
#define MAX_LINE 4096
#endif
#ifndef INIT_MSGS
#define INIT_MSGS 8
#endif

// Current version of the table of contents (index) files
#define INDEX_VERSION 1506

static int _rename(const char* oldname, const char* newname)
{
  return rename(oldname, newname);
}


//-----------------------------------------------------------------------------
KMFolder :: KMFolder(KMFolderDir* aParent, const QString& aName) :
  KMFolderInherited(aParent, aName), mMsgList(INIT_MSGS)
{
  mIndexStream    = NULL;
  mOpenCount      = 0;
  mQuiet          = 0;
  mChanged        = FALSE;
  mHeaderOffset   = 0;
  mAutoCreateIndex= TRUE;
  mIsSystemFolder = FALSE;
  mType           = "plain";
  mAcctList       = NULL;
  mDirty          = FALSE;
  mUnreadMsgs      = -1;
  mGuessedUnreadMsgs = -1;
  needsCompact    = FALSE;
  mChild          = 0;
  mConvertToUtf8  = FALSE;
  mMailingListEnabled = FALSE;
  mIndexId        = -1;
  mIndexStreamPtr = NULL;
  mIndexStreamPtrLength = 0;
  mIndexSwapByteOrder = false;
  mIndexSizeOfLong = sizeof(long);
  mCompactable     = TRUE;
  mNoContent      = FALSE;
  expireMessages = FALSE;
  unreadExpireAge = 28;
  unreadExpireUnits = expireNever;
  readExpireAge = 14;
  readExpireUnits = expireNever;
  mRDict = 0;
}


//-----------------------------------------------------------------------------
KMFolder :: ~KMFolder()
{
  delete mAcctList;
  KMMsgDict::deleteRentry(mRDict);
}


//-----------------------------------------------------------------------------
QString KMFolder::dotEscape(const QString& aStr) const
{
  if (aStr[0] != '.') return aStr;
  return aStr.left(aStr.find(QRegExp("[^\\.]"))) + aStr;
}


//-----------------------------------------------------------------------------
QString KMFolder::location() const
{
  QString sLocation(path());

  if (!sLocation.isEmpty()) sLocation += '/';
  sLocation += dotEscape(fileName());

  return sLocation;
}


//-----------------------------------------------------------------------------
QString KMFolder::indexLocation() const
{
  QString sLocation(path());

  if (!sLocation.isEmpty()) sLocation += '/';
  sLocation += '.';
  sLocation += dotEscape(fileName());
  sLocation += ".index";

  return sLocation;
}

//-----------------------------------------------------------------------------
QString KMFolder::subdirLocation() const
{
  QString sLocation(path());

  if (!sLocation.isEmpty()) sLocation += '/';
  sLocation += '.';
  sLocation += dotEscape(fileName());
  sLocation += ".directory";

  return sLocation;
}

//-----------------------------------------------------------------------------
KMFolderDir* KMFolder::createChildFolder()
{
  QString childName = "." + fileName() + ".directory";
  QString childDir = path() + "/" + childName;
  bool ok = true;

  if (mChild)
    return mChild;

  if (access(childDir.local8Bit(), W_OK) != 0) // Not there or not writable
  {
    if (mkdir(childDir.local8Bit(), S_IRWXU) != 0
      && chmod(childDir.local8Bit(), S_IRWXU) != 0)
        ok=false; //failed create new or chmod existing tmp/
  }

  if (!ok) {
    QString wmsg = QString(" '%1': %2").arg(childDir).arg(strerror(errno));
    KMessageBox::information(0,i18n("Failed to create directory") + wmsg);
    return 0;
  }

  KMFolderDir* folderDir = new KMFolderDir(parent(), childName,
    protocol() == "imap");
  if (!folderDir)
    return 0;
  folderDir->reload();
  parent()->append(folderDir);
  mChild = folderDir;
  return folderDir;
}

//-----------------------------------------------------------------------------
bool KMFolder::isIndexOutdated()
{
  QFileInfo contInfo(location());
  QFileInfo indInfo(indexLocation());

  if (!contInfo.exists()) return FALSE;
  if (!indInfo.exists()) return TRUE;

  return (contInfo.lastModified() > indInfo.lastModified());
}


//-----------------------------------------------------------------------------
int KMFolder::writeIndex()
{
  QString tempName;
  KMMsgBase* msgBase;
  int old_umask;
  int i=0, len;
  long tmp;
  const uchar *buffer = NULL;
  old_umask = umask(077);

  //the sorted file must be removed, BIG kludge, don made me do it!
  unlink(indexLocation().local8Bit() + ".sorted");
  tempName = indexLocation() + ".temp";
  unlink(tempName.local8Bit());

  // We touch the folder, otherwise the index is regenerated, if KMail is
  // running, while the clock switches from daylight savings time to normal time
  KProcess p;
  p << "touch" << location();
  p.start(KProcess::Block);

  FILE *tmpIndexStream = fopen(tempName.local8Bit(), "w");
  umask(old_umask);
  if (!tmpIndexStream)
    return errno;

  fprintf(tmpIndexStream, "# KMail-Index V%d\n", INDEX_VERSION);

  // Header
  Q_UINT32 byteOrder = 0x12345678;
  Q_UINT32 sizeOfLong = sizeof(long);

  Q_UINT32 header_length = sizeof(byteOrder)+sizeof(sizeOfLong); 
  char pad_char = '\0';
  fwrite(&pad_char, sizeof(pad_char), 1, tmpIndexStream);
  fwrite(&header_length, sizeof(header_length), 1, tmpIndexStream);

  // Write header 
  fwrite(&byteOrder, sizeof(byteOrder), 1, tmpIndexStream);
  fwrite(&sizeOfLong, sizeof(sizeOfLong), 1, tmpIndexStream);

  long nho = ftell(tmpIndexStream);
  for (i=0; i<mMsgList.high(); i++)
  {
    if (!(msgBase = mMsgList[i])) continue;
    buffer = msgBase->asIndexString(len);
    fwrite(&len,sizeof(len), 1, tmpIndexStream);
	
    tmp = ftell(tmpIndexStream);
    msgBase->setIndexOffset(tmp);
    msgBase->setIndexLength(len);
    if(fwrite(buffer, len, 1, tmpIndexStream) != 1)
	kdDebug(5006) << "Whoa! " << __FILE__ << ":" << __LINE__ << endl;
  }
  if (ferror(tmpIndexStream)) return ferror(tmpIndexStream);
  if (fflush(tmpIndexStream) != 0) return errno;
  if (fsync(fileno(tmpIndexStream)) != 0) return errno;
  if (fclose(tmpIndexStream) != 0) return errno;

  _rename(tempName.local8Bit(), indexLocation().local8Bit());
  if (mIndexStream)
      fclose(mIndexStream);
  mHeaderOffset = nho;

  mIndexStream = fopen(indexLocation().local8Bit(), "r+"); // index file
  updateIndexStreamPtr();

  writeMsgDict();

  mDirty = FALSE;
  return 0;
}


//-----------------------------------------------------------------------------
void KMFolder::setAutoCreateIndex(bool autoIndex)
{
  mAutoCreateIndex = autoIndex;
}


//-----------------------------------------------------------------------------
bool KMFolder::readIndexHeader(int *gv)
{
  int indexVersion;
  assert(mIndexStream != NULL);
  mIndexSwapByteOrder = false;
  mIndexSizeOfLong = sizeof(long);

  fscanf(mIndexStream, "# KMail-Index V%d\n", &indexVersion);
  if(gv)
      *gv = indexVersion;
  if (indexVersion < 1505 ) {
      if(indexVersion == 1503) {
	  kdDebug(5006) << "Converting old index file " << indexLocation() << " to utf-8" << endl;
	  mConvertToUtf8 = TRUE;
      }
      return TRUE;
  } else if (indexVersion == 1505) {
  } else if (indexVersion < INDEX_VERSION) {
      kdDebug(5006) << "Index file " << indexLocation() << " is out of date. Re-creating it." << endl;
      createIndexFromContents();
      return FALSE;
  } else if(indexVersion > INDEX_VERSION) {
      kapp->setOverrideCursor(KCursor::arrowCursor());
      int r = KMessageBox::questionYesNo(0,
					 i18n(
					    "The mail index for '%1' is from an unknown version of KMail (%2).\n"
					    "This index can be regenerated from your mail folder, but some "
					    "information, including status flags, may be lost. Do you wish "
					    "to downgrade your index file?") .arg(name()) .arg(indexVersion) );
      kapp->restoreOverrideCursor();
      if (r == KMessageBox::Yes)
	  createIndexFromContents();
      return FALSE;
  }
  else {
      // Header
      Q_UINT32 byteOrder = 0;
      Q_UINT32 sizeOfLong = sizeof(long); // default

      Q_UINT32 header_length = 0;
      fseek(mIndexStream, sizeof(char), SEEK_CUR );
      fread(&header_length, sizeof(header_length), 1, mIndexStream);
      if (header_length > 0xFFFF)
         header_length = kmail_swap_32(header_length);
      
      long endOfHeader = ftell(mIndexStream) + header_length;

      bool needs_update = true;
      // Process available header parts
      if (header_length >= sizeof(byteOrder))
      {
         fread(&byteOrder, sizeof(byteOrder), 1, mIndexStream);
         mIndexSwapByteOrder = (byteOrder == 0x78563412);
         header_length -= sizeof(byteOrder);
      
         if (header_length >= sizeof(sizeOfLong))
         {
            fread(&sizeOfLong, sizeof(sizeOfLong), 1, mIndexStream);
            if (mIndexSwapByteOrder)
               sizeOfLong = kmail_swap_32(sizeOfLong);
            mIndexSizeOfLong = sizeOfLong;
            header_length -= sizeof(sizeOfLong);
            needs_update = false;
         }
      }
      if (needs_update || mIndexSwapByteOrder)
	mDirty = true;
      // Seek to end of header
      fseek(mIndexStream, endOfHeader, SEEK_SET );

      if (mIndexSwapByteOrder)
         kdDebug(5006) << "Index File has byte order swapped!" << endl;
      if (mIndexSizeOfLong != sizeof(long))
         kdDebug(5006) << "Index File sizeOfLong is " << mIndexSizeOfLong << " while sizeof(long) is " << sizeof(long) << " !" << endl;

  }
  return TRUE;
}


//-----------------------------------------------------------------------------
bool KMFolder::readIndex()
{
  Q_INT32 len;
  int offs;
  KMMsgInfo* mi;

  assert(mIndexStream != NULL);
  rewind(mIndexStream);

  mMsgList.clear();
  int version;

  mDirty = false;
  
  if (!readIndexHeader(&version)) return false;

  mUnreadMsgs = 0;
  mHeaderOffset = ftell(mIndexStream);

  mMsgList.clear();
  while (!feof(mIndexStream))
  {
    mi = NULL;
    if(version >= 1505) {
      if(!fread(&len, sizeof(len), 1, mIndexStream))
        break;

      if (mIndexSwapByteOrder)
        len = kmail_swap_32(len);

      offs = ftell(mIndexStream);
      if(fseek(mIndexStream, len, SEEK_CUR))
        break;
      mi = new KMMsgInfo(this, offs, len);
    } 
    else 
    {
      QCString line(MAX_LINE);
      fgets(line.data(), MAX_LINE, mIndexStream);
      if (feof(mIndexStream)) break;
      if (*line.data() == '\0') {
	  fclose(mIndexStream);
	  mIndexStream = NULL;
	  mMsgList.clear();
	  return false;
      }
      mi = new KMMsgInfo(this);
      mi->compat_fromOldIndexString(line, mConvertToUtf8);
    }	
    if(!mi)
      break;

    if (mi->status() == KMMsgStatusDeleted)
    {
      delete mi;  // skip messages that are marked as deleted
      mDirty = TRUE;
      needsCompact = true;  //We have deleted messages - needs to be compacted
      continue;
    }
#ifdef OBSOLETE
    else if (mi->status() == KMMsgStatusNew)
    {
      mi->setStatus(KMMsgStatusUnread);
      mi->setDirty(FALSE);
    }
#endif
    if ((mi->status() == KMMsgStatusNew) ||
	(mi->status() == KMMsgStatusUnread) ||
        (this == kernel->outboxFolder()))
    {
      ++mUnreadMsgs;
      if (mUnreadMsgs == 0) ++mUnreadMsgs;
    }
    mMsgList.append(mi, false);
  }
  if( version < 1505)
  {
    mConvertToUtf8 = FALSE;
    mDirty = TRUE;
    writeIndex();
  }
  return true;
}


//-----------------------------------------------------------------------------
void KMFolder::markNewAsUnread()
{
  KMMsgBase* msgBase;
  int i;

  for (i=0; i<mMsgList.high(); i++)
  {
    if (!(msgBase = mMsgList[i])) continue;
    if (msgBase->status() == KMMsgStatusNew)
    {
      msgBase->setStatus(KMMsgStatusUnread);
      msgBase->setDirty(TRUE);
    }
  }
}

void KMFolder::markUnreadAsRead()
{
  KMMsgBase* msgBase;
  QValueList<int> items;

  for (int i=count()-1; i>=0; --i)
  {
    msgBase = getMsgBase(i);
    assert(msgBase);
    if (msgBase->status() == KMMsgStatusNew || msgBase->status() == KMMsgStatusUnread)
    {
      items += i;
    }
  }

  if (items.count() > 0)
    setStatus(items, KMMsgStatusRead);
}

//-----------------------------------------------------------------------------
void KMFolder::quiet(bool beQuiet)
{
  if (beQuiet)
    mQuiet++;
  else {
    mQuiet--;
    if (mQuiet <= 0)
    {
      mQuiet = 0;
      if (mChanged)
	emit changed();
      mChanged = FALSE;
    }
  }
}


//-----------------------------------------------------------------------------

// Needed to use QSortedList in reduceSize()

/** Compare message's date. This is useful for message sorting */
int operator<( KMMsgBase & m1, KMMsgBase & m2 )
{
  return (m1.date() < m2.date());
}

/** Compare message's date. This is useful for message sorting */
int operator==( KMMsgBase & m1, KMMsgBase & m2 )
{
  return (m1.date() == m2.date());
}

//-----------------------------------------------------------------------------
int KMFolder::reduceSize( int aSize )
{
  kdDebug(5006) << "Reducing folder to size of " << aSize << " Mo" << endl;
  QSortedList<KMMsgBase> * slice=0L;
  QPtrList< QSortedList<KMMsgBase> > sliceArr;
  KMMsgBase* mb;
  ulong folderSize, msgSize, sliceSize, firstSliceSize, lastSliceSize, size;
  int sliceIndex;
  int delMsg = 0;
  int i;

  sliceArr.setAutoDelete( true );

  // I put each email in a slice according to its size (slices of 500Ko, 1Mo,
  // 2Mo, ... 10 Mo). Then I delete the oldest mail until the good size is
  // reached. 10 slices of 1Mo each is probably overkill. 500Ko, 1Mo, 5Mo
  // and 10Mo could be enough.


// Is it 1000 or 1024 ?
#define KILO 	(1000)

  size = KILO * KILO * aSize; // to have size in Ko;
  sliceSize = KILO * KILO ; // slice of 1 Mo
  lastSliceSize = 10 * sliceSize; // last slice is for item > 10 Mo
  firstSliceSize = sliceSize / 2; // first slice is for < 500 Ko
  folderSize = 0;

  for(i=0; i<12; i++) {
    sliceArr.append( new QSortedList<KMMsgBase> );
    sliceArr.at(i)->setAutoDelete(false);
  }

  for (i=count()-1; i>=0; i--) {
    mb = getMsgBase(i);
    assert(mb);
    msgSize = mb->msgSize();
    folderSize += msgSize;

    if (msgSize < firstSliceSize) {
      sliceIndex = 0;
    } else if (msgSize >= lastSliceSize) {
      sliceIndex = 11;
    } else {
      sliceIndex = 1 + (int) (msgSize / sliceSize); //  1 <= n < 10
    }

    sliceArr.at(sliceIndex)->append( mb );
  }

  //kdDebug(5006) << "Folder size : " << (folderSize/KILO) << " ko" << endl;

  // Ok, now we have our slices

  slice = sliceArr.last();
  while (folderSize > size) {
    //kdDebug(5006) << "Treating slice " << sliceArr.at()-1 << " Mo : " << slice->count() << endl;
    assert( slice );

    slice->sort();

    // Empty this slice taking the oldest mails first:
    while( slice->count() > 0 && folderSize > size ) {
      mb = slice->take(0);
      msgSize = mb->msgSize();
      //kdDebug(5006) << "deleting msg : " << (msgSize / KILO) << " ko - " << mb->subject() << " - " << mb->dateStr();
      assert( folderSize >= msgSize );
      folderSize -= msgSize;
      delMsg++;
      removeMsg(mb);
    }

    slice = sliceArr.prev();

  }

  return delMsg;
}



//-----------------------------------------------------------------------------
int KMFolder::expungeOldMsg(int days)
{
  int i, msgnb=0;
  time_t msgTime, maxTime;
  KMMsgBase* mb;
  QValueList<int> rmvMsgList;

  maxTime = time(0L) - days * 3600 * 24;

  for (i=count()-1; i>=0; i--) {
    mb = getMsgBase(i);
    assert(mb);
    msgTime = mb->date();

    if (msgTime < maxTime) {
      //kdDebug(5006) << "deleting msg " << i << " : " << mb->subject() << " - " << mb->dateStr(); // << endl;
      removeMsg( i );
      msgnb++;
    }
  }
  return msgnb;
}


//-----------------------------------------------------------------------------
/**
 * Return the number of days given some value, and the units for that
 * value. Currently, supported units are days, weeks and months.
 */
int
KMFolder::daysToExpire(int number, ExpireUnits units) {
  switch (units) {
  case expireDays: // Days
    return number;
  case expireWeeks: // Weeks
    return number * 7;
  case expireMonths: // Months - this could be better rather than assuming 31day months.
    return number * 31;
  default: // this avoids a compiler warning (not handled enumeration values)
    ;
  }

  return -1;

}

//-----------------------------------------------------------------------------
/**
 * Expire old messages from this folder. Read and unread messages have
 * different expiry times. An expiry time of 0 or less is considered to
 * mean no-expiry. Also check the general 'expire' flag as well.
 */
void KMFolder::expireOldMessages() {
  int             days = 0;
  int             maxUnreadTime = 0;
  int             maxReadTime = 0;
  KMMsgBase       *mb = NULL;
  QValueList<int> rmvMsgList;
  int             i = 0;
  time_t          msgTime, maxTime = 0;

  kdDebug(5006) << "expireOldMessages " << name() << endl;

  if (protocol() == "imap") {
    return;
  }

  days = daysToExpire(getUnreadExpireAge(), getUnreadExpireUnits());
  if (days > 0) {
    kdDebug(5006) << "deleting unread older than "<< days << " days" << endl;
    maxUnreadTime = time(0L) - days * 3600 * 24;
  }

  days = daysToExpire(getReadExpireAge(), getReadExpireUnits());
  if (days > 0) {
    kdDebug(5006) << "deleting read older than "<< days << " days" << endl;
    maxReadTime = time(0L) - days * 3600 * 24;
  }

  if ((maxUnreadTime == 0) && (maxReadTime == 0)) {
    return;
  }
  open();
  for (i=mMsgList.count()-1; i>=0; i--) {
    mb = getMsgBase(i);
    if (mb == NULL) {
	  continue;
    }
    msgTime = mb->date();

	if (mb->isUnread()) {
	  maxTime = maxUnreadTime;
	} else {
	  maxTime = maxReadTime;
	}

    if (msgTime < maxTime) {
      removeMsg( i );
    }
  }
  close();
  
  return;
}




//-----------------------------------------------------------------------------
bool KMFolder::canAddMsgNow(KMMessage* aMsg, int* aIndex_ret)
{
  if (aIndex_ret) *aIndex_ret = -1;
  KMFolder *msgParent = aMsg->parent();
  if (msgParent && msgParent->protocol() == "imap" && !aMsg->isComplete())
  {
    KMImapJob *imapJob = new KMImapJob(aMsg);
    connect(imapJob, SIGNAL(messageRetrieved(KMMessage*)),
      SLOT(reallyAddMsg(KMMessage*)));
    aMsg->setTransferInProgress(TRUE);
    kernel->filterMgr()->tempOpenFolder(this);
    return FALSE;
  }
  return TRUE;
}


//-----------------------------------------------------------------------------
void KMFolder::reallyAddMsg(KMMessage* aMsg)
{
  KMFolder *folder = aMsg->parent();
  int index;
  ulong serNum = aMsg->getMsgSerNum();
  addMsg(aMsg, &index);
  if (index < 0) return;
  unGetMsg(index);
  kernel->undoStack()->pushAction( serNum, folder, this );
}


//-----------------------------------------------------------------------------
void KMFolder::reallyAddCopyOfMsg(KMMessage* aMsg)
{
  aMsg->setParent( NULL );
  addMsg( aMsg );
  unGetMsg( count() - 1 );
}


//-----------------------------------------------------------------------------
void KMFolder::removeMsg(KMMsgBasePtr aMsg)
{
  int idx = find(aMsg);
  assert( idx != -1);
  removeMsg(idx);
}

void KMFolder::removeMsg(QPtrList<KMMessage> msgList, bool imapQuiet)
{
  for ( KMMessage* msg = msgList.first(); msg; msg = msgList.next() )
  {
    int idx = find(msg);
    assert( idx != -1);
    removeMsg(idx, imapQuiet);
  }
}

//-----------------------------------------------------------------------------
void KMFolder::removeMsg(int idx, bool)
{
  //assert(idx>=0);
  if(idx < 0)
  {
    kdDebug(5006) << "KMFolder::removeMsg() : idx < 0\n" << endl;
    return;
  }
  KMMsgBase* mb = mMsgList[idx];
  QString msgIdMD5 = mb->msgIdMD5();
  mb = mMsgList.take(idx);

  mDirty = TRUE;
  needsCompact=true; // message is taken from here - needs to be compacted

  if (mb->status()==KMMsgStatusUnread ||
      mb->status()==KMMsgStatusNew || 
      (this == kernel->outboxFolder())) {
    --mUnreadMsgs;
    emit numUnreadMsgsChanged( this );
  }

  if (!mQuiet)
    emit msgRemoved(idx, msgIdMD5);
  else
    mChanged = TRUE;
}


//-----------------------------------------------------------------------------
KMMessage* KMFolder::take(int idx)
{
  KMMsgBase* mb;
  KMMessage* msg;

  assert(idx>=0 && idx<=mMsgList.high());

  mb = mMsgList[idx];
  if (!mb) return NULL;
  if (!mb->isMessage()) readMsg(idx);

  QString msgIdMD5 = mMsgList[idx]->msgIdMD5();
  msg = (KMMessage*)mMsgList.take(idx);
  if (msg->status()==KMMsgStatusUnread ||
      msg->status()==KMMsgStatusNew || 
      (this == kernel->outboxFolder())) {
    --mUnreadMsgs;
    emit numUnreadMsgsChanged( this );
  }
  msg->setParent(NULL);
  mDirty = TRUE;
  needsCompact=true; // message is taken from here - needs to be compacted
  if (!mQuiet)
    emit msgRemoved(idx,msgIdMD5);
  else
    mChanged = TRUE;

  return msg;
}

void KMFolder::take(QPtrList<KMMessage> msgList)
{
  for ( KMMessage* msg = msgList.first(); msg; msg = msgList.next() )
  {
    if (msg->parent())
    {
      int idx = msg->parent()->find(msg);
      assert( idx != -1);
      KMFolder::take(idx);
    }
  }
}


//-----------------------------------------------------------------------------
KMMessage* KMFolder::getMsg(int idx)
{
  KMMsgBase* mb;

  // assert(idx>=0 && idx<=mMsgList.high());
  if(!(idx >= 0 && idx <= mMsgList.high()))
    return 0L;

  mb = mMsgList[idx];
  if (!mb) return NULL;

#if 0
  if (mb->isMessage()) return ((KMMessage*)mb);
  return readMsg(idx);
#else
  KMMessage *msg = 0;
  if (mb->isMessage()) {
      msg = ((KMMessage*)mb);
  } else {
      QString mbSubject = mb->subject();
      msg = readMsg(idx);
      // sanity check
      if (mCompactable && (!msg || (msg->subject().isEmpty() != mbSubject.isEmpty()))) {
	  kdDebug(5006) << "Error: " << location() <<
	  " Index file is inconsistent with folder file. This should never happen." << endl;
	  mCompactable = FALSE; // Don't compact
	  writeConfig();
      }
  }
  
  if (msg->getMsgSerNum() == 0) {
    msg->setMsgSerNum(kernel->msgDict()->insert(0, msg, idx));
    kdDebug(5006) << "Serial number generated for message in folder " << label() << endl;
  }
  
  return msg;
#endif


}


//-----------------------------------------------------------------------------
KMMsgInfo* KMFolder::unGetMsg(int idx)
{
  KMMsgBase* mb;

  if(!(idx >= 0 && idx <= mMsgList.high()))
    return 0L;

  mb = mMsgList[idx];
  if (!mb) return NULL;

  if (mb->isMessage()) {
    KMMsgInfo *msgInfo = new KMMsgInfo( this );
    *msgInfo = *((KMMessage*)mb);
    mMsgList.set( idx, msgInfo );
    return msgInfo;
  }

  return 0L;
}


//-----------------------------------------------------------------------------
bool KMFolder::isMessage(int idx)
{
  KMMsgBase* mb;
  if (!(idx >= 0 && idx <= mMsgList.high())) return FALSE;
  mb = mMsgList[idx];
  return (mb && mb->isMessage());
}


//-----------------------------------------------------------------------------
int KMFolder::moveMsg(KMMessage* aMsg, int* aIndex_ret)
{
  KMFolder* msgParent;
  int rc;

  assert(aMsg != NULL);
  msgParent = aMsg->parent();

  if (msgParent)
    msgParent->open();

  open();
  rc = addMsg(aMsg, aIndex_ret);
  close();

  if (msgParent)
    msgParent->close();

  return rc;
}

//-----------------------------------------------------------------------------
int KMFolder::moveMsg(QPtrList<KMMessage> msglist, int* aIndex_ret)
{
  KMFolder* msgParent;
  int rc;

  KMMessage* aMsg = msglist.first();
  assert(aMsg != NULL);
  msgParent = aMsg->parent();

  if (msgParent)
    msgParent->open();

  open();
  rc = static_cast<KMFolderImap*>(this)->addMsg(msglist, aIndex_ret);
  close();

  if (msgParent)
    msgParent->close();

  return rc;
}


//-----------------------------------------------------------------------------
int KMFolder::find(const QString& msgIdMD5) const
{
  for (int i=0; i<mMsgList.high(); ++i)
    if (mMsgList[i]->msgIdMD5() == msgIdMD5)
      return i;


  return -1;
}


//-----------------------------------------------------------------------------
int KMFolder::rename(const QString& aName, KMFolderDir *aParent)
{
  QString oldLoc, oldIndexLoc, newLoc, newIndexLoc;
  QString oldSubDirLoc, newSubDirLoc;
  QString oldName;
  int rc=0, openCount=mOpenCount;
  KMFolderDir *oldParent;

  assert(!aName.isEmpty());

  oldLoc = location();
  oldIndexLoc = indexLocation();
  oldSubDirLoc = subdirLocation();

  close(TRUE);

  oldName = fileName();
  oldParent = parent();
  if (aParent)
    setParent( aParent );

  setName(aName);
  newLoc = location();
  newIndexLoc = indexLocation();
  newSubDirLoc = subdirLocation();

  if (_rename(oldLoc.local8Bit(), newLoc.local8Bit())) {
    setName(oldName);
    setParent(oldParent);
    rc = errno;
  }
  else if (!oldIndexLoc.isEmpty()) {
    _rename(oldIndexLoc.local8Bit(), newIndexLoc.local8Bit());
    if (!_rename(oldSubDirLoc.local8Bit(), newSubDirLoc.local8Bit() )) {
      KMFolderDir* fdir = parent();
      KMFolderNode* fN;

      for (fN = fdir->first(); fN != 0; fN = fdir->next())
	if (fN->name() == "." + oldName + ".directory" ) {
	  fN->setName( "." + fileName().local8Bit() + ".directory" );
	  break;
	}
    }
  }

  if (aParent) {
    if (oldParent->findRef( this ) != -1)
      oldParent->take();
    aParent->inSort( this );
    if (mChild) {
      if (mChild->parent()->findRef( mChild ) != -1)
	mChild->parent()->take();
      aParent->inSort( mChild );
    }
  }

  if (openCount > 0)
  {
    open();
    mOpenCount = openCount;
  }

  return rc;
}


//-----------------------------------------------------------------------------
int KMFolder::remove()
{
  assert(name() != "");

  mMsgList.clear(true, true);   // delete and remove from dict
  close(TRUE);

  if (kernel->msgDict()) kernel->msgDict()->removeFolderIds(this);
  unlink(indexLocation().local8Bit() + ".sorted");
  unlink(indexLocation().local8Bit());
  
  int rc = removeContents();
  if (rc) return rc;

  needsCompact = false; //we are dead - no need to compact us
  return 0;
}


//-----------------------------------------------------------------------------
int KMFolder::expunge()
{
  int openCount = mOpenCount;

  assert(name() != "");

  mMsgList.clear(true, true);   // delete and remove from dict
  close(TRUE);

  kernel->msgDict()->removeFolderIds(this);
  if (mAutoCreateIndex) truncate(indexLocation().local8Bit(), mHeaderOffset);
  else unlink(indexLocation().local8Bit());
  
  int rc = expungeContents();
  if (rc) return rc;
  
  mDirty = FALSE;
  needsCompact = false; //we're cleared and truncated no need to compact

  if (openCount > 0)
  {
    open();
    mOpenCount = openCount;
  }

  mUnreadMsgs = 0;
  emit numUnreadMsgsChanged( this );
  if (mAutoCreateIndex)
    writeConfig();
  if (!mQuiet)
    emit changed();
  else
    mChanged = TRUE;
  return 0;
}


//-----------------------------------------------------------------------------
const char* KMFolder::type() const
{
  if (mAcctList) return "In";
  return KMFolderInherited::type();
}


//-----------------------------------------------------------------------------
QString KMFolder::label() const
{
  if (mIsSystemFolder && !mLabel.isEmpty()) return mLabel;
  if (mIsSystemFolder) return i18n(name().latin1());
  return name();
}


//-----------------------------------------------------------------------------
int KMFolder::countUnread()
{
  if (mGuessedUnreadMsgs > -1)
    return mGuessedUnreadMsgs;
  if (mUnreadMsgs > -1)
    return mUnreadMsgs;

  readConfig();

  if (mUnreadMsgs > -1)
    return mUnreadMsgs;

  open(); // will update unreadMsgs
  close();
  return (mUnreadMsgs > 0) ? mUnreadMsgs : 0;
}

//-----------------------------------------------------------------------------
int KMFolder::countUnreadRecursive()
{
  KMFolder *folder;
  int count = countUnread();
  KMFolderDir *dir = child();
  if (!dir)
    return count;

  QPtrListIterator<KMFolderNode> it(*dir);
  for ( ; it.current(); ++it )
    if (!it.current()->isDir()) {
      folder = static_cast<KMFolder*>(it.current());
      count += folder->countUnreadRecursive();
    }

  return count;
}

//-----------------------------------------------------------------------------
void KMFolder::msgStatusChanged(const KMMsgStatus oldStatus,
  const KMMsgStatus newStatus)
{
  int oldUnread = 0;
  int newUnread = 0;

  if (oldStatus==KMMsgStatusUnread || oldStatus==KMMsgStatusNew ||
      (this == kernel->outboxFolder()))
    oldUnread = 1;
  if (newStatus==KMMsgStatusUnread || newStatus==KMMsgStatusNew ||
      (this == kernel->outboxFolder()))
    newUnread = 1;
  int deltaUnread = newUnread - oldUnread;

  if (deltaUnread != 0) {
    if (mUnreadMsgs < 0) mUnreadMsgs = 0;
    mUnreadMsgs += deltaUnread;
    emit numUnreadMsgsChanged( this );
  }
}

//-----------------------------------------------------------------------------
void KMFolder::headerOfMsgChanged(const KMMsgBase* aMsg, int idx)
{
  if (mQuiet)
  {
    mChanged = TRUE;
    return;
  }
  if (idx < 0)
    idx = mMsgList.find((KMMsgBasePtr)aMsg);
  if (idx >= 0)
    emit msgHeaderChanged(idx);
  else
    mChanged = TRUE;
}


//-----------------------------------------------------------------------------
QString KMFolder::idString()
{
  KMFolderNode* folderNode = parent();
  if (!folderNode)
    return "";
  while (folderNode->parent())
    folderNode = folderNode->parent();
  int pathLen = path().length() - folderNode->path().length();
  QString relativePath = path().right( pathLen );
  if (!relativePath.isEmpty())
    relativePath = relativePath.right( relativePath.length() - 1 ) + "/";
  return relativePath + QString(name());
}
//-----------------------------------------------------------------------------
void KMFolder::readConfig()
{
  KConfig* config = kapp->config();
  KConfigGroupSaver saver(config, "Folder-" + idString());
  if (mUnreadMsgs == -1)
    mUnreadMsgs = config->readNumEntry("UnreadMsgs", -1);
  mMailingListEnabled = config->readBoolEntry("MailingListEnabled");
  mMailingListPostingAddress = config->readEntry("MailingListPostingAddress");
  mMailingListAdminAddress = config->readEntry("MailingListAdminAddress");
  mIdentity = config->readEntry("Identity");
  if ( mIdentity.isEmpty() ) // backward compatiblity
      mIdentity = config->readEntry("MailingListIdentity");
  mCompactable = config->readBoolEntry("Compactable", TRUE);

  expireMessages = config->readBoolEntry("ExpireMessages", FALSE);
  readExpireAge = config->readNumEntry("ReadExpireAge", 3);
  readExpireUnits = (ExpireUnits)config->readNumEntry("ReadExpireUnits", expireMonths);
  unreadExpireAge = config->readNumEntry("UnreadExpireAge", 12);
  unreadExpireUnits = (ExpireUnits)config->readNumEntry("UnreadExpireUnits", expireNever);
	setUserWhoField( config->readEntry("WhoField") );
}

//-----------------------------------------------------------------------------
void KMFolder::writeConfig()
{
  KConfig* config = kapp->config();
  KConfigGroupSaver saver(config, "Folder-" + idString());
  config->writeEntry("UnreadMsgs", mUnreadMsgs);
  config->writeEntry("MailingListEnabled", mMailingListEnabled);
  config->writeEntry("MailingListPostingAddress", mMailingListPostingAddress);
  config->writeEntry("MailingListAdminAddress", mMailingListAdminAddress);
  config->writeEntry("Identity", mIdentity);
  config->writeEntry("Compactable", mCompactable);
  config->writeEntry("ExpireMessages", expireMessages);
  config->writeEntry("ReadExpireAge", readExpireAge);
  config->writeEntry("ReadExpireUnits", readExpireUnits);
  config->writeEntry("UnreadExpireAge", unreadExpireAge);
  config->writeEntry("UnreadExpireUnits", unreadExpireUnits);
	config->writeEntry("WhoField", mUserWhoField);
}

//-----------------------------------------------------------------------------
void KMFolder::correctUnreadMsgsCount()
{
  open();
  close();
  emit numUnreadMsgsChanged( this );
}

#ifdef HAVE_MMAP
bool KMFolder::updateIndexStreamPtr(bool just_close)
#else
bool KMFolder::updateIndexStreamPtr(bool)
#endif
{
  // We touch the folder, otherwise the index is regenerated, if KMail is
  // running, while the clock switches from daylight savings time to normal time
  KProcess p, q;
  p << "touch" << location();
  p.start(KProcess::Block);
  q << "touch" << indexLocation();
  q.start(KProcess::Block);

  mIndexSwapByteOrder = false;
#ifdef HAVE_MMAP
    if(just_close) {
	if(mIndexStreamPtr)
	    munmap(mIndexStreamPtr, mIndexStreamPtrLength);
	mIndexStreamPtr = NULL;
	mIndexStreamPtrLength = 0;
	return TRUE;
    }

    assert(mIndexStream);
    struct stat stat_buf;
    if(fstat(fileno(mIndexStream), &stat_buf) == -1) {
	if(mIndexStreamPtr)
	    munmap(mIndexStreamPtr, mIndexStreamPtrLength);
	mIndexStreamPtr = NULL;
	mIndexStreamPtrLength = 0;
	return FALSE;
    }
    if(mIndexStreamPtr)
	munmap(mIndexStreamPtr, mIndexStreamPtrLength);
    mIndexStreamPtrLength = stat_buf.st_size;
    mIndexStreamPtr = (uchar *)mmap(0, mIndexStreamPtrLength, PROT_READ, MAP_SHARED,
				    fileno(mIndexStream), 0);
    if(mIndexStreamPtr == MAP_FAILED) {
	mIndexStreamPtr = NULL;
	mIndexStreamPtrLength = 0;
	return FALSE;
    }
#endif
    return TRUE;
}

//-----------------------------------------------------------------------------
void KMFolder::fillMsgDict(KMMsgDict *dict)
{
  open();
  mMsgList.fillMsgDict(dict);
  close();
}

//-----------------------------------------------------------------------------
int KMFolder::writeMsgDict(KMMsgDict *dict)
{
  int ret = 0;
  if (!dict)
    dict = kernel->msgDict();
  if (dict)
    ret = dict->writeFolderIds(this);
  return ret;
}

//-----------------------------------------------------------------------------
int KMFolder::touchMsgDict()
{
  int ret = 0;
  KMMsgDict *dict = kernel->msgDict();
  if (dict)
    ret = dict->touchFolderIds(this);
  return ret;
}

//-----------------------------------------------------------------------------
int KMFolder::appendtoMsgDict(int idx)
{
  int ret = 0;
  KMMsgDict *dict = kernel->msgDict();
  if (dict)
    ret = dict->appendtoFolderIds(this, idx);
  return ret;
}

//-----------------------------------------------------------------------------
void KMFolder::setStatus(int idx, KMMsgStatus status)
{
  KMMsgBase *msg = getMsgBase(idx);
  msg->setStatus(status, idx);
}

void KMFolder::setRDict(KMMsgDictREntry *rentry) {
  if (rentry == mRDict)
	return;
  KMMsgDict::deleteRentry(mRDict);
  mRDict = rentry; 
}

//-----------------------------------------------------------------------------
void KMFolder::setStatus(QValueList<int>& ids, KMMsgStatus status)
{
  for ( QValueList<int>::Iterator it = ids.begin(); it != ids.end(); ++it )
  {
    KMFolder::setStatus(*it, status);
  }  
}

//-----------------------------------------------------------------------------
void KMFolder::setStatus(KMMsgBase *msg, KMMsgStatus status)
{
  KMMsgDict *dict = kernel->msgDict();
  if (dict) {
    KMFolder *folder;
    int index;
    dict->getLocation(msg->getMsgSerNum(), &folder, &index);
    if (folder == this && folder == msg->parent())
      setStatus(index, status);
  }
}

//-----------------------------------------------------------------------------
void KMFolder::setUserWhoField(const QString &whoField)
{
  mUserWhoField = whoField;
  if ( whoField.isEmpty() )
  {
    // default setting
    QString ident = mIdentity; if (ident.isEmpty()) ident = i18n("Default");
    KMIdentity identity (ident);
    identity.readConfig();

    if ( mIsSystemFolder && protocol() != "imap" ) 
    {	
      // local system folders
      if ( this == kernel->inboxFolder() || this == kernel->trashFolder() ) mWhoField = "From";
      if ( this == kernel->outboxFolder() || this == kernel->sentFolder() || this == kernel->draftsFolder() ) mWhoField = "To";

    } else if ( identity.drafts() == idString() || identity.fcc() == idString() ) {
      // drafts or sent of the identity
      mWhoField = "To";
    } else { 
      mWhoField = "From";
    }	

  } else if ( whoField == "From" || whoField == "To" ) {

    // set the whoField according to the user-setting
    mWhoField = whoField;

  } else {
    // this should not happen...
    kdDebug(5006) << "Illegal setting " << whoField << " for userWhoField!" << endl;
  }

  writeConfig();
}

//-----------------------------------------------------------------------------

#include "kmfolder.moc"
