/*
  Copyright (c) 2016 Montel Laurent <montel@kde.org>

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


#include "kmailplugineditorcheckbeforesendmanagerinterface.h"
#include "kmail_debug.h"
#include <MessageComposer/PluginEditorCheckBeforeSendInterface>
#include <MessageComposer/PluginEditorCheckBeforeSend>
#include <MessageComposer/PluginEditorCheckBeforeSendManager>

KMailPluginEditorCheckBeforeSendManagerInterface::KMailPluginEditorCheckBeforeSendManagerInterface(QObject *parent)
    : QObject(parent),
      mParentWidget(Q_NULLPTR)
{

}

KMailPluginEditorCheckBeforeSendManagerInterface::~KMailPluginEditorCheckBeforeSendManagerInterface()
{

}

QWidget *KMailPluginEditorCheckBeforeSendManagerInterface::parentWidget() const
{
    return mParentWidget;
}

void KMailPluginEditorCheckBeforeSendManagerInterface::setParentWidget(QWidget *parentWidget)
{
    mParentWidget = parentWidget;
}

void KMailPluginEditorCheckBeforeSendManagerInterface::initializePlugins()
{
    if (!mListPluginInterface.isEmpty()) {
        qCDebug(KMAIL_LOG) << "Plugin was already initialized. This is a bug";
        return;
    }
    const QVector<MessageComposer::PluginEditorCheckBeforeSend *> lstPlugin = MessageComposer::PluginEditorCheckBeforeSendManager::self()->pluginsList();
    Q_FOREACH (MessageComposer::PluginEditorCheckBeforeSend *plugin, lstPlugin) {
        MessageComposer::PluginEditorCheckBeforeSendInterface *interface = plugin->createInterface(this);
        interface->setParentWidget(mParentWidget);
        mListPluginInterface.append(interface);
    }
}
