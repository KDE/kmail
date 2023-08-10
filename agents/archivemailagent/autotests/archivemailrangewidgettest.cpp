/*
   SPDX-FileCopyrightText: 2012-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "archivemailrangewidgettest.h"
#include "../archivemailrangewidget.h"
#include <KTimeComboBox>
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

    auto mStartRange = w.findChild<KTimeComboBox *>(QStringLiteral("mStartRange"));
    QVERIFY(mStartRange);
    QCOMPARE(mStartRange->timeListInterval(), 60);

    auto mEndRange = w.findChild<KTimeComboBox *>(QStringLiteral("mEndRange"));
    QVERIFY(mEndRange);
    QCOMPARE(mEndRange->timeListInterval(), 60);
}

#include "moc_archivemailrangewidgettest.cpp"
