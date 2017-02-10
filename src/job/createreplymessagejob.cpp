/*
   Copyright (C) 2017 Laurent Montel <montel@kde.org>

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
#include <QUrl>
#include <QDebug>

CreateReplyMessageJob::CreateReplyMessageJob(QObject *parent)
    : QObject(parent)
    #ifdef KDEPIM_TEMPLATEPARSER_ASYNC_BUILD
    , mMessageFactory(nullptr)
    #endif
{
    qDebug() << " CreateReplyMessageJob::CreateReplyMessageJob(QObject *parent)"<<this;
}

CreateReplyMessageJob::~CreateReplyMessageJob()
{
#ifdef KDEPIM_TEMPLATEPARSER_ASYNC_BUILD
    delete mMessageFactory;
#endif
    qDebug() << " CreateReplyMessageJob::~CreateReplyMessageJob(QObject *parent)"<<this;
}

void CreateReplyMessageJob::setSettings(const CreateReplyMessageJobSettings &settings)
{
    mSettings = settings;
}

/// Small helper function to get the composer context from a reply
#ifdef KDEPIM_TEMPLATEPARSER_ASYNC_BUILD
static KMail::Composer::TemplateContext replyContext(MessageComposer::MessageFactoryNG::MessageReply reply)
#else
static KMail::Composer::TemplateContext replyContext(MessageComposer::MessageFactory::MessageReply reply)
#endif
{
    if (reply.replyAll) {
        return KMail::Composer::ReplyToAll;
    } else {
        return KMail::Composer::Reply;
    }
}

void CreateReplyMessageJob::start()
{
#ifdef KDEPIM_TEMPLATEPARSER_ASYNC_BUILD
    qDebug() << " void CreateReplyMessageJob::start()";
    mMessageFactory = new MessageComposer::MessageFactoryNG(mSettings.mMsg, mSettings.mItem.id(), MailCommon::Util::updatedCollection(mSettings.mItem.parentCollection()));
    mMessageFactory->setIdentityManager(KMKernel::self()->identityManager());
    mMessageFactory->setFolderIdentity(MailCommon::Util::folderIdentity(mSettings.mItem));
    mMessageFactory->setMailingListAddresses(KMail::Util::mailingListsFromMessage(mSettings.mItem));
    mMessageFactory->putRepliesInSameFolder(KMail::Util::putRepliesInSameFolder(mSettings.mItem));
    mMessageFactory->setSelection(mSettings.mSelection);
    if (!mSettings.mTemplate.isEmpty()) {
        mMessageFactory->setTemplate(mSettings.mTemplate);
    }
    if (mSettings.mNoQuote) {
        mMessageFactory->setQuote(false);
    }
    connect(mMessageFactory, &MessageComposer::MessageFactoryNG::createReplyDone, this, &CreateReplyMessageJob::slotCreateReplyDone);
    mMessageFactory->setReplyStrategy(mSettings.m_replyStrategy);
    mMessageFactory->createReplyAsync();

#else
    MessageComposer::MessageFactory factory(mSettings.mMsg, mSettings.mItem.id(), MailCommon::Util::updatedCollection(mSettings.mItem.parentCollection()));
    factory.setIdentityManager(KMKernel::self()->identityManager());
    factory.setFolderIdentity(MailCommon::Util::folderIdentity(mSettings.mItem));
    factory.setMailingListAddresses(KMail::Util::mailingListsFromMessage(mSettings.mItem));
    factory.putRepliesInSameFolder(KMail::Util::putRepliesInSameFolder(mSettings.mItem));
    factory.setSelection(mSettings.mSelection);
    if (!mSettings.mTemplate.isEmpty()) {
        factory.setTemplate(mSettings.mTemplate);
    }
    if (mSettings.mNoQuote) {
        factory.setQuote(false);
    }
    factory.setReplyStrategy(mSettings.m_replyStrategy);

    MessageComposer::MessageFactory::MessageReply reply = factory.createReply();
    KMime::Message::Ptr rmsg = reply.msg;
    if (mSettings.mUrl.isValid()) {
        rmsg->to()->fromUnicodeString(KEmailAddress::decodeMailtoUrl(mSettings.mUrl), "utf-8");
    }
    bool lastEncrypt = false;
    bool lastSign = false;
    KMail::Util::lastEncryptAndSignState(lastEncrypt, lastSign, mSettings.mMsg);

    KMail::Composer *win = KMail::makeComposer(rmsg,
                                               lastSign,
                                               lastEncrypt,
                                               (mSettings.m_replyStrategy == MessageComposer::ReplyNone) ? KMail::Composer::Reply : replyContext(reply),
                                               0,
                                               mSettings.mSelection, mSettings.mTemplate);
    win->setFocusToEditor();
    win->show();
    deleteLater();
#endif
}

#ifdef KDEPIM_TEMPLATEPARSER_ASYNC_BUILD
void CreateReplyMessageJob::slotCreateReplyDone(const MessageComposer::MessageFactoryNG::MessageReply &reply)
{
    qDebug() << " void CreateReplyMessageJob::slotCreateReplyDone(const MessageComposer::MessageFactoryNG::MessageReply &reply)";
    KMime::Message::Ptr rmsg = reply.msg;
    if (mSettings.mUrl.isValid()) {
        rmsg->to()->fromUnicodeString(KEmailAddress::decodeMailtoUrl(mSettings.mUrl), "utf-8");
    }
    bool lastEncrypt = false;
    bool lastSign = false;
    KMail::Util::lastEncryptAndSignState(lastEncrypt, lastSign, mSettings.mMsg);

    KMail::Composer *win = KMail::makeComposer(rmsg,
                                               lastSign,
                                               lastEncrypt,
                                               (mSettings.m_replyStrategy == MessageComposer::ReplyNone) ? KMail::Composer::Reply : replyContext(reply),
                                               0,
                                               mSettings.mSelection, mSettings.mTemplate);
    win->setFocusToEditor();
    win->show();
    deleteLater();
}
#endif
