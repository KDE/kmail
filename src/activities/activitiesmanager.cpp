/*
  SPDX-FileCopyrightText: 2024 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "activitiesmanager.h"
#include "identityactivities.h"
#include "kmail_activities_debug.h"
#include "ldapactivities.h"
#include "transportactivities.h"

#include <PlasmaActivities/Consumer>

ActivitiesManager::ActivitiesManager(QObject *parent)
    : QObject{parent}
    , mTransportActivities(new TransportActivities(this))
    , mIdentityActivities(new IdentityActivities(this))
    , mLdapActivities(new LdapActivities(this))
    , mActivitiesConsumer(new KActivities::Consumer(this))
{
    connect(mActivitiesConsumer, &KActivities::Consumer::currentActivityChanged, this, [this](const QString &activityId) {
        qCDebug(KMAIL_ACTIVITIES_LOG) << " switch to activity " << activityId;
        Q_EMIT activitiesChanged();
    });
    connect(mActivitiesConsumer, &KActivities::Consumer::activityRemoved, this, [this](const QString &activityId) {
        qCDebug(KMAIL_ACTIVITIES_LOG) << " Activity removed " << activityId;
        Q_EMIT activitiesChanged();
    });
    connect(mActivitiesConsumer, &KActivities::Consumer::serviceStatusChanged, this, &ActivitiesManager::activitiesChanged);
    if (mActivitiesConsumer->serviceStatus() != KActivities::Consumer::ServiceStatus::Running) {
        qCWarning(KMAIL_ACTIVITIES_LOG) << "Plasma activities is not running: " << mActivitiesConsumer->serviceStatus();
    }
    connect(this, &ActivitiesManager::activitiesChanged, this, [this]() {
        Q_EMIT mIdentityActivities->activitiesChanged();
        Q_EMIT mTransportActivities->activitiesChanged();
        Q_EMIT mLdapActivities->activitiesChanged();
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

bool ActivitiesManager::enabled() const
{
    return mEnabled;
}

void ActivitiesManager::setEnabled(bool newEnabled)
{
    if (mEnabled != newEnabled) {
        mEnabled = newEnabled;
        Q_EMIT activitiesChanged();
    }
}

TransportActivities *ActivitiesManager::transportActivities() const
{
    return mTransportActivities;
}

bool ActivitiesManager::isInCurrentActivity(const QStringList &lst) const
{
    if (mActivitiesConsumer->serviceStatus() == KActivities::Consumer::ServiceStatus::Running) {
        if (lst.contains(mActivitiesConsumer->currentActivity())) {
            return true;
        } else {
            const QStringList activities = mActivitiesConsumer->activities();
            auto index = std::find_if(activities.constBegin(), activities.constEnd(), [lst](const QString &str) {
                return lst.contains(str);
            });
            // Account doesn't contains valid activities => show it.
            if (index == activities.constEnd()) {
                return true;
            }
            return false;
        }
    } else {
        return true;
    }
}

QString ActivitiesManager::currentActivity() const
{
    return mActivitiesConsumer->currentActivity();
}

LdapActivities *ActivitiesManager::ldapActivities() const
{
    return mLdapActivities;
}

#include "moc_activitiesmanager.cpp"
