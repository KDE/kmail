/*
   Copyright (C) 2015-2016 Montel Laurent <montel@kde.org>

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
namespace KPIMTextEdit
{
class RichTextEditor;
}
namespace MessageComposer
{
class PluginEditorInterface;
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

    QHash<MessageComposer::ActionType::Type, QList<QAction *> > actionsType();
    QList<QAction *> actionsType(MessageComposer::ActionType::Type type);

private:
    void slotPluginActivated(MessageComposer::PluginEditorInterface *interface);
    KPIMTextEdit::RichTextEditor *mRichTextEditor;
    QWidget *mParentWidget;
    KActionCollection *mActionCollection;
    QList<MessageComposer::PluginEditorInterface *> mListPluginInterface;
    QHash<MessageComposer::ActionType::Type, QList<QAction *> > mActionHash;
};

#endif // KMAILPLUGINEDITORMANAGERINTERFACE_H
