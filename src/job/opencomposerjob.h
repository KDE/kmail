/*
   Copyright (C) 2017-2019 Laurent Montel <montel@kde.org>

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

#ifndef OPENCOMPOSERJOB_H
#define OPENCOMPOSERJOB_H

#include <QObject>
#include <KMime/Message>
#include "editor/kmcomposerwin.h"
struct OpenComposerSettings
{
    OpenComposerSettings()
        : mHidden(false)
    {
    }

    OpenComposerSettings(const QString &to, const QString &cc, const QString &bcc, const QString &subject, const QString &body, bool hidden, const QString &messageFile, const QStringList &attachmentPaths, const QStringList &customHeaders, const QString &replyTo, const QString &inReplyTo, const QString &identity)
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

class OpenComposerJob : public QObject
{
    Q_OBJECT
public:
    explicit OpenComposerJob(QObject *parent = nullptr);
    ~OpenComposerJob();
    void start();

    void setOpenComposerSettings(const OpenComposerSettings &openComposerSettings);

private:
    Q_DISABLE_COPY(OpenComposerJob)
    void slotOpenComposer();
    OpenComposerSettings mOpenComposerSettings;
    KMime::Message::Ptr mMsg;
    KMail::Composer::TemplateContext mContext;
    uint mIdentityId = 0;
};

#endif // OPENCOMPOSERJOB_H
