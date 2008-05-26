// -*- mode: C++; c-file-style: "gnu" -*-
// kmfoldermaildir.cpp
// Author: Kurt Granroth <granroth@kde.org>

#include <QDir>
#include <QRegExp>
#include <QByteArray>
#include <QFileInfo>

#include <kpimutils/kfileio.h>
#include "kmfoldermaildir.h"
#include "kmfoldermgr.h"
#include "kmfolder.h"
#include "undostack.h"
#include "maildirjob.h"
#include "kcursorsaver.h"
#include "jobscheduler.h"
using KMail::MaildirJob;
#include "compactionjob.h"
#include "kmmsgdict.h"
#include "util.h"

#include <kio/directorysizejob.h>

#include <kdebug.h>
#include <kde_file.h>
#include <kfileitem.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <krandom.h>

#include <QDateTime>
#include <QPointer>

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <limits.h>
#include <fcntl.h>

#ifndef MAX_LINE
#define MAX_LINE 4096
#endif
#ifndef INIT_MSGS
#define INIT_MSGS 8
#endif

using KPIMUtils::removeDirAndContentsRecursively;

// A separator for "uniq:info" (see the original maildir specification
// at http://cr.yp.to/proto/maildir.html.
// Windows uses '!' charater instead as ':' is not supported by the OS.
// TODO make it configurable - jstaniek
// TODO check what the choice for Thunderbird 3 - jstaniek
#ifdef Q_WS_WIN
#define KMAIL_MAILDIR_FNAME_SEPARATOR "!"
#else
#define KMAIL_MAILDIR_FNAME_SEPARATOR ":"
#endif

#ifdef KMAIL_SQLITE_INDEX
#include <sqlite3.h>
#endif

//-----------------------------------------------------------------------------
KMFolderMaildir::KMFolderMaildir(KMFolder* folder, const char* name)
  : KMFolderIndex(folder, name), mCurrentlyCheckingFolderSize(false)
{

}


//-----------------------------------------------------------------------------
KMFolderMaildir::~KMFolderMaildir()
{
  if ( mOpenCount > 0 ) {
    close( "~foldermaildir", true );
  }
  if ( kmkernel->undoStack() ) {
    kmkernel->undoStack()->folderDestroyed( folder() );
  }
}

//-----------------------------------------------------------------------------
bool KMFolderMaildir::canAccess() const
{
  assert(!folder()->name().isEmpty());

  QString sBadFolderName;
  QStringList files;
  files << "" << "/new" << "/cur" << "/tmp";
  foreach( const QString& fname, files ) {
    QFileInfo finfo( location() + fname );
    if ( !finfo.isDir() || !finfo.isReadable() || !finfo.isWritable() ) {
      sBadFolderName = location() + fname;
      break;
    }
  }

  if ( sBadFolderName.isEmpty() )
    return true;

  KCursorSaver idle(KBusyPtr::idle());
  if ( !QFile::exists(sBadFolderName) )
    KMessageBox::sorry(0, i18n("Error opening %1; this folder is missing.",
                        sBadFolderName));
  else
    KMessageBox::sorry(0, i18n("Error opening %1; either this is not a valid "
                               "maildir folder, or you do not have sufficient access permissions.",
                        sBadFolderName));
  return false;
}

//-----------------------------------------------------------------------------
int KMFolderMaildir::open( const char * )
{
  mOpenCount++;
  kmkernel->jobScheduler()->notifyOpeningFolder( folder() );

  if (mOpenCount > 1) return 0;  // already open

  assert(!folder()->name().isEmpty());

  if ( !canAccess() )
    return 1;

  int rc = openInternal( NoOptions );
/* moved to openInternal()
  if (!folder()->path().isEmpty())
  {
    bool shouldCreateIndexFromContents = false;
    if (KMFolderIndex::IndexOk != indexStatus()) // test if contents file has changed
    {
#ifdef KMAIL_SQLITE_INDEX
#else
      mIndexStream = 0;
#endif
      shouldCreateIndexFromContents = true;
      emit statusMsg( i18n("Folder `%1' changed; recreating index.", objectName()) );
    } else {
#ifdef KMAIL_SQLITE_INDEX
#else
      mIndexStream = KDE_fopen(QFile::encodeName(indexLocation()), "r+"); // index file
      kDebug( StorageDebug ) << "KDE_fopen(indexLocation()=" << indexLocation() << ", \"r+\") == mIndexStream == " << mIndexStream;
      if ( mIndexStream ) {
# ifndef Q_WS_WIN
        fcntl(fileno(mIndexStream), F_SETFD, FD_CLOEXEC);
# endif
        if (!updateIndexStreamPtr())
          return 1;
      }
      else
        shouldCreateIndexFromContents = true;
#endif
    }

    if ( shouldCreateIndexFromContents )
      rc = createIndexFromContents();
    else
      rc = readIndex() ? 0 : 1;
  }
  else
  {
    mAutoCreateIndex = false;
    rc = createIndexFromContents();
  }

  mChanged = false;*/

  //readConfig();

  return rc;
}

