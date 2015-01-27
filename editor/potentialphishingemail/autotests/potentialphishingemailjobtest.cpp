/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

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
#include <qtest_kde.h>
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
    QTest::newRow("NoPotentialPhishing") <<  (QStringList() << QLatin1String("foo@kde.org")) << QStringList() << false;
    QTest::newRow("HasPotentialPhishing") <<  (QStringList() << QLatin1String("\"bla@kde.org\" <foo@kde.org>")) << QStringList() << true;
    const QString email = QLatin1String("\"bla@kde.org\" <foo@kde.org>");
    QTest::newRow("EmailInWhiteList") <<  (QStringList() << email) << (QStringList() << email) << false;
    QTest::newRow("NotAllEmailInWhiteList") <<  (QStringList() << email << QLatin1String("\"c@kde.org\" <dd@kde.org>")) << (QStringList() << email) << true;
}

void PotentialPhishingEmailJobTest::shouldReturnPotentialPhishingEmails()
{
    QFETCH( QStringList, listEmails );
    QFETCH( QStringList,whiteListEmail );
    QFETCH( bool, hasPotentialPhishing );

    PotentialPhishingEmailJob *job = new PotentialPhishingEmailJob;
    job->setEmailWhiteList(whiteListEmail);
    job->setEmails(listEmails);
    QVERIFY(job->start());
    QCOMPARE(job->potentialPhisingEmails().isEmpty(), !hasPotentialPhishing);
}

void PotentialPhishingEmailJobTest::shouldEmitSignal()
{
    PotentialPhishingEmailJob *job = new PotentialPhishingEmailJob;
    QSignalSpy spy(job, SIGNAL(potentialPhishingEmailsFound(QStringList)));
    job->setEmails((QStringList() << QLatin1String("\"bla@kde.org\" <foo@kde.org>")));
    job->start();
    QCOMPARE(spy.count(), 1);
}


QTEST_KDEMAIN(PotentialPhishingEmailJobTest, NoGUI)
