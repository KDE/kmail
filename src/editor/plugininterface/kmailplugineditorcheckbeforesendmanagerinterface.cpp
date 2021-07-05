/*
   SPDX-FileCopyrightText: 2016-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kmailplugineditorcheckbeforesendmanagerinterface.h"
#include "kmail_debug.h"

#include <MessageComposer/PluginEditorCheckBeforeSend>
#include <MessageComposer/PluginEditorCheckBeforeSendInterface>
#include <MessageComposer/PluginEditorCheckBeforeSendManager>

KMailPluginEditorCheckBeforeSendManagerInterface::KMailPluginEditorCheckBeforeSendManagerInterface(QObject *parent)
    : QObject(parent)
{
}

KMailPluginEditorCheckBeforeSendManagerInterface::~KMailPluginEditorCheckBeforeSendManagerInterface() = default;

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
        qCWarning(KMAIL_LOG) << "Plugin was already initialized. This is a bug";
        return;
    }
    if (!mParentWidget) {
        qCWarning(KMAIL_LOG) << "KMailPluginEditorCheckBeforeSendManagerInterface : Parent is null. This is a bug";
    }

    const QVector<MessageComposer::PluginEditorCheckBeforeSend *> lstPlugin = MessageComposer::PluginEditorCheckBeforeSendManager::self()->pluginsList();
    for (MessageComposer::PluginEditorCheckBeforeSend *plugin : lstPlugin) {
        if (plugin->isEnabled()) {
            MessageComposer::PluginEditorCheckBeforeSendInterface *interface = plugin->createInterface(this);
            interface->setParentWidget(mParentWidget);
            interface->reloadConfig();
            mListPluginInterface.append(interface);
        }
    }
}

bool KMailPluginEditorCheckBeforeSendManagerInterface::execute(const MessageComposer::PluginEditorCheckBeforeSendParams &params) const
{
    for (MessageComposer::PluginEditorCheckBeforeSendInterface *interface : std::as_const(mListPluginInterface)) {
        if (!interface->exec(params)) {
            return false;
        }
    }
    return true;
}
