/*
  SPDX-FileCopyrightText: 2024 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "activitiesmanager.h"
#include "kmail_activities_debug.h"
#include "transportactivities.h"

#include <PlasmaActivities/Consumer>

ActivitiesManager::ActivitiesManager(QObject *parent)
    : QObject{parent}
    , mTransportActivities(new TransportActivities(this))
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
}

ActivitiesManager::~ActivitiesManager() = default;

bool ActivitiesManager::enabled() const
{
    return mEnabled;
}

void ActivitiesManager::setEnabled(bool newEnabled)
{
    if (mEnabled != newEnabled) {
        mEnabled = newEnabled;
        mTransportActivities->setEnabled(newEnabled);
    }
}

TransportActivities *ActivitiesManager::transportActivities() const
{
    return mTransportActivities;
}

#include "moc_activitiesmanager.cpp"
