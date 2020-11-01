/*
   SPDX-FileCopyrightText: 2015-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KACTIONMENUACCOUNT_H
#define KACTIONMENUACCOUNT_H

#include <KActionMenu>

class AgentIdentifier
{
public:
    AgentIdentifier()
    = default;

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

class KActionMenuAccount : public KActionMenu
{
    Q_OBJECT
public:
    explicit KActionMenuAccount(QObject *parent = nullptr);
    ~KActionMenuAccount();

    void setAccountOrder(const QStringList &identifier);

private:
    void updateAccountMenu();
    void slotCheckTransportMenu();
    void slotSelectAccount(QAction *act);

    QStringList mOrderIdentifier;
    bool mInitialized = false;
};

#endif // KACTIONMENUACCOUNT_H
