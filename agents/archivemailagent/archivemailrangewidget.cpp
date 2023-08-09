/*
   SPDX-FileCopyrightText: 2012-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "archivemailrangewidget.h"
#include <KLocalizedString>
#include <KTimeComboBox>
#include <QHBoxLayout>
#include <QLabel>

ArchiveMailRangeWidget::ArchiveMailRangeWidget(QWidget *parent)
    : QWidget{parent}
    , mStartRange(new KTimeComboBox(this))
    , mEndRange(new KTimeComboBox(this))
{
    auto mainLayout = new QHBoxLayout(this);
    mainLayout->setObjectName(QStringLiteral("mainLayout"));
    mainLayout->setContentsMargins({});
    mainLayout->addWidget(new QLabel(i18n("Range:"), this));

    mStartRange->setObjectName(QStringLiteral("mStartRange"));
    mStartRange->setTimeListInterval(60);

    mEndRange->setObjectName(QStringLiteral("mEndRange"));
    mEndRange->setTimeListInterval(60);

    mainLayout->addWidget(mStartRange);
    mainLayout->addWidget(mEndRange);
}

ArchiveMailRangeWidget::~ArchiveMailRangeWidget() = default;
