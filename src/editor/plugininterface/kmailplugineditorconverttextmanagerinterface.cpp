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
#include <QAction>

KMailPluginEditorConvertTextManagerInterface::KMailPluginEditorConvertTextManagerInterface(QObject *parent)
    : QObject(parent)
{
}

KMailPluginEditorConvertTextManagerInterface::~KMailPluginEditorConvertTextManagerInterface()
{
}

void KMailPluginEditorConvertTextManagerInterface::enableDisablePluginActions(bool richText)
{
    for (MessageComposer::PluginEditorConvertTextInterface *interface : qAsConst(mListPluginInterface)) {
        interface->enableDisablePluginActions(richText);
    }
}

void KMailPluginEditorConvertTextManagerInterface::reformatText()
{
    for (MessageComposer::PluginEditorConvertTextInterface *interface : qAsConst(mListPluginInterface)) {
        if (interface->reformatText()) {
            //TODO signal that it was reformating.
            //Stop it.?
        }
    }
    Q_EMIT reformatingTextDone();
}

MessageComposer::PluginEditorConvertTextInterface::ConvertTextStatus KMailPluginEditorConvertTextManagerInterface::convertTextToFormat(MessageComposer::TextPart *textPart)
{
    MessageComposer::PluginEditorConvertTextInterface::ConvertTextStatus status = MessageComposer::PluginEditorConvertTextInterface::ConvertTextStatus::NotConverted;
    for (MessageComposer::PluginEditorConvertTextInterface *interface : qAsConst(mListPluginInterface)) {
        switch (interface->convertTextToFormat(textPart)) {
        case MessageComposer::PluginEditorConvertTextInterface::ConvertTextStatus::NotConverted:
            if (status != MessageComposer::PluginEditorConvertTextInterface::ConvertTextStatus::Converted) {
                status = MessageComposer::PluginEditorConvertTextInterface::ConvertTextStatus::NotConverted;
            }
            break;
        case MessageComposer::PluginEditorConvertTextInterface::ConvertTextStatus::Converted:
            status = MessageComposer::PluginEditorConvertTextInterface::ConvertTextStatus::Converted;
            break;
        case MessageComposer::PluginEditorConvertTextInterface::ConvertTextStatus::Error:
            status = MessageComposer::PluginEditorConvertTextInterface::ConvertTextStatus::Error;
            return status;
        }
    }
    return status;
}

void KMailPluginEditorConvertTextManagerInterface::setInitialData(const MessageComposer::PluginEditorConverterInitialData &data)
{
    for (MessageComposer::PluginEditorConvertTextInterface *interface : qAsConst(mListPluginInterface)) {
        interface->setInitialData(data);
    }
}

void KMailPluginEditorConvertTextManagerInterface::setDataBeforeConvertingText(const MessageComposer::PluginEditorConverterBeforeConvertingData &data)
{
    for (MessageComposer::PluginEditorConvertTextInterface *interface : qAsConst(mListPluginInterface)) {
        interface->setBeforeConvertingData(data);
    }
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

QHash<MessageComposer::PluginActionType::Type, QList<QAction *> > KMailPluginEditorConvertTextManagerInterface::actionsType()
{
    if (mActionHash.isEmpty()) {
        for (MessageComposer::PluginEditorConvertTextInterface *interface : qAsConst(mListPluginInterface)) {
            const MessageComposer::PluginActionType actionType = interface->actionType();
            MessageComposer::PluginActionType::Type type = actionType.type();
            QList<QAction *> lst = mActionHash.value(type);
            if (!lst.isEmpty()) {
                QAction *act = new QAction(this);
                act->setSeparator(true);
                lst << act << actionType.action();
                mActionHash.insert(type, lst);
            } else {
                mActionHash.insert(type, QList<QAction *>() << actionType.action());
            }
            /*
            if (interface->plugin()->hasPopupMenuSupport()) {
                type = MessageComposer::PluginActionType::PopupMenu;
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
                type = MessageComposer::PluginActionType::ToolBar;
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
            */
        }
    }
    return mActionHash;
}

QList<QAction *> KMailPluginEditorConvertTextManagerInterface::actionsType(MessageComposer::PluginActionType::Type type)
{
    return mActionHash.value(type);
}
