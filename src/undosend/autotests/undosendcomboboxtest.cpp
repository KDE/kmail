/*
   SPDX-FileCopyrightText: 2019-2021 Laurent Montel <montel@kde.org>

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
    QCOMPARE(w.count(), 5);
}
