/*
   SPDX-FileCopyrightText: 2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mailmergeconfigurewidget.h"
#include "ui_mailmergeconfigurewidget.h"
#include <QVBoxLayout>

MailMergeConfigureWidget::MailMergeConfigureWidget(QWidget *parent)
    : QWidget(parent)
{
    mWidget = new Ui::MailMergeConfigureWidget;
    mWidget->setupUi(this);
}

MailMergeConfigureWidget::~MailMergeConfigureWidget()
{
}
