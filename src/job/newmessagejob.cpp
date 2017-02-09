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

#include "newmessagejob.h"
#include "config-kmail.h"
#include "kmkernel.h"
#include "composer.h"
#include "editor/kmcomposerwin.h"

#include <KMime/Message>
#include <MessageComposer/MessageHelper>
#ifdef KDEPIM_TEMPLATEPARSER_ASYNC_BUILD
#include <TemplateParser/TemplateParserJob>
#else
#include <TemplateParser/TemplateParser>
#endif

NewMessageJob::NewMessageJob(QObject *parent)
    : QObject(parent),
      mMsg(nullptr)
{

}

NewMessageJob::~NewMessageJob()
{

}

void NewMessageJob::start()
{
    mAttachURL = QUrl::fromLocalFile(mNewMessageJobSettings.mAttachURL);
    mMsg = KMime::Message::Ptr(new KMime::Message);
    MessageHelper::initHeader(mMsg, KMKernel::self()->identityManager(), mNewMessageJobSettings.mIdentity);
    mMsg->contentType()->setCharset("utf-8");
    //set basic headers
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

    mCollection = mNewMessageJobSettings.mFolder ? mNewMessageJobSettings.mFolder->collection() : Akonadi::Collection();
#ifdef KDEPIM_TEMPLATEPARSER_ASYNC_BUILD
    TemplateParser::TemplateParserJob *parser = new TemplateParser::TemplateParserJob(mMsg, TemplateParser::TemplateParserJob::NewMessage);
    connect(parser, &TemplateParser::TemplateParserJob::parsingDone, this, &NewMessageJob::slotOpenComposer);
    parser->setIdentityManager(KMKernel::self()->identityManager());
    parser->process(mMsg, mCollection);
#else
    TemplateParser::TemplateParser parser(mMsg, TemplateParser::TemplateParser::NewMessage);
    parser.setIdentityManager(KMKernel::self()->identityManager());
    parser.process(mMsg, mCollection);
    slotOpenComposer();
#endif
}

void NewMessageJob::slotOpenComposer()
{
    KMail::Composer *win = makeComposer(mMsg, false, false, KMail::Composer::New, mNewMessageJobSettings.mIdentity);

    win->setCollectionForNewMessage(mCollection);
    //Add the attachment if we have one
    if (!mAttachURL.isEmpty() && mAttachURL.isValid()) {
        win->addAttachment(mAttachURL, QString());
    }

    //only show window when required
    if (!mNewMessageJobSettings.mHidden) {
        win->show();
    }
    deleteLater();
}

void NewMessageJob::setNewMessageJobSettings(const NewMessageJobSettings &newMessageJobSettings)
{
    mNewMessageJobSettings = newMessageJobSettings;
}
