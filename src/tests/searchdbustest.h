/*
   SPDX-FileCopyrightText: 2016-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QWidget>

class searchdbustest : public QWidget
{
    Q_OBJECT
public:
    explicit searchdbustest(QWidget *parent = nullptr);
    ~searchdbustest() override = default;
private Q_SLOTS:
    void slotReindexCollections();
};

