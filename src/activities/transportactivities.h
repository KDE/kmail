/*
  SPDX-FileCopyrightText: 2024 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include <MailTransport/TransportActivitiesAbstract>

class TransportActivities : public MailTransport::TransportActivitiesAbstract
{
    Q_OBJECT
public:
    explicit TransportActivities(QObject *parent = nullptr);
    ~TransportActivities() override;

    [[nodiscard]] bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    [[nodiscard]] bool hasActivitySupport() const override;
};
