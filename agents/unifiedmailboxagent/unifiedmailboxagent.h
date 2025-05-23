/*
   SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/ResourceWidgetBase>
#include <Akonadi/SpecialMailCollections>

#include "unifiedmailboxmanager.h"

#include <QHash>

/* Despite its name, this is actually a Resource, but it acts as an Agent: it
 * listens to notifications about Items that belong to other resources and acts
 * on them.
 * The only reason this agent has to implement ResourceBase is to be able to own
 * the virtual unified collections into which content of other collections is
 * linked.
 */
class UnifiedMailboxAgent : public Akonadi::ResourceWidgetBase
{
    Q_OBJECT

public:
    explicit UnifiedMailboxAgent(const QString &id);
    ~UnifiedMailboxAgent() override = default;

    void configure(WId windowId) override;

    void setEnableAgent(bool enable);
    [[nodiscard]] bool enabledAgent() const;

    void retrieveCollections() override;
    void retrieveItems(const Akonadi::Collection &collection) override;
    [[nodiscard]] bool retrieveItems(const Akonadi::Item::List &items, const QSet<QByteArray> &parts) override;
    [[nodiscard]] bool retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts) override;

private:
    void delayedInit();

    void fixSpecialCollections();
    void fixSpecialCollection(const QString &colId, Akonadi::SpecialMailCollections::Type type);

    UnifiedMailboxManager mBoxManager;
};
