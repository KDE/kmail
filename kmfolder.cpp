// kmfolder.cpp
// Author: Stefan Taferner <taferner@alpin.or.at>

#include <qfileinfo.h>

#include "kmglobal.h"
#include "kmfolder.h"
#include "kmmessage.h"
#include "kmfolderdir.h"
#include "kbusyptr.h"
#include "kmundostack.h"

#include <kapp.h>
#include <kconfig.h>
#include <mimelib/mimepp.h>
#include <qregexp.h>
#include <kmessagebox.h>
#include <kdebug.h>

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

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <klocale.h>

#ifndef isblank
#  define isblank(x) ((x)==' '||(x)=='\t')
#endif

#define MAX_LINE 4096
#define INIT_MSGS 8

// Current version of the table of contents (index) files
#define INDEX_VERSION 1504

// Regular expression to find the line that seperates messages in a mail
// folder:
#define MSG_SEPERATOR_START "From "
#define MSG_SEPERATOR_REGEX "^From .*..:...*$"
static short msgSepLen = strlen(MSG_SEPERATOR_START);

static int _rename(const char* oldname, const char* newname)
{
  return rename(oldname, newname);
}


//-----------------------------------------------------------------------------
KMFolder :: KMFolder(KMFolderDir* aParent, const QString& aName) :
  KMFolderInherited(aParent, aName), mMsgList(INIT_MSGS)
{
  //-- in case that the compiler has problems with the static version above:
  //msgSepLen = strlen(MSG_SEPERATOR_START);

  initMetaObject();

  mStream         = NULL;
  mIndexStream    = NULL;
  mOpenCount      = 0;
  mQuiet          = 0;
  mChanged        = FALSE;
  mHeaderOffset   = 0;
  mAutoCreateIndex= TRUE;
  mFilesLocked    = FALSE;
  mIsSystemFolder = FALSE;
  mType           = "plain";
  mAcctList       = NULL;
  mDirty          = FALSE;
  mUnreadMsgs      = -1;
  needsCompact    = FALSE;
  mChild          = 0;
  mLockType       = None;
  mConvertToUtf8  = FALSE;
}


//-----------------------------------------------------------------------------
KMFolder :: ~KMFolder()
{
  if (mOpenCount>0) close(TRUE);

  //if (mAcctList) delete mAcctList;
  /* Well, this is a problem. If I add the above line then kmfolder depends
   * on kmaccount and is then not that portable. Hmm.
   */
  if (kernel->undoStack()) kernel->undoStack()->folderDestroyed(this);
}


//-----------------------------------------------------------------------------
const QString KMFolder::location() const
{
  QString sLocation(path().local8Bit());

  if (!sLocation.isEmpty()) sLocation += '/';
  sLocation += name().local8Bit();

  return sLocation;
}


//-----------------------------------------------------------------------------
const QString KMFolder::indexLocation() const
{
  QString sLocation(path().local8Bit());

  if (!sLocation.isEmpty()) sLocation += '/';
  sLocation += '.';
  sLocation += name().local8Bit();
  sLocation += ".index";

  return sLocation;
}

//-----------------------------------------------------------------------------
const QString KMFolder::subdirLocation() const
{
  QString sLocation(path().local8Bit());

  if (!sLocation.isEmpty()) sLocation += '/';
  sLocation += '.';
  sLocation += name().local8Bit();
  sLocation += ".directory";

  return sLocation;
}

//-----------------------------------------------------------------------------
KMFolderDir* KMFolder::createChildFolder()
{
  QCString childName = "." + name().local8Bit() + ".directory";
  QCString childDir = path().local8Bit() + "/" + childName;
  bool ok = true;

  if (mChild)
    return mChild;

  if (access(childDir, W_OK) != 0) // Not there or not writable
  {
    if (mkdir(childDir, S_IRWXU) != 0 && chmod(childDir, S_IRWXU) != 0)
      ok=false; //failed create new or chmod existing tmp/
  }

  if (!ok) {
    QString wmsg = QString(" '%1': %2").arg(childDir).arg(strerror(errno));
    KMessageBox::information(0,i18n("Failed to create directory") + wmsg);
    return 0;
  }

  KMFolderDir* folderDir = new KMFolderDir(parent(),
    QString::fromLocal8Bit(childName));
  if (!folderDir)
    return 0;
  folderDir->reload();
  parent()->append(folderDir);
  mChild = folderDir;
  return folderDir;
}

