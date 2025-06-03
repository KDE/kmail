/*
  SPDX-FileCopyrightText: 2024-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KIdentityManagementCore/IdentityActivitiesAbstract>
class ActivitiesManager;
class IdentityActivities : public KIdentityManagementCore::IdentityActivitiesAbstract
{
    Q_OBJECT
public:
    explicit IdentityActivities(ActivitiesManager *manager);
    ~IdentityActivities() override;

    [[nodiscard]] bool filterAcceptsRow(const QStringList &lst) const override;
    [[nodiscard]] bool hasActivitySupport() const override;
    [[nodiscard]] QString currentActivity() const override;

private:
    ActivitiesManager *const mActivitiesManager;
};
