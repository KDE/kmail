// kmfolder.cpp
// Author: Stefan Taferner <taferner@alpin.or.at>

#include <qfileinfo.h>
#include <qsortedlist.h>

#include "kmglobal.h"
#include "kmfolder.h"
#include "kmmessage.h"
#include "kmfolderdir.h"
#include "kbusyptr.h"

#include <kapp.h>
#include <kconfig.h>
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
  needsCompact    = FALSE;
  mChild          = 0;
  mConvertToUtf8  = FALSE;
  mMailingListEnabled = FALSE;
  mIndexId        = -1;
  mIndexStreamPtr = NULL;
  mIndexStreamPtrLength = 0;
  mConsistent     = TRUE;
}


//-----------------------------------------------------------------------------
KMFolder :: ~KMFolder()
{
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
  sLocation += dotEscape(name());

  return sLocation;
}


//-----------------------------------------------------------------------------
QString KMFolder::indexLocation() const
{
  QString sLocation(path());

  if (!sLocation.isEmpty()) sLocation += '/';
  sLocation += '.';
  sLocation += dotEscape(name());
  sLocation += ".index";

  return sLocation;
}

//-----------------------------------------------------------------------------
QString KMFolder::subdirLocation() const
{
  QString sLocation(path());

  if (!sLocation.isEmpty()) sLocation += '/';
  sLocation += '.';
  sLocation += dotEscape(name());
  sLocation += ".directory";

  return sLocation;
}

//-----------------------------------------------------------------------------
KMFolderDir* KMFolder::createChildFolder()
{
  QString childName = "." + name() + ".directory";
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

  KMFolderDir* folderDir = new KMFolderDir(parent(), childName);
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

  FILE *tmpIndexStream = fopen(tempName.local8Bit(), "w");
  umask(old_umask);
  if (!tmpIndexStream)
    return errno;

  fprintf(tmpIndexStream, "# KMail-Index V%d\n", INDEX_VERSION);
  char pad_char = '\0';
  int header_length = 0; // Reserved for future expansion
  fwrite(&pad_char, sizeof(pad_char), 1, tmpIndexStream);
  fwrite(&header_length, sizeof(header_length), 1, tmpIndexStream);

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
      fseek(mIndexStream, sizeof(char), SEEK_CUR );
  } else if (indexVersion < INDEX_VERSION) {
      kdDebug(5006) << "Index file " << indexLocation() << " is out of date. Re-creating it." << endl;
      createIndexFromContents();
      return FALSE;
  } else if(indexVersion > INDEX_VERSION) {
      kapp->setOverrideCursor(KCursor::arrowCursor());
      int r = KMessageBox::questionYesNo(0,
					 i18n(
					    "The mail index for '%1' is from an unknown version of KMail (%2).\n"
					    "This index can be regenerated from your mail folder, but some\n"
					    "information, including status flags, may be lost. Do you wish\n"
					    "to downgrade your index file ?") .arg(name()) .arg(indexVersion) );
      kapp->restoreOverrideCursor();
      if (r == KMessageBox::Yes)
	  createIndexFromContents();
      return FALSE;
  }
  else {
      int header_length = 0;
      fseek(mIndexStream, sizeof(char), SEEK_CUR );
      fread(&header_length, sizeof(header_length), 1, mIndexStream);
      fseek(mIndexStream, header_length, SEEK_CUR );
  }
  return TRUE;
}


//-----------------------------------------------------------------------------
bool KMFolder::readIndex()
{
  int len, offs;
  KMMsgInfo* mi;

  assert(mIndexStream != NULL);
  rewind(mIndexStream);

  mMsgList.clear();
  int version;
  if (!readIndexHeader(&version)) return false;

  mUnreadMsgs = 0;
  mDirty = FALSE;
  mHeaderOffset = ftell(mIndexStream);

  mMsgList.clear();
  while (!feof(mIndexStream))
  {
    mi = NULL;
    if(version >= 1505) {
      if(!fread(&len, sizeof(len), 1, mIndexStream))
	break;
      offs = ftell(mIndexStream);
      if(fseek(mIndexStream, len, SEEK_CUR))
	break;
      mi = new KMMsgInfo(this, offs, len);
    } else {
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
	(mi->status() == KMMsgStatusUnread))
    {
      ++mUnreadMsgs;
      if (mUnreadMsgs == 0) ++mUnreadMsgs;
    }
    mMsgList.append(mi);
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
  QList< QSortedList<KMMsgBase> > sliceArr;
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
void KMFolder::removeMsg(KMMsgBasePtr aMsg)
{
  int idx = find(aMsg);
  assert( idx != -1);
  removeMsg(idx);
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
      mb->status()==KMMsgStatusNew) {
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
      msg->status()==KMMsgStatusNew) {
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
      time_t mbDate = mb->date();
      msg = readMsg(idx);
      // sanity check
      if (mConsistent && (!msg || (msg->date() != mbDate) || (msg->subject() != mbSubject))) {
	  kdDebug(5006) << "Error: " << location() <<
	  " Index file is inconsistent with folder file. This should never happen." << endl;
	  mConsistent = FALSE; // Don't compact
	  writeConfig();
      }
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

  oldName = name();
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
	  fN->setName( "." + name().local8Bit() + ".directory" );
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
  int rc;

  assert(name() != "");

  close(TRUE);
  unlink(indexLocation().local8Bit() + ".sorted");
  unlink(indexLocation().local8Bit());
  rc = unlink(location().local8Bit());
  if (rc) return rc;

  mMsgList.reset(INIT_MSGS);
  needsCompact = false; //we are dead - no need to compact us
  return 0;
}


//-----------------------------------------------------------------------------
int KMFolder::expunge()
{
  int openCount = mOpenCount;

  assert(name() != "");

  close(TRUE);

  if (mAutoCreateIndex) truncate(indexLocation().local8Bit(), mHeaderOffset);
  else unlink(indexLocation().local8Bit());

  if (truncate(location().local8Bit(), 0)) return errno;
  mDirty = FALSE;

  mMsgList.reset(INIT_MSGS);
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

  QListIterator<KMFolderNode> it(*dir);
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

  if (oldStatus==KMMsgStatusUnread || oldStatus==KMMsgStatusNew)
    oldUnread = 1;
  if (newStatus==KMMsgStatusUnread || newStatus==KMMsgStatusNew)
    newUnread = 1;
  int deltaUnread = newUnread - oldUnread;

  if (deltaUnread != 0) {
    if (mUnreadMsgs < 0) mUnreadMsgs = 0;
    mUnreadMsgs += deltaUnread;
    emit numUnreadMsgsChanged( this );
  }
}

//-----------------------------------------------------------------------------
void KMFolder::headerOfMsgChanged(const KMMsgBase* aMsg)
{
  int idx = mMsgList.find((KMMsgBasePtr)aMsg);
  if (idx >= 0 && !mQuiet)
    emit msgHeaderChanged(idx);
  else
    mChanged = TRUE;
}


//-----------------------------------------------------------------------------
const char* KMFolder::whoField() const
{
  return (mWhoField.isEmpty() ? "From" : mWhoField.latin1());
}


//-----------------------------------------------------------------------------
void KMFolder::setWhoField(const QString& aWhoField)
{
    mWhoField = aWhoField;
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
  mConsistent = config->readBoolEntry("Consistent", TRUE);
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
  config->writeEntry("Consistent", mConsistent);
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

#include "kmfolder.moc"


