// kmfolder.cpp
// Author: Stefan Taferner <taferner@alpin.or.at>

#include <qfileinfo.h>
#include <qsortedlist.h>

#include "kmglobal.h"
#include "kmfolder.h"
#include "kmmessage.h"
#include "kmfolderdir.h"
#include "kbusyptr.h"
#include "kmundostack.h"
#include "kmacctimap.h"

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

#ifndef isblank
#  define isblank(x) ((x)==' '||(x)=='\t')
#endif

#define MAX_LINE 4096
#define INIT_MSGS 8

// Current version of the table of contents (index) files
#define INDEX_VERSION 1506

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
  mAccount        = NULL;
  mMailingListEnabled = FALSE;
  mIndexId        = -1;
  mIndexStreamPtr = NULL;
  mIndexStreamPtrLength = 0;
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
QString KMFolder::location() const
{
  QString sLocation(path());

  if (!sLocation.isEmpty()) sLocation += '/';
  sLocation += name();

  return sLocation;
}


//-----------------------------------------------------------------------------
QString KMFolder::indexLocation() const
{
  QString sLocation(path());

  if (!sLocation.isEmpty()) sLocation += '/';
  sLocation += '.';
  sLocation += name();
  sLocation += ".index";

  return sLocation;
}

//-----------------------------------------------------------------------------
QString KMFolder::subdirLocation() const
{
  QString sLocation(path());

  if (!sLocation.isEmpty()) sLocation += '/';
  sLocation += '.';
  sLocation += name();
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
int KMFolder::open()
{
  int rc = 0;

  mOpenCount++;
  if (mOpenCount > 1) return 0;  // already open

  assert(name() != "");

  mFilesLocked = FALSE;
  mStream = fopen(location().local8Bit(), "r+"); // messages file
  if (!mStream)
  {
    kdDebug(5006) << "Cannot open folder `" << location() << "': " << strerror(errno) << endl;
    mOpenCount = 0;
    return errno;
  }

  lock();

  if (!path().isEmpty())
  {
    if (isIndexOutdated()) // test if contents file has changed
    {
      QString str;
      mIndexStream = NULL;
      str = i18n("Folder `%1' changed. Recreating index.")
		  .arg(name());
      emit statusMsg(str);
    } else {
      mIndexStream = fopen(indexLocation().local8Bit(), "r+"); // index file
      updateIndexStreamPtr();
    }	

    if (!mIndexStream)
      rc = createIndexFromContents();
    else
      readIndex();
  }
  else
  {
    mAutoCreateIndex = FALSE;
    rc = createIndexFromContents();
  }

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

  kdDebug(5006) << "Creating folder " << endl;
  if (access(location().local8Bit(), F_OK) == 0) {
    kdDebug(5006) << "KMFolder::create call to access function failed." << endl;
    kdDebug(5006) << "File:: " << endl;
    kdDebug(5006) << "Error " << endl;
    return EEXIST;
  }

  old_umask = umask(077);
  mStream = fopen(location().local8Bit(), "w+"); //sven; open RW
  umask(old_umask);

  if (!mStream) return errno;

  if (!path().isEmpty())
  {
    old_umask = umask(077);
    mIndexStream = fopen(indexLocation().local8Bit(), "w+"); //sven; open RW
	updateIndexStreamPtr(TRUE);
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
      bool dirty = mDirty;
      for (int i=0; !dirty && i<mMsgList.high(); i++)
	  if (mMsgList[i])
	      dirty = !mMsgList[i]->syncIndexString();
      if(dirty)
	  writeIndex();
      writeConfig();
  }

  unlock();
  mMsgList.clear(TRUE);

  if (mStream) fclose(mStream);
    if (mIndexStream) {
	fclose(mIndexStream);
	updateIndexStreamPtr(TRUE);
    }

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
	kdDebug(5006) << "Cannot lock folder `" << location() << "': "
		  << strerror(errno) << " (" << errno << ")" << endl;
	return errno;
      }

      if (mIndexStream)
      {
	rc = fcntl(fileno(mIndexStream), F_SETLK, &fl);

	if (rc < 0)
	{
          kdDebug(5006) << "Cannot lock index of folder `" << location() << "': "
                    << strerror(errno) << " (" << errno << ")" << endl;
	  rc = errno;
	  fl.l_type = F_UNLCK;
	  rc = fcntl(fileno(mIndexStream), F_SETLK, &fl);
	  return rc;
	}
      }
      break;

    case procmail_lockfile:
      cmd_str = "lockfile ";
      if (!mProcmailLockFileName.isEmpty())
	cmd_str += mProcmailLockFileName;
      else
        cmd_str += location() + ".lock";

      rc = system( cmd_str.local8Bit() );
      if( rc != 0 )
      {
	kdDebug(5006) << "Cannot lock folder `" << location() << "': "
                  << strerror(rc) << " (" << rc << ")" << endl;
        return rc;
      }
      if( mIndexStream )
      {
        cmd_str = "lockfile " + indexLocation() + ".lock";
        rc = system( cmd_str.local8Bit() );
        if( rc != 0 )
        {
          kdDebug(5006) << "Cannot lock index of folder `" << location() << "': "
                    << strerror(rc) << " (" << rc << ")" << endl;
          return rc;
        }
      }
      break;

    case mutt_dotlock:
      cmd_str = "mutt_dotlock " + location();
      rc = system( cmd_str.local8Bit() );
      if( rc != 0 )
      {
        kdDebug(5006) << "Cannot lock folder `" << location() << "': "
                  << strerror(rc) << " (" << rc << ")" << endl;
        return rc;
      }
      if( mIndexStream )
      {
        cmd_str = "mutt_dotlock " + indexLocation();
        rc = system( cmd_str.local8Bit() );
        if( rc != 0 )
        {
          kdDebug(5006) << "Cannot lock index of folder `" << location() << "': "
                    << strerror(rc) << " (" << rc << ")" << endl;
          return rc;
        }
      }
      break;

    case mutt_dotlock_privileged:
      cmd_str = "mutt_dotlock -p " + location();
      rc = system( cmd_str.local8Bit() );
      if( rc != 0 )
      {
        kdDebug(5006) << "Cannot lock folder `" << location() << "': "
                  << strerror(rc) << " (" << rc << ")" << endl;
        return rc;
      }
      if( mIndexStream )
      {
        cmd_str = "mutt_dotlock -p " + indexLocation();
        rc = system( cmd_str.local8Bit() );
        if( rc != 0 )
        {
          kdDebug(5006) << "Cannot lock index of folder `" << location() << "': "
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
      cmd_str = "rm -f ";
      if (!mProcmailLockFileName.isEmpty())
        cmd_str += mProcmailLockFileName;
      else
        cmd_str += location() + ".lock";

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

  //kdDebug(5006) << "***whoField: " << //      (const char*)whoFieldName << " (" << whoFieldLen << ")" << endl;

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
	  msgStr = i18n("Creating index file: %n message done", "Creating index file: %n messages done", num);
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
	  mi->setDate(dateStr.latin1());
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
        xmarkStr = QString(line+14);
    else if (strncasecmp(line,"In-Reply-To:",12)==0 && isblank(line[12])) {
      int rightAngle;
      replyToIdStr = QString(line+13);
      rightAngle = replyToIdStr.find( '>' );
      if (rightAngle != -1)
	replyToIdStr.truncate( rightAngle + 1 );
    }
    else if (strncasecmp(line,"References:",11)==0 && isblank(line[11])) {
      int leftAngle, rightAngle;
      referencesStr = QString(line+12);
      leftAngle = referencesStr.findRev( '<' );
      if (leftAngle != -1)
	referencesStr = referencesStr.mid( leftAngle );
      rightAngle = referencesStr.find( '>' );
      if (rightAngle != -1)
	referencesStr.truncate( rightAngle + 1 );
    }
    else if (strncasecmp(line,"Message-Id:",11)==0 && isblank(line[11])) {
      int rightAngle;
      msgIdStr = QString(line+12);
      rightAngle = msgIdStr.find( '>' );
      if (rightAngle != -1)
	msgIdStr.truncate( rightAngle + 1 );
    }
    else if (strncasecmp(line,"Date:",5)==0 && isblank(line[5]))
    {
        dateStr = QString(line+6);
      lastStr = &dateStr;
    }
    else if (strncasecmp(line,"From:", 5)==0 &&
	     isblank(line[5]))
    {
        fromStr = QString(line+6);
      lastStr = &fromStr;
    }
    else if (strncasecmp(line,"To:", 3)==0 &&
	     isblank(line[3]))
    {
        toStr = QString(line+4);
      lastStr = &toStr;
    }
    else if (strncasecmp(line,"Subject:",8)==0 && isblank(line[8]))
    {
        subjStr = QString(line+9);
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

  if (kernel->outboxFolder() == this && count() > 0)
    KMessageBox::information(0, i18n("Your outbox contains messages which were "
    "most likely not created by KMail.\nPlease remove them from there, if you "
    "don't want KMail to send them."));

  return 0;
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
void KMFolder::readIndex()
{
  int len, offs;
  KMMsgInfo* mi;
  assert(mIndexStream != NULL);
  rewind(mIndexStream);

  mMsgList.clear();
  int version;
  if (!readIndexHeader(&version)) return;

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
void KMFolder::removeMsg(int idx, bool imapQuiet)
{
  //assert(idx>=0);
  if(idx < 0)
  {
    kdDebug(5006) << "KMFolder::removeMsg() : idx < 0\n" << endl;
    return;
  }
  KMMsgBase* mb = mMsgList[idx];
  QString msgIdMD5 = mb->msgIdMD5();
  if (mAccount && !imapQuiet && !mb->isMessage()) readMsg(idx);
  mb = mMsgList.take(idx);

  if (mAccount && !imapQuiet)
  {
    KMMessage *msg = static_cast<KMMessage*>(mb);
    mAccount->deleteMessage(msg);
  }

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

  if (mAccount)
  {
    KMMessage *msg = static_cast<KMMessage*>(mb);
    mAccount->deleteMessage(msg);
  }

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
  if (mb->isMessage())
      msg = ((KMMessage*)mb);
  else
      msg = readMsg(idx);
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
QCString& KMFolder::getMsgString(int idx, QCString &mDest)
{
  unsigned long msgSize;
  KMMsgInfo* mi = (KMMsgInfo*)mMsgList[idx];

  assert(mi!=NULL);
  assert(mStream != NULL);

  msgSize = mi->msgSize();
  mDest.resize(msgSize+2);

  fseek(mStream, mi->folderOffset(), SEEK_SET);
  fread(mDest.data(), msgSize, 1, mStream);
  mDest[msgSize] = '\0';

  return mDest;
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
void KMFolder::reallyAddMsg(KMMessage* aMsg)
{
  KMFolder *folder = aMsg->parent();
  int index;
  addMsg(aMsg, &index);
  if (index < 0) return;
  KMMsgBase *mb = unGetMsg(count() - 1);
  kernel->undoStack()->pushAction( mb->msgIdMD5(), folder, this );
}


//-----------------------------------------------------------------------------
void KMFolder::reallyAddCopyOfMsg(KMMessage* aMsg)
{
  aMsg->setParent( NULL );
  addMsg( aMsg );
  unGetMsg( count() - 1 );
}


//-----------------------------------------------------------------------------
void KMFolder::addMsgQuiet(KMMessage* aMsg)
{
  KMFolder *folder = aMsg->parent();
  kernel->undoStack()->pushAction( aMsg->msgIdMD5(), folder, this );
  if (folder) folder->take(folder->find(aMsg));
  delete aMsg;
}


//-----------------------------------------------------------------------------
int KMFolder::addMsg(KMMessage* aMsg, int* aIndex_ret, bool imapQuiet)
{
  long offs, size, len, revert;
  bool opened = FALSE;
  QCString msgText;
  char endStr[3];
  int idx = -1, rc;
  KMFolder* msgParent;
  bool editing = false;
  int growth = 0;

  if (!mStream)
  {
    opened = TRUE;
    rc = open();
    kdDebug(5006) << "addMsg-open: " << rc << endl;
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
    if (!imapQuiet && msgParent->account())
    {
      if (msgParent->account() == account())
      {
        KMImapJob *imapJob = new KMImapJob(aMsg, KMImapJob::tCopyMessage, this);
        connect(imapJob, SIGNAL(messageCopied(KMMessage*)),
          SLOT(addMsgQuiet(KMMessage*)));
        aMsg->setTransferInProgress(TRUE);
        if (aIndex_ret) *aIndex_ret = -1;
        return 0;
      }
      else if (!aMsg->isComplete())
      {
        KMImapJob *imapJob = new KMImapJob(aMsg);
        connect(imapJob, SIGNAL(messageRetrieved(KMMessage*)),
          SLOT(reallyAddMsg(KMMessage*)));
        aMsg->setTransferInProgress(TRUE);
        if (aIndex_ret) *aIndex_ret = -1;
        return 0;
      }
    }
  }

  if (mAccount && !imapQuiet)
  {
    aMsg->setTransferInProgress(TRUE);
    KMImapJob *imapJob = new KMImapJob(aMsg, KMImapJob::tPutMessage, this);
    connect(imapJob, SIGNAL(messageStored(KMMessage*)),
      SLOT(addMsgQuiet(KMMessage*)));
    if (aIndex_ret) *aIndex_ret = -1;
    return 0;
  }

  aMsg->setStatusFields();
  if (aMsg->headerField("Content-Type").isEmpty())  // This might be added by
    aMsg->removeHeaderField("Content-Type");        // the line above
  msgText = aMsg->asString();
  msgText.replace(QRegExp("\nFrom "),"\n>From ");
  len = msgText.length();

  assert(mStream != NULL);
  clearerr(mStream);
  if (len <= 0)
  {
    kdDebug(5006) << "Message added to folder `" << name() << "' contains no data. Ignoring it." << endl;
    if (opened) close();
    return 0;
  }

  // Make sure the file is large enough to check for an end
  // character
  fseek(mStream, 0, SEEK_END);
  if (ftell(mStream) >= 2) {
      // write message to folder file
      fseek(mStream, -2, SEEK_END);
      fread(endStr, 1, 2, mStream); // ensure separating empty line
      if (ftell(mStream) > 0 && endStr[0]!='\n') {
	  ++growth;
	  if (endStr[1]!='\n') {
	      //printf ("****endStr[1]=%c\n", endStr[1]);
	      fwrite("\n\n", 1, 2, mStream);
	      ++growth;
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
  if (msgText[(int)len-1]!='\n') fwrite("\n\n", 1, 2, mStream);
  fflush(mStream);
  size = ftell(mStream) - offs;

  error = ferror(mStream);
  if (error) {
    kdDebug(5006) << "Error: Could not add message to folder (No space left on device?)" << endl;
    if (ftell(mStream) > revert) {
      kdDebug(5006) << "Undoing changes" << endl;
      truncate( location().local8Bit(), revert );
    }
    kdDebug(5006) << "Abnormally terminating to prevent data loss, now." << endl;
    exit(1);
    /* This code is not 100% reliable
    bool busy = kernel->kbp()->isBusy();
    if (busy) kernel->kbp()->idle();
    KMessageBox::sorry(0,
	  i18n("Unable to add message to folder.\n"
	       "(No space left on device or insufficient quota?)\n\n"
	       "Free space and sufficient quota are required to continue safely."));
    if (busy) kernel->kbp()->busy();
    if (opened) close();
    kernel->kbp()->idle();
    */
    return error;
  }

  if (msgParent) {
    if (idx >= 0) msgParent->take(idx);
  }
//  if (mAccount) aMsg->removeHeaderField("X-UID");

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

  // change the length of the previous message to encompass white space added
  if ((idx > 0) && (growth > 0))
      mMsgList[idx - 1]->setMsgSize( mMsgList[idx - 1]->msgSize() + growth );

  // write index entry if desired
  if (mAutoCreateIndex)
  {
    assert(mIndexStream != NULL);
    clearerr(mIndexStream);
    fseek(mIndexStream, 0, SEEK_END);
    revert = ftell(mIndexStream);
	
	int len;
	const uchar *buffer = aMsg->asIndexString(len);
	fwrite(&len,sizeof(len), 1, mIndexStream);
	aMsg->setIndexOffset( ftell(mIndexStream) );
	aMsg->setIndexLength( len );
	if(fwrite(buffer, len, 1, mIndexStream) != 1)
	    kdDebug(5006) << "Whoa! " << __FILE__ << ":" << __LINE__ << endl;

    fflush(mIndexStream);
    error = ferror(mIndexStream);
    if (error) {
      kdDebug(5006) << "Error: Could not add message to folder (No space left on device?)" << endl;
      if (ftell(mIndexStream) > revert) {
	kdDebug(5006) << "Undoing changes" << endl;
	truncate( indexLocation().local8Bit(), revert );
      }
      kdDebug(5006) << "Abnormally terminating to prevent data loss, now." << endl;
      exit(1);
      /* This code may not be 100% reliable
      bool busy = kernel->kbp()->isBusy();
      if (busy) kernel->kbp()->idle();
      KMessageBox::sorry(0,
        i18n("Unable to add message to folder.\n"
	     "(No space left on device or insufficient quota?)\n\n"
	     "Free space and sufficient quota are required to continue safely."));
      if (busy) kernel->kbp()->busy();
      if (opened) close();
      */
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
int KMFolder::compact()
{
  QString tempName;
  QString msgStr;
  int rc = 0;
  int openCount = mOpenCount;

  if (!needsCompact)
    return 0;
  kdDebug(5006) << "Compacting " << endl;

  tempName = path() + "/." + name() + ".compacted";
  mode_t old_umask = umask(077);
  FILE *tmpfile = fopen(tempName.local8Bit(), "w");
  umask(old_umask);
  if (!tmpfile)
    return errno;
  open();

  KMMsgInfo* mi;
  int msize, offs=0, msgs=0, folder_offset;
  QCString mtext;
  for(int idx = 0; idx < mMsgList.count(); idx++) {
    if(!(msgs++ % 10)) {
      msgStr = i18n("Compacting folder: %1 messages done").arg(msgs);
      emit statusMsg(msgStr);
    }
    mi = (KMMsgInfo*)mMsgList[idx];
    msize = mi->msgSize();
    mtext.resize(msize+2);
    folder_offset = mi->folderOffset();

    //now we need to find the separator! grr...
    for(int i = folder_offset-25; TRUE; i -= 20) {
      int chunk_offset = i <= 0 ? 0 : i;
      if(fseek(mStream, chunk_offset, SEEK_SET) == -1) {
        rc = errno;
	break;
      }
      fread(mtext.data(), 20, 1, mStream);
      if(i <= 0) { //woops we've reached the top of the file, last try..
	if(!strncasecmp(mtext.data(), "from ", 5)) {
	  if(fseek(mStream, chunk_offset, SEEK_SET) == -1 ||
	     !fread(mtext.data(), folder_offset, 1, mStream) ||
	     !fwrite(mtext.data(), folder_offset, 1, tmpfile)) {
	      rc = errno;
	      break;
	  }
	  offs += folder_offset;
	} else {
	    rc = 666;
	}
	break;
      } else {
	int last_crlf = -1;
	for(int i2 = 0; i2 < 20; i2++) {
	  if(*(mtext.data()+i2) == '\n')
	    last_crlf = i2;
	}
	if(last_crlf != -1) {
	  int size = folder_offset - (i + last_crlf+1);
	  if(fseek(mStream, i + last_crlf+1, SEEK_SET) == -1 ||
	     !fread(mtext.data(), size, 1, mStream) ||
	     !fwrite(mtext.data(), size, 1, tmpfile)) {
	      rc = errno;
	      break;
	  }
	  offs += size;
	  break;
	}
      }
    }
    if (rc)
      break;
	
    //now actually write the message
    if(fseek(mStream, folder_offset, SEEK_SET) == -1 ||
       !fread(mtext.data(), msize, 1, mStream) || !fwrite(mtext.data(), msize, 1, tmpfile)) {
	rc = errno;
	break;
    }
    mi->setFolderOffset(offs);
    offs += msize;
  }
  fclose(tmpfile);

  if (!rc) {
    writeIndex();
    close(TRUE);
    _rename(tempName.local8Bit(), location().local8Bit());
  }
  else
  {
    close();
    kdDebug(5006) << "Error occurred while compacting" << endl;
    kdDebug(5006) << "Compaction aborted." << endl;
  }

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

//-----------------------------------------------------------------------------
void KMFolder::setProcmailLockFileName( const QString &fname )
{
  mProcmailLockFileName = fname;
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


