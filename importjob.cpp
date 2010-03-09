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
#include "importjob.h"

#include "kmfolder.h"
#include "folderutil.h"
#include "kmfolderdir.h"
#include "kmfolderimap.h"
#include "imapjob.h"

#include "progressmanager.h"

#include <kdebug.h>
#include <kzip.h>
#include <ktar.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetype.h>

#include <qwidget.h>
#include <qtimer.h>
#include <qfile.h>

using namespace KMail;

ImportJob::ImportJob( QWidget *parentWidget )
  : QObject( parentWidget ),
    mArchive( 0 ),
    mRootFolder( 0 ),
    mParentWidget( parentWidget ),
    mNumberOfImportedMessages( 0 ),
    mCurrentFolder( 0 ),
    mCurrentMessage( 0 ),
    mCurrentMessageFile( 0 ),
    mProgressItem( 0 ),
    mAborted( false )
{
}

ImportJob::~ImportJob()
{
  if ( mArchive && mArchive->isOpened() ) {
    mArchive->close();
  }
  delete mArchive;
  mArchive = 0;
}

void ImportJob::setFile( const KURL &archiveFile )
{
  mArchiveFile = archiveFile;
}

void ImportJob::setRootFolder( KMFolder *rootFolder )
{
  mRootFolder = rootFolder;
}

void ImportJob::finish()
{
  kdDebug(5006) << "Finished import job." << endl;
  mProgressItem->setComplete();
  mProgressItem = 0;
  QString text = i18n( "Importing the archive file '%1' into the folder '%2' succeeded." )
                     .arg( mArchiveFile.path() ).arg( mRootFolder->name() );
  text += "\n" + i18n( "1 message was imported.", "%n messages were imported.", mNumberOfImportedMessages );
  KMessageBox::information( mParentWidget, text, i18n( "Import finished." ) );
  deleteLater();
}

void ImportJob::cancelJob()
{
  abort( i18n( "The operation was canceled by the user." ) );
}

void ImportJob::abort( const QString &errorMessage )
{
  if ( mAborted )
    return;

  mAborted = true;
  QString text = i18n( "Failed to import the archive into folder '%1'." ).arg( mRootFolder->name() );
  text += "\n" + errorMessage;
  if ( mProgressItem ) {
    mProgressItem->setComplete();
    mProgressItem = 0;
    // The progressmanager will delete it
  }
  KMessageBox::sorry( mParentWidget, text, i18n( "Importing archive failed." ) );
  deleteLater();
}

KMFolder * ImportJob::createSubFolder( KMFolder *parent, const QString &folderName, mode_t permissions )
{
  KMFolder *newFolder = FolderUtil::createSubFolder( parent, parent->child(), folderName, QString(),
                                                     KMFolderTypeMaildir );
  if ( !newFolder ) {
    abort( i18n( "Unable to create subfolder for folder '%1'." ).arg( parent->name() ) );
    return 0;
  }
  else {
    newFolder->createChildFolder(); // TODO: Just creating a child folder here is wasteful, only do
                                    //       that if really needed. We do it here so we can set the
                                    //       permissions
    chmod( newFolder->location().latin1(), permissions | S_IXUSR );
    chmod( newFolder->subdirLocation().latin1(), permissions | S_IXUSR );
    // TODO: chown?
    // TODO: what about subdirectories like "cur"?
    return newFolder;
  }
}

void ImportJob::enqueueMessages( const KArchiveDirectory *dir, KMFolder *folder )
{
  const KArchiveDirectory *messageDir = dynamic_cast<const KArchiveDirectory*>( dir->entry( "cur" ) );
  if ( messageDir ) {
    Messages messagesToQueue;
    messagesToQueue.parent = folder;
    const QStringList entries = messageDir->entries();
    for ( uint i = 0; i < entries.size(); i++ ) {
      const KArchiveEntry *entry = messageDir->entry( entries[i] );
      Q_ASSERT( entry );
      if ( entry->isDirectory() ) {
        kdWarning(5006) << "Unexpected subdirectory in archive folder " << dir->name() << endl;
      }
      else {
        kdDebug(5006) << "Queueing message " << entry->name() << endl;
        const KArchiveFile *file = static_cast<const KArchiveFile*>( entry );
        messagesToQueue.files.append( file );
      }
    }
    mQueuedMessages.append( messagesToQueue );
  }
  else {
    kdWarning(5006) << "No 'cur' subdirectory for archive directory " << dir->name() << endl;
  }
}

