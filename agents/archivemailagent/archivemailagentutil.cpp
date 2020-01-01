/*
   Copyright (C) 2012-2020 Laurent Montel <montel@kde.org>

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
