/*
   SPDX-FileCopyrightText: 2013-2024 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/AgentBase>

#include <MessageComposer/SendLaterInfo>

class SendLaterManager;

class SendLaterAgent : public Akonadi::AgentBase, public Akonadi::AgentBase::ObserverV3
{
    Q_OBJECT
public:
    explicit SendLaterAgent(const QString &id);
    ~SendLaterAgent() override;

    [[nodiscard]] QString printDebugInfo() const;

    void setEnableAgent(bool b);
    [[nodiscard]] bool enabledAgent() const;

Q_SIGNALS:
    void needUpdateConfigDialogBox();

public Q_SLOTS:
    void reload();
    void removeItem(qint64 item);
    void
    addItem(qint64 timestamp, bool recurrence, int recurrenceValue, int recurrenceUnit, Akonadi::Item::Id itemId, const QString &subject, const QString &to);

protected:
    void itemsRemoved(const Akonadi::Item::List &item) override;
    void itemsMoved(const Akonadi::Item::List &items, const Akonadi::Collection &sourceCollection, const Akonadi::Collection &destinationCollection) override;
    void doSetOnline(bool online) override;

private:
    void slotSendNow(Akonadi::Item::Id id);
    void slotStartAgent();
    bool mAgentInitialized = false;
    SendLaterManager *const mManager;
};
