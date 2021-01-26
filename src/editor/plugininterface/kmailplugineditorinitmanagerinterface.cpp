/*
   SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kmailplugineditorinitmanagerinterface.h"
#include "kmail_debug.h"
#include <MessageComposer/PluginEditorInit>
#include <MessageComposer/PluginEditorInitInterface>
#include <MessageComposer/PluginEditorInitManager>

KMailPluginEditorInitManagerInterface::KMailPluginEditorInitManagerInterface(QObject *parent)
    : QObject(parent)
{
}

KPIMTextEdit::RichTextComposer *KMailPluginEditorInitManagerInterface::richTextEditor() const
{
    return mRichTextEditor;
}

void KMailPluginEditorInitManagerInterface::setRichTextEditor(KPIMTextEdit::RichTextComposer *richTextEditor)
{
    mRichTextEditor = richTextEditor;
}

QWidget *KMailPluginEditorInitManagerInterface::parentWidget() const
{
    return mParentWidget;
}

void KMailPluginEditorInitManagerInterface::setParentWidget(QWidget *parentWidget)
{
    mParentWidget = parentWidget;
}

void KMailPluginEditorInitManagerInterface::initializePlugins()
{
    if (mWasInitialized) {
        qCDebug(KMAIL_LOG) << "KMailPluginEditorInitManagerInterface : Plugin was already initialized. This is a bug";
        return;
    }
    if (!mRichTextEditor) {
        qCDebug(KMAIL_LOG) << "KMailPluginEditorInitManagerInterface : Richtexteditor is null. This is a bug";
        return;
    }
    if (!mParentWidget) {
        qCDebug(KMAIL_LOG) << "KMailPluginEditorInitManagerInterface : Parent is null. This is a bug";
    }

    const QVector<MessageComposer::PluginEditorInit *> lstPlugin = MessageComposer::PluginEditorInitManager::self()->pluginsList();
    for (MessageComposer::PluginEditorInit *plugin : lstPlugin) {
        if (plugin->isEnabled()) {
            MessageComposer::PluginEditorInitInterface *interface = plugin->createInterface(this);
            interface->setParentWidget(mParentWidget);
            interface->setRichTextEditor(mRichTextEditor);
            interface->reloadConfig();
            if (!interface->exec()) {
                qCWarning(KMAIL_LOG) << "KMailPluginEditorInitManagerInterface::initializePlugins: error during execution of plugin:" << interface;
            }
        }
    }
    mWasInitialized = true;
}
