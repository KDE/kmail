/*
   SPDX-FileCopyrightText: 2019-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "undosendcombobox.h"
#include <KLocalizedString>

UndoSendCombobox::UndoSendCombobox(QWidget *parent)
    : QComboBox(parent)
{
    initialize();
}

UndoSendCombobox::~UndoSendCombobox() = default;

void UndoSendCombobox::initialize()
{
    for (int i = 1; i < 6; i++) {
        const int numberOfSeconds = i * 10;
        addItem(i18n("%1 seconds", numberOfSeconds), numberOfSeconds);
    }
    constexpr int fivemutes = 5 * 60;
    addItem(i18n("5 minutes"), fivemutes);
}

int UndoSendCombobox::delay() const
{
    return currentData().toInt();
}

void UndoSendCombobox::setDelay(int val)
{
    const int index = findData(val);
    setCurrentIndex(index != -1 ? index : 0);
}

#include "moc_undosendcombobox.cpp"
