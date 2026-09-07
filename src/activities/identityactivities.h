/*
  SPDX-FileCopyrightText: 2024-2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KIdentityManagementCore/IdentityActivitiesAbstract>
#include <PimCommonActivities/ActivitiesBaseManager>
class ActivitiesManager;
class IdentityActivities : public PimCommonActivities::ActivitiesFilter<KIdentityManagementCore::IdentityActivitiesAbstract>
{
    Q_OBJECT
public:
    explicit IdentityActivities(ActivitiesManager *manager);
    ~IdentityActivities() override;
};
