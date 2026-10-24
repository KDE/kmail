/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>

class SnoozeMenu : public QObject
{
    Q_OBJECT
public:
    explicit SnoozeMenu(QObject *parent = nullptr);
    ~SnoozeMenu() override;
};
