/*
   SPDX-FileCopyrightText: 2021-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once
#include <QString>
/** Send later utilities. */
namespace MailMergeUtil
{
[[nodiscard]] QString mailMergePattern();
[[nodiscard]] bool mailMergeAgentEnabled();
void forceReparseConfiguration();
}
