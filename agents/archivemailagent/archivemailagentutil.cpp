/*
   SPDX-FileCopyrightText: 2012-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "archivemailagentutil.h"

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

bool ArchiveMailAgentUtil::timeIsInRange(const QList<int> &range, const QTime &time)
{
    const int hour = time.hour();
    const int startRange = range.at(0);
    const int endRange = range.at(1);
    if ((hour >= startRange) && (hour <= endRange)) {
        return true;
    } else {
        // Range as 23h -> 5h
        if ((hour >= startRange) && (hour > endRange)) {
            return true;
        } else if ((startRange > endRange) && (hour < startRange && (hour <= endRange))) { // Range as 23h -> 5h
            return true;
        }
        return false;
    }
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
        if (info->useRange()) {
            return ArchiveMailAgentUtil::timeIsInRange(info->range(), QTime::currentTime());
        }
        return true;
    } else {
        if (QDate::currentDate() >= diffDate(info)) {
            if (info->useRange()) {
                return ArchiveMailAgentUtil::timeIsInRange(info->range(), QTime::currentTime());
            }
            return true;
        }
    }
    return false;
}
