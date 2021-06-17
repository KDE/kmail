/*
   SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "sendlateragent.h"
#include "sendlateragent_debug.h"
#include "sendlateragentadaptor.h"
#include "sendlateragentsettings.h"
#include "sendlaterconfiguredialog.h"
#include "sendlaterinfo.h"
#include "sendlatermanager.h"
#include "sendlaterremovemessagejob.h"
#include "sendlaterutil.h"
#include <AgentInstance>
#include <AgentManager>
#include <Akonadi/KMime/SpecialMailCollections>
#include <AkonadiCore/ServerManager>
#include <AkonadiCore/session.h>
#include <AttributeFactory>
#include <CollectionFetchScope>
#include <KMime/Message>
#include <QDBusConnection>
#include <changerecorder.h>
#include <itemfetchscope.h>

#include <KWindowSystem>
#include <kcoreaddons_version.h>
#if KCOREADDONS_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <Kdelibs4ConfigMigrator>
#endif
#include <QPointer>
#include <chrono>

using namespace std::chrono_literals;

//#define DEBUG_SENDLATERAGENT 1

SendLaterAgent::SendLaterAgent(const QString &id)
    : Akonadi::AgentBase(id)
    , mManager(new SendLaterManager(this))
{
#if KCOREADDONS_VERSION < QT_VERSION_CHECK(6, 0, 0)
    Kdelibs4ConfigMigrator migrate(QStringLiteral("sendlateragent"));
    migrate.setConfigFiles(QStringList() << QStringLiteral("akonadi_sendlater_agentrc") << QStringLiteral("akonadi_sendlater_agent.notifyrc"));
    migrate.migrate();
#endif

    connect(mManager, &SendLaterManager::needUpdateConfigDialogBox, this, &SendLaterAgent::needUpdateConfigDialogBox);
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
        QTimer::singleShot(1000, this, &SendLaterAgent::slotStartAgent);
#else
        QTimer::singleShot(4min, this, &SendLaterAgent::slotStartAgent);
#endif
    }
    // For extra safety, check list every hour, in case we didn't properly get
    // notified about the network going up or down.
    auto reloadListTimer = new QTimer(this);
    connect(reloadListTimer, &QTimer::timeout, this, &SendLaterAgent::reload);
    reloadListTimer->start(1h); // 1 hour
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

void SendLaterAgent::configure(WId windowId)
{
    QPointer<SendLaterConfigureDialog> dialog = new SendLaterConfigureDialog();
    if (windowId) {
        dialog->setAttribute(Qt::WA_NativeWindow, true);
        KWindowSystem::setMainWindow(dialog->windowHandle(), windowId);
    }
    connect(this, &SendLaterAgent::needUpdateConfigDialogBox, dialog.data(), &SendLaterConfigureDialog::slotNeedToReloadConfig);
    connect(dialog.data(), &SendLaterConfigureDialog::sendNow, this, &SendLaterAgent::slotSendNow);
    if (dialog->exec()) {
        mManager->load();
        const QVector<Akonadi::Item::Id> listMessage = dialog->messagesToRemove();
        if (!listMessage.isEmpty()) {
            // Will delete in specific job when done.
            auto sendlaterremovejob = new SendLaterRemoveMessageJob(listMessage, this);
            sendlaterremovejob->start();
        }
    }
    delete dialog;
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
