/*
  SPDX-FileCopyrightText: 2024-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "activitiesmanager.h"
#include "accountactivities.h"
#include "identityactivities.h"
#include "ldapactivities.h"
#include "transportactivities.h"

ActivitiesManager::ActivitiesManager(QObject *parent)
    : PimCommonActivities::ActivitiesBaseManager{parent}
    , mTransportActivities(new TransportActivities(this))
    , mIdentityActivities(new IdentityActivities(this))
    , mLdapActivities(new LdapActivities(this))
    , mAccountActivities(new AccountActivities(this))
{
    connect(this, &ActivitiesManager::activitiesChanged, this, [this]() {
        Q_EMIT mIdentityActivities->activitiesChanged();
        Q_EMIT mTransportActivities->activitiesChanged();
        Q_EMIT mLdapActivities->activitiesChanged();
        Q_EMIT mAccountActivities->activitiesChanged();
    });
}

ActivitiesManager::~ActivitiesManager() = default;

ActivitiesManager *ActivitiesManager::self()
{
    static ActivitiesManager s_self;
    return &s_self;
}

IdentityActivities *ActivitiesManager::identityActivities() const
{
    return mIdentityActivities;
}

TransportActivities *ActivitiesManager::transportActivities() const
{
    return mTransportActivities;
}

LdapActivities *ActivitiesManager::ldapActivities() const
{
    return mLdapActivities;
}

AccountActivities *ActivitiesManager::accountActivities() const
{
    return mAccountActivities;
}

#include "moc_activitiesmanager.cpp"