//-----------------------------------------------------------------------------
int KMFolderMaildir::createMaildirFolders( const QString &folderPath )
{
  // Make sure that neither a new, cur or tmp subfolder exists already.
  QFileInfo dirinfo;
  dirinfo.setFile( folderPath + "/new" );
  if ( dirinfo.exists() ) {
    return EEXIST;
  }

  dirinfo.setFile( folderPath + "/cur" );
  if ( dirinfo.exists() ) {
    return EEXIST;
  }

  dirinfo.setFile( folderPath + "/tmp" );
  if ( dirinfo.exists() ) {
    return EEXIST;
  }

  // create the maildir directory structure
  if ( KDE_mkdir( QFile::encodeName( folderPath ), S_IRWXU ) > 0 ) {
    kDebug(5006) << "Could not create folder" << folderPath;
    return errno;
  }
  if ( KDE_mkdir( QFile::encodeName( folderPath + "/new" ), S_IRWXU ) > 0 ) {
    kDebug(5006) << "Could not create folder" << folderPath << "/new";
    return errno;
  }
  if ( KDE_mkdir( QFile::encodeName( folderPath + "/cur" ), S_IRWXU ) > 0 ) {
    kDebug(5006) << "Could not create folder" << folderPath << "/cur";
    return errno;
  }
  if ( KDE_mkdir( QFile::encodeName( folderPath + "/tmp" ), S_IRWXU ) > 0 ) {
    kDebug(5006) << "Could not create folder" << folderPath << "/tmp";
    return errno;
  }

  return 0; // no error
}

//-----------------------------------------------------------------------------
int KMFolderMaildir::create()
{
  assert(!folder()->name().isEmpty());
  assert(mOpenCount == 0);

  int rc = createMaildirFolders( location() );
  if ( rc != 0 )
    return rc;

  // FIXME no path == no index? - till
  return createInternal();
}


//-----------------------------------------------------------------------------
void KMFolderMaildir::close( const char *,  bool aForced )
{
  if (mOpenCount <= 0) return;
  if (mOpenCount > 0) mOpenCount--;

  if (mOpenCount > 0 && !aForced) return;

#if 0 // removed hack that prevented closing system folders (see kmail-devel discussion about mail expiring)
  if ( (folder() != kmkernel->inboxFolder())
       && folder()->isSystemFolder() && !aForced)
  {
     mOpenCount = 1;
     return;
  }
#endif

  if (mAutoCreateIndex)
  {
      updateIndex();
      writeConfig();
  }

  mMsgList.clear(true);

#ifdef KMAIL_SQLITE_INDEX
  if ( mIndexDb ) {
    sqlite3_close( mIndexDb );
    mIndexDb = 0;
  }
#else
    if (mIndexStream) {
	fclose(mIndexStream);
      kDebug( StorageDebug ) << "fclose(mIndexStream = " << mIndexStream << ")";
	updateIndexStreamPtr(true);
    }
  mIndexStream = 0;
#endif

  mOpenCount   = 0;
  mUnreadMsgs  = -1;

  mMsgList.reset(INIT_MSGS);
}

//-----------------------------------------------------------------------------
void KMFolderMaildir::sync()
{
#ifdef KMAIL_SQLITE_INDEX
#else
  if (mOpenCount > 0)
    if (!mIndexStream || fsync(fileno(mIndexStream))) {
    kmkernel->emergencyExit( i18n("Could not sync maildir folder.") );
    }
#endif
}

//-----------------------------------------------------------------------------
int KMFolderMaildir::expungeContents()
{
  // nuke all messages in this folder now
  QDir d(location() + "/new");
  // d.setFilter(QDir::Files); coolo: QFile::remove returns false for non-files
  QStringList files(d.entryList());
  QStringList::ConstIterator it(files.begin());
  for ( ; it != files.end(); ++it)
    QFile::remove(d.filePath(*it));

  d.setPath(location() + "/cur");
  files = d.entryList();
  for (it = files.begin(); it != files.end(); ++it)
    QFile::remove(d.filePath(*it));

  return 0;
}

int KMFolderMaildir::compact( unsigned int startIndex, int nbMessages, const QStringList& entryList, bool& done )
{
  QString subdirNew(location() + "/new/");
  QString subdirCur(location() + "/cur/");

  unsigned int stopIndex = nbMessages == -1 ? mMsgList.count() :
                           qMin( mMsgList.count(), startIndex + nbMessages );
  //kDebug(5006) <<"KMFolderMaildir: compacting from" << startIndex <<" to" << stopIndex;
  for(unsigned int idx = startIndex; idx < stopIndex; ++idx) {
    KMMsgInfo* mi = (KMMsgInfo*)mMsgList.at(idx);
    if (!mi)
      continue;

    QString filename(mi->fileName());
    if (filename.isEmpty())
      continue;

    // first, make sure this isn't in the 'new' subdir
    if ( entryList.contains( filename ) )
      moveInternal(subdirNew + filename, subdirCur + filename, mi);

    // construct a valid filename.  if it's already valid, then
    // nothing happens
    filename = constructValidFileName( filename, mi->messageStatus() );

    // if the name changed, then we need to update the actual filename
    if (filename != mi->fileName())
    {
      moveInternal(subdirCur + mi->fileName(), subdirCur + filename, mi);
      mi->setFileName(filename);
      setDirty( true );
    }

#if 0
    // we can't have any New messages at this point
    if (mi->isNew())
    {
      mi->setStatus(KMMsgStatusUnread);
      setDirty( true );
    }
#endif
  }
  done = ( stopIndex == mMsgList.count() );
  return 0;
}

