/*
   SPDX-FileCopyrightText: 2012-2024 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "archivemailinfo.h"
#include <QDate>

namespace ArchiveMailAgentUtil
{
static QString archivePattern = QStringLiteral("ArchiveMailCollection %1");
[[nodiscard]] QDate diffDate(ArchiveMailInfo *info);
[[nodiscard]] bool needToArchive(ArchiveMailInfo *info);
[[nodiscard]] bool timeIsInRange(const QList<int> &range, const QTime &time);
}
