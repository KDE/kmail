/*
   SPDX-FileCopyrightText: 2012-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QComboBox>

class HourComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit HourComboBox(QWidget *parent = nullptr);
    ~HourComboBox() override;

    void setHour(int hour);
    Q_REQUIRED_RESULT int hour() const;

private:
    void initializeList();
};