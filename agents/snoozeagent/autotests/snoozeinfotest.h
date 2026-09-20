/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>

class SnoozeInfoTest : public QObject
{
    Q_OBJECT
public:
    explicit SnoozeInfoTest(QObject *parent = nullptr);
    ~SnoozeInfoTest() override = default;

private Q_SLOTS:
    void shouldHaveDefaultValue();
    void shouldBeInvalidWithoutWakeUpDate();
    void shouldRestoreFromConfig();
};
