// -*- c-basic-offset: 2 -*-
// kmfoldermbox.cpp
// Author: Stefan Taferner <taferner@alpin.or.at>

#include <config.h>
#include <qfileinfo.h>
#include <qregexp.h>

#include "kmfoldermbox.h"
#include "kmfoldermgr.h"
#include "undostack.h"
#include "kcursorsaver.h"

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <knotifyclient.h>
#include <kprocess.h>
#include <kconfig.h>

#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

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
  : KMFolderIndex(aParent, aName)
{
  mStream         = 0;
  mFilesLocked    = false;
  mLockType       = None;
}


//-----------------------------------------------------------------------------
KMFolderMbox::~KMFolderMbox()
{
  if (mOpenCount>0) close(true);
  if (kmkernel->undoStack()) kmkernel->undoStack()->folderDestroyed(this);
}

//-----------------------------------------------------------------------------
int KMFolderMbox::open()
{
  int rc = 0;

  mOpenCount++;
  if (mOpenCount > 1) return 0;  // already open

  assert(!name().isEmpty());

  mFilesLocked = false;
  mStream = fopen(QFile::encodeName(location()), "r+"); // messages file
  if (!mStream)
  {
    KNotifyClient::event( 0, "warning",
    i18n("Cannot open file \"%1\":\n%2").arg(location()).arg(strerror(errno)));
    kdDebug(5006) << "Cannot open folder `" << location() << "': " << strerror(errno) << endl;
    mOpenCount = 0;
    return errno;
  }

  lock();

  if (!path().isEmpty())
  {
     KMFolderIndex::IndexStatus index_status = indexStatus();
     // test if index file exists and is up-to-date
     if (KMFolderIndex::IndexOk != index_status)
     {
       // only show a warning if the index file exists, otherwise it can be
       // silently regenerated
       if (KMFolderIndex::IndexTooOld == index_status) {
        QString msg = i18n("<qt><p>The index of folder '%2' seems "
                           "to be out of date. To prevent message "
                           "corruption the index will be "
                           "regenerated. As a result deleted "
                           "messages might reappear and status "
                           "flags might be lost.</p>"
                           "<p>Please read the corresponding entry "
                           "in the <a href=\"%1\">FAQ section of the manual "
                           "of KMail</a> for "
                           "information about how to prevent this "
                           "problem from happening again.</p></qt>")
                      .arg("help:/kmail/faq.html#faq-index-regeneration")
                      .arg(name());
        // When KMail is starting up we have to show a non-blocking message
        // box so that the initialization can continue. We don't show a
        // queued message box when KMail isn't starting up because queued
        // message boxes don't have a "Don't ask again" checkbox.
        if (kmkernel->startingUp())
        {
          KConfigGroup configGroup( KMKernel::config(), "Notification Messages" );
          bool showMessage =
            configGroup.readBoolEntry( "showIndexRegenerationMessage", true );
          if (showMessage)
            KMessageBox::queuedMessageBox( 0, KMessageBox::Information,
                                           msg, i18n("Index Out of Date"),
                                           KMessageBox::AllowLink );
        }
        else
        {
            KCursorSaver idle(KBusyPtr::idle());
            KMessageBox::information( 0, msg, i18n("Index Out of Date"),
                                      "showIndexRegenerationMessage",
                                      KMessageBox::AllowLink );
        }
       }
       QString str;
       mIndexStream = 0;
       str = i18n("Folder `%1' changed. Recreating index.")
             .arg(name());
       emit statusMsg(str);
     } else {
       mIndexStream = fopen(QFile::encodeName(indexLocation()), "r+"); // index file
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
    mAutoCreateIndex = false;
    rc = createIndexFromContents();
  }

  mChanged = false;

  return rc;
}

//----------------------------------------------------------------------------
int KMFolderMbox::canAccess()
{
  assert(!name().isEmpty());

  if (access(QFile::encodeName(location()), R_OK | W_OK) != 0) {
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

  Q_UNUSED(imap);

  assert(!name().isEmpty());
  assert(mOpenCount == 0);

  kdDebug(5006) << "Creating folder " << name() << endl;
  if (access(QFile::encodeName(location()), F_OK) == 0) {
    kdDebug(5006) << "KMFolderMbox::create call to access function failed." << endl;
    kdDebug(5006) << "File:: " << endl;
    kdDebug(5006) << "Error " << endl;
    return EEXIST;
  }

  old_umask = umask(077);
  mStream = fopen(QFile::encodeName(location()), "w+"); //sven; open RW
  umask(old_umask);

  if (!mStream) return errno;

  if (!path().isEmpty())
  {
    old_umask = umask(077);
    mIndexStream = fopen(QFile::encodeName(indexLocation()), "w+"); //sven; open RW
        updateIndexStreamPtr(true);
    umask(old_umask);

    if (!mIndexStream) return errno;
  }
  else
  {
    mAutoCreateIndex = false;
  }

  mOpenCount++;
  mChanged = false;

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
  if ((this != kmkernel->inboxFolder()) && isSystemFolder() && !aForced)
  {
      mOpenCount = 1;
      return;
  }

  if (mAutoCreateIndex)
  {
      if (KMFolderIndex::IndexOk != indexStatus()) {
          kdDebug(5006) << "Critical error: " << location() <<
              " has been modified by an external application while KMail was running." << endl;
          //      exit(1); backed out due to broken nfs
      }

      updateIndex();
      writeConfig();
  }

  if (!noContent()) {
    if (mStream) unlock();
    mMsgList.clear(true);

    if (mStream) fclose(mStream);
    if (mIndexStream) {
      fclose(mIndexStream);
      updateIndexStreamPtr(true);
    }
  }
  mOpenCount   = 0;
  mStream      = 0;
  mIndexStream = 0;
  mFilesLocked = false;
  mUnreadMsgs  = -1;

  mMsgList.reset(INIT_MSGS);
}

//-----------------------------------------------------------------------------
void KMFolderMbox::sync()
{
  if (mOpenCount > 0)
    if (!mStream || fsync(fileno(mStream)) ||
        !mIndexStream || fsync(fileno(mIndexStream))) {
    kmkernel->emergencyExit( i18n("Could not sync index file <b>%1</b>: %2").arg( indexLocation() ).arg(errno ? QString::fromLocal8Bit(strerror(errno)) : i18n("Internal error. Please copy down the details and report a bug.")));
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
  QCString cmd_str;
  assert(mStream != 0);
  mFilesLocked = false;

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
        cmd_str += QFile::encodeName(KProcess::quote(mProcmailLockFileName));
      else
        cmd_str += QFile::encodeName(KProcess::quote(location() + ".lock"));

      rc = system( cmd_str.data() );
      if( rc != 0 )
      {
        kdDebug(5006) << "Cannot lock folder `" << location() << "': "
                  << strerror(rc) << " (" << rc << ")" << endl;
        return rc;
      }
      if( mIndexStream )
      {
        cmd_str = "lockfile -l20 -r5 " + QFile::encodeName(KProcess::quote(indexLocation() + ".lock"));
        rc = system( cmd_str.data() );
        if( rc != 0 )
        {
          kdDebug(5006) << "Cannot lock index of folder `" << location() << "': "
                    << strerror(rc) << " (" << rc << ")" << endl;
          return rc;
        }
      }
      break;

    case mutt_dotlock:
      cmd_str = "mutt_dotlock " + QFile::encodeName(KProcess::quote(location()));
      rc = system( cmd_str.data() );
      if( rc != 0 )
      {
        kdDebug(5006) << "Cannot lock folder `" << location() << "': "
                  << strerror(rc) << " (" << rc << ")" << endl;
        return rc;
      }
      if( mIndexStream )
      {
        cmd_str = "mutt_dotlock " + QFile::encodeName(KProcess::quote(indexLocation()));
        rc = system( cmd_str.data() );
        if( rc != 0 )
        {
          kdDebug(5006) << "Cannot lock index of folder `" << location() << "': "
                    << strerror(rc) << " (" << rc << ")" << endl;
          return rc;
        }
      }
      break;

    case mutt_dotlock_privileged:
      cmd_str = "mutt_dotlock -p " + QFile::encodeName(KProcess::quote(location()));
      rc = system( cmd_str.data() );
      if( rc != 0 )
      {
        kdDebug(5006) << "Cannot lock folder `" << location() << "': "
                  << strerror(rc) << " (" << rc << ")" << endl;
        return rc;
      }
      if( mIndexStream )
      {
        cmd_str = "mutt_dotlock -p " + QFile::encodeName(KProcess::quote(indexLocation()));
        rc = system( cmd_str.data() );
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


  mFilesLocked = true;
  return 0;
}

//-------------------------------------------------------------
FolderJob*
KMFolderMbox::doCreateJob( KMMessage *msg, FolderJob::JobType jt,
                           KMFolder *folder, QString, const AttachmentStrategy* ) const
{
  MboxJob *job = new MboxJob( msg, jt, folder );
  job->setParent( this );
  return job;
}

//-------------------------------------------------------------
FolderJob*
KMFolderMbox::doCreateJob( QPtrList<KMMessage>& msgList, const QString& sets,
                           FolderJob::JobType jt, KMFolder *folder ) const
{
  MboxJob *job = new MboxJob( msgList, sets, jt, folder );
  job->setParent( this );
  return job;
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
  QCString cmd_str;

  assert(mStream != 0);
  mFilesLocked = false;

  switch( mLockType )
  {
    case FCNTL:
      if (mIndexStream) fcntl(fileno(mIndexStream), F_SETLK, &fl);
      fcntl(fileno(mStream), F_SETLK, &fl);
      rc = errno;
      break;

    case procmail_lockfile:
      cmd_str = "rm -f ";
      if (!mProcmailLockFileName.isEmpty())
        cmd_str += QFile::encodeName(KProcess::quote(mProcmailLockFileName));
      else
        cmd_str += QFile::encodeName(KProcess::quote(location() + ".lock"));

      rc = system( cmd_str.data() );
      if( mIndexStream )
      {
        cmd_str = "rm -f " + QFile::encodeName(KProcess::quote(indexLocation() + ".lock"));
        rc = system( cmd_str.data() );
      }
      break;

    case mutt_dotlock:
      cmd_str = "mutt_dotlock -u " + QFile::encodeName(KProcess::quote(location()));
      rc = system( cmd_str.data() );
      if( mIndexStream )
      {
        cmd_str = "mutt_dotlock -u " + QFile::encodeName(KProcess::quote(indexLocation()));
        rc = system( cmd_str.data() );
      }
      break;

    case mutt_dotlock_privileged:
      cmd_str = "mutt_dotlock -p -u " + QFile::encodeName(KProcess::quote(location()));
      rc = system( cmd_str.data() );
      if( mIndexStream )
      {
        cmd_str = "mutt_dotlock -p -u " + QFile::encodeName(KProcess::quote(indexLocation()));
        rc = system( cmd_str.data() );
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
KMFolderIndex::IndexStatus KMFolderMbox::indexStatus()
{
  QFileInfo contInfo(location());
  QFileInfo indInfo(indexLocation());

  if (!contInfo.exists()) return KMFolderIndex::IndexOk;
  if (!indInfo.exists()) return KMFolderIndex::IndexMissing;

  // Check whether the mbox file is more than 5 seconds newer than the index
  // file. The 5 seconds are added to reduce the number of false alerts due
  // to slightly out of sync clocks of the NFS server and the local machine.
  return ( contInfo.lastModified() > indInfo.lastModified().addSecs(5) )
      ? KMFolderIndex::IndexTooOld
      : KMFolderIndex::IndexOk;
}


//-----------------------------------------------------------------------------
int KMFolderMbox::createIndexFromContents()
{
  char line[MAX_LINE];
  char status[8], xstatus[8];
  QCString subjStr, dateStr, fromStr, toStr, xmarkStr, *lastStr=0;
  QCString replyToIdStr, replyToAuxIdStr, referencesStr, msgIdStr;
  bool atEof = false;
  bool inHeader = true;
  KMMsgInfo* mi;
  QString msgStr;
  QRegExp regexp(MSG_SEPERATOR_REGEX);
  int i, num, numStatus;
  short needStatus;

  assert(mStream != 0);
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
  replyToAuxIdStr = "";
  referencesStr = "";
  msgIdStr = "";
  needStatus = 3;


  while (!atEof)
  {
    off_t pos = ftell(mStream);
    if (!fgets(line, MAX_LINE, mStream)) atEof = true;

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
          msgStr = i18n("Creating index file: one message done", "Creating index file: %n messages done", num);
          emit statusMsg(msgStr);
          numStatus = 10;
        }

        if (size > 0)
        {
          msgIdStr = msgIdStr.stripWhiteSpace();
          if( !msgIdStr.isEmpty() ) {
            int rightAngle;
            rightAngle = msgIdStr.find( '>' );
            if( rightAngle != -1 )
              msgIdStr.truncate( rightAngle + 1 );
          }

          replyToIdStr = replyToIdStr.stripWhiteSpace();
          if( !replyToIdStr.isEmpty() ) {
            int rightAngle;
            rightAngle = replyToIdStr.find( '>' );
            if( rightAngle != -1 )
              replyToIdStr.truncate( rightAngle + 1 );
          }

          referencesStr = referencesStr.stripWhiteSpace();
          if( !referencesStr.isEmpty() ) {
            int leftAngle, rightAngle;
            leftAngle = referencesStr.findRev( '<' );
            if( ( leftAngle != -1 )
                && ( replyToIdStr.isEmpty() || ( replyToIdStr[0] != '<' ) ) ) {
              // use the last reference, instead of missing In-Reply-To
              replyToIdStr = referencesStr.mid( leftAngle );
            }

            // find second last reference
            leftAngle = referencesStr.findRev( '<', leftAngle - 1 );
            if( leftAngle != -1 )
              referencesStr = referencesStr.mid( leftAngle );
            rightAngle = referencesStr.findRev( '>' );
            if( rightAngle != -1 )
              referencesStr.truncate( rightAngle + 1 );

            // Store the second to last reference in the replyToAuxIdStr
            // It is a good candidate for threading the message below if the
            // message In-Reply-To points to is not kept in this folder,
            // but e.g. in an Outbox
            replyToAuxIdStr = referencesStr;
            rightAngle = referencesStr.find( '>' );
            if( rightAngle != -1 )
              replyToAuxIdStr.truncate( rightAngle + 1 );
          }

          mi = new KMMsgInfo(this);
          mi->init( subjStr.stripWhiteSpace(),
                    fromStr.stripWhiteSpace(),
                    toStr.stripWhiteSpace(),
                    0, KMMsgStatusNew,
                    xmarkStr.stripWhiteSpace(),
                    replyToIdStr, replyToAuxIdStr, msgIdStr,
                    KMMsgEncryptionStateUnknown, KMMsgSignatureStateUnknown,
                    KMMsgMDNStateUnknown, offs, size );
          mi->setStatus(status, xstatus);
          mi->setDate( dateStr.stripWhiteSpace() );
          mi->setDirty(false);
          mMsgList.append(mi);

          *status = '\0';
          *xstatus = '\0';
          needStatus = 3;
          xmarkStr = "";
          replyToIdStr = "";
          replyToAuxIdStr = "";
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
      inHeader = true;
      continue;
    }
    // Is this a long header line?
    if (inHeader && (line[0]=='\t' || line[0]==' '))
    {
      i = 0;
      while (line [i]=='\t' || line [i]==' ') i++;
      if (line [i] < ' ' && line [i]>0) inHeader = false;
      else if (lastStr) *lastStr += line + i;
    }
    else lastStr = 0;

    if (inHeader && (line [0]=='\n' || line [0]=='\r'))
      inHeader = false;
    if (!inHeader) continue;

    /* -sanders Make all messages read when auto-recreating index */
    /* Reverted, as it breaks reading the sent mail status, for example.
       -till */
    if ((needStatus & 1) && strncasecmp(line, "Status:", 7) == 0)
    {
      for(i=0; i<4 && line[i+8] > ' '; i++)
        status[i] = line[i+8];
      status[i] = '\0';
      needStatus &= ~1;
    }
    else if ((needStatus & 2) && strncasecmp(line, "X-Status:", 9)==0)
    {
      for(i=0; i<4 && line[i+10] > ' '; i++)
        xstatus[i] = line[i+10];
      xstatus[i] = '\0';
      needStatus &= ~2;
    }
    else if (strncasecmp(line,"X-KMail-Mark:",13)==0)
        xmarkStr = QCString(line+13);
    else if (strncasecmp(line,"In-Reply-To:",12)==0) {
      replyToIdStr = QCString(line+12);
      lastStr = &replyToIdStr;
    }
    else if (strncasecmp(line,"References:",11)==0) {
      referencesStr = QCString(line+11);
      lastStr = &referencesStr;
    }
    else if (strncasecmp(line,"Message-Id:",11)==0) {
      msgIdStr = QCString(line+11);
      lastStr = &msgIdStr;
    }
    else if (strncasecmp(line,"Date:",5)==0)
    {
      dateStr = QCString(line+5);
      lastStr = &dateStr;
    }
    else if (strncasecmp(line,"From:", 5)==0)
    {
      fromStr = QCString(line+5);
      lastStr = &fromStr;
    }
    else if (strncasecmp(line,"To:", 3)==0)
    {
      toStr = QCString(line+3);
      lastStr = &toStr;
    }
    else if (strncasecmp(line,"Subject:",8)==0)
    {
      subjStr = QCString(line+8);
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

  if (kmkernel->outboxFolder() == this && count() > 0)
    KMessageBox::queuedMessageBox(0, KMessageBox::Information,
                                  i18n("Your outbox contains messages which were "
    "most likely not created by KMail.\nPlease remove them from there, if you "
    "don't want KMail to send them."));

  if ( parent() )
      parent()->manager()->invalidateFolder(kmkernel->msgDict(), this);
  return 0;
}


//-----------------------------------------------------------------------------
KMMessage* KMFolderMbox::readMsg(int idx)
{
  KMMsgInfo* mi = (KMMsgInfo*)mMsgList[idx];

  assert(mi!=0 && !mi->isMessage());
  assert(mStream != 0);

  KMMessage* msg = new KMMessage(*mi);
  msg->fromDwString( getDwString( idx ) );
  mMsgList.set(idx,&msg->toMsgBase());

  return msg;
}


#define STRDIM(x) (sizeof(x)/sizeof(*x)-1)
// performs (\n|^)>{n}From_ -> \1>{n-1}From_ conversion
static size_t unescapeFrom( char* str, size_t strLen ) {
  if ( !str )
    return 0;
  if ( strLen <= STRDIM(">From ") )
    return strLen;

  // yes, *d++ = *s++ is a no-op as long as d == s (until after the
  // first >From_), but writes are cheap compared to reads and the
  // data is already in the cache from the read, so special-casing
  // might even be slower...
  const char * s = str;
  char * d = str;
  const char * const e = str + strLen - STRDIM(">From ");

  while ( s < e ) {
    if ( *s == '\n' && *(s+1) == '>' ) { // we can do the lookahead, since e is 6 chars from the end!
      *d++ = *s++;  // == '\n'
      *d++ = *s++;  // == '>'
      while ( s < e && *s == '>' )
        *d++ = *s++;
      if ( qstrncmp( s, "From ", STRDIM("From ") ) == 0 )
        --d;
    }
    *d++ = *s++; // yes, s might be e here, but e is not the end :-)
  }
  // copy the rest:
  while ( s < str + strLen )
    *d++ = *s++;
  if ( d < s ) // only NUL-terminate if it's shorter
    *d = 0;

  return d - str;
}

//static
QCString KMFolderMbox::escapeFrom( const QCString & str ) {
  const unsigned int strLen = str.length();
  if ( strLen <= STRDIM("From ") )
    return str;
  // worst case: \nFrom_\nFrom_\nFrom_... => grows to 7/6
  QCString result( int( strLen + 5 ) / 6 * 7 + 1 );

  const char * s = str.data();
  const char * const e = s + strLen - STRDIM("From ");
  char * d = result.data();

  bool onlyAnglesAfterLF = false; // dont' match ^From_
  while ( s < e ) {
    switch ( *s ) {
    case '\n':
      onlyAnglesAfterLF = true;
      break;
    case '>':
      break;
    case 'F':
      if ( onlyAnglesAfterLF && qstrncmp( s+1, "rom ", STRDIM("rom ") ) == 0 )
        *d++ = '>';
      // fall through
    default:
      onlyAnglesAfterLF = false;
      break;
    }
    *d++ = *s++;
  }
  while ( s < str.data() + strLen )
    *d++ = *s++;

  result.truncate( d - result.data() );
  return result;
}

#undef STRDIM

//-----------------------------------------------------------------------------
QCString& KMFolderMbox::getMsgString(int idx, QCString &mDest)
{
  unsigned long msgSize;
  KMMsgInfo* mi = (KMMsgInfo*)mMsgList[idx];

  assert(mi!=0);
  assert(mStream != 0);

  msgSize = mi->msgSize();
  mDest.resize(msgSize+2);

  fseek(mStream, mi->folderOffset(), SEEK_SET);
  fread(mDest.data(), msgSize, 1, mStream);
  mDest[msgSize] = '\0';

  size_t newMsgSize = unescapeFrom( mDest.data(), msgSize );
  newMsgSize = crlf2lf( mDest.data(), newMsgSize );

  return mDest;
}


//-----------------------------------------------------------------------------
DwString KMFolderMbox::getDwString(int idx)
{
  KMMsgInfo* mi = (KMMsgInfo*)mMsgList[idx];

  assert(mi!=0);
  assert(mStream != 0);

  size_t msgSize = mi->msgSize();
  char* msgText = new char[ msgSize + 1 ];

  fseek(mStream, mi->folderOffset(), SEEK_SET);
  fread(msgText, msgSize, 1, mStream);
  msgText[msgSize] = '\0';

  size_t newMsgSize = unescapeFrom( msgText, msgSize );
  newMsgSize = crlf2lf( msgText, newMsgSize );

  DwString msgStr;
  // the DwString takes possession of msgText, so we must not delete msgText
  msgStr.TakeBuffer( msgText, msgSize + 1, 0, newMsgSize );
  return msgStr;
}


//-----------------------------------------------------------------------------
int KMFolderMbox::addMsg(KMMessage* aMsg, int* aIndex_ret)
{
  if (!canAddMsgNow(aMsg, aIndex_ret)) return 0;
  bool opened = false;
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
    opened = true;
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
        if (kmkernel->folderIsDraftOrOutbox(this))
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

  if (folderType() != KMFolderTypeImap)
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
  escapeFrom( msgText );
  size_t len = msgText.length();

  assert(mStream != 0);
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

  QCString address( aMsg->fromEmail() );
  if ( address.isEmpty() )
    address = "unknown@unknown.invalid";
  fprintf(mStream, "From %s %s\n", address.data(),
          (const char *)aMsg->dateShortStr());
  off_t offs = ftell(mStream);
  fwrite(msgText, len, 1, mStream);
  if (msgText[(int)len-1]!='\n') fwrite("\n\n", 1, 2, mStream);
  fflush(mStream);
  size_t size = ftell(mStream) - offs;

  error = ferror(mStream);
  if (error) {
    kdDebug(5006) << "Error: Could not add message to folder: " << strerror(errno) << endl;
    if (ftell(mStream) > revert) {
      kdDebug(5006) << "Undoing changes" << endl;
      truncate( QFile::encodeName(location()), revert );
    }
    kmkernel->emergencyExit( i18n("Could not add message to folder: ") + QString::fromLocal8Bit(strerror(errno)));

    /* This code is not 100% reliable
    bool busy = kmkernel->kbp()->isBusy();
    if (busy) kmkernel->kbp()->idle();
    KMessageBox::sorry(0,
          i18n("Unable to add message to folder.\n"
               "(No space left on device or insufficient quota?)\n"
               "Free space and sufficient quota are required to continue safely."));
    if (busy) kmkernel->kbp()->busy();
    if (opened) close();
    kmkernel->kbp()->idle();
    */
    return error;
  }

  if (msgParent) {
    if (idx >= 0) msgParent->take(idx);
  }
//  if (mAccount) aMsg->removeHeaderField("X-UID");

  if (aMsg->isUnread() || aMsg->isNew() ||
      (this == kmkernel->outboxFolder())) {
    if (mUnreadMsgs == -1) mUnreadMsgs = 1;
    else ++mUnreadMsgs;
    emit numUnreadMsgsChanged( this );
  }
  ++mTotalMsgs;

  // store information about the position in the folder file in the message
  aMsg->setParent(this);
  aMsg->setFolderOffset(offs);
  aMsg->setMsgSize(size);
  idx = mMsgList.append(&aMsg->toMsgBase());
  if (aMsg->getMsgSerNum() <= 0)
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
    assert(mIndexStream != 0);
    clearerr(mIndexStream);
    fseek(mIndexStream, 0, SEEK_END);
    revert = ftell(mIndexStream);

    KMMsgBase * mb = &aMsg->toMsgBase();
        int len;
        const uchar *buffer = mb->asIndexString(len);
        fwrite(&len,sizeof(len), 1, mIndexStream);
        mb->setIndexOffset( ftell(mIndexStream) );
        mb->setIndexLength( len );
        if(fwrite(buffer, len, 1, mIndexStream) != 1)
            kdDebug(5006) << "Whoa! " << __FILE__ << ":" << __LINE__ << endl;

    fflush(mIndexStream);
    error = ferror(mIndexStream);

    error |= appendtoMsgDict(idx);

    if (error) {
      kdWarning(5006) << "Error: Could not add message to folder (No space left on device?)" << endl;
      if (ftell(mIndexStream) > revert) {
        kdWarning(5006) << "Undoing changes" << endl;
        truncate( QFile::encodeName(indexLocation()), revert );
      }
      if ( errno )
        kmkernel->emergencyExit( i18n("Could not add message to folder:") + QString::fromLocal8Bit(strerror(errno)));
      else
        kmkernel->emergencyExit( i18n("Could not add message to folder (No space left on device?)") );

      /* This code may not be 100% reliable
      bool busy = kmkernel->kbp()->isBusy();
      if (busy) kmkernel->kbp()->idle();
      KMessageBox::sorry(0,
        i18n("Unable to add message to folder.\n"
             "(No space left on device or insufficient quota?)\n"
             "Free space and sufficient quota are required to continue safely."));
      if (busy) kmkernel->kbp()->busy();
      if (opened) close();
      */
      return error;
    }
  }

  // some "paper work"
  if (aIndex_ret) *aIndex_ret = idx;
  emitMsgAddedSignals(idx);
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

  if (KMFolderIndex::IndexOk != indexStatus()) {
      kdDebug(5006) << "Critical error: " << location() <<
          " has been modified by an external application while KMail was running." << endl;
      //      exit(1); backed out due to broken nfs
  }

  tempName = path() + "/." + name() + ".compacted";
  mode_t old_umask = umask(077);
  FILE *tmpfile = fopen(QFile::encodeName(tempName), "w");
  umask(old_umask);
  if (!tmpfile)
    return errno;
  open();

  KMMsgInfo* mi;
  size_t msize;
  off_t folder_offset;
  off_t offs=0;
  int msgs=0;
  QCString mtext;
  for(unsigned int idx = 0; idx < mMsgList.count(); idx++) {
    if(!(msgs++ % 10)) {
      msgStr = i18n("Compacting folder: one message done",
                                "Compacting folder: %n messages done", msgs);
      if (!kmkernel->shuttingDown())
          emit statusMsg(msgStr);
    }
    mi = (KMMsgInfo*)mMsgList.at(idx);
    msize = mi->msgSize();
    if (mtext.size() < msize + 2)
      mtext.resize(msize+2);
    folder_offset = mi->folderOffset();

    //now we need to find the separator! grr...
    for(off_t i = folder_offset-25; true; i -= 20) {
      off_t chunk_offset = i <= 0 ? 0 : i;
      if(fseek(mStream, chunk_offset, SEEK_SET) == -1) {
        rc = errno;
        break;
      }
      if (mtext.size() < 20)
        mtext.resize(20);
      fread(mtext.data(), 20, 1, mStream);
      if(i <= 0) { //woops we've reached the top of the file, last try..
        if ( mtext.contains( "from ", false ) ) {
          if (mtext.size() < (size_t)folder_offset)
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
    QFileInfo inf(location());
    QString box;
    if (inf.isSymLink())
      box = inf.readLink();
    if (!box)
      box = location();
    ::rename(QFile::encodeName(tempName), QFile::encodeName(box));
    writeIndex();
    writeConfig();
    mAutoCreateIndex = false;
    close(true);
    mAutoCreateIndex = autoCreate;
    needsCompact = false;             // We are clean now
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
  emit changed();
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
  rc = unlink(QFile::encodeName(location()));
  return rc;
}

//-----------------------------------------------------------------------------
int KMFolderMbox::expungeContents()
{
  int rc = 0;
  if (truncate(QFile::encodeName(location()), 0))
    rc = errno;
  return rc;
}

//-----------------------------------------------------------------------------
#include "kmfoldermbox.moc"
