/*
   SPDX-FileCopyrightText: 2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once
#include <QString>
/** Send later utilities. */
namespace MailMergeUtil
{
Q_REQUIRED_RESULT QString mailMergePattern();
Q_REQUIRED_RESULT bool mailMergeAgentEnabled();
void forceReparseConfiguration();
};
