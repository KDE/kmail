/*
  SPDX-FileCopyrightText: 2024 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "activitiesmanager.h"
#include <PlasmaActivities/Consumer>
ActivitiesManager::ActivitiesManager(QObject *parent)
    : QObject{parent}
    , mActivitiesConsumer(new KActivities::Consumer(this))
{
}

ActivitiesManager::~ActivitiesManager() = default;

bool ActivitiesManager::enabled() const
{
    return mEnabled;
}

void ActivitiesManager::setEnabled(bool newEnabled)
{
    mEnabled = newEnabled;
}

#include "moc_activitiesmanager.cpp"
