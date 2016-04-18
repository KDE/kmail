/*
  Copyright (c) 2012-2016 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef ARCHIVEJOB_H
#define ARCHIVEJOB_H

#include <MailCommon/JobScheduler>
#include <Collection>
#include <QPixmap>
class ArchiveMailInfo;
class ArchiveMailManager;

class ArchiveJob : public MailCommon::ScheduledJob
{
    Q_OBJECT
public:
    explicit ArchiveJob(ArchiveMailManager *manager, ArchiveMailInfo *info, const Akonadi::Collection &folder, bool immediate);
    ~ArchiveJob();

    void execute() Q_DECL_OVERRIDE;
    void kill() Q_DECL_OVERRIDE;

private Q_SLOTS:
    void slotBackupDone(const QString &info);
    void slotError(const QString &error);
private:
    QPixmap mPixmap;
    ArchiveMailInfo *mInfo;
    ArchiveMailManager *mManager;
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

    ~ScheduledArchiveTask()
    {
    }

    MailCommon::ScheduledJob *run() Q_DECL_OVERRIDE;

    int taskTypeId() const Q_DECL_OVERRIDE
    {
        return 2;
    }
private:
    ArchiveMailInfo *mInfo;
    ArchiveMailManager *mManager;
};

#endif // ARCHIVEJOB_H
