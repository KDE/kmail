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


#include "configureagentitem.h"

ConfigureAgentItem::ConfigureAgentItem()
    : mChecked(false),
      mFailed(false)
{

}

ConfigureAgentItem::~ConfigureAgentItem()
{

}

QString ConfigureAgentItem::agentName() const
{
    return mAgentName;
}

void ConfigureAgentItem::setAgentName(const QString &agentName)
{
    mAgentName = agentName;
}

QString ConfigureAgentItem::description() const
{
    return mDescription;
}

void ConfigureAgentItem::setDescription(const QString &description)
{
    mDescription = description;
}

QString ConfigureAgentItem::path() const
{
    return mPath;
}

void ConfigureAgentItem::setPath(const QString &path)
{
    mPath = path;
}

QString ConfigureAgentItem::interfaceName() const
{
    return mInterfaceName;
}

void ConfigureAgentItem::setInterfaceName(const QString &interfaceName)
{
    mInterfaceName = interfaceName;
}

bool ConfigureAgentItem::checked() const
{
    return mChecked;
}

void ConfigureAgentItem::setChecked(bool checked)
{
    mChecked = checked;
}

bool ConfigureAgentItem::failed() const
{
    return mFailed;
}

void ConfigureAgentItem::setFailed(bool failed)
{
    mFailed = failed;
}

bool ConfigureAgentItem::operator ==(const ConfigureAgentItem &other) const
{
    return (mAgentName == other.agentName()) &&
            (mDescription == other.description()) &&
            (mPath == other.path()) &&
            (mInterfaceName == other.interfaceName()) &&
            (mChecked == other.checked()) &&
            (mFailed == other.failed());
}







