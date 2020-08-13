/*
   SPDX-FileCopyrightText: 2015-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef FORMATCOMBOBOXTEST_H
#define FORMATCOMBOBOXTEST_H

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

#endif // FORMATCOMBOBOXTEST_H
