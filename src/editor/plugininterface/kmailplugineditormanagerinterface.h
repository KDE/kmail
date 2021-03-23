/*
   SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <MessageComposer/PluginEditorInterface>
#include <QHash>
#include <QObject>
namespace KPIMTextEdit
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

    KPIMTextEdit::RichTextEditor *richTextEditor() const;
    void setRichTextEditor(KPIMTextEdit::RichTextEditor *richTextEditor);

    QWidget *parentWidget() const;
    void setParentWidget(QWidget *parentWidget);

    void initializePlugins();

    KActionCollection *actionCollection() const;
    void setActionCollection(KActionCollection *actionCollection);

    Q_REQUIRED_RESULT QHash<MessageComposer::PluginActionType::Type, QList<QAction *>> actionsType();
    Q_REQUIRED_RESULT QList<QAction *> actionsType(MessageComposer::PluginActionType::Type type);
    Q_REQUIRED_RESULT QList<QWidget *> statusBarWidgetList();

    MessageComposer::ComposerViewBase *composerInterface() const;
    void setComposerInterface(MessageComposer::ComposerViewBase *composerInterface);

    Q_REQUIRED_RESULT bool processProcessKeyEvent(QKeyEvent *event);

    void setStatusBarWidgetEnabled(MessageComposer::PluginEditorInterface::ApplyOnFieldType type);
Q_SIGNALS:
    void textSelectionChanged(bool hasSelection);
    void message(const QString &str);
    void insertText(const QString &str);

private:
    Q_DISABLE_COPY(KMailPluginEditorManagerInterface)
    void slotPluginActivated(MessageComposer::PluginEditorInterface *interface);
    KPIMTextEdit::RichTextEditor *mRichTextEditor = nullptr;
    MessageComposer::ComposerViewBase *mComposerInterface = nullptr;
    QWidget *mParentWidget = nullptr;
    KActionCollection *mActionCollection = nullptr;
    QList<MessageComposer::PluginEditorInterface *> mListPluginInterface;
    QHash<MessageComposer::PluginActionType::Type, QList<QAction *>> mActionHash;
    QList<QWidget *> mStatusBarWidget;
};

