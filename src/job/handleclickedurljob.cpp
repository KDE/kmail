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

#include "handleclickedurljob.h"
#include "kmkernel.h"
#include "composer.h"
#include "editor/kmcomposerwin.h"
#include <KMime/Message>
#include <MessageCore/StringUtil>
#include <TemplateParser/TemplateParser>
#include <MessageComposer/MessageHelper>

HandleClickedUrlJob::HandleClickedUrlJob(QObject *parent)
    : QObject(parent)
{
}

HandleClickedUrlJob::~HandleClickedUrlJob()
{
}

void HandleClickedUrlJob::start()
{
    KMime::Message::Ptr msg(new KMime::Message);
    uint identity = !mFolder.isNull() ? mFolder->identity() : 0;
    MessageHelper::initHeader(msg, KMKernel::self()->identityManager(), identity);
    msg->contentType()->setCharset("utf-8");

    QMap<QString, QString> fields =  MessageCore::StringUtil::parseMailtoUrl(mUrl);

    msg->to()->fromUnicodeString(fields.value(QStringLiteral("to")), "utf-8");
    const QString subject = fields.value(QStringLiteral("subject"));
    if (!subject.isEmpty()) {
        msg->subject()->fromUnicodeString(subject, "utf-8");
    }
    const QString body = fields.value(QStringLiteral("body"));
    if (!body.isEmpty()) {
        msg->setBody(body.toUtf8());
    }
    const QString cc = fields.value(QStringLiteral("cc"));
    if (!cc.isEmpty()) {
        msg->cc()->fromUnicodeString(cc, "utf-8");
    }
    const QString bcc = fields.value(QStringLiteral("bcc"));
    if (!bcc.isEmpty()) {
        msg->bcc()->fromUnicodeString(bcc, "utf-8");
    }
    const QString attach = fields.value(QStringLiteral("attach"));
    if (!attach.isEmpty()) {
        //TODO
    }

    if (!mFolder.isNull()) {
        TemplateParser::TemplateParser parser(msg, TemplateParser::TemplateParser::NewMessage);
        parser.setIdentityManager(KMKernel::self()->identityManager());
        parser.process(msg, mFolder->collection());
    }

    KMail::Composer *win = KMail::makeComposer(msg, false, false, KMail::Composer::New, identity);
    win->setFocusToSubject();
    if (!mFolder.isNull()) {
        win->setCollectionForNewMessage(mFolder->collection());
    }
    win->show();
    deleteLater();
}

void HandleClickedUrlJob::setUrl(const QUrl &url)
{
    mUrl = url;
}

void HandleClickedUrlJob::setFolder(const QSharedPointer<MailCommon::FolderCollection> &folder)
{
    mFolder = folder;
}
