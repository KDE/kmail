/*
   SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>

class FormatComboBoxTest : public QObject
{
    Q_OBJECT
public:
    explicit FormatComboBoxTest(QObject *parent = nullptr);
    ~FormatComboBoxTest();
private Q_SLOTS:
    void shouldHaveDefaultValue();
    void changeCurrentItem_data();
    void changeCurrentItem();
};

