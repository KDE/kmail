/*
   Copyright (C) 2019 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "refreshsettingsfirstpage.h"
#include <QLabel>
#include <QHBoxLayout>
#include <KLocalizedString>

RefreshSettingsFirstPage::RefreshSettingsFirstPage(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setObjectName(QStringLiteral("mainLayout"));
    mainLayout->setContentsMargins(0, 0, 0, 0);
    QLabel *label = new QLabel(i18n("Please close KMail/Kontact before using it."));
    QFont f = label->font();
    f.setBold(true);
    f.setPointSize(22);
    label->setFont(f);
    label->setObjectName(QStringLiteral("label"));
    mainLayout->addWidget(label, 0, Qt::AlignHCenter);

}

RefreshSettingsFirstPage::~RefreshSettingsFirstPage()
{
}
