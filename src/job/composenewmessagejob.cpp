/*
   SPDX-FileCopyrightText: 2017-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "composenewmessagejob.h"
#include "kmkernel.h"
#include <MessageComposer/ComposerJob>

#include <KMime/Message>
#include <MessageComposer/MessageHelper>
#include <TemplateParser/TemplateParserJob>

ComposeNewMessageJob::ComposeNewMessageJob(QObject *parent)
    : QObject(parent)
{
}

ComposeNewMessageJob::~ComposeNewMessageJob() = default;

void ComposeNewMessageJob::setCurrentCollection(const Akonadi::Collection &col)
{
    mCurrentCollection = col;
}

static void copyAddresses(const KMime::Headers::Generics::AddressList *from, KMime::Headers::Generics::AddressList *to)
{
    if (!from) { // no such headers to copy from
        return;
    }

    const QList<KMime::Types::Mailbox> mailboxes = from->mailboxes();
    for (const KMime::Types::Mailbox &mbox : mailboxes) {
        to->addAddress(mbox);
    }
}

void ComposeNewMessageJob::start()
{
    mMsg = std::shared_ptr<KMime::Message>(new KMime::Message());

    mIdentity = mFolder ? mFolder->identity() : 0;
    MessageHelper::initHeader(mMsg, KMKernel::self()->identityManager(), mIdentity);
    auto parser = new TemplateParser::TemplateParserJob(mMsg, TemplateParser::TemplateParserJob::NewMessage, this);
    connect(parser, &TemplateParser::TemplateParserJob::parsingDone, this, &ComposeNewMessageJob::slotOpenComposer);
    parser->setIdentityManager(KMKernel::self()->identityManager());
    if (mFolder) {
        parser->process(mMsg, mCurrentCollection.id());
    } else {
        parser->process(std::shared_ptr<KMime::Message>());
    }

    if (mRecipientsFrom.isValid()) {
        // Copy the recipient list from the original message
        const std::shared_ptr<KMime::Message> msg = MessageComposer::Util::message(mRecipientsFrom);
        if (msg) {
            copyAddresses(msg->to(false), mMsg->to());
            copyAddresses(msg->cc(false), mMsg->cc());
            copyAddresses(msg->bcc(false), mMsg->bcc());
            copyAddresses(msg->replyTo(false), mMsg->replyTo());
        } else {
            qCWarning(KMAIL_LOG) << "Original message" << mRecipientsFrom.id() << "not found";
        }
    }
}

void ComposeNewMessageJob::slotOpenComposer(bool forceCursorPosition)
{
    KMail::Composer *win = KMail::makeComposer(mMsg, false, false, KMail::Composer::TemplateContext::New, mIdentity);
    win->setCollectionForNewMessage(mCurrentCollection);

    if (forceCursorPosition) {
        win->setFocusToEditor();
    }
    win->show();
    deleteLater();
}

void ComposeNewMessageJob::setFolderSettings(const QSharedPointer<MailCommon::FolderSettings> &folder)
{
    mFolder = folder;
}

void ComposeNewMessageJob::setRecipientsFromMessage(const Akonadi::Item &from)
{
    mRecipientsFrom = from;
}

#include "moc_composenewmessagejob.cpp"
