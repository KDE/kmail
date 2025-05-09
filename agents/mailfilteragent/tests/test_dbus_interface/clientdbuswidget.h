/*
   SPDX-FileCopyrightText: 2023-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "dbusinterface.h"
#include <QWidget>

class ClientDbusWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ClientDbusWidget(QWidget *parent = nullptr);
    ~ClientDbusWidget() override;

private:
    OrgFreedesktopTestDbusInterface *mDbusInterface = nullptr;
};
