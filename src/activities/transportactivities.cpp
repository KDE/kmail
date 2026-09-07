/*
  SPDX-FileCopyrightText: 2024-2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "transportactivities.h"
#include "activitiesmanager.h"

TransportActivities::TransportActivities(ActivitiesManager *manager)
    : PimCommonActivities::ActivitiesFilter<MailTransport::TransportActivitiesAbstract>{manager}
{
}

TransportActivities::~TransportActivities() = default;

#include "moc_transportactivities.cpp"
