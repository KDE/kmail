/*
  Copyright (c) 2015-2016 Montel Laurent <montel@kde.org>

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

#ifndef CONFIGUREAGENTITEM_H
#define CONFIGUREAGENTITEM_H

#include <QString>

class ConfigureAgentItem
{
public:
    ConfigureAgentItem();
    ~ConfigureAgentItem();

    QString agentName() const;
    void setAgentName(const QString &agentName);

    QString description() const;
    void setDescription(const QString &description);

    QString path() const;
    void setPath(const QString &path);

    QString interfaceName() const;
    void setInterfaceName(const QString &interfaceName);

    bool checked() const;
    void setChecked(bool checked);

    bool failed() const;
    void setFailed(bool failed);

    bool operator ==(const ConfigureAgentItem &other) const;

private:
    QString mAgentName;
    QString mDescription;
    QString mPath;
    QString mInterfaceName;
    bool mChecked;
    bool mFailed;
};
Q_DECLARE_TYPEINFO(ConfigureAgentItem, Q_MOVABLE_TYPE);

#endif // CONFIGUREAGENTITEM_H
