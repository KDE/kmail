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
#include "unifiedmailboxagent_debug.h"
#include "settingsdialog.h"
#include "settings.h"

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

#include <memory>
#include <unordered_set>
#include <chrono>

namespace {
static const auto MailMimeType = QStringLiteral("message/rfc822");
static const auto Inbox = QStringLiteral("inbox");
static const auto Sent = QStringLiteral("sent-mail");
static const auto Drafts = QStringLiteral("drafts");
}


UnifiedMailboxAgent::UnifiedMailboxAgent(const QString &id)
    : Akonadi::ResourceBase(id)
    , mBoxManager(config())
{
    setAgentName(i18n("Unified Mailboxes"));

    // HACK: Act as if we were an Agent instead of a Resource and listen to changes
    // in everyone's Items instead of just ours
    changeRecorder()->setResourceMonitored(id.toUtf8(), false);

    // We are only interested in emails...
    changeRecorder()->setMimeTypeMonitored(MailMimeType);
    changeRecorder()->setTypeMonitored(Akonadi::Monitor::Items);

    auto &ifs = changeRecorder()->itemFetchScope();
    ifs.setAncestorRetrieval(Akonadi::ItemFetchScope::None);
    ifs.setCacheOnly(true);
    ifs.fetchFullPayload(false);

    QMetaObject::invokeMethod(this, "delayedInit", Qt::QueuedConnection);
}

void UnifiedMailboxAgent::configure(WId windowId)
{
    QPointer<UnifiedMailboxAgent> agent(this);
    if (SettingsDialog(config(), mBoxManager, windowId).exec() && agent) {
        QMetaObject::invokeMethod(this, "delayedInit", Qt::QueuedConnection);
        Q_EMIT configurationDialogAccepted();
    }
}

void UnifiedMailboxAgent::delayedInit()
{
    qCDebug(agent_log) << "init";

    fixSpecialCollections();
    mBoxManager.loadBoxes([this]() {
        // start monitoring all source collections
        for (const auto &box : mBoxManager) {
            const auto sources = box.sourceCollections();
            for (auto source : sources) {
                changeRecorder()->setCollectionMonitored(Akonadi::Collection(source), true);
            }
        }

        // boxes are loaded, schedule tree sync and afterwards check for missing items
        synchronizeCollectionTree();
        scheduleCustomTask(this, "rediscoverLocalBoxes", {});
        scheduleCustomTask(this, "checkForMissingItems", {});
    });
}

void UnifiedMailboxAgent::checkForMissingItems(const QVariant & /* dummy */)
{
    const auto lastSeenEvent = QDateTime::fromTime_t(Settings::self()->lastSeenEvent());
    qCDebug(agent_log) << "Checking for missing Items in boxes, last seen event was on" << lastSeenEvent;

    // First check all source Collections that we monitor and verify that all items
    // are linked into a box that matches the given Collection
    const auto collections = changeRecorder()->collectionsMonitored();
    Q_EMIT status(Running, i18n("Checking if unified boxes are up to date"));
    for (const auto &collection : collections) {
        auto unifiedBox = mBoxManager.unifiedMailboxForSource(collection.id());
        if (!unifiedBox) {
            qCWarning(agent_log) << "Failed to retrieve box ID for collection " << collection.id();
            continue;
        }
        Akonadi::Collection unifiedCollection(mBoxManager.collectionIdForUnifiedMailbox(unifiedBox->id()));
        qCDebug(agent_log) << "\tChecking source collection" << collection.id();
        auto fetch = new Akonadi::ItemFetchJob(collection, this);
        fetch->setDeliveryOption(Akonadi::ItemFetchJob::EmitItemsInBatches);
        // Optimize: we could've only missed events that occured since the last time we saw one
        //fetch->fetchScope().setFetchChangedSince(lastSeenEvent);
        fetch->fetchScope().setFetchVirtualReferences(true);
        fetch->fetchScope().setCacheOnly(true);
        connect(fetch, &Akonadi::ItemFetchJob::itemsReceived,
                this, [this, unifiedCollection](const Akonadi::Item::List &items) {
                    Akonadi::Item::List toLink;
                    std::copy_if(items.cbegin(), items.cend(), std::back_inserter(toLink),
                        [&unifiedCollection](const Akonadi::Item &item) {
                            return !item.virtualReferences().contains(unifiedCollection);
                        });
                    if (!toLink.isEmpty()) {
                        new Akonadi::LinkJob(unifiedCollection, toLink, this);
                    }
                });
    }

    // Then check content of all boxes and verify that each Item still belongs to
    // a Collection that is part of the box
    for (const auto &box : mBoxManager) {
        qCDebug(agent_log) << "\tChecking box" << box.name();
        std::unordered_set<qint64> boxSources;
        const auto sourceList = box.sourceCollections();
        boxSources.reserve(sourceList.size());
        std::copy(sourceList.cbegin(), sourceList.cend(), std::inserter(boxSources, boxSources.end()));

        Akonadi::Collection unifiedCollection(mBoxManager.collectionIdForUnifiedMailbox(box.id()));
        auto fetch = new Akonadi::ItemFetchJob(unifiedCollection, this);
        fetch->setDeliveryOption(Akonadi::ItemFetchJob::EmitItemsInBatches);
        // TODO: Does this optimization here make sense?
        //fetch->fetchScope().setFetchChangedSince(lastSeenEvent);
        fetch->fetchScope().setCacheOnly(true);
        fetch->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
        connect(fetch, &Akonadi::ItemFetchJob::itemsReceived,
                this, [this, boxSources = std::move(boxSources), unifiedCollection](const Akonadi::Item::List &items) {
                    Akonadi::Item::List toUnlink;
                    std::copy_if(items.cbegin(), items.cend(), std::back_inserter(toUnlink),
                        [&boxSources](const Akonadi::Item &item) {
                            return boxSources.find(item.storageCollectionId()) == boxSources.cend();
                        });
                    if (!toUnlink.isEmpty()) {
                        new Akonadi::UnlinkJob(unifiedCollection, toUnlink, this);
                    }
                });
    }
    Q_EMIT status(Idle);
    taskDone();
}

