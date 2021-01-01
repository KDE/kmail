/*
   SPDX-FileCopyrightText: 2012-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "archivemailagentutil.h"
#include "archivemailagent_debug.h"

QDate ArchiveMailAgentUtil::diffDate(ArchiveMailInfo *info)
{
    QDate diffDate(info->lastDateSaved());
    switch (info->archiveUnit()) {
    case ArchiveMailInfo::ArchiveDays:
        diffDate = diffDate.addDays(info->archiveAge());
        break;
    case ArchiveMailInfo::ArchiveWeeks:
        diffDate = diffDate.addDays(info->archiveAge() * 7);
        break;
    case ArchiveMailInfo::ArchiveMonths:
        diffDate = diffDate.addMonths(info->archiveAge());
        break;
    case ArchiveMailInfo::ArchiveYears:
        diffDate = diffDate.addYears(info->archiveAge());
        break;
    }
    return diffDate;
}

bool ArchiveMailAgentUtil::needToArchive(ArchiveMailInfo *info)
{
    if (!info->isEnabled()) {
        return false;
    }
    if (info->url().isEmpty()) {
        return false;
    }
    if (!info->lastDateSaved().isValid()) {
        return true;
    } else {
        if (QDate::currentDate() >= diffDate(info)) {
            return true;
        }
    }
    return false;
}
