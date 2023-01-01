/*
  SPDX-FileCopyrightText: 2015-2023 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#pragma once

#include <QObject>

class PotentialPhishingDetailDialogTest : public QObject
{
    Q_OBJECT
public:
    explicit PotentialPhishingDetailDialogTest(QObject *parent = nullptr);
    ~PotentialPhishingDetailDialogTest() override;
private Q_SLOTS:
    void shouldHaveDefaultValue();
    void initTestCase();
};