//-----------------------------------------------------------------------------
int KMFolderMaildir::compact( bool silent )
{
  KMail::MaildirCompactionJob* job = new KMail::MaildirCompactionJob( folder(), true /*immediate*/ );
  int rc = job->executeNow( silent );
  // Note that job autodeletes itself.
  return rc;
}

//-------------------------------------------------------------
FolderJob*
KMFolderMaildir::doCreateJob( KMMessage *msg, FolderJob::JobType jt,
                              KMFolder *folder,  const QString&, const AttachmentStrategy* ) const
{
  MaildirJob *job = new MaildirJob( msg, jt, folder );
  job->setParentFolder( this );
  return job;
}

//-------------------------------------------------------------
FolderJob*
KMFolderMaildir::doCreateJob( QList<KMMessage*>& msgList, const QString& sets,
                              FolderJob::JobType jt, KMFolder *folder ) const
{
  MaildirJob *job = new MaildirJob( msgList, sets, jt, folder );
  job->setParentFolder( this );
  return job;
}

//-------------------------------------------------------------
int KMFolderMaildir::addMsg(KMMessage* aMsg, int* index_return)
{
  if (!canAddMsgNow(aMsg, index_return)) return 0;
  return addMsgInternal( aMsg, index_return );
}

//-------------------------------------------------------------
int KMFolderMaildir::addMsgInternal( KMMessage* aMsg, int* index_return,
                                     bool stripUid )
{
/*
QFile fileD0( "testdat_xx-kmfoldermaildir-0" );
if( fileD0.open( QIODevice::WriteOnly ) ) {
    QDataStream ds( &fileD0 );
    ds.writeRawData( aMsg->asString(), aMsg->asString().length() );
    fileD0.close();  // If data is 0 we just create a zero length file.
}
*/
  int idx(-1);

  // take message out of the folder it is currently in, if any
  KMFolder* msgParent = aMsg->parent();
  if (msgParent)
  {
    if (msgParent==folder() && !kmkernel->folderIsDraftOrOutbox(folder()))
        return 0;

    idx = msgParent->find(aMsg);
    msgParent->getMsg( idx );
  }

  aMsg->setStatusFields();
  if (aMsg->headerField("Content-Type").isEmpty())  // This might be added by
    aMsg->removeHeaderField("Content-Type");        // the line above


  const QString uidHeader = aMsg->headerField( "X-UID" );
  if ( !uidHeader.isEmpty() && stripUid )
    aMsg->removeHeaderField( "X-UID" );

  const QByteArray msgText = aMsg->asString();

  // Re-add the uid so that the take can make use of it, in case the
  // message is currently in an imap folder
  if ( !uidHeader.isEmpty() && stripUid )
    aMsg->setHeaderField( "X-UID", uidHeader );

  if ( msgText.length() <= 0 ) {
    kDebug(5006) <<"Message added to folder `" << objectName() <<"' contains no data. Ignoring it.";
    return 0;
  }

  // make sure the filename has the correct extension
  QString filename = constructValidFileName( aMsg->fileName(), aMsg->messageStatus() );

  QString tmp_file(location() + "/tmp/");
  tmp_file += filename;

  if ( ! KPIMUtils::kByteArrayToFile( msgText, tmp_file, false, false, false ) )
    kmkernel->emergencyExit( i18n("Message could not be added to the folder, possibly disk space is low.") );

  QFile file(tmp_file);

  KMFolderOpener openThis(folder(), "maildir");
  if (openThis.openResult())
  {
    kDebug(5006) << openThis.openResult() << "of folder:" << label();
    return openThis.openResult();
  }

  // now move the file to the correct location
  QString new_loc(location() + "/cur/");
  new_loc += filename;
  if (moveInternal(tmp_file, new_loc, filename, aMsg->messageStatus()).isNull())
  {
    file.remove();
    return -1;
  }

  if (msgParent && idx >= 0)
    msgParent->take(idx);

  // just to be sure it does not end up in the index
  if ( stripUid ) aMsg->setUID( 0 );

  if (filename != aMsg->fileName())
    aMsg->setFileName(filename);

  if ( aMsg->status().isUnread() || aMsg->status().isNew() ||
       folder() == kmkernel->outboxFolder())
  {
    if (mUnreadMsgs == -1)
      mUnreadMsgs = 1;
    else
      ++mUnreadMsgs;
    if ( !mQuiet ) {
      kDebug( 5006 ) << "FolderStorage::msgStatusChanged";
      emit numUnreadMsgsChanged( folder() );
    }else{
      if ( !mEmitChangedTimer->isActive() ) {
//        kDebug( 5006 )<<"QuietTimer started";
        mEmitChangedTimer->start( 3000 );
      }
      mChanged = true;
    }
  }
  ++mTotalMsgs;
  mSize = -1;

  if ( aMsg->attachmentState() == KMMsgAttachmentUnknown &&
       aMsg->readyToShow() )
    aMsg->updateAttachmentState();

  // store information about the position in the folder file in the message
  aMsg->setParent(folder());
  aMsg->setMsgSize( msgText.length() );
  idx = mMsgList.append( &aMsg->toMsgBase(), mExportsSernums );
  const unsigned long msgSerNum = aMsg->getMsgSerNum();
  kDebug( Test1Area ) << "getMsgSerNum:" << msgSerNum;
  if ( msgSerNum <= 0 )
    aMsg->setMsgSerNum();
  else
    replaceMsgSerNum( msgSerNum, &aMsg->toMsgBase(), idx );

  // write index entry if desired
  if (mAutoCreateIndex)
  {
#ifdef KMAIL_SQLITE_INDEX
    // reset the db id, in case we have one, we are about to change folders
    // and can't reuse it there
    aMsg->setDbId( 0 );
    aMsg->setDirty( true );
#else
    assert(mIndexStream != 0);
    clearerr(mIndexStream);
    KDE_fseek(mIndexStream, 0, SEEK_END);
    off_t revert = KDE_ftell(mIndexStream);
#endif

    KMMsgBase * mb = &aMsg->toMsgBase();
    int error = writeMessages( mb, true /*flush*/ );

    if ( mExportsSernums )
      error |= appendToFolderIdsFile( idx );

    if ( error ) {
      kDebug(5006) << "Error: Could not add message to folder (No space left on device?)";
#ifdef KMAIL_SQLITE_INDEX
#else
      if ( KDE_ftell( mIndexStream ) > revert ) {
	kDebug(5006) << "Undoing changes";
	truncate( QFile::encodeName( indexLocation() ), revert );
      }
#endif
      kmkernel->emergencyExit(i18n("KMFolderMaildir::addMsg: abnormally terminating to prevent data loss."));
      // exit(1); // don't ever use exit(), use the above!

      /* This code may not be 100% reliable
      bool busy = kmkernel->kbp()->isBusy();
      if (busy) kmkernel->kbp()->idle();
      KMessageBox::sorry(0,
        i18n("Unable to add message to folder.\n"
	     "(No space left on device or insufficient quota?)\n"
	     "Free space and sufficient quota are required to continue safely."));
      if (busy) kmkernel->kbp()->busy();
      */
      return error;
    }
  }

  if (index_return)
    *index_return = idx;

  emitMsgAddedSignals(idx);
  needsCompact = true;

/*
QFile fileD1( "testdat_xx-kmfoldermaildir-1" );
if( fileD1.open( QIODevice::WriteOnly ) ) {
    QDataStream ds( &fileD1 );
    ds.writeRawData( aMsg->asString(), aMsg->asString().length() );
    fileD1.close();  // If data is 0 we just create a zero length file.
}
*/
  return 0;
}

