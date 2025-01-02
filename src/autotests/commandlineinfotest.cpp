/*
   SPDX-FileCopyrightText: 2023-2025 Laurent Montel <montel@kde.org>

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
    QVERIFY(!w.htmlBody());
}

void CommandLineInfoTest::parseCommandLineInfo_data()
{
    QTest::addColumn<QStringList>("args");
    QTest::addColumn<QString>("workingDir");
    QTest::addColumn<CommandLineInfo>("output");
    {
        QStringList args;
        args << QStringLiteral("kmail");
        QTest::newRow("empty") << args << QString() << CommandLineInfo();
    }
    {
        QStringList args;
        args << QStringLiteral("kmail");
        args << QStringLiteral(
            "mailto:rostedt@goodmis.org?In-Reply-To=%3C20231105160139.660634360@goodmis.org%3E&Cc=akaher%40vmware.com%2Cakpm%40linux-foundation.org%2Cgregkh%"
            "40linuxfoundation.org%2Clinux-kernel%40vger.kernel.org%2Cmark.rutland%40arm.com%2Cmhiramat%40kernel.org%2Cstable%40vger.kernel.org&Subject=Re%3A%"
            "20%5Bv6.6%5D%5BPATCH%203%2F5%5D%20eventfs%3A%20Save%20ownership%20and%20mode");
        CommandLineInfo info;
        info.setSubject(QStringLiteral("Re: [v6.6][PATCH 3/5] eventfs: Save ownership and mode"));
        info.setTo(QStringLiteral("rostedt@goodmis.org"));
        info.setCc(
            QStringLiteral("akaher@vmware.com,akpm@linux-foundation.org,gregkh@linuxfoundation.org,linux-kernel@vger.kernel.org,mark.rutland@arm.com,mhiramat@"
                           "kernel.org,stable@vger.kernel.org, "));
        info.setInReplyTo(QStringLiteral("<20231105160139.660634360@goodmis.org>"));
        info.setMailto(true);
        QTest::newRow("test1") << args << QString() << info;
    }
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

#include "moc_commandlineinfotest.cpp"
