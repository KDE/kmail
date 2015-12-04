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

#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H

#include <QObject>
#include <QVector>

#include <genericplugins/genericplugininterface.h>

class PluginInterface : public QObject
{
    Q_OBJECT
public:
    explicit PluginInterface(QObject *parent = Q_NULLPTR);
    ~PluginInterface();

    void setParentWidget(QWidget *widget);
    QVector<PimCommon::ActionType> actionsType() const;
private:
    QWidget *mParentWidget;
};

#endif // PLUGININTERFACE_H
