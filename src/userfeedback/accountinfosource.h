/*
   SPDX-FileCopyrightText: 2020-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once
#include "kmail_private_export.h"
#include <KUserFeedback/AbstractDataSource>

class KMAILTESTS_TESTS_EXPORT AccountInfoSource : public KUserFeedback::AbstractDataSource
{
public:
    AccountInfoSource();

    [[nodiscard]] QString name() const override;
    [[nodiscard]] QString description() const override;

    [[nodiscard]] QVariant data() override;

private:
    KMAIL_NO_EXPORT void updateAccountInfo(const QString &resourceName, int count, QVariantList &l);
};
