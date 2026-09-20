/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Collection>
#include <KSharedConfig>
#include <QList>
#include <QString>

class KConfigGroup;

namespace Snooze
{
class SnoozeInfo;

/** Snooze agent utilities. */
namespace SnoozeUtil
{
[[nodiscard]] bool snoozeAgentWasRegistered();

[[nodiscard]] bool snoozeAgentEnabled();

void reload();

void forceReparseConfiguration();

[[nodiscard]] KSharedConfig::Ptr defaultConfig();

/** Config group pattern, "SnoozeItem \\d+". */
[[nodiscard]] QString snoozePattern();

[[nodiscard]] SnoozeInfo *readSnoozeInfo(const KConfigGroup &config);

void writeSnoozeInfo(const KSharedConfig::Ptr &config, Snooze::SnoozeInfo *info, bool forceReload);

[[nodiscard]] bool removeSnoozeInfo(const KSharedConfig::Ptr &config, const QList<qint32> &listRemove, bool forceReload = false);

[[nodiscard]] bool compareSnoozeInfo(Snooze::SnoozeInfo *left, Snooze::SnoozeInfo *right);

/**
 * The virtual collection snoozed messages are linked into. Created on demand
 * by the agent; a single one for every account.
 */
[[nodiscard]] Akonadi::Collection::Id snoozeCollectionId();
void setSnoozeCollectionId(Akonadi::Collection::Id id);
}
}
