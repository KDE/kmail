/*
   SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QWidget>

class AddArchiveMailWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AddArchiveMailWidget(QWidget *parent = nullptr);
    ~AddArchiveMailWidget() override;
};
