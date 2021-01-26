/*
   SPDX-FileCopyrightText: 2018-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "mailfilterpurposemenuwidget.h"
#include <KPIMTextEdit/PlainTextEditor>

MailfilterPurposeMenuWidget::MailfilterPurposeMenuWidget(QWidget *parentWidget, QObject *parent)
    : PimCommon::PurposeMenuWidget(parentWidget, parent)
{
}

MailfilterPurposeMenuWidget::~MailfilterPurposeMenuWidget() = default;

QByteArray MailfilterPurposeMenuWidget::text()
{
    if (mEditor) {
        return mEditor->toPlainText().toUtf8();
    }
    return {};
}

void MailfilterPurposeMenuWidget::setEditorWidget(KPIMTextEdit::PlainTextEditor *editor)
{
    mEditor = editor;
}
