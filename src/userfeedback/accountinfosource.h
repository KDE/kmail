/*
   SPDX-FileCopyrightText: 2020-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once
#include "kmail_private_export.h"
#include <KUserFeedback/AbstractDataSource>

class KMAILTESTS_TESTS_EXPORT AccountInfoSource : public KUserFeedback::AbstractDataSource
{
public:
    AccountInfoSource();

    Q_REQUIRED_RESULT QString name() const override;
    Q_REQUIRED_RESULT QString description() const override;

    Q_REQUIRED_RESULT QVariant data() override;
};

