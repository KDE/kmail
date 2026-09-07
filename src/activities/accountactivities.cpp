/*
  SPDX-FileCopyrightText: 2024-2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "accountactivities.h"
#include "activitiesmanager.h"

AccountActivities::AccountActivities(ActivitiesManager *manager)
    : PimCommonActivities::ActivitiesFilter<Akonadi::AccountActivitiesAbstract>{manager}
{
}

AccountActivities::~AccountActivities() = default;

#include "moc_accountactivities.cpp"
