/* Copyright 2009 Klarälvdalens Datakonsult AB

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "backupjob.h"

#include "kmmsgdict.h"
#include "kmfolder.h"
#include "kmfoldercachedimap.h"
#include "kmfolderdir.h"
#include "folderutil.h"

#include "progressmanager.h"

#include "kzip.h"
#include "ktar.h"
#include "kmessagebox.h"

#include "qfile.h"
#include "qfileinfo.h"
#include "qstringlist.h"

using namespace KMail;

BackupJob::BackupJob( QWidget *parent )
  : QObject( parent ),
    mArchiveType( Zip ),
    mRootFolder( 0 ),
    mArchive( 0 ),
    mParentWidget( parent ),
    mCurrentFolderOpen( false ),
    mArchivedMessages( 0 ),
    mArchivedSize( 0 ),
    mProgressItem( 0 ),
    mAborted( false ),
    mDeleteFoldersAfterCompletion( false ),
    mCurrentFolder( 0 ),
    mCurrentMessage( 0 ),
    mCurrentJob( 0 )
{
}

BackupJob::~BackupJob()
{
  mPendingFolders.clear();
  if ( mArchive ) {
    delete mArchive;
    mArchive = 0;
  }
}

void BackupJob::setRootFolder( KMFolder *rootFolder )
{
  mRootFolder = rootFolder;
}

void BackupJob::setSaveLocation( const KUrl savePath )
{
  mMailArchivePath = savePath;
}

void BackupJob::setArchiveType( ArchiveType type )
{
  mArchiveType = type;
}

void BackupJob::setDeleteFoldersAfterCompletion( bool deleteThem )
{
  mDeleteFoldersAfterCompletion = deleteThem;
}

QString BackupJob::stripRootPath( const QString &path ) const
{
  QString ret = path;
  ret = ret.remove( mRootFolder->path() );
  if ( ret.startsWith( QLatin1String( "/" ) ) )
    ret = ret.right( ret.length() - 1 );
  return ret;
}

void BackupJob::queueFolders( KMFolder *root )
{
  mPendingFolders.append( root );
  kDebug() << "Queueing folder " << root->name();
  KMFolderDir *dir = root->child();
  if ( dir ) {
    QListIterator<KMFolderNode*> it( *dir );
    while ( it.hasNext() ) {
      KMFolderNode *node = it.next();
      if ( node->isDir() )
        continue;
      KMFolder *folder = static_cast<KMFolder*>( node );
      queueFolders( folder );
    }
  }
}

bool BackupJob::hasChildren( KMFolder *folder ) const
{
  KMFolderDir *dir = folder->child();
  if ( dir ) {
    QListIterator<KMFolderNode*> it( *dir );
    while ( it.hasNext() ) {
      KMFolderNode *node = it.next();
      if ( !node->isDir() )
        return true;
    }
  }
  return false;
}

void BackupJob::cancelJob()
{
  abort( i18n( "The operation was canceled by the user." ) );
}

void BackupJob::abort( const QString &errorMessage )
{
  // We could be called this twice, since killing the current job below will cause the job to fail,
  // and that will call abort()
  if ( mAborted )
    return;

  mAborted = true;
  if ( mCurrentFolderOpen && mCurrentFolder ) {
    mCurrentFolder->close( "BackupJob" );
    mCurrentFolder = 0;
  }
  if ( mArchive && mArchive->isOpen() ) {
    mArchive->close();
  }
  if ( mCurrentJob ) {
    mCurrentJob->kill();
    mCurrentJob = 0;
  }
  if ( mProgressItem ) {
    mProgressItem->setComplete();
    mProgressItem = 0;
    // The progressmanager will delete it
  }

  QString text = i18n( "Failed to archive the folder '%1'.", mRootFolder->name() );
  text += '\n' + errorMessage;
  KMessageBox::sorry( mParentWidget, text, i18n( "Archiving failed." ) );
  deleteLater();
  // Clean up archive file here?
}

void BackupJob::finish()
{
  if ( mArchive->isOpen() ) {
    if ( !mArchive->close() ) {
      abort( i18n( "Unable to finalize the archive file." ) );
      return;
    }
  }

  mProgressItem->setStatus( i18n( "Archiving finished" ) );
  mProgressItem->setComplete();
  mProgressItem = 0;

  QFileInfo archiveFileInfo( mMailArchivePath.path() );
  QString text = i18n( "Archiving folder '%1' successfully completed. "
                       "The archive was written to the file '%2'.",
                       mRootFolder->name(), mMailArchivePath.path() );
  text += '\n' + i18np( "1 message of size %2 was archived.",
                        "%1 messages with the total size of %2 were archived.",
                        mArchivedMessages, KIO::convertSize( mArchivedSize ) );
  text += '\n' + i18n( "The archive file has a size of %1.",
                       KIO::convertSize( archiveFileInfo.size() ) );
  KMessageBox::information( mParentWidget, text, i18n( "Archiving finished." ) );

  if ( mDeleteFoldersAfterCompletion ) {
    // Some safety checks first...
    if ( archiveFileInfo.size() > 0 && ( mArchivedSize > 0 || mArchivedMessages == 0 ) ) {
      // Sorry for any data loss!
      FolderUtil::deleteFolder( mRootFolder, mParentWidget );
    }
  }

  deleteLater();
}

void BackupJob::archiveNextMessage()
{
  if ( mAborted )
    return;

  mCurrentMessage = 0;
  if ( mPendingMessages.isEmpty() ) {
    kDebug() << "===> All messages done in folder " << mCurrentFolder->name();
    mCurrentFolder->close( "BackupJob" );
    mCurrentFolderOpen = false;
    archiveNextFolder();
    return;
  }

  unsigned long serNum = mPendingMessages.front();
  mPendingMessages.pop_front();

  KMFolder *folder;
  int index = -1;
  KMMsgDict::instance()->getLocation( serNum, &folder, &index );
  if ( index == -1 ) {
    kWarning() << "Failed to get message location for sernum " << serNum;
    abort( i18n( "Unable to retrieve a message for folder '%1'.", mCurrentFolder->name() ) );
    return;
  }

  Q_ASSERT( folder == mCurrentFolder );
  KMMessage *message = mCurrentFolder->getMsg( index );
  if ( !message ) {
    kWarning() << "Failed to retrieve message with index " << index;
    abort( i18n( "Unable to retrieve a message for folder '%1'.", mCurrentFolder->name() ) );
    return;
  }

  kDebug() << "Going to get next message with subject " << message->subject() << ", "
           << mPendingMessages.size() << " messages left in the folder.";

  if ( message->isComplete() ) {
    // Use a singleshot timer, or otherwise we risk ending up in a very big recursion
    // for folders that have many messages
    mCurrentMessage = message;
    QTimer::singleShot( 0, this, SLOT( processCurrentMessage() ) );
  }
  else if ( message->parent() ) {
    mCurrentJob = message->parent()->createJob( message );
    mCurrentJob->setCancellable( false );
    connect( mCurrentJob, SIGNAL( messageRetrieved( KMMessage* ) ),
             this, SLOT( messageRetrieved( KMMessage* ) ) );
    connect( mCurrentJob, SIGNAL( result( KMail::FolderJob* ) ),
             this, SLOT( folderJobFinished( KMail::FolderJob* ) ) );
    mCurrentJob->start();
  }
  else {
    kWarning() << "Message with subject " << mCurrentMessage->subject()
               << " is neither complete nor has a parent!";
    abort( i18n( "Internal error while trying to retrieve a message from folder '%1'.",
                 mCurrentFolder->name() ) );
  }
}

static int fileInfoToUnixPermissions( const QFileInfo &fileInfo )
{
  int perm = 0;
  if ( fileInfo.permission( QFile::ExeOther ) ) perm += S_IXOTH;
  if ( fileInfo.permission( QFile::WriteOther ) ) perm += S_IWOTH;
  if ( fileInfo.permission( QFile::ReadOther ) ) perm += S_IROTH;
  if ( fileInfo.permission( QFile::ExeGroup ) ) perm += S_IXGRP;
  if ( fileInfo.permission( QFile::WriteGroup ) ) perm += S_IWGRP;
  if ( fileInfo.permission( QFile::ReadGroup ) ) perm += S_IRGRP;
  if ( fileInfo.permission( QFile::ExeOwner ) ) perm += S_IXUSR;
  if ( fileInfo.permission( QFile::WriteOwner ) ) perm += S_IWUSR;
  if ( fileInfo.permission( QFile::ReadOwner ) ) perm += S_IRUSR;
  return perm;
}

void BackupJob::processCurrentMessage()
{
  if ( mAborted )
    return;

  if ( mCurrentMessage ) {
    kDebug() << "Processing message with subject " << mCurrentMessage->subject();
    const DwString &messageDWString = mCurrentMessage->asDwString();
    const qint64 messageSize = messageDWString.size();
    const char *messageString = mCurrentMessage->asDwString().c_str();
    QString messageName;
    QFileInfo fileInfo;
    if ( messageName.isEmpty() ) {
      messageName = QString::number( mCurrentMessage->getMsgSerNum() ); // IMAP doesn't have filenames
      if ( mCurrentMessage->storage() ) {
        fileInfo.setFile( mCurrentMessage->storage()->location() );
        // TODO: what permissions etc to take when there is no storage file?
      }
    }
    else {
      // TODO: What if the message is not in the "cur" directory?
      fileInfo.setFile( mCurrentFolder->location() + "/cur/" + mCurrentMessage->fileName() );
      messageName = mCurrentMessage->fileName();
    }

    const QString fileName = stripRootPath( mCurrentFolder->location() ) +
                             "/cur/" + messageName;

    QString user;
    QString group;
    mode_t permissions = 0700;
    time_t creationTime = time( 0 );
    time_t modificationTime = time( 0 );
    time_t accessTime = time( 0 );
    if ( !fileInfo.fileName().isEmpty() ) {
      user = fileInfo.owner();
      group = fileInfo.group();
      permissions = fileInfoToUnixPermissions( fileInfo );
      creationTime = fileInfo.created().toTime_t();
      modificationTime = fileInfo.lastModified().toTime_t();
      accessTime = fileInfo.lastRead().toTime_t();
    }
    else {
      kWarning() << "Unable to find file for message " << fileName;
    }

    if ( !mArchive->writeFile( fileName, user, group,
                               messageString, messageSize, permissions,
                               accessTime, modificationTime, creationTime ) ) {
      abort( i18n( "Failed to write a message into the archive folder '%1'.", mCurrentFolder->name() ) );
      return;
    }

    mArchivedMessages++;
    mArchivedSize += messageSize;
  }
  else {
    // No message? According to ImapJob::slotGetMessageResult(), that means the message is no
    // longer on the server. So ignore this one.
    kWarning() << "Unable to download a message for folder " << mCurrentFolder->name();
  }
  archiveNextMessage();
}

void BackupJob::messageRetrieved( KMMessage *message )
{
  mCurrentMessage = message;
  processCurrentMessage();
}

void BackupJob::folderJobFinished( KMail::FolderJob *job )
{
  if ( mAborted )
    return;

  // The job might finish after it has emitted messageRetrieved(), in which case we have already
  // started a new job. Don't set the current job to 0 in that case.
  if ( job == mCurrentJob ) {
    mCurrentJob = 0;
  }

  if ( job->error() ) {
    if ( mCurrentFolder )
      abort( i18n( "Downloading a message in folder '%1' failed.", mCurrentFolder->name() ) );
    else
      abort( i18n( "Downloading a message in the current folder failed." ) );
  }
}

bool BackupJob::writeDirHelper( const QString &directoryPath, const QString &permissionPath )
{
  QFileInfo fileInfo( permissionPath );
  QString user = fileInfo.owner();
  QString group = fileInfo.group();
  mode_t permissions = fileInfoToUnixPermissions( fileInfo );
  time_t creationTime = fileInfo.created().toTime_t();
  time_t modificationTime = fileInfo.lastModified().toTime_t();
  time_t accessTime = fileInfo.lastRead().toTime_t();
  return mArchive->writeDir( stripRootPath( directoryPath ), user, group, permissions, accessTime,
                             modificationTime, creationTime );
}

void BackupJob::archiveNextFolder()
{
  if ( mAborted )
    return;

  if ( mPendingFolders.isEmpty() ) {
    finish();
    return;
  }

  mCurrentFolder = mPendingFolders.takeAt( 0 );
  kDebug() << "===> Archiving next folder: " << mCurrentFolder->name();
  mProgressItem->setStatus( i18n( "Archiving folder %1", mCurrentFolder->name() ) );
  if ( mCurrentFolder->open( "BackupJob" ) != 0 ) {
    abort( i18n( "Unable to open folder '%1'.", mCurrentFolder->name() ) );
    return;
  }
  mCurrentFolderOpen = true;

  const QString folderName = mCurrentFolder->name();
  bool success = true;
  if ( hasChildren( mCurrentFolder ) ) {
    if ( !writeDirHelper( mCurrentFolder->subdirLocation(), mCurrentFolder->subdirLocation() ) )
      success = false;
  }
  if ( !writeDirHelper( mCurrentFolder->location(), mCurrentFolder->location() ) )
    success = false;
  if ( !writeDirHelper( mCurrentFolder->location() + "/cur", mCurrentFolder->location() ) )
    success = false;
  if ( !writeDirHelper( mCurrentFolder->location() + "/new", mCurrentFolder->location() ) )
    success = false;
  if ( !writeDirHelper( mCurrentFolder->location() + "/tmp", mCurrentFolder->location() ) )
    success = false;
  if ( !success ) {
    abort( i18n( "Unable to create folder structure for folder '%1' within archive file.",
                 mCurrentFolder->name() ) );
    return;
  }

  for ( int i = 0; i < mCurrentFolder->count( false /* no cache */ ); i++ ) {
    unsigned long serNum = KMMsgDict::instance()->getMsgSerNum( mCurrentFolder, i );
    if ( serNum == 0 ) {
      // Uh oh
      kWarning() << "Got serial number zero in " << mCurrentFolder->name()
                 << " at index " << i << "!";
      // TODO: handle error in a nicer way. this is _very_ bad
      abort( i18n( "Unable to backup messages in folder '%1', the index file is corrupted.",
                   mCurrentFolder->name() ) );
      return;
    }
    else
      mPendingMessages.append( serNum );
  }
  archiveNextMessage();
}