void ImportJob::messageAdded()
{
  mNumberOfImportedMessages++;
  if ( mCurrentFolder->folderType() == KMFolderTypeMaildir ||
       mCurrentFolder->folderType() == KMFolderTypeCachedImap ) {
    const QString messageFile = mCurrentFolder->location() + "/cur/" + mCurrentMessage->fileName();
    // TODO: what if the file is not in the "cur" subdirectory?
    if ( QFile::exists( messageFile ) ) {
      chmod( messageFile.latin1(), mCurrentMessageFile->permissions() );
      // TODO: changing user/group he requires a bit more work, requires converting the strings
      //       to uid_t and gid_t
      //getpwnam()
      //chown( messageFile,
    }
    else {
      kdWarning(5006) << "Unable to change permissions for newly created file: " << messageFile << endl;
    }
  }
  // TODO: Else?

  mCurrentMessage = 0;
  mCurrentMessageFile = 0;
  QTimer::singleShot( 0, this, SLOT( importNextMessage() ) );
}

void ImportJob::importNextMessage()
{
  if ( mAborted )
    return;

  if ( mQueuedMessages.isEmpty() ) {
    kdDebug(5006) << "importNextMessage(): Processed all messages in the queue." << endl;
    if ( mCurrentFolder ) {
      mCurrentFolder->close( "ImportJob" );
    }
    mCurrentFolder = 0;
    importNextDirectory();
    return;
  }

  Messages &messages = mQueuedMessages.front();
  if ( messages.files.isEmpty() ) {
    mQueuedMessages.pop_front();
    importNextMessage();
    return;
  }

  KMFolder *folder = messages.parent;
  if ( folder != mCurrentFolder ) {
    kdDebug(5006) << "importNextMessage(): Processed all messages in the current folder of the queue." << endl;
    if ( mCurrentFolder ) {
      mCurrentFolder->close( "ImportJob" );
    }
    mCurrentFolder = folder;
    if ( mCurrentFolder->open( "ImportJob" ) != 0 ) {
      abort( i18n( "Unable to open folder '%1'." ).arg( mCurrentFolder->name() ) );
      return;
    }
    kdDebug(5006) << "importNextMessage(): Current folder of queue is now: " << mCurrentFolder->name() << endl;
    mProgressItem->setStatus( i18n( "Importing folder %1" ).arg( mCurrentFolder->name() ) );
  }

  mProgressItem->setProgress( ( mProgressItem->progress() + 5 ) );

  mCurrentMessageFile = messages.files.first();
  Q_ASSERT( mCurrentMessageFile );
  messages.files.removeFirst();

  mCurrentMessage = new KMMessage();
  mCurrentMessage->fromByteArray( mCurrentMessageFile->data(), true /* setStatus */ );
  int retIndex;

  // If this is not an IMAP folder, we can add the message directly. Otherwise, the whole thing is
  // async, for online IMAP. While addMsg() fakes a sync call, we rather do it the async way here
  // ourselves, as otherwise the whole thing gets pretty much messed up with regards to folder
  // refcounting. Furthermore, the completion dialog would be shown before the messages are actually
  // uploaded.
  if ( mCurrentFolder->folderType() != KMFolderTypeImap ) {
    if ( mCurrentFolder->addMsg( mCurrentMessage, &retIndex ) != 0 ) {
      abort( i18n( "Failed to add a message to the folder '%1'." ).arg( mCurrentFolder->name() ) );
      return;
    }
    messageAdded();
  }
  else {
    ImapJob *imapJob = new ImapJob( mCurrentMessage, ImapJob::tPutMessage,
                                    dynamic_cast<KMFolderImap*>( mCurrentFolder->storage() ) );
    connect( imapJob, SIGNAL(result(KMail::FolderJob*)),
             SLOT(messagePutResult(KMail::FolderJob*)) );
    imapJob->start();
  }
}

void ImportJob::messagePutResult( KMail::FolderJob *job )
{
  if ( mAborted )
    return;

  if ( job->error() ) {
    abort( i18n( "Failed to upload a message to the IMAP server." ) );
    return;
  } else {

    KMFolderImap *imap = dynamic_cast<KMFolderImap*>( mCurrentFolder->storage() );
    Q_ASSERT( imap );

    // Ok, we uploaded the message, but we still need to add it to the folder. Use addMsgQuiet(),
    // otherwise it will be uploaded again.
    imap->addMsgQuiet( mCurrentMessage );
    messageAdded();
  }
}

