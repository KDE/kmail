/*
   SPDX-FileCopyrightText: 2019-2024 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "refreshsettringsfinishpage.h"
#include <QHBoxLayout>
#include <QPlainTextEdit>

RefreshSettringsFinishPage::RefreshSettringsFinishPage(QWidget *parent)
    : QWidget(parent)
    , mTextEdit(new QPlainTextEdit(this))
{
    auto mainLayout = new QHBoxLayout(this);
    mainLayout->setObjectName(QLatin1StringView("mainLayout"));
    mainLayout->setContentsMargins({});

    mTextEdit->setObjectName(QLatin1StringView("textedit"));
    mTextEdit->setReadOnly(true);
    mainLayout->addWidget(mTextEdit);
    connect(this, &RefreshSettringsFinishPage::cleanDoneInfo, this, &RefreshSettringsFinishPage::slotCleanDoneInfo);
}

RefreshSettringsFinishPage::~RefreshSettringsFinishPage() = default;

void RefreshSettringsFinishPage::slotCleanDoneInfo(const QString &str)
{
    mTextEdit->insertPlainText(str + QLatin1Char('\n'));
}

#include "moc_refreshsettringsfinishpage.cpp"
