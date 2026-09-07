/*
  SPDX-FileCopyrightText: 2024-2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
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
    if (!hasActivitySupport()) {
        return true;
    }
    return !activities.isEmpty() && mActivitiesManager->isInCurrentActivity(activities);
}

bool LdapActivities::hasActivitySupport() const
{
    return mActivitiesManager ? mActivitiesManager->enabled() : false;
}

QString LdapActivities::currentActivity() const
{
    return mActivitiesManager ? mActivitiesManager->currentActivity() : QString();
}

#include "moc_ldapactivities.cpp"
