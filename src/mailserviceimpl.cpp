/*
 *
 *  This file is part of KMail, the KDE mail client.
 *  SPDX-FileCopyrightText: 2003 Zack Rusin <zack@kde.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-only
 */

#include "mailserviceimpl.h"
#include "editor/composer.h"
#include "kmkernel.h"
#include <serviceadaptor.h>

// kdepim includes
#include <MessageComposer/MessageHelper>

#include <QUrl>

#include <QDBusConnection>

using namespace KMail;
MailServiceImpl::MailServiceImpl(QObject *parent)
    : QObject(parent)
{
    new ServiceAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/MailTransportService"), this);
}

bool MailServiceImpl::sendMessage(const QString &from,
                                  const QString &to,
                                  const QString &cc,
                                  const QString &bcc,
                                  const QString &subject,
                                  const QString &body,
                                  const QStringList &attachments)
{
    if (to.isEmpty() && cc.isEmpty() && bcc.isEmpty()) {
        return false;
    }

    std::shared_ptr<KMime::Message> msg(new KMime::Message);
    MessageHelper::initHeader(msg, KMKernel::self()->identityManager());

    // Already defined in MessageHelper::initHeader
    msg->contentType(KMime::CreatePolicy::DontCreate)->setCharset(QByteArrayLiteral("utf-8"));

    if (!from.isEmpty()) {
        msg->from()->fromUnicodeString(from);
    }
    if (!to.isEmpty()) {
        msg->to()->fromUnicodeString(to);
    }
    if (!cc.isEmpty()) {
        msg->cc()->fromUnicodeString(cc);
    }
    if (!bcc.isEmpty()) {
        msg->bcc()->fromUnicodeString(bcc);
    }
    if (!subject.isEmpty()) {
        msg->subject()->fromUnicodeString(subject);
    }
    if (!body.isEmpty()) {
        msg->setBody(body.toUtf8());
    }

    KMail::Composer *cWin = KMail::makeComposer(msg);

    QList<QUrl> attachUrls;
    const int nbAttachments = attachments.count();
    attachUrls.reserve(nbAttachments);
    for (int i = 0; i < nbAttachments; ++i) {
        attachUrls += QUrl::fromLocalFile(attachments[i]);
    }

    cWin->addAttachmentsAndSend(attachUrls, QString(), 1); // send now
    return true;
}

bool MailServiceImpl::sendMessage(const QString &from,
                                  const QString &to,
                                  const QString &cc,
                                  const QString &bcc,
                                  const QString &subject,
                                  const QString &body,
                                  const QByteArray &attachment)
{
    if (to.isEmpty() && cc.isEmpty() && bcc.isEmpty()) {
        return false;
    }

    std::shared_ptr<KMime::Message> msg(new KMime::Message);
    MessageHelper::initHeader(msg, KMKernel::self()->identityManager());
    // Already defined in MessageHelper::initHeader
    msg->contentType(KMime::CreatePolicy::DontCreate)->setCharset(QByteArrayLiteral("utf-8"));

    if (!from.isEmpty()) {
        msg->from()->fromUnicodeString(from);
    }
    if (!to.isEmpty()) {
        msg->to()->fromUnicodeString(to);
    }
    if (!cc.isEmpty()) {
        msg->cc()->fromUnicodeString(cc);
    }
    if (!bcc.isEmpty()) {
        msg->bcc()->fromUnicodeString(bcc);
    }
    if (!subject.isEmpty()) {
        msg->subject()->fromUnicodeString(subject);
    }
    if (!body.isEmpty()) {
        msg->setBody(body.toUtf8());
    }

    auto part = std::unique_ptr<KMime::Content>(new KMime::Content);
    part->contentTransferEncoding()->setEncoding(KMime::Headers::CEbase64);
    part->setBody(attachment); // TODO: check it!
    msg->appendContent(std::move(part));

    KMail::makeComposer(msg, false, false);
    return true;
}

#include "moc_mailserviceimpl.cpp"
