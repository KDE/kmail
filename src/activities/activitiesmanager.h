/*
  SPDX-FileCopyrightText: 2024-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once
#include "kmail_private_export.h"
#include <PimCommonActivities/ActivitiesBaseManager>
#include <QObject>
class TransportActivities;
class IdentityActivities;
class LdapActivities;
class AccountActivities;
class KMAILTESTS_TESTS_EXPORT ActivitiesManager : public PimCommonActivities::ActivitiesBaseManager
{
    Q_OBJECT
public:
    static ActivitiesManager *self();

    explicit ActivitiesManager(QObject *parent = nullptr);
    ~ActivitiesManager() override;

    [[nodiscard]] TransportActivities *transportActivities() const;

    [[nodiscard]] IdentityActivities *identityActivities() const;

    [[nodiscard]] LdapActivities *ldapActivities() const;

    [[nodiscard]] AccountActivities *accountActivities() const;

private:
    TransportActivities *const mTransportActivities;
    IdentityActivities *const mIdentityActivities;
    LdapActivities *const mLdapActivities;
    AccountActivities *const mAccountActivities;
};
