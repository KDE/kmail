/*
   SPDX-FileCopyrightText: 2018-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "dndfromarkjob.h"
#include "editor/kmcomposerwin.h"
#include "kmail_debug.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDir>
#include <QMimeData>
#include <QTemporaryDir>
#include <QUrl>

#include <editor/composer.h>

DndFromArkJob::DndFromArkJob(QObject *parent)
    : QObject(parent)
{
}

bool DndFromArkJob::dndFromArk(const QMimeData *source)
{
    if (source->hasFormat(QStringLiteral("application/x-kde-ark-dndextract-service"))
        && source->hasFormat(QStringLiteral("application/x-kde-ark-dndextract-path"))) {
        return true;
    }
    return false;
}

bool DndFromArkJob::extract(const QMimeData *source)
{
    if (dndFromArk(source)) {
        if (!mComposerWin) {
            qCWarning(KMAIL_LOG) << "mComposerWin is null, it's a bug";
            deleteLater();
            return false;
        }
        const QString remoteDBusClient = QString::fromLatin1(source->data(QStringLiteral("application/x-kde-ark-dndextract-service")));
        const QString remoteDBusPath = QString::fromLatin1(source->data(QStringLiteral("application/x-kde-ark-dndextract-path")));

        const QString tmpPath = QDir::tempPath() + QLatin1String("/attachments_ark");
        QDir().mkpath(tmpPath);

        auto linkDir = new QTemporaryDir(tmpPath);
        const QString arkPath = linkDir->path();
        QDBusMessage message = QDBusMessage::createMethodCall(remoteDBusClient,
                                                              remoteDBusPath,
                                                              QStringLiteral("org.kde.ark.DndExtract"),
                                                              QStringLiteral("extractSelectedFilesTo"));
        message.setArguments({arkPath});
        QDBusConnection::sessionBus().call(message);
        QDir dir(arkPath);
        const QStringList list = dir.entryList(QDir::NoDotAndDotDot | QDir::Files);
        QVector<KMail::Composer::AttachmentInfo> infoList;
        infoList.reserve(list.size());
        for (int i = 0; i < list.size(); ++i) {
            KMail::Composer::AttachmentInfo info;
            info.url = QUrl::fromLocalFile(list.at(i));
            infoList.append(info);
        }
        mComposerWin->addAttachment(infoList, false);
        delete linkDir;
        deleteLater();
        return true;
    }
    deleteLater();
    return false;
}

void DndFromArkJob::setComposerWin(KMComposerWin *composerWin)
{
    mComposerWin = composerWin;
}
