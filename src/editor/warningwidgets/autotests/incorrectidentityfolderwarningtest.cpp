/*
   SPDX-FileCopyrightText: 2017-2026 Laurent Montel <montel@kde.org>

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
    QWidget wid;
    auto layout = new QHBoxLayout(&wid);
    IncorrectIdentityFolderWarning w;
    layout->addWidget(&w);
    wid.show();
    QVERIFY(!w.isVisible());
    // QVERIFY(w.isCloseButtonVisible());
    QCOMPARE(w.messageType(), KMessageWidget::Warning);
    QVERIFY(w.wordWrap());
}

void IncorrectIdentityFolderWarningTest::shouldShowWarningInvalidIdentity()
{
    QWidget wid;
    auto layout = new QHBoxLayout(&wid);
    IncorrectIdentityFolderWarning w;
    layout->addWidget(&w);
    wid.show();
    QVERIFY(!w.isVisible());
    w.identityInvalid();
    QVERIFY(w.isVisible());
    QVERIFY(!w.text().isEmpty());
}

void IncorrectIdentityFolderWarningTest::shouldShowWarningInvalidMailTransport()
{
    QWidget wid;
    auto layout = new QHBoxLayout(&wid);
    IncorrectIdentityFolderWarning w;
    layout->addWidget(&w);
    wid.show();
    QVERIFY(!w.isVisible());
    w.mailTransportIsInvalid();
    QVERIFY(w.isVisible());
    QVERIFY(!w.text().isEmpty());
}

void IncorrectIdentityFolderWarningTest::shouldShowWarningInvalidFcc()
{
    QWidget wid;
    auto layout = new QHBoxLayout(&wid);
    IncorrectIdentityFolderWarning w;
    layout->addWidget(&w);
    wid.show();
    QVERIFY(!w.isVisible());
    w.fccIsInvalid();
    QVERIFY(w.isVisible());
    QVERIFY(!w.text().isEmpty());
}

#include "moc_incorrectidentityfolderwarningtest.cpp"
