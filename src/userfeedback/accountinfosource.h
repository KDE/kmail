/*
   SPDX-FileCopyrightText: 2020-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once
#include "kmail_private_export.h"
#include <KF6/KUserFeedback/AbstractDataSource>

class KMAILTESTS_TESTS_EXPORT AccountInfoSource : public KUserFeedback::AbstractDataSource
{
public:
    AccountInfoSource();

    [[nodiscard]] QString name() const override;
    [[nodiscard]] QString description() const override;

    [[nodiscard]] QVariant data() override;

private:
    void updateAccountInfo(const QString &resourceName, int numberOfResource, QVariantList &l);
};
