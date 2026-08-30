/*
   SPDX-FileCopyrightText: 2013-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "folderarchiveutil.h"

#include <KConfig>
#include <KConfigGroup>

using namespace FolderArchive;
using namespace Qt::Literals::StringLiterals;

QString FolderArchiveUtil::groupConfigPattern()
{
    return u"FolderArchiveAccount "_s;
}

QString FolderArchiveUtil::configFileName()
{
    return u"foldermailarchiverc"_s;
}

bool FolderArchiveUtil::resourceSupportArchiving(const QString &resource)
{
    if (KConfig config(FolderArchiveUtil::configFileName()); config.hasGroup(groupConfigPattern() + resource)) {
        if (KConfigGroup grp = config.group(groupConfigPattern() + resource);
            grp.readEntry("enabled", false) && (grp.readEntry("topLevelCollectionId", -1) > 0)) {
            return true;
        }
    }
    return false;
}
