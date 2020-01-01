/*
   Copyright (C) 2015-2020 Laurent Montel <montel@kde.org>

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

#ifndef KMAILPLUGINEDITORMANAGERINTERFACE_H
#define KMAILPLUGINEDITORMANAGERINTERFACE_H

#include <QObject>
#include <QHash>
#include "messagecomposer/plugineditorinterface.h"
namespace KPIMTextEdit {
class RichTextEditor;
}
namespace MessageComposer {
class PluginEditorInterface;
class ComposerViewBase;
}
class KActionCollection;
class KMailPluginEditorManagerInterface : public QObject
{
    Q_OBJECT
public:
    explicit KMailPluginEditorManagerInterface(QObject *parent = nullptr);
    ~KMailPluginEditorManagerInterface();

    KPIMTextEdit::RichTextEditor *richTextEditor() const;
    void setRichTextEditor(KPIMTextEdit::RichTextEditor *richTextEditor);

    QWidget *parentWidget() const;
    void setParentWidget(QWidget *parentWidget);

    void initializePlugins();

    KActionCollection *actionCollection() const;
    void setActionCollection(KActionCollection *actionCollection);

    Q_REQUIRED_RESULT QHash<MessageComposer::PluginActionType::Type, QList<QAction *> > actionsType();
    Q_REQUIRED_RESULT QList<QAction *> actionsType(MessageComposer::PluginActionType::Type type);
    Q_REQUIRED_RESULT QList<QWidget *> statusBarWidgetList();

    MessageComposer::ComposerViewBase *composerInterface() const;
    void setComposerInterface(MessageComposer::ComposerViewBase *composerInterface);

    Q_REQUIRED_RESULT bool processProcessKeyEvent(QKeyEvent *event);

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
    QHash<MessageComposer::PluginActionType::Type, QList<QAction *> > mActionHash;
    QList<QWidget *> mStatusBarWidget;
};

#endif // KMAILPLUGINEDITORMANAGERINTERFACE_H
