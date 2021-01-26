/*
   SPDX-FileCopyrightText: 2019-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "refreshsettingsfirstpage.h"
#include <KLocalizedString>
#include <QHBoxLayout>
#include <QLabel>

RefreshSettingsFirstPage::RefreshSettingsFirstPage(QWidget *parent)
    : QWidget(parent)
{
    auto mainLayout = new QHBoxLayout(this);
    mainLayout->setObjectName(QStringLiteral("mainLayout"));
    mainLayout->setContentsMargins({});
    auto label = new QLabel(i18n("Please close KMail/Kontact before using it."));
    QFont f = label->font();
    f.setBold(true);
    f.setPointSize(22);
    label->setFont(f);
    label->setObjectName(QStringLiteral("label"));
    mainLayout->addWidget(label, 0, Qt::AlignHCenter);
}

RefreshSettingsFirstPage::~RefreshSettingsFirstPage() = default;
