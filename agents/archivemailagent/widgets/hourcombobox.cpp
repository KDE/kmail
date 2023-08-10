
#include <QTime>
/*
   SPDX-FileCopyrightText: 2012-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "hourcombobox.h"

HourComboBox::HourComboBox(QWidget *parent)
    : QComboBox(parent)
{
    initializeList();
}

HourComboBox::~HourComboBox() = default;

void HourComboBox::initializeList()
{
    for (int i = 0; i < 24; ++i) {
        addItem(QTime(i, 0, 0).toString(), i);
    }
}

void HourComboBox::setHour(int hour)
{
    const int index = findData(hour);
    if (index != -1) {
        setCurrentIndex(index);
    }
}

int HourComboBox::hour() const
{
    return currentData().toInt();
}

#include "moc_hourcombobox.cpp"
