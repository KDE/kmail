/*
   SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "unifiedmailboxmanager.h"
#include "common.h"
#include "settings.h"
#include "unifiedmailbox.h"
#include "unifiedmailboxagent_debug.h"

#include <KLocalizedString>

#include <Akonadi/KMime/SpecialMailCollections>
#include <AkonadiCore/CollectionFetchJob>
#include <AkonadiCore/CollectionFetchScope>
#include <AkonadiCore/ItemFetchScope>
#include <AkonadiCore/LinkJob>
#include <AkonadiCore/SpecialCollectionAttribute>
#include <AkonadiCore/UnlinkJob>

#include <QTimer>

#include <stdexcept> // for std::out_of_range

namespace
{
/**
 * A little RAII helper to make sure changeProcessed() and replayNext() gets
 * called on the ChangeRecorder whenever we are done with handling a change.
 */
class ReplayNextOnExit
{
public:
    ReplayNextOnExit(Akonadi::ChangeRecorder &recorder)
        : mRecorder(recorder)
    {
    }

    ~ReplayNextOnExit()
    {
        mRecorder.changeProcessed();
        mRecorder.replayNext();
    }

private:
    Akonadi::ChangeRecorder &mRecorder;
};
}

// static
bool UnifiedMailboxManager::isUnifiedMailbox(const Akonadi::Collection &col)
{
#ifdef UNIT_TESTS
    return col.parentCollection().name() == Common::AgentIdentifier;
#else
    return col.resource() == Common::AgentIdentifier;
#endif
}

