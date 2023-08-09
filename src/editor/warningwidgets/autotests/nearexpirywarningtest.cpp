/*
   SPDX-FileCopyrightText: 2023 Sandro Knauß <knauss@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "nearexpirywarningtest.h"
#include "../nearexpirywarning.h"
#include <QHBoxLayout>
#include <QTest>

QTEST_MAIN(NearExpiryWarningTest)

NearExpiryWarningTest::NearExpiryWarningTest(QObject *parent)
    : QObject(parent)
{
}

void NearExpiryWarningTest::shouldHaveDefaultValue()
{
    QWidget wid;
    auto layout = new QHBoxLayout(&wid);
    NearExpiryWarning w;
    layout->addWidget(&w);
    wid.show();
    QVERIFY(!w.isVisible());
    QVERIFY(!w.isCloseButtonVisible());
    QCOMPARE(w.messageType(), KMessageWidget::Information);
    QVERIFY(w.wordWrap());
    QVERIFY(w.text().isEmpty());

    w.setVisible(true);
    QVERIFY(w.isVisible());
    QVERIFY(w.isCloseButtonVisible());
}

void NearExpiryWarningTest::clearInfo()
{
    QWidget wid;
    auto layout = new QHBoxLayout(&wid);
    NearExpiryWarning w;
    layout->addWidget(&w);
    wid.show();
    w.addInfo(QStringLiteral("test"));
    w.setWarning(true);
    w.clearInfo();
    QCOMPARE(w.messageType(), KMessageWidget::Information);
    QVERIFY(w.wordWrap());
    QVERIFY(w.text().isEmpty());
}

void NearExpiryWarningTest::setWarning()
{
    QWidget wid;
    auto layout = new QHBoxLayout(&wid);
    NearExpiryWarning w;
    layout->addWidget(&w);
    wid.show();
    w.setWarning(true);
    QCOMPARE(w.messageType(), KMessageWidget::Warning);
    w.setWarning(false);
    QCOMPARE(w.messageType(), KMessageWidget::Information);
}

void NearExpiryWarningTest::addInfo()
{
    QWidget wid;
    auto layout = new QHBoxLayout(&wid);
    NearExpiryWarning w;
    layout->addWidget(&w);
    wid.show();
    w.addInfo(QStringLiteral("test1"));
    QCOMPARE(w.text(), QStringLiteral("<p>test1</p>"));
    w.addInfo(QStringLiteral("test2"));
    QCOMPARE(w.text(), QStringLiteral("<p>test1</p>\n<p>test2</p>"));
}
