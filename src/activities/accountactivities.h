/*
  SPDX-FileCopyrightText: 2024-2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <Akonadi/AccountActivitiesAbstract>
#include <PimCommonActivities/ActivitiesBaseManager>
class ActivitiesManager;
class AccountActivities : public PimCommonActivities::ActivitiesFilter<Akonadi::AccountActivitiesAbstract>
{
    Q_OBJECT
public:
    explicit AccountActivities(ActivitiesManager *manager);
    ~AccountActivities() override;
};
