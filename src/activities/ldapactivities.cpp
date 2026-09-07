/*
  SPDX-FileCopyrightText: 2024-2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ldapactivities.h"
#include "activitiesmanager.h"

LdapActivities::LdapActivities(ActivitiesManager *manager)
    : PimCommonActivities::ActivitiesFilter<KLDAPCore::LdapActivitiesAbstract>{manager}
{
}

LdapActivities::~LdapActivities() = default;

#include "moc_ldapactivities.cpp"
