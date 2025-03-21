/*
   SPDX-FileCopyrightText: 2021-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once
#include <Akonadi/AgentWidgetBase>

class MailMergeManager;

class MailMergeAgent : public Akonadi::AgentWidgetBase, public Akonadi::AgentBase::ObserverV3
{
    Q_OBJECT
public:
    explicit MailMergeAgent(const QString &id);
    ~MailMergeAgent() override;

    [[nodiscard]] QString printDebugInfo() const;

    void setEnableAgent(bool b);
    [[nodiscard]] bool enabledAgent() const;

    void removeItem(qint64 item);
Q_SIGNALS:
    void needUpdateConfigDialogBox();

public Q_SLOTS:
    void reload();
    void configure(WId windowId) override;

protected:
    void addItem(qint64 timestamp, bool recurrence, int recurrenceValue, int recurrenceUnit, Akonadi::Item::Id id, const QString &subject, const QString &to);
    void itemsRemoved(const Akonadi::Item::List &item) override;
    void itemsMoved(const Akonadi::Item::List &items, const Akonadi::Collection &sourceCollection, const Akonadi::Collection &destinationCollection) override;
    void doSetOnline(bool online) override;

private:
    void slotStartAgent();
    MailMergeManager *const mManager;
    bool mAgentInitialized = false;
};
