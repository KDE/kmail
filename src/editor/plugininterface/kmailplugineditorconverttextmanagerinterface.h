/*
   SPDX-FileCopyrightText: 2018-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <MessageComposer/PluginEditorConvertTextInterface>
#include <QHash>
#include <QObject>
class QWidget;
namespace KPIMTextEdit
{
class RichTextComposer;
}
class KActionCollection;
class KMailPluginEditorConvertTextManagerInterface : public QObject
{
    Q_OBJECT
public:
    explicit KMailPluginEditorConvertTextManagerInterface(QObject *parent = nullptr);
    ~KMailPluginEditorConvertTextManagerInterface() override;
    QWidget *parentWidget() const;
    void setParentWidget(QWidget *parentWidget);

    KActionCollection *actionCollection() const;
    void setActionCollection(KActionCollection *actionCollection);

    void initializePlugins();

    KPIMTextEdit::RichTextComposer *richTextEditor() const;
    void setRichTextEditor(KPIMTextEdit::RichTextComposer *richTextEditor);

    Q_REQUIRED_RESULT QHash<MessageComposer::PluginActionType::Type, QList<QAction *>> actionsType();
    Q_REQUIRED_RESULT QList<QAction *> actionsType(MessageComposer::PluginActionType::Type type);

    void reformatText();
    MessageComposer::PluginEditorConvertTextInterface::ConvertTextStatus convertTextToFormat(MessageComposer::TextPart *textPart);

    void setInitialData(const MessageComposer::PluginEditorConverterInitialData &data);
    void setDataBeforeConvertingText(const MessageComposer::PluginEditorConverterBeforeConvertingData &data);
    void enableDisablePluginActions(bool richText);

    Q_REQUIRED_RESULT QList<QWidget *> statusBarWidgetList();
Q_SIGNALS:
    void reformatingTextDone();

private:
    Q_DISABLE_COPY(KMailPluginEditorConvertTextManagerInterface)
    QList<MessageComposer::PluginEditorConvertTextInterface *> mListPluginInterface;
    QHash<MessageComposer::PluginActionType::Type, QList<QAction *>> mActionHash;
    KPIMTextEdit::RichTextComposer *mRichTextEditor = nullptr;
    QWidget *mParentWidget = nullptr;
    KActionCollection *mActionCollection = nullptr;
    QList<QWidget *> mStatusBarWidget;
};