//-----------------------------------------------------------------------------
int KMFolder::open()
{
  int rc = 0;

  mOpenCount++;
  if (mOpenCount > 1) return 0;  // already open

  assert(name() != "");

  mFilesLocked = FALSE;
  mStream = fopen(location(), "r+"); // messages file
  if (!mStream)
  {
    kdDebug() << "Cannot open folder `" << (const char*)location() << "': " << strerror(errno) << endl;
    mOpenCount = 0;
    return errno;
  }

  if (!path().isEmpty())
  {
    if (isIndexOutdated()) // test if contents file has changed
    {
      QString str;
      mIndexStream = NULL;
      str = i18n("Folder `%1' changed. Recreating index.")
		  .arg(name());
      emit statusMsg(str);
    }
    else mIndexStream = fopen(indexLocation(), "r+"); // index file

    if (!mIndexStream) rc = createIndexFromContents();
    else readIndex();
  }
  else
  {
    mAutoCreateIndex = FALSE;
    rc = createIndexFromContents();
  }

  if (!rc) lock();
  mQuiet = 0;
  mChanged = FALSE;

  return rc;
}


//-----------------------------------------------------------------------------
int KMFolder::create()
{
  int rc;
  int old_umask;

  assert(name() != "");
  assert(mOpenCount == 0);

  kdDebug() << "Creating folder " << endl;
  if (access(location(), F_OK) == 0) {
    kdDebug() << "KMFolder::create call to access function failed." << endl;
    kdDebug() << "File:: " << endl;
    kdDebug() << "Error " << endl;
    return EEXIST;
  }

  old_umask = umask(077);
  mStream = fopen(location(), "w+"); //sven; open RW
  umask(old_umask);

  if (!mStream) return errno;

  if (!path().isEmpty())
  {
    old_umask = umask(077);
    mIndexStream = fopen(indexLocation(), "w+"); //sven; open RW
    umask(old_umask);

    if (!mIndexStream) return errno;
  }
  else
  {
    mAutoCreateIndex = FALSE;
  }

  mOpenCount++;
  mQuiet = 0;
  mChanged = FALSE;

  rc = writeIndex();
  if (!rc) lock();
  return rc;
}


//-----------------------------------------------------------------------------
void KMFolder::close(bool aForced)
{
  if (mOpenCount <= 0 || !mStream) return;
  if (mOpenCount > 0) mOpenCount--;
  if (mOpenCount > 0 && !aForced) return;

  if (mAutoCreateIndex)
  {
    if (mDirty) writeIndex();
    else sync();
    writeConfig();
  }

  unlock();
  mMsgList.clear(TRUE);

  if (mStream) fclose(mStream);
  if (mIndexStream) fclose(mIndexStream);

  mOpenCount   = 0;
  mStream      = NULL;
  mIndexStream = NULL;
  mFilesLocked = FALSE;
  mUnreadMsgs  = -1;

  mMsgList.reset(INIT_MSGS);
}


