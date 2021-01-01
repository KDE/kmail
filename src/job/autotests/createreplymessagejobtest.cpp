/*
   SPDX-FileCopyrightText: 2020-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "createreplymessagejobtest.h"
#include "job/createreplymessagejob.h"
#include <QTest>
QTEST_MAIN(CreateReplyMessageJobTest)

CreateReplyMessageJobTest::CreateReplyMessageJobTest(QObject *parent)
    : QObject(parent)
{
}

void CreateReplyMessageJobTest::shouldHaveDefaultValue()
{
    CreateReplyMessageJobSettings settings;
    QVERIFY(settings.url.isEmpty());
    QVERIFY(settings.selection.isEmpty());
    QVERIFY(settings.templateStr.isEmpty());
    QVERIFY(!settings.msg);
    QVERIFY(!settings.noQuote);
    QVERIFY(!settings.replyAsHtml);
    QCOMPARE(settings.replyStrategy, MessageComposer::ReplySmart);
}
