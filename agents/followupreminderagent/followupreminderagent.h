/*
   Copyright (C) 2014-2019 Montel Laurent <montel@kde.org>

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

#ifndef FOLLOWUPREMINDERAGENT_H
#define FOLLOWUPREMINDERAGENT_H

#include <AkonadiAgentBase/agentbase.h>
class FollowUpReminderManager;
class FollowUpReminderAgent : public Akonadi::AgentBase, public Akonadi::AgentBase::ObserverV3
{
    Q_OBJECT
public:
    explicit FollowUpReminderAgent(const QString &id);
    ~FollowUpReminderAgent() override;

    void setEnableAgent(bool b);
    bool enabledAgent() const;

    QString printDebugInfo();

public Q_SLOTS:
    void reload();

protected:
    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override;

private:
    FollowUpReminderManager *mManager = nullptr;
    QTimer *mTimer = nullptr;
};

#endif // FOLLOWUPREMINDERAGENT_H
