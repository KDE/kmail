/*
   SPDX-FileCopyrightText: 2012-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef ARCHIVEJOB_H
#define ARCHIVEJOB_H

#include <MailCommon/JobScheduler>
#include <Collection>
class ArchiveMailInfo;
class ArchiveMailManager;

class ArchiveJob : public MailCommon::ScheduledJob
{
    Q_OBJECT
public:
    explicit ArchiveJob(ArchiveMailManager *manager, ArchiveMailInfo *info, const Akonadi::Collection &folder, bool immediate);
    ~ArchiveJob() override;

    void execute() override;
    void kill() override;

private:
    void slotBackupDone(const QString &info);
    void slotError(const QString &error);
    QString mDefaultIconName;
    ArchiveMailInfo *const mInfo;
    ArchiveMailManager *const mManager;
};

/// A scheduled "expire mails in this folder" task.
class ScheduledArchiveTask : public MailCommon::ScheduledTask
{
public:
    /// If immediate is set, the job will execute synchronously. This is used when
    /// the user requests explicitly that the operation should happen immediately.
    ScheduledArchiveTask(ArchiveMailManager *manager, ArchiveMailInfo *info, const Akonadi::Collection &folder, bool immediate)
        : MailCommon::ScheduledTask(folder, immediate)
        , mInfo(info)
        , mManager(manager)
    {
    }

    ~ScheduledArchiveTask() override
    = default;

    MailCommon::ScheduledJob *run() override;

    int taskTypeId() const override
    {
        return 2;
    }

private:
    ArchiveMailInfo *mInfo = nullptr;
    ArchiveMailManager *mManager = nullptr;
};

#endif // ARCHIVEJOB_H
