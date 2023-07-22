/*
  SPDX-FileCopyrightText: 2023 Sandro Knauß <sknauss@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>

class EncryptionStateTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void override();

    void unsetOverride();

    void toggleOverride();

    void acceptedSolution();

    void possibleEncrypt();

    void autoEncrypt();

    void shouldHaveDefaults();

    void encrypt();
};
