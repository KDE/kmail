/*
  SPDX-FileCopyrightText: 2024-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KLDAPCore/LdapActivitiesAbstract>
class ActivitiesManager;
class LdapActivities : public KLDAPCore::LdapActivitiesAbstract
{
    Q_OBJECT
public:
    explicit LdapActivities(ActivitiesManager *manager);
    ~LdapActivities() override;

    [[nodiscard]] bool filterAcceptsRow(const QStringList &activities) const override;
    [[nodiscard]] bool hasActivitySupport() const override;
    [[nodiscard]] QString currentActivity() const override;

private:
    ActivitiesManager *const mActivitiesManager;
};
