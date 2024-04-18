/*
  SPDX-FileCopyrightText: 2024 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include <KIdentityManagementCore/IdentityActivitiesAbstract>

class IdentityActivities : public KIdentityManagementCore::IdentityActivitiesAbstract
{
    Q_OBJECT
public:
    explicit IdentityActivities(QObject *parent = nullptr);
    ~IdentityActivities() override;

    [[nodiscard]] bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    [[nodiscard]] bool hasActivitySupport() const override;
};
