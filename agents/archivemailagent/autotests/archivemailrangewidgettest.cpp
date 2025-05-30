/*
   SPDX-FileCopyrightText: 2012-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "archivemailrangewidgettest.h"
#include "../archivemailrangewidget.h"
#include "../widgets/hourcombobox.h"
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

    auto mRangeEnabled = w.findChild<QCheckBox *>(QStringLiteral("mRangeEnabled"));
    QVERIFY(mRangeEnabled);
    QVERIFY(!mRangeEnabled->isChecked());

    auto mStartRange = w.findChild<HourComboBox *>(QStringLiteral("mStartRange"));
    QVERIFY(mStartRange);

    auto mEndRange = w.findChild<HourComboBox *>(QStringLiteral("mEndRange"));
    QVERIFY(mEndRange);

    QVERIFY(!w.isRangeEnabled());
}

#include "moc_archivemailrangewidgettest.cpp"
