/*
   SPDX-FileCopyrightText: 2019-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "refreshsettringsfinishpage.h"

#include <QHBoxLayout>
#include <QPlainTextEdit>

using namespace Qt::Literals::StringLiterals;
RefreshSettringsFinishPage::RefreshSettringsFinishPage(QWidget *parent)
    : QWidget(parent)
    , mTextEdit(new QPlainTextEdit(this))
{
    auto mainLayout = new QHBoxLayout(this);
    mainLayout->setObjectName("mainLayout"_L1);
    mainLayout->setContentsMargins({});

    mTextEdit->setObjectName("textedit"_L1);
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
