/*
   SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "handleclickedurljob.h"
#include "composer.h"
#include "editor/kmcomposerwin.h"
#include "kmail_debug.h"
#include "kmkernel.h"
#include <KMime/Message>
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

    const QVector<QPair<QString, QString>> fields = MessageCore::StringUtil::parseMailtoUrl(mUrl);
    for (int i = 0; i < fields.count(); ++i) {
        const QPair<QString, QString> element = fields.at(i);
        if (element.first == QLatin1String("to")) {
            mMsg->to()->fromUnicodeString(element.second, "utf-8");
        } else if (element.first == QLatin1String("subject")) {
            const QString subject = element.second;
            if (!subject.isEmpty()) {
                mMsg->subject()->fromUnicodeString(subject, "utf-8");
            }
        } else if (element.first == QLatin1String("body")) {
            const QString body = element.second;
            if (!body.isEmpty()) {
                mMsg->setBody(body.toUtf8());
            }
        } else if (element.first == QLatin1String("cc")) {
            const QString cc = element.second;
            if (!cc.isEmpty()) {
                mMsg->cc()->fromUnicodeString(cc, "utf-8");
            }
        } else if (element.first == QLatin1String("bcc")) {
            const QString bcc = element.second;
            if (!bcc.isEmpty()) {
                mMsg->bcc()->fromUnicodeString(bcc, "utf-8");
            }
        } else if (element.first == QLatin1String("attach")) {
            const QString attach = element.second;
            if (!attach.isEmpty()) {
                qCDebug(KMAIL_LOG) << "Attachment not supported yet";
                // TODO
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
