/*
  SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
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

