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

#include "plugininterface.h"
#include "pimcommon/genericpluginmanager.h"
#include "kmail_debug.h"

#include <KActionCollection>

#include <pimcommon/genericplugin.h>

PluginInterface::PluginInterface(KActionCollection *ac, QObject *parent)
    : QObject(parent),
      mParentWidget(Q_NULLPTR),
      mActionCollection(ac)
{
    PimCommon::GenericPluginManager::self()->setPluginName(QStringLiteral("kmail"));
    PimCommon::GenericPluginManager::self()->setServiceTypeName(QStringLiteral("KMail/MainViewPlugin"));
    if (!PimCommon::GenericPluginManager::self()->initializePlugins()) {
        qCDebug(KMAIL_LOG) << " Impossible to initialize plugins";
    }
}

PluginInterface::~PluginInterface()
{

}

void PluginInterface::createPluginInterface()
{
    Q_FOREACH(PimCommon::GenericPlugin *plugin, PimCommon::GenericPluginManager::self()->pluginsList()) {
        mListGenericInterface.append(plugin->createInterface(mActionCollection, mParentWidget));
    }
}

void PluginInterface::setParentWidget(QWidget *widget)
{
    mParentWidget = widget;
}

QVector<PimCommon::ActionType> PluginInterface::actionsType() const
{
    QVector<PimCommon::ActionType> typeList;
    Q_FOREACH(PimCommon::GenericPluginInterface *interface, mListGenericInterface) {
        typeList.append(interface->actionType());
    }

    return typeList;
}
