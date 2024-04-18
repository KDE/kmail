/*
  SPDX-FileCopyrightText: 2024 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
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

private:
    ActivitiesManager *const mActivitiesManager;
};
