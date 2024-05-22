/*
  SPDX-FileCopyrightText: 2024 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "accountactivities.h"
#include "activitiesmanager.h"

AccountActivities::AccountActivities(ActivitiesManager *manager)
    : Akonadi::AccountActivitiesAbstract{manager}
    , mActivitiesManager(manager)
{
}

AccountActivities::~AccountActivities() = default;

bool AccountActivities::filterAcceptsRow(const QStringList &activities) const
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

bool AccountActivities::hasActivitySupport() const
{
    return mActivitiesManager->enabled();
}

QString AccountActivities::currentActivity() const
{
    return mActivitiesManager->currentActivity();
}

#include "moc_accountactivities.cpp"
