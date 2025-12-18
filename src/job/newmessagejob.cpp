/*
   SPDX-FileCopyrightText: 2017-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "newmessagejob.h"
#include "kmkernel.h"
#include <MessageComposer/ComposerJob>

#include <KMime/Message>
#include <MessageComposer/MessageHelper>
#include <TemplateParser/TemplateParserJob>

NewMessageJob::NewMessageJob(QObject *parent)
    : QObject(parent)
{
}

NewMessageJob::~NewMessageJob() = default;

void NewMessageJob::start()
{
    mAttachURL = QUrl::fromLocalFile(mNewMessageJobSettings.mAttachURL);
    mMsg = std::shared_ptr<KMime::Message>(new KMime::Message);
    MessageHelper::initHeader(mMsg, KMKernel::self()->identityManager(), mNewMessageJobSettings.mIdentity);
    // Already defined in MessageHelper::initHeader
    mMsg->contentType(KMime::CreatePolicy::DontCreate)->setCharset(QByteArrayLiteral("utf-8"));
    // set basic headers
    if (!mNewMessageJobSettings.mCc.isEmpty()) {
        mMsg->cc()->fromUnicodeString(mNewMessageJobSettings.mCc);
    }
    if (!mNewMessageJobSettings.mBcc.isEmpty()) {
        mMsg->bcc()->fromUnicodeString(mNewMessageJobSettings.mBcc);
    }
    if (!mNewMessageJobSettings.mTo.isEmpty()) {
        mMsg->to()->fromUnicodeString(mNewMessageJobSettings.mTo);
    }

    mMsg->assemble();

    mCollection = mNewMessageJobSettings.mCurrentCollection;
    auto parser = new TemplateParser::TemplateParserJob(mMsg, TemplateParser::TemplateParserJob::NewMessage, this);
    connect(parser, &TemplateParser::TemplateParserJob::parsingDone, this, &NewMessageJob::slotOpenComposer);
    parser->setIdentityManager(KMKernel::self()->identityManager());
    parser->process(mMsg, mCollection.id());
}

void NewMessageJob::slotOpenComposer()
{
    KMail::Composer *win = makeComposer(mMsg, false, false, KMail::Composer::TemplateContext::New, mNewMessageJobSettings.mIdentity);

    win->setCollectionForNewMessage(mCollection);
    // Add the attachment if we have one

    if (!mAttachURL.isEmpty() && mAttachURL.isValid()) {
        QList<KMail::Composer::AttachmentInfo> infoList;
        KMail::Composer::AttachmentInfo info;
        info.url = mAttachURL;
        infoList.append(std::move(info));
        win->addAttachment(infoList, false);
    }

    // only show window when required
    if (!mNewMessageJobSettings.mHidden) {
        win->show();
    }
    deleteLater();
}

void NewMessageJob::setNewMessageJobSettings(const NewMessageJobSettings &newMessageJobSettings)
{
    mNewMessageJobSettings = newMessageJobSettings;
}

#include "moc_newmessagejob.cpp"
