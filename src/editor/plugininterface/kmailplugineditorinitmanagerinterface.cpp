/*
   Copyright (C) 2017-2018 Montel Laurent <montel@kde.org>

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

#include "kmailplugineditorinitmanagerinterface.h"
#include "kmail_debug.h"
#include <MessageComposer/PluginEditorInitManager>
#include <MessageComposer/PluginEditorInit>
#include <MessageComposer/PluginEditorInitInterface>

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
