/*
   SPDX-FileCopyrightText: 2023-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "mailfilteragentinterface.h"
#include <QWidget>

class MailAgentDbusWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MailAgentDbusWidget(QWidget *parent = nullptr);
    ~MailAgentDbusWidget() override;

private:
    OrgFreedesktopAkonadiMailFilterAgentInterface *mMailFilterAgentInterface = nullptr;
};
