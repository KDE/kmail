/*
  SPDX-FileCopyrightText: 2014-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "cryptostateindicatorwidgettest.h"
#include "src/editor/widgets/cryptostateindicatorwidget.h"
#include <QLabel>
#include <QTest>
QTEST_MAIN(CryptoStateIndicatorWidgetTest)
CryptoStateIndicatorWidgetTest::CryptoStateIndicatorWidgetTest(QObject *parent)
    : QObject(parent)
{
    if (qEnvironmentVariableIntValue("KDECI_CANNOT_CREATE_WINDOWS")) {
        QSKIP("KDE CI can't create a window on this platform, skipping some gui tests");
    }
}

CryptoStateIndicatorWidgetTest::~CryptoStateIndicatorWidgetTest() = default;

void CryptoStateIndicatorWidgetTest::shouldHaveDefaultValue()
{
    CryptoStateIndicatorWidget w;
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    auto signature = w.findChild<QLabel *>(QStringLiteral("signatureindicator"));
    QVERIFY(signature);
    auto encryption = w.findChild<QLabel *>(QStringLiteral("encryptionindicator"));
    QVERIFY(encryption);
    QVERIFY(signature->isVisible());
    QVERIFY(encryption->isVisible());
}

void CryptoStateIndicatorWidgetTest::shouldBeNotVisibleWhenShowAlwaysIsFalse()
{
    CryptoStateIndicatorWidget w;
    w.setShowAlwaysIndicator(false);
    w.show();
    auto signature = w.findChild<QLabel *>(QStringLiteral("signatureindicator"));
    auto encryption = w.findChild<QLabel *>(QStringLiteral("encryptionindicator"));
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
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    auto signature = w.findChild<QLabel *>(QStringLiteral("signatureindicator"));
    auto encryption = w.findChild<QLabel *>(QStringLiteral("encryptionindicator"));
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

#include "moc_cryptostateindicatorwidgettest.cpp"
