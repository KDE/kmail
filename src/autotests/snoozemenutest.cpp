/*
  SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozemenutest.h"
#include "snooze/snoozemenu.h"
#include <QMenu>
#include <QSignalSpy>
#include <QTest>
QTEST_MAIN(SnoozeMenuTest)

SnoozeMenuTest::SnoozeMenuTest(QObject *parent)
    : QObject(parent)
{
}

void SnoozeMenuTest::shouldHaveDefaultValue()
{
    SnoozeMenu menu;
    QVERIFY(!menu.text().isEmpty());
    QCOMPARE(menu.menu()->actions().count(), 5);
}

void SnoozeMenuTest::shouldEmitSnoozeRequested()
{
    SnoozeMenu menu;
    QSignalSpy spy(&menu, &SnoozeMenu::snoozeRequested);
    menu.menu()->actions().at(2)->trigger();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).value<SnoozeMenu::SnoozeType>(), SnoozeMenu::SnoozeType::Tomorrow);
}

void SnoozeMenuTest::shouldComputeWakeUpDateTime_data()
{
    QTest::addColumn<SnoozeMenu::SnoozeType>("type");
    QTest::addColumn<QDateTime>("now");
    QTest::addColumn<QDateTime>("expected");

    // 2026-10-29 is a Thursday, 2026-11-02 a Monday.
    const QDateTime thursday(QDate(2026, 10, 29), QTime(14, 30), QTimeZone::UTC);
    const QDateTime monday(QDate(2026, 11, 2), QTime(7, 0), QTimeZone::UTC);
    QTest::newRow("unknown") << SnoozeMenu::SnoozeType::Unknown << thursday << QDateTime();
    QTest::newRow("custom") << SnoozeMenu::SnoozeType::Custom << thursday << QDateTime();
    QTest::newRow("onehour") << SnoozeMenu::SnoozeType::OneHourLater << thursday << QDateTime(QDate(2026, 10, 29), QTime(15, 30), QTimeZone::UTC);
    QTest::newRow("eighthours") << SnoozeMenu::SnoozeType::EightHoursLater << thursday << QDateTime(QDate(2026, 10, 29), QTime(22, 30), QTimeZone::UTC);
    QTest::newRow("tomorrow") << SnoozeMenu::SnoozeType::Tomorrow << thursday << QDateTime(QDate(2026, 10, 30), QTime(8, 0), QTimeZone::UTC);
    QTest::newRow("nextweek") << SnoozeMenu::SnoozeType::NextWeek << thursday << QDateTime(QDate(2026, 11, 2), QTime(8, 0), QTimeZone::UTC);
    QTest::newRow("nextweek-from-monday") << SnoozeMenu::SnoozeType::NextWeek << monday << QDateTime(QDate(2026, 11, 9), QTime(8, 0), QTimeZone::UTC);
}

void SnoozeMenuTest::shouldComputeWakeUpDateTime()
{
    QFETCH(SnoozeMenu::SnoozeType, type);
    QFETCH(QDateTime, now);
    QFETCH(QDateTime, expected);
    QCOMPARE(SnoozeMenu::wakeUpDateTime(type, now), expected);
}

#include "moc_snoozemenutest.cpp"
