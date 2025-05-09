/*
   SPDX-FileCopyrightText: 2013-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "sendlateragent.h"
#include "sendlateragent_debug.h"
#include "sendlateragentadaptor.h"
#include "sendlateragentsettings.h"
#include "sendlatermanager.h"
#include "sendlaterutil.h"
#include <Akonadi/AgentInstance>
#include <Akonadi/AgentManager>
#include <Akonadi/AttributeFactory>
#include <Akonadi/ChangeRecorder>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ServerManager>
#include <Akonadi/Session>
#include <Akonadi/SpecialMailCollections>
#include <KMime/Message>
#include <QDBusConnection>

#include <KWindowSystem>
#include <QPointer>
#include <QTimer>
#include <chrono>

using namespace std::chrono_literals;

// #define DEBUG_SENDLATERAGENT 1

SendLaterAgent::SendLaterAgent(const QString &id)
    : Akonadi::AgentWidgetBase(id)
    , mManager(new SendLaterManager(this))
{
    connect(mManager, &SendLaterManager::needUpdateConfigDialogBox, this, &SendLaterAgent::needUpdateConfigDialogBox);
    connect(this, &SendLaterAgent::configurationDialogAccepted, mManager, [this]() {
        mManager->load();
    });
    new SendLaterAgentAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/SendLaterAgent"), this, QDBusConnection::ExportAdaptors);

    const QString service = Akonadi::ServerManager::self()->agentServiceName(Akonadi::ServerManager::Agent, QStringLiteral("akonadi_sendlater_agent"));

    QDBusConnection::sessionBus().registerService(service);

    changeRecorder()->setMimeTypeMonitored(KMime::Message::mimeType());
    changeRecorder()->itemFetchScope().setCacheOnly(true);
    changeRecorder()->itemFetchScope().setFetchModificationTime(false);
    changeRecorder()->setChangeRecordingEnabled(false);
    changeRecorder()->ignoreSession(Akonadi::Session::defaultSession());
    setNeedsNetwork(true);

    if (SendLaterAgentSettings::enabled()) {
#ifdef DEBUG_SENDLATERAGENT
        QTimer::singleShot(1s, this, &SendLaterAgent::slotStartAgent);
#else
        QTimer::singleShot(4min, this, &SendLaterAgent::slotStartAgent);
#endif
    }
    // For extra safety, check list every hour, in case we didn't properly get
    // notified about the network going up or down.
    auto reloadListTimer = new QTimer(this);
    connect(reloadListTimer, &QTimer::timeout, this, &SendLaterAgent::reload);
    reloadListTimer->start(1h);
}

SendLaterAgent::~SendLaterAgent() = default;

void SendLaterAgent::slotStartAgent()
{
    mAgentInitialized = true;
    if (isOnline()) {
        mManager->load();
    }
}

void SendLaterAgent::doSetOnline(bool online)
{
    if (mAgentInitialized) {
        if (online) {
            reload();
        } else {
            mManager->stopAll();
        }
    }
}

void SendLaterAgent::reload()
{
    qCDebug(SENDLATERAGENT_LOG) << " void SendLaterAgent::reload()";
    if (SendLaterAgentSettings::enabled()) {
        mManager->load(true);
    }
}

void SendLaterAgent::setEnableAgent(bool enabled)
{
    if (SendLaterAgentSettings::enabled() == enabled) {
        return;
    }

    SendLaterAgentSettings::setEnabled(enabled);
    SendLaterAgentSettings::self()->save();
    if (enabled) {
        mManager->load();
    } else {
        mManager->stopAll();
    }
}

bool SendLaterAgent::enabledAgent() const
{
    return SendLaterAgentSettings::enabled();
}

void SendLaterAgent::removeItem(qint64 item)
{
    if (mManager->itemRemoved(item)) {
        reload();
    }
}

void SendLaterAgent::addItem(qint64 timestamp,
                             bool recurrence,
                             int recurrenceValue,
                             int recurrenceUnit,
                             Akonadi::Item::Id id,
                             const QString &subject,
                             const QString &to)
{
    auto info = new MessageComposer::SendLaterInfo;
    info->setDateTime(QDateTime::fromSecsSinceEpoch(timestamp));
    info->setRecurrence(recurrence);
    info->setRecurrenceEachValue(recurrenceValue);
    info->setRecurrenceUnit(static_cast<MessageComposer::SendLaterInfo::RecurrenceUnit>(recurrenceUnit));
    info->setItemId(id);
    info->setSubject(subject);
    info->setTo(to);

    SendLaterUtil::writeSendLaterInfo(SendLaterUtil::defaultConfig(), info);
    reload();
}

void SendLaterAgent::slotSendNow(Akonadi::Item::Id id)
{
    mManager->sendNow(id);
}

void SendLaterAgent::itemsRemoved(const Akonadi::Item::List &items)
{
    bool needToReload = false;
    for (const Akonadi::Item &item : items) {
        if (mManager->itemRemoved(item.id())) {
            needToReload = true;
        }
    }
    if (needToReload) {
        reload();
    }
}

void SendLaterAgent::itemsMoved(const Akonadi::Item::List &items,
                                const Akonadi::Collection & /*sourceCollection*/,
                                const Akonadi::Collection &destinationCollection)
{
    if (Akonadi::SpecialMailCollections::self()->specialCollectionType(destinationCollection) != Akonadi::SpecialMailCollections::Trash) {
        return;
    }
    itemsRemoved(items);
}

QString SendLaterAgent::printDebugInfo() const
{
    return mManager->printDebugInfo();
}

AKONADI_AGENT_MAIN(SendLaterAgent)

#include "moc_sendlateragent.cpp"
