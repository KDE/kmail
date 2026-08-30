/*
   SPDX-FileCopyrightText: 2023-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "commandlineinfotest.h"
#include "commandlineinfo.h"
#include <QTest>
using namespace Qt::Literals::StringLiterals;

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
        args << u"kmail"_s;
        QTest::newRow("empty") << args << QString() << CommandLineInfo();
    }
    {
        QStringList args;
        args << u"kmail"_s;
        args << u"mailto:rostedt@goodmis.org?In-Reply-To=%3C20231105160139.660634360@goodmis.org%3E&Cc=akaher%40vmware.com%2Cakpm%40linux-foundation.org%"
                u"2Cgregkh%"
                "40linuxfoundation.org%2Clinux-kernel%40vger.kernel.org%2Cmark.rutland%40arm.com%2Cmhiramat%40kernel.org%2Cstable%40vger.kernel.org&Subject=Re%"
                "3A%"
                "20%5Bv6.6%5D%5BPATCH%203%2F5%5D%20eventfs%3A%20Save%20ownership%20and%20mode"_s;
        CommandLineInfo info;
        info.setSubject(u"Re: [v6.6][PATCH 3/5] eventfs: Save ownership and mode"_s);
        info.setTo(u"rostedt@goodmis.org"_s);
        info.setCc(
            u"akaher@vmware.com,akpm@linux-foundation.org,gregkh@linuxfoundation.org,linux-kernel@vger.kernel.org,mark.rutland@arm.com,mhiramat@"
            "kernel.org,stable@vger.kernel.org, "_s);
        info.setInReplyTo(u"<20231105160139.660634360@goodmis.org>"_s);
        info.setMailto(true);
        QTest::newRow("test1") << args << QString() << info;
    }
    {
        QStringList args;
        args << u"kmail"_s;
        args << u"mailto:person@example.com?body=one&two=three"_s;
        CommandLineInfo info;
        info.setTo(u"person@example.com"_s);
        info.setBody(u"one&two=three"_s);
        info.setMailto(true);
        QTest::newRow("mailto body keeps unknown fragments") << args << QString() << info;
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
