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

#include <AkonadiCore/ChangeRecorder>
#include <AkonadiCore/CollectionDeleteJob>
#include <AkonadiCore/CollectionFetchJob>
#include <AkonadiCore/CollectionFetchScope>
#include <AkonadiCore/EntityDisplayAttribute>
#include <AkonadiCore/ItemFetchJob>
#include <AkonadiCore/ItemFetchScope>
#include <AkonadiCore/LinkJob>
#include <AkonadiCore/ServerManager>
#include <AkonadiCore/Session>
#include <AkonadiCore/SpecialCollectionAttribute>
#include <AkonadiCore/UnlinkJob>

#include <KIdentityManagement/Identity>
#include <KIdentityManagement/IdentityManager>

#include <KLocalizedString>
#include <QDBusConnection>

#include <QPointer>
#include <QTimer>

#include <chrono>
#include <memory>
#include <unordered_set>

UnifiedMailboxAgent::UnifiedMailboxAgent(const QString &id)
    : Akonadi::ResourceBase(id)
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
    QPointer<UnifiedMailboxAgent> agent(this);
    if (agent) {
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
    for (auto source : sources) {
        auto fetch = new Akonadi::ItemFetchJob(Akonadi::Collection(source), this);
        fetch->setDeliveryOption(Akonadi::ItemFetchJob::EmitItemsInBatches);
        fetch->fetchScope().setFetchVirtualReferences(true);
        fetch->fetchScope().setCacheOnly(true);
        connect(fetch, &Akonadi::ItemFetchJob::itemsReceived, this, [this, c](const Akonadi::Item::List &items) {
            Akonadi::Item::List toLink;
            std::copy_if(items.cbegin(), items.cend(), std::back_inserter(toLink), [&c](const Akonadi::Item &item) {
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
    connect(fetch, &Akonadi::ItemFetchJob::itemsReceived, this, [this, unifiedBox, c](const Akonadi::Item::List &items) {
        Akonadi::Item::List toUnlink;
        std::copy_if(items.cbegin(), items.cend(), std::back_inserter(toUnlink), [&unifiedBox](const Akonadi::Item &item) {
            return !unifiedBox->sourceCollections().contains(item.storageCollectionId());
        });
        if (!toUnlink.isEmpty()) {
            new Akonadi::UnlinkJob(c, toUnlink, this);
        }
    });
    connect(fetch, &Akonadi::ItemFetchJob::result, this, [this]() {
        itemsRetrievedIncremental({}, {}); // fake incremental retrieval
    });
}

bool UnifiedMailboxAgent::retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts)
{
    // This method should never be called by Akonadi
    Q_UNUSED(parts)
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
    // assigned trough Identities. This should happen automatically in KMail when user changes
    // the special collections on the identity page, but until recent master (2018-07-24) this
    // wasn't the case and there's no automatic migration, so we need to fix up manually here.

    if (Settings::self()->fixedSpecialCollections()) {
        return;
    }

    qCDebug(UNIFIEDMAILBOXAGENT_LOG) << "Fixing special collections assigned from Identities";

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
