/*
   SPDX-FileCopyrightText: 2017-2024 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>
namespace KPIMTextEdit
{
class RichTextComposer;
}

class KMailPluginEditorInitManagerInterface : public QObject
{
    Q_OBJECT
public:
    explicit KMailPluginEditorInitManagerInterface(QObject *parent = nullptr);
    ~KMailPluginEditorInitManagerInterface() override = default;

    [[nodiscard]] KPIMTextEdit::RichTextComposer *richTextEditor() const;
    void setRichTextEditor(KPIMTextEdit::RichTextComposer *richTextEditor);

    [[nodiscard]] QWidget *parentWidget() const;
    void setParentWidget(QWidget *parentWidget);

    void initializePlugins();

private:
    KPIMTextEdit::RichTextComposer *mRichTextEditor = nullptr;
    QWidget *mParentWidget = nullptr;
    bool mWasInitialized = false;
};
