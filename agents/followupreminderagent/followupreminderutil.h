/*
   SPDX-FileCopyrightText: 2014-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KSharedConfig>

namespace FollowUpReminder
{
class FollowUpReminderInfo;

/** Follow up reminder utilities. */
namespace FollowUpReminderUtil
{
[[nodiscard]] bool followupReminderAgentWasRegistered();

[[nodiscard]] bool followupReminderAgentEnabled();

void reload();

void forceReparseConfiguration();

[[nodiscard]] KSharedConfig::Ptr defaultConfig();

void writeFollowupReminderInfo(const KSharedConfig::Ptr &config, FollowUpReminder::FollowUpReminderInfo *info, bool forceReload);

[[nodiscard]] bool removeFollowupReminderInfo(const KSharedConfig::Ptr &config, const QList<qint32> &listRemove, bool forceReload = false);

[[nodiscard]] QString followUpReminderPattern();
}
}
