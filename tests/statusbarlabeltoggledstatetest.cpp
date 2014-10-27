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

#include "statusbarlabeltoggledstatetest.h"
#include "../widgets/statusbarlabeltoggledstate.h"
#include <qtest_kde.h>
#include <qtestmouse.h>
#include <QSignalSpy>

StatusBarLabelToggledStateTest::StatusBarLabelToggledStateTest(QObject *parent)
    : QObject(parent)
{

}

StatusBarLabelToggledStateTest::~StatusBarLabelToggledStateTest()
{

}

void StatusBarLabelToggledStateTest::shouldHasDefaultValue()
{
    StatusBarLabelToggledState widget;
    QVERIFY(!widget.overwriteMode());
}

void StatusBarLabelToggledStateTest::shouldChangeState()
{
    StatusBarLabelToggledState widget;
    widget.setOverwriteMode(true);
    QVERIFY(widget.overwriteMode());
    QVERIFY(!widget.text().isEmpty());

    widget.setOverwriteMode(true);
    QVERIFY(widget.overwriteMode());

    widget.setOverwriteMode(false);
    QVERIFY(!widget.overwriteMode());
}

void StatusBarLabelToggledStateTest::shouldEmitSignalWhenChangeState()
{
    StatusBarLabelToggledState widget;
    QSignalSpy spy(&widget, SIGNAL(overwriteModeChanged(bool)));
    widget.setOverwriteMode(true);
    QCOMPARE(spy.count(), 1);

    widget.setOverwriteMode(false);
    QCOMPARE(spy.count(), 2);
}

void StatusBarLabelToggledStateTest::shouldNotEmitSignalWhenWeDontChangeState()
{
    StatusBarLabelToggledState widget;
    QSignalSpy spy(&widget, SIGNAL(overwriteModeChanged(bool)));
    widget.setOverwriteMode(false);
    QCOMPARE(spy.count(), 0);

    widget.setOverwriteMode(true);
    QCOMPARE(spy.count(), 1);

    widget.setOverwriteMode(true);
    QCOMPARE(spy.count(), 1);
}

void StatusBarLabelToggledStateTest::shouldEmitSignalWhenClickOnLabel()
{
    StatusBarLabelToggledState widget;
    QSignalSpy spy(&widget, SIGNAL(overwriteModeChanged(bool)));
    widget.show();
    QTest::qWaitForWindowShown(&widget);
    QTest::mouseClick(&widget, Qt::LeftButton);
    QCOMPARE(spy.count(), 1);

    QTest::mouseClick(&widget, Qt::LeftButton);
    QCOMPARE(spy.count(), 2);

}

void StatusBarLabelToggledStateTest::shouldChangeTestWhenStateChanged()
{
    StatusBarLabelToggledState widget;
    const QString initialText = widget.text();
    widget.setOverwriteMode(true);
    const QString newText = widget.text();
    QVERIFY(initialText!=newText);

    widget.setOverwriteMode(false);
    QCOMPARE(widget.text(), initialText);

    widget.setOverwriteMode(true);
    QCOMPARE(widget.text(), newText);
}



QTEST_KDEMAIN(StatusBarLabelToggledStateTest, GUI)
