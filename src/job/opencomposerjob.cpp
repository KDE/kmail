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

#include "opencomposerjob.h"
#include "kmail_debug.h"
#include "kmkernel.h"
#include "composer.h"
#include "editor/kmcomposerwin.h"
#include <KMime/Message>
#include <MessageCore/StringUtil>
#include <TemplateParser/TemplateParserJob>
#include <KIdentityManagement/IdentityManager>
#include <KIdentityManagement/Identity>
#include <MessageComposer/MessageHelper>
#include <QMimeDatabase>
#include <QFile>
#include <KMessageBox>
#include <KLocalizedString>

OpenComposerJob::OpenComposerJob(QObject *parent)
    : QObject(parent)
    , mMsg(nullptr)
    , mContext(KMail::Composer::New)
{
}

void OpenComposerJob::setOpenComposerSettings(const OpenComposerSettings &openComposerSettings)
{
    mOpenComposerSettings = openComposerSettings;
}

OpenComposerJob::~OpenComposerJob()
{
}

void OpenComposerJob::start()
{
    mMsg = KMime::Message::Ptr(new KMime::Message);
    MessageHelper::initHeader(mMsg, KIdentityManagement::IdentityManager::self());
    mMsg->contentType()->setCharset("utf-8");
    if (!mOpenComposerSettings.mTo.isEmpty()) {
        mMsg->to()->fromUnicodeString(mOpenComposerSettings.mTo, "utf-8");
    }

    if (!mOpenComposerSettings.mCc.isEmpty()) {
        mMsg->cc()->fromUnicodeString(mOpenComposerSettings.mCc, "utf-8");
    }

    if (!mOpenComposerSettings.mBcc.isEmpty()) {
        mMsg->bcc()->fromUnicodeString(mOpenComposerSettings.mBcc, "utf-8");
    }

    if (!mOpenComposerSettings.mSubject.isEmpty()) {
        mMsg->subject()->fromUnicodeString(mOpenComposerSettings.mSubject, "utf-8");
    }

    if (!mOpenComposerSettings.mMessageFile.isEmpty() && QFile::exists(mOpenComposerSettings.mMessageFile)) {
        QFile f(mOpenComposerSettings.mMessageFile);
        QByteArray str;
        if (!f.open(QIODevice::ReadOnly)) {
            qCWarning(KMAIL_LOG) << "Failed to load message: " << f.errorString();
        } else {
            str = f.readAll();
            f.close();
        }
        if (!str.isEmpty()) {
            mContext = KMail::Composer::NoTemplate;
            mMsg->setBody(QString::fromLocal8Bit(str.data(), str.size()).toUtf8());
            slotOpenComposer();
        } else {
            TemplateParser::TemplateParserJob *parser = new TemplateParser::TemplateParserJob(mMsg, TemplateParser::TemplateParserJob::NewMessage);
            connect(parser, &TemplateParser::TemplateParserJob::parsingDone, this, &OpenComposerJob::slotOpenComposer);
            parser->setIdentityManager(KMKernel::self()->identityManager());
            parser->process(mMsg);
        }
    } else if (!mOpenComposerSettings.mBody.isEmpty()) {
        mContext = KMail::Composer::NoTemplate;
        mMsg->setBody(mOpenComposerSettings.mBody.toUtf8());
        slotOpenComposer();
    } else {
        TemplateParser::TemplateParserJob *parser = new TemplateParser::TemplateParserJob(mMsg, TemplateParser::TemplateParserJob::NewMessage);
        connect(parser, &TemplateParser::TemplateParserJob::parsingDone, this, &OpenComposerJob::slotOpenComposer);
        parser->setIdentityManager(KMKernel::self()->identityManager());
        parser->process(mMsg);
    }
}

void OpenComposerJob::slotOpenComposer()
{
    if (!mOpenComposerSettings.mInReplyTo.isEmpty()) {
        KMime::Headers::InReplyTo *header = new KMime::Headers::InReplyTo;
        header->fromUnicodeString(mOpenComposerSettings.mInReplyTo, "utf-8");
        mMsg->setHeader(header);
    }

    mMsg->assemble();

    uint identityId = 0;
    if (!mOpenComposerSettings.mIdentity.isEmpty()) {
        if (KMKernel::self()->identityManager()->identities().contains(mOpenComposerSettings.mIdentity)) {
            const KIdentityManagement::Identity id = KMKernel::self()->identityManager()->modifyIdentityForName(mOpenComposerSettings.mIdentity);
            identityId = id.uoid();
        }
    }
    KMail::Composer *cWin = KMail::makeComposer(mMsg, false, false, mContext, identityId);
    if (!mOpenComposerSettings.mTo.isEmpty()) {
        cWin->setFocusToSubject();
    }
    QList<QUrl> attachURLs = QUrl::fromStringList(mOpenComposerSettings.mAttachmentPaths);
    QList<QUrl>::ConstIterator endAttachment(attachURLs.constEnd());
    for (QList<QUrl>::ConstIterator it = attachURLs.constBegin(); it != endAttachment; ++it) {
        QMimeDatabase mimeDb;
        if (mimeDb.mimeTypeForUrl(*it).name() == QLatin1String("inode/directory")) {
            if (KMessageBox::questionYesNo(nullptr, i18n("Do you want to attach this folder \"%1\"?", (*it).toDisplayString()), i18n("Attach Folder")) == KMessageBox::No) {
                continue;
            }
        }
        cWin->addAttachment((*it), QString());
    }
    if (!mOpenComposerSettings.mReplyTo.isEmpty()) {
        cWin->setCurrentReplyTo(mOpenComposerSettings.mReplyTo);
    }

    if (!mOpenComposerSettings.mCustomHeaders.isEmpty()) {
        QMap<QByteArray, QString> extraCustomHeaders;
        QStringList::ConstIterator end = mOpenComposerSettings.mCustomHeaders.constEnd();
        for (QStringList::ConstIterator it = mOpenComposerSettings.mCustomHeaders.constBegin(); it != end; ++it) {
            if (!(*it).isEmpty()) {
                const int pos = (*it).indexOf(QLatin1Char(':'));
                if (pos > 0) {
                    const QString header = (*it).left(pos).trimmed();
                    const QString value = (*it).mid(pos + 1).trimmed();
                    if (!header.isEmpty() && !value.isEmpty()) {
                        extraCustomHeaders.insert(header.toUtf8(), value);
                    }
                }
            }
        }
        if (!extraCustomHeaders.isEmpty()) {
            cWin->addExtraCustomHeaders(extraCustomHeaders);
        }
    }
    if (!mOpenComposerSettings.mHidden) {
        cWin->show();
    }
    deleteLater();
}
