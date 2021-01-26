/*
   SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "opencomposerjob.h"
#include "composer.h"
#include "kmail_debug.h"
#include "kmkernel.h"
#include <KIdentityManagement/Identity>
#include <KIdentityManagement/IdentityManager>
#include <KLocalizedString>
#include <KMessageBox>
#include <MessageComposer/MessageHelper>
#include <MessageCore/StringUtil>
#include <QFile>
#include <QMimeDatabase>
#include <TemplateParser/TemplateParserJob>

OpenComposerJob::OpenComposerJob(QObject *parent)
    : QObject(parent)
{
}

void OpenComposerJob::setOpenComposerSettings(const OpenComposerSettings &openComposerSettings)
{
    mOpenComposerSettings = openComposerSettings;
}

OpenComposerJob::~OpenComposerJob() = default;

void OpenComposerJob::start()
{
    mMsg = KMime::Message::Ptr(new KMime::Message);
    if (!mOpenComposerSettings.mIdentity.isEmpty()) {
        if (KMKernel::self()->identityManager()->identities().contains(mOpenComposerSettings.mIdentity)) {
            const KIdentityManagement::Identity id = KMKernel::self()->identityManager()->modifyIdentityForName(mOpenComposerSettings.mIdentity);
            mIdentityId = id.uoid();
        } else {
            qCWarning(KMAIL_LOG) << "Identity name doesn't exist " << mOpenComposerSettings.mIdentity;
        }
    }

    MessageHelper::initHeader(mMsg, KIdentityManagement::IdentityManager::self(), mIdentityId);

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
    if (!mOpenComposerSettings.mReplyTo.isEmpty()) {
        mMsg->replyTo()->fromUnicodeString(mOpenComposerSettings.mReplyTo, "utf-8");
    }
    if (!mOpenComposerSettings.mInReplyTo.isEmpty()) {
        mMsg->inReplyTo()->fromUnicodeString(mOpenComposerSettings.mInReplyTo, "utf-8");
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
            mMsg->setBody(str);
            slotOpenComposer();
        } else {
            auto parser = new TemplateParser::TemplateParserJob(mMsg, TemplateParser::TemplateParserJob::NewMessage);
            connect(parser, &TemplateParser::TemplateParserJob::parsingDone, this, &OpenComposerJob::slotOpenComposer);
            parser->setIdentityManager(KMKernel::self()->identityManager());
            parser->process(mMsg);
        }
    } else if (!mOpenComposerSettings.mBody.isEmpty()) {
        mContext = KMail::Composer::NoTemplate;
        mMsg->setBody(mOpenComposerSettings.mBody.toUtf8());
        mMsg->assemble();
        mMsg->parse();
        slotOpenComposer();
    } else {
        auto parser = new TemplateParser::TemplateParserJob(mMsg, TemplateParser::TemplateParserJob::NewMessage);
        connect(parser, &TemplateParser::TemplateParserJob::parsingDone, this, &OpenComposerJob::slotOpenComposer);
        parser->setIdentityManager(KMKernel::self()->identityManager());
        parser->process(mMsg);
    }
}

void OpenComposerJob::slotOpenComposer()
{
    KMail::Composer *cWin = KMail::makeComposer(mMsg, false, false, mContext, mIdentityId);
    if (!mOpenComposerSettings.mTo.isEmpty()) {
        cWin->setFocusToSubject();
    }
    const QList<QUrl> attachURLs = QUrl::fromStringList(mOpenComposerSettings.mAttachmentPaths);
    QList<QUrl>::ConstIterator endAttachment(attachURLs.constEnd());
    QVector<KMail::Composer::AttachmentInfo> infoList;
    for (QList<QUrl>::ConstIterator it = attachURLs.constBegin(); it != endAttachment; ++it) {
        QMimeDatabase mimeDb;
        if (mimeDb.mimeTypeForUrl(*it).name() == QLatin1String("inode/directory")) {
            if (KMessageBox::questionYesNo(nullptr, i18n("Do you want to attach this folder \"%1\"?", (*it).toDisplayString()), i18n("Attach Folder"))
                == KMessageBox::No) {
                continue;
            }
        }
        KMail::Composer::AttachmentInfo info;
        info.url = (*it);
        infoList.append(info);
    }
    if (!infoList.isEmpty()) {
        cWin->addAttachment(infoList, true);
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
        cWin->showAndActivateComposer();
    }
    deleteLater();
}
