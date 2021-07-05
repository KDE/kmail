/*
   SPDX-FileCopyrightText: 2018-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kmailplugineditorconverttextmanagerinterface.h"
#include "kmail_debug.h"
#include <MessageComposer/PluginEditorConvertText>
#include <MessageComposer/PluginEditorConvertTextManager>
#include <QAction>

KMailPluginEditorConvertTextManagerInterface::KMailPluginEditorConvertTextManagerInterface(QObject *parent)
    : QObject(parent)
{
}

KMailPluginEditorConvertTextManagerInterface::~KMailPluginEditorConvertTextManagerInterface() = default;

void KMailPluginEditorConvertTextManagerInterface::enableDisablePluginActions(bool richText)
{
    for (MessageComposer::PluginEditorConvertTextInterface *interface : std::as_const(mListPluginInterface)) {
        interface->enableDisablePluginActions(richText);
    }
}

void KMailPluginEditorConvertTextManagerInterface::reformatText()
{
    for (MessageComposer::PluginEditorConvertTextInterface *interface : std::as_const(mListPluginInterface)) {
        if (interface->reformatText()) {
            // TODO signal that it was reformating.
            // Stop it.?
        }
    }
    Q_EMIT reformatingTextDone();
}

MessageComposer::PluginEditorConvertTextInterface::ConvertTextStatus
KMailPluginEditorConvertTextManagerInterface::convertTextToFormat(MessageComposer::TextPart *textPart)
{
    MessageComposer::PluginEditorConvertTextInterface::ConvertTextStatus status =
        MessageComposer::PluginEditorConvertTextInterface::ConvertTextStatus::NotConverted;
    for (MessageComposer::PluginEditorConvertTextInterface *interface : std::as_const(mListPluginInterface)) {
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
    for (MessageComposer::PluginEditorConvertTextInterface *interface : std::as_const(mListPluginInterface)) {
        interface->setInitialData(data);
    }
}

void KMailPluginEditorConvertTextManagerInterface::setDataBeforeConvertingText(const MessageComposer::PluginEditorConverterBeforeConvertingData &data)
{
    for (MessageComposer::PluginEditorConvertTextInterface *interface : std::as_const(mListPluginInterface)) {
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
        qCWarning(KMAIL_LOG) << "KMailPluginEditorConvertTextManagerInterface : Plugin was already initialized. This is a bug";
        return;
    }
    if (!mRichTextEditor) {
        qCWarning(KMAIL_LOG) << "KMailPluginEditorConvertTextManagerInterface : Richtexteditor is null. This is a bug";
        return;
    }
    if (!mParentWidget) {
        qCWarning(KMAIL_LOG) << "KMailPluginEditorConvertTextManagerInterface : Parent is null. This is a bug";
    }
    const QVector<MessageComposer::PluginEditorConvertText *> lstPlugin = MessageComposer::PluginEditorConvertTextManager::self()->pluginsList();
    for (MessageComposer::PluginEditorConvertText *plugin : lstPlugin) {
        if (plugin->isEnabled()) {
            MessageComposer::PluginEditorConvertTextInterface *interface = plugin->createInterface(this);
            interface->setRichTextEditor(mRichTextEditor);
            interface->setParentWidget(mParentWidget);
            interface->createAction(mActionCollection);
            interface->setPlugin(plugin);
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

QHash<MessageComposer::PluginActionType::Type, QList<QAction *>> KMailPluginEditorConvertTextManagerInterface::actionsType()
{
    if (mActionHash.isEmpty()) {
        for (MessageComposer::PluginEditorConvertTextInterface *interface : std::as_const(mListPluginInterface)) {
            const auto actionTypes = interface->actionTypes();
            for (const MessageComposer::PluginActionType &actionType : actionTypes) {
                MessageComposer::PluginActionType::Type type = actionType.type();
                QAction *currentAction = actionType.action();
                if (!currentAction) {
                    continue;
                }
                QList<QAction *> lst = mActionHash.value(type);
                if (!lst.isEmpty()) {
                    auto act = new QAction(this);
                    act->setSeparator(true);
                    lst << act << currentAction;
                    mActionHash.insert(type, lst);
                } else {
                    mActionHash.insert(type, QList<QAction *>() << currentAction);
                }
                if (interface->plugin()->hasPopupMenuSupport()) {
                    type = MessageComposer::PluginActionType::PopupMenu;
                    if (currentAction) {
                        lst = mActionHash.value(type);
                        if (!lst.isEmpty()) {
                            auto act = new QAction(this);
                            act->setSeparator(true);
                            lst << act << currentAction;
                            mActionHash.insert(type, lst);
                        } else {
                            mActionHash.insert(type, QList<QAction *>() << currentAction);
                        }
                    }
                }
                if (interface->plugin()->hasToolBarSupport()) {
                    type = MessageComposer::PluginActionType::ToolBar;
                    lst = mActionHash.value(type);
                    if (!lst.isEmpty()) {
                        auto act = new QAction(this);
                        act->setSeparator(true);
                        lst << act << currentAction;
                        mActionHash.insert(type, lst);
                    } else {
                        mActionHash.insert(type, QList<QAction *>() << currentAction);
                    }
                }
            }
        }
    }
    return mActionHash;
}

QList<QAction *> KMailPluginEditorConvertTextManagerInterface::actionsType(MessageComposer::PluginActionType::Type type)
{
    return mActionHash.value(type);
}

QList<QWidget *> KMailPluginEditorConvertTextManagerInterface::statusBarWidgetList()
{
    if (mStatusBarWidget.isEmpty() && !mListPluginInterface.isEmpty()) {
        for (MessageComposer::PluginEditorConvertTextInterface *interface : std::as_const(mListPluginInterface)) {
            if (interface->plugin()->hasStatusBarSupport()) {
                mStatusBarWidget.append(interface->statusBarWidget());
            }
        }
    }
    return mStatusBarWidget;
}
