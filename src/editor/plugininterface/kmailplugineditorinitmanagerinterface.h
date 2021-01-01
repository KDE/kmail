/*
   SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KMAILPLUGINEDITORINITMANAGERINTERFACE_H
#define KMAILPLUGINEDITORINITMANAGERINTERFACE_H

#include <QObject>
namespace KPIMTextEdit {
class RichTextComposer;
}

class KMailPluginEditorInitManagerInterface : public QObject
{
    Q_OBJECT
public:
    explicit KMailPluginEditorInitManagerInterface(QObject *parent = nullptr);
    ~KMailPluginEditorInitManagerInterface() = default;

    KPIMTextEdit::RichTextComposer *richTextEditor() const;
    void setRichTextEditor(KPIMTextEdit::RichTextComposer *richTextEditor);

    QWidget *parentWidget() const;
    void setParentWidget(QWidget *parentWidget);

    void initializePlugins();
private:
    Q_DISABLE_COPY(KMailPluginEditorInitManagerInterface)
    KPIMTextEdit::RichTextComposer *mRichTextEditor = nullptr;
    QWidget *mParentWidget = nullptr;
    bool mWasInitialized = false;
};

#endif // KMAILPLUGINEDITORINITMANAGERINTERFACE_H
