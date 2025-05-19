/*
   SPDX-FileCopyrightText: 2015-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "config-kmail.h"
#include <KActionMenu>

class AgentIdentifier
{
public:
    AgentIdentifier() = default;

    AgentIdentifier(const QString &identifier, const QString &name, int index = -1)
        : mIdentifier(identifier)
        , mName(name)
        , mIndex(index)
    {
    }

    QString mIdentifier;
    QString mName;
    int mIndex = -1;
};
#if KMAIL_HAVE_ACTIVITY_SUPPORT
class AccountActivities;
#endif
class KActionMenuAccount : public KActionMenu
{
    Q_OBJECT
public:
    explicit KActionMenuAccount(QObject *parent = nullptr);
    ~KActionMenuAccount() override;

    void setAccountsOrder(const QStringList &orderedAccountIdentifiers);
#if KMAIL_HAVE_ACTIVITY_SUPPORT
    void setAccountActivitiesAbstract(AccountActivities *activities);
#endif
private:
    void updateAccountMenu();
    void slotCheckTransportMenu();
    void slotSelectAccount(QAction *act);
    void forceUpdateAccountMenu();

    QStringList mOrderedAccountIdentifiers;
    bool mInitialized = false;
#if KMAIL_HAVE_ACTIVITY_SUPPORT
    AccountActivities *mAccountActivities = nullptr;
#endif
};
