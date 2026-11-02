/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "kmail_private_export.h"
#include <Akonadi/Collection>
#include <QString>

namespace KMail
{
/** KMail side helpers for the snooze agent. */
namespace SnoozeUtil
{
[[nodiscard]] KMAILTESTS_TESTS_EXPORT QString agentIdentifier();

/** The agent instance exists and is enabled in its configuration. */
[[nodiscard]] KMAILTESTS_TESTS_EXPORT bool snoozeAgentAvailable();

/** Messages of @p collection can be snoozed. */
[[nodiscard]] KMAILTESTS_TESTS_EXPORT bool canSnoozeInCollection(const Akonadi::Collection &collection);
}
}
