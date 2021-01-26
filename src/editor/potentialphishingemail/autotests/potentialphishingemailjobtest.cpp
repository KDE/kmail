/*
  SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#include "potentialphishingemailjobtest.h"
#include "../potentialphishingemailjob.h"
#include <QSignalSpy>
#include <QStringList>
#include <QTest>

PotentialPhishingEmailJobTest::PotentialPhishingEmailJobTest(QObject *parent)
    : QObject(parent)
{
}

PotentialPhishingEmailJobTest::~PotentialPhishingEmailJobTest() = default;

void PotentialPhishingEmailJobTest::shouldNotStartIfNoEmails()
{
    auto job = new PotentialPhishingEmailJob;
    QVERIFY(!job->start());
    QVERIFY(job->potentialPhisingEmails().isEmpty());
}

void PotentialPhishingEmailJobTest::shouldReturnPotentialPhishingEmails_data()
{
    QTest::addColumn<QStringList>("listEmails");
    QTest::addColumn<QStringList>("whiteListEmail");
    QTest::addColumn<bool>("hasPotentialPhishing");
    QTest::newRow("NoPotentialPhishing") << (QStringList() << QStringLiteral("foo@kde.org")) << QStringList() << false;
    QTest::newRow("HasPotentialPhishing") << (QStringList() << QStringLiteral("\"bla@kde.org\" <foo@kde.org>")) << QStringList() << true;
    const QString email = QStringLiteral("\"bla@kde.org\" <foo@kde.org>");
    QTest::newRow("EmailInWhiteList") << (QStringList() << email) << (QStringList() << email) << false;
    QTest::newRow("NotAllEmailInWhiteList") << (QStringList() << email << QStringLiteral("\"c@kde.org\" <dd@kde.org>")) << (QStringList() << email) << true;
    QTest::newRow("EmailInWhiteListWithSpace") << (QStringList() << QStringLiteral(" \"bla@kde.org\" <foo@kde.org> ")) << (QStringList() << email) << false;
    QTest::newRow("EmailWithSameNameAndDisplayName") << (QStringList() << QStringLiteral("\"<foo@kde.com>\" <foo@kde.com>")) << (QStringList() << email)
                                                     << false;
    QTest::newRow("EmailWithSameNameAndDisplayNameWithSpace")
        << (QStringList() << QStringLiteral(" \"<foo@kde.com>\" <foo@kde.com> ")) << (QStringList() << email) << false;

    QTest::newRow("notsamecase") << (QStringList() << QStringLiteral("\"Foo@kde.org\" <foo@kde.org>")) << QStringList() << false;
    QTest::newRow("notsamecaseaddress") << (QStringList() << QStringLiteral("\"Foo@kde.org\" <FOO@kde.ORG>")) << QStringList() << false;

    QTest::newRow("emailinparenthese") << (QStringList() << QStringLiteral("\"bla (Foo@kde.org)\" <FOO@kde.ORG>")) << QStringList() << false;
    QTest::newRow("notemailinparenthese") << (QStringList() << QStringLiteral("\"bla (bli@kde.org)\" <FOO@kde.ORG>")) << QStringList() << true;
    QTest::newRow("erroremailinparenthese") << (QStringList() << QStringLiteral("\"bla Foo@kde.org\" <FOO@kde.ORG>")) << QStringList() << true;

    QTest::newRow("WithMultiSameEmail") << (QStringList() << QStringLiteral("\"foo@kde.org foo@kde.org\" <foo@kde.org>")) << QStringList() << false;
    QTest::newRow("WithMultiSameEmailWithSpace") << (QStringList() << QStringLiteral("\"  foo@kde.org   foo@kde.org  \" <foo@kde.org>")) << QStringList()
                                                 << false;
    QTest::newRow("WithMultiNotSameEmail") << (QStringList() << QStringLiteral("\"  bla@kde.org   foo@kde.org  \" <foo@kde.org>")) << QStringList() << true;

    QTest::newRow("EmailWithSimpleQuote") << (QStringList() << QStringLiteral("\"\'foo@kde.org\'\" <foo@kde.org>")) << QStringList() << false;

    QTest::newRow("BadCompletion") << (QStringList() << QStringLiteral("@kde.org <foo@kde.org>")) << QStringList() << false;
}

void PotentialPhishingEmailJobTest::shouldReturnPotentialPhishingEmails()
{
    QFETCH(QStringList, listEmails);
    QFETCH(QStringList, whiteListEmail);
    QFETCH(bool, hasPotentialPhishing);

    auto job = new PotentialPhishingEmailJob;
    job->setEmailWhiteList(whiteListEmail);
    job->setPotentialPhishingEmails(listEmails);
    QVERIFY(job->start());
    QCOMPARE(job->potentialPhisingEmails().isEmpty(), !hasPotentialPhishing);
}

void PotentialPhishingEmailJobTest::shouldEmitSignal()
{
    auto job = new PotentialPhishingEmailJob;
    QSignalSpy spy(job, &PotentialPhishingEmailJob::potentialPhishingEmailsFound);
    job->setPotentialPhishingEmails((QStringList() << QStringLiteral("\"bla@kde.org\" <foo@kde.org>")));
    QVERIFY(job->start());
    QCOMPARE(spy.count(), 1);
}

void PotentialPhishingEmailJobTest::shouldCreateCorrectListOfEmails_data()
{
    QTest::addColumn<QStringList>("emails");
    QTest::addColumn<QStringList>("createdListOfEmails");
    QTest::newRow("emptylist") << QStringList() << QStringList();
    QStringList emails{QStringLiteral("foo@kde.org"), QStringLiteral("bla@kde.org")};
    QStringList createdList{QStringLiteral("foo@kde.org"), QStringLiteral("bla@kde.org")};
    QTest::newRow("nonempty") << emails << createdList;
    emails = QStringList{QStringLiteral("\"bla\" <foo@kde.org>"), QStringLiteral("bla@kde.org")};
    QTest::newRow("potentialerrors") << emails << emails;

    emails = QStringList{QStringLiteral("\"bla, foo\" <foo@kde.org>"), QStringLiteral("bla@kde.org")};
    QTest::newRow("emailswithquote") << emails << emails;

    emails = QStringList{QStringLiteral("\"bla, foo\" <foo@kde.org>"), QStringLiteral("bla@kde.org"), QStringLiteral(" ")};
    createdList = QStringList{QStringLiteral("\"bla, foo\" <foo@kde.org>"), QStringLiteral("bla@kde.org")};
    QTest::newRow("emailswithemptystr") << emails << createdList;
}

void PotentialPhishingEmailJobTest::shouldCreateCorrectListOfEmails()
{
    QFETCH(QStringList, emails);
    QFETCH(QStringList, createdListOfEmails);
    auto job = new PotentialPhishingEmailJob;
    job->setPotentialPhishingEmails(emails);
    QCOMPARE(job->checkEmails(), createdListOfEmails);
    delete job;
}

QTEST_MAIN(PotentialPhishingEmailJobTest)
