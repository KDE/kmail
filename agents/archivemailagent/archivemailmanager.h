/*
   Copyright (C) 2012-2019 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef ARCHIVEMAILMANAGER_H
#define ARCHIVEMAILMANAGER_H

#include <QObject>

#include <KSharedConfig>
#include <Collection>

class ArchiveMailKernel;
class ArchiveMailInfo;

class ArchiveMailManager : public QObject
{
    Q_OBJECT
public:
    explicit ArchiveMailManager(QObject *parent = nullptr);
    ~ArchiveMailManager();
    void removeCollection(const Akonadi::Collection &collection);
    void backupDone(ArchiveMailInfo *info);
    void pause();
    void resume();

    QString printArchiveListInfo() const;
    void collectionDoesntExist(ArchiveMailInfo *info);

    QString printCurrentListInfo() const;

    void archiveFolder(const QString &path, Akonadi::Collection::Id collectionId);

    ArchiveMailKernel *kernel() const
    {
        return mArchiveMailKernel;
    }

public Q_SLOTS:
    void load();

Q_SIGNALS:
    void needUpdateConfigDialogBox();

private:
    Q_DISABLE_COPY(ArchiveMailManager)
    QString infoToStr(ArchiveMailInfo *info) const;
    void removeCollectionId(Akonadi::Collection::Id id);
    KSharedConfig::Ptr mConfig;
    QVector<ArchiveMailInfo *> mListArchiveInfo;
    ArchiveMailKernel *mArchiveMailKernel = nullptr;
};

#endif /* ARCHIVEMAILMANAGER_H */
