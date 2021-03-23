/*
  SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#pragma once

#include <QObject>

class PotentialPhishingEmailWarningTest : public QObject
{
    Q_OBJECT
public:
    explicit PotentialPhishingEmailWarningTest(QObject *parent = nullptr);
    ~PotentialPhishingEmailWarningTest() override;
private Q_SLOTS:
    void shouldHaveDefaultValue();
};

