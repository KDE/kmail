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

#include "progressmanager.h"

#include <Akonadi/CollectionCreateJob>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/Item>
#include <Akonadi/ItemCreateJob>
#include <KMime/Message>

#include <kdebug.h>
#include <kzip.h>
#include <ktar.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetype.h>

#include <QWidget>
#include <QTimer>

using namespace KMail;
using namespace Akonadi;

ImportJob::ImportJob( QWidget *parentWidget )
  : QObject( parentWidget ),
    mArchive( 0 ),
    mRootFolder( 0 ),
    mParentWidget( parentWidget ),
    mNumberOfImportedMessages( 0 ),
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

void ImportJob::setRootFolder( const Collection &rootFolder )
{
  mRootFolder = rootFolder;
}

void ImportJob::finish()
{
  kDebug() << "Finished import job.";
  mProgressItem->setComplete();
  mProgressItem = 0;
  QString text = i18n( "Importing the archive file '%1' into the folder '%2' succeeded.",
                       mArchiveFile.path(), mRootFolder.name() );
  text += '\n' + i18np( "1 message was imported.", "%1 messages were imported.", mNumberOfImportedMessages );
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
  QString text = i18n( "Failed to import the archive into folder '%1'.", mRootFolder.name() );
  text += '\n' + errorMessage;
  if ( mProgressItem ) {
    mProgressItem->setComplete();
    mProgressItem = 0;
    // The progressmanager will delete it
  }
  KMessageBox::sorry( mParentWidget, text, i18n( "Importing archive failed." ) );
  deleteLater();
}

void ImportJob::enqueueMessages( const KArchiveDirectory *dir, const Collection &folder )
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
    mCurrentFolder = Collection();
    importNextDirectory();
    return;
  }

  Messages &messages = mQueuedMessages.front();
  if ( messages.files.isEmpty() ) {
    mQueuedMessages.pop_front();
    importNextMessage();
    return;
  }

  Collection folder = messages.parent;
  if ( folder != mCurrentFolder ) {
    kDebug() << "Processed all messages in the current folder of the queue.";
    mCurrentFolder = folder;
    kDebug() << "Current folder of queue is now: " << mCurrentFolder.name();
    mProgressItem->setStatus( i18n( "Importing folder '%1'", mCurrentFolder.name() ) );
  }

  const KArchiveFile *file = messages.files.first();
  Q_ASSERT( file );
  messages.files.removeFirst();

  KMime::Message::Ptr newMessage( new KMime::Message() );
  newMessage->setContent( file->data() );
  newMessage->parse();

  Akonadi::Item item;
  item.setMimeType( "message/rfc822" );
  item.setPayload<KMime::Message::Ptr>( newMessage );

  // FIXME: Get rid of the exec()
  ItemCreateJob *job = new ItemCreateJob( item, mCurrentFolder );
  job->exec();
  if ( job->error() ) {
    abort( i18n( "Failed to add a message to the folder '%1'.", mCurrentFolder.name() ) );
    return;
  }
  else {
    mNumberOfImportedMessages++;
    kDebug() << "Added message with subject" << newMessage->subject()
             << "to folder" << mCurrentFolder.name();
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

Collection ImportJob::getOrCreateSubFolder( const Collection &parentFolder,
                                            const QString &subFolderName )
{
  // First, get list of subcollections and see if it already has the requested
  // folder
  // FIXME: Get rid of the exec()
  CollectionFetchJob *fetchJob = new CollectionFetchJob( parentFolder );
  fetchJob->exec();
  if ( fetchJob->error() ) {
    abort( i18n( "Unable to retrieve list of subfolders for folder '%1'", parentFolder.name() ) );
    return Collection();
  }

  foreach( const Collection &col, fetchJob->collections() ) {
    if ( col.name() == subFolderName )
      return col;
  }

  // No subcollection with this name yet, create it
  // FIXME: This does not make the new collection inherit the parent's settings
  // FIXME: Get rid of the exec()
  Collection newCollection;
  newCollection.setName( subFolderName );
  newCollection.parentCollection().setId( parentFolder.id() );
  CollectionCreateJob *job = new CollectionCreateJob( newCollection );
  job->exec();
  if ( job->error() ) {
    abort( i18n( "Unable to create subfolder for folder '%1'", parentFolder.name() ) );
    return Collection();
  }
  return job->collection();
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
  Collection currentFolder = folder.parent;
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
        Collection subFolder = getOrCreateSubFolder( currentFolder, entry->name() );
        if ( !subFolder.isValid() )
          return;

        enqueueMessages( dir, subFolder );
      }

      // Entry starts with a dot, so we assume it is a subdirectory
      else {

        const QString folderName = folderNameForDirectoryName( entry->name() );
        if ( folderName.isEmpty() ) {
          abort( i18n( "Unexpected subdirectory named '%1'.", entry->name() ) );
          return;
        }
        Collection subFolder = getOrCreateSubFolder( currentFolder, folderName );
        if ( !subFolder.isValid() )
          return;

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

void ImportJob::start()
{
  Q_ASSERT( mRootFolder.isValid() );
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

  if ( !mArchive->open( QIODevice::ReadOnly ) ) {
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
