/*
   SPDX-FileCopyrightText: 2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mailmergeconfigurewidget.h"
#include <QVBoxLayout>

MailMergeConfigureWidget::MailMergeConfigureWidget(QWidget *parent)
    : QWidget(parent)
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setObjectName(QStringLiteral("mainLayout"));
}

MailMergeConfigureWidget::~MailMergeConfigureWidget()
{
}