// Input: .inbox.directory
// Output: inbox
// Can also return an empty string if this is no valid dir name
static QString folderNameForDirectoryName( const QString &dirName )
{
  Q_ASSERT( dirName.startsWith( "." ) );
  const QString end = ".directory";
  const int expectedIndex = dirName.length() - end.length();
  if ( dirName.lower().find( end ) != expectedIndex )
    return QString();
  QString returnName = dirName.left( dirName.length() - end.length() );
  returnName = returnName.right( returnName.length() - 1 );
  return returnName;
}

KMFolder* ImportJob::getOrCreateSubFolder( KMFolder *parentFolder, const QString &subFolderName,
                                           mode_t subFolderPermissions )
{
  if ( !parentFolder->createChildFolder() ) {
    abort( i18n( "Unable to create subfolder for folder '%1'." ).arg( parentFolder->name() ) );
    return 0;
  }

  KMFolder *subFolder = 0;
  subFolder = dynamic_cast<KMFolder*>( parentFolder->child()->hasNamedFolder( subFolderName ) );

  if ( !subFolder ) {
    subFolder = createSubFolder( parentFolder, subFolderName, subFolderPermissions );
  }
  return subFolder;
}

void ImportJob::importNextDirectory()
{
  if ( mAborted )
    return;

  if ( mQueuedDirectories.isEmpty() ) {
    finish();
    return;
  }

  Folder folder = mQueuedDirectories.first();
  KMFolder *currentFolder = folder.parent;
  mQueuedDirectories.pop_front();
  kdDebug(5006) << "importNextDirectory(): Working on directory " << folder.archiveDir->name() << endl;

  QStringList entries = folder.archiveDir->entries();
  for ( uint i = 0; i < entries.size(); i++ ) {
    const KArchiveEntry *entry = folder.archiveDir->entry( entries[i] );
    Q_ASSERT( entry );
    kdDebug(5006) << "Queueing entry " << entry->name() << endl;
    if ( entry->isDirectory() ) {
      const KArchiveDirectory *dir = static_cast<const KArchiveDirectory*>( entry );
      if ( !dir->name().startsWith( "." ) ) {

        kdDebug(5006) << "Queueing messages in folder " << entry->name() << endl;
        KMFolder *subFolder = getOrCreateSubFolder( currentFolder, entry->name(), entry->permissions() );
        if ( !subFolder )
          return;

        enqueueMessages( dir, subFolder );
      }

      // Entry starts with a dot, so we assume it is a subdirectory
      else {

        const QString folderName = folderNameForDirectoryName( entry->name() );
        if ( folderName.isEmpty() ) {
          abort( i18n( "Unexpected subdirectory named '%1'." ).arg( entry->name() ) );
          return;
        }
        KMFolder *subFolder = getOrCreateSubFolder( currentFolder, folderName, entry->permissions() );
        if ( !subFolder )
          return;

        Folder newFolder;
        newFolder.archiveDir = dir;
        newFolder.parent = subFolder;
        kdDebug(5006) << "Enqueueing directory " << entry->name() << endl;
        mQueuedDirectories.push_back( newFolder );
      }
    }
  }

  importNextMessage();
}

// TODO:
// BUGS:
//    Online IMAP can fail spectacular, for example when cancelling upload
//    Online IMAP: Inform that messages are still being uploaded on finish()!
void ImportJob::start()
{
  Q_ASSERT( mRootFolder );
  Q_ASSERT( mArchiveFile.isValid() );

  KMimeType::Ptr mimeType = KMimeType::findByURL( mArchiveFile, 0, true /* local file */ );
  if ( !mimeType->patterns().grep( "tar", false /* no case-sensitive */ ).isEmpty() )
    mArchive = new KTar( mArchiveFile.path() );
  else if ( !mimeType->patterns().grep( "zip", false ).isEmpty() )
    mArchive = new KZip( mArchiveFile.path() );
  else {
    abort( i18n( "The file '%1' does not appear to be a valid archive." ).arg( mArchiveFile.path() ) );
    return;
  }

  if ( !mArchive->open( IO_ReadOnly ) ) {
    abort( i18n( "Unable to open archive file '%1'" ).arg( mArchiveFile.path() ) );
    return;
  }

  mProgressItem = KPIM::ProgressManager::createProgressItem(
      "ImportJob",
      i18n( "Importing Archive" ),
      QString(),
      true );
  mProgressItem->setUsesBusyIndicator( true );
  connect( mProgressItem, SIGNAL(progressItemCanceled(KPIM::ProgressItem*)),
           this, SLOT(cancelJob()) );

  Folder nextFolder;
  nextFolder.archiveDir = mArchive->directory();
  nextFolder.parent = mRootFolder;
  mQueuedDirectories.push_back( nextFolder );
  importNextDirectory();
}

#include "importjob.moc"
