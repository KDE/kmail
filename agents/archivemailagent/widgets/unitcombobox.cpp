/*
   Copyright (C) 2015-2017 Montel Laurent <montel@kde.org>

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

#include "unitcombobox.h"
#include <KLocalizedString>

UnitComboBox::UnitComboBox(QWidget *parent)
    : QComboBox(parent)
{
    addItem(i18n("Days"), (int)ArchiveMailInfo::ArchiveDays);
    addItem(i18n("Weeks"), (int)ArchiveMailInfo::ArchiveWeeks);
    addItem(i18n("Months"), (int)ArchiveMailInfo::ArchiveMonths);
    addItem(i18n("Years"), (int)ArchiveMailInfo::ArchiveYears);
}

UnitComboBox::~UnitComboBox()
{
}

void UnitComboBox::setUnit(ArchiveMailInfo::ArchiveUnit unit)
{
    const int index = findData((int)unit);
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
