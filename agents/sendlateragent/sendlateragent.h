/*
   Copyright (C) 2013-2020 Laurent Montel <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef SENDLATERAGENT_H
#define SENDLATERAGENT_H

#include <agentbase.h>
#include "sendlaterdialog.h"

class SendLaterManager;

class SendLaterAgent : public Akonadi::AgentBase, public Akonadi::AgentBase::ObserverV3
{
    Q_OBJECT
public:
    explicit SendLaterAgent(const QString &id);
    ~SendLaterAgent() override;

    Q_REQUIRED_RESULT QString printDebugInfo() const;

    void setEnableAgent(bool b);
    Q_REQUIRED_RESULT bool enabledAgent() const;

Q_SIGNALS:
    void needUpdateConfigDialogBox();

public Q_SLOTS:
    void reload();
    void configure(WId windowId) override;
    void removeItem(qint64 item);

protected:
    void itemsRemoved(const Akonadi::Item::List &item) override;
    void itemsMoved(const Akonadi::Item::List &items, const Akonadi::Collection &sourceCollection, const Akonadi::Collection &destinationCollection) override;
    void doSetOnline(bool online) override;

private:
    void slotSendNow(Akonadi::Item::Id id);
    void slotStartAgent();
    bool mAgentInitialized;
    SendLaterManager *mManager = nullptr;
};

#endif // SENDLATERAGENT_H
