/*
  SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>

class SnoozeFilterProxyModelTest : public QObject
{
    Q_OBJECT
public:
    explicit SnoozeFilterProxyModelTest(QObject *parent = nullptr);
    ~SnoozeFilterProxyModelTest() override = default;
private Q_SLOTS:
    void shouldHaveDefaultValue();
    void shouldHideSnoozedMessages();
    void shouldShowMessageWhenSnoozeIsRemoved();
};
