/*
   SPDX-FileCopyrightText: 2023-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "commandlineinfo.h"
#include "kmail_debug.h"
#include "kmail_options.h"
#include "messagecore/stringutil.h"
#include <QCommandLineParser>
#include <QDebug>
using namespace Qt::Literals::StringLiterals;
CommandLineInfo::CommandLineInfo() = default;

CommandLineInfo::~CommandLineInfo() = default;

QDebug operator<<(QDebug d, const CommandLineInfo &t)
{
    d << "mCustomHeaders " << t.customHeaders();
    d << "mAttachURLs " << t.attachURLs();
    d << "mTo " << t.to();
    d << "mCc " << t.cc();
    d << "mBcc " << t.bcc();
    d << "mSubject " << t.subject();
    d << "mBody " << t.body();
    d << "mInReplyTo " << t.inReplyTo();
    d << "mReplyTo " << t.replyTo();
    d << "mIdentity " << t.identity();
    d << "mMessageFile " << t.messageFile();
    d << "mStartInTray " << t.startInTray();
    d << "mMailto " << t.mailto();
    d << "mCheckMail " << t.checkMail();
    d << "mViewOnly " << t.viewOnly();
    d << "mCalledWithSession " << t.calledWithSession();
    d << "mHtmlBody " << t.htmlBody();
    return d;
}

static QUrl makeAbsoluteUrl(const QString &str, const QString &cwd)
{
    return QUrl::fromUserInput(str, cwd, QUrl::AssumeLocalFile);
}

void CommandLineInfo::parseCommandLine(const QStringList &args, const QString &workingDir)
{
    // process args:
    QCommandLineParser parser;
    kmail_options(&parser);
    QStringList newargs;
    bool addAttachmentAttribute = false;
    for (const QString &argument : std::as_const(args)) {
        if (argument == "--attach"_L1) {
            addAttachmentAttribute = true;
        } else {
            if (argument.startsWith("--"_L1)) {
                addAttachmentAttribute = false;
            }
            if (argument.contains(QLatin1Char('@')) || argument.startsWith("mailto:"_L1)) { // address mustn't be trade as a attachment
                addAttachmentAttribute = false;
            }
            if (addAttachmentAttribute) {
                newargs.append(QStringLiteral("--attach"));
                newargs.append(argument);
            } else {
                newargs.append(argument);
            }
        }
    }

    parser.process(newargs);
    if (parser.isSet(QStringLiteral("subject"))) {
        mSubject = parser.value(QStringLiteral("subject"));
        // if kmail is called with 'kmail -session abc' then this doesn't mean
        // that the user wants to send a message with subject "ession" but
        // (most likely) that the user clicked on KMail's system tray applet
        // which results in KMKernel::raise() calling "kmail kmail newInstance"
        // via D-Bus which apparently executes the application with the original
        // command line arguments and those include "-session ..." if
        // kmail/kontact was restored by session management
        if (mSubject == "ession"_L1) {
            mSubject.clear();
            mCalledWithSession = true;
        } else {
            mMailto = true;
        }
    }

    const QStringList ccList = parser.values(QStringLiteral("cc"));
    if (!ccList.isEmpty()) {
        mMailto = true;
        mCc = ccList.join(QStringLiteral(", "));
    }

    const QStringList bccList = parser.values(QStringLiteral("bcc"));
    if (!bccList.isEmpty()) {
        mMailto = true;
        mBcc = bccList.join(QStringLiteral(", "));
    }

    if (parser.isSet(QStringLiteral("replyTo"))) {
        mMailto = true;
        mReplyTo = parser.value(QStringLiteral("replyTo"));
    }

    if (parser.isSet(QStringLiteral("msg"))) {
        mMailto = true;
        const QString file = parser.value(QStringLiteral("msg"));
        mMessageFile = makeAbsoluteUrl(file, workingDir);
    }

    if (parser.isSet(QStringLiteral("body"))) {
        mMailto = true;
        mBody = parser.value(QStringLiteral("body"));
    }

    if (parser.isSet(QStringLiteral("html"))) {
        mMailto = true;
        mBody = parser.value(QStringLiteral("html"));
        mHtmlBody = true;
    }

    const QStringList attachList = parser.values(QStringLiteral("attach"));
    if (!attachList.isEmpty()) {
        mMailto = true;
        for (const QString &attach : attachList) {
            if (!attach.isEmpty()) {
                mAttachURLs.append(makeAbsoluteUrl(attach, workingDir));
            }
        }
    }

    mCustomHeaders = parser.values(QStringLiteral("header"));

    if (parser.isSet(QStringLiteral("composer"))) {
        mMailto = true;
    }

    if (parser.isSet(QStringLiteral("check"))) {
        mCheckMail = true;
    }

    if (parser.isSet(QStringLiteral("startintray"))) {
        mStartInTray = true;
    }

    if (parser.isSet(QStringLiteral("identity"))) {
        mIdentity = parser.value(QStringLiteral("identity"));
    }

    if (parser.isSet(QStringLiteral("view"))) {
        mViewOnly = true;
        const QString filename = parser.value(QStringLiteral("view"));
        mMessageFile = QUrl::fromUserInput(filename, workingDir);
    }

    if (!mCalledWithSession) {
        // only read additional command line arguments if kmail/kontact is
        // not called with "-session foo"
        const QStringList lstPositionalArguments = parser.positionalArguments();
        for (const QString &arg : lstPositionalArguments) {
            if (arg.startsWith("mailto:"_L1, Qt::CaseInsensitive)) {
                const QUrl urlDecoded(QUrl::fromPercentEncoding(arg.toUtf8()));
                const QList<QPair<QString, QString>> values = MessageCore::StringUtil::parseMailtoUrl(urlDecoded);
                QString previousKey;
                for (int i = 0; i < values.count(); ++i) {
                    const QPair<QString, QString> element = values.at(i);
                    const QString key = element.first.toLower();
                    if (key == "to"_L1) {
                        if (!element.second.isEmpty()) {
                            mTo += element.second + QStringLiteral(", ");
                        }
                        previousKey.clear();
                    } else if (key == "cc"_L1) {
                        if (!element.second.isEmpty()) {
                            mCc += element.second + QStringLiteral(", ");
                        }
                        previousKey.clear();
                    } else if (key == "bcc"_L1) {
                        if (!element.second.isEmpty()) {
                            mBcc += element.second + QStringLiteral(", ");
                        }
                        previousKey.clear();
                    } else if (key == "subject"_L1) {
                        mSubject = element.second;
                        previousKey.clear();
                    } else if (key == "body"_L1) {
                        mBody = element.second;
                        previousKey = key;
                    } else if (key == "in-reply-to"_L1) {
                        mInReplyTo = element.second;
                        previousKey.clear();
                    } else if (key == "attachment"_L1 || key == "attach"_L1) {
                        if (!element.second.isEmpty()) {
                            mAttachURLs << makeAbsoluteUrl(element.second, workingDir);
                        }
                        previousKey.clear();
                    } else {
                        qCWarning(KMAIL_LOG) << "unknown key" << key;
                        // Workaround: https://bugs.kde.org/show_bug.cgi?id=390939
                        // QMap<QString, QString> parseMailtoUrl(const QUrl &url) parses correctly url
                        // But if we have a "&" unknown key we lost it.
                        if (previousKey == "mBody"_L1) {
                            mBody += QLatin1Char('&') + key + QLatin1Char('=') + element.second;
                        }
                        // Don't clear previous key.
                    }
                }
            } else {
                const QUrl url(arg);
                if (url.isValid() && !url.scheme().isEmpty()) {
                    mAttachURLs += url;
                } else {
                    mTo += arg + QStringLiteral(", ");
                }
            }
            mMailto = true;
        }
        if (!mTo.isEmpty()) {
            // cut off the superfluous trailing ", "
            mTo.chop(2);
        }
    }
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

bool CommandLineInfo::operator==(const CommandLineInfo &other) const
{
    return mCustomHeaders == other.mCustomHeaders && mAttachURLs == other.mAttachURLs && mTo == other.mTo && mCc == other.mCc && mBcc == other.mBcc
        && mSubject == other.mSubject && mBody == other.mBody && mInReplyTo == other.mInReplyTo && mReplyTo == other.mReplyTo && mIdentity == other.mIdentity
        && mMessageFile == other.mMessageFile && mStartInTray == other.mStartInTray && mMailto == other.mMailto && mCheckMail == other.mCheckMail
        && mViewOnly == other.mViewOnly && mCalledWithSession == other.mCalledWithSession && mHtmlBody == other.mHtmlBody;
}

void CommandLineInfo::setCustomHeaders(const QStringList &newCustomHeaders)
{
    mCustomHeaders = newCustomHeaders;
}

void CommandLineInfo::setAttachURLs(const QList<QUrl> &newAttachURLs)
{
    mAttachURLs = newAttachURLs;
}

void CommandLineInfo::setTo(const QString &newTo)
{
    mTo = newTo;
}

void CommandLineInfo::setCc(const QString &newCc)
{
    mCc = newCc;
}

void CommandLineInfo::setBcc(const QString &newBcc)
{
    mBcc = newBcc;
}

void CommandLineInfo::setSubject(const QString &newSubject)
{
    mSubject = newSubject;
}

void CommandLineInfo::setBody(const QString &newBody)
{
    mBody = newBody;
}

void CommandLineInfo::setInReplyTo(const QString &newInReplyTo)
{
    mInReplyTo = newInReplyTo;
}

void CommandLineInfo::setReplyTo(const QString &newReplyTo)
{
    mReplyTo = newReplyTo;
}

void CommandLineInfo::setIdentity(const QString &newIdentity)
{
    mIdentity = newIdentity;
}

void CommandLineInfo::setMessageFile(const QUrl &newMessageFile)
{
    mMessageFile = newMessageFile;
}

void CommandLineInfo::setStartInTray(bool newStartInTray)
{
    mStartInTray = newStartInTray;
}

void CommandLineInfo::setMailto(bool newMailto)
{
    mMailto = newMailto;
}

void CommandLineInfo::setCheckMail(bool newCheckMail)
{
    mCheckMail = newCheckMail;
}

void CommandLineInfo::setViewOnly(bool newViewOnly)
{
    mViewOnly = newViewOnly;
}

void CommandLineInfo::setCalledWithSession(bool newCalledWithSession)
{
    mCalledWithSession = newCalledWithSession;
}

bool CommandLineInfo::htmlBody() const
{
    return mHtmlBody;
}

void CommandLineInfo::setHtmlBody(bool newHtmlBody)
{
    mHtmlBody = newHtmlBody;
}
