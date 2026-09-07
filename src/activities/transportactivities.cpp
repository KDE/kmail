/*
  SPDX-FileCopyrightText: 2024-2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
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
    if (!hasActivitySupport()) {
        return true;
    }
    return !activities.isEmpty() && mActivitiesManager->isInCurrentActivity(activities);
}

bool TransportActivities::hasActivitySupport() const
{
    return mActivitiesManager ? mActivitiesManager->enabled() : false;
}

QString TransportActivities::currentActivity() const
{
    return mActivitiesManager ? mActivitiesManager->currentActivity() : QString();
}

#include "moc_transportactivities.cpp"
