/*
   SPDX-FileCopyrightText: 2012-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "archivejob.h"
#include "archivemailagent_debug.h"
#include "archivemailinfo.h"
#include "archivemailkernel.h"
#include "archivemailmanager.h"

#include <MailCommon/BackupJob>
#include <MailCommon/MailUtil>

#include <AkonadiCore/EntityMimeTypeFilterModel>

#include <KLocalizedString>
#include <KNotification>

ArchiveJob::ArchiveJob(ArchiveMailManager *manager, ArchiveMailInfo *info, const Akonadi::Collection &folder, bool immediate)
    : MailCommon::ScheduledJob(folder, immediate)
    , mInfo(info)
    , mManager(manager)
{
    mDefaultIconName = QStringLiteral("kmail");
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
                                 QString(),
                                 i18n("Directory does not exist. Please verify settings. Archive postponed."),
                                 mDefaultIconName,
                                 nullptr,
                                 KNotification::CloseOnTimeout,
                                 QStringLiteral("akonadi_archivemail_agent"));
            deleteLater();
            return;
        }

        auto backupJob = new MailCommon::BackupJob();
        backupJob->setRootFolder(Akonadi::EntityTreeModel::updatedCollection(mManager->kernel()->collectionModel(), collection));

        backupJob->setSaveLocation(archivePath);
        backupJob->setArchiveType(mInfo->archiveType());
        backupJob->setDeleteFoldersAfterCompletion(false);
        backupJob->setRecursive(mInfo->saveSubCollection());
        backupJob->setDisplayMessageBox(false);
        backupJob->setRealPath(realPath);
        const QString summary = i18n("Start to archive %1", realPath);
        KNotification::event(QStringLiteral("archivemailstarted"),
                             QString(),
                             summary,
                             mDefaultIconName,
                             nullptr,
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
                         QString(),
                         error,
                         mDefaultIconName,
                         nullptr,
                         KNotification::CloseOnTimeout,
                         QStringLiteral("akonadi_archivemail_agent"));
    mManager->backupDone(mInfo);
    deleteLater();
}

void ArchiveJob::slotBackupDone(const QString &info)
{
    KNotification::event(QStringLiteral("archivemailfinished"),
                         QString(),
                         info,
                         mDefaultIconName,
                         nullptr,
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
    return folder().isValid() ? new ArchiveJob(mManager, mInfo, folder(), isImmediate()) : nullptr;
}
