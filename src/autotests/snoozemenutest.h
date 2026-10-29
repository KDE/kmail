/*
  SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>

class SnoozeMenuTest : public QObject
{
    Q_OBJECT
public:
    explicit SnoozeMenuTest(QObject *parent = nullptr);
    ~SnoozeMenuTest() override = default;
private Q_SLOTS:
    void shouldHaveDefaultValue();
    void shouldEmitSnoozeRequested();
    void shouldComputeWakeUpDateTime_data();
    void shouldComputeWakeUpDateTime();
};
