/*
  SPDX-FileCopyrightText: 2024 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "activitiesmanagertest.h"
#include "activities/activitiesmanager.h"
#include <QTest>
QTEST_MAIN(ActivitiesManagerTest)
ActivitiesManagerTest::ActivitiesManagerTest(QObject *parent)
    : QObject{parent}
{
}

void ActivitiesManagerTest::shouldHaveDefaultValues()
{
    ActivitiesManager w;
    QVERIFY(!w.enabled());
    QVERIFY(w.transportActivities());
    QVERIFY(w.identityActivities());
    QVERIFY(w.ldapActivities());
}

#include "moc_activitiesmanagertest.cpp"
