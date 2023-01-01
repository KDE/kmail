/*
   SPDX-FileCopyrightText: 2015-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>

class UnitComboBoxTest : public QObject
{
    Q_OBJECT
public:
    explicit UnitComboBoxTest(QObject *parent = nullptr);
    ~UnitComboBoxTest() override;
private Q_SLOTS:
    void shouldHaveDefaultValue();
    void changeCurrentItem();
    void changeCurrentItem_data();
};