UnifiedMailboxManager::UnifiedMailboxManager(const KSharedConfigPtr &config, QObject *parent)
    : QObject(parent)
    , mConfig(config)
{
    mMonitor.setObjectName(QStringLiteral("UnifiedMailboxChangeRecorder"));
    mMonitor.setConfig(&mMonitorSettings);
    mMonitor.setChangeRecordingEnabled(true);
    mMonitor.setTypeMonitored(Akonadi::Monitor::Items);
    mMonitor.setTypeMonitored(Akonadi::Monitor::Collections);
    mMonitor.itemFetchScope().setCacheOnly(true);
    mMonitor.itemFetchScope().setFetchRemoteIdentification(false);
    mMonitor.itemFetchScope().setFetchModificationTime(false);
    mMonitor.collectionFetchScope().fetchAttribute<Akonadi::SpecialCollectionAttribute>();
    connect(&mMonitor, &Akonadi::Monitor::itemAdded, this, [this](const Akonadi::Item &item, const Akonadi::Collection &collection) {
        ReplayNextOnExit replayNext(mMonitor);

        qCDebug(UNIFIEDMAILBOXAGENT_LOG) << "Item" << item.id() << "added to collection" << collection.id();
        const auto box = unifiedMailboxForSource(collection.id());
        if (!box) {
            qCWarning(UNIFIEDMAILBOXAGENT_LOG) << "Failed to find unified mailbox for source collection " << collection.id();
            return;
        }

        if (box->collectionId() <= -1) {
            qCWarning(UNIFIEDMAILBOXAGENT_LOG) << "Missing box->collection mapping for unified mailbox" << box->id();
            return;
        }

        new Akonadi::LinkJob(Akonadi::Collection{box->collectionId()}, {item}, this);
    });
    connect(&mMonitor, &Akonadi::Monitor::itemsRemoved, this, [this](const Akonadi::Item::List &items) {
        ReplayNextOnExit replayNext(mMonitor);

        // Monitor did the heavy lifting for us and already figured out that
        // we only monitor the source collection of the Items and translated
        // it into REMOVE change.

        // This relies on Akonadi never mixing Items from different sources or
        // destination during batch-moves.
        const auto parentId = items.first().parentCollection().id();
        const auto box = unifiedMailboxForSource(parentId);
        if (!box) {
            qCWarning(UNIFIEDMAILBOXAGENT_LOG) << "Received Remove notification for Items belonging to" << parentId << "which we don't monitor";
            return;
        }
        if (box->collectionId() <= -1) {
            qCWarning(UNIFIEDMAILBOXAGENT_LOG) << "Missing box->collection mapping for unified mailbox" << box->id();
            return;
        }

        new Akonadi::UnlinkJob(Akonadi::Collection{box->collectionId()}, items, this);
    });
    connect(&mMonitor,
            &Akonadi::Monitor::itemsMoved,
            this,
            [this](const Akonadi::Item::List &items, const Akonadi::Collection &srcCollection, const Akonadi::Collection &dstCollection) {
                ReplayNextOnExit replayNext(mMonitor);

                if (const auto srcBox = unifiedMailboxForSource(srcCollection.id())) {
                    // Move source collection was our source, unlink the Item from a box
                    new Akonadi::UnlinkJob(Akonadi::Collection{srcBox->collectionId()}, items, this);
                }
                if (const auto dstBox = unifiedMailboxForSource(dstCollection.id())) {
                    // Move destination collection is our source, link the Item into a box
                    new Akonadi::LinkJob(Akonadi::Collection{dstBox->collectionId()}, items, this);
                }
            });

    connect(&mMonitor, &Akonadi::Monitor::collectionRemoved, this, [this](const Akonadi::Collection &col) {
        ReplayNextOnExit replayNext(mMonitor);

        if (auto box = unifiedMailboxForSource(col.id())) {
            box->removeSourceCollection(col.id());
            mMonitor.setCollectionMonitored(col, false);
            if (box->sourceCollections().isEmpty()) {
                removeBox(box->id());
            }
            saveBoxes();
            // No need to resync the box collection, the linked Items got removed by Akonadi
        } else {
            qCWarning(UNIFIEDMAILBOXAGENT_LOG) << "Received notification about removal of Collection" << col.id() << "which we don't monitor";
        }
    });
    connect(&mMonitor,
            qOverload<const Akonadi::Collection &, const QSet<QByteArray> &>(&Akonadi::Monitor::collectionChanged),
            this,
            [this](const Akonadi::Collection &col, const QSet<QByteArray> &parts) {
                ReplayNextOnExit replayNext(mMonitor);

                qCDebug(UNIFIEDMAILBOXAGENT_LOG) << "Collection changed:" << parts;
                if (!parts.contains(Akonadi::SpecialCollectionAttribute().type())) {
                    return;
                }

                if (col.hasAttribute<Akonadi::SpecialCollectionAttribute>()) {
                    const auto srcBox = unregisterSpecialSourceCollection(col.id());
                    const auto dstBox = registerSpecialSourceCollection(col);
                    if (srcBox == dstBox) {
                        return;
                    }

                    saveBoxes();

                    if (srcBox && srcBox->sourceCollections().isEmpty()) {
                        removeBox(srcBox->id());
                        return;
                    }

                    if (srcBox) {
                        Q_EMIT updateBox(srcBox);
                    }
                    if (dstBox) {
                        Q_EMIT updateBox(dstBox);
                    }
                } else {
                    if (const auto box = unregisterSpecialSourceCollection(col.id())) {
                        saveBoxes();
                        if (box->sourceCollections().isEmpty()) {
                            removeBox(box->id());
                        } else {
                            Q_EMIT updateBox(box);
                        }
                    }
                }
            });
}

UnifiedMailboxManager::~UnifiedMailboxManager() = default;

Akonadi::ChangeRecorder &UnifiedMailboxManager::changeRecorder()
{
    return mMonitor;
}

void UnifiedMailboxManager::loadBoxes(FinishedCallback &&finishedCb)
{
    qCDebug(UNIFIEDMAILBOXAGENT_LOG) << "loading boxes";
    const auto group = mConfig->group("UnifiedMailboxes");
    const auto boxGroups = group.groupList();
    for (const auto &boxGroupName : boxGroups) {
        const auto boxGroup = group.group(boxGroupName);
        auto box = std::make_unique<UnifiedMailbox>();
        box->load(boxGroup);
        insertBox(std::move(box));
    }

    const auto cb = [this, finishedCb = std::move(finishedCb)]() {
        qCDebug(UNIFIEDMAILBOXAGENT_LOG) << "Finished callback: enabling change recorder";
        // Only now start processing changes from change recorder
        connect(&mMonitor, &Akonadi::ChangeRecorder::changesAdded, &mMonitor, &Akonadi::ChangeRecorder::replayNext, Qt::QueuedConnection);
        // And start replaying any potentially pending notification
        QTimer::singleShot(0, &mMonitor, &Akonadi::ChangeRecorder::replayNext);

        if (finishedCb) {
            finishedCb();
        }
    };

    qCDebug(UNIFIEDMAILBOXAGENT_LOG) << "Loaded" << mMailboxes.size() << "boxes from config";

    if (mMailboxes.empty()) {
        createDefaultBoxes(std::move(cb));
    } else {
        discoverBoxCollections(std::move(cb));
    }
}