//-----------------------------------------------------------------------------
int KMFolder::lock()
{
  int rc;
  struct flock fl;
  fl.l_type=F_WRLCK;
  fl.l_whence=0;
  fl.l_start=0;
  fl.l_len=0;
  fl.l_pid=-1;
  QString cmd_str;
  assert(mStream != NULL);
  mFilesLocked = FALSE;

  switch( mLockType )
  {
    case FCNTL:
      rc = fcntl(fileno(mStream), F_SETLKW, &fl);

  if (rc < 0)
  {
        kdDebug() << "Cannot lock folder `" << (const char*)location() << "': "
                  << strerror(errno) << " (" << errno << ")" << endl;
    return errno;
  }

  if (mIndexStream)
  {
    rc = fcntl(fileno(mIndexStream), F_SETLK, &fl);

    if (rc < 0)
    {
          kdDebug() << "Cannot lock index of folder `" << (const char*)location() << "': "
                    << strerror(errno) << " (" << errno << ")" << endl;
      rc = errno;
      fl.l_type = F_UNLCK;
      rc = fcntl(fileno(mIndexStream), F_SETLK, &fl);
      return rc;
    }
  }
      break;

    case procmail_lockfile:
      cmd_str = "lockfile " + location() + ".lock";
      rc = system( cmd_str.latin1() );
      if( rc != 0 )
      {
        kdDebug() << "Cannot lock folder `" << (const char*)location() << "': "
                  << strerror(rc) << " (" << rc << ")" << endl;
        return rc;
      }
      if( mIndexStream )
      {
        cmd_str = "lockfile " + indexLocation() + ".lock";
        rc = system( cmd_str.latin1() );
        if( rc != 0 )
        {
          kdDebug() << "Cannot lock index of folder `" << (const char*)location() << "': "
                    << strerror(rc) << " (" << rc << ")" << endl;
          return rc;
        }
      }
      break;

    case mutt_dotlock:
      cmd_str = "mutt_dotlock " + location();
      rc = system( cmd_str.latin1() );
      if( rc != 0 )
      {
        kdDebug() << "Cannot lock folder `" << (const char*)location() << "': "
                  << strerror(rc) << " (" << rc << ")" << endl;
        return rc;
      }
      if( mIndexStream )
      {
        cmd_str = "mutt_dotlock " + indexLocation();
        rc = system( cmd_str.latin1() );
        if( rc != 0 )
        {
          kdDebug() << "Cannot lock index of folder `" << (const char*)location() << "': "
                    << strerror(rc) << " (" << rc << ")" << endl;
          return rc;
        }
      }
      break;

    case mutt_dotlock_privileged:
      cmd_str = "mutt_dotlock -p " + location();
      rc = system( cmd_str.latin1() );
      if( rc != 0 )
      {
        kdDebug() << "Cannot lock folder `" << (const char*)location() << "': "
                  << strerror(rc) << " (" << rc << ")" << endl;
        return rc;
      }
      if( mIndexStream )
      {
        cmd_str = "mutt_dotlock -p " + indexLocation();
        rc = system( cmd_str.latin1() );
        if( rc != 0 )
        {
          kdDebug() << "Cannot lock index of folder `" << (const char*)location() << "': "
                    << strerror(rc) << " (" << rc << ")" << endl;
          return rc;
        }
      }
      break;

    case None:
    default:
      break;
  }


  mFilesLocked = TRUE;
  return 0;
}


