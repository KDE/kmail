/*
   SPDX-FileCopyrightText: 2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mailmergeagent.h"
#include "mailmergeagent_debug.h"
#include "mailmergeagentadaptor.h"
#include "mailmergeagentsettings.h"
#include "mailmergeconfiguredialog.h"
#include "mailmergemanager.h"
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

#include <QPointer>
#include <QTimer>
#include <chrono>

using namespace std::chrono_literals;

//#define DEBUG_MAILMERGEAGENT 1

MailMergeAgent::MailMergeAgent(const QString &id)
    : Akonadi::AgentBase(id)
    , mManager(new MailMergeManager(this))
{
    connect(mManager, &MailMergeManager::needUpdateConfigDialogBox, this, &MailMergeAgent::needUpdateConfigDialogBox);
    new MailMergeAgentAdaptor(this);
    QDBusConnection::sessionBus().registerObject(
                QStringLiteral("/MailMergeAgent"), this, QDBusConnection::ExportAdaptors);

    const QString service = Akonadi::ServerManager::self()->agentServiceName(
                Akonadi::ServerManager::Agent, QStringLiteral("akonadi_mergemail_agent"));

    QDBusConnection::sessionBus().registerService(service);

    changeRecorder()->setMimeTypeMonitored(KMime::Message::mimeType());
    changeRecorder()->itemFetchScope().setCacheOnly(true);
    changeRecorder()->itemFetchScope().setFetchModificationTime(false);
    changeRecorder()->setChangeRecordingEnabled(false);
    changeRecorder()->ignoreSession(Akonadi::Session::defaultSession());
    setNeedsNetwork(true);

    if (MailMergeAgentSettings::enabled()) {
#ifdef DEBUG_MailMergeAgent
        QTimer::singleShot(1000, this, &MailMergeAgent::slotStartAgent);
#else
        QTimer::singleShot(4min, this, &MailMergeAgent::slotStartAgent);
#endif
    }
    // For extra safety, check list every hour, in case we didn't properly get
    // notified about the network going up or down.
    auto reloadListTimer = new QTimer(this);
    connect(reloadListTimer, &QTimer::timeout, this, &MailMergeAgent::reload);
    reloadListTimer->start(1h); // 1 hour
}

MailMergeAgent::~MailMergeAgent() = default;

void MailMergeAgent::slotStartAgent()
{
    mAgentInitialized = true;
    if (isOnline()) {
        mManager->load();
    }
}

void MailMergeAgent::doSetOnline(bool online)
{
    if (mAgentInitialized) {
        if (online) {
            reload();
        } else {
            mManager->stopAll();
        }
    }
}

void MailMergeAgent::reload()
{
    qCDebug(MAILMERGEAGENT_LOG) << " void MailMergeAgent::reload()";
    if (MailMergeAgentSettings::enabled()) {
        mManager->load(true);
    }
}

void MailMergeAgent::setEnableAgent(bool enabled)
{
    if (MailMergeAgentSettings::enabled() == enabled) {
        return;
    }

    MailMergeAgentSettings::setEnabled(enabled);
    MailMergeAgentSettings::self()->save();
    if (enabled) {
        mManager->load();
    } else {
        mManager->stopAll();
    }
}

bool MailMergeAgent::enabledAgent() const
{
    return MailMergeAgentSettings::enabled();
}

void MailMergeAgent::configure(WId windowId)
{
    QPointer<MailMergeConfigureDialog> dialog = new MailMergeConfigureDialog();
    if (windowId) {
        dialog->setAttribute(Qt::WA_NativeWindow, true);
        KWindowSystem::setMainWindow(dialog->windowHandle(), windowId);
    }
    //    connect(this, &MailMergeAgent::needUpdateConfigDialogBox, dialog.data(), &MailMergeConfigureDialog::slotNeedToReloadConfig);
    //    connect(dialog.data(), &SendLaterConfigureDialog::sendNow, this, &MailMergeAgent::slotSendNow);
    if (dialog->exec()) {
        // TODO
        //        mManager->load();
        //        const QVector<Akonadi::Item::Id> listMessage = dialog->messagesToRemove();
        //        if (!listMessage.isEmpty()) {
        //            // Will delete in specific job when done.
        //            auto sendlaterremovejob = new SendLaterRemoveMessageJob(listMessage, this);
        //            sendlaterremovejob->start();
        //        }
    }
    delete dialog;
}

void MailMergeAgent::removeItem(qint64 item)
{
    if (mManager->itemRemoved(item)) {
        reload();
    }
}

void MailMergeAgent::addItem(qint64 timestamp,
                             bool recurrence,
                             int recurrenceValue,
                             int recurrenceUnit,
                             Akonadi::Item::Id id,
                             const QString &subject,
                             const QString &to)
{
    //    auto info = new MessageComposer::SendLaterInfo;
    //    info->setDateTime(QDateTime::fromSecsSinceEpoch(timestamp));
    //    info->setRecurrence(recurrence);
    //    info->setRecurrenceEachValue(recurrenceValue);
    //    info->setRecurrenceUnit(static_cast<MessageComposer::SendLaterInfo::RecurrenceUnit>(recurrenceUnit));
    //    info->setItemId(id);
    //    info->setSubject(subject);
    //    info->setTo(to);

    //    SendLaterUtil::writeSendLaterInfo(SendLaterUtil::defaultConfig(), info);
    //    reload();
}

void MailMergeAgent::itemsRemoved(const Akonadi::Item::List &items)
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

void MailMergeAgent::itemsMoved(const Akonadi::Item::List &items,
                                const Akonadi::Collection & /*sourceCollection*/,
                                const Akonadi::Collection &destinationCollection)
{
    if (Akonadi::SpecialMailCollections::self()->specialCollectionType(destinationCollection) != Akonadi::SpecialMailCollections::Trash) {
        return;
    }
    itemsRemoved(items);
}

QString MailMergeAgent::printDebugInfo() const
{
    return mManager->printDebugInfo();
}

AKONADI_AGENT_MAIN(MailMergeAgent)
