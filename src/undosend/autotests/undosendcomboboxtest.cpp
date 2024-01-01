/*
   SPDX-FileCopyrightText: 2019-2024 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "undosendcomboboxtest.h"
#include "undosend/undosendcombobox.h"
#include <QTest>
QTEST_MAIN(UndoSendComboboxTest)

UndoSendComboboxTest::UndoSendComboboxTest(QObject *parent)
    : QObject(parent)
{
}

void UndoSendComboboxTest::shouldHaveDefaultValues()
{
    UndoSendCombobox w;
    QCOMPARE(w.count(), 6);
}

#include "moc_undosendcomboboxtest.cpp"
