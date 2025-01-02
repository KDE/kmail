/*
  SPDX-FileCopyrightText: 2024-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include <MailTransport/TransportActivitiesAbstract>
class ActivitiesManager;
class TransportActivities : public MailTransport::TransportActivitiesAbstract
{
    Q_OBJECT
public:
    explicit TransportActivities(ActivitiesManager *manager);
    ~TransportActivities() override;

    [[nodiscard]] bool filterAcceptsRow(const QStringList &activities) const override;
    [[nodiscard]] bool hasActivitySupport() const override;
    [[nodiscard]] QString currentActivity() const override;

private:
    ActivitiesManager *const mActivitiesManager;
};