KMMessage* KMFolderMaildir::readMsg(int idx)
{
  KMMsgInfo* mi = (KMMsgInfo*)mMsgList[idx];
  KMMessage *msg = new KMMessage(*mi); // note that mi is deleted by the line below
  mMsgList.set(idx,&msg->toMsgBase()); // done now so that the serial number can be computed
  msg->setComplete( true );
  msg->fromDwString(getDwString(idx));
  return msg;
}

DwString KMFolderMaildir::getDwString(int idx)
{
  KMMsgInfo* mi = (KMMsgInfo*)mMsgList[idx];
  QString abs_file(location() + "/cur/");
  abs_file += mi->fileName();
  QFileInfo fi( abs_file );

  if (fi.exists() && fi.isFile() && fi.isWritable() && fi.size() > 0)
  {
    FILE* stream = KDE_fopen(QFile::encodeName(abs_file), "r+");
    kDebug( StorageDebug ) << "KDE_fopen(abs_file=" << abs_file << ", \"r+\") == stream == " << stream;
    if (stream) {
      size_t msgSize = fi.size();
      char* msgText = new char[ msgSize + 1 ];
      fread(msgText, msgSize, 1, stream);
      fclose( stream );
      kDebug( StorageDebug ) << "fclose(mIndexStream = " << stream << ")";
      msgText[msgSize] = '\0';
      size_t newMsgSize = KMail::Util::crlf2lf( msgText, msgSize );
      DwString str;
      // the DwString takes possession of msgText, so we must not delete it
      str.TakeBuffer( msgText, msgSize + 1, 0, newMsgSize );
      return str;
    }
  }
  kDebug(5006) <<"Could not open file r+" << abs_file;
  return DwString();
}


