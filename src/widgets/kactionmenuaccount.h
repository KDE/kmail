/*
   SPDX-FileCopyrightText: 2015-2024 Laurent Montel <montel@kde.org>

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
#if HAVE_ACTIVITY_SUPPORT
class AccountActivities;
#endif
class KActionMenuAccount : public KActionMenu
{
    Q_OBJECT
public:
    explicit KActionMenuAccount(QObject *parent = nullptr);
    ~KActionMenuAccount() override;

    void setAccountOrder(const QStringList &identifier);
#if HAVE_ACTIVITY_SUPPORT
    void setAccountActivitiesAbstract(AccountActivities *activities);
#endif
private:
    void updateAccountMenu();
    void slotCheckTransportMenu();
    void slotSelectAccount(QAction *act);

    QStringList mOrderIdentifier;
    bool mInitialized = false;
#if HAVE_ACTIVITY_SUPPORT
    AccountActivities *mAccountActivities = nullptr;
#endif
};
