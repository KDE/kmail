// kmfoldermbox.cpp
// Author: Stefan Taferner <taferner@alpin.or.at>

#include <config.h>
#include <qfileinfo.h>
#include <qregexp.h>

#include "kmfoldermbox.h"
#include "kmmessage.h"
#include "kmundostack.h"
#include "kbusyptr.h"

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <knotifyclient.h>

#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

#ifndef isblank
#  define isblank(x) ((x)==' '||(x)=='\t')
#endif

#ifndef MAX_LINE
#define MAX_LINE 4096
#endif
#ifndef INIT_MSGS
#define INIT_MSGS 8
#endif

// Regular expression to find the line that seperates messages in a mail
// folder:
#define MSG_SEPERATOR_START "From "
#define MSG_SEPERATOR_REGEX "^From .*..:...*$"
static short msgSepLen = strlen(MSG_SEPERATOR_START);

//-----------------------------------------------------------------------------
KMFolderMbox::KMFolderMbox(KMFolderDir* aParent, const QString& aName)
  : KMFolderMboxInherited(aParent, aName)
{
  mStream         = NULL;
  mFilesLocked    = FALSE;
  mLockType       = None;
}


//-----------------------------------------------------------------------------
KMFolderMbox::~KMFolderMbox()
{
  if (mOpenCount>0) close(TRUE);
  if (kernel->undoStack()) kernel->undoStack()->folderDestroyed(this);
}

