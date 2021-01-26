/*
   SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "sendlaterjob.h"

#include <MessageComposer/AkonadiSender>
#include <MessageComposer/SendLaterInfo>
#include <MessageComposer/Util>
#include <MessageCore/StringUtil>

#include <MailTransport/Transport>
#include <MailTransport/TransportManager>
#include <MailTransportAkonadi/SentBehaviourAttribute>
#include <MailTransportAkonadi/TransportAttribute>

#include <ItemDeleteJob>
#include <ItemFetchJob>

#include "sendlateragent_debug.h"
#include <KLocalizedString>
#include <KNotification>

SendLaterJob::SendLaterJob(SendLaterManager *manager, MessageComposer::SendLaterInfo *info, QObject *parent)
    : QObject(parent)
    , mManager(manager)
    , mInfo(info)
{
    qCDebug(SENDLATERAGENT_LOG) << " SendLaterJob::SendLaterJob" << this;
}

SendLaterJob::~SendLaterJob()
{
    qCDebug(SENDLATERAGENT_LOG) << " SendLaterJob::~SendLaterJob()" << this;
}

void SendLaterJob::start()
{
    if (mInfo) {
        if (mInfo->itemId() > -1) {
            const Akonadi::Item item = Akonadi::Item(mInfo->itemId());
            auto fetch = new Akonadi::ItemFetchJob(item, this);
            mFetchScope.fetchAttribute<MailTransport::TransportAttribute>();
            mFetchScope.fetchAttribute<MailTransport::SentBehaviourAttribute>();
            mFetchScope.setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
            mFetchScope.fetchFullPayload(true);
            fetch->setFetchScope(mFetchScope);
            connect(fetch, &Akonadi::ItemFetchJob::itemsReceived, this, &SendLaterJob::slotMessageTransfered);
            connect(fetch, &Akonadi::ItemFetchJob::result, this, &SendLaterJob::slotJobFinished);
            fetch->start();
        } else {
            qCDebug(SENDLATERAGENT_LOG) << " message Id is invalid";
            sendError(i18n("Not message found."), SendLaterManager::ItemNotFound);
        }
    } else {
        sendError(i18n("Not message found."), SendLaterManager::UnknownError);
        qCDebug(SENDLATERAGENT_LOG) << " Item is null. It's a bug!";
    }
}

void SendLaterJob::slotMessageTransfered(const Akonadi::Item::List &items)
{
    if (items.isEmpty()) {
        sendError(i18n("No message found."), SendLaterManager::ItemNotFound);
        qCDebug(SENDLATERAGENT_LOG) << " slotMessageTransfered failed !";
        return;
    } else if (items.count() == 1) {
        // Success
        mItem = items.first();
        return;
    }
    qCDebug(SENDLATERAGENT_LOG) << "Error during fetching message.";
    sendError(i18n("Error during fetching message."), SendLaterManager::TooManyItemFound);
}

void SendLaterJob::slotJobFinished(KJob *job)
{
    if (job->error()) {
        sendError(i18n("Cannot fetch message. %1", job->errorString()), SendLaterManager::CanNotFetchItem);
        return;
    }
    if (!MailTransport::TransportManager::self()->showTransportCreationDialog(nullptr, MailTransport::TransportManager::IfNoTransportExists)) {
        qCDebug(SENDLATERAGENT_LOG) << " we can't create transport ";
        sendError(i18n("We can't create transport"), SendLaterManager::CanNotCreateTransport);
        return;
    }

    if (mItem.isValid()) {
        const KMime::Message::Ptr msg = MessageComposer::Util::message(mItem);
        if (!msg) {
            sendError(i18n("Message is not a real message"), SendLaterManager::CanNotFetchItem);
            return;
        }
        // TODO verify encryption + signed
        updateAndCleanMessageBeforeSending(msg);

        if (!mManager->sender()->send(msg, MessageComposer::MessageSender::SendImmediate)) {
            sendError(i18n("Cannot send message."), SendLaterManager::MailDispatchDoesntWork);
        } else {
            if (!mInfo->isRecurrence()) {
                auto fetch = new Akonadi::ItemDeleteJob(mItem, this);
                connect(fetch, &Akonadi::ItemDeleteJob::result, this, &SendLaterJob::slotDeleteItem);
            } else {
                sendDone();
            }
        }
    }
}

void SendLaterJob::updateAndCleanMessageBeforeSending(const KMime::Message::Ptr &msg)
{
    msg->date()->setDateTime(QDateTime::currentDateTime());
    MessageComposer::Util::removeNotNecessaryHeaders(msg);
    msg->assemble();
}

void SendLaterJob::slotDeleteItem(KJob *job)
{
    if (job->error()) {
        qCDebug(SENDLATERAGENT_LOG) << " void SendLaterJob::slotDeleteItem( KJob *job ) :" << job->errorString();
    }
    sendDone();
}

void SendLaterJob::sendDone()
{
    KNotification::event(QStringLiteral("mailsend"),
                         QString(),
                         i18n("Message sent"),
                         QStringLiteral("kmail"),
                         nullptr,
                         KNotification::CloseOnTimeout,
                         QStringLiteral("akonadi_sendlater_agent"));
    mManager->sendDone(mInfo);
    deleteLater();
}

void SendLaterJob::sendError(const QString &error, SendLaterManager::ErrorType type)
{
    KNotification::event(QStringLiteral("mailsendfailed"),
                         QString(),
                         error,
                         QStringLiteral("kmail"),
                         nullptr,
                         KNotification::CloseOnTimeout,
                         QStringLiteral("akonadi_sendlater_agent"));
    mManager->sendError(mInfo, type);
    deleteLater();
}
