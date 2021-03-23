/*
   SPDX-FileCopyrightText: 2018-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <PimCommon/PurposeMenuWidget>
namespace KPIMTextEdit
{
class PlainTextEditor;
}
class MailfilterPurposeMenuWidget : public PimCommon::PurposeMenuWidget
{
    Q_OBJECT
public:
    explicit MailfilterPurposeMenuWidget(QWidget *parentWidget, QObject *parent = nullptr);
    ~MailfilterPurposeMenuWidget() override;

    Q_REQUIRED_RESULT QByteArray text() override;
    void setEditorWidget(KPIMTextEdit::PlainTextEditor *editor);

private:
    KPIMTextEdit::PlainTextEditor *mEditor = nullptr;
};

