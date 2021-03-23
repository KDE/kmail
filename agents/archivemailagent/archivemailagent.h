/*
   SPDX-FileCopyrightText: 2012-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <AkonadiAgentBase/agentbase.h>

#include <AkonadiCore/Collection>
class QTimer;

class ArchiveMailManager;

class ArchiveMailAgent : public Akonadi::AgentBase, public Akonadi::AgentBase::ObserverV3
{
    Q_OBJECT

public:
    explicit ArchiveMailAgent(const QString &id);
    ~ArchiveMailAgent() override;

    Q_REQUIRED_RESULT QString printArchiveListInfo() const;

    void setEnableAgent(bool b);
    Q_REQUIRED_RESULT bool enabledAgent() const;

    Q_REQUIRED_RESULT QString printCurrentListInfo() const;
    void archiveFolder(const QString &path, Akonadi::Collection::Id collectionId);
Q_SIGNALS:
    void needUpdateConfigDialogBox();

public Q_SLOTS:
    void reload();
    void pause();
    void resume();

protected:
    void doSetOnline(bool online) override;

private:
    void mailCollectionRemoved(const Akonadi::Collection &collection);
    QTimer *mTimer = nullptr;
    ArchiveMailManager *const mArchiveManager;
};