void KMFolderMaildir::readFileHeaderIntern( const QString& dir,
                                            const QString& file,
                                            MessageStatus& status )
{
  // we keep our current directory to restore it later
  char path_buffer[PATH_MAX];
  if ( !::getcwd( path_buffer, PATH_MAX - 1 ) ) {
    return;
  }

  ::chdir( QFile::encodeName( dir ) );

  // messages in the 'cur' directory are Read by default.. but may
  // actually be some other state (but not New)
  if ( status.isRead() ) {
    if ( !file.contains(KMAIL_MAILDIR_FNAME_SEPARATOR "2,") ) {
      status.setUnread();
    } else if ( file.right(5) == KMAIL_MAILDIR_FNAME_SEPARATOR "2,RS" ) {
      status.setReplied();
    }
  }

  // open the file and get a pointer to it
  QFile f( file );
  if ( f.open( QIODevice::ReadOnly ) == false ) {
    kWarning(5006) <<"The file '" << QFile::encodeName(dir) <<"/" << file
                   << "' could not be opened for reading the message."
                   << "Please check ownership and permissions.";
    return;
  }

  char line[MAX_LINE];
  bool atEof    = false;
  bool inHeader = true;
  QByteArray *lastStr = 0;

  QByteArray dateStr, fromStr, toStr, subjStr;
  QByteArray xmarkStr, replyToIdStr, msgIdStr, referencesStr;
  QByteArray statusStr, replyToAuxIdStr, uidStr;
  QByteArray contentTypeStr, charset;

  // iterate through this file until done
  while (!atEof) {
    // if the end of the file has been reached or if there was an error
    if ( f.atEnd() || ( -1 == f.readLine(line, MAX_LINE) ) ) {
      atEof = true;
    }

    // are we done with this file?  if so, compile our info and store
    // it in a KMMsgInfo object
    if (atEof || !inHeader)
    {
      msgIdStr = msgIdStr.trimmed();
      if( !msgIdStr.isEmpty() ) {
        int rightAngle;
        rightAngle = msgIdStr.indexOf( '>' );
        if( rightAngle != -1 )
          msgIdStr.truncate( rightAngle + 1 );
      }

      replyToIdStr = replyToIdStr.trimmed();
      if( !replyToIdStr.isEmpty() ) {
        int rightAngle;
        rightAngle = replyToIdStr.indexOf( '>' );
        if( rightAngle != -1 )
          replyToIdStr.truncate( rightAngle + 1 );
      }

      referencesStr = referencesStr.trimmed();
      if( !referencesStr.isEmpty() ) {
        int leftAngle, rightAngle;
        leftAngle = referencesStr.lastIndexOf( '<' );
        if( ( leftAngle != -1 )
            && ( replyToIdStr.isEmpty() || ( replyToIdStr[0] != '<' ) ) ) {
          // use the last reference, instead of missing In-Reply-To
          replyToIdStr = referencesStr.mid( leftAngle );
        }

        // find second last reference
        leftAngle = referencesStr.lastIndexOf( '<', leftAngle - 1 );
        if( leftAngle != -1 )
          referencesStr = referencesStr.mid( leftAngle );
        rightAngle = referencesStr.lastIndexOf( '>' );
        if( rightAngle != -1 )
          referencesStr.truncate( rightAngle + 1 );

        // Store the second to last reference in the replyToAuxIdStr
        // It is a good candidate for threading the message below if the
        // message In-Reply-To points to is not kept in this folder,
        // but e.g. in an Outbox
        replyToAuxIdStr = referencesStr;
        rightAngle = referencesStr.indexOf( '>' );
        if( rightAngle != -1 )
          replyToAuxIdStr.truncate( rightAngle + 1 );
      }

      statusStr = statusStr.trimmed();
      if (!statusStr.isEmpty())
      {
        // only handle those states not determined by the file suffix
        if (statusStr[0] == 'S')
          status.setSent();
        else if (statusStr[0] == 'F')
          status.setForwarded();
        else if (statusStr[0] == 'D')
          status.setDeleted();
        else if (statusStr[0] == 'Q')
          status.setQueued();
        else if (statusStr[0] == 'G')
          status.setImportant();
      }

      contentTypeStr = contentTypeStr.trimmed();
      charset = "";
      if ( !contentTypeStr.isEmpty() ) {
        int cidx = contentTypeStr.indexOf( "charset=" );
        if ( cidx != -1 ) {
          charset = contentTypeStr.mid( cidx + 8 );
          if ( charset[0] == '"' ) {
            charset = charset.mid( 1 );
          }
          cidx = 0;
          while ( cidx < charset.length() ) {
            if ( charset[cidx] == '"' ||
                 ( !isalnum(charset[cidx] ) &&
                   charset[cidx] != '-' && charset[cidx] != '_' ) ) {
              break;
            }
            ++cidx;
          }
          charset.truncate( cidx );
        }
      }

      KMMsgInfo *mi = new KMMsgInfo(folder());
      mi->init( subjStr.trimmed(),
                fromStr.trimmed(),
                toStr.trimmed(),
                0, status,
                xmarkStr.trimmed(),
                replyToIdStr, replyToAuxIdStr, msgIdStr,
				file.toLocal8Bit(),
                KMMsgEncryptionStateUnknown, KMMsgSignatureStateUnknown,
                KMMsgMDNStateUnknown, charset, f.size() );

      dateStr = dateStr.trimmed();
      if (!dateStr.isEmpty())
        mi->setDate(dateStr.constData());
      if ( !uidStr.isEmpty() )
         mi->setUID( uidStr.toULong() );
      mi->setDirty(false);
      mMsgList.append( mi, mExportsSernums );

      // if this is a New file and is in 'new', we move it to 'cur'
      if ( status.isNew() )
      {
        QString newDir(location() + "/new/");
        QString curDir(location() + "/cur/");
        moveInternal(newDir + file, curDir + file, mi);
      }

      break;
    }

    // Is this a long header line?
    if (inHeader && line[0] == '\t' || line[0] == ' ')
    {
      int i = 0;
      while (line[i] == '\t' || line[i] == ' ')
        i++;
      if (line[i] < ' ' && line[i] > 0)
        inHeader = false;
      else
        if (lastStr)
          *lastStr += line + i;
    }
    else
      lastStr = 0;

    if (inHeader && (line[0] == '\n' || line[0] == '\r'))
      inHeader = false;
    if (!inHeader)
      continue;

    if ( strncasecmp(line, "Date:", 5) == 0) {
      dateStr = QByteArray( line + 5 );
      lastStr = &dateStr;
    } else if ( strncasecmp( line, "From:", 5 ) == 0 ) {
      fromStr = QByteArray( line + 5 );
      lastStr = &fromStr;
    } else if ( strncasecmp( line, "To:", 3 ) == 0 ) {
      toStr = QByteArray( line + 3 );
      lastStr = &toStr;
    } else if ( strncasecmp( line, "Subject:", 8 ) == 0 ) {
      subjStr = QByteArray( line + 8 );
      lastStr = &subjStr;
    } else if ( strncasecmp( line, "References:", 11 ) == 0 ) {
      referencesStr = QByteArray( line + 11 );
      lastStr = &referencesStr;
    } else if ( strncasecmp( line, "Message-Id:", 11 ) == 0 ) {
      msgIdStr = QByteArray( line + 11 );
      lastStr = &msgIdStr;
    } else if ( strncasecmp( line, "X-KMail-Mark:", 13 ) == 0 ) {
      xmarkStr = QByteArray( line + 13 );
    } else if ( strncasecmp( line, "X-Status:", 9 ) == 0 ) {
      statusStr = QByteArray( line + 9 );
    } else if ( strncasecmp( line, "In-Reply-To:", 12 ) == 0 ) {
      replyToIdStr = QByteArray( line + 12 );
      lastStr = &replyToIdStr;
    } else if ( strncasecmp( line, "X-UID:", 6 ) == 0 ) {
      uidStr = QByteArray( line + 6 );
      lastStr = &uidStr;
    } else if ( strncasecmp( line, "Content-Type:", 13 ) == 0) {
      contentTypeStr = QByteArray( line + 13 );
      lastStr = &contentTypeStr;
    }

  }

  if ( status.isNew() || status.isUnread() ||
      (folder() == kmkernel->outboxFolder()))
  {
    mUnreadMsgs++;
   if (mUnreadMsgs == 0) ++mUnreadMsgs;
  }

  ::chdir(path_buffer);
}

