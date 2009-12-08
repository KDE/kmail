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

#include <Akonadi/Collection>

#include <kurl.h>

#include <qobject.h>
#include <qlist.h>

class QWidget;
class KArchive;
class KArchiveDirectory;
class KArchiveFile;

namespace KPIM
{
  class ProgressItem;
}

namespace KMail
{

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
    void setFile( const KUrl &archiveFile );
    void setRootFolder( const Akonadi::Collection &rootFolder );

  private slots:

    void importNextMessage();
    void cancelJob();

  private:

#if 0
    struct Folder
    {
      KMFolder *parent;
      const KArchiveDirectory *archiveDir;
    };

    struct Messages
    {
      KMFolder *parent;
      QList<const KArchiveFile*> files;
    };
#endif

    void finish();
    void abort( const QString &errorMessage );
    void queueFolders();
    void importNextDirectory();
#if 0
    KMFolder* createSubFolder( KMFolder *parent, const QString &folderName, mode_t permissions );
    KMFolder* getOrCreateSubFolder( KMFolder *parentFolder, const QString &subFolderName,
                                    mode_t subFolderPermissions );
    void enqueueMessages( const KArchiveDirectory *dir, KMFolder *folder );
#endif
    KArchive *mArchive;

    // The root folder which the user has selected as the folder to which everything should be
    // imported
    Akonadi::Collection mRootFolder;

    QWidget *mParentWidget;
    KUrl mArchiveFile;
    int mNumberOfImportedMessages;

#if 0
    // List of archive folders with their corresponding KMail parent folder that are awaiting
    // processing
    QList<Folder> mQueuedDirectories;

    // List of list of messages and their parent folders which are awaiting processing
    QList<Messages> mQueuedMessages;

    // The folder to which we are currently importing messages
    KMFolder *mCurrentFolder;
#endif
    KPIM::ProgressItem *mProgressItem;
    bool mAborted;
};

}

#endif
