/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozeagent.h"
#include "snoozeagentadaptor.h"
#include "snoozeagentsettings.h"
#include "snoozeinfo.h"
#include "snoozemanager.h"

#include <MailCommon/SnoozeAttribute>

#include <Akonadi/AttributeFactory>
#include <Akonadi/ChangeRecorder>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ServerManager>
#include <Akonadi/Session>
#include <KMime/Message>

#include <QDBusConnection>
#include <QTimer>

using namespace Qt::Literals::StringLiterals;

SnoozeAgent::SnoozeAgent(const QString &id)
    : Akonadi::AgentWidgetBase(id)
    , mManager(new SnoozeManager(this))
{
    Akonadi::AttributeFactory::registerAttribute<MailCommon::SnoozeAttribute>();

    new SnoozeAgentAdaptor(this);
    QDBusConnection::sessionBus().registerObject(u"/SnoozeAgent"_s, this, QDBusConnection::ExportAdaptors);
    const QString service = Akonadi::ServerManager::self()->agentServiceName(Akonadi::ServerManager::Agent, u"akonadi_snooze_agent"_s);
    QDBusConnection::sessionBus().registerService(service);

    changeRecorder()->setMimeTypeMonitored(KMime::Message::mimeType());
    changeRecorder()->itemFetchScope().setCacheOnly(true);
    changeRecorder()->itemFetchScope().setFetchModificationTime(false);
    changeRecorder()->fetchCollection(true);
    changeRecorder()->setChangeRecordingEnabled(false);
    changeRecorder()->ignoreSession(Akonadi::Session::defaultSession());

    setNeedsNetwork(false);

    QTimer::singleShot(0, this, &SnoozeAgent::slotStartAgent);
}

SnoozeAgent::~SnoozeAgent() = default;

void SnoozeAgent::slotStartAgent()
{
    mAgentInitialized = true;
    if (isOnline() && enabledAgent()) {
        mManager->load();
    }
}

void SnoozeAgent::setEnableAgent(bool enabled)
{
    if (SnoozeAgentSettings::self()->enabled() == enabled) {
        return;
    }
    SnoozeAgentSettings::self()->setEnabled(enabled);
    SnoozeAgentSettings::self()->save();
    if (enabled) {
        mManager->load();
    } else {
        mManager->stopAll();
    }
}

bool SnoozeAgent::enabledAgent() const
{
    return SnoozeAgentSettings::self()->enabled();
}

void SnoozeAgent::reload()
{
    if (enabledAgent()) {
        mManager->load(true);
    }
}

void SnoozeAgent::snoozeItem(qint64 itemId, qint64 wakeUpDateTime, const QString &subject, const QString &from)
{
    auto snoozeInfo = new Snooze::SnoozeInfo();
    snoozeInfo->setFrom(from);
    snoozeInfo->setSubject(subject);
    snoozeInfo->setWakeUpDateTime(QDateTime::fromMSecsSinceEpoch(wakeUpDateTime));
    snoozeInfo->setItemId(itemId);
    mManager->addSnooze(snoozeInfo);
}

void SnoozeAgent::cancelSnooze(qint64 itemId)
{
    mManager->cancelSnooze(itemId);
}

void SnoozeAgent::wakeUpNow(qint64 itemId)
{
    mManager->wakeUpNow(itemId);
}

void SnoozeAgent::itemsRemoved(const Akonadi::Item::List &items)
{
    // The manager keeps its list and the config in sync itself: no reload needed.
    for (const Akonadi::Item &item : items) {
        (void)mManager->itemRemoved(item.id());
    }
}

void SnoozeAgent::itemsMoved(const Akonadi::Item::List &items,
                             const Akonadi::Collection & /*sourceCollection*/,
                             const Akonadi::Collection & /*destinationCollection*/)
{
    // Moving a snoozed message by hand is an explicit "I am dealing with it
    // now": cancel the snooze rather than letting it pop back up later.
    for (const Akonadi::Item &item : items) {
        if (mManager->isSnoozed(item.id())) {
            mManager->cancelSnooze(item.id());
        }
    }
}

void SnoozeAgent::doSetOnline(bool online)
{
    if (mAgentInitialized) {
        if (online) {
            reload();
        } else {
            mManager->stopAll();
        }
    }
}

QString SnoozeAgent::printDebugInfo() const
{
    return mManager->printDebugInfo();
}

AKONADI_AGENT_MAIN(SnoozeAgent)

#include "moc_snoozeagent.cpp"
