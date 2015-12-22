/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

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

#ifndef PLUGINEDITORINTERFACE_H
#define PLUGINEDITORINTERFACE_H

#include <QObject>
class QAction;
class KActionCollection;

namespace KPIMTextEdit
{
class RichTextEditor;
}

class ActionType
{
public:
    enum Type {
        Tools = 0,
        Edit = 1,
        File = 2,
        Action = 3,
        PopupMenu = 4
    };
    ActionType();

    ActionType(QAction *action, Type type);
    QAction *action() const;
    Type type() const;

private:
    QAction *mAction;
    Type mType;
};

class PluginEditorInterfacePrivate;
class PluginEditorInterface : public QObject
{
    Q_OBJECT
public:
    explicit PluginEditorInterface(QObject *parent = Q_NULLPTR);
    ~PluginEditorInterface();

    void setActionType(const ActionType &type);
    ActionType actionType() const;

    virtual void createAction(KActionCollection *ac) = 0;
    virtual void exec() = 0;

    void setParentWidget(QWidget *parent);
    QWidget *parentWidget() const;


    KPIMTextEdit::RichTextEditor *richTextEditor() const;
    void setRichTextEditor(KPIMTextEdit::RichTextEditor *richTextEditor);

    virtual bool hasPopupMenuSupport() const;
    virtual bool hasConfigureDialog() const;

    virtual void showConfigureDialog(QWidget *parentWidget = Q_NULLPTR);

Q_SIGNALS:
    void emitPluginActivated(PluginEditorInterface *interface);

private:
    PluginEditorInterfacePrivate *const d;
};

#endif // PLUGINEDITORINTERFACE_H
