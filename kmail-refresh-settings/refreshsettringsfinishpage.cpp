/*
   SPDX-FileCopyrightText: 2019-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "refreshsettringsfinishpage.h"
#include <QHBoxLayout>
#include <QPlainTextEdit>

RefreshSettringsFinishPage::RefreshSettringsFinishPage(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setObjectName(QStringLiteral("mainLayout"));
    mainLayout->setContentsMargins({});

    mTextEdit = new QPlainTextEdit(this);
    mTextEdit->setObjectName(QStringLiteral("textedit"));
    mTextEdit->setReadOnly(true);
    mainLayout->addWidget(mTextEdit);
    connect(this, &RefreshSettringsFinishPage::cleanDoneInfo, this, &RefreshSettringsFinishPage::slotCleanDoneInfo);
}

RefreshSettringsFinishPage::~RefreshSettringsFinishPage()
{
}

void RefreshSettringsFinishPage::slotCleanDoneInfo(const QString &str)
{
    mTextEdit->insertPlainText(str + QLatin1Char('\n'));
}