//-----------------------------------------------------------------------------
int KMFolderMbox::open()
{
  int rc = 0;

  mOpenCount++;
  if (mOpenCount > 1) return 0;  // already open

  assert(name() != "");

  mFilesLocked = FALSE;
  mStream = fopen(location().local8Bit(), "r+"); // messages file
  if (!mStream)
  {
    KNotifyClient::event("warning",
    i18n("Cannot open file \"%1\":\n%2").arg(location()).arg(strerror(errno)));
    kdDebug(5006) << "Cannot open folder `" << location() << "': " << strerror(errno) << endl;
    mOpenCount = 0;
    return errno;
  }

  lock();

  if (!path().isEmpty())
  {
    KMFolder::IndexStatus index_status = indexStatus();
    // test if index file exists and is up-to-date
    if (KMFolder::IndexOk != index_status)
    {
      // only show a warning if the index file exists, otherwise it can be
      // silently regenerated
      if (KMFolder::IndexTooOld == index_status)
      {
        bool busy = kernel->kbp()->isBusy();
        if (busy) kernel->kbp()->idle();
        if( 0 ) {
          // #### FIXME: Either make the link work or remove the following code.
          //             It's only there to get translated.
        KMessageBox::information( 0,
                                  i18n("<qt><p>The index of folder '%1' seems "
                                       "to be out of date. To prevent message "
                                       "corruption the index will be "
                                       "regenerated. As a result deleted "
                                       "messages might reappear and status "
                                       "flags might be lost.</p>"
                                       "<p>Please read the corresponding entry "
                                       "in the <a href=\"%2\">FAQ section of the manual of "
                                       "KMail</a> for "
                                       "information about how to prevent this "
                                       "problem from happening again.</p></qt>")
                                  .arg(name())
                                  .arg("help:/kmail/faq.html"),
                                  i18n("Index Out of Date"),
                                  "dontshowIndexRegenerationWarning");
        }
        else {
        KMessageBox::information( 0,
                                  i18n("<qt><p>The index of folder '%1' seems "
                                       "to be out of date. To prevent message "
                                       "corruption the index will be "
                                       "regenerated. As a result deleted "
                                       "messages might reappear and status "
                                       "flags might be lost.</p>"
                                       "<p>Please read the corresponding entry "
                                       "in the FAQ section of the manual of "
                                       "KMail for "
                                       "information about how to prevent this "
                                       "problem from happening again.</p></qt>")
                                  .arg(name()),
                                  i18n("Index Out of Date"),
                                  "dontshowIndexRegenerationWarning");
        }
        if (busy) kernel->kbp()->busy();
      }
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
      if (!readIndex())
	rc = createIndexFromContents();
  }
  else
  {
    mAutoCreateIndex = FALSE;
    rc = createIndexFromContents();
  }

  mQuiet = 0;
  mChanged = FALSE;

  readConfig();

  return rc;
}

//----------------------------------------------------------------------------
int KMFolderMbox::canAccess()
{
  assert(name() != "");

  if (access(location().local8Bit(), R_OK | W_OK) != 0) {
    kdDebug(5006) << "KMFolderMbox::access call to access function failed" << endl;
      return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
int KMFolderMbox::create(bool imap)
{
  int rc;
  int old_umask;

  assert(name() != "");
  assert(mOpenCount == 0);

  kdDebug(5006) << "Creating folder " << endl;
  if (access(location().local8Bit(), F_OK) == 0) {
    kdDebug(5006) << "KMFolderMbox::create call to access function failed." << endl;
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
  if (imap) {
    readConfig();
    mUnreadMsgs = -1;
  }

  rc = writeIndex();
  if (!rc) lock();
  return rc;
}


//-----------------------------------------------------------------------------
void KMFolderMbox::close(bool aForced)
{
  if (mOpenCount <= 0 || !mStream) return;
  if (mOpenCount > 0) mOpenCount--;
  if (mOpenCount > 0 && !aForced) return;

  if (mAutoCreateIndex)
  {
      if (KMFolder::IndexOk != indexStatus()) {
	  kdDebug(5006) << "Critical error: " << location() <<
	      " has been modified by an external application while KMail was running." << endl;
	  //	  exit(1); backed out due to broken nfs
      }

      bool dirty = mDirty;
      for (int i=0; !dirty && i<mMsgList.high(); i++)
	  if (mMsgList[i])
	      dirty = !mMsgList[i]->syncIndexString();
      if(dirty)
	  writeIndex();
      else
          touchMsgDict();
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
void KMFolderMbox::sync()
{
  if (mOpenCount > 0)
    if (!mStream || fsync(fileno(mStream)) ||
	!mIndexStream || fsync(fileno(mIndexStream))) {
    kernel->emergencyExit( i18n("Not enough free disk space." ));
    }
}

//-----------------------------------------------------------------------------
int KMFolderMbox::lock()
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
      cmd_str = "lockfile -l20 -r5 ";
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
        cmd_str = "lockfile -l20 -r5 " + indexLocation() + ".lock";
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
int KMFolderMbox::unlock()
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
KMFolder::IndexStatus KMFolderMbox::indexStatus()
{
  QFileInfo contInfo(location());
  QFileInfo indInfo(indexLocation());

  if (!contInfo.exists()) return KMFolder::IndexOk;
  if (!indInfo.exists()) return KMFolder::IndexMissing;

  return ( contInfo.lastModified() > indInfo.lastModified() )
         ? KMFolder::IndexTooOld
         : KMFolder::IndexOk;
}


//-----------------------------------------------------------------------------
int KMFolderMbox::createIndexFromContents()
{
  char line[MAX_LINE];
  char status[8], xstatus[8];
  QCString subjStr, dateStr, fromStr, toStr, xmarkStr, *lastStr=NULL;
  QCString replyToIdStr, referencesStr, msgIdStr;
  bool atEof = FALSE;
  bool inHeader = TRUE;
  KMMsgInfo* mi;
  QString msgStr;
  QRegExp regexp(MSG_SEPERATOR_REGEX);
  int i, num, numStatus;
  short needStatus;

  quiet(TRUE);
  assert(mStream != NULL);
  rewind(mStream);

  mMsgList.clear();

  num     = -1;
  numStatus= 11;
  off_t offs = 0;
  size_t size = 0;
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

  while (!atEof)
  {
    off_t pos = ftell(mStream);
    if (!fgets(line, MAX_LINE, mStream)) atEof = TRUE;

    if (atEof ||
	(strncmp(line,MSG_SEPERATOR_START, msgSepLen)==0 &&
	 regexp.search(line) >= 0))
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
        xmarkStr = QCString(line+14);
    else if (strncasecmp(line,"In-Reply-To:",12)==0 && isblank(line[12])) {
      int rightAngle;
      replyToIdStr = QCString(line+13);
      rightAngle = replyToIdStr.find( '>' );
      if (rightAngle != -1)
	replyToIdStr.truncate( rightAngle + 1 );
    }
    else if (strncasecmp(line,"References:",11)==0 && isblank(line[11])) {
      int leftAngle, rightAngle;
      referencesStr = QCString(line+12);
      leftAngle = referencesStr.findRev( '<' );
      if (leftAngle != -1)
	referencesStr = referencesStr.mid( leftAngle );
      rightAngle = referencesStr.find( '>' );
      if (rightAngle != -1)
	referencesStr.truncate( rightAngle + 1 );
    }
    else if (strncasecmp(line,"Message-Id:",11)==0 && isblank(line[11])) {
      int rightAngle;
      msgIdStr = QCString(line+12);
      rightAngle = msgIdStr.find( '>' );
      if (rightAngle != -1)
	msgIdStr.truncate( rightAngle + 1 );
    }
    else if (strncasecmp(line,"Date:",5)==0 && isblank(line[5]))
    {
        dateStr = QCString(line+6);
      lastStr = &dateStr;
    }
    else if (strncasecmp(line,"From:", 5)==0 &&
	     isblank(line[5]))
    {
        fromStr = QCString(line+6);
      lastStr = &fromStr;
    }
    else if (strncasecmp(line,"To:", 3)==0 &&
	     isblank(line[3]))
    {
        toStr = QCString(line+4);
      lastStr = &toStr;
    }
    else if (strncasecmp(line,"Subject:",8)==0 && isblank(line[8]))
    {
        subjStr = QCString(line+9);
      lastStr = &subjStr;
    }
  }

  if (mAutoCreateIndex)
  {
    emit statusMsg(i18n("Writing index file"));
    writeIndex();
  }
  else mHeaderOffset = 0;

  quiet(FALSE);
  correctUnreadMsgsCount();

  if (kernel->outboxFolder() == this && count() > 0)
    KMessageBox::information(0, i18n("Your outbox contains messages which were "
    "most likely not created by KMail.\nPlease remove them from there, if you "
    "don't want KMail to send them."));

  return 0;
}


//-----------------------------------------------------------------------------
KMMessage* KMFolderMbox::readMsg(int idx)
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
QCString& KMFolderMbox::getMsgString(int idx, QCString &mDest)
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
int KMFolderMbox::addMsg(KMMessage* aMsg, int* aIndex_ret)
{
  if (!canAddMsgNow(aMsg, aIndex_ret)) return 0;
  bool opened = FALSE;
  QCString msgText;
  char endStr[3];
  int idx = -1, rc;
  KMFolder* msgParent;
  bool editing = false;
  int growth = 0;

/* Then we can also disable it completely, this wastes time, at least for IMAP
  if (KMFolder::IndexOk != indexStatus()) {
      kdDebug(5006) << "Critical error: " << location() <<
	  " has been modified by an external application while KMail was running." << endl;
      //      exit(1); backed out due to broken nfs
  } */

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
	if (kernel->folderIsDraftOrOutbox(this))
          //special case for Edit message.
	  {
	    kdDebug(5006) << "Editing message in outbox or drafts" << endl;
	    editing = true;
	  }
	else
	  return 0;
      }

    idx = msgParent->find(aMsg);
    msgParent->getMsg( idx );
  }

  if (protocol() != "imap")
  {
/*
QFile fileD0( "testdat_xx-kmfoldermbox-0" );
if( fileD0.open( IO_WriteOnly ) ) {
    QDataStream ds( &fileD0 );
    ds.writeRawBytes( aMsg->asString(), aMsg->asString().length() );
    fileD0.close();  // If data is 0 we just create a zero length file.
}
*/  
    aMsg->setStatusFields();
/*  
QFile fileD1( "testdat_xx-kmfoldermbox-1" );
if( fileD1.open( IO_WriteOnly ) ) {
    QDataStream ds( &fileD1 );
    ds.writeRawBytes( aMsg->asString(), aMsg->asString().length() );
    fileD1.close();  // If data is 0 we just create a zero length file.
}
*/  
    if (aMsg->headerField("Content-Type").isEmpty())  // This might be added by
      aMsg->removeHeaderField("Content-Type");        // the line above
  }
  msgText = aMsg->asString();
  msgText.replace(QRegExp("\nFrom "),"\n>From ");
  size_t len = msgText.length();

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
  off_t revert = ftell(mStream);
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
  int error = ferror(mStream);
  if (error)
  {
    if (opened) close();
    return error;
  }

  fprintf(mStream, "From %s %s\n", (const char *)aMsg->fromEmail(),
          (const char *)aMsg->dateShortStr());
  off_t offs = ftell(mStream);
  fwrite(msgText, len, 1, mStream);
  if (msgText[(int)len-1]!='\n') fwrite("\n\n", 1, 2, mStream);
  fflush(mStream);
  size_t size = ftell(mStream) - offs;

  error = ferror(mStream);
  if (error) {
    kdDebug(5006) << "Error: Could not add message to folder (No space left on device?)" << endl;
    if (ftell(mStream) > revert) {
      kdDebug(5006) << "Undoing changes" << endl;
      truncate( location().local8Bit(), revert );
    }
    kernel->emergencyExit( i18n("Not enough free disk space.") );

    /* This code is not 100% reliable
    bool busy = kernel->kbp()->isBusy();
    if (busy) kernel->kbp()->idle();
    KMessageBox::sorry(0,
	  i18n("Unable to add message to folder.\n"
	       "(No space left on device or insufficient quota?)\n"
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
      aMsg->status()==KMMsgStatusNew ||
      (this == kernel->outboxFolder())) {
    if (mUnreadMsgs == -1) mUnreadMsgs = 1;
    else ++mUnreadMsgs;
    emit numUnreadMsgsChanged( this );
  }
  ++mTotalMsgs;

  // store information about the position in the folder file in the message
  aMsg->setParent(this);
  aMsg->setFolderOffset(offs);
  aMsg->setMsgSize(size);
  idx = mMsgList.append(aMsg);
  aMsg->setMsgSerNum();

  // change the length of the previous message to encompass white space added
  if ((idx > 0) && (growth > 0)) {
    // don't grow if a deleted message claims space at the end of the file
    if ((ulong)revert == mMsgList[idx - 1]->folderOffset() + mMsgList[idx - 1]->msgSize() )
      mMsgList[idx - 1]->setMsgSize( mMsgList[idx - 1]->msgSize() + growth );
  }

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

    error |= appendtoMsgDict(idx);

    if (error) {
      kdWarning(5006) << "Error: Could not add message to folder (No space left on device?)" << endl;
      if (ftell(mIndexStream) > revert) {
	kdWarning(5006) << "Undoing changes" << endl;
	truncate( indexLocation().local8Bit(), revert );
      }
      kernel->emergencyExit( i18n("Not enough free disk space.") );

      /* This code may not be 100% reliable
      bool busy = kernel->kbp()->isBusy();
      if (busy) kernel->kbp()->idle();
      KMessageBox::sorry(0,
        i18n("Unable to add message to folder.\n"
	     "(No space left on device or insufficient quota?)\n"
	     "Free space and sufficient quota are required to continue safely."));
      if (busy) kernel->kbp()->busy();
      if (opened) close();
      */
      return error;
    }
  }

  // some "paper work"
  if (aIndex_ret) *aIndex_ret = idx;
  if (!mQuiet) {
    emit msgAdded(idx);
    emit msgAdded(this);
  } else
    mChanged = TRUE;

  if (opened) close();

  // All streams have been flushed without errors if we arrive here
  // Return success!
  // (Don't return status of stream, it may have been closed already.)
  return 0;
}


//-----------------------------------------------------------------------------
int KMFolderMbox::compact()
{
  QString tempName;
  QString msgStr;
  int rc = 0;
  int openCount = mOpenCount;

  if (!needsCompact)
    return 0;

  if (!mCompactable) {
    kdDebug(5006) << location() << " compaction skipped." << endl;
    return 0;
  }
  kdDebug(5006) << "Compacting " << idString() << endl;

  if (KMFolder::IndexOk != indexStatus()) {
      kdDebug(5006) << "Critical error: " << location() <<
	  " has been modified by an external application while KMail was running." << endl;
      //      exit(1); backed out due to broken nfs
  }

  tempName = path() + "/." + name() + ".compacted";
  mode_t old_umask = umask(077);
  FILE *tmpfile = fopen(tempName.local8Bit(), "w");
  umask(old_umask);
  if (!tmpfile)
    return errno;
  open();

  KMMsgInfo* mi;
  size_t msize;
  off_t folder_offset;
  off_t offs=0;
  size_t msgs=0;
  QCString mtext;
  for(int idx = 0; idx < mMsgList.count(); idx++) {
    if(!(msgs++ % 10)) {
      msgStr = i18n("Compacting folder: %1 messages done").arg(msgs);
      emit statusMsg(msgStr);
    }
    mi = (KMMsgInfo*)mMsgList[idx];
    msize = mi->msgSize();
    if (mtext.size() < msize + 2)
      mtext.resize(msize+2);
    folder_offset = mi->folderOffset();

    //now we need to find the separator! grr...
    for(off_t i = folder_offset-25; TRUE; i -= 20) {
      off_t chunk_offset = i <= 0 ? 0 : i;
      if(fseek(mStream, chunk_offset, SEEK_SET) == -1) {
        rc = errno;
	break;
      }
      if (mtext.size() < 20)
	mtext.resize(20);
      fread(mtext.data(), 20, 1, mStream);
      if(i <= 0) { //woops we've reached the top of the file, last try..
	if(!strncasecmp(mtext.data(), "from ", 5)) {
	  if (mtext.size() < folder_offset)
	      mtext.resize(folder_offset);
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
	  if ((int)mtext.size() < size)
	      mtext.resize(size);
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
  if (!rc)
      rc = fflush(tmpfile);
  if (!rc)
      rc = fsync(fileno(tmpfile));
  rc |= fclose(tmpfile);
  if (!rc) {
    bool autoCreate = mAutoCreateIndex;
    ::rename(tempName.local8Bit(), location().local8Bit());
    writeIndex();
    writeConfig();
    mAutoCreateIndex = false;
    close(TRUE);
    mAutoCreateIndex = autoCreate;
  }
  else
  {
    close();
    kdDebug(5006) << "Error occurred while compacting" << endl;
    kdDebug(5006) << location() << endl;
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
void KMFolderMbox::setLockType( LockType ltype )
{
  mLockType = ltype;
}

//-----------------------------------------------------------------------------
void KMFolderMbox::setProcmailLockFileName( const QString &fname )
{
  mProcmailLockFileName = fname;
}

//-----------------------------------------------------------------------------
int KMFolderMbox::removeContents()
{
  int rc = 0;
  rc = unlink(location().local8Bit());
  return rc;
}

//-----------------------------------------------------------------------------
int KMFolderMbox::expungeContents()
{
  int rc = 0;
  if (truncate(location().local8Bit(), 0))
    rc = errno;
  return rc;
}

//-----------------------------------------------------------------------------
#include "kmfoldermbox.moc"
