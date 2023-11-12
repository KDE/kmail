/*
   SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "commandlineinfo.h"

CommandLineInfo::CommandLineInfo() = default;

CommandLineInfo::~CommandLineInfo() = default;

QDebug operator<<(QDebug d, const CommandLineInfo &t)
{
    // TODO
    return d;
}

void CommandLineInfo::parseCommandLine(const QStringList &args, const QString &workingDir)
{
    // TODO
}

QStringList CommandLineInfo::customHeaders() const
{
    return mCustomHeaders;
}

QList<QUrl> CommandLineInfo::attachURLs() const
{
    return mAttachURLs;
}

QString CommandLineInfo::to() const
{
    return mTo;
}

QString CommandLineInfo::cc() const
{
    return mCc;
}

QString CommandLineInfo::bcc() const
{
    return mBcc;
}

QString CommandLineInfo::subject() const
{
    return mSubject;
}

QString CommandLineInfo::body() const
{
    return mBody;
}

QString CommandLineInfo::inReplyTo() const
{
    return mInReplyTo;
}

QString CommandLineInfo::replyTo() const
{
    return mReplyTo;
}

QString CommandLineInfo::identity() const
{
    return mIdentity;
}

QUrl CommandLineInfo::messageFile() const
{
    return mMessageFile;
}

bool CommandLineInfo::startInTray() const
{
    return mStartInTray;
}

bool CommandLineInfo::mailto() const
{
    return mMailto;
}

bool CommandLineInfo::checkMail() const
{
    return mCheckMail;
}

bool CommandLineInfo::viewOnly() const
{
    return mViewOnly;
}

bool CommandLineInfo::calledWithSession() const
{
    return mCalledWithSession;
}
