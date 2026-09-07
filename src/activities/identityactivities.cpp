/*
  SPDX-FileCopyrightText: 2024-2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "identityactivities.h"
#include "activitiesmanager.h"

IdentityActivities::IdentityActivities(ActivitiesManager *manager)
    : KIdentityManagementCore::IdentityActivitiesAbstract{manager}
    , mActivitiesManager(manager)
{
}

IdentityActivities::~IdentityActivities() = default;

bool IdentityActivities::filterAcceptsRow(const QStringList &activities) const
{
    if (!hasActivitySupport()) {
        return true;
    }
    return !activities.isEmpty() && mActivitiesManager->isInCurrentActivity(activities);
}

bool IdentityActivities::hasActivitySupport() const
{
    return mActivitiesManager ? mActivitiesManager->enabled() : false;
}

QString IdentityActivities::currentActivity() const
{
    return mActivitiesManager ? mActivitiesManager->currentActivity() : QString();
}

#include "moc_identityactivities.cpp"
