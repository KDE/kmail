/*
  SPDX-FileCopyrightText: 2014-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class CryptoStateIndicatorWidgetTest : public QObject
{
    Q_OBJECT
public:
    explicit CryptoStateIndicatorWidgetTest(QObject *parent = nullptr);
    ~CryptoStateIndicatorWidgetTest() override;
private Q_SLOTS:
    void shouldHaveDefaultValue();
    void shouldBeNotVisibleWhenShowAlwaysIsFalse();
    void shouldVisibleWhenChangeStatus();
};
