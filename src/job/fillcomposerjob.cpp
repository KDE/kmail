/*
   SPDX-FileCopyrightText: 2017-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "fillcomposerjob.h"

#include "kmkernel.h"
#include <MessageComposer/ComposerJob>
#include <MessageViewer/MessageViewerSettings>

#include <MessageComposer/MessageHelper>
#include <TemplateParser/TemplateParserJob>

using namespace Qt::Literals::StringLiterals;
FillComposerJob::FillComposerJob(QObject *parent)
    : QObject(parent)
{
}

FillComposerJob::~FillComposerJob() = default;

void FillComposerJob::start()
{
    mMsg = QSharedPointer<KMime::Message>(new KMime::Message);
    MessageHelper::initHeader(mMsg, KMKernel::self()->identityManager(), mSettings.mIdentity);
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
    if (mSettings.mIdentity > 0) {
        auto h = std::unique_ptr<KMime::Headers::Generic>(new KMime::Headers::Generic("X-KMail-Identity"));
        h->from7BitString(QByteArray::number(mSettings.mIdentity));
        mMsg->setHeader(std::move(h));
    }
    if (!mSettings.mBody.isEmpty()) {
        mMsg->setBody(mSettings.mBody.toUtf8());
        slotOpenComposer();
    } else {
        auto parser = new TemplateParser::TemplateParserJob(mMsg, TemplateParser::TemplateParserJob::NewMessage, this);
        connect(parser, &TemplateParser::TemplateParserJob::parsingDone, this, &FillComposerJob::slotOpenComposer);
        parser->setIdentityManager(KMKernel::self()->identityManager());
        parser->process(QSharedPointer<KMime::Message>());
    }
}

void FillComposerJob::slotOpenComposer()
{
    KMail::Composer::TemplateContext context = KMail::Composer::TemplateContext::New;
    KMime::Content *msgPart = nullptr;
    bool iCalAutoSend = false;
    bool noWordWrap = false;
    bool isICalInvitation = false;
    if (!mSettings.mAttachData.isEmpty()) {
        isICalInvitation = (mSettings.mAttachName == "cal.ics"_L1) && mSettings.mAttachType == "text" && mSettings.mAttachSubType == "calendar"
            && mSettings.mAttachParamAttr == "method";
        // Remove BCC from identity on ical invitations (https://intevation.de/roundup/kolab/issue474)
        if (isICalInvitation && mSettings.mBcc.isEmpty()) {
            mMsg->removeHeader<KMime::Headers::Bcc>();
        }
        if (isICalInvitation && MessageViewer::MessageViewerSettings::self()->legacyBodyInvites()) {
            // KOrganizer invitation caught and to be sent as body instead
            mMsg->setBody(mSettings.mAttachData);
            mMsg->contentType()->from7BitString(QStringLiteral("text/calendar; method=%1; "
                                                               "charset=\"utf-8\"")
                                                    .arg(mSettings.mAttachParamValue)
                                                    .toLatin1());

            iCalAutoSend = true; // no point in editing raw ICAL
            noWordWrap = true; // we shouldn't word wrap inline invitations
        } else {
            // Just do what we're told to do
            msgPart = new KMime::Content;
            msgPart->contentTransferEncoding()->fromUnicodeString(QLatin1StringView(mSettings.mAttachCte));
            msgPart->setBody(mSettings.mAttachData); // TODO: check if was setBodyEncoded
            auto ct = msgPart->contentType(); // Create
            ct->setMimeType(mSettings.mAttachType + '/' + mSettings.mAttachSubType);
            ct->setParameter(mSettings.mAttachParamAttr, mSettings.mAttachParamValue); // TODO: Check if the content disposition parameter needs to be set!
            if (!MessageViewer::MessageViewerSettings::self()->exchangeCompatibleInvitations()) {
                msgPart->contentDisposition()->fromUnicodeString(QLatin1StringView(mSettings.mAttachContDisp));
            }
            if (!mSettings.mAttachCharset.isEmpty()) {
                // qCDebug(KMAIL_LOG) << "Set attachCharset to" << attachCharset;
                ct->setCharset(mSettings.mAttachCharset);
            }

            ct->setName(mSettings.mAttachName);
            msgPart->assemble();
            // Don't show the composer window if the automatic sending is checked
            iCalAutoSend = MessageViewer::MessageViewerSettings::self()->automaticSending();
        }
    }

    mMsg->assemble();

    if (!mMsg->body().isEmpty()) {
        context = KMail::Composer::TemplateContext::NoTemplate;
    }

    KMail::Composer *cWin = KMail::makeComposer(QSharedPointer<KMime::Message>(), false, false, context);
    cWin->setMessage(mMsg, false, false, !isICalInvitation /* mayAutoSign */);
    cWin->setSigningAndEncryptionDisabled(isICalInvitation && MessageViewer::MessageViewerSettings::self()->legacyBodyInvites());
    if (noWordWrap) {
        cWin->disableWordWrap();
    }
    if (msgPart) {
        cWin->addAttach(msgPart);
    }
    if (isICalInvitation) {
        cWin->disableWordWrap();
        cWin->forceDisableHtml();
        cWin->disableForgottenAttachmentsCheck();
    }
    if (mSettings.mForceShowWindow || (!mSettings.mHidden && !iCalAutoSend)) {
        cWin->showAndActivateComposer();
    } else {
        // Always disable word wrap when we don't show the composer, since otherwise *TextEdit
        // gets the widget size wrong and wraps much too early.
        cWin->disableWordWrap();
        cWin->slotSendNow();
    }
    deleteLater();
}

void FillComposerJob::setSettings(const FillComposerJobSettings &settings)
{
    mSettings = settings;
}

#include "moc_fillcomposerjob.cpp"
