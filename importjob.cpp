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

#include <kdebug.h>
#include <kzip.h>
#include <ktar.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetype.h>

#include <qwidget.h>
#include <qtimer.h>

using namespace KMail;

ImportJob::ImportJob( QWidget *parentWidget )
  : QObject( parentWidget ),
    mArchive( 0 ),
    mRootFolder( 0 ),
    mParentWidget( parentWidget ),
    mCurrentFolder( 0 )
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
  kDebug(5006) << "Finished import job.";
  deleteLater();
}

void ImportJob::abort( const QString &errorMessage )
{
  QString text = i18n( "Failed to import the archive into folder '%1'.", mRootFolder->name() );
  text += '\n' + errorMessage;
  KMessageBox::sorry( mParentWidget, text, i18n( "Importing archive failed." ) );
  deleteLater();
}

KMFolder * ImportJob::createSubFolder( KMFolder *parent, const QString &folderName )
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
  else return newFolder;
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
        kWarning(5006) << "Unexpected subdirectory in archive folder " << dir->name();
      }
      else {
        kDebug(5006) << "Queueing message " << entry->name();
        const KArchiveFile *file = static_cast<const KArchiveFile*>( entry );
        messagesToQueue.files.append( file );
      }
    }
    mQueuedMessages.append( messagesToQueue );
  }
  else {
    kWarning(5006) << "No 'cur' subdirectory for archive directory " << dir->name();
  }
}

void ImportJob::importNextMessage()
{
  if ( mQueuedMessages.isEmpty() ) {
    kDebug(5006) << "Processed all messages in the queue.";
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
    kDebug(5006) << "Processed all messages in the current folder of the queue.";
    if ( mCurrentFolder ) {
      mCurrentFolder->close( "ImportJob" );
    }
    mCurrentFolder = folder;
    if ( mCurrentFolder->open( "ImportJob" ) != 0 ) {
      abort( i18n( "Unable to open folder '%1'.", mCurrentFolder->name() ) );
      return;
    }
    kDebug(5006) << "Current folder of queue is now: " << mCurrentFolder->name();
  }
  const KArchiveFile *file = messages.files.first();
  Q_ASSERT( file );
  messages.files.removeFirst();

  KMMessage *newMessage = new KMMessage();
  newMessage->fromString( file->data() );
  int retIndex;
  if ( mCurrentFolder->addMsg( newMessage, &retIndex ) != 0 ) {
    abort( i18n( "Failed to add a message to the folder '%1'.", mCurrentFolder->name() ) );
    return;
  }
  else {
    kDebug(5006) << "Added message with subject " /*<< newMessage->subject()*/ // < this causes a pure virtual method to be called...
                  << " to folder " << mCurrentFolder->name() << " at index " << retIndex;
  }
  QTimer::singleShot( 0, this, SLOT( importNextMessage() ) );
}

void ImportJob::importNextDirectory()
{
  if ( mQueuedDirectories.isEmpty() ) {
    finish();
    return;
  }

  Folder folder = mQueuedDirectories.first();
  KMFolder *currentFolder = folder.parent;
  mQueuedDirectories.pop_front();
  kDebug(5006) << "Working on directory " << folder.archiveDir->name();

  QStringList entries = folder.archiveDir->entries();
  for ( int i = 0; i < entries.size(); i++ ) {
    const KArchiveEntry *entry = folder.archiveDir->entry( entries[i] );
    Q_ASSERT( entry );
    kDebug(5006) << "Queueing entry " << entry->name();
    if ( entry->isDirectory() ) {
      const KArchiveDirectory *dir = static_cast<const KArchiveDirectory*>( entry );
      if ( !dir->name().startsWith( QLatin1String( "." ) ) ) {

        kDebug(5006) << "Queueing messages in folder " << entry->name();
        KMFolder *subFolder = createSubFolder( currentFolder, entry->name() );
        if ( !subFolder )
          return;

        enqueueMessages( dir, subFolder );

        const QString dirName = '.' + entries[i] + ".directory";
        if ( entries.contains( dirName ) ) {
          Q_ASSERT( folder.archiveDir->entry( dirName )->isDirectory() );
          Q_ASSERT( folder.archiveDir->entry( dirName )->name() == dirName );
          Folder newFolder;
          newFolder.archiveDir = static_cast<const KArchiveDirectory*>( folder.archiveDir->entry( dirName ) );
          newFolder.parent = subFolder;
          kDebug(5006) << "Enqueueing directory " << newFolder.archiveDir->name();
          mQueuedDirectories.push_back( newFolder );
        }
      }
    }
  }

  importNextMessage();
}

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

  if ( !mArchive->open( QIODevice::ReadOnly ) ) {
    abort( i18n( "Unable to open archive file '%1'", mArchiveFile.path() ) );
    return;
  }

  Folder nextFolder;
  nextFolder.archiveDir = mArchive->directory();
  nextFolder.parent = mRootFolder;
  mQueuedDirectories.push_back( nextFolder );
  importNextDirectory();
}

#include "importjob.moc"