//-----------------------------------------------------------------------------
int KMFolder::unlock()
{
  int rc;
  struct flock fl;
  fl.l_type=F_UNLCK;
  fl.l_whence=0;
  fl.l_start=0;
  fl.l_len=0;
  QString cmd_str;

  assert(mStream != NULL);
  mFilesLocked = FALSE;

  switch( mLockType )
  {
    case FCNTL:
  if (mIndexStream) fcntl(fileno(mIndexStream), F_SETLK, &fl);
      fcntl(fileno(mStream), F_SETLK, F_UNLCK);
      rc = errno;
      break;

    case procmail_lockfile:
      cmd_str = "rm -f " + location() + ".lock";
      rc = system( cmd_str.latin1() );
      if( mIndexStream )
      {
        cmd_str = "rm -f " + indexLocation() + ".lock";
        rc = system( cmd_str.latin1() );
}
      break;

    case mutt_dotlock:
      cmd_str = "mutt_dotlock -u " + location();
      rc = system( cmd_str.latin1() );
      if( mIndexStream )
      {
        cmd_str = "mutt_dotlock -u " + indexLocation();
        rc = system( cmd_str.latin1() );
      }
      break;

    case mutt_dotlock_privileged:
      cmd_str = "mutt_dotlock -p -u " + location();
      rc = system( cmd_str.latin1() );
      if( mIndexStream )
      {
        cmd_str = "mutt_dotlock -p -u " + indexLocation();
        rc = system( cmd_str.latin1() );
      }
      break;

    case None:
    default:
      rc = 0;
      break;
  }

  return rc;
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
int KMFolder::createIndexFromContents()
{
  char line[MAX_LINE];
  char status[8], xstatus[8];
  QString subjStr, dateStr, fromStr, toStr, xmarkStr, *lastStr=NULL;
  QString replyToIdStr, referencesStr, msgIdStr;
  QString whoFieldName;
  unsigned long offs, size, pos;
  bool atEof = FALSE;
  bool inHeader = TRUE;
  KMMsgInfo* mi;
  QString msgStr;
  QRegExp regexp(MSG_SEPERATOR_REGEX);
  int i, num, numStatus, whoFieldLen;
  short needStatus;

  assert(mStream != NULL);
  rewind(mStream);

  mMsgList.clear();

  num     = -1;
  numStatus= 11;
  offs    = 0;
  size    = 0;
  dateStr = "";
  fromStr = "";
  toStr = "";
  subjStr = "";
  *status = '\0';
  *xstatus = '\0';
  xmarkStr = "";
  replyToIdStr = "";
  msgIdStr = "";
  needStatus = 3;
  whoFieldName = QString(whoField()) + ':'; //unused (sven)
  whoFieldLen = whoFieldName.length();      //unused (sven)

  //kdDebug() << "***whoField: " << //      (const char*)whoFieldName << " (" << whoFieldLen << ")" << endl;

  while (!atEof)
  {
    pos = ftell(mStream);
    if (!fgets(line, MAX_LINE, mStream)) atEof = TRUE;

    if (atEof ||
	(strncmp(line,MSG_SEPERATOR_START, msgSepLen)==0 &&
	 regexp.match(line) >= 0))
    {
      size = pos - offs;
      pos = ftell(mStream);

      if (num >= 0)
      {
	if (numStatus <= 0)
	{
	  msgStr = i18n("Creating index file: %1 messages done").arg(num);
	  emit statusMsg(msgStr);
	  numStatus = 10;
	}

	if (size > 0)
	{
	  if ((replyToIdStr.isEmpty() || (replyToIdStr[0] != '<'))  &&
	      !referencesStr.isEmpty() && referencesStr[0] == '<') {
	    replyToIdStr = referencesStr;
	  }
	  mi = new KMMsgInfo(this);
	  mi->init(subjStr, fromStr, toStr, 0, KMMsgStatusNew, xmarkStr, replyToIdStr, msgIdStr, offs, size);
	  mi->setStatus("RO","O");
	  mi->setDate(dateStr);
	  mi->setDirty(FALSE);
	  mMsgList.append(mi);

	  *status = '\0';
	  *xstatus = '\0';
	  needStatus = 3;
	  xmarkStr = "";
	  replyToIdStr = "";
	  referencesStr = "";
	  msgIdStr = "";
	  dateStr = "";
	  fromStr = "";
	  subjStr = "";
	}
	else num--,numStatus++;
      }

      offs = ftell(mStream);
      num++;
      numStatus--;
      inHeader = TRUE;
      continue;
    }
    // Is this a long header line?
    if (inHeader && (line[0]=='\t' || line[0]==' '))
    {
      i = 0;
      while (line [i]=='\t' || line [i]==' ') i++;
      if (line [i] < ' ' && line [i]>0) inHeader = FALSE;
      else if (lastStr) *lastStr += line + i;
    }
    else lastStr = NULL;

    if (inHeader && (line [0]=='\n' || line [0]=='\r'))
      inHeader = FALSE;
    if (!inHeader) continue;

    /* -sanders Make all messages read when auto-recreating index
    if ((needStatus & 1) && strncasecmp(line, "Status:", 7) == 0 &&
	isblank(line[7]))
    {
      for(i=0; i<4 && line[i+8] > ' '; i++)
	status[i] = line[i+8];
      status[i] = '\0';
      needStatus &= ~1;
    }
    else if ((needStatus & 2) && strncasecmp(line, "X-Status:", 9)==0 &&
	     isblank(line[9]))
    {
      for(i=0; i<4 && line[i+10] > ' '; i++)
	xstatus[i] = line[i+10];
      xstatus[i] = '\0';
      needStatus &= ~2;
    }
    else*/ if (strncasecmp(line,"X-KMail-Mark:",13)==0 && isblank(line[13]))
      xmarkStr = QString(line+14).copy();
    else if (strncasecmp(line,"In-Reply-To:",12)==0 && isblank(line[12])) {
      int rightAngle;
      replyToIdStr = QString(line+13).copy();
      rightAngle = replyToIdStr.find( '>' );
      if (rightAngle != -1)
	replyToIdStr.truncate( rightAngle + 1 );
    }
    else if (strncasecmp(line,"References:",11)==0 && isblank(line[11])) {
      int leftAngle, rightAngle;
      referencesStr = QString(line+12).copy();
      leftAngle = referencesStr.findRev( '<' );
      if (leftAngle != -1)
	referencesStr = referencesStr.mid( leftAngle );
      rightAngle = referencesStr.find( '>' );
      if (rightAngle != -1)
	referencesStr.truncate( rightAngle + 1 );
    }
    else if (strncasecmp(line,"Message-Id:",11)==0 && isblank(line[11])) {
      int rightAngle;
      msgIdStr = QString(line+12).copy();
      rightAngle = msgIdStr.find( '>' );
      if (rightAngle != -1)
	msgIdStr.truncate( rightAngle + 1 );
    }
    else if (strncasecmp(line,"Date:",5)==0 && isblank(line[5]))
    {
      dateStr = QString(line+6).copy();
      lastStr = &dateStr;
    }
    else if (strncasecmp(line,"From:", 5)==0 &&
	     isblank(line[5]))
    {
      fromStr = QString(line+6).copy();
      lastStr = &fromStr;
    }
    else if (strncasecmp(line,"To:", 3)==0 &&
	     isblank(line[3]))
    {
      toStr = QString(line+4).copy();
      lastStr = &toStr;
    }
    else if (strncasecmp(line,"Subject:",8)==0 && isblank(line[8]))
    {
      subjStr = QString(line+9).copy();
      lastStr = &subjStr;
    }
  }

  if (mAutoCreateIndex)
  {
    emit statusMsg(i18n("Writing index file"));
    writeIndex();
  }
  else mHeaderOffset = 0;

  correctUnreadMsgsCount();
  return 0;
}


//-----------------------------------------------------------------------------
int KMFolder::writeIndex()
{
  QString tempName;
  KMMsgBase* msgBase;
  int old_umask;
  int i=0;

  if (mIndexStream) fclose(mIndexStream);
  old_umask = umask(077);

  tempName = indexLocation() + ".temp";
  unlink(tempName);

  mIndexStream = fopen(tempName, "w");
  umask(old_umask);
  if (!mIndexStream) return errno;

  fprintf(mIndexStream, "# KMail-Index V%d\n", INDEX_VERSION);

  mHeaderOffset = ftell(mIndexStream);
  for (i=0; i<mMsgList.high(); i++)
  {
    if (!(msgBase = mMsgList[i])) continue;
    fprintf(mIndexStream, "%s\n", (const char*)msgBase->asIndexString());
  }
  if (fflush(mIndexStream) != 0) return errno;
  if (fclose(mIndexStream) != 0) return errno;

  _rename(tempName, indexLocation());
  mIndexStream = fopen(indexLocation(), "r+"); // index file

  mDirty = FALSE;
  return 0;
}


//-----------------------------------------------------------------------------
void KMFolder::setAutoCreateIndex(bool autoIndex)
{
  mAutoCreateIndex = autoIndex;
}


//-----------------------------------------------------------------------------
bool KMFolder::readIndexHeader()
{
  int indexVersion;

  assert(mIndexStream != NULL);

  fscanf(mIndexStream, "# KMail-Index V%d\n", &indexVersion);
  if (indexVersion == 1503 && INDEX_VERSION == 1504)
  {
    kdDebug() << "Converting index file " << (const char*)indexLocation() << " to utf-8" << endl;
    mConvertToUtf8 = TRUE;
  }
  else if (indexVersion < INDEX_VERSION)
  {
    kdDebug() << "Index file " << (const char*)indexLocation() << " is out of date. Re-creating it." << endl;
    createIndexFromContents();
    return FALSE;
  }

  return TRUE;
}


//-----------------------------------------------------------------------------
void KMFolder::readIndex()
{
  char line[MAX_LINE];
  KMMsgInfo* mi;

  assert(mIndexStream != NULL);
  rewind(mIndexStream);

  mMsgList.clear();
  if (!readIndexHeader()) return;

  mUnreadMsgs = 0;
  mDirty = FALSE;
  mHeaderOffset = ftell(mIndexStream);

  mMsgList.clear();
  while (!feof(mIndexStream))
  {
    fgets(line, MAX_LINE, mIndexStream);
    if (feof(mIndexStream)) break;

    mi = new KMMsgInfo(this);
    mi->fromIndexString(line, mConvertToUtf8);
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
      ++mUnreadMsgs;
    mMsgList.append(mi);
  }
  if (mConvertToUtf8)
  {
    mConvertToUtf8 = FALSE;
    mDirty = TRUE;
    writeIndex();
  }
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
void KMFolder::removeMsg(KMMsgBasePtr aMsg)
{
  removeMsg(find(aMsg));
}


//-----------------------------------------------------------------------------
void KMFolder::removeMsg(int idx)
{
  KMMsgBase* mb;

  //assert(idx>=0);
  if(idx < 0)
    {
      kdDebug() << "KMFolder::removeMsg() : idx < 0\n" << endl;
      return;
    }
  QString msgIdMD5 = mMsgList[idx]->msgIdMD5();
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

  assert(mStream!=NULL);
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

  if (mb->isMessage()) return ((KMMessage*)mb);
  return readMsg(idx);
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
KMMessage* KMFolder::readMsg(int idx)
{
  KMMessage* msg;
  unsigned long msgSize;
  QCString msgText;
  KMMsgInfo* mi = (KMMsgInfo*)mMsgList[idx];

  assert(mi!=NULL && !mi->isMessage());
  assert(mStream != NULL);

  msgSize = mi->msgSize();
  msgText.resize(msgSize+2);

  fseek(mStream, mi->folderOffset(), SEEK_SET);
  fread(msgText.data(), msgSize, 1, mStream);
  msgText[msgSize] = '\0';

  msg = new KMMessage(*mi);
  msg->fromString(msgText);

  mMsgList.set(idx,msg);

  return msg;
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
int KMFolder::addMsg(KMMessage* aMsg, int* aIndex_ret)
{
  long offs, size, len, revert;
  bool opened = FALSE;
  QString msgText;
  char endStr[3];
  int idx = -1, rc;
  KMFolder* msgParent;
  bool editing = false;

  if (!mStream)
  {
    opened = TRUE;
    rc = open();
    kdDebug() << "addMsg-open: " << rc << endl;
    if (rc) return rc;
  }

  // take message out of the folder it is currently in, if any
  msgParent = aMsg->parent();
  if (msgParent)
  {
    if (msgParent==this)
      {
	if (name() == "outbox" || name() == "drafts")
          //special case for Edit message.
	  {
	    // debug ("Editing message in outbox");
	    editing = true;
	  }
	else
	  return 0;
      }

    idx = msgParent->find(aMsg);
    msgParent->getMsg( idx );
  }

  aMsg->setStatusFields();
  msgText = aMsg->asString();
  msgText.replace(QRegExp("\nFrom "),"\n>From ");
  len = msgText.length();

  assert(mStream != NULL);
  clearerr(mStream);
  if (len <= 0)
  {
    kdDebug() << "Message added to folder `" << (const char*)name() << "' contains no data. Ignoring it." << endl;
    if (opened) close();
    return 0;
  }

  // Make sure the file is large enough to check for an end
  // character
  fseek(mStream, 0, SEEK_END);
  if (ftell(mStream) >= 2)
    {
      // write message to folder file
      fseek(mStream, -2, SEEK_END);
      fread(endStr, 1, 2, mStream); // ensure separating empty line
      if (ftell(mStream) > 0 && endStr[0]!='\n')
	{
	  if (endStr[1]!='\n')
	    {
	      //printf ("****endStr[1]=%c\n", endStr[1]);
	      fwrite("\n\n", 1, 2, mStream);
	    }
	  else fwrite("\n", 1, 1, mStream);
	}
    }
  fseek(mStream,0,SEEK_END); // this is needed on solaris and others
  revert = ftell(mStream);
  int error = ferror(mStream);
  if (error)
    {
      if (opened) close();
      return error;
    }

  fprintf(mStream, "From %s %s\n", (const char *)aMsg->fromEmail(),
          (const char *)aMsg->dateShortStr());
  offs = ftell(mStream);
  fwrite(msgText, len, 1, mStream);
  if (msgText[(int)len-1]!=QChar('\n')) fwrite("\n\n", 1, 2, mStream);
  fflush(mStream);
  size = ftell(mStream) - offs;

  error = ferror(mStream);
  if (error) {
    kdDebug() << "Error: Could not add message to folder (No space left on device?)" << endl;
    if (ftell(mStream) > revert) {
      kdDebug() << "Undoing changes" << endl;
      truncate( location(), revert );
    }
    bool busy = kernel->kbp()->isBusy();
    if (busy) kernel->kbp()->idle();
    KMessageBox::sorry(0,
	  i18n("Unable to add message to folder.\n"
	       "(No space left on device or insufficient quota?)\n\n"
	       "Free space and sufficient quota are required to continue safely."));
    if (busy) kernel->kbp()->busy();
    if (opened) close();
    kernel->kbp()->idle();
    return error;
  }

  if (msgParent) {
    if (idx >= 0) msgParent->take(idx);
  }

  if (aMsg->status()==KMMsgStatusUnread ||
      aMsg->status()==KMMsgStatusNew) {
    if (mUnreadMsgs == -1) mUnreadMsgs = 1;
    else ++mUnreadMsgs;
    emit numUnreadMsgsChanged( this );
  }

  // store information about the position in the folder file in the message
  aMsg->setParent(this);
  aMsg->setFolderOffset(offs);
  aMsg->setMsgSize(size);
  idx = mMsgList.append(aMsg);

  // write index entry if desired
  if (mAutoCreateIndex)
  {
    assert(mIndexStream != NULL);
    clearerr(mIndexStream);
    fseek(mIndexStream, 0, SEEK_END);
    revert = ftell(mIndexStream);
    fprintf(mIndexStream, "%s\n", (const char*)aMsg->asIndexString());
    fflush(mIndexStream);
    error = ferror(mIndexStream);
    if (error) {
      kdDebug() << "Error: Could not add message to folder (No space left on device?)" << endl;
      if (ftell(mIndexStream) > revert) {
	kdDebug() << "Undoing changes" << endl;
	truncate( indexLocation(), revert );
      }
      bool busy = kernel->kbp()->isBusy();
      if (busy) kernel->kbp()->idle();
      KMessageBox::sorry(0,
        i18n("Unable to add message to folder.\n"
	     "(No space left on device or insufficient quota?)\n\n"
	     "Free space and sufficient quota are required to continue safely."));
      if (busy) kernel->kbp()->busy();
      if (opened) close();
      return error;
    }
  }

  // some "paper work"
  if (aIndex_ret) *aIndex_ret = idx;
  if (!mQuiet)
    emit msgAdded(idx);
  else
    mChanged = TRUE;

  if (opened) close();

  // All streams have been flushed without errors if we arrive here
  // Return success!
  // (Don't return status of stream, it may have been closed already.)
  return 0;
}


//-----------------------------------------------------------------------------
int KMFolder::rename(const QString& aName, KMFolderDir *aParent)
{
  QString oldLoc, oldIndexLoc, newLoc, newIndexLoc, oldName;
  QString oldSubDirLoc, newSubDirLoc;
  int rc=0, openCount=mOpenCount;
  KMFolderDir *oldParent;

  assert(!aName.isEmpty());

  oldLoc = location().copy();
  oldIndexLoc = indexLocation().copy();
  oldSubDirLoc = subdirLocation().copy();

  close(TRUE);

  oldName = name();
  oldParent = parent();
  if (aParent)
    setParent( aParent );

  setName(aName);
  newLoc = location();
  newIndexLoc = indexLocation();
  newSubDirLoc = subdirLocation();

  if (_rename(oldLoc, newLoc)) {
    setName(oldName);
    setParent(oldParent);
    rc = errno;
  }
  else if (!oldIndexLoc.isEmpty()) {
    _rename(oldIndexLoc, newIndexLoc);
    if (!_rename(oldSubDirLoc, newSubDirLoc )) {
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
  unlink(indexLocation());
  rc = unlink(location());
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

  if (mAutoCreateIndex) truncate(indexLocation(), mHeaderOffset);
  else unlink(indexLocation());

  if (truncate(location(), 0)) return errno;
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
int KMFolder::compact()
{
  KMFolder* tempFolder;
  KMMessage* msg;
  QCString tempName;
  QString msgStr;
  int openCount = mOpenCount;
  int num, numStatus;
  int rc = 0;

  if (!needsCompact)
    return 0;
  kdDebug() << "Compacting " << endl;
  tempName = "." + name().local8Bit();

  tempName += ".compacted";
  unlink(path().local8Bit() + "/" + tempName);
  tempFolder = new KMFolder(parent(), tempName);   //sven: we create it
  if(tempFolder->create()) {
    kdDebug() << "KMFolder::compact() Creating tempFolder failed!\n" << endl;
    delete tempFolder;                             //sven: and we delete it
    return 0;
  }

  quiet(TRUE);
  tempFolder->open();
  open();

  for(num=1,numStatus=9; count() > 0; num++, numStatus--)
  {
    if (numStatus <= 0)
    {
      msgStr = i18n("Compacting folder: %1 messages done").arg(num);
      emit statusMsg(msgStr);
      numStatus = 10;
    }

    msg = getMsg(0);
    if(msg)
      rc = tempFolder->moveMsg(msg);
    if (rc)
      break;
    tempFolder->unGetMsg(tempFolder->count() - 1);
  }
  tempName = tempFolder->location();
  tempFolder->close(TRUE);
  close(TRUE);
  mMsgList.clear(TRUE);

  if (!rc) {
    _rename(tempName, location());
    _rename(tempFolder->indexLocation(), indexLocation());
  }
  else
  {
    kdDebug() << "Error occurred while compacting" << endl;
    kdDebug() << "Compaction aborted." << endl;
  }

  // Now really free all memory
  delete tempFolder;                //sven: we delete it, not the manager

  if (openCount > 0)
  {
    open();
    mOpenCount = openCount;
  }
  quiet(FALSE);

  if (!mQuiet)
    emit changed();
  else
    mChanged = TRUE;
  needsCompact = false;             // We are clean now
  return 0;
}


//-----------------------------------------------------------------------------
int KMFolder::sync()
{
  KMMsgBasePtr mb;
  unsigned long offset = mHeaderOffset;
  int i, rc, recSize = KMMsgBase::indexStringLength()+1;
  int high = mMsgList.high();

  if (!mIndexStream) return 0;

  for (rc=0,i=0; i<high; i++)
  {
    mb = mMsgList[i];
    if (mb->dirty())
    {
      fseek(mIndexStream, offset, SEEK_SET);
      fprintf(mIndexStream, "%s\n", (const char*)mb->asIndexString());
      rc = errno;
      if (rc) break;
      mb->setDirty(FALSE);
    }
    offset += recSize;
  }
  fflush(mIndexStream);

  mDirty = FALSE;
  return rc;
}


//-----------------------------------------------------------------------------
const char* KMFolder::type() const
{
  if (mAcctList) return "In";
  return KMFolderInherited::type();
}


//-----------------------------------------------------------------------------
const QString KMFolder::label() const
{
  if (mIsSystemFolder && !mLabel.isEmpty()) return mLabel;
  if (mIsSystemFolder) return i18n(name());
  if (name() == "inbox") return i18n(name());
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
  return mUnreadMsgs;
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
    mDirty = TRUE;
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
  return (mWhoField.isEmpty() ? "From" : (const char*)mWhoField);
}


//-----------------------------------------------------------------------------
void KMFolder::setWhoField(const QString& aWhoField)
{
  mWhoField = aWhoField.copy();
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
  config->setGroup("Folder-" + idString());
  if (mUnreadMsgs == -1)
    mUnreadMsgs = config->readNumEntry("UnreadMsgs", -1);
  mMailingListEnabled = config->readBoolEntry("MailingListEnabled");
  mMailingListPostingAddress = config->readEntry("MailingListPostingAddress");
  mMailingListAdminAddress = config->readEntry("MailingListAdminAddress");
}

//-----------------------------------------------------------------------------
void KMFolder::writeConfig()
{
  KConfig* config = kapp->config();
  config->setGroup("Folder-" + idString());
  config->writeEntry("UnreadMsgs", mUnreadMsgs);
  config->writeEntry("MailingListEnabled", mMailingListEnabled);
  config->writeEntry("MailingListPostingAddress", mMailingListPostingAddress);
  config->writeEntry("MailingListAdminAddress", mMailingListAdminAddress);
}

//-----------------------------------------------------------------------------
void KMFolder::correctUnreadMsgsCount()
{
  open();
  close();
  emit numUnreadMsgsChanged( this );
}

//-----------------------------------------------------------------------------
void KMFolder::setLockType( LockType ltype )
{
  mLockType = ltype;
}

#include "kmfolder.moc"
