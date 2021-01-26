/*
   SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "newmessagejob.h"
#include "composer.h"
#include "editor/kmcomposerwin.h"
#include "kmkernel.h"

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
    mMsg = KMime::Message::Ptr(new KMime::Message);
    MessageHelper::initHeader(mMsg, KMKernel::self()->identityManager(), mNewMessageJobSettings.mIdentity);
    mMsg->contentType()->setCharset("utf-8");
    // set basic headers
    if (!mNewMessageJobSettings.mCc.isEmpty()) {
        mMsg->cc()->fromUnicodeString(mNewMessageJobSettings.mCc, "utf-8");
    }
    if (!mNewMessageJobSettings.mBcc.isEmpty()) {
        mMsg->bcc()->fromUnicodeString(mNewMessageJobSettings.mBcc, "utf-8");
    }
    if (!mNewMessageJobSettings.mTo.isEmpty()) {
        mMsg->to()->fromUnicodeString(mNewMessageJobSettings.mTo, "utf-8");
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
    KMail::Composer *win = makeComposer(mMsg, false, false, KMail::Composer::New, mNewMessageJobSettings.mIdentity);

    win->setCollectionForNewMessage(mCollection);
    // Add the attachment if we have one

    if (!mAttachURL.isEmpty() && mAttachURL.isValid()) {
        QVector<KMail::Composer::AttachmentInfo> infoList;
        KMail::Composer::AttachmentInfo info;
        info.url = mAttachURL;
        infoList.append(info);
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
