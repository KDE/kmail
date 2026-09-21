/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/AgentWidgetBase>
#include <Akonadi/Collection>
#include <Akonadi/Item>

class SnoozeManager;

class SnoozeAgent : public Akonadi::AgentWidgetBase, public Akonadi::AgentBase::ObserverV3
{
    Q_OBJECT
public:
    explicit SnoozeAgent(const QString &id);
    ~SnoozeAgent() override;

    [[nodiscard]] QString printDebugInfo() const;

    void setEnableAgent(bool enabled);
    [[nodiscard]] bool enabledAgent() const;

public Q_SLOTS:
    void reload();

    /** @p wakeUpDateTime is a UTC seconds-since-epoch timestamp. */
    void snoozeItem(qint64 itemId, qint64 wakeUpDateTime, const QString &subject, const QString &from);
    void cancelSnooze(qint64 itemId);
    void wakeUpNow(qint64 itemId);

protected:
    void itemsRemoved(const Akonadi::Item::List &items) override;
    void itemsMoved(const Akonadi::Item::List &items, const Akonadi::Collection &sourceCollection, const Akonadi::Collection &destinationCollection) override;
    void doSetOnline(bool online) override;

private:
    void slotStartAgent();

    SnoozeManager *const mManager;
    bool mAgentInitialized = false;
};
