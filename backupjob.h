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

#include <kurl.h>
#include <qptrlist.h>

#include <qobject.h>

class KMFolder;
class KMMessage;
class KArchive;
class KProcess;
class QWidget;

namespace KMail
{
  class FolderJob;

class BackupJob : public QObject
{
  Q_OBJECT

  public:

    enum ArchiveType { Zip, Tar, TarBz2, TarGz };

    explicit BackupJob( QWidget *parent = 0 );
    ~BackupJob();
    void setRootFolder( KMFolder *rootFolder );
    void setSaveLocation( const KURL &savePath );
    void setArchiveType( ArchiveType type );
    void start();

  private slots:

    void messageRetrieved( KMMessage *message );
    void folderJobFinished( KMail::FolderJob *job );
    void processCurrentMessage();

  private:

    void queueFolders( KMFolder *root );
    void archiveNextFolder();
    void archiveNextMessage();
    QString stripRootPath( const QString &path ) const;
    bool hasChildren( KMFolder *folder ) const;
    void finish();
    void abort( const QString &errorMessage );

    KURL mMailArchivePath;
    ArchiveType mArchiveType;
    KMFolder *mRootFolder;
    KArchive *mArchive;
    QWidget *mParentWidget;
    bool mCurrentFolderOpen;
    int mArchivedMessages;
    uint mArchivedSize;

    QPtrList<KMFolder> mPendingFolders;
    KMFolder *mCurrentFolder;
    QValueList<unsigned long> mPendingMessages;
    KMMessage *mCurrentMessage;
    FolderJob *mCurrentJob;
};

}

#endif
