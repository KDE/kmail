/*
   SPDX-FileCopyrightText: 2018-2023 Laurent Montel <montel@kde.org>

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
    Q_REQUIRED_RESULT QWidget *parentWidget() const;
    void setParentWidget(QWidget *parentWidget);

    Q_REQUIRED_RESULT KActionCollection *actionCollection() const;
    void setActionCollection(KActionCollection *actionCollection);

    void initializePlugins();

    Q_REQUIRED_RESULT KPIMTextEdit::RichTextComposer *richTextEditor() const;
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
    QList<MessageComposer::PluginEditorConvertTextInterface *> mListPluginInterface;
    QHash<MessageComposer::PluginActionType::Type, QList<QAction *>> mActionHash;
    KPIMTextEdit::RichTextComposer *mRichTextEditor = nullptr;
    QWidget *mParentWidget = nullptr;
    KActionCollection *mActionCollection = nullptr;
    QList<QWidget *> mStatusBarWidget;
};
