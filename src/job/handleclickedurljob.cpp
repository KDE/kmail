/*
   SPDX-FileCopyrightText: 2017-2024 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "handleclickedurljob.h"
using namespace Qt::Literals::StringLiterals;

#include "kmail_debug.h"
#include "kmkernel.h"
#include <KMime/Message>
#include <MessageComposer/Composer>
#include <MessageComposer/MessageHelper>
#include <MessageCore/StringUtil>
#include <TemplateParser/TemplateParserJob>

HandleClickedUrlJob::HandleClickedUrlJob(QObject *parent)
    : QObject(parent)
{
}

HandleClickedUrlJob::~HandleClickedUrlJob() = default;

void HandleClickedUrlJob::start()
{
    mMsg = KMime::Message::Ptr(new KMime::Message);
    mIdentity = !mFolder.isNull() ? mFolder->identity() : 0;
    MessageHelper::initHeader(mMsg, KMKernel::self()->identityManager(), mIdentity);
    mMsg->contentType()->setCharset("utf-8");

    const QList<QPair<QString, QString>> fields = MessageCore::StringUtil::parseMailtoUrl(mUrl);
    for (int i = 0; i < fields.count(); ++i) {
        const QPair<QString, QString> element = fields.at(i);
        if (element.first == "to"_L1) {
            mMsg->to()->fromUnicodeString(element.second, QByteArrayLiteral("utf-8"));
        } else if (element.first == "subject"_L1) {
            const QString subject = element.second;
            if (!subject.isEmpty()) {
                mMsg->subject()->fromUnicodeString(subject, QByteArrayLiteral("utf-8"));
            }
        } else if (element.first == "body"_L1) {
            const QString body = element.second;
            if (!body.isEmpty()) {
                mMsg->setBody(body.toUtf8());
            }
        } else if (element.first == "cc"_L1) {
            const QString cc = element.second;
            if (!cc.isEmpty()) {
                mMsg->cc()->fromUnicodeString(cc, QByteArrayLiteral("utf-8"));
            }
        } else if (element.first == "bcc"_L1) {
            const QString bcc = element.second;
            if (!bcc.isEmpty()) {
                mMsg->bcc()->fromUnicodeString(bcc, QByteArrayLiteral("utf-8"));
            }
        } else if (element.first == "attach"_L1) {
            const QString attach = element.second;
            if (!attach.isEmpty()) {
                qCDebug(KMAIL_LOG) << "Attachment not supported yet";
            }
        }
    }

    auto parser = new TemplateParser::TemplateParserJob(mMsg, TemplateParser::TemplateParserJob::NewMessage, this);
    connect(parser, &TemplateParser::TemplateParserJob::parsingDone, this, &HandleClickedUrlJob::slotOpenComposer);
    parser->setIdentityManager(KMKernel::self()->identityManager());
    parser->process(mMsg, mCurrentCollection.id());
}

void HandleClickedUrlJob::slotOpenComposer()
{
    KMail::Composer *win = KMail::makeComposer(mMsg, false, false, KMail::Composer::New, mIdentity);
    win->setFocusToSubject();
    win->setCollectionForNewMessage(mCurrentCollection);
    win->show();
    deleteLater();
}

void HandleClickedUrlJob::setUrl(const QUrl &url)
{
    mUrl = url;
}

void HandleClickedUrlJob::setFolder(const QSharedPointer<MailCommon::FolderSettings> &folder)
{
    mFolder = folder;
}

void HandleClickedUrlJob::setCurrentCollection(const Akonadi::Collection &currentCollection)
{
    mCurrentCollection = currentCollection;
}

#include "moc_handleclickedurljob.cpp"
