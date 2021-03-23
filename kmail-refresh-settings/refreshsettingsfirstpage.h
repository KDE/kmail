/*
   SPDX-FileCopyrightText: 2019-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QWidget>

class RefreshSettingsFirstPage : public QWidget
{
    Q_OBJECT
public:
    explicit RefreshSettingsFirstPage(QWidget *parent = nullptr);
    ~RefreshSettingsFirstPage() override;
};

