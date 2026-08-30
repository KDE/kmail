/*
  SPDX-FileCopyrightText: 2015-2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#include "potentialphishingemailjobtest.h"
#include "../potentialphishingemailjob.h"
#include <QSignalSpy>
#include <QStringList>
#include <QTest>
using namespace Qt::Literals::StringLiterals;

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
    QTest::newRow("NoPotentialPhishing") << (QStringList() << u"foo@kde.org"_s) << QStringList() << false;
    QTest::newRow("HasPotentialPhishing") << (QStringList() << u"\"bla@kde.org\" <foo@kde.org>"_s) << QStringList() << true;
    const QString email = u"\"bla@kde.org\" <foo@kde.org>"_s;
    QTest::newRow("EmailInWhiteList") << (QStringList() << email) << (QStringList() << email) << false;
    QTest::newRow("EmailInWhiteListCaseInsensitive") << (QStringList() << u"\"BLA@kde.org\" <FOO@kde.org>"_s)
                                                     << (QStringList() << u"\"bla@kde.org\" <foo@kde.org>"_s) << false;
    QTest::newRow("NotAllEmailInWhiteList") << (QStringList() << email << u"\"c@kde.org\" <dd@kde.org>"_s) << (QStringList() << email) << true;
    QTest::newRow("EmailInWhiteListWithSpace") << (QStringList() << u" \"bla@kde.org\" <foo@kde.org> "_s) << (QStringList() << email) << false;
    QTest::newRow("EmailWithSameNameAndDisplayName") << (QStringList() << u"\"<foo@kde.com>\" <foo@kde.com>"_s) << (QStringList() << email) << false;
    QTest::newRow("EmailWithSameNameAndDisplayNameWithSpace") << (QStringList() << u" \"<foo@kde.com>\" <foo@kde.com> "_s) << (QStringList() << email) << false;

    QTest::newRow("notsamecase") << (QStringList() << u"\"Foo@kde.org\" <foo@kde.org>"_s) << QStringList() << false;
    QTest::newRow("notsamecaseaddress") << (QStringList() << u"\"Foo@kde.org\" <FOO@kde.ORG>"_s) << QStringList() << false;

    QTest::newRow("emailinparenthese") << (QStringList() << u"\"bla (Foo@kde.org)\" <FOO@kde.ORG>"_s) << QStringList() << false;
    QTest::newRow("notemailinparenthese") << (QStringList() << u"\"bla (bli@kde.org)\" <FOO@kde.ORG>"_s) << QStringList() << true;
    QTest::newRow("erroremailinparenthese") << (QStringList() << u"\"bla Foo@kde.org\" <FOO@kde.ORG>"_s) << QStringList() << true;

    QTest::newRow("WithMultiSameEmail") << (QStringList() << u"\"foo@kde.org foo@kde.org\" <foo@kde.org>"_s) << QStringList() << false;
    QTest::newRow("WithMultiSameEmailWithSpace") << (QStringList() << u"\"  foo@kde.org   foo@kde.org  \" <foo@kde.org>"_s) << QStringList() << false;
    QTest::newRow("WithMultiNotSameEmail") << (QStringList() << u"\"  bla@kde.org   foo@kde.org  \" <foo@kde.org>"_s) << QStringList() << true;

    QTest::newRow("EmailWithSimpleQuote") << (QStringList() << u"\"\'foo@kde.org\'\" <foo@kde.org>"_s) << QStringList() << false;

    QTest::newRow("BadCompletion") << (QStringList() << u"@kde.org <foo@kde.org>"_s) << QStringList() << false;
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
    job->setPotentialPhishingEmails((QStringList() << u"\"bla@kde.org\" <foo@kde.org>"_s));
    QVERIFY(job->start());
    QCOMPARE(spy.count(), 1);
}

void PotentialPhishingEmailJobTest::shouldCreateCorrectListOfEmails_data()
{
    QTest::addColumn<QStringList>("emails");
    QTest::addColumn<QStringList>("createdListOfEmails");
    QTest::newRow("emptylist") << QStringList() << QStringList();
    QStringList emails{u"foo@kde.org"_s, u"bla@kde.org"_s};
    QStringList createdList{u"foo@kde.org"_s, u"bla@kde.org"_s};
    QTest::newRow("nonempty") << emails << createdList;
    emails = QStringList{u"\"bla\" <foo@kde.org>"_s, u"bla@kde.org"_s};
    QTest::newRow("potentialerrors") << emails << emails;

    emails = QStringList{u"\"bla, foo\" <foo@kde.org>"_s, u"bla@kde.org"_s};
    QTest::newRow("emailswithquote") << emails << emails;

    emails = QStringList{u"\"bla, foo\" <foo@kde.org>"_s, u"bla@kde.org"_s, u" "_s};
    createdList = QStringList{u"\"bla, foo\" <foo@kde.org>"_s, u"bla@kde.org"_s};
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

#include "moc_potentialphishingemailjobtest.cpp"
