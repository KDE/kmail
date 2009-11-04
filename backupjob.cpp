/* Copyright 2009 Thomas McGuire <mcguire@kde.org>

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

#include "kzip.h"
#include "ktar.h"
#include "kmessagebox.h"

#include "qfile.h"
#include "qstringlist.h"

using namespace KMail;

BackupJob::BackupJob( QWidget *parent )
  : QObject( parent ),
    mArchiveType( Zip ),
    mRootFolder( 0 ),
    mArchive( 0 ),
    mParentWidget( parent ),
    mCurrentFolder( 0 ),
    mCurrentMessage( 0 )
{
}

BackupJob::~BackupJob()
{
  mPendingFolders.clear();
  if ( mArchive ) {
    delete mArchive;
    mArchive = 0;
  }
  deleteLater();
}

void BackupJob::setRootFolder( KMFolder *rootFolder )
{
  mRootFolder = rootFolder;
}

void BackupJob::setSaveLocation( KUrl savePath )
{
  mMailArchivePath = savePath;
}

void BackupJob::setArchiveType( ArchiveType type )
{
  mArchiveType = type;
}

QString BackupJob::stripRootPath( const QString &path )
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
  kDebug(5006) << "Queueing folder " << root->name();
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

void BackupJob::finish()
{
  mArchive->close();
  kDebug(5006) << "Finished backup job.";
  // TODO: nice error/success dialog
}

void BackupJob::archiveNextMessage()
{
  mCurrentMessage = 0;
  if ( mPendingMessages.isEmpty() ) {
    kDebug(5006) << "===> All messages done in folder " << mCurrentFolder->name();
    mCurrentFolder->close( "BackupJob" );
    archiveNextFolder();
    return;
  }

  unsigned long serNum = mPendingMessages.front();
  mPendingMessages.pop_front();

  KMFolder *folder;
  int index = -1;
  kDebug(5006) << "SerNum: " << serNum;
  KMMsgDict::instance()->getLocation( serNum, &folder, &index );
  kDebug(5006) << "Index: " << index;
  // TODO: handle error
  Q_ASSERT( folder == mCurrentFolder );
  KMMessage *message = mCurrentFolder->getMsg( index );
  if ( !message ) {
    // TODO: handle error case!
  }

  kDebug(5006) << "Going to get next message with subject " << message->subject() << ", "
               << mPendingMessages.size() << " messages left in the folder.";

  if ( message->isComplete() ) {
    // Use a singleshot timer, or otherwise we risk ending up in a very big recursion
    // for folders that have many messages
    mCurrentMessage = message;
    QTimer::singleShot( 0, this, SLOT( processCurrentMessage() ) );
  }
  else if ( message->parent() ) {
    FolderJob *job = message->parent()->createJob( message );
    job->setCancellable( false );
    connect( job, SIGNAL( messageRetrieved( KMMessage* ) ),
             this, SLOT( messageRetrieved( KMMessage* ) ) );
    connect( job, SIGNAL( result( KMail::FolderJob* ) ),
             this, SLOT( folderJobFinished( KMail::FolderJob* ) ) );
    job->start();
  }
  else {
    // WTF, handle error
  }
}

void BackupJob::processCurrentMessage()
{
  if ( mCurrentMessage ) {
    kDebug(5006) << "Processing message with subject " << mCurrentMessage->subject();
    const DwString &messageDWString = mCurrentMessage->asDwString();
    const qint64 messageSize = messageDWString.size();
    const char *messageString = mCurrentMessage->asDwString().c_str();
    QString messageName = mCurrentMessage->fileName();
    if ( messageName.isEmpty() ) {
      messageName = QString::number( mCurrentMessage->getMsgSerNum() );
    }
    const QString fileName = stripRootPath( mCurrentFolder->location() ) +
                             "/cur/" + messageName;

    mArchive->writeFile( fileName, "root", "root", messageString, messageSize );
    // TODO: check error code
  }
  else {
    // No message? According to ImapJob::slotGetMessageResult(), that means the message is no
    // longer on the server. So ignore this one.
    kWarning(5006) << "Unable to download a message for folder " << mCurrentFolder->name();
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
  // TODO: error handling
}

void BackupJob::archiveNextFolder()
{
  if ( mPendingFolders.isEmpty() ) {
    finish();
    return;
  }

  mCurrentFolder = mPendingFolders.takeAt( 0 );
  kDebug(5006) << "===> Archiving next folder: " << mCurrentFolder->name();
  mCurrentFolder->open( "BackupJob" );

  // TODO: error handling
  const QString folderName = mCurrentFolder->name();
  if ( hasChildren( mCurrentFolder ) ) {
    mArchive->writeDir( stripRootPath( mCurrentFolder->subdirLocation() ), "root", "root" );
  }
  mArchive->writeDir( stripRootPath( mCurrentFolder->location() ), "root", "root" );
  mArchive->writeDir( stripRootPath( mCurrentFolder->location() ) + "/cur", "root", "root" );


  KMFolderCachedImap *dimapFolder = dynamic_cast<KMFolderCachedImap*>( mCurrentFolder->storage() );
  /*if ( dimapFolder ) {
    mArchive->addLocalFile( dimapFolder->uidCacheLocation(), stripRootPath( dimapFolder->uidCacheLocation() ) );
    // TODO: error handling
  }*/
  //mArchive->addLocalFile( mCurrentFolder->indexLocation(), stripRootPath( mCurrentFolder->indexLocation() ) );
  // TODO: error handling

  for ( int i = 0; i < mCurrentFolder->count( true ); i++ ) {
    unsigned long serNum = KMMsgDict::instance()->getMsgSerNum( mCurrentFolder, i );
    if ( serNum == 0 ) {
      // Uh oh
      kWarning(5006) << "Got serial number zero in " << mCurrentFolder->name()
                     << " at index " << i << "!";
      // TODO: handle error. this is _very_ bad
    }
    else
      mPendingMessages.append( serNum );
  }
  archiveNextMessage();
}

// TODO
// - error handling
// - import
// - connect to progressmanager
// - messagebox when finished (?)
// - ui dialog
// - use correct permissions
// - save index and serial number?
// - guarded pointers for folders
// - online IMAP: check mails first, so sernums are up-to-date?
//
// BUGS
// - Online IMAP: Test Mails -> Test%20Mails
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

  kDebug(5006) << "Starting backup.";
  mArchive->open( IO_WriteOnly );
  // TODO: error handling

  archiveNextFolder();
}

#include "backupjob.moc"

