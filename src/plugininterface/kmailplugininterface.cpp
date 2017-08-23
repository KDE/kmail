/*
   Copyright (C) 2015-2017 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kmailplugininterface.h"
#include <KActionCollection>
#include <kmmainwidget.h>
#include "kmail_debug.h"

KMailPluginInterface::KMailPluginInterface(QObject *parent)
    : PimCommon::PluginInterface(parent)
{
    setPluginName(QStringLiteral("kmail"));
    setServiceTypeName(QStringLiteral("KMail/MainViewPlugin"));
}

KMailPluginInterface::~KMailPluginInterface()
{
}

KMailPluginInterface *KMailPluginInterface::self()
{
    static KMailPluginInterface s_self;
    return &s_self;
}

void KMailPluginInterface::setMainWidget(KMMainWidget *mainwindow)
{
    mMainWindow = mainwindow;
}

void KMailPluginInterface::initializeInterfaceRequires(PimCommon::AbstractGenericPluginInterface *abstractInterface)
{
    if (!mMainWindow) {
        qCCritical(KMAIL_LOG) << "mainwindows not defined";
        return;
    }
    PimCommon::GenericPluginInterface *interface = static_cast<PimCommon::GenericPluginInterface *>(abstractInterface);
    PimCommon::GenericPluginInterface::RequireTypes requires = interface->requires();
    if (requires & PimCommon::GenericPluginInterface::CurrentItems) {
        interface->setItems(mMainWindow->currentSelection());
    }
    if (requires & PimCommon::GenericPluginInterface::Items) {
        qCDebug(KMAIL_LOG) << "PimCommon::GenericPluginInterface::Items not implemented";
    }
    if (requires & PimCommon::GenericPluginInterface::CurrentCollection) {
        if (mMainWindow->currentCollection().isValid()) {
            interface->setCurrentCollection(mMainWindow->currentCollection());
        } else {
            qCDebug(KMAIL_LOG) << "Current Collection not defined";
        }
    }
    if (requires & PimCommon::GenericPluginInterface::Collections) {
        qCDebug(KMAIL_LOG) << "PimCommon::GenericPluginInterface::Collection not implemented";
    }
}
