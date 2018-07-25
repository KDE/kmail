/*
   Copyright (C) 2018 Daniel Vrátil <dvratil@kde.org>

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

#ifndef UNIFIEDMAILBOXAGENT_H
#define UNIFIEDMAILBOXAGENT_H

#include <AkonadiAgentBase/ResourceBase>
#include <Akonadi/KMime/SpecialMailCollections>

#include "unifiedmailboxmanager.h"

#include <QHash>
#include <QVector>

/* Despite its name, this is actually a Resource, but it acts as an Agent: it
 * listens to notifications about Items that belong to other resources and acts
 * on them.
 * The only reason this agent has to implement ResourceBase is to be able to own
 * the virtual unified collections into which content of other collections is
 * linked.
 */
class UnifiedMailboxAgent : public Akonadi::ResourceBase
                          , public Akonadi::AgentBase::ObserverV4
{
    Q_OBJECT

public:
    explicit UnifiedMailboxAgent(const QString &id);
    ~UnifiedMailboxAgent() override = default;

    void configure(WId windowId) override;

    void retrieveCollections() override;
    void retrieveItems(const Akonadi::Collection &collection) override;
    bool retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts) override;

    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override;
    void itemsMoved(const Akonadi::Item::List &items, const Akonadi::Collection &sourceCollection,
                    const Akonadi::Collection &destinationCollection) override;

private Q_SLOTS:
    void delayedInit();
    void checkForMissingItems(const QVariant & /* dummy */);
    void rediscoverLocalBoxes(const QVariant & /* dummy */);

private:
    void fixSpecialCollections();
    void fixSpecialCollection(const QString &colId, Akonadi::SpecialMailCollections::Type type);

    UnifiedMailboxManager mBoxManager;
};

#endif
