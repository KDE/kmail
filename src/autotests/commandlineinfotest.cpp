/*
   SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "commandlineinfotest.h"
#include "commandlineinfo.h"
#include <QTest>

QTEST_GUILESS_MAIN(CommandLineInfoTest)
CommandLineInfoTest::CommandLineInfoTest(QObject *parent)
    : QObject{parent}
{
}

void CommandLineInfoTest::shouldHaveDefaultValues()
{
    CommandLineInfo w;
    QVERIFY(w.customHeaders().isEmpty());
    QVERIFY(w.attachURLs().isEmpty());
    QVERIFY(w.to().isEmpty());
    QVERIFY(w.cc().isEmpty());
    QVERIFY(w.bcc().isEmpty());
    QVERIFY(w.subject().isEmpty());
    QVERIFY(w.body().isEmpty());
    QVERIFY(w.inReplyTo().isEmpty());
    QVERIFY(w.replyTo().isEmpty());
    QVERIFY(w.identity().isEmpty());
    QVERIFY(w.messageFile().isEmpty());
    QVERIFY(!w.startInTray());
    QVERIFY(!w.mailto());
    QVERIFY(!w.checkMail());
    QVERIFY(!w.viewOnly());
    QVERIFY(!w.calledWithSession());
}

void CommandLineInfoTest::parseCommandLineInfo_data()
{
    QTest::addColumn<QStringList>("args");
    QTest::addColumn<QString>("workingDir");
    QTest::addColumn<CommandLineInfo>("output");
    QStringList args;
    args << QStringLiteral("kmail");
    QTest::newRow("empty") << args << QString() << CommandLineInfo();
}

void CommandLineInfoTest::parseCommandLineInfo()
{
    QFETCH(QStringList, args);
    QFETCH(QString, workingDir);
    QFETCH(CommandLineInfo, output);
    CommandLineInfo input;
    input.parseCommandLine(args, workingDir);
    QCOMPARE(input, output);
}
