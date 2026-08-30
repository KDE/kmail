/*
   SPDX-FileCopyrightText: 2012-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "archivemailinfo.h"
#include <QDate>
using namespace Qt::Literals::StringLiterals;

namespace ArchiveMailAgentUtil
{
static QString archivePattern = u"ArchiveMailCollection %1"_s;
[[nodiscard]] QDate diffDate(const ArchiveMailInfo *info);
[[nodiscard]] bool needToArchive(ArchiveMailInfo *info);
[[nodiscard]] bool timeIsInRange(const QList<int> &range, QTime time);
}
