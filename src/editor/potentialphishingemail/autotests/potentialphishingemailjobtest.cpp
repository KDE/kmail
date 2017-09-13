/*
  Copyright (c) 2015-2017 Montel Laurent <montel@kde.org>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

*/

#include "potentialphishingemailjobtest.h"
#include "../potentialphishingemailjob.h"
#include <qtest.h>
#include <QStringList>
#include <QSignalSpy>

PotentialPhishingEmailJobTest::PotentialPhishingEmailJobTest(QObject *parent)
    : QObject(parent)
{
}

PotentialPhishingEmailJobTest::~PotentialPhishingEmailJobTest()
{
}

void PotentialPhishingEmailJobTest::shouldNotStartIfNoEmails()
{
    PotentialPhishingEmailJob *job = new PotentialPhishingEmailJob;
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
    QTest::newRow("EmailWithSameNameAndDisplayName") << (QStringList() << QStringLiteral("\"<foo@kde.com>\" <foo@kde.com>")) << (QStringList() << email) << false;
    QTest::newRow("EmailWithSameNameAndDisplayNameWithSpace") << (QStringList() << QStringLiteral(" \"<foo@kde.com>\" <foo@kde.com> ")) << (QStringList() << email) << false;

    QTest::newRow("notsamecase") << (QStringList() << QStringLiteral("\"Foo@kde.org\" <foo@kde.org>")) << QStringList() << false;
    QTest::newRow("notsamecaseaddress") << (QStringList() << QStringLiteral("\"Foo@kde.org\" <FOO@kde.ORG>")) << QStringList() << false;

    QTest::newRow("emailinparenthese") << (QStringList() << QStringLiteral("\"bla (Foo@kde.org)\" <FOO@kde.ORG>")) << QStringList() << false;
    QTest::newRow("notemailinparenthese") << (QStringList() << QStringLiteral("\"bla (bli@kde.org)\" <FOO@kde.ORG>")) << QStringList() << true;
    QTest::newRow("erroremailinparenthese") << (QStringList() << QStringLiteral("\"bla Foo@kde.org\" <FOO@kde.ORG>")) << QStringList() << true;

    QTest::newRow("WithMultiSameEmail") << (QStringList() << QStringLiteral("\"foo@kde.org foo@kde.org\" <foo@kde.org>")) << QStringList() << false;
    QTest::newRow("WithMultiSameEmailWithSpace") << (QStringList() << QStringLiteral("\"  foo@kde.org   foo@kde.org  \" <foo@kde.org>")) << QStringList() << false;
    QTest::newRow("WithMultiNotSameEmail") << (QStringList() << QStringLiteral("\"  bla@kde.org   foo@kde.org  \" <foo@kde.org>")) << QStringList() << true;

    QTest::newRow("EmailWithSimpleQuote") << (QStringList() << QStringLiteral("\"\'foo@kde.org\'\" <foo@kde.org>")) << QStringList() << false;
}

void PotentialPhishingEmailJobTest::shouldReturnPotentialPhishingEmails()
{
    QFETCH(QStringList, listEmails);
    QFETCH(QStringList, whiteListEmail);
    QFETCH(bool, hasPotentialPhishing);

    PotentialPhishingEmailJob *job = new PotentialPhishingEmailJob;
    job->setEmailWhiteList(whiteListEmail);
    job->setPotentialPhishingEmails(listEmails);
    QVERIFY(job->start());
    QCOMPARE(job->potentialPhisingEmails().isEmpty(), !hasPotentialPhishing);
}

void PotentialPhishingEmailJobTest::shouldEmitSignal()
{
    PotentialPhishingEmailJob *job = new PotentialPhishingEmailJob;
    QSignalSpy spy(job, SIGNAL(potentialPhishingEmailsFound(QStringList)));
    job->setPotentialPhishingEmails((QStringList() << QStringLiteral("\"bla@kde.org\" <foo@kde.org>")));
    job->start();
    QCOMPARE(spy.count(), 1);
}

void PotentialPhishingEmailJobTest::shouldCreateCorrectListOfEmails_data()
{
    QTest::addColumn<QStringList>("emails");
    QTest::addColumn<QStringList>("createdListOfEmails");
    QTest::newRow("emptylist") << QStringList() << QStringList();
    QStringList emails{
        QStringLiteral("foo@kde.org"), QStringLiteral("bla@kde.org")
    };
    QStringList createdList{
        QStringLiteral("foo@kde.org"), QStringLiteral("bla@kde.org")
    };
    QTest::newRow("nonempty") << emails << createdList;
    emails = QStringList{
        QStringLiteral("\"bla\" <foo@kde.org>"), QStringLiteral("bla@kde.org")
    };
    QTest::newRow("potentialerrors") << emails << emails;

    emails = QStringList{
        QStringLiteral("\"bla, foo\" <foo@kde.org>"), QStringLiteral("bla@kde.org")
    };
    QTest::newRow("emailswithquote") << emails << emails;

    emails = QStringList{
        QStringLiteral("\"bla, foo\" <foo@kde.org>"), QStringLiteral("bla@kde.org"), QStringLiteral(" ")
    };
    createdList = QStringList{
        QStringLiteral("\"bla, foo\" <foo@kde.org>"), QStringLiteral("bla@kde.org")
    };
    QTest::newRow("emailswithemptystr") << emails << createdList;
}

void PotentialPhishingEmailJobTest::shouldCreateCorrectListOfEmails()
{
    QFETCH(QStringList, emails);
    QFETCH(QStringList, createdListOfEmails);
    PotentialPhishingEmailJob *job = new PotentialPhishingEmailJob;
    job->setPotentialPhishingEmails(emails);
    QCOMPARE(job->checkEmails(), createdListOfEmails);
}

QTEST_MAIN(PotentialPhishingEmailJobTest)
