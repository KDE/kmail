/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

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

#include "migrateinfo.h"

MigrateInfo::MigrateInfo()
    : mFolder(false)
{

}

bool MigrateInfo::isValid() const
{
    return !mType.isEmpty() && !mPath.isEmpty();
}

QString MigrateInfo::type() const
{
    return mType;
}

void MigrateInfo::setType(const QString &type)
{
    mType = type;
}

QString MigrateInfo::path() const
{
    return mPath;
}

void MigrateInfo::setPath(const QString &path)
{
    mPath = path;
}

bool MigrateInfo::folder() const
{
    return mFolder;
}

void MigrateInfo::setFolder(bool folder)
{
    mFolder = folder;
}

