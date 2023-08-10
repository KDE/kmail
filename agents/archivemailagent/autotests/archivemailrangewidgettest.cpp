/*
   SPDX-FileCopyrightText: 2012-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "archivemailrangewidgettest.h"
#include "../archivemailrangewidget.h"
#include "hourcombobox.h"
#include <QCheckBox>
#include <QHBoxLayout>
#include <QTest>
QTEST_MAIN(ArchiveMailRangeWidgetTest)

ArchiveMailRangeWidgetTest::ArchiveMailRangeWidgetTest(QObject *parent)
    : QObject{parent}
{
}

void ArchiveMailRangeWidgetTest::shouldHaveDefaultValues()
{
    ArchiveMailRangeWidget w;

    auto mainLayout = w.findChild<QHBoxLayout *>(QStringLiteral("mainLayout"));
    QVERIFY(mainLayout);
    QCOMPARE(mainLayout->contentsMargins(), QMargins{});

    auto mEnabled = w.findChild<QCheckBox *>(QStringLiteral("mEnabled"));
    QVERIFY(mEnabled);
    QVERIFY(!mEnabled->isChecked());

    auto mStartRange = w.findChild<HourComboBox *>(QStringLiteral("mStartRange"));
    QVERIFY(mStartRange);

    auto mEndRange = w.findChild<HourComboBox *>(QStringLiteral("mEndRange"));
    QVERIFY(mEndRange);
}

#include "moc_archivemailrangewidgettest.cpp"
