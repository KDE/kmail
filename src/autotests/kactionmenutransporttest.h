/*
  SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#ifndef KACTIONMENUTRANSPORTTEST_H
#define KACTIONMENUTRANSPORTTEST_H

#include <QObject>

class KActionMenuTransportTest : public QObject
{
    Q_OBJECT
public:
    explicit KActionMenuTransportTest(QObject *parent = nullptr);
    ~KActionMenuTransportTest() override;
private Q_SLOTS:
    void shouldHaveDefaultValue();
};

#endif // KACTIONMENUTRANSPORTTEST_H
