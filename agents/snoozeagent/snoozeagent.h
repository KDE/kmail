/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Collection>
#include <Akonadi/Item>
#include <Akonadi/ResourceWidgetBase>

namespace Akonadi
{
class Monitor;
}
class SnoozeManager;

/* Like the unified mailbox agent, this is a Resource acting as an Agent: being
 * a resource is the only way to own the virtual "Snoozed" collection into
 * which snoozed messages are linked.
 * The resource's ChangeRecorder only watches that collection, so the messages
 * of the other resources are watched through a separate Monitor.
 */
class SnoozeAgent : public Akonadi::ResourceWidgetBase
{
    Q_OBJECT
public:
    explicit SnoozeAgent(const QString &id);
    ~SnoozeAgent() override;

    [[nodiscard]] QString printDebugInfo() const;

    void setEnableAgent(bool enabled);
    [[nodiscard]] bool enabledAgent() const;

Q_SIGNALS:
    void needUpdateConfigDialogBox();

public Q_SLOTS:
    void reload();

    /** @p wakeUpDateTime is a UTC seconds-since-epoch timestamp. */
    void snoozeItem(qint64 itemId, qint64 wakeUpDateTime, const QString &subject, const QString &from);
    void cancelSnooze(qint64 itemId);
    void wakeUpNow(qint64 itemId);

protected:
    void retrieveCollections() override;
    void retrieveItems(const Akonadi::Collection &collection) override;
    [[nodiscard]] bool retrieveItems(const Akonadi::Item::List &items, const QSet<QByteArray> &parts) override;
    void doSetOnline(bool online) override;

private:
    void slotStartAgent();
    void slotItemsRemoved(const Akonadi::Item::List &items);
    void slotItemsMoved(const Akonadi::Item::List &items);
    void discoverSnoozeCollection();

    SnoozeManager *const mManager;
    Akonadi::Monitor *const mMonitor;
    bool mAgentInitialized = false;
};
