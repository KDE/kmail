/*
   SPDX-FileCopyrightText: 2012-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/AgentBase>

#include <Akonadi/Collection>
class QTimer;

class ArchiveMailManager;

class ArchiveMailAgent : public Akonadi::AgentBase, public Akonadi::AgentBase::ObserverV3
{
    Q_OBJECT

public:
    explicit ArchiveMailAgent(const QString &id);
    ~ArchiveMailAgent() override;

    [[nodiscard]] QString printArchiveListInfo() const;

    void setEnableAgent(bool b);
    [[nodiscard]] bool enabledAgent() const;

    [[nodiscard]] QString printCurrentListInfo() const;
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
    QTimer *const mTimer;
    ArchiveMailManager *const mArchiveManager;
};