int KMFolderMaildir::createIndexFromContents()
{
  kDebug() << "Creating index for" << location();

  mUnreadMsgs = 0;

  mMsgList.clear(true);
  mMsgList.reset(INIT_MSGS);

  mChanged = false;

  // first, we make sure that all the directories are here as they
  // should be
  QFileInfo dirinfo;

  dirinfo.setFile(location() + "/new");
  if (!dirinfo.exists() || !dirinfo.isDir())
  {
    kDebug(5006) << "Directory" << location() <<"/new doesn't exist or is a file";
    return 1;
  }
  QDir newDir(location() + "/new");
  newDir.setFilter(QDir::Files);

  dirinfo.setFile(location() + "/cur");
  if (!dirinfo.exists() || !dirinfo.isDir())
  {
    kDebug(5006) << "Directory" << location() <<"/cur doesn't exist or is a file";
    return 1;
  }
  QDir curDir(location() + "/cur");
  curDir.setFilter(QDir::Files);

  // then, we look for all the 'cur' files
  QFileInfoList list = curDir.entryInfoList();
  QFileInfo fi;

  Q_FOREACH( fi, list )
  {
    MessageStatus st = MessageStatus::statusRead();
    readFileHeaderIntern( curDir.path(), fi.fileName(), st );
  }

  // then, we look for all the 'new' files
  list = newDir.entryInfoList();

  Q_FOREACH( fi, list )
  {
    MessageStatus st = MessageStatus::statusNew();
    readFileHeaderIntern( newDir.path(), fi.fileName(), st );
  }

  if ( autoCreateIndex() ) {
    emit statusMsg(i18n("Writing index file"));
    writeIndex();
  }
  else {
#ifdef KMAIL_SQLITE_INDEX
#else
    mHeaderOffset = 0;
#endif
  }

  correctUnreadMsgsCount();

  if (kmkernel->outboxFolder() == folder() && count() > 0)
    KMessageBox::information(0, i18n("Your outbox contains messages which were "
    "most-likely not created by KMail;\nplease remove them from there if you "
    "do not want KMail to send them."));

  needsCompact = true;

  invalidateFolder();
  return 0;
}

