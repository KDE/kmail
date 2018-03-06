/*
   Copyright (C) 2018 Montel Laurent <montel@kde.org>

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


#include "kmailplugineditorconverttextmanagerinterface.h"
#include "kmail_debug.h"
#include <MessageComposer/PluginEditorConvertText>
#include <MessageComposer/PluginEditorConvertTextManager>
#include <MessageComposer/PluginEditorConvertTextInterface>

KMailPluginEditorConvertTextManagerInterface::KMailPluginEditorConvertTextManagerInterface(QObject *parent)
    : QObject(parent)
{

}

KMailPluginEditorConvertTextManagerInterface::~KMailPluginEditorConvertTextManagerInterface()
{

}

QWidget *KMailPluginEditorConvertTextManagerInterface::parentWidget() const
{
    return mParentWidget;
}

void KMailPluginEditorConvertTextManagerInterface::setParentWidget(QWidget *parentWidget)
{
    mParentWidget = parentWidget;
}

KActionCollection *KMailPluginEditorConvertTextManagerInterface::actionCollection() const
{
    return mActionCollection;
}

void KMailPluginEditorConvertTextManagerInterface::setActionCollection(KActionCollection *actionCollection)
{
    mActionCollection = actionCollection;
}

void KMailPluginEditorConvertTextManagerInterface::initializePlugins()
{
    if (!mListPluginInterface.isEmpty()) {
        qCDebug(KMAIL_LOG) << "KMailPluginEditorConvertTextManagerInterface : Plugin was already initialized. This is a bug";
        return;
    }
    if (!mRichTextEditor) {
        qCDebug(KMAIL_LOG) << "KMailPluginEditorConvertTextManagerInterface : Richtexteditor is null. This is a bug";
        return;
    }
    if (!mParentWidget) {
        qCDebug(KMAIL_LOG) << "KMailPluginEditorConvertTextManagerInterface : Parent is null. This is a bug";
    }
    const QVector<MessageComposer::PluginEditorConvertText *> lstPlugin = MessageComposer::PluginEditorConvertTextManager::self()->pluginsList();
    for (MessageComposer::PluginEditorConvertText *plugin : lstPlugin) {
        if (plugin->isEnabled()) {
            MessageComposer::PluginEditorConvertTextInterface *interface = static_cast<MessageComposer::PluginEditorConvertTextInterface *>(plugin->createInterface(mActionCollection, this));
            interface->setRichTextEditor(mRichTextEditor);
            interface->setParentWidget(mParentWidget);
            mListPluginInterface.append(interface);
        }
    }
    qCDebug(KMAIL_LOG) << "KMailPluginEditorConvertTextManagerInterface : Initialize done, number of plugins found: " << mListPluginInterface.count();
}

KPIMTextEdit::RichTextComposer *KMailPluginEditorConvertTextManagerInterface::richTextEditor() const
{
    return mRichTextEditor;
}

void KMailPluginEditorConvertTextManagerInterface::setRichTextEditor(KPIMTextEdit::RichTextComposer *richTextEditor)
{
    mRichTextEditor = richTextEditor;
}
