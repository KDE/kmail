/*
   SPDX-FileCopyrightText: 2018-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <PimCommon/PurposeMenuWidget>
namespace TextCustomEditor
{
class PlainTextEditor;
}
class MailfilterPurposeMenuWidget : public PimCommon::PurposeMenuWidget
{
    Q_OBJECT
public:
    explicit MailfilterPurposeMenuWidget(QWidget *parentWidget, QObject *parent = nullptr);
    ~MailfilterPurposeMenuWidget() override;

    [[nodiscard]] QByteArray text() override;
    void setEditorWidget(TextCustomEditor::PlainTextEditor *editor);

private:
    TextCustomEditor::PlainTextEditor *mEditor = nullptr;
};
