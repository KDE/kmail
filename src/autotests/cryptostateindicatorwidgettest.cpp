/*
  Copyright (c) 2014-2017 Montel Laurent <montel@kde.org>

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

#include "cryptostateindicatorwidgettest.h"
#include <qtest.h>
#include "src/editor/widgets/cryptostateindicatorwidget.h"
#include <QLabel>
CryptoStateIndicatorWidgetTest::CryptoStateIndicatorWidgetTest(QObject *parent) : QObject(parent)
{
}

CryptoStateIndicatorWidgetTest::~CryptoStateIndicatorWidgetTest()
{
}

void CryptoStateIndicatorWidgetTest::shouldHaveDefaultValue()
{
    CryptoStateIndicatorWidget w;
    w.show();
    QTest::qWaitForWindowExposed(&w);
    QLabel *signature = w.findChild<QLabel *>(QStringLiteral("signatureindicator"));
    QVERIFY(signature);
    QLabel *encryption = w.findChild<QLabel *>(QStringLiteral("encryptionindicator"));
    QVERIFY(encryption);
    QVERIFY(signature->isVisible());
    QVERIFY(encryption->isVisible());
}

void CryptoStateIndicatorWidgetTest::shouldBeNotVisibleWhenShowAlwaysIsFalse()
{
    CryptoStateIndicatorWidget w;
    w.setShowAlwaysIndicator(false);
    w.show();
    QTest::qWaitForWindowExposed(&w);
    QLabel *signature = w.findChild<QLabel *>(QStringLiteral("signatureindicator"));
    QLabel *encryption = w.findChild<QLabel *>(QStringLiteral("encryptionindicator"));
    QVERIFY(!signature->isVisible());
    QVERIFY(!encryption->isVisible());
    w.updateSignatureAndEncrypionStateIndicators(true, true);

    QVERIFY(!signature->isVisible());
    QVERIFY(!encryption->isVisible());
}

void CryptoStateIndicatorWidgetTest::shouldVisibleWhenChangeStatus()
{
    CryptoStateIndicatorWidget w;
    w.setShowAlwaysIndicator(true);
    w.show();
    QTest::qWaitForWindowExposed(&w);
    QLabel *signature = w.findChild<QLabel *>(QStringLiteral("signatureindicator"));
    QLabel *encryption = w.findChild<QLabel *>(QStringLiteral("encryptionindicator"));
    w.updateSignatureAndEncrypionStateIndicators(true, false);
    QVERIFY(signature->isVisible());
    QVERIFY(!encryption->isVisible());

    w.updateSignatureAndEncrypionStateIndicators(false, true);
    QVERIFY(!signature->isVisible());
    QVERIFY(encryption->isVisible());

    w.updateSignatureAndEncrypionStateIndicators(true, true);
    QVERIFY(signature->isVisible());
    QVERIFY(encryption->isVisible());

    w.updateSignatureAndEncrypionStateIndicators(false, false);
    QVERIFY(!signature->isVisible());
    QVERIFY(!encryption->isVisible());
}

QTEST_MAIN(CryptoStateIndicatorWidgetTest)