void UnifiedMailboxManager::saveBoxes()
{
    auto group = mConfig->group("UnifiedMailboxes");
    const auto currentGroups = group.groupList();
    for (const auto &groupName : currentGroups) {
        group.deleteGroup(groupName);
    }
    for (const auto &boxIt : mMailboxes) {
        auto boxGroup = group.group(boxIt.second->id());
        boxIt.second->save(boxGroup);
    }
    mConfig->sync();
    mConfig->reparseConfiguration();
}

void UnifiedMailboxManager::insertBox(std::unique_ptr<UnifiedMailbox> box)
{
    auto it = mMailboxes.emplace(std::make_pair(box->id(), std::move(box)));
    it.first->second->attachManager(this);
}

void UnifiedMailboxManager::removeBox(const QString &id)
{
    auto box = std::find_if(mMailboxes.begin(), mMailboxes.end(), [&id](const std::pair<const QString, std::unique_ptr<UnifiedMailbox>> &box) {
        return box.second->id() == id;
    });
    if (box == mMailboxes.end()) {
        return;
    }

    box->second->attachManager(nullptr);
    mMailboxes.erase(box);
}

UnifiedMailbox *UnifiedMailboxManager::unifiedMailboxForSource(qint64 source) const
{
    const auto box = mSourceToBoxMap.find(source);
    if (box == mSourceToBoxMap.cend()) {
        return {};
    }
    return box->second;
}

UnifiedMailbox *UnifiedMailboxManager::unifiedMailboxFromCollection(const Akonadi::Collection &col) const
{
    if (!isUnifiedMailbox(col)) {
        return nullptr;
    }

    const auto box = mMailboxes.find(col.name());
    if (box == mMailboxes.cend()) {
        return {};
    }
    return box->second.get();
}

void UnifiedMailboxManager::createDefaultBoxes(FinishedCallback &&finishedCb)
{
    if (!Settings::self()->createDefaultBoxes()) {
        return;
    }
    // First build empty boxes
    auto inbox = std::make_unique<UnifiedMailbox>();
    inbox->attachManager(this);
    inbox->setId(Common::InboxBoxId);
    inbox->setName(i18n("Inbox"));
    inbox->setIcon(QStringLiteral("mail-folder-inbox"));
    insertBox(std::move(inbox));

    auto sent = std::make_unique<UnifiedMailbox>();
    sent->attachManager(this);
    sent->setId(Common::SentBoxId);
    sent->setName(i18n("Sent"));
    sent->setIcon(QStringLiteral("mail-folder-sent"));
    insertBox(std::move(sent));

    auto drafts = std::make_unique<UnifiedMailbox>();
    drafts->attachManager(this);
    drafts->setId(Common::DraftsBoxId);
    drafts->setName(i18n("Drafts"));
    drafts->setIcon(QStringLiteral("document-properties"));
    insertBox(std::move(drafts));

    auto list = new Akonadi::CollectionFetchJob(Akonadi::Collection::root(), Akonadi::CollectionFetchJob::Recursive, this);
    list->fetchScope().fetchAttribute<Akonadi::SpecialCollectionAttribute>();
    list->fetchScope().setContentMimeTypes({QStringLiteral("message/rfc822")});
#ifdef UNIT_TESTS
    list->fetchScope().setAncestorRetrieval(Akonadi::CollectionFetchScope::Parent);
#else
    list->fetchScope().setAncestorRetrieval(Akonadi::CollectionFetchScope::None);
#endif
    connect(list, &Akonadi::CollectionFetchJob::collectionsReceived, this, [this](const Akonadi::Collection::List &list) {
        for (const auto &col : list) {
            if (isUnifiedMailbox(col)) {
                continue;
            }

            try {
                switch (Akonadi::SpecialMailCollections::self()->specialCollectionType(col)) {
                case Akonadi::SpecialMailCollections::Inbox:
                    mMailboxes.at(Common::InboxBoxId)->addSourceCollection(col.id());
                    break;
                case Akonadi::SpecialMailCollections::SentMail:
                    mMailboxes.at(Common::SentBoxId)->addSourceCollection(col.id());
                    break;
                case Akonadi::SpecialMailCollections::Drafts:
                    mMailboxes.at(Common::DraftsBoxId)->addSourceCollection(col.id());
                    break;
                default:
                    continue;
                }
            } catch (const std::out_of_range &) {
                qCWarning(UNIFIEDMAILBOXAGENT_LOG) << "Failed to find a special unified mailbox for source collection" << col.id();
                continue;
            }
        }
    });
    connect(list, &Akonadi::CollectionFetchJob::result, this, [this, finishedCb = std::move(finishedCb)]() {
        saveBoxes();
        if (finishedCb) {
            finishedCb();
        }
    });
#ifndef UNIT_TESTS
    Settings::self()->setCreateDefaultBoxes(false);
    Settings::self()->save();
#endif
}