KMFolderIndex::IndexStatus KMFolderMaildir::indexStatus()
{
  QFileInfo new_info(location() + "/new");
  QFileInfo cur_info(location() + "/cur");
  QFileInfo index_info(indexLocation());

  if (!index_info.exists())
    return KMFolderIndex::IndexMissing;

  // Check whether the directories are more than 5 seconds newer than the index
  // file. The 5 seconds are added to reduce the number of false alerts due
  // to slightly out of sync clocks of the NFS server and the local machine.
  KMFolderIndex::IndexStatus status =
      ( ( new_info.lastModified() > index_info.lastModified().addSecs( 5 ) ) ||
        ( cur_info.lastModified() > index_info.lastModified().addSecs( 5 ) ) )
         ? KMFolderIndex::IndexTooOld
         : KMFolderIndex::IndexOk;
  if ( status == KMFolderIndex::IndexTooOld ) {
    kWarning() << "Index" << indexLocation() << "out of date!";
    kWarning() << "  new:" << new_info.lastModified();
    kWarning() << "  cur:" << cur_info.lastModified();
    kWarning() << "  index:" << index_info.lastModified();
  }
  return status;
}

//-----------------------------------------------------------------------------
void KMFolderMaildir::removeMsg(int idx, bool)
{
  KMMsgBase* msg = mMsgList[idx];
  if (!msg || msg->fileName().isNull()) return;

  removeFile(msg->fileName());

  KMFolderIndex::removeMsg(idx);
}

//-----------------------------------------------------------------------------
KMMessage* KMFolderMaildir::take(int idx)
{
  // first, we do the high-level stuff.. then delete later
  KMMessage *msg = KMFolderIndex::take(idx);

  if (!msg || msg->fileName().isNull()) {
    return 0;
  }

  if ( removeFile(msg->fileName()) ) {
    return msg;
  } else {
    return 0;
  }
}


// static
bool KMFolderMaildir::removeFile( const QString & folderPath,
                                  const QString & filename )
{
  // we need to look in both 'new' and 'cur' since it's possible to
  // delete a message before the folder is compacted. Since the file
  // naming and moving is done in ::compact, we can't assume any
  // location at this point.
  QByteArray abs_file( QFile::encodeName( folderPath + "/cur/" + filename ) );
  if ( ::unlink( abs_file ) == 0 )
    return true;

  if ( errno == ENOENT ) { // doesn't exist
    abs_file = QFile::encodeName( folderPath + "/new/" + filename );
    if ( ::unlink( abs_file ) == 0 )
      return true;
  }

  kDebug(5006) <<"Can't delete" << abs_file << perror;
  return false;
}

bool KMFolderMaildir::removeFile( const QString & filename )
{
  return removeFile( location(), filename );
}

//-----------------------------------------------------------------------------
int KMFolderMaildir::removeContents()
{
  // NOTE: Don' use KIO::netaccess, it has reentrancy problems and multiple
  // mailchecks going on trigger them, when removing dirs
  if ( !removeDirAndContentsRecursively( location() + "/new/" ) ) return 1;
  if ( !removeDirAndContentsRecursively( location() + "/cur/" ) ) return 1;
  if ( !removeDirAndContentsRecursively( location() + "/tmp/" ) ) return 1;
  /* The subdirs are removed now. Check if there is anything else in the dir
   * and only if not delete the dir itself. The user could have data stored
   * that would otherwise be deleted. */
  QDir dir(location());
  if ( dir.count() == 2 ) { // only . and ..
    if ( !removeDirAndContentsRecursively( location() ), 0 ) return 1;
  }
  return 0;
}

K_GLOBAL_STATIC_WITH_ARGS(QRegExp, s_suffixRegExp, (KMAIL_MAILDIR_FNAME_SEPARATOR "2,?R?S?$"))

