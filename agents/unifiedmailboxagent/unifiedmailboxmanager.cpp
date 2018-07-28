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

#include "unifiedmailboxmanager.h"
#include "unifiedmailboxagent_debug.h"

#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocalizedString>

#include <AkonadiCore/SpecialCollectionAttribute>
#include <AkonadiCore/CollectionFetchJob>
#include <AkonadiCore/CollectionFetchScope>
#include <AkonadiCore/LinkJob>
#include <AkonadiCore/UnlinkJob>
#include <AkonadiCore/ItemFetchScope>
#include <Akonadi/KMime/SpecialMailCollections>

#include <QList>

#include "utils.h"

namespace {

/**
 * A little RAII helper to make sure changeProcessed() and replayNext() gets
 * called on the ChangeRecorder whenever we are done with handling a change.
 */
class ReplayNextOnExit
{
public:
    ReplayNextOnExit(Akonadi::ChangeRecorder &recorder)
        : mRecorder(recorder)
    {}
    ~ReplayNextOnExit()
    {
        mRecorder.changeProcessed();
        mRecorder.replayNext();
    }
private:
    Akonadi::ChangeRecorder &mRecorder;
};

}


void UnifiedMailbox::setId(const QString &id)
{
    mId = id;
}

QString UnifiedMailbox::id() const
{
    return mId;
}

void UnifiedMailbox::setName(const QString &name)
{
    mName = name;
}

QString UnifiedMailbox::name() const
{
    return mName;
}

void UnifiedMailbox::setIcon(const QString &icon)
{
    mIcon = icon;
}

QString UnifiedMailbox::icon() const
{
    return mIcon;
}

void UnifiedMailbox::addSourceCollection(qint64 source)
{
    mSources.insert(source);
}

void UnifiedMailbox::removeSourceCollection(qint64 source)
{
    mSources.remove(source);
}

void UnifiedMailbox::setSourceCollections(const QSet<qint64> &sources)
{
    mSources = sources;
}

QSet<qint64> UnifiedMailbox::sourceCollections() const
{
    return mSources;
}

// static
bool UnifiedMailboxManager::isUnifiedMailbox(const Akonadi::Collection &col)
{
    return col.resource() == QLatin1String("akonadi_unifiedmailbox_agent");
}

