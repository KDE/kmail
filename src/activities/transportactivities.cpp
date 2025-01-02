/*
  SPDX-FileCopyrightText: 2024-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "transportactivities.h"
#include "activitiesmanager.h"

TransportActivities::TransportActivities(ActivitiesManager *manager)
    : MailTransport::TransportActivitiesAbstract{manager}
    , mActivitiesManager(manager)
{
}

TransportActivities::~TransportActivities() = default;

bool TransportActivities::filterAcceptsRow(const QStringList &activities) const
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

bool TransportActivities::hasActivitySupport() const
{
    return mActivitiesManager->enabled();
}

QString TransportActivities::currentActivity() const
{
    return mActivitiesManager->currentActivity();
}

#include "moc_transportactivities.cpp"
