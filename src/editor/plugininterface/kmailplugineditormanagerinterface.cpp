/*
   Copyright (C) 2015-2018 Montel Laurent <montel@kde.org>

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

#include "kmailplugineditormanagerinterface.h"
#include "messagecomposer/plugineditormanager.h"
#include "messagecomposer/plugineditor.h"
#include "kmail_debug.h"

#include <QAction>
#include <QVector>

KMailPluginEditorManagerInterface::KMailPluginEditorManagerInterface(QObject *parent)
    : QObject(parent)
{
}

KMailPluginEditorManagerInterface::~KMailPluginEditorManagerInterface()
{
}

KPIMTextEdit::RichTextEditor *KMailPluginEditorManagerInterface::richTextEditor() const
{
    return mRichTextEditor;
}

void KMailPluginEditorManagerInterface::setRichTextEditor(KPIMTextEdit::RichTextEditor *richTextEditor)
{
    mRichTextEditor = richTextEditor;
}

QWidget *KMailPluginEditorManagerInterface::parentWidget() const
{
    return mParentWidget;
}

void KMailPluginEditorManagerInterface::setParentWidget(QWidget *parentWidget)
{
    mParentWidget = parentWidget;
}

void KMailPluginEditorManagerInterface::initializePlugins()
{
    if (!mListPluginInterface.isEmpty()) {
        qCDebug(KMAIL_LOG) << "Plugin was already initialized. This is a bug";
        return;
    }
    if (!mRichTextEditor) {
        qCDebug(KMAIL_LOG) << "Missing richtexteditor";
        return;
    }
    const QVector<MessageComposer::PluginEditor *> lstPlugin = MessageComposer::PluginEditorManager::self()->pluginsList();
    for (MessageComposer::PluginEditor *plugin : lstPlugin) {
        if (plugin->isEnabled()) {
            MessageComposer::PluginEditorInterface *interface = static_cast<MessageComposer::PluginEditorInterface *>(plugin->createInterface(mActionCollection, this));
            interface->setRichTextEditor(mRichTextEditor);
            interface->setParentWidget(mParentWidget);
            interface->setPlugin(plugin);
            connect(interface, &MessageComposer::PluginEditorInterface::emitPluginActivated, this, &KMailPluginEditorManagerInterface::slotPluginActivated);
            connect(interface, &MessageComposer::PluginEditorInterface::message, this, &KMailPluginEditorManagerInterface::message);
            mListPluginInterface.append(interface);
        }
    }
}

void KMailPluginEditorManagerInterface::slotPluginActivated(MessageComposer::PluginEditorInterface *interface)
{
    interface->exec();
}

KActionCollection *KMailPluginEditorManagerInterface::actionCollection() const
{
    return mActionCollection;
}

void KMailPluginEditorManagerInterface::setActionCollection(KActionCollection *actionCollection)
{
    mActionCollection = actionCollection;
}

QList<QAction *> KMailPluginEditorManagerInterface::actionsType(MessageComposer::ActionType::Type type)
{
    return mActionHash.value(type);
}

QHash<MessageComposer::ActionType::Type, QList<QAction *> > KMailPluginEditorManagerInterface::actionsType()
{
    if (mActionHash.isEmpty()) {
        for (MessageComposer::PluginEditorInterface *interface : qAsConst(mListPluginInterface)) {
            const MessageComposer::ActionType actionType = interface->actionType();
            MessageComposer::ActionType::Type type = actionType.type();
            const bool needSelectedText = interface->needSelectedText();
            if (needSelectedText) {
                //Disable by default as we don't have selection by default.
                actionType.action()->setEnabled(false);
                connect(this, &KMailPluginEditorManagerInterface::textSelectionChanged, actionType.action(), &QAction::setEnabled);
            }
            QList<QAction *> lst = mActionHash.value(type);
            if (!lst.isEmpty()) {
                QAction *act = new QAction(this);
                act->setSeparator(true);
                lst << act << actionType.action();
                mActionHash.insert(type, lst);
            } else {
                mActionHash.insert(type, QList<QAction *>() << actionType.action());
            }
            if (interface->plugin()->hasPopupMenuSupport()) {
                type = MessageComposer::ActionType::PopupMenu;
                QList<QAction *> lst = mActionHash.value(type);
                if (!lst.isEmpty()) {
                    QAction *act = new QAction(this);
                    act->setSeparator(true);
                    lst << act << actionType.action();
                    mActionHash.insert(type, lst);
                } else {
                    mActionHash.insert(type, QList<QAction *>() << actionType.action());
                }
            }
            if (interface->plugin()->hasToolBarSupport()) {
                type = MessageComposer::ActionType::ToolBar;
                QList<QAction *> lst = mActionHash.value(type);
                if (!lst.isEmpty()) {
                    QAction *act = new QAction(this);
                    act->setSeparator(true);
                    lst << act << actionType.action();
                    mActionHash.insert(type, lst);
                } else {
                    mActionHash.insert(type, QList<QAction *>() << actionType.action());
                }
            }
        }
    }
    return mActionHash;
}
