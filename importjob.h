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
#include <qlist.h>

class QWidget;
class KArchive;
class KArchiveDirectory;
class KArchiveFile;
class KMFolder;

namespace KMail
{

class ImportJob : public QObject
{
  Q_OBJECT

  public:

    explicit ImportJob( QWidget *parentWidget = 0 );
    ~ImportJob();
    void start();
    void setFile( const KUrl &archiveFile );
    void setRootFolder( KMFolder *rootFolder );

  private slots:

    void importNextMessage();

  private:

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

    void finish();
    void abort( const QString &errorMessage );
    void queueFolders();
    void importNextDirectory();
    KMFolder* createSubFolder( KMFolder *parent, const QString &folderName );
    void enqueueMessages( const KArchiveDirectory *dir, KMFolder *folder );

    KArchive *mArchive;

    // The root folder which the user has selected as the folder to which everything should be
    // imported
    KMFolder *mRootFolder;

    QWidget *mParentWidget;
    KUrl mArchiveFile;
    int mNumberOfImportedMessages;

    // List of archive folders with their corresponding KMail parent folder that are awaiting
    // processing
    QList<Folder> mQueuedDirectories;

    // List of list of messages and their parent folders which are awaiting processing
    QList<Messages> mQueuedMessages;

    // The folder to which we are currently importing messages
    KMFolder *mCurrentFolder;
};

}

#endif
