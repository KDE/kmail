/*
  Copyright (c) 2015-2019 Montel Laurent <montel@kde.org>

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

#include "potentialphishingemailwarningtest.h"
#include "../potentialphishingemailwarning.h"
#include <QAction>
#include <QTest>

PotentialPhishingEmailWarningTest::PotentialPhishingEmailWarningTest(QObject *parent)
    : QObject(parent)
{
}

PotentialPhishingEmailWarningTest::~PotentialPhishingEmailWarningTest()
{
}

void PotentialPhishingEmailWarningTest::shouldHaveDefaultValue()
{
    PotentialPhishingEmailWarning w;
    QVERIFY(!w.isVisible());
    //Verify QVERIFY(w.isCloseButtonVisible());
    QAction *act = w.findChild<QAction *>(QStringLiteral("sendnow"));
    QVERIFY(act);
}

QTEST_MAIN(PotentialPhishingEmailWarningTest)
