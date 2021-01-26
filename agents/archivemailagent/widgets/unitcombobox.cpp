/*
   SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "unitcombobox.h"
#include <KLocalizedString>

UnitComboBox::UnitComboBox(QWidget *parent)
    : QComboBox(parent)
{
    addItem(i18n("Days"), static_cast<int>(ArchiveMailInfo::ArchiveDays));
    addItem(i18n("Weeks"), static_cast<int>(ArchiveMailInfo::ArchiveWeeks));
    addItem(i18n("Months"), static_cast<int>(ArchiveMailInfo::ArchiveMonths));
    addItem(i18n("Years"), static_cast<int>(ArchiveMailInfo::ArchiveYears));
}

UnitComboBox::~UnitComboBox() = default;

void UnitComboBox::setUnit(ArchiveMailInfo::ArchiveUnit unit)
{
    const int index = findData(static_cast<int>(unit));
    if (index != -1) {
        setCurrentIndex(index);
    } else {
        setCurrentIndex(0);
    }
}

ArchiveMailInfo::ArchiveUnit UnitComboBox::unit() const
{
    return static_cast<ArchiveMailInfo::ArchiveUnit>(itemData(currentIndex()).toInt());
}