// TODO
// - error handling
// - import
// - connect to progressmanager, especially abort
// - messagebox when finished (?)
// - ui dialog
// - use correct permissions
// - save index and serial number?
// - guarded pointers for folders
// - online IMAP: check mails first, so sernums are up-to-date?
// - "ignore errors"-mode, with summary how many messages couldn't be archived?
// - do something when the user quits KMail while the backup job is running
// - run in a thread?
// - delete source folder after completion. dangerous!!!
//
// BUGS
// - Online IMAP: Test Mails -> Test%20Mails
// - corrupted sernums indices stop backup job
void BackupJob::start()
{
  Q_ASSERT( !mMailArchivePath.isEmpty() );
  Q_ASSERT( mRootFolder );

  queueFolders( mRootFolder );

  switch ( mArchiveType ) {
    case Zip: {
      KZip *zip = new KZip( mMailArchivePath.path() );
      zip->setCompression( KZip::DeflateCompression );
      mArchive = zip;
      break;
    }
    case Tar: {
      mArchive = new KTar( mMailArchivePath.path(), "application/x-tar" );
      break;
    }
    case TarGz: {
      mArchive = new KTar( mMailArchivePath.path(), "application/x-gzip" );
      break;
    }
    case TarBz2: {
      mArchive = new KTar( mMailArchivePath.path(), "application/x-bzip2" );
      break;
    }
  }

  kDebug() << "Starting backup.";
  if ( !mArchive->open( QIODevice::WriteOnly ) ) {
    abort( i18n( "Unable to open archive for writing." ) );
    return;
  }

  mProgressItem = KPIM::ProgressManager::createProgressItem(
      "BackupJob",
      i18n( "Archiving" ),
      QString(),
      true );
  mProgressItem->setUsesBusyIndicator( true );
  connect( mProgressItem, SIGNAL(progressItemCanceled(KPIM::ProgressItem*)),
           this, SLOT(cancelJob()) );

  archiveNextFolder();
}

#include "backupjob.moc"

