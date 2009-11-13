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
    mProgressItem( 0 ),
    mAborted( false )
{
}

ImportJob::~ImportJob()
{
  if ( mArchive && mArchive->isOpen() ) {
    mArchive->close();
  }
  delete mArchive;
  mArchive = 0;
}

void ImportJob::setFile( const KUrl &archiveFile )
{
  mArchiveFile = archiveFile;
}

void ImportJob::setRootFolder( KMFolder *rootFolder )
{
  mRootFolder = rootFolder;
}

void ImportJob::finish()
{
  kDebug() << "Finished import job.";
  mProgressItem->setComplete();
  mProgressItem = 0;
  QString text = i18n( "Importing the archive file '%1' into the folder '%2' succeeded.",
                       mArchiveFile.path(), mRootFolder->name() );
  text += '\n' + i18n( "%1 messages were imported.", mNumberOfImportedMessages );
  KMessageBox::information( mParentWidget, text, i18n( "Import finished." ) );
  deleteLater();
}

void ImportJob::cancelJob()
{
  abort( i18n( "The operation was cancelled by the user." ) );
}

void ImportJob::abort( const QString &errorMessage )
{
  if ( mAborted )
    return;

  mAborted = true;
  QString text = i18n( "Failed to import the archive into folder '%1'.", mRootFolder->name() );
  text += '\n' + errorMessage;
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
  if ( !parent->createChildFolder() ) {
    abort( i18n( "Unable to create subfolder for folder '%1'.", parent->name() ) );
    return 0;
  }

  KMFolder *newFolder = FolderUtil::createSubFolder( parent, parent->child(), folderName, QString(),
                                                     KMFolderTypeMaildir );
  if ( !newFolder ) {
    abort( i18n( "Unable to create subfolder for folder '%1'.", parent->name() ) );
    return 0;
  }
  else {
    newFolder->createChildFolder(); // TODO: Just creating a child folder here is wasteful, only do
                                    //       that if really needed. We do it here so we can set the
                                    //       permissions
    chmod( newFolder->location().toLatin1(), permissions );
    chmod( newFolder->subdirLocation().toLatin1(), permissions );
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
    for ( int i = 0; i < entries.size(); i++ ) {
      const KArchiveEntry *entry = messageDir->entry( entries[i] );
      Q_ASSERT( entry );
      if ( entry->isDirectory() ) {
        kWarning() << "Unexpected subdirectory in archive folder " << dir->name();
      }
      else {
        kDebug() << "Queueing message " << entry->name();
        const KArchiveFile *file = static_cast<const KArchiveFile*>( entry );
        messagesToQueue.files.append( file );
      }
    }
    mQueuedMessages.append( messagesToQueue );
  }
  else {
    kWarning() << "No 'cur' subdirectory for archive directory " << dir->name();
  }
}

void ImportJob::importNextMessage()
{
  if ( mAborted )
    return;

  if ( mQueuedMessages.isEmpty() ) {
    kDebug() << "Processed all messages in the queue.";
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
    kDebug() << "Processed all messages in the current folder of the queue.";
    if ( mCurrentFolder ) {
      mCurrentFolder->close( "ImportJob" );
    }
    mCurrentFolder = folder;
    if ( mCurrentFolder->open( "ImportJob" ) != 0 ) {
      abort( i18n( "Unable to open folder '%1'.", mCurrentFolder->name() ) );
      return;
    }
    kDebug() << "Current folder of queue is now: " << mCurrentFolder->name();
    mProgressItem->setStatus( i18n( "Importing folder %1", mCurrentFolder->name() ) );
  }

  mProgressItem->setProgress( ( mProgressItem->progress() + 5 ) );

  const KArchiveFile *file = messages.files.first();
  Q_ASSERT( file );
  messages.files.removeFirst();

  KMMessage *newMessage = new KMMessage();
  newMessage->fromString( file->data(), true /* setStatus */ );
  int retIndex;
  if ( mCurrentFolder->addMsg( newMessage, &retIndex ) != 0 ) {
    abort( i18n( "Failed to add a message to the folder '%1'.", mCurrentFolder->name() ) );
    return;
  }
  else {
    mNumberOfImportedMessages++;
    if ( mCurrentFolder->folderType() == KMFolderTypeMaildir ||
         mCurrentFolder->folderType() == KMFolderTypeCachedImap ) {
      const QString messageFile = mCurrentFolder->location() + "/cur/" + newMessage->fileName();
      // TODO: what if the file is not in the "cur" subdirectory?
      if ( QFile::exists( messageFile ) ) {
        chmod( messageFile.toLatin1(), file->permissions() );
        // TODO: changing user/group he requires a bit more work, requires converting the strings
        //       to uid_t and gid_t
        //getpwnam()
        //chown( messageFile,
      }
      else {
        kWarning() << "Unable to change permissions for newly created file: " << messageFile;
      }
    }
    // TODO: Else?
    kDebug() << "Added message with subject " /*<< newMessage->subject()*/ // < this causes a pure virtual method to be called...
             << " to folder " << mCurrentFolder->name() << " at index " << retIndex;
  }
  QTimer::singleShot( 0, this, SLOT( importNextMessage() ) );
}

