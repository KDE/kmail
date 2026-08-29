/*
   SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "unifiedmailboxagent.h"
#include "common.h"
#include "settings.h"
#include "settingsdialog.h"
#include "unifiedmailbox.h"
#include "unifiedmailboxagent_debug.h"
#include "unifiedmailboxagentadaptor.h"

#include <Akonadi/CachePolicy>
#include <Akonadi/ChangeRecorder>
#include <Akonadi/CollectionDeleteJob>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/LinkJob>
#include <Akonadi/ServerManager>
#include <Akonadi/Session>
#include <Akonadi/SpecialCollectionAttribute>
#include <Akonadi/UnlinkJob>

#include <KIdentityManagementCore/Identity>
#include <KIdentityManagementCore/IdentityManager>

#include <KLocalizedString>
#include <QDBusConnection>

#include <QHash>
#include <QPointer>
#include <QTimer>

#include <memory>

UnifiedMailboxAgent::UnifiedMailboxAgent(const QString &id)
    : Akonadi::ResourceWidgetBase(id)
    , mBoxManager(config())
{
    setAgentName(i18n("Unified Mailboxes"));

    new UnifiedMailboxAgentAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/UnifiedMailboxAgent"), this, QDBusConnection::ExportAdaptors);
    const auto service = Akonadi::ServerManager::agentServiceName(Akonadi::ServerManager::Resource, identifier());
    QDBusConnection::sessionBus().registerService(service);

    connect(&mBoxManager, &UnifiedMailboxManager::updateBox, this, [this](const UnifiedMailbox *box) {
        if (box->collectionId() <= -1) {
            qCWarning(UNIFIEDMAILBOXAGENT_LOG) << "MailboxManager wants us to update Box but does not have its CollectionId!?";
            return;
        }

        // Schedule collection sync for the box
        synchronizeCollection(box->collectionId());
    });

    auto &ifs = changeRecorder()->itemFetchScope();
    ifs.setAncestorRetrieval(Akonadi::ItemFetchScope::None);
    ifs.setCacheOnly(true);
    ifs.fetchFullPayload(false);

    if (Settings::self()->enabled()) {
        QTimer::singleShot(0, this, &UnifiedMailboxAgent::delayedInit);
    }
}

void UnifiedMailboxAgent::configure(WId windowId)
{
    if (QPointer<UnifiedMailboxAgent> agent(this); agent) {
        SettingsDialog(config(), mBoxManager, windowId).exec();
        synchronize();
        Q_EMIT configurationDialogAccepted();
    }
}

void UnifiedMailboxAgent::delayedInit()
{
    qCDebug(UNIFIEDMAILBOXAGENT_LOG) << "delayed init";

    fixSpecialCollections();
    mBoxManager.loadBoxes([this]() {
        // boxes loaded, let's sync up
        synchronize();
    });
}

bool UnifiedMailboxAgent::enabledAgent() const
{
    return Settings::self()->enabled();
}

void UnifiedMailboxAgent::setEnableAgent(bool enabled)
{
    if (enabled != Settings::self()->enabled()) {
        Settings::self()->setEnabled(enabled);
        Settings::self()->save();
        if (!enabled) {
            setOnline(false);
            auto fetch = new Akonadi::CollectionFetchJob(Akonadi::Collection::root(), Akonadi::CollectionFetchJob::Recursive, this);
            fetch->fetchScope().setResource(identifier());
            connect(fetch, &Akonadi::CollectionFetchJob::collectionsReceived, this, [this](const Akonadi::Collection::List &cols) {
                for (const auto &col : cols) {
                    new Akonadi::CollectionDeleteJob(col, this);
                }
            });
        } else {
            setOnline(true);
            delayedInit();
        }
    }
}

void UnifiedMailboxAgent::retrieveCollections()
{
    if (!Settings::self()->enabled()) {
        collectionsRetrieved({});
        return;
    }

    Akonadi::Collection::List collections;

    Akonadi::Collection topLevel;
    topLevel.setName(identifier());
    topLevel.setRemoteId(identifier());
    topLevel.setParentCollection(Akonadi::Collection::root());
    topLevel.setContentMimeTypes({Akonadi::Collection::mimeType()});
    topLevel.setRights(Akonadi::Collection::ReadOnly);
    auto topLevelDisplayAttr = topLevel.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Collection::AddIfMissing);
    topLevelDisplayAttr->setDisplayName(i18n("Unified Mailboxes"));
    topLevelDisplayAttr->setActiveIconName(QStringLiteral("globe"));
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
        // Periodically re-sync as a fallback in case real-time change notifications
        // are missed, so stale or missing links are eventually corrected.
        Akonadi::CachePolicy cachePolicy;
        cachePolicy.setInheritFromParent(false);
        cachePolicy.setSyncOnDemand(true);
        cachePolicy.setIntervalCheckTime(5); // minutes
        col.setCachePolicy(cachePolicy);
        auto displayAttr = col.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Collection::AddIfMissing);
        displayAttr->setDisplayName(box->name());
        displayAttr->setIconName(box->icon());
        collections.push_back(std::move(col));
    }

    collectionsRetrieved(collections);

    // Add mapping between boxes and collections
    mBoxManager.discoverBoxCollections();
}

void UnifiedMailboxAgent::retrieveItems(const Akonadi::Collection &c)
{
    if (!Settings::self()->enabled()) {
        itemsRetrieved({});
        return;
    }

    // First check that we have all Items from all source collections
    Q_EMIT status(Running, i18n("Synchronizing unified mailbox %1", c.displayName()));
    const auto unifiedBox = mBoxManager.unifiedMailboxFromCollection(c);
    if (!unifiedBox) {
        qCWarning(UNIFIEDMAILBOXAGENT_LOG) << "Failed to retrieve box ID for collection " << c.id();
        itemsRetrievedIncremental({}, {}); // fake incremental retrieval
        return;
    }

    const auto sources = unifiedBox->sourceCollections();

    // We reconcile the unified collection in two phases:
    //
    // Phase 1: fetch all source collections to build a complete map of
    //          source item IDs. Each source is tracked independently so
    //          a fetch failure for one source does not affect the others.
    //
    // Phase 2: fetch the unified collection itself, then:
    //   - unlink items whose source collection is no longer part of the box,
    //     or whose item ID is absent from the (successfully fetched) source.
    //   - link items that are present in a source but missing from the unified
    //     collection.
    //
    // Doing both link and unlink in Phase 2 (rather than linking eagerly in
    // Phase 1) ensures we never issue duplicate LinkJobs even when
    // retrieveItems() is called concurrently by the periodic timer and a
    // real-time notification.

    struct SyncState {
        QHash<qint64, QSet<Akonadi::Item::Id>> sourceItemIdsByCollection;
        QSet<qint64> failedSources;
        int pendingFetches = 0;
    };

    const auto startUnifiedSync = [this, c, unifiedBox](const std::shared_ptr<SyncState> &state) {
        auto fetch = new Akonadi::ItemFetchJob(c, this);
        fetch->setDeliveryOption(Akonadi::ItemFetchJob::EmitItemsInBatches);
        fetch->fetchScope().setCacheOnly(true);
        fetch->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
        connect(fetch, &Akonadi::ItemFetchJob::itemsReceived, this, [this, c, unifiedBox, state](const Akonadi::Item::List &unifiedItems) {
            // Build set of item IDs already in the unified collection.
            QSet<Akonadi::Item::Id> alreadyLinked;
            alreadyLinked.reserve(unifiedItems.size());
            for (const auto &item : unifiedItems) {
                alreadyLinked.insert(item.id());
            }

            Akonadi::Item::List toUnlink;
            for (const auto &item : unifiedItems) {
                const auto srcColId = item.storageCollectionId();
                if (!unifiedBox->sourceCollections().contains(srcColId)) {
                    toUnlink.append(item); // source no longer part of this box
                    continue;
                }
                if (state->failedSources.contains(srcColId)) {
                    continue; // don't remove items whose source fetch failed
                }
                if (const auto idsIt = state->sourceItemIdsByCollection.constFind(srcColId);
                    idsIt != state->sourceItemIdsByCollection.cend() && !idsIt->contains(item.id())) {
                    toUnlink.append(item); // item deleted from source
                }
            }
            if (!toUnlink.isEmpty()) {
                new Akonadi::UnlinkJob(c, toUnlink, this);
            }

            // Link items present in sources but not yet in the unified collection.
            for (auto it = state->sourceItemIdsByCollection.cbegin(); it != state->sourceItemIdsByCollection.cend(); ++it) {
                Akonadi::Item::List toLink;
                for (const auto id : it.value()) {
                    if (!alreadyLinked.contains(id)) {
                        toLink.append(Akonadi::Item(id));
                    }
                }
                if (!toLink.isEmpty()) {
                    new Akonadi::LinkJob(c, toLink, this);
                }
            }
        });
        connect(fetch, &Akonadi::ItemFetchJob::result, this, [this]() {
            itemsRetrievedIncremental({}, {}); // fake incremental retrieval
        });
    };

    if (sources.isEmpty()) {
        startUnifiedSync(std::make_shared<SyncState>());
        return;
    }

    auto state = std::make_shared<SyncState>();
    state->pendingFetches = sources.size();

    for (auto source : sources) {
        auto fetch = new Akonadi::ItemFetchJob(Akonadi::Collection(source), this);
        fetch->setDeliveryOption(Akonadi::ItemFetchJob::EmitItemsInBatches);
        fetch->fetchScope().setCacheOnly(false);
        connect(fetch, &Akonadi::ItemFetchJob::itemsReceived, this, [state, source](const Akonadi::Item::List &items) {
            auto &ids = state->sourceItemIdsByCollection[source];
            for (const auto &item : items) {
                ids.insert(item.id());
            }
        });
        connect(fetch, &Akonadi::ItemFetchJob::result, this, [this, state, source, startUnifiedSync](KJob *job) {
            if (job->error()) {
                state->failedSources.insert(source);
            }
            if (--state->pendingFetches == 0) {
                startUnifiedSync(state);
            }
        });
    }
}

bool UnifiedMailboxAgent::retrieveItems([[maybe_unused]] const Akonadi::Item::List &items, [[maybe_unused]] const QSet<QByteArray> &parts)
{
    qCWarning(UNIFIEDMAILBOXAGENT_LOG) << "retrieveItems() called but we can't own any items! This is a bug in Akonadi";
    return false;
}

bool UnifiedMailboxAgent::retrieveItem(const Akonadi::Item &item, [[maybe_unused]] const QSet<QByteArray> &parts)
{
    // This method should never be called by Akonadi
    qCWarning(UNIFIEDMAILBOXAGENT_LOG) << "retrieveItem() for item" << item.id() << "called but we can't own any items! This is a bug in Akonadi";
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
            this,
            [type](const Akonadi::Collection::List &cols) {
                if (cols.count() != 1) {
                    qCWarning(UNIFIEDMAILBOXAGENT_LOG) << "Identity special collection retrieval did not find a valid collection";
                    return;
                }
                Akonadi::SpecialMailCollections::self()->registerCollection(type, cols.first());
            });
}

void UnifiedMailboxAgent::fixSpecialCollections()
{
    // This is a tiny hack to assign proper SpecialCollectionAttribute to special collections
    // assigned through Identities. This should happen automatically in KMail when user changes
    // the special collections on the identity page, but until recent master (2018-07-24) this
    // wasn't the case and there's no automatic migration, so we need to fix up manually here.

    if (Settings::self()->fixedSpecialCollections()) {
        return;
    }

    qCDebug(UNIFIEDMAILBOXAGENT_LOG) << "Fixing special collections assigned from Identities";

    for (const auto &identity : *KIdentityManagementCore::IdentityManager::self()) {
        if (!identity.disabledFcc()) {
            fixSpecialCollection(identity.fcc(), Akonadi::SpecialMailCollections::SentMail);
        }
        fixSpecialCollection(identity.drafts(), Akonadi::SpecialMailCollections::Drafts);
        fixSpecialCollection(identity.templates(), Akonadi::SpecialMailCollections::Templates);
    }

    Settings::self()->setFixedSpecialCollections(true);
}

AKONADI_RESOURCE_MAIN(UnifiedMailboxAgent)

#include "moc_unifiedmailboxagent.cpp"