UnifiedMailboxManager::UnifiedMailboxManager(KSharedConfigPtr config, QObject* parent)
    : QObject(parent)
    , mConfig(std::move(config))
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
    connect(&mMonitor, &Akonadi::Monitor::itemAdded,
            this, [this](const Akonadi::Item &item, const Akonadi::Collection &collection) {
                ReplayNextOnExit replayNext(mMonitor);

                qCDebug(agent_log) << "Item" << item.id() << "added to collection" << collection.id();
                const auto box = unifiedMailboxForSource(collection.id());
                if (!box) {
                    qCWarning(agent_log) << "Failed to find unified mailbox for source collection " << collection.id();
                    return;
                }

                const auto boxId = collectionIdFromUnifiedMailbox(box->id());
                qCDebug(agent_log) << "Unified box:" << box->name() << ", collection" << boxId;
                if (boxId <= -1) {
                    qCWarning(agent_log) << "Missing box->collection mapping for unified mailbox" << box->id();
                    return;
                }

                new Akonadi::LinkJob(Akonadi::Collection{boxId}, {item}, this);
            });
    connect(&mMonitor, &Akonadi::Monitor::itemsRemoved,
            this, [this](const Akonadi::Item::List &items) {
                ReplayNextOnExit replayNext(mMonitor);

                // Monitor did the heavy lifting for us and already figured out that
                // we only monitor the source collection of the Items and translated
                // it into REMOVE change.

                // This relies on Akonadi never mixing Items from different sources or
                // destination during batch-moves.
                const auto parentId = items.first().parentCollection().id();
                const auto box = unifiedMailboxForSource(parentId);
                if (!box) {
                    qCWarning(agent_log) << "Received Remove notification for Items belonging to" << parentId << "which we don't monitor";
                    return;
                }
                const auto boxId = collectionIdFromUnifiedMailbox(box->id());
                if (boxId <= -1) {
                    qCWarning(agent_log) << "Missing box->collection mapping for unified mailbox" << box->id();
                }

                new Akonadi::UnlinkJob(Akonadi::Collection{boxId}, items, this);
            });
    connect(&mMonitor, &Akonadi::Monitor::itemsMoved,
            this, [this](const Akonadi::Item::List &items, const Akonadi::Collection &srcCollection,
                         const Akonadi::Collection &dstCollection) {
                ReplayNextOnExit replayNext(mMonitor);

                const auto srcBox = unifiedMailboxForSource(srcCollection.id());
                const auto dstBox = unifiedMailboxForSource(dstCollection.id());
                qCDebug(agent_log ) << "Items moved away from " << srcCollection.id() << "(box" << srcBox << ") to " << dstCollection.id() << "(Box" << dstBox << ")";
                if (srcBox) {
                    // Move source collection was our source, unlink the Item from a box
                    const auto srcBoxId = collectionIdFromUnifiedMailbox(srcBox->id());
                    new Akonadi::UnlinkJob(Akonadi::Collection{srcBoxId}, items, this);
                }
                if (dstBox) {
                    // Move destination collection is our source, link the Item into a box
                    const auto dstBoxId = collectionIdFromUnifiedMailbox(dstBox->id());
                    new Akonadi::LinkJob(Akonadi::Collection{dstBoxId}, items, this);
                }

                //TODO Settings::self()->setLastSeenEvent(std::chrono::steady_clock::now().time_since_epoch().count());
            });
    connect(&mMonitor, &Akonadi::Monitor::collectionAdded,
            this, [this](const Akonadi::Collection &col) {
                ReplayNextOnExit replayNext(mMonitor);

                if (isUnifiedMailbox(col)) {
                    mBoxId.insert(col.name(), col.id());
                } else {
                    // TODO: Potentially a new special collection: we should auto-add it to our box
                }
            });
    connect(&mMonitor, &Akonadi::Monitor::collectionRemoved,
            this, [this](const Akonadi::Collection &col) {
                ReplayNextOnExit replayNext(mMonitor);

                // TODO: If it was a source collection for one of our boxes, remove it from box's sources
            });
    connect(&mMonitor, QOverload<const Akonadi::Collection &, const QSet<QByteArray> &>::of(&Akonadi::Monitor::collectionChanged),
            this, [this](const Akonadi::Collection &col, const QSet<QByteArray> &parts) {
                ReplayNextOnExit replayNext(mMonitor);

                if (col.hasAttribute<Akonadi::SpecialCollectionAttribute>()) {
                    // TODO: Remove collection from whichever special mailbox it may
                    // have belonged to before and add it to correct special mailbox
                } else {
                    // TODO: Check whether it's a source of one of our special
                    // mailboxes and remove it from there
                }
            });

}

UnifiedMailboxManager::~UnifiedMailboxManager()
{
}

void UnifiedMailboxManager::loadBoxes(LoadCallback &&cb)
{
    const auto group = mConfig->group("UnifiedMailboxes");
    const auto boxGroups = group.groupList();
    for (const auto &boxGroupName : boxGroups) {
        const auto boxGroup = group.group(boxGroupName);
        UnifiedMailbox box;
        box.setId(boxGroupName);
        box.setName(boxGroup.readEntry("name"));
        box.setIcon(boxGroup.readEntry("icon", QStringLiteral("folder-mail")));
        QList<qint64> sources = boxGroup.readEntry("sources", QList<qint64>{});
        for (auto source : sources) {
            box.addSourceCollection(source);
            mMonitor.setCollectionMonitored(Akonadi::Collection(source));
        }
        insertBox(std::move(box));
    }

    if (mBoxes.isEmpty()) {
        createDefaultBoxes(std::move(cb));
    } else {
        discoverBoxCollections([this, cb = std::move(cb)]() {
            // Only now start processing changes from change recorder
            connect(&mMonitor, &Akonadi::ChangeRecorder::changesAdded, &mMonitor, &Akonadi::ChangeRecorder::replayNext, Qt::QueuedConnection);
            // And start replaying any potentially pending notification
            mMonitor.replayNext();

            if (cb) {
                cb();
            }
        });
    }
}

