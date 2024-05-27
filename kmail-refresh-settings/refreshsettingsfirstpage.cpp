/*
   SPDX-FileCopyrightText: 2019-2024 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "refreshsettingsfirstpage.h"
using namespace Qt::Literals::StringLiterals;

#include <KLocalizedString>
#include <QHBoxLayout>
#include <QLabel>

RefreshSettingsFirstPage::RefreshSettingsFirstPage(QWidget *parent)
    : QWidget(parent)
{
    auto mainLayout = new QHBoxLayout(this);
    mainLayout->setObjectName("mainLayout"_L1);
    mainLayout->setContentsMargins({});
    auto label = new QLabel(i18nc("@label:textbox", "Please close KMail/Kontact before using it."));
    QFont f = label->font();
    f.setBold(true);
    f.setPixelSize(22);
    label->setFont(f);
    label->setObjectName("label"_L1);
    mainLayout->addWidget(label, 0, Qt::AlignHCenter);
}

RefreshSettingsFirstPage::~RefreshSettingsFirstPage() = default;

#include "moc_refreshsettingsfirstpage.cpp"
