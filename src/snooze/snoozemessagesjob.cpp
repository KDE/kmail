/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozemessagesjob.h"
#include "kmail_debug.h"
#include "snoozeagentinterface.h"
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/MessageParts>
#include <Akonadi/ServerManager>
#include <KMime/Message>

using namespace Qt::Literals::StringLiterals;

SnoozeMessagesJob::SnoozeMessagesJob(QObject *parent)
    : QObject(parent)
{
}

SnoozeMessagesJob::~SnoozeMessagesJob() = default;

void SnoozeMessagesJob::start()
{
    if (!canStart()) {
        qCDebug(KMAIL_LOG) << "SnoozeMessagesJob can not be started";
        deleteLater();
        return;
    }
    auto job = new Akonadi::ItemFetchJob(mItems, this);
    job->fetchScope().fetchPayloadPart(Akonadi::MessagePart::Envelope);
    connect(job, &KJob::result, this, &SnoozeMessagesJob::slotItemFetchDone);
}

void SnoozeMessagesJob::slotItemFetchDone(KJob *job)
{
    if (job->error()) {
        qCWarning(KMAIL_LOG) << "SnoozeMessagesJob: error during fetch:" << job->errorString();
        deleteLater();
        return;
    }
    const auto service = Akonadi::ServerManager::self()->agentServiceName(Akonadi::ServerManager::Agent, u"akonadi_snooze_agent"_s);
    OrgFreedesktopAkonadiSnoozeAgentInterface snoozeInterface(service, u"/SnoozeAgent"_s, QDBusConnection::sessionBus());
    if (!snoozeInterface.isValid()) {
        qCWarning(KMAIL_LOG) << "org.freedesktop.Akonadi.SnoozeAgent not found. Please verify your installation";
        deleteLater();
        return;
    }
    const qint64 wakeUp = mWakeUpDateTime.toUTC().toSecsSinceEpoch();
    const Akonadi::Item::List items = static_cast<Akonadi::ItemFetchJob *>(job)->items();
    for (const Akonadi::Item &item : items) {
        QString subject;
        QString from;
        if (item.hasPayload<std::shared_ptr<KMime::Message>>()) {
            const auto msg = item.payload<std::shared_ptr<KMime::Message>>();
            if (const KMime::Headers::Subject *subjectHeader = msg->subject(KMime::CreatePolicy::DontCreate)) {
                subject = subjectHeader->asUnicodeString();
            }
            if (const KMime::Headers::From *fromHeader = msg->from(KMime::CreatePolicy::DontCreate)) {
                from = fromHeader->asUnicodeString();
            }
        }
        snoozeInterface.snoozeItem(item.id(), wakeUp, subject, from);
    }
    deleteLater();
}

Akonadi::Item::List SnoozeMessagesJob::items() const
{
    return mItems;
}

void SnoozeMessagesJob::setItems(const Akonadi::Item::List &items)
{
    mItems = items;
}

QDateTime SnoozeMessagesJob::wakeUpDateTime() const
{
    return mWakeUpDateTime;
}

void SnoozeMessagesJob::setWakeUpDateTime(const QDateTime &wakeUpDateTime)
{
    mWakeUpDateTime = wakeUpDateTime;
}

bool SnoozeMessagesJob::canStart() const
{
    return !mItems.isEmpty() && mWakeUpDateTime.isValid();
}

#include "moc_snoozemessagesjob.cpp"
