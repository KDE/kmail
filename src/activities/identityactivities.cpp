/*
  SPDX-FileCopyrightText: 2024 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
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
    if (mActivitiesManager && mActivitiesManager->enabled()) {
        if (!activities.isEmpty()) {
            return mActivitiesManager->isInCurrentActivity(activities);
        } else {
            return false;
        }
    }
    return true;
}

bool IdentityActivities::hasActivitySupport() const
{
    return mActivitiesManager->enabled();
}

QString IdentityActivities::currentActivity() const
{
    return mActivitiesManager->currentActivity();
}

#include "moc_identityactivities.cpp"
