/*
   SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once
#include "kmail_private_export.h"
#include <QDebug>
#include <QUrl>

class KMAILTESTS_TESTS_EXPORT CommandLineInfo
{
public:
    CommandLineInfo();
    ~CommandLineInfo();

    void parseCommandLine(const QStringList &args, const QString &workingDir);
    [[nodiscard]] QStringList customHeaders() const;

    [[nodiscard]] QList<QUrl> attachURLs() const;

    [[nodiscard]] QString to() const;

    [[nodiscard]] QString cc() const;

    [[nodiscard]] QString bcc() const;

    [[nodiscard]] QString subject() const;

    [[nodiscard]] QString body() const;

    [[nodiscard]] QString inReplyTo() const;

    [[nodiscard]] QString replyTo() const;

    [[nodiscard]] QString identity() const;

    [[nodiscard]] QUrl messageFile() const;

    [[nodiscard]] bool startInTray() const;

    [[nodiscard]] bool mailto() const;

    [[nodiscard]] bool checkMail() const;

    [[nodiscard]] bool viewOnly() const;

    [[nodiscard]] bool calledWithSession() const;

    [[nodiscard]] bool operator==(const CommandLineInfo &other) const;

    void setCustomHeaders(const QStringList &newCustomHeaders);

    void setAttachURLs(const QList<QUrl> &newAttachURLs);

    void setTo(const QString &newTo);

    void setCc(const QString &newCc);

    void setBcc(const QString &newBcc);

    void setSubject(const QString &newSubject);

    void setBody(const QString &newBody);

    void setInReplyTo(const QString &newInReplyTo);

    void setReplyTo(const QString &newReplyTo);

    void setIdentity(const QString &newIdentity);

    void setMessageFile(const QUrl &newMessageFile);

    void setStartInTray(bool newStartInTray);

    void setMailto(bool newMailto);

    void setCheckMail(bool newCheckMail);

    void setViewOnly(bool newViewOnly);

    void setCalledWithSession(bool newCalledWithSession);

private:
    QStringList mCustomHeaders;
    QList<QUrl> mAttachURLs;
    QString mTo;
    QString mCc;
    QString mBcc;
    QString mSubject;
    QString mBody;
    QString mInReplyTo;
    QString mReplyTo;
    QString mIdentity;
    QUrl mMessageFile;
    bool mStartInTray = false;
    bool mMailto = false;
    bool mCheckMail = false;
    bool mViewOnly = false;
    bool mCalledWithSession = false; // for ignoring '-session foo'
};

Q_DECLARE_TYPEINFO(CommandLineInfo, Q_MOVABLE_TYPE);
KMAILTESTS_TESTS_EXPORT QDebug operator<<(QDebug d, const CommandLineInfo &t);
