/*
  Copyright (c) 2014 Montel Laurent <montel@kde.org>

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

#include "overwritemodewidgettest.h"
#include "../widgets/overwritemodewidget.h"
#include <qtest.h>
#include <qtestmouse.h>
#include <QSignalSpy>

OverwriteModeWidgetTest::OverwriteModeWidgetTest(QObject *parent)
    : QObject(parent)
{

}

OverwriteModeWidgetTest::~OverwriteModeWidgetTest()
{

}

void OverwriteModeWidgetTest::shouldHasDefaultValue()
{
    OverwriteModeWidget widget;
    QVERIFY(!widget.overwriteMode());
}

void OverwriteModeWidgetTest::shouldChangeState()
{
    OverwriteModeWidget widget;
    widget.setOverwriteMode(true);
    QVERIFY(widget.overwriteMode());

    widget.setOverwriteMode(true);
    QVERIFY(widget.overwriteMode());

    widget.setOverwriteMode(false);
    QVERIFY(!widget.overwriteMode());
}

void OverwriteModeWidgetTest::shouldEmitSignalWhenChangeState()
{
    OverwriteModeWidget widget;
    QSignalSpy spy(&widget, SIGNAL(overwriteModeChanged(bool)));
    widget.setOverwriteMode(true);
    QCOMPARE(spy.count(), 1);

    widget.setOverwriteMode(false);
    QCOMPARE(spy.count(), 2);
}

void OverwriteModeWidgetTest::shouldNotEmitSignalWhenWeDontChangeState()
{
    OverwriteModeWidget widget;
    QSignalSpy spy(&widget, SIGNAL(overwriteModeChanged(bool)));
    widget.setOverwriteMode(false);
    QCOMPARE(spy.count(), 0);

    widget.setOverwriteMode(true);
    QCOMPARE(spy.count(), 1);

    widget.setOverwriteMode(true);
    QCOMPARE(spy.count(), 1);
}

void OverwriteModeWidgetTest::shouldEmitSignalWhenClickOnLabel()
{
    OverwriteModeWidget widget;
    QSignalSpy spy(&widget, SIGNAL(overwriteModeChanged(bool)));
    widget.show();
    QTest::qWaitForWindowShown(&widget);
    QTest::mouseClick(&widget, Qt::LeftButton);
    QCOMPARE(spy.count(), 1);

    QTest::mouseClick(&widget, Qt::LeftButton);
    QCOMPARE(spy.count(), 2);

}



QTEST_MAIN(OverwriteModeWidgetTest)