//-----------------------------------------------------------------------------
// static
QString KMFolderMaildir::constructValidFileName( const QString & filename,
                                                 const MessageStatus & status )
{
  QString aFileName( filename );

  if (aFileName.isEmpty())
  {
    aFileName.sprintf("%ld.%d.", (long)time(0), getpid());
    aFileName += KRandom::randomString(5);
  }
  int pos = aFileName.lastIndexOf( *s_suffixRegExp );
  if ( pos >= 0 )
    aFileName.truncate( pos );

  // only add status suffix if the message is neither new nor unread
  if (! ( status.isNew() || status.isUnread() ) )
  {
    QString suffix( KMAIL_MAILDIR_FNAME_SEPARATOR "2," );
    if ( status.isReplied() )
      suffix += "RS";
    else
      suffix += 'S';
    aFileName += suffix;
  }

  return aFileName;
}

//-----------------------------------------------------------------------------
QString KMFolderMaildir::moveInternal(const QString& oldLoc, const QString& newLoc, KMMsgInfo *mi)
{
  QString filename(mi->fileName());
  QString ret(moveInternal(oldLoc, newLoc, filename, mi->messageStatus()));

  if (filename != mi->fileName())
    mi->setFileName(filename);

  return ret;
}

//-----------------------------------------------------------------------------
QString KMFolderMaildir::moveInternal(const QString& oldLoc, const QString& newLoc, QString& aFileName, const MessageStatus& status)
{
  QString dest(newLoc);
  // make sure that our destination filename doesn't already exist
  while (QFile::exists(dest))
  {
    aFileName = constructValidFileName( QString(), status );

    QFileInfo fi(dest);
    dest = fi.absolutePath() + '/' + aFileName;
    setDirty( true );
  }

  QDir d;
  if (d.rename(oldLoc, dest) == false)
    return QString();
  else
    return dest;
}

//-----------------------------------------------------------------------------
void KMFolderMaildir::msgStatusChanged( const MessageStatus& oldStatus,
                                        const MessageStatus& newStatus,
                                        int idx )
{
  // if the status of any message changes, then we need to compact
  needsCompact = true;

  KMFolderIndex::msgStatusChanged(oldStatus, newStatus, idx);
}


// define the global static s_DirSizeJobQueue used below
typedef QPair<QPointer<KMFolderMaildir>,KFileItemList> DirSizeJobQueueEntry;
typedef QList<DirSizeJobQueueEntry> DirSizeJobQueue;
K_GLOBAL_STATIC( DirSizeJobQueue, s_DirSizeJobQueue )

/*virtual*/
qint64 KMFolderMaildir::doFolderSize() const
{
  if ( mCurrentlyCheckingFolderSize )
  {
    return -1;
  }
  mCurrentlyCheckingFolderSize = true;

  KFileItemList list;
  list.append( KFileItem( S_IFDIR, KFileItem::Unknown, location() + "/cur" ) );
  list.append( KFileItem( S_IFDIR, KFileItem::Unknown, location() + "/new" ) );
  list.append( KFileItem( S_IFDIR, KFileItem::Unknown, location() + "/tmp" ) );
  // using QPointer<const KMFolderMaildir> would be nicer, but QPointer does not support
  // const types as template parameter; therefore we have to const_cast this
  s_DirSizeJobQueue->append(
    qMakePair( QPointer<KMFolderMaildir>( const_cast<KMFolderMaildir*>( this ) ), list ) );

  // if there's only one entry in the queue then we can start
  // a DirectorySizeJob right away
  if ( s_DirSizeJobQueue->size() == 1 )
  {
    //kDebug(5006) << "Starting DirectorySizeJob for folder" << location();
    KIO::DirectorySizeJob* job = KIO::directorySize( list );
    connect( job, SIGNAL( result( KJob* ) ),
             this, SLOT( slotDirSizeJobResult( KJob*) ) );
  }

  return -1;
}

void KMFolderMaildir::slotDirSizeJobResult( KJob* job )
{
  mCurrentlyCheckingFolderSize = false;
  KIO::DirectorySizeJob * dirsize = dynamic_cast<KIO::DirectorySizeJob*>( job );
  if ( dirsize && !dirsize->error() )
  {
    mSize = dirsize->totalSize();
    //kDebug(5006) << << "DirectorySizeJob completed. Folder"
    //             << location() << "has size" << mSize;
    emit folderSizeChanged();
  }
  // remove the completed job from the queue
  s_DirSizeJobQueue->pop_front();

  // process the next entry in the queue
  while ( s_DirSizeJobQueue->size() > 0 )
  {
    DirSizeJobQueueEntry entry = s_DirSizeJobQueue->first();
    // check whether the entry is valid, i.e. whether the folder still exists
    if ( entry.first )
    {
      // start the next dirSizeJob
      //kDebug(5006) << "Starting DirectorySizeJob for folder"
      //             << entry.first->location();
      KIO::DirectorySizeJob* job = KIO::directorySize( entry.second );
      connect( job, SIGNAL( result( KJob* ) ),
               entry.first, SLOT( slotDirSizeJobResult( KJob*) ) );
      break;
    }
    else
    {
      // remove the invalid entry from the queue
      s_DirSizeJobQueue->pop_front();
    }
  }
}

#include "kmfoldermaildir.moc"