// Input: .inbox.directory
// Output: inbox
// Can also return an empty string if this is no valid dir name
static QString folderNameForDirectoryName( const QString &dirName )
{
  Q_ASSERT( dirName.startsWith( QLatin1String( "." ) ) );
  const QString end = ".directory";
  const int expectedIndex = dirName.length() - end.length();
  if ( dirName.toLower().indexOf( end ) != expectedIndex )
    return QString();
  QString returnName = dirName.left( dirName.length() - end.length() );
  returnName = returnName.right( returnName.length() - 1 );
  return returnName;
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
  kDebug() << "Working on directory " << folder.archiveDir->name();

  QStringList entries = folder.archiveDir->entries();
  for ( int i = 0; i < entries.size(); i++ ) {
    const KArchiveEntry *entry = folder.archiveDir->entry( entries[i] );
    Q_ASSERT( entry );
    kDebug() << "Queueing entry " << entry->name();
    if ( entry->isDirectory() ) {
      const KArchiveDirectory *dir = static_cast<const KArchiveDirectory*>( entry );
      if ( !dir->name().startsWith( QLatin1String( "." ) ) ) {

        kDebug() << "Queueing messages in folder " << entry->name();
        KMFolder *subFolder = createSubFolder( currentFolder, entry->name(), entry->permissions() );
        if ( !subFolder )
          return;

        enqueueMessages( dir, subFolder );
      }

      // Entry starts with a dot, so we assume it is a subdirectory
      else {

        // Check if the subfolder already exists or create it
        KMFolder *subFolder = 0;
        const QString folderName = folderNameForDirectoryName( entry->name() );
        if ( folderName.isEmpty() ) {
          abort( i18n( "Unexpected subdirectory named '%1'.", entry->name() ) );
          return;
        }
        if ( currentFolder->child() ) {
          subFolder = dynamic_cast<KMFolder*>( currentFolder->child()->hasNamedFolder( folderName ) );
        }
        if ( !subFolder ) {
          kDebug() << "Creating subfolder for directory " << entry->name();
          subFolder = createSubFolder( currentFolder, folderName, entry->permissions() );
          if ( !subFolder )
            return;
        }

        Folder newFolder;
        newFolder.archiveDir = dir;
        newFolder.parent = subFolder;
        kDebug() << "Enqueueing directory " << entry->name();
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

  KMimeType::Ptr mimeType = KMimeType::findByUrl( mArchiveFile, 0, true /* local file */ );
  if ( !mimeType->patterns().filter( "tar", Qt::CaseInsensitive ).isEmpty() )
    mArchive = new KTar( mArchiveFile.path() );
  else if ( !mimeType->patterns().filter( "zip", Qt::CaseInsensitive ).isEmpty() )
    mArchive = new KZip( mArchiveFile.path() );
  else {
    abort( i18n( "The file '%1' does not appear to be a valid archive.", mArchiveFile.path() ) );
    return;
  }

  if ( !mArchive->open( IO_ReadOnly ) ) {
    abort( i18n( "Unable to open archive file '%1'", mArchiveFile.path() ) );
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
