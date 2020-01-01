/*
   Copyright (C) 2019-2020 Laurent Montel <montel@kde.org>

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

#include "kmailplugingrammareditormanagerinterface.h"
#include "kmail_debug.h"
#include <PimCommon/CustomToolsWidgetng>
#include <PimCommon/CustomToolsPlugin>
#include <MessageComposer/PluginEditorGrammarManager>
#include <MessageComposer/PluginEditorGrammarCustomToolsViewInterface>

KMailPluginGrammarEditorManagerInterface::KMailPluginGrammarEditorManagerInterface(QObject *parent)
    : QObject(parent)
{
}

KPIMTextEdit::RichTextComposer *KMailPluginGrammarEditorManagerInterface::richTextEditor() const
{
    return mRichTextEditor;
}

void KMailPluginGrammarEditorManagerInterface::setRichTextEditor(KPIMTextEdit::RichTextComposer *richTextEditor)
{
    mRichTextEditor = richTextEditor;
}

QWidget *KMailPluginGrammarEditorManagerInterface::parentWidget() const
{
    return mParentWidget;
}

void KMailPluginGrammarEditorManagerInterface::setParentWidget(QWidget *parentWidget)
{
    mParentWidget = parentWidget;
}

KActionCollection *KMailPluginGrammarEditorManagerInterface::actionCollection() const
{
    return mActionCollection;
}

void KMailPluginGrammarEditorManagerInterface::setActionCollection(KActionCollection *actionCollection)
{
    mActionCollection = actionCollection;
}

PimCommon::CustomToolsWidgetNg *KMailPluginGrammarEditorManagerInterface::customToolsWidget() const
{
    return mCustomToolsWidget;
}

void KMailPluginGrammarEditorManagerInterface::setCustomToolsWidget(PimCommon::CustomToolsWidgetNg *customToolsWidget)
{
    mCustomToolsWidget = customToolsWidget;
}

void KMailPluginGrammarEditorManagerInterface::initializePlugins()
{
    if (mWasInitialized) {
        qCDebug(KMAIL_LOG) << "KMailPluginGrammarEditorManagerInterface : Plugin was already initialized. This is a bug";
        return;
    }
    if (!mRichTextEditor) {
        qCDebug(KMAIL_LOG) << "KMailPluginGrammarEditorManagerInterface : Richtexteditor is null. This is a bug";
        return;
    }
    if (!mParentWidget) {
        qCDebug(KMAIL_LOG) << "KMailPluginGrammarEditorManagerInterface : Parent is null. This is a bug";
    }
    if (!mCustomToolsWidget) {
        qCDebug(KMAIL_LOG) << "KMailPluginGrammarEditorManagerInterface : mCustomToolsWidget is null. This is a bug";
        return;
    }

    const QVector<PimCommon::CustomToolsPlugin *> lstPlugin = MessageComposer::PluginEditorGrammarManager::self()->pluginsList();
    for (PimCommon::CustomToolsPlugin *plugin : lstPlugin) {
        if (plugin->isEnabled()) {
            MessageComposer::PluginEditorGrammarCustomToolsViewInterface *interface = static_cast<MessageComposer::PluginEditorGrammarCustomToolsViewInterface *>(plugin->createView(mActionCollection, mCustomToolsWidget));
            mCustomToolsWidget->addCustomToolViewInterface(interface);
            interface->setParentWidget(mParentWidget);
            interface->setRichTextEditor(mRichTextEditor);
        }
    }
    mWasInitialized = true;
}
