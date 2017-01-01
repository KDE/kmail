/*
   Copyright (C) 2012-2017 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "archivejob.h"
#include "archivemailinfo.h"
#include "archivemailmanager.h"
#include "archivemailagent_debug.h"
#include "MailCommon/MailUtil"

#include <MailCommon/BackupJob>

#include <KNotification>
#include <KLocalizedString>
#include <QIcon>
#include <KIconLoader>

ArchiveJob::ArchiveJob(ArchiveMailManager *manager, ArchiveMailInfo *info, const Akonadi::Collection &folder, bool immediate)
    : MailCommon::ScheduledJob(folder, immediate)
    , mInfo(info)
    , mManager(manager)
{
    mPixmap = QIcon::fromTheme(QStringLiteral("kmail")).pixmap(KIconLoader::SizeSmall, KIconLoader::SizeSmall);
}

ArchiveJob::~ArchiveJob()
{
    delete mInfo;
}

void ArchiveJob::execute()
{
    if (mInfo) {

        Akonadi::Collection collection(mInfo->saveCollectionId());
        const QString realPath = MailCommon::Util::fullCollectionPath(collection);
        if (realPath.isEmpty()) {
            qCDebug(ARCHIVEMAILAGENT_LOG) << " We cannot find real path, collection doesn't exist";
            mManager->collectionDoesntExist(mInfo);
            deleteLater();
            return;
        }
        if (mInfo->url().isEmpty()) {
            qCDebug(ARCHIVEMAILAGENT_LOG) << " Path is empty";
            mManager->collectionDoesntExist(mInfo);
            deleteLater();
            return;
        }

        bool dirExit = true;
        const QUrl archivePath = mInfo->realUrl(realPath, dirExit);
        if (!dirExit) {
            mManager->backupDone(mInfo);
            KNotification::event(QStringLiteral("archivemailfolderdoesntexist"),
                                 i18n("Directory does not exist. Please verify settings. Archive postponed."),
                                 mPixmap,
                                 Q_NULLPTR,
                                 KNotification::CloseOnTimeout,
                                 QStringLiteral("akonadi_archivemail_agent"));
            deleteLater();
            return;
        }

        MailCommon::BackupJob *backupJob = new MailCommon::BackupJob();
        backupJob->setRootFolder(MailCommon::Util::updatedCollection(collection));

        backupJob->setSaveLocation(archivePath);
        backupJob->setArchiveType(mInfo->archiveType());
        backupJob->setDeleteFoldersAfterCompletion(false);
        backupJob->setRecursive(mInfo->saveSubCollection());
        backupJob->setDisplayMessageBox(false);
        backupJob->setRealPath(realPath);
        const QString summary = i18n("Start to archive %1", realPath);
        KNotification::event(QStringLiteral("archivemailstarted"),
                             summary,
                             mPixmap,
                             Q_NULLPTR,
                             KNotification::CloseOnTimeout,
                             QStringLiteral("akonadi_archivemail_agent"));
        connect(backupJob, &MailCommon::BackupJob::backupDone, this, &ArchiveJob::slotBackupDone);
        connect(backupJob, &MailCommon::BackupJob::error, this, &ArchiveJob::slotError);
        backupJob->start();
    }
}

void ArchiveJob::slotError(const QString &error)
{
    KNotification::event(QStringLiteral("archivemailerror"),
                         error,
                         mPixmap,
                         Q_NULLPTR,
                         KNotification::CloseOnTimeout,
                         QStringLiteral("akonadi_archivemail_agent"));
    mManager->backupDone(mInfo);
    deleteLater();
}

void ArchiveJob::slotBackupDone(const QString &info)
{
    KNotification::event(QStringLiteral("archivemailfinished"),
                         info,
                         mPixmap,
                         Q_NULLPTR,
                         KNotification::CloseOnTimeout,
                         QStringLiteral("akonadi_archivemail_agent"));
    mManager->backupDone(mInfo);
    deleteLater();
}

void ArchiveJob::kill()
{
    ScheduledJob::kill();
}

MailCommon::ScheduledJob *ScheduledArchiveTask::run()
{
    return folder().isValid() ? new ArchiveJob(mManager, mInfo, folder(), isImmediate()) : Q_NULLPTR;
}

