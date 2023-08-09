/*
   SPDX-FileCopyrightText: 2022 Sandro Knauß <knauss@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once
#include "kmail_private_export.h"
#include <KMessageWidget>

class KMAILTESTS_TESTS_EXPORT NearExpiryWarning : public KMessageWidget
{
    Q_OBJECT
public:
    explicit NearExpiryWarning(QWidget *parent = nullptr);
    ~NearExpiryWarning() override;

    void addInfo(const QString &msg);
    void setWarning(bool warning);
    void clearInfo();
};
