/*
  SPDX-FileCopyrightText: 2024-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "ldapactivities.h"
#include "activitiesmanager.h"

LdapActivities::LdapActivities(ActivitiesManager *manager)
    : KLDAPCore::LdapActivitiesAbstract{manager}
    , mActivitiesManager(manager)
{
}

LdapActivities::~LdapActivities() = default;

bool LdapActivities::filterAcceptsRow(const QStringList &activities) const
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

bool LdapActivities::hasActivitySupport() const
{
    return mActivitiesManager->enabled();
}

QString LdapActivities::currentActivity() const
{
    return mActivitiesManager->currentActivity();
}

#include "moc_ldapactivities.cpp"
