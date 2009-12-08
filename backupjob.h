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
#ifndef BACKUPJOB_H
#define BACKUPJOB_H

#include <Akonadi/Collection>
#include <kurl.h>
#include <qlist.h>

#include <qobject.h>

class KArchive;
class KProcess;
class QWidget;

namespace KPIM {
  class ProgressItem;
}

namespace KMail
{

/**
 * Writes an entire folder structure to an archive file.
 * The archive is structured like a hierarchy of maildir folders. However, every type of folder
 * works as the source, i.e. also online IMAP folders.
 *
 * The job deletes itself after it finished.
 */
class BackupJob : public QObject
{
  Q_OBJECT

  public:

    // These enum values have to stay in sync with the format combobox of ArchiveFolderDialog!
    enum ArchiveType { Zip = 0, Tar = 1, TarBz2 = 2, TarGz = 3 };

    explicit BackupJob( QWidget *parent = 0 );
    ~BackupJob();
    void setRootFolder( const Akonadi::Collection &rootFolder );
    void setSaveLocation( const KUrl savePath );
    void setArchiveType( ArchiveType type );
    void setDeleteFoldersAfterCompletion( bool deleteThem );
    void start();

  private slots:

#if 0
    void messageRetrieved( KMMessage *message );
    void folderJobFinished( KMail::FolderJob *job );
#endif
    void processCurrentMessage();
    void cancelJob();

  private:

#if 0
    void queueFolders( KMFolder *root );
#endif
    void archiveNextFolder();
    void archiveNextMessage();
    QString stripRootPath( const QString &path ) const;
    bool hasChildren( const Akonadi::Collection &collection ) const;
    void finish();
    void abort( const QString &errorMessage );
    bool writeDirHelper( const QString &directoryPath );

    KUrl mMailArchivePath;
    ArchiveType mArchiveType;
    Akonadi::Collection mRootFolder;
    KArchive *mArchive;
    QWidget *mParentWidget;
    int mArchivedMessages;
    uint mArchivedSize;
    KPIM::ProgressItem *mProgressItem;
    bool mAborted;
    bool mDeleteFoldersAfterCompletion;

    QList<Akonadi::Collection> mPendingFolders;
    Akonadi::Collection mCurrentFolder;
#if 0
    QList<unsigned long> mPendingMessages;
    KMMessage *mCurrentMessage;
    FolderJob *mCurrentJob;
#endif
};

}

#endif
