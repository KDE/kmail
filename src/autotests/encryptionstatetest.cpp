/*
  SPDX-FileCopyrightText: 2023 Sandro Knauß <sknauss@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "encryptionstatetest.h"
#include "editor/encryptionstate.h"

#include <QSignalSpy>
#include <QTest>

QTEST_GUILESS_MAIN(EncryptionStateTest)

void EncryptionStateTest::shouldHaveDefaults()
{
    EncryptionState e;
    QCOMPARE(e.override(), false);
    QCOMPARE(e.hasOverride(), false);
    QCOMPARE(e.possibleEncrypt(), false);
    QCOMPARE(e.autoEncrypt(), false);
    QCOMPARE(e.acceptedSolution(), false);
    QCOMPARE(e.encrypt(), false);
}

void EncryptionStateTest::override()
{
    EncryptionState e;
    QSignalSpy spy(&e, &EncryptionState::overrideChanged);
    QSignalSpy spyHasOverride(&e, &EncryptionState::hasOverrideChanged);
    QSignalSpy spyEncrypt(&e, &EncryptionState::encryptChanged);

    e.setOverride(false);
    QCOMPARE(e.override(), false);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0), false);
    QCOMPARE(spyHasOverride.count(), 1);
    QCOMPARE(spyHasOverride.at(0).at(0), true);
    QCOMPARE(spyEncrypt.count(), 0);

    e.setOverride(false);
    QCOMPARE(e.override(), false);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spyHasOverride.count(), 1);
    QCOMPARE(spyEncrypt.count(), 0);

    e.setOverride(true);
    QCOMPARE(e.override(), true);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.at(1).at(0), true);
    QCOMPARE(spyHasOverride.count(), 1);
    QCOMPARE(spyEncrypt.count(), 1);
    QCOMPARE(spyEncrypt.at(0).at(0), true);

    e.setOverride(true);
    QCOMPARE(e.override(), true);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spyHasOverride.count(), 1);
    QCOMPARE(spyEncrypt.count(), 1);

    e.setOverride(false);
    QCOMPARE(e.override(), false);
    QCOMPARE(spy.count(), 3);
    QCOMPARE(spy.at(2).at(0), false);
    QCOMPARE(spyHasOverride.count(), 1);
    QCOMPARE(spyEncrypt.count(), 2);
    QCOMPARE(spyEncrypt.at(1).at(0), false);
}

void EncryptionStateTest::unsetOverride()
{
    EncryptionState e;
    QSignalSpy spy(&e, &EncryptionState::overrideChanged);
    QSignalSpy spyHasOverride(&e, &EncryptionState::hasOverrideChanged);
    QSignalSpy spyEncrypt(&e, &EncryptionState::encryptChanged);

    e.unsetOverride();
    QCOMPARE(e.hasOverride(), false);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(spyHasOverride.count(), 0);
    QCOMPARE(spyEncrypt.count(), 0);

    e.setOverride(false);
    QCOMPARE(e.hasOverride(), true);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spyHasOverride.count(), 1);
    QCOMPARE(spyEncrypt.count(), 0);

    e.unsetOverride();
    QCOMPARE(e.hasOverride(), false);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.at(1).at(0), false);
    QCOMPARE(spyHasOverride.count(), 2);
    QCOMPARE(spyHasOverride.at(1).at(0), false);
    QCOMPARE(spyEncrypt.count(), 0);
}

void EncryptionStateTest::toggleOverride()
{
    EncryptionState e;

    e.setPossibleEncrypt(true);
    e.setAutoEncrypt(true);
    e.setAcceptedSolution(true);

    QSignalSpy spy(&e, &EncryptionState::overrideChanged);
    QSignalSpy spyHasOverride(&e, &EncryptionState::hasOverrideChanged);
    QSignalSpy spyEncrypt(&e, &EncryptionState::encryptChanged);

    e.toggleOverride();
    QCOMPARE(e.hasOverride(), true);
    QCOMPARE(e.encrypt(), false);
    QCOMPARE(e.override(), false);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0), false);
    QCOMPARE(spyHasOverride.count(), 1);
    QCOMPARE(spyHasOverride.at(0).at(0), true);
    QCOMPARE(spyEncrypt.count(), 1);
    QCOMPARE(spyEncrypt.at(0).at(0), false);

    e.toggleOverride();
    QCOMPARE(e.hasOverride(), true);
    QCOMPARE(e.encrypt(), true);
    QCOMPARE(e.override(), true);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.at(1).at(0), true);
    QCOMPARE(spyHasOverride.count(), 1);
    QCOMPARE(spyEncrypt.count(), 2);
    QCOMPARE(spyEncrypt.at(1).at(0), true);

    e.toggleOverride();
    QCOMPARE(e.hasOverride(), true);
    QCOMPARE(e.encrypt(), false);
    QCOMPARE(e.override(), false);
    QCOMPARE(spy.count(), 3);
    QCOMPARE(spy.at(2).at(0), false);
    QCOMPARE(spyHasOverride.count(), 1);
    QCOMPARE(spyEncrypt.count(), 3);
    QCOMPARE(spyEncrypt.at(2).at(0), false);
}

void EncryptionStateTest::acceptedSolution()
{
    EncryptionState e;
    QSignalSpy spy(&e, &EncryptionState::acceptedSolutionChanged);
    QSignalSpy spyEncrypt(&e, &EncryptionState::encryptChanged);

    e.setAcceptedSolution(false);
    QCOMPARE(e.acceptedSolution(), false);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(spyEncrypt.count(), 0);

    e.setAcceptedSolution(true);
    QCOMPARE(e.acceptedSolution(), true);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0), true);
    QCOMPARE(spyEncrypt.count(), 0);

    e.setAcceptedSolution(true);
    QCOMPARE(e.acceptedSolution(), true);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spyEncrypt.count(), 0);

    e.setAcceptedSolution(false);
    QCOMPARE(e.acceptedSolution(), false);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.at(1).at(0), false);
    QCOMPARE(spyEncrypt.count(), 0);
}

void EncryptionStateTest::possibleEncrypt()
{
    EncryptionState e;
    QSignalSpy spy(&e, &EncryptionState::possibleEncryptChanged);
    QSignalSpy spyEncrypt(&e, &EncryptionState::encryptChanged);

    e.setPossibleEncrypt(false);
    QCOMPARE(e.possibleEncrypt(), false);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(spyEncrypt.count(), 0);

    e.setPossibleEncrypt(true);
    QCOMPARE(e.possibleEncrypt(), true);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0), true);
    QCOMPARE(spyEncrypt.count(), 0);

    e.setPossibleEncrypt(true);
    QCOMPARE(e.possibleEncrypt(), true);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spyEncrypt.count(), 0);

    e.setPossibleEncrypt(false);
    QCOMPARE(e.possibleEncrypt(), false);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.at(1).at(0), false);
    QCOMPARE(spyEncrypt.count(), 0);
}

void EncryptionStateTest::autoEncrypt()
{
    EncryptionState e;
    QSignalSpy spy(&e, &EncryptionState::autoEncryptChanged);
    QSignalSpy spyEncrypt(&e, &EncryptionState::encryptChanged);

    e.setAutoEncrypt(false);
    QCOMPARE(e.autoEncrypt(), false);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(spyEncrypt.count(), 0);

    e.setAutoEncrypt(true);
    QCOMPARE(e.autoEncrypt(), true);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0), true);
    QCOMPARE(spyEncrypt.count(), 0);

    e.setAutoEncrypt(true);
    QCOMPARE(e.autoEncrypt(), true);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spyEncrypt.count(), 0);

    e.setAutoEncrypt(false);
    QCOMPARE(e.autoEncrypt(), false);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.at(1).at(0), false);
    QCOMPARE(spyEncrypt.count(), 0);
}

void EncryptionStateTest::encrypt()
{
    EncryptionState e;
    QSignalSpy spyEncrypt(&e, &EncryptionState::encryptChanged);

    e.setPossibleEncrypt(true);
    e.setAutoEncrypt(true);
    QCOMPARE(e.encrypt(), false);
    QCOMPARE(spyEncrypt.count(), 0);

    e.setAcceptedSolution(true);
    QCOMPARE(e.encrypt(), true);
    QCOMPARE(spyEncrypt.count(), 1);
    QCOMPARE(spyEncrypt.at(0).at(0), true);

    e.setAutoEncrypt(false);
    QCOMPARE(e.encrypt(), false);
    QCOMPARE(spyEncrypt.count(), 2);
    QCOMPARE(spyEncrypt.at(1).at(0), false);

    e.setAutoEncrypt(true);
    QCOMPARE(e.encrypt(), true);
    QCOMPARE(spyEncrypt.count(), 3);
    QCOMPARE(spyEncrypt.at(2).at(0), true);

    e.setPossibleEncrypt(false);
    QCOMPARE(e.encrypt(), false);
    QCOMPARE(spyEncrypt.count(), 4);
    QCOMPARE(spyEncrypt.at(3).at(0), false);

    e.setPossibleEncrypt(true);
    QCOMPARE(e.encrypt(), true);
    QCOMPARE(spyEncrypt.count(), 5);
    QCOMPARE(spyEncrypt.at(4).at(0), true);
}

#include "moc_encryptionstatetest.cpp"
