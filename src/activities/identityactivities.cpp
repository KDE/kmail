/*
  SPDX-FileCopyrightText: 2024-2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "identityactivities.h"
#include "activitiesmanager.h"

IdentityActivities::IdentityActivities(ActivitiesManager *manager)
    : PimCommonActivities::ActivitiesFilter<KIdentityManagementCore::IdentityActivitiesAbstract>{manager}
{
}

IdentityActivities::~IdentityActivities() = default;

#include "moc_identityactivities.cpp"
