/*
   SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "incorrectidentityfolderwarningtest.h"
#include "../incorrectidentityfolderwarning.h"
#include <QHBoxLayout>
#include <QTest>
QTEST_MAIN(IncorrectIdentityFolderWarningTest)

IncorrectIdentityFolderWarningTest::IncorrectIdentityFolderWarningTest(QObject *parent)
    : QObject(parent)
{
}

void IncorrectIdentityFolderWarningTest::shouldHaveDefaultValues()
{
    auto wid = new QWidget;
    auto layout = new QHBoxLayout(wid);
    IncorrectIdentityFolderWarning w;
    layout->addWidget(&w);
    wid->show();
    QVERIFY(!w.isVisible());
    // QVERIFY(w.isCloseButtonVisible());
    QCOMPARE(w.messageType(), KMessageWidget::Warning);
    QVERIFY(w.wordWrap());
}

void IncorrectIdentityFolderWarningTest::shouldShowWarningInvalidIdentity()
{
    auto wid = new QWidget;
    auto layout = new QHBoxLayout(wid);
    IncorrectIdentityFolderWarning w;
    layout->addWidget(&w);
    wid->show();
    QVERIFY(!w.isVisible());
    w.identityInvalid();
    QVERIFY(w.isVisible());
    QVERIFY(!w.text().isEmpty());
}

void IncorrectIdentityFolderWarningTest::shouldShowWarningInvalidMailTransport()
{
    auto wid = new QWidget;
    auto layout = new QHBoxLayout(wid);
    IncorrectIdentityFolderWarning w;
    layout->addWidget(&w);
    wid->show();
    QVERIFY(!w.isVisible());
    w.mailTransportIsInvalid();
    QVERIFY(w.isVisible());
    QVERIFY(!w.text().isEmpty());
}

void IncorrectIdentityFolderWarningTest::shouldShowWarningInvalidFcc()
{
    auto wid = new QWidget;
    auto layout = new QHBoxLayout(wid);
    IncorrectIdentityFolderWarning w;
    layout->addWidget(&w);
    wid->show();
    QVERIFY(!w.isVisible());
    w.fccIsInvalid();
    QVERIFY(w.isVisible());
    QVERIFY(!w.text().isEmpty());
}
