/*
   SPDX-FileCopyrightText: 2023 Sandro Knauß <knauss@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>

class NearExpiryWarningTest : public QObject
{
    Q_OBJECT
public:
    explicit NearExpiryWarningTest(QObject *parent = nullptr);
    ~NearExpiryWarningTest() override = default;
private Q_SLOTS:
    void shouldHaveDefaultValue();
    void clearInfo();
    void setWarning();
    void addInfo();
};
