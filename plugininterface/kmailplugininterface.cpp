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
#if 0
    PimCommon::GenericPluginInterface::RequireTypes requires = interface->requires();
    if (requires & PimCommon::GenericPluginInterface::CurrentItems) {

    }
    if (requires & PimCommon::GenericPluginInterface::Items) {
        //TODO
    }
    if (requires & PimCommon::GenericPluginInterface::CurrentCollection) {
        //TODO
    }
    if (requires & PimCommon::GenericPluginInterface::Collections) {
        //TODO
    }
#else
    Q_UNUSED(interface);
#endif

}
