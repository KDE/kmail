/*
  Copyright (c) 2015-2016 Montel Laurent <montel@kde.org>

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

#include "kmailplugineditormanagerinterface.h"
#include "messagecomposer/plugineditormanager.h"
#include "messagecomposer/plugineditor.h"
#include "kmail_debug.h"

#include <QVector>

KMailPluginEditorManagerInterface::KMailPluginEditorManagerInterface(QObject *parent)
    : QObject(parent),
      mRichTextEditor(Q_NULLPTR),
      mParentWidget(Q_NULLPTR),
      mActionCollection(Q_NULLPTR)
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
    Q_FOREACH (MessageComposer::PluginEditor *plugin, lstPlugin) {
        MessageComposer::PluginEditorInterface *interface = plugin->createInterface(mActionCollection, this);
        interface->setRichTextEditor(mRichTextEditor);
        connect(interface, &MessageComposer::PluginEditorInterface::emitPluginActivated, this, &KMailPluginEditorManagerInterface::slotPluginActivated);
        mListPluginInterface.append(interface);
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

QHash<MessageComposer::ActionType::Type, QList<QAction *> > KMailPluginEditorManagerInterface::actionsType() const
{
    QHash<MessageComposer::ActionType::Type, QList<QAction *> > listType;
    Q_FOREACH (MessageComposer::PluginEditorInterface *interface, mListPluginInterface) {
        MessageComposer::ActionType actionType = interface->actionType();
        MessageComposer::ActionType::Type type = actionType.type();
        if (listType.contains(type)) {
            QList<QAction *> lst = listType.value(type);
            lst << actionType.action();
            listType.insert(type, lst);
        } else {
            listType.insert(type, QList<QAction *>() << actionType.action());
        }
        if (interface->hasPopupMenuSupport()) {
            type = MessageComposer::ActionType::PopupMenu;
            if (listType.contains(type)) {
                QList<QAction *> lst = listType.value(type);
                lst << actionType.action();
                listType.insert(type, lst);
            } else {
                listType.insert(type, QList<QAction *>() << actionType.action());
            }
        }
        if (interface->hasToolBarSupport()) {
            type = MessageComposer::ActionType::ToolBar;
            if (listType.contains(type)) {
                QList<QAction *> lst = listType.value(type);
                lst << actionType.action();
                listType.insert(type, lst);
            } else {
                listType.insert(type, QList<QAction *>() << actionType.action());
            }
        }
    }
    return listType;
}
