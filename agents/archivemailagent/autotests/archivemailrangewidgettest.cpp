/*
   SPDX-FileCopyrightText: 2012-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "archivemailrangewidgettest.h"
#include "../archivemailrangewidget.h"
#include "../widgets/hourcombobox.h"
#include <QCheckBox>
#include <QHBoxLayout>
#include <QTest>
using namespace Qt::Literals::StringLiterals;

QTEST_MAIN(ArchiveMailRangeWidgetTest)

ArchiveMailRangeWidgetTest::ArchiveMailRangeWidgetTest(QObject *parent)
    : QObject{parent}
{
}

void ArchiveMailRangeWidgetTest::shouldHaveDefaultValues()
{
    ArchiveMailRangeWidget w;

    auto mainLayout = w.findChild<QHBoxLayout *>(u"mainLayout"_s);
    QVERIFY(mainLayout);
    QCOMPARE(mainLayout->contentsMargins(), QMargins{});

    auto mRangeEnabled = w.findChild<QCheckBox *>(u"mRangeEnabled"_s);
    QVERIFY(mRangeEnabled);
    QVERIFY(!mRangeEnabled->isChecked());

    auto mStartRange = w.findChild<HourComboBox *>(u"mStartRange"_s);
    QVERIFY(mStartRange);

    auto mEndRange = w.findChild<HourComboBox *>(u"mEndRange"_s);
    QVERIFY(mEndRange);

    QVERIFY(!w.isRangeEnabled());
}

#include "moc_archivemailrangewidgettest.cpp"
