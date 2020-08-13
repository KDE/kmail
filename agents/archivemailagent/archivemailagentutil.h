/*
   SPDX-FileCopyrightText: 2012-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef ARCHIVEMAILAGENTUTIL_H
#define ARCHIVEMAILAGENTUTIL_H

#include "archivemailinfo.h"
#include <QDate>

namespace ArchiveMailAgentUtil {
static QString archivePattern = QStringLiteral("ArchiveMailCollection %1");
Q_REQUIRED_RESULT QDate diffDate(ArchiveMailInfo *info);
Q_REQUIRED_RESULT bool needToArchive(ArchiveMailInfo *info);
}

#endif // ARCHIVEMAILAGENTUTIL_H
