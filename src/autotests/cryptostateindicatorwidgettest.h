/*
  SPDX-FileCopyrightText: 2014-2020 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#ifndef CRYPTOSTATEINDICATORWIDGETTEST_H
#define CRYPTOSTATEINDICATORWIDGETTEST_H

#include <QObject>

class CryptoStateIndicatorWidgetTest : public QObject
{
    Q_OBJECT
public:
    explicit CryptoStateIndicatorWidgetTest(QObject *parent = nullptr);
    ~CryptoStateIndicatorWidgetTest();
private Q_SLOTS:
    void shouldHaveDefaultValue();
    void shouldBeNotVisibleWhenShowAlwaysIsFalse();
    void shouldVisibleWhenChangeStatus();
};

#endif // CRYPTOSTATEINDICATORWIDGETTEST_H