void UnifiedMailboxManager::discoverBoxCollections(FinishedCallback &&finishedCb)
{
    auto list = new Akonadi::CollectionFetchJob(Akonadi::Collection::root(), Akonadi::CollectionFetchJob::Recursive, this);
#ifdef UNIT_TESTS
    list->fetchScope().setAncestorRetrieval(Akonadi::CollectionFetchScope::Parent);
#else
    list->fetchScope().setResource(Common::AgentIdentifier);
#endif
    connect(list, &Akonadi::CollectionFetchJob::collectionsReceived, this, [this](const Akonadi::Collection::List &list) {
        for (const auto &col : list) {
            if (!isUnifiedMailbox(col) || col.parentCollection() == Akonadi::Collection::root()) {
                continue;
            }
            const auto it = mMailboxes.find(col.name());
            if (it == mMailboxes.end()) {
                qCWarning(UNIFIEDMAILBOXAGENT_LOG) << "Failed to find an unified mailbox for source collection" << col.id();
            } else {
                it->second->setCollectionId(col.id());
            }
        }
    });
    if (finishedCb) {
        connect(list, &Akonadi::CollectionFetchJob::result, this, finishedCb);
    }
}

const UnifiedMailbox *UnifiedMailboxManager::registerSpecialSourceCollection(const Akonadi::Collection &col)
{
    // This is slightly awkward, wold be better if we could use SpecialMailCollections,
    // but it also relies on Monitor internally, so there's a possible race condition
    // between our ChangeRecorder and SpecialMailCollections' Monitor
    auto attr = col.attribute<Akonadi::SpecialCollectionAttribute>();
    Q_ASSERT(attr);
    if (!attr) {
        return {};
    }

    decltype(mMailboxes)::iterator box;
    if (attr->collectionType() == Common::SpecialCollectionInbox) {
        box = mMailboxes.find(Common::InboxBoxId);
    } else if (attr->collectionType() == Common::SpecialCollectionSentMail) {
        box = mMailboxes.find(Common::SentBoxId);
    } else if (attr->collectionType() == Common::SpecialCollectionDrafts) {
        box = mMailboxes.find(Common::DraftsBoxId);
    }
    if (box == mMailboxes.end()) {
        return {};
    }

    box->second->addSourceCollection(col.id());
    return box->second.get();
}

const UnifiedMailbox *UnifiedMailboxManager::unregisterSpecialSourceCollection(qint64 colId)
{
    auto box = unifiedMailboxForSource(colId);
    if (!box) {
        return {};
    }

    if (!box->isSpecial()) {
        qCDebug(UNIFIEDMAILBOXAGENT_LOG) << colId << "does not belong to a special unified box" << box->id();
        return {};
    }

    box->removeSourceCollection(colId);
    return box;
}