void UnifiedMailboxManager::saveBoxes()
{
    auto group = mConfig->group("UnifiedMailboxes");
    const auto currentGroups = group.groupList();
    for (const auto &groupName : currentGroups) {
        group.deleteGroup(groupName);
    }
    for (const auto &box : mBoxes) {
        auto boxGroup = group.group(box.id());
        boxGroup.writeEntry("name", box.name());
        boxGroup.writeEntry("icon", box.icon());
        boxGroup.writeEntry("sources", setToList(box.sourceCollections()));
    }
}

void UnifiedMailboxManager::insertBox(UnifiedMailbox box)
{
    mBoxes.insert(box.id(), box);
}

void UnifiedMailboxManager::removeBox(const QString &name)
{
    mBoxes.remove(name);
}

const UnifiedMailbox *UnifiedMailboxManager::unifiedMailboxForSource(qint64 source) const
{
    for (const auto &box : mBoxes) {
        if (box.mSources.contains(source)) {
            return &box;
        }
    }

    return nullptr;
}

const UnifiedMailbox * UnifiedMailboxManager::unifiedMailboxFromCollection(const Akonadi::Collection &col) const
{
    if (!isUnifiedMailbox(col)) {
        return nullptr;
    }

    const auto box = mBoxes.find(col.name());
    if (box == mBoxes.cend()) {
        return nullptr;
    }
    return &(*box);
}


qint64 UnifiedMailboxManager::collectionIdFromUnifiedMailbox(const QString &id) const
{
    return mBoxId.value(id, -1);
}

void UnifiedMailboxManager::createDefaultBoxes(LoadCallback &&cb)
{
    // First build empty boxes
    UnifiedMailbox inbox;
    inbox.setId(QStringLiteral("inbox"));
    inbox.setName(i18n("Inbox"));
    inbox.setIcon(QStringLiteral("mail-folder-inbox"));
    insertBox(std::move(inbox));

    UnifiedMailbox sent;
    sent.setId(QStringLiteral("sent-mail"));
    sent.setName(i18n("Sent"));
    sent.setIcon(QStringLiteral("mail-folder-sent"));
    insertBox(std::move(sent));

    UnifiedMailbox drafts;
    drafts.setId(QStringLiteral("drafts"));
    drafts.setName(i18n("Drafts"));
    drafts.setIcon(QStringLiteral("document-properties"));
    insertBox(std::move(drafts));

    auto list = new Akonadi::CollectionFetchJob(Akonadi::Collection::root(), Akonadi::CollectionFetchJob::Recursive, this);
    list->fetchScope().fetchAttribute<Akonadi::SpecialCollectionAttribute>();
    list->fetchScope().setContentMimeTypes({QStringLiteral("message/rfc822")});
    connect(list, &Akonadi::CollectionFetchJob::collectionsReceived,
            this, [this](const Akonadi::Collection::List &list) {
                for (const auto &col : list) {
                    if (isUnifiedMailbox(col)) {
                        continue;
                    }

                    switch (Akonadi::SpecialMailCollections::self()->specialCollectionType(col)) {
                    case Akonadi::SpecialMailCollections::Inbox:
                        mBoxes.find(QStringLiteral("inbox"))->addSourceCollection(col.id());
                        break;
                    case Akonadi::SpecialMailCollections::SentMail:
                        mBoxes.find(QStringLiteral("sent-mail"))->addSourceCollection(col.id());
                        break;
                    case Akonadi::SpecialMailCollections::Drafts:
                        mBoxes.find(QStringLiteral("drafts"))->addSourceCollection(col.id());
                        break;
                    default:
                        continue;
                    }
                }
            });
    connect(list, &Akonadi::CollectionFetchJob::finished,
            this, [this, cb = std::move(cb)]() {
                saveBoxes();
                if (cb) {
                    cb();
                }
            });
}

void UnifiedMailboxManager::discoverBoxCollections(LoadCallback &&cb)
{
    auto list = new Akonadi::CollectionFetchJob(Akonadi::Collection::root(), Akonadi::CollectionFetchJob::Recursive, this);
    list->fetchScope().setResource(QStringLiteral("akonadi_unifiedmailbox_agent"));
    connect(list, &Akonadi::CollectionFetchJob::collectionsReceived,
            this, [this](const Akonadi::Collection::List &list) {
                for (const auto &col : list) {
                    mBoxId.insert(col.name(), col.id());
                }
            });
    connect(list, &Akonadi::CollectionFetchJob::finished,
            this, [cb = std::move(cb)]() {
                if (cb) {
                    cb();
                }
            });
}
