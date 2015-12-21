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

#include "plugineditorinterface.h"

ActionType::ActionType()
    : mAction(Q_NULLPTR),
      mType(Tools)
{

}

ActionType::ActionType(QAction *action, ActionType::Type type)
    : mAction(action),
      mType(type)
{
}

QAction *ActionType::action() const
{
    return mAction;
}

ActionType::Type ActionType::type() const
{
    return mType;
}

PluginEditorInterface::PluginEditorInterface(QObject *parent)
    : QObject(parent),
      mParentWidget(Q_NULLPTR)
{

}

PluginEditorInterface::~PluginEditorInterface()
{

}

void PluginEditorInterface::setActionType(const ActionType &type)
{
    mActionType = type;
}

ActionType PluginEditorInterface::actionType() const
{
    return mActionType;
}

void PluginEditorInterface::setParentWidget(QWidget *parent)
{
    mParentWidget = parent;
}

QWidget *PluginEditorInterface::parentWidget() const
{
    return mParentWidget;
}
