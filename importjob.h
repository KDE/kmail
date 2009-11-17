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
#ifndef IMPORTJOB_H
#define IMPORTJOB_H

#include <kurl.h>

#include <qobject.h>
#include <qvaluelist.h>
#include <qptrlist.h>

#include <sys/types.h>

class QWidget;
class KArchive;
class KArchiveDirectory;
class KArchiveFile;
class KMFolder;
class KMMessage;

namespace KPIM
{
  class ProgressItem;
}

namespace KMail
{
  class FolderJob;

/**
 * Imports an archive that was previously backed up with an BackupJob.
 * This job will re-create the folder structure, under the root folder given in setRootFolder().
 *
 * The job deletes itself after it finished.
 */
class ImportJob : public QObject
{
  Q_OBJECT

  public:

    explicit ImportJob( QWidget *parentWidget = 0 );
    ~ImportJob();
    void start();
    void setFile( const KURL &archiveFile );
    void setRootFolder( KMFolder *rootFolder );

  private slots:

    void importNextMessage();
    void cancelJob();
    void messagePutResult( KMail::FolderJob *job );

  private:

    struct Folder
    {
      KMFolder *parent;
      const KArchiveDirectory *archiveDir;
    };

    struct Messages
    {
      KMFolder *parent;
      QPtrList<KArchiveFile> files;
    };

    void finish();
    void abort( const QString &errorMessage );
    void queueFolders();
    void importNextDirectory();
    KMFolder* createSubFolder( KMFolder *parent, const QString &folderName, mode_t permissions );
    KMFolder* getOrCreateSubFolder( KMFolder *parentFolder, const QString &subFolderName,
                                    mode_t subFolderPermissions );
    void enqueueMessages( const KArchiveDirectory *dir, KMFolder *folder );
    void messageAdded();

    KArchive *mArchive;

    // The root folder which the user has selected as the folder to which everything should be
    // imported
    KMFolder *mRootFolder;

    QWidget *mParentWidget;
    KURL mArchiveFile;
    int mNumberOfImportedMessages;

    // List of archive folders with their corresponding KMail parent folder that are awaiting
    // processing
    QValueList<Folder> mQueuedDirectories;

    // List of list of messages and their parent folders which are awaiting processing
    QValueList<Messages> mQueuedMessages;

    // The folder to which we are currently importing messages
    KMFolder *mCurrentFolder;

    // The message which is currently being added
    KMMessage *mCurrentMessage;

    // The archive file of the current message that is being added
    KArchiveFile *mCurrentMessageFile;

    KPIM::ProgressItem *mProgressItem;
    bool mAborted;
};

}

#endif
