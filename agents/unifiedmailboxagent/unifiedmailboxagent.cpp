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

#include "unifiedmailboxagent.h"
#include "unifiedmailbox.h"
#include "unifiedmailboxagent_debug.h"
#include "settingsdialog.h"
#include "settings.h"
#include "common.h"

#include <AkonadiCore/ChangeRecorder>
#include <AkonadiCore/Session>
#include <AkonadiCore/CollectionFetchJob>
#include <AkonadiCore/CollectionFetchScope>
#include <AkonadiCore/SpecialCollectionAttribute>
#include <AkonadiCore/EntityDisplayAttribute>
#include <AkonadiCore/ItemFetchScope>
#include <AkonadiCore/ItemFetchJob>
#include <AkonadiCore/LinkJob>
#include <AkonadiCore/UnlinkJob>
#include <Akonadi/KMime/SpecialMailCollections>

#include <KIdentityManagement/IdentityManager>
#include <KIdentityManagement/Identity>

#include <KLocalizedString>

#include <QPointer>
#include <QTimer>

#include <memory>
#include <unordered_set>
#include <chrono>


UnifiedMailboxAgent::UnifiedMailboxAgent(const QString &id)
    : Akonadi::ResourceBase(id)
    , mBoxManager(config())
{
    setAgentName(i18n("Unified Mailboxes"));

    connect(&mBoxManager, &UnifiedMailboxManager::updateBox,
            this, [this](const UnifiedMailbox *box) {
                if (box->collectionId() <= -1) {
                    qCWarning(agent_log) << "MailboxManager wants us to update Box but does not have its CollectionId!?";
                    return;
                }

                // Schedule collection sync for the box
                synchronizeCollection(box->collectionId());
            });

    auto &ifs = changeRecorder()->itemFetchScope();
    ifs.setAncestorRetrieval(Akonadi::ItemFetchScope::None);
    ifs.setCacheOnly(true);
    ifs.fetchFullPayload(false);

    QTimer::singleShot(0, this, [this]() {
        qCDebug(agent_log) << "delayed init";

        fixSpecialCollections();
        mBoxManager.loadBoxes([this]() {
            // boxes loaded, let's sync up
            synchronize();
        });
    });
}

void UnifiedMailboxAgent::configure(WId windowId)
{
    QPointer<UnifiedMailboxAgent> agent(this);
    if (SettingsDialog(config(), mBoxManager, windowId).exec() && agent) {
        mBoxManager.saveBoxes();
        synchronize();
        Q_EMIT configurationDialogAccepted();
    } else {
        mBoxManager.loadBoxes();
    }
}

void UnifiedMailboxAgent::retrieveCollections()
{
    Akonadi::Collection::List collections;

    Akonadi::Collection topLevel;
    topLevel.setName(identifier());
    topLevel.setRemoteId(identifier());
    topLevel.setParentCollection(Akonadi::Collection::root());
    topLevel.setContentMimeTypes({Akonadi::Collection::mimeType()});
    topLevel.setRights(Akonadi::Collection::ReadOnly);
    auto displayAttr = topLevel.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Collection::AddIfMissing);
    displayAttr->setDisplayName(i18n("Unified Mailboxes"));
    displayAttr->setActiveIconName(QStringLiteral("globe"));
    collections.push_back(topLevel);

    for (const auto &boxIt : mBoxManager) {
        const auto &box = boxIt.second;
        Akonadi::Collection col;
        col.setName(box->id());
        col.setRemoteId(box->id());
        col.setParentCollection(topLevel);
        col.setContentMimeTypes({Common::MailMimeType});
        col.setRights(Akonadi::Collection::CanChangeItem | Akonadi::Collection::CanDeleteItem);
        col.setVirtual(true);
        auto displayAttr = col.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Collection::AddIfMissing);
        displayAttr->setDisplayName(box->name());
        displayAttr->setIconName(box->icon());
        collections.push_back(std::move(col));
    }

    collectionsRetrieved(std::move(collections));

    // Add mapping between boxes and collections
    mBoxManager.discoverBoxCollections();
}