void UnifiedMailboxAgent::rediscoverLocalBoxes(const QVariant &)
{
    mBoxManager.discoverBoxCollections([this]() {
        taskDone();
    });
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

    for (const auto &box : mBoxManager) {
        Akonadi::Collection col;
        col.setName(box.id());
        col.setRemoteId(box.id());
        col.setParentCollection(topLevel);
        col.setContentMimeTypes({MailMimeType});
        col.setRights(Akonadi::Collection::CanChangeItem);
        col.setVirtual(true);
        auto displayAttr = col.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Collection::AddIfMissing);
        displayAttr->setDisplayName(box.name());
        displayAttr->setIconName(box.icon());
        collections.push_back(std::move(col));
    }

    // TODO: Support custom folders

    collectionsRetrieved(std::move(collections));

    scheduleCustomTask(this, "rediscoverLocalBoxes", {});
}

void UnifiedMailboxAgent::retrieveItems(const Akonadi::Collection &c)
{
    // TODO: Check if our box is up-to-date
    cancelTask();
}

bool UnifiedMailboxAgent::retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts)
{
    // This method should never be called by Akonadi
    Q_UNUSED(parts);
    qCWarning(agent_log) << "retrieveItem() for item" << item.id() << "called but we can't own any items! This is a bug in Akonadi";
    return false;
}

void UnifiedMailboxAgent::itemAdded(const Akonadi::Item &item,
                                    const Akonadi::Collection &collection)
{
    // We are subscribed only to our source collections so we will only be notified
    // when an item is added into one of them.

    const auto box = mBoxManager.unifiedMailboxForSource(collection.id());
    if (!box) {
        qCWarning(agent_log) << "Failed to find unified mailbox for source collection " << collection.id();
        cancelTask();
        return;
    }

    const auto boxId = mBoxManager.collectionIdForUnifiedMailbox(box->id());
    if (boxId > -1) {
        new Akonadi::LinkJob(Akonadi::Collection{boxId}, {item}, this);
    }
    changeProcessed();

    Settings::self()->setLastSeenEvent(std::chrono::steady_clock::now().time_since_epoch().count());
}

void UnifiedMailboxAgent::itemsMoved(const Akonadi::Item::List&items,
                                     const Akonadi::Collection &sourceCollection,
                                     const Akonadi::Collection &destinationCollection)
{
    const auto srcBox = mBoxManager.unifiedMailboxForSource(sourceCollection.id());
    const auto dstBox = mBoxManager.unifiedMailboxForSource(destinationCollection.id());
    if (srcBox) {
        // Move source collection was our source, unlink the Item from a box
        const auto srcBoxId = mBoxManager.collectionIdForUnifiedMailbox(srcBox->id());
        new Akonadi::UnlinkJob(Akonadi::Collection{srcBoxId}, items, this);
    }

    if (dstBox) {
        // Move destination collection is our source, link the Item into a box
        const auto dstBoxId = mBoxManager.collectionIdForUnifiedMailbox(dstBox->id());
        new Akonadi::LinkJob(Akonadi::Collection{dstBoxId}, items, this);
    }

    changeProcessed();

    Settings::self()->setLastSeenEvent(std::chrono::steady_clock::now().time_since_epoch().count());
}

void UnifiedMailboxAgent::fixSpecialCollection(const QString &colId, Akonadi::SpecialMailCollections::Type type)
{
    if (colId.isEmpty()) {
        return;
    }
    const auto id = colId.toLongLong();
    // SpecialMailCollection requires the Collection to have a Resource set as well, so
    // we have to retrieve it first.
    auto fetch = new Akonadi::CollectionFetchJob(Akonadi::Collection(id), Akonadi::CollectionFetchJob::Base, this);
    connect(fetch, &Akonadi::CollectionFetchJob::collectionsReceived,
            this, [this, type](const Akonadi::Collection::List &cols) {
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
