/*
   Copyright (C) 2017-2020 Laurent Montel <montel@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "createreplymessagejob.h"

#include "kmkernel.h"
#include "../util.h"
#include "composer.h"
#include "editor/kmcomposerwin.h"
#include <KMime/Message>
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
                                               mSettings.selection, mSettings.templateStr);
    win->setFocusToEditor();
    win->show();
    deleteLater();
}
