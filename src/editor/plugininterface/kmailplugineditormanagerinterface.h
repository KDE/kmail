/*
   SPDX-FileCopyrightText: 2015-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <MessageComposer/PluginEditorInterface>
#include <QHash>
#include <QObject>
namespace TextCustomEditor
{
class RichTextEditor;
}
namespace MessageComposer
{
class PluginEditorInterface;
class ComposerViewBase;
}
class KActionCollection;
class KMailPluginEditorManagerInterface : public QObject
{
    Q_OBJECT
public:
    explicit KMailPluginEditorManagerInterface(QObject *parent = nullptr);
    ~KMailPluginEditorManagerInterface() override;

    [[nodiscard]] TextCustomEditor::RichTextEditor *richTextEditor() const;
    void setRichTextEditor(TextCustomEditor::RichTextEditor *richTextEditor);

    [[nodiscard]] QWidget *parentWidget() const;
    void setParentWidget(QWidget *parentWidget);

    void initializePlugins();

    [[nodiscard]] KActionCollection *actionCollection() const;
    void setActionCollection(KActionCollection *actionCollection);

    [[nodiscard]] QHash<MessageComposer::PluginActionType::Type, QList<QAction *>> actionsType();
    [[nodiscard]] QList<QAction *> actionsType(MessageComposer::PluginActionType::Type type);
    [[nodiscard]] QList<QWidget *> statusBarWidgetList();

    [[nodiscard]] MessageComposer::ComposerViewBase *composerInterface() const;
    void setComposerInterface(MessageComposer::ComposerViewBase *composerInterface);

    [[nodiscard]] bool processProcessKeyEvent(QKeyEvent *event);

    void setStatusBarWidgetEnabled(MessageComposer::PluginEditorInterface::ApplyOnFieldType type);
Q_SIGNALS:
    void textSelectionChanged(bool hasSelection);
    void message(const QString &str);
    void insertText(const QString &str);
    void errorMessage(const QString &message);
    void successMessage(const QString &message);

private:
    void slotPluginActivated(MessageComposer::PluginEditorInterface *interface);
    TextCustomEditor::RichTextEditor *mRichTextEditor = nullptr;
    MessageComposer::ComposerViewBase *mComposerInterface = nullptr;
    QWidget *mParentWidget = nullptr;
    KActionCollection *mActionCollection = nullptr;
    QList<MessageComposer::PluginEditorInterface *> mListPluginInterface;
    QHash<MessageComposer::PluginActionType::Type, QList<QAction *>> mActionHash;
    QList<QWidget *> mStatusBarWidget;
};
