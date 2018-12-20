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

#include "handleclickedurljob.h"
#include "kmkernel.h"
#include "composer.h"
#include "kmail_debug.h"
#include "editor/kmcomposerwin.h"
#include <KMime/Message>
#include <MessageCore/StringUtil>
#include <TemplateParser/TemplateParserJob>
#include <MessageComposer/MessageHelper>

HandleClickedUrlJob::HandleClickedUrlJob(QObject *parent)
    : QObject(parent)
    , mIdentity(0)
    , mMsg(nullptr)
{
}

HandleClickedUrlJob::~HandleClickedUrlJob()
{
}

void HandleClickedUrlJob::start()
{
    mMsg = KMime::Message::Ptr(new KMime::Message);
    mIdentity = !mFolder.isNull() ? mFolder->identity() : 0;
    MessageHelper::initHeader(mMsg, KMKernel::self()->identityManager(), mIdentity);
    mMsg->contentType()->setCharset("utf-8");

    const QList<QPair<QString, QString> > fields = MessageCore::StringUtil::parseMailtoUrl(mUrl);
    for (int i = 0; i < fields.count(); ++i) {
        const QPair<QString, QString> element = fields.at(i);
        if (element.first == QStringLiteral("to")) {
            mMsg->to()->fromUnicodeString(element.second, "utf-8");
        } else if (element.first == QStringLiteral("subject")) {
            const QString subject = element.second;
            if (!subject.isEmpty()) {
                mMsg->subject()->fromUnicodeString(subject, "utf-8");
            }
        } else if (element.first == QStringLiteral("body")) {
            const QString body = element.second;
            if (!body.isEmpty()) {
                mMsg->setBody(body.toUtf8());
            }
        } else if (element.first == QStringLiteral("cc")) {
            const QString cc = element.second;
            if (!cc.isEmpty()) {
                mMsg->cc()->fromUnicodeString(cc, "utf-8");
            }
        } else if (element.first == QStringLiteral("bcc")) {
            const QString bcc = element.second;
            if (!bcc.isEmpty()) {
                mMsg->bcc()->fromUnicodeString(bcc, "utf-8");
            }
        } else if (element.first == QStringLiteral("attach")) {
            const QString attach = element.second;
            if (!attach.isEmpty()) {
                qCDebug(KMAIL_LOG) << "Attachment not supported yet";
                //TODO
            }
        }
    }

    TemplateParser::TemplateParserJob *parser = new TemplateParser::TemplateParserJob(mMsg, TemplateParser::TemplateParserJob::NewMessage, this);
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
