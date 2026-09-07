/*
  SPDX-FileCopyrightText: 2024-2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <MailTransport/TransportActivitiesAbstract>
#include <PimCommonActivities/ActivitiesBaseManager>
class ActivitiesManager;
class TransportActivities : public PimCommonActivities::ActivitiesFilter<MailTransport::TransportActivitiesAbstract>
{
    Q_OBJECT
public:
    explicit TransportActivities(ActivitiesManager *manager);
    ~TransportActivities() override;
};
