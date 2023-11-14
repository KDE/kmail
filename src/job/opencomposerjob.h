/*
   SPDX-FileCopyrightText: 2017-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "editor/kmcomposerwin.h"
#include "kmail_private_export.h"
#include <KMime/Message>
#include <QObject>
struct KMAILTESTS_TESTS_EXPORT OpenComposerSettings {
    OpenComposerSettings() = default;

    OpenComposerSettings(const QString &to,
                         const QString &cc,
                         const QString &bcc,
                         const QString &subject,
                         const QString &body,
                         bool hidden,
                         const QString &messageFile,
                         const QStringList &attachmentPaths,
                         const QStringList &customHeaders,
                         const QString &replyTo,
                         const QString &inReplyTo,
                         const QString &identity)
        : mAttachmentPaths(attachmentPaths)
        , mCustomHeaders(customHeaders)
        , mTo(to)
        , mCc(cc)
        , mBcc(bcc)
        , mSubject(subject)
        , mBody(body)
        , mMessageFile(messageFile)
        , mReplyTo(replyTo)
        , mInReplyTo(inReplyTo)
        , mIdentity(identity)
        , mHidden(hidden)
    {
    }

    QStringList mAttachmentPaths;
    QStringList mCustomHeaders;
    QString mTo;
    QString mCc;
    QString mBcc;
    QString mSubject;
    QString mBody;
    QString mMessageFile;
    QString mReplyTo;
    QString mInReplyTo;
    QString mIdentity;
    bool mHidden = false;
};

class KMAILTESTS_TESTS_EXPORT OpenComposerJob : public QObject
{
    Q_OBJECT
public:
    explicit OpenComposerJob(QObject *parent = nullptr);
    ~OpenComposerJob() override;
    void start();

    void setOpenComposerSettings(const OpenComposerSettings &openComposerSettings);

private:
    void slotOpenComposer();
    OpenComposerSettings mOpenComposerSettings;
    KMime::Message::Ptr mMsg = nullptr;
    KMail::Composer::TemplateContext mContext = KMail::Composer::New;
    uint mIdentityId = 0;
};
