/*
   SPDX-FileCopyrightText: 2014-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "followupreminderagent.h"
#include "followupreminderadaptor.h"
#include "followupreminderagentsettings.h"
#include "followupreminderinfo.h"
#include "followupremindermanager.h"

#include <KMime/Message>

#include <Akonadi/ChangeRecorder>
#include <Akonadi/ItemFetchScope>
#include <QDBusConnection>

#include "followupreminderagent_debug.h"
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/ServerManager>
#include <Akonadi/Session>
#include <QTimer>
#include <chrono>

using namespace std::chrono_literals;

FollowUpReminderAgent::FollowUpReminderAgent(const QString &id)
    : Akonadi::AgentWidgetBase(id)
    , mManager(new FollowUpReminderManager(this))
    , mTimer(new QTimer(this))
{
    new FollowUpReminderAgentAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/FollowUpReminder"), this, QDBusConnection::ExportAdaptors);
    const QString service = Akonadi::ServerManager::self()->agentServiceName(Akonadi::ServerManager::Agent, QStringLiteral("akonadi_followupreminder_agent"));
    QDBusConnection::sessionBus().registerService(service);
    setNeedsNetwork(true);

    changeRecorder()->setMimeTypeMonitored(KMime::Message::mimeType());
    changeRecorder()->itemFetchScope().setCacheOnly(true);
    changeRecorder()->itemFetchScope().setFetchModificationTime(false);
    changeRecorder()->fetchCollection(true);
    changeRecorder()->setChangeRecordingEnabled(false);
    changeRecorder()->ignoreSession(Akonadi::Session::defaultSession());
    changeRecorder()->collectionFetchScope().setAncestorRetrieval(Akonadi::CollectionFetchScope::All);
    changeRecorder()->setCollectionMonitored(Akonadi::Collection::root(), true);

    if (FollowUpReminderAgentSettings::enabled()) {
        mManager->load();
    }

    connect(mTimer, &QTimer::timeout, this, &FollowUpReminderAgent::reload);
    // Reload each day
    mTimer->start(24h);
}

FollowUpReminderAgent::~FollowUpReminderAgent() = default;

void FollowUpReminderAgent::setEnableAgent(bool enabled)
{
    if (FollowUpReminderAgentSettings::self()->enabled() == enabled) {
        return;
    }

    FollowUpReminderAgentSettings::self()->setEnabled(enabled);
    FollowUpReminderAgentSettings::self()->save();
    if (enabled) {
        mManager->load();
        mTimer->start();
    } else {
        mTimer->stop();
    }
}

bool FollowUpReminderAgent::enabledAgent() const
{
    return FollowUpReminderAgentSettings::self()->enabled();
}

void FollowUpReminderAgent::itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    if (!enabledAgent()) {
        return;
    }

    if (item.mimeType() != KMime::Message::mimeType()) {
        qCDebug(FOLLOWUPREMINDERAGENT_LOG) << "FollowUpReminderAgent::itemAdded called for a non-message item!";
        return;
    }
    mManager->checkFollowUp(item, collection);
}

void FollowUpReminderAgent::reload()
{
    if (enabledAgent()) {
        mManager->load(true);
        mTimer->start();
    }
}

void FollowUpReminderAgent::addReminder(const QString &messageId,
                                        Akonadi::Item::Id messageItemId,
                                        const QString &to,
                                        const QString &subject,
                                        QDate followupDate,
                                        Akonadi::Item::Id todoId)
{
    auto info = new FollowUpReminder::FollowUpReminderInfo();
    info->setMessageId(messageId);
    info->setOriginalMessageItemId(messageItemId);
    info->setTo(to);
    info->setSubject(subject);
    info->setFollowUpReminderDate(followupDate);
    info->setTodoId(todoId);

    mManager->addReminder(info);
}

QString FollowUpReminderAgent::printDebugInfo() const
{
    return mManager->printDebugInfo();
}

AKONADI_AGENT_MAIN(FollowUpReminderAgent)

#include "moc_followupreminderagent.cpp"
