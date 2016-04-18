/*
  Copyright (c) 2015-2016 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "formatcomboboxtest.h"
#include <qtest.h>
#include "../widgets/formatcombobox.h"

FormatComboBoxTest::FormatComboBoxTest(QObject *parent)
    : QObject(parent)
{

}

FormatComboBoxTest::~FormatComboBoxTest()
{

}

void FormatComboBoxTest::shouldHaveDefaultValue()
{
    FormatComboBox combo;
    QCOMPARE(combo.count(), 4);
}

void FormatComboBoxTest::changeCurrentItem_data()
{
    QTest::addColumn<int>("input");
    QTest::addColumn<int>("output");
    QTest::newRow("first") <<  0 << 0;
    QTest::newRow("second") <<  1 << 1;
    QTest::newRow("third") <<  2 << 2;
    QTest::newRow("fourth") <<  3 << 3;
    QTest::newRow("invalid") <<  5 << 0;
}

void FormatComboBoxTest::changeCurrentItem()
{
    QFETCH(int, input);
    QFETCH(int, output);
    FormatComboBox combo;
    combo.setFormat(static_cast<MailCommon::BackupJob::ArchiveType>(input));
    QCOMPARE(combo.format(), static_cast<MailCommon::BackupJob::ArchiveType>(output));
}

QTEST_MAIN(FormatComboBoxTest)
