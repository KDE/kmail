/*
   Copyright (C) 2014-2020 Laurent Montel <montel@kde.org>

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

#ifndef FOLLOWUPREMINDERUTIL_H
#define FOLLOWUPREMINDERUTIL_H

#include <KSharedConfig>

namespace FollowUpReminder {
class FollowUpReminderInfo;

/** Follow up reminder utilities. */
namespace FollowUpReminderUtil {
Q_REQUIRED_RESULT bool followupReminderAgentWasRegistered();

Q_REQUIRED_RESULT bool followupReminderAgentEnabled();

void reload();

void forceReparseConfiguration();

KSharedConfig::Ptr defaultConfig();

void writeFollowupReminderInfo(KSharedConfig::Ptr config, FollowUpReminder::FollowUpReminderInfo *info, bool forceReload);

Q_REQUIRED_RESULT bool removeFollowupReminderInfo(KSharedConfig::Ptr config, const QList<qint32> &listRemove, bool forceReload = false);

Q_REQUIRED_RESULT QString followUpReminderPattern();

}
}

#endif // FOLLOWUPREMINDERUTIL_H
