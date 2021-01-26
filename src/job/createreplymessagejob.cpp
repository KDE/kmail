/*
   SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "createreplymessagejob.h"

#include "../util.h"
#include "composer.h"
#include "editor/kmcomposerwin.h"
#include "kmkernel.h"
#include <KEmailAddress>
#include <MailCommon/MailUtil>

CreateReplyMessageJob::CreateReplyMessageJob(QObject *parent)
    : QObject(parent)
{
}

CreateReplyMessageJob::~CreateReplyMessageJob()
{
    delete mMessageFactory;
}

void CreateReplyMessageJob::setSettings(const CreateReplyMessageJobSettings &settings)
{
    mSettings = settings;
}

/// Small helper function to get the composer context from a reply
inline KMail::Composer::TemplateContext replyContext(const MessageComposer::MessageFactoryNG::MessageReply &reply)
{
    if (reply.replyAll) {
        return KMail::Composer::ReplyToAll;
    } else {
        return KMail::Composer::Reply;
    }
}

void CreateReplyMessageJob::start()
{
    const auto col = CommonKernel->collectionFromId(mSettings.item.parentCollection().id());
    mMessageFactory = new MessageComposer::MessageFactoryNG(mSettings.msg, mSettings.item.id(), col);
    mMessageFactory->setIdentityManager(KMKernel::self()->identityManager());
    mMessageFactory->setFolderIdentity(MailCommon::Util::folderIdentity(mSettings.item));
    mMessageFactory->setMailingListAddresses(KMail::Util::mailingListsFromMessage(mSettings.item));
    mMessageFactory->putRepliesInSameFolder(KMail::Util::putRepliesInSameFolder(mSettings.item));
    mMessageFactory->setSelection(mSettings.selection);
    mMessageFactory->setTemplate(mSettings.templateStr);
    mMessageFactory->setReplyAsHtml(mSettings.replyAsHtml);
    if (mSettings.noQuote) {
        mMessageFactory->setQuote(false);
    }
    connect(mMessageFactory, &MessageComposer::MessageFactoryNG::createReplyDone, this, &CreateReplyMessageJob::slotCreateReplyDone);
    mMessageFactory->setReplyStrategy(mSettings.replyStrategy);
    mMessageFactory->createReplyAsync();
}

void CreateReplyMessageJob::slotCreateReplyDone(const MessageComposer::MessageFactoryNG::MessageReply &reply)
{
    KMime::Message::Ptr rmsg = reply.msg;
    if (mSettings.url.isValid()) {
        rmsg->to()->fromUnicodeString(KEmailAddress::decodeMailtoUrl(mSettings.url), "utf-8");
    }
    bool lastEncrypt = false;
    bool lastSign = false;
    KMail::Util::lastEncryptAndSignState(lastEncrypt, lastSign, mSettings.msg);

    KMail::Composer *win = KMail::makeComposer(rmsg,
                                               lastSign,
                                               lastEncrypt,
                                               (mSettings.replyStrategy == MessageComposer::ReplyNone) ? KMail::Composer::Reply : replyContext(reply),
                                               0,
                                               mSettings.selection,
                                               mSettings.templateStr);
    win->setFocusToEditor();
    win->show();
    deleteLater();
}
