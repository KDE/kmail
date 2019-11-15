/*
   Copyright (C) 2018-2019 Montel Laurent <montel@kde.org>

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

#ifndef KMAILPLUGINEDITORCONVERTTEXTMANAGERINTERFACE_H
#define KMAILPLUGINEDITORCONVERTTEXTMANAGERINTERFACE_H

#include <QObject>
#include <MessageComposer/PluginEditorConvertTextInterface>
#include <QHash>
class QWidget;
namespace KPIMTextEdit {
class RichTextComposer;
}
class KActionCollection;
class KMailPluginEditorConvertTextManagerInterface : public QObject
{
    Q_OBJECT
public:
    explicit KMailPluginEditorConvertTextManagerInterface(QObject *parent = nullptr);
    ~KMailPluginEditorConvertTextManagerInterface();
    QWidget *parentWidget() const;
    void setParentWidget(QWidget *parentWidget);

    KActionCollection *actionCollection() const;
    void setActionCollection(KActionCollection *actionCollection);

    void initializePlugins();

    KPIMTextEdit::RichTextComposer *richTextEditor() const;
    void setRichTextEditor(KPIMTextEdit::RichTextComposer *richTextEditor);

    Q_REQUIRED_RESULT QHash<MessageComposer::PluginActionType::Type, QList<QAction *> > actionsType();
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
    QHash<MessageComposer::PluginActionType::Type, QList<QAction *> > mActionHash;
    KPIMTextEdit::RichTextComposer *mRichTextEditor = nullptr;
    QWidget *mParentWidget = nullptr;
    KActionCollection *mActionCollection = nullptr;
    QList<QWidget *> mStatusBarWidget;
};

#endif // KMAILPLUGINEDITORCONVERTTEXTMANAGERINTERFACE_H
