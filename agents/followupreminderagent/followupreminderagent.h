/*
   SPDX-FileCopyrightText: 2014-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/AgentBase>
#include <QDate>
class FollowUpReminderManager;
class FollowUpReminderAgent : public Akonadi::AgentBase, public Akonadi::AgentBase::ObserverV3
{
    Q_OBJECT
public:
    explicit FollowUpReminderAgent(const QString &id);
    ~FollowUpReminderAgent() override;

    void setEnableAgent(bool b);
    [[nodiscard]] bool enabledAgent() const;

    [[nodiscard]] QString printDebugInfo() const;

public Q_SLOTS:
    void reload();
    void addReminder(const QString &messageId,
                     Akonadi::Item::Id messageItemId,
                     const QString &to,
                     const QString &subject,
                     QDate followupDate,
                     Akonadi::Item::Id todoId);

protected:
    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override;

private:
    FollowUpReminderManager *const mManager;
    QTimer *const mTimer;
};
