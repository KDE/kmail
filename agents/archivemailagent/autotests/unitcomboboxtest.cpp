/*
   SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "unitcomboboxtest.h"
#include "../widgets/unitcombobox.h"
#include <QTest>

UnitComboBoxTest::UnitComboBoxTest(QObject *parent)
    : QObject(parent)
{
}

UnitComboBoxTest::~UnitComboBoxTest() = default;

void UnitComboBoxTest::shouldHaveDefaultValue()
{
    UnitComboBox combo;
    QCOMPARE(combo.count(), 4);
}

void UnitComboBoxTest::changeCurrentItem_data()
{
    QTest::addColumn<int>("input");
    QTest::addColumn<int>("output");
    QTest::newRow("first") << 0 << 0;
    QTest::newRow("second") << 1 << 1;
    QTest::newRow("third") << 2 << 2;
    QTest::newRow("fourth") << 3 << 3;
    QTest::newRow("invalid") << 5 << 0;
}

void UnitComboBoxTest::changeCurrentItem()
{
    QFETCH(int, input);
    QFETCH(int, output);
    UnitComboBox combo;
    combo.setUnit(static_cast<ArchiveMailInfo::ArchiveUnit>(input));
    QCOMPARE(combo.unit(), static_cast<ArchiveMailInfo::ArchiveUnit>(output));
}

QTEST_MAIN(UnitComboBoxTest)
