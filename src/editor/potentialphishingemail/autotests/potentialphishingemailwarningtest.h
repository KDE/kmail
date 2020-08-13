/*
  SPDX-FileCopyrightText: 2015-2020 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#ifndef POTENTIALPHISHINGEMAILWARNINGTEST_H
#define POTENTIALPHISHINGEMAILWARNINGTEST_H

#include <QObject>

class PotentialPhishingEmailWarningTest : public QObject
{
    Q_OBJECT
public:
    explicit PotentialPhishingEmailWarningTest(QObject *parent = nullptr);
    ~PotentialPhishingEmailWarningTest();
private Q_SLOTS:
    void shouldHaveDefaultValue();
};

#endif // POTENTIALPHISHINGEMAILWARNINGTEST_H
