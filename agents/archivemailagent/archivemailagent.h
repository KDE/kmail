/*
   Copyright (C) 2012-2018 Montel Laurent <montel@kde.org>

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

#ifndef ARCHIVEMAILAGENT_H
#define ARCHIVEMAILAGENT_H

#include <AkonadiAgentBase/agentbase.h>

#include <AkonadiCore/Collection>
class QTimer;

class ArchiveMailManager;
class ArchiveMailInfo;

class ArchiveMailAgent : public Akonadi::AgentBase, public Akonadi::AgentBase::ObserverV3
{
    Q_OBJECT

public:
    explicit ArchiveMailAgent(const QString &id);
    ~ArchiveMailAgent() override;

    void showConfigureDialog(qlonglong windowId = 0);
    QString printArchiveListInfo();

    void setEnableAgent(bool b);
    bool enabledAgent() const;

    QString printCurrentListInfo();
    void archiveFolder(const QString &path, Akonadi::Collection::Id collectionId);
Q_SIGNALS:
    void archiveNow(ArchiveMailInfo *info);
    void needUpdateConfigDialogBox();

public Q_SLOTS:
    void configure(WId windowId) override;
    void reload();
    void pause();
    void resume();

protected:
    void doSetOnline(bool online) override;

private:
    void mailCollectionRemoved(const Akonadi::Collection &collection);
    QTimer *mTimer = nullptr;
    ArchiveMailManager *mArchiveManager = nullptr;
};

#endif /* ARCHIVEMAILAGENT_H */
