/*
   Copyright (C) 2018 Laurent Montel <montel@kde.org>

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

#include "dndfromarkjob.h"
#include "editor/kmcomposerwin.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDir>
#include <QMimeData>
#include <QTemporaryDir>
#include <QUrl>

DndFromArkJob::DndFromArkJob(QObject *parent)
    : QObject(parent)
{

}

bool DndFromArkJob::dndFromArk(const QMimeData *source)
{
    if (source->hasFormat(QStringLiteral("application/x-kde-ark-dndextract-service")) &&
            source->hasFormat(QStringLiteral("application/x-kde-ark-dndextract-path"))) {
        return true;
    }
    return false;
}

bool DndFromArkJob::extract(const QMimeData *source)
{
    if (dndFromArk(source)) {
        const QString remoteDBusClient = QString::fromLatin1(source->data(QStringLiteral("application/x-kde-ark-dndextract-service")));
        const QString remoteDBusPath = QString::fromLatin1(source->data(QStringLiteral("application/x-kde-ark-dndextract-path")));

        const QString tmpPath = QDir::tempPath() + QLatin1String("/attachments_ark");
        QDir().mkpath(tmpPath);

        QTemporaryDir *linkDir = new QTemporaryDir(tmpPath);
        const QString arkPath = linkDir->path();
        QDBusMessage message = QDBusMessage::createMethodCall(remoteDBusClient, remoteDBusPath,
                                                              QStringLiteral("org.kde.ark.DndExtract"), QStringLiteral("extractSelectedFilesTo"));
        message.setArguments({arkPath});
        QDBusConnection::sessionBus().call(message);
        QDir dir(arkPath);
        QStringList list = dir.entryList(QDir::NoDotAndDotDot | QDir::Files);
        for (int i = 0; i < list.size(); ++i) {
            mComposerWin->addAttachment(QUrl::fromLocalFile(list.at(i)), QString());
        }
        delete linkDir;
        return true;
    }
    return false;
}

void DndFromArkJob::setComposerWin(KMComposerWin *composerWin)
{
    mComposerWin = composerWin;
}
