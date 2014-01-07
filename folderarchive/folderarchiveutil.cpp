/*
  Copyright (c) 2013, 2014 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "folderarchiveutil.h"
//#include "folderarchiveagentsettings.h"

#include <KConfig>
#include <KConfigGroup>

using namespace FolderArchive;

void FolderArchiveUtil::forceReparseConfiguration()
{
    //FolderArchiveAgentSettings::self()->writeConfig();
    //FolderArchiveAgentSettings::self()->config()->reparseConfiguration();
}

QString FolderArchiveUtil::groupConfigPattern()
{
    return QLatin1String("FolderArchiveAccount ");
}

bool FolderArchiveUtil::resourceSupportArchiving(const QString &resource)
{
    KConfig config(QLatin1String("akonadi_folderarchive_agentrc"));
    if (config.hasGroup(groupConfigPattern() + resource)) {
        KConfigGroup grp = config.group(groupConfigPattern() + resource);
        if (grp.readEntry("enabled", false) && (grp.readEntry("topLevelCollectionId", -1) > 0) ) {
            return true;
        }
    }
    return false;
}
