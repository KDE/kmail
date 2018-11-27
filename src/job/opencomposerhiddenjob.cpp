/*
   Copyright (C) 2017-2018 Laurent Montel <montel@kde.org>

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

#include "opencomposerhiddenjob.h"
#include "kmkernel.h"
#include "composer.h"
#include "editor/kmcomposerwin.h"

#include <KMime/Message>
#include <MessageComposer/MessageHelper>

#include <TemplateParser/TemplateParserJob>

OpenComposerHiddenJob::OpenComposerHiddenJob(QObject *parent)
    : QObject(parent)
    , mMsg(nullptr)
{
}

OpenComposerHiddenJob::~OpenComposerHiddenJob()
{
}

void OpenComposerHiddenJob::start()
{
    mMsg = KMime::Message::Ptr(new KMime::Message);
    MessageHelper::initHeader(mMsg, KMKernel::self()->identityManager());
    mMsg->contentType()->setCharset("utf-8");
    if (!mSettings.mCc.isEmpty()) {
        mMsg->cc()->fromUnicodeString(mSettings.mCc, "utf-8");
    }
    if (!mSettings.mBcc.isEmpty()) {
        mMsg->bcc()->fromUnicodeString(mSettings.mBcc, "utf-8");
    }
    if (!mSettings.mSubject.isEmpty()) {
        mMsg->subject()->fromUnicodeString(mSettings.mSubject, "utf-8");
    }
    if (!mSettings.mTo.isEmpty()) {
        mMsg->to()->fromUnicodeString(mSettings.mTo, "utf-8");
    }
    if (!mSettings.mBody.isEmpty()) {
        mMsg->setBody(mSettings.mBody.toUtf8());
        slotOpenComposer();
    } else {
        TemplateParser::TemplateParserJob *parser = new TemplateParser::TemplateParserJob(mMsg, TemplateParser::TemplateParserJob::NewMessage, this);
        connect(parser, &TemplateParser::TemplateParserJob::parsingDone, this, &OpenComposerHiddenJob::slotOpenComposer);
        parser->setIdentityManager(KMKernel::self()->identityManager());
        parser->process(KMime::Message::Ptr());
    }
}

void OpenComposerHiddenJob::slotOpenComposer()
{
    mMsg->assemble();
    const KMail::Composer::TemplateContext context = mSettings.mBody.isEmpty() ? KMail::Composer::New
                                                     : KMail::Composer::NoTemplate;
    KMail::Composer *cWin = KMail::makeComposer(mMsg, false, false, context);
    if (!mSettings.mHidden) {
        cWin->show();
    } else {
        // Always disable word wrap when we don't show the composer; otherwise,
        // QTextEdit gets the widget size wrong and wraps much too early.
        cWin->disableWordWrap();
        cWin->slotSendNow();
    }
    deleteLater();
}

void OpenComposerHiddenJob::setSettings(const OpenComposerHiddenJobSettings &settings)
{
    mSettings = settings;
}
