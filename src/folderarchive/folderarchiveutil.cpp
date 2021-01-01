/*
   SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "folderarchiveutil.h"

#include <KConfig>
#include <KConfigGroup>

using namespace FolderArchive;

QString FolderArchiveUtil::groupConfigPattern()
{
    return QStringLiteral("FolderArchiveAccount ");
}

QString FolderArchiveUtil::configFileName()
{
    return QStringLiteral("foldermailarchiverc");
}

bool FolderArchiveUtil::resourceSupportArchiving(const QString &resource)
{
    KConfig config(FolderArchiveUtil::configFileName());
    if (config.hasGroup(groupConfigPattern() + resource)) {
        KConfigGroup grp = config.group(groupConfigPattern() + resource);
        if (grp.readEntry("enabled", false) && (grp.readEntry("topLevelCollectionId", -1) > 0)) {
            return true;
        }
    }
    return false;
}
