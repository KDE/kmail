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

#include "kmailplugininterface.h"
#include <KActionCollection>
#include "kmail_debug.h"

KMailPluginInterface::KMailPluginInterface(KActionCollection *ac, QObject *parent)
    : PimCommon::PluginInterface(ac, parent),
      mMainWindow(Q_NULLPTR)
{
    setPluginName(QStringLiteral("kmail"));
    setServiceTypeName(QStringLiteral("KMail/MainViewPlugin"));
    initializePlugins();
}

KMailPluginInterface::~KMailPluginInterface()
{

}

void KMailPluginInterface::setMainWidget(KMMainWidget *mainwindow)
{
    mMainWindow = mainwindow;
}

void KMailPluginInterface::initializeInterfaceRequires(PimCommon::GenericPluginInterface *interface)
{
    PimCommon::GenericPluginInterface::RequireTypes requires = interface->requires();
    if (requires & PimCommon::GenericPluginInterface::CurrentItems) {
        qCDebug(KMAIL_LOG) << "PimCommon::GenericPluginInterface::CurrentItems not implemented";
    }
    if (requires & PimCommon::GenericPluginInterface::Items) {
        qCDebug(KMAIL_LOG) << "PimCommon::GenericPluginInterface::Items not implemented";
        //TODO
    }
    if (requires & PimCommon::GenericPluginInterface::CurrentCollection) {
        qCDebug(KMAIL_LOG) << "PimCommon::GenericPluginInterface::CurrentCollection not implemented";
        //TODO
    }
    if (requires & PimCommon::GenericPluginInterface::Collections) {
        qCDebug(KMAIL_LOG) << "PimCommon::GenericPluginInterface::Collection not implemented";
        //TODO
    }
}
