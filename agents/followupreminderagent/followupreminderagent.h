/*
   Copyright (C) 2014-2016 Montel Laurent <montel@kde.org>

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
    ~FollowUpReminderAgent();

    void setEnableAgent(bool b);
    bool enabledAgent() const;

    void showConfigureDialog(qlonglong windowId = 0);

    QString printDebugInfo();

public Q_SLOTS:
    void configure(WId windowId) Q_DECL_OVERRIDE;
    void reload();

protected:
    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) Q_DECL_OVERRIDE;

private:
    FollowUpReminderManager *mManager;
    QTimer *mTimer;
};

#endif // FOLLOWUPREMINDERAGENT_H
