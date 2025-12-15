/*
   SPDX-FileCopyrightText: 2017-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "opencomposerhiddenjob.h"
#include "kmkernel.h"
#include <MessageComposer/ComposerJob>

#include <MessageComposer/MessageHelper>

#include <TemplateParser/TemplateParserJob>

OpenComposerHiddenJob::OpenComposerHiddenJob(QObject *parent)
    : QObject(parent)
{
}

OpenComposerHiddenJob::~OpenComposerHiddenJob() = default;

void OpenComposerHiddenJob::start()
{
    mMsg = QSharedPointer<KMime::Message>(new KMime::Message);
    MessageHelper::initHeader(mMsg, KMKernel::self()->identityManager());
    // Already defined in MessageHelper::initHeader
    mMsg->contentType(false)->setCharset(QByteArrayLiteral("utf-8"));
    if (!mSettings.mCc.isEmpty()) {
        mMsg->cc()->fromUnicodeString(mSettings.mCc);
    }
    if (!mSettings.mBcc.isEmpty()) {
        mMsg->bcc()->fromUnicodeString(mSettings.mBcc);
    }
    if (!mSettings.mSubject.isEmpty()) {
        mMsg->subject()->fromUnicodeString(mSettings.mSubject);
    }
    if (!mSettings.mTo.isEmpty()) {
        mMsg->to()->fromUnicodeString(mSettings.mTo);
    }
    if (!mSettings.mBody.isEmpty()) {
        mMsg->setBody(mSettings.mBody.toUtf8());
        slotOpenComposer();
    } else {
        auto parser = new TemplateParser::TemplateParserJob(mMsg, TemplateParser::TemplateParserJob::NewMessage, this);
        connect(parser, &TemplateParser::TemplateParserJob::parsingDone, this, &OpenComposerHiddenJob::slotOpenComposer);
        parser->setIdentityManager(KMKernel::self()->identityManager());
        parser->process(QSharedPointer<KMime::Message>());
    }
}

void OpenComposerHiddenJob::slotOpenComposer()
{
    mMsg->assemble();
    const KMail::Composer::TemplateContext context =
        mSettings.mBody.isEmpty() ? KMail::Composer::TemplateContext::New : KMail::Composer::TemplateContext::NoTemplate;
    KMail::Composer *cWin = KMail::makeComposer(mMsg, false, false, context);
    if (!mSettings.mHidden) {
        cWin->showAndActivateComposer();
    } else {
        // Always disable word wrap when we don't show the composer; otherwise,
        // *TextEdit gets the widget size wrong and wraps much too early.
        cWin->disableWordWrap();
        cWin->slotSendNow();
    }
    deleteLater();
}

void OpenComposerHiddenJob::setSettings(const OpenComposerHiddenJobSettings &settings)
{
    mSettings = settings;
}

#include "moc_opencomposerhiddenjob.cpp"
