/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "csvwidgettest.h"
#include "../widgets/csvwidget.h"
#include <KUrlRequester>
#include <QLabel>
#include <qtest.h>
CsvWidgetTest::CsvWidgetTest(QObject *parent)
    : QObject(parent)
{

}

CsvWidgetTest::~CsvWidgetTest()
{

}

void CsvWidgetTest::shouldHaveDefaultValue()
{
    MailMerge::CsvWidget w;

    QLabel *lab = w.findChild<QLabel *>(QStringLiteral("label"));
    QVERIFY(lab);

    KUrlRequester *urlrequester = w.findChild<KUrlRequester *>(QStringLiteral("cvsurlrequester"));
    QVERIFY(urlrequester);
    QVERIFY(urlrequester->url().isEmpty());
}

void CsvWidgetTest::shouldChangePath()
{
    MailMerge::CsvWidget w;

    KUrlRequester *urlrequester = w.findChild<KUrlRequester *>(QStringLiteral("cvsurlrequester"));
    QVERIFY(urlrequester->url().isEmpty());
    QUrl url(QStringLiteral("file:///tmp/foo.txt"));
    urlrequester->setUrl(url);
    QCOMPARE(urlrequester->url(), url);
}

QTEST_MAIN(CsvWidgetTest)
