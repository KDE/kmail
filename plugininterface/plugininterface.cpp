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
        PimCommon::GenericPluginInterface *interface = plugin->createInterface(mActionCollection, mParentWidget);
        connect(interface, &PimCommon::GenericPluginInterface::emitPluginActivated, this, &PluginInterface::slotPluginActivated);
        mListGenericInterface.append(interface);
    }
}

void PluginInterface::slotPluginActivated(PimCommon::GenericPluginInterface *interface)
{
    if (interface) {
        interface->exec();
    }
}

void PluginInterface::setParentWidget(QWidget *widget)
{
    mParentWidget = widget;
}

QHash<PimCommon::ActionType::Type, QList<QAction *> > PluginInterface::actionsType() const
{
    QHash<PimCommon::ActionType::Type, QList<QAction *> > listType;
    Q_FOREACH(PimCommon::GenericPluginInterface *interface, mListGenericInterface) {
        PimCommon::ActionType actionType = interface->actionType();
        const PimCommon::ActionType::Type type = actionType.type();
        if (listType.contains(type)) {
            QList<QAction *> lst = listType.value(type);
            lst << actionType.action();
            listType.insert(type, lst);
        } else {
            listType.insert(type, QList<QAction *>() << actionType.action());
        }
    }

    return listType;
}
