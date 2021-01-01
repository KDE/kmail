/*
   SPDX-FileCopyrightText: 2020-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "createforwardmessagejobtest.h"
#include "job/createforwardmessagejob.h"
#include <QTest>
QTEST_MAIN(CreateForwardMessageJobTest)
CreateForwardMessageJobTest::CreateForwardMessageJobTest(QObject *parent)
    : QObject(parent)
{
}

void CreateForwardMessageJobTest::shouldHaveDefaultValues()
{
    CreateForwardMessageJobSettings settings;
    QVERIFY(settings.url.isEmpty());
    QVERIFY(settings.selection.isEmpty());
    QVERIFY(settings.templateStr.isEmpty());
    QVERIFY(!settings.msg);
    QCOMPARE(settings.identity, 0);
}
