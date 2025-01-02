/*
  SPDX-FileCopyrightText: 2024-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include <Akonadi/AccountActivitiesAbstract>
class ActivitiesManager;
class AccountActivities : public Akonadi::AccountActivitiesAbstract
{
    Q_OBJECT
public:
    explicit AccountActivities(ActivitiesManager *manager);
    ~AccountActivities() override;

    [[nodiscard]] bool filterAcceptsRow(const QStringList &activities) const override;
    [[nodiscard]] bool hasActivitySupport() const override;
    [[nodiscard]] QString currentActivity() const override;

private:
    ActivitiesManager *const mActivitiesManager;
};