void UnifiedMailboxAgent::retrieveItems(const Akonadi::Collection &c)
{
    // First check that we have all Items from all source collections
    Q_EMIT status(Running, i18n("Synchronizing unified mailbox %1", c.displayName()));
    const auto unifiedBox = mBoxManager.unifiedMailboxFromCollection(c);
    if (!unifiedBox) {
        qCWarning(agent_log) << "Failed to retrieve box ID for collection " << c.id();
        itemsRetrievedIncremental({}, {}); // fake incremental retrieval
        return;
    }

    const auto lastSeenEvent = QDateTime::fromSecsSinceEpoch(c.remoteRevision().toLongLong());

    const auto sources = unifiedBox->sourceCollections();
    for (auto source  : sources) {
        auto fetch = new Akonadi::ItemFetchJob(Akonadi::Collection(source), this);
        fetch->setDeliveryOption(Akonadi::ItemFetchJob::EmitItemsInBatches);
        fetch->fetchScope().setFetchVirtualReferences(true);
        fetch->fetchScope().setCacheOnly(true);
        connect(fetch, &Akonadi::ItemFetchJob::itemsReceived,
                this, [this, c](const Akonadi::Item::List &items) {
                    Akonadi::Item::List toLink;
                    std::copy_if(items.cbegin(), items.cend(), std::back_inserter(toLink),
                        [&c](const Akonadi::Item &item) {
                            return !item.virtualReferences().contains(c);
                        });
                    if (!toLink.isEmpty()) {
                        new Akonadi::LinkJob(c, toLink, this);
                    }
                });
    }

    auto fetch = new Akonadi::ItemFetchJob(c, this);
    fetch->setDeliveryOption(Akonadi::ItemFetchJob::EmitItemsInBatches);
    fetch->fetchScope().setCacheOnly(true);
    fetch->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
    connect(fetch, &Akonadi::ItemFetchJob::itemsReceived,
            this, [this, unifiedBox, c](const Akonadi::Item::List &items) {
                Akonadi::Item::List toUnlink;
                std::copy_if(items.cbegin(), items.cend(), std::back_inserter(toUnlink),
                    [&unifiedBox](const Akonadi::Item &item) {
                        return !unifiedBox->sourceCollections().contains(item.storageCollectionId());
                    });
                if (!toUnlink.isEmpty()) {
                    new Akonadi::UnlinkJob(c, toUnlink, this);
                }
            });
    connect(fetch, &Akonadi::ItemFetchJob::result,
            this, [this]() {
                itemsRetrievedIncremental({}, {}); // fake incremental retrieval
            });
}

bool UnifiedMailboxAgent::retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts)
{
    // This method should never be called by Akonadi
    Q_UNUSED(parts);
    qCWarning(agent_log) << "retrieveItem() for item" << item.id() << "called but we can't own any items! This is a bug in Akonadi";
    return false;
}


void UnifiedMailboxAgent::fixSpecialCollection(const QString &colId, Akonadi::SpecialMailCollections::Type type)
{
    if (colId.isEmpty()) {
        return;
    }
    const auto id = colId.toLongLong();
    // SpecialMailCollection requires the Collection to have a Resource set as well, so
    // we have to retrieve it first.
    connect(new Akonadi::CollectionFetchJob(Akonadi::Collection(id), Akonadi::CollectionFetchJob::Base, this),
            &Akonadi::CollectionFetchJob::collectionsReceived,
            this, [type](const Akonadi::Collection::List &cols) {
                if (cols.count() != 1) {
                    qCWarning(agent_log) << "Identity special collection retrieval did not find a valid collection";
                    return;
                }
                Akonadi::SpecialMailCollections::self()->registerCollection(type, cols.first());
            });
}

void UnifiedMailboxAgent::fixSpecialCollections()
{
    // This is a tiny hack to assign proper SpecialCollectionAttribute to special collections
    // assigned trough Identities. This should happen automatically in KMail when user changes
    // the special collections on the identity page, but until recent master (2018-07-24) this
    // wasn't the case and there's no automatic migration, so we need to fix up manually here.

    if (Settings::self()->fixedSpecialCollections()) {
        return;
    }

    qCDebug(agent_log) << "Fixing special collections assigned from Identities";

    for (const auto &identity : *KIdentityManagement::IdentityManager::self()) {
        if (!identity.disabledFcc()) {
            fixSpecialCollection(identity.fcc(), Akonadi::SpecialMailCollections::SentMail);
        }
        fixSpecialCollection(identity.drafts(), Akonadi::SpecialMailCollections::Drafts);
        fixSpecialCollection(identity.templates(), Akonadi::SpecialMailCollections::Templates);
    }

    Settings::self()->setFixedSpecialCollections(true);
}


AKONADI_RESOURCE_MAIN(UnifiedMailboxAgent)
