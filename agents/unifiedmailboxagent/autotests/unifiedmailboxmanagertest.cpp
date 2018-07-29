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

#include "../unifiedmailboxmanager.h"
#include "../unifiedmailbox.h"
#include "../common.h"

#include <KSharedConfig>
#include <KConfigGroup>

#include <AkonadiCore/SpecialCollectionAttribute>
#include <AkonadiCore/CollectionFetchJob>
#include <AkonadiCore/CollectionFetchScope>
#include <AkonadiCore/CollectionCreateJob>
#include <AkonadiCore/CollectionDeleteJob>
#include <AkonadiCore/CollectionModifyJob>
#include <AkonadiCore/ItemCreateJob>
#include <AkonadiCore/ItemDeleteJob>
#include <AkonadiCore/ItemModifyJob>
#include <AkonadiCore/ItemMoveJob>
#include <AkonadiCore/qtest_akonadi.h>

#include <QTest>

#include <memory>
#include <chrono>

using namespace std::chrono;
using namespace std::chrono_literals;

namespace {

#define AKVERIFY_RET(statement, ret) \
do {\
    if (!QTest::qVerify(static_cast<bool>(statement), #statement, "", __FILE__, __LINE__))\
        return ret;\
} while (false)

#define AKCOMPARE_RET(actual, expected, ret) \
do {\
    if (!QTest::qCompare(actual, expected, #actual, #expected, __FILE__, __LINE__))\
        return ret;\
} while (false)



Akonadi::Collection collectionForId(qint64 id)
{
    auto fetch = new Akonadi::CollectionFetchJob(Akonadi::Collection(id), Akonadi::CollectionFetchJob::Base);
    fetch->fetchScope().fetchAttribute<Akonadi::SpecialCollectionAttribute>();
    AKVERIFY_RET(fetch->exec(), {});
    const auto cols = fetch->collections();
    AKCOMPARE_RET(cols.count(), 1, {});
    return cols.first();
}

Akonadi::Collection collectionForRid(const QString &rid)
{
    auto fetch = new Akonadi::CollectionFetchJob(Akonadi::Collection::root(), Akonadi::CollectionFetchJob::Recursive);
    fetch->fetchScope().fetchAttribute<Akonadi::SpecialCollectionAttribute>();
    fetch->fetchScope().setAncestorRetrieval(Akonadi::CollectionFetchScope::All);
    AKVERIFY_RET(fetch->exec(), {});
    const auto cols = fetch->collections();
    auto colIt = std::find_if(cols.cbegin(), cols.cend(), [&rid](const Akonadi::Collection &col) {
        return col.remoteId() == rid;
    });
    AKVERIFY_RET(colIt != cols.cend(), {});
    return *colIt;
}

// A kingdom and a horse for std::optional!
std::unique_ptr<UnifiedMailbox> createUnifiedMailbox(const QString &id, const QString &name, const QStringList &sourceRids)
{
    auto mailbox = std::make_unique<UnifiedMailbox>();
    mailbox->setId(id);
    mailbox->setName(name);
    mailbox->setIcon(QStringLiteral("dummy-icon"));
    for (const auto &srcRid : sourceRids) {
        const auto srcCol = collectionForRid(srcRid);
        AKVERIFY_RET(srcCol.isValid(), {});
        mailbox->addSourceCollection(srcCol.id());
    }
    return mailbox;
}

class EntityDeleter
{
public:
    ~EntityDeleter()
    {
        while (!cols.isEmpty()) {
            if (!(new Akonadi::CollectionDeleteJob(cols.takeFirst()))->exec()) {
                QFAIL("Failed to cleanup collection!");
            }
        }
        while (!items.isEmpty()) {
            if (!(new Akonadi::ItemDeleteJob(items.takeFirst()))->exec()) {
                QFAIL("Failed to cleanup Item");
            }
        }
    }

    EntityDeleter &operator<<(const Akonadi::Collection &col)
    {
        cols.push_back(col);
        return *this;
    }

    EntityDeleter &operator<<(const Akonadi::Item &item)
    {
        items.push_back(item);
        return *this;
    }
private:
    Akonadi::Collection::List cols;
    Akonadi::Item::List items;
};

Akonadi::Collection createCollection(const QString &name, const Akonadi::Collection &parent, EntityDeleter &deleter)
{
    Akonadi::Collection col;
    col.setName(name);
    col.setParentCollection(parent);
    col.setVirtual(true);
    auto createCol = new Akonadi::CollectionCreateJob(col);
    AKVERIFY_RET(createCol->exec(), {});
    col = createCol->collection();
    if (col.isValid()) {
        deleter << col;
    }
    return col;
}


} // namespace


class UnifiedMailboxManagerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void initTestCase()
    {
        AkonadiTest::checkTestIsIsolated();
    }

    void testCreateDefaultBoxes()
    {
        // Setup
        auto kcfg = KSharedConfig::openConfig(QString::fromUtf8(QTest::currentTestFunction()));
        const auto boxesGroup = kcfg->group("UnifiedMailboxes");
        UnifiedMailboxManager manager(kcfg);

        // Make sure the config is empty
        QVERIFY(boxesGroup.groupList().empty());

        // Call loadBoxes and wait for it to finish
        bool loadingDone = false;
        manager.loadBoxes([&loadingDone]() { loadingDone = true; });
        QTRY_VERIFY_WITH_TIMEOUT(loadingDone, milliseconds(10s).count());

        // Check all the three boxes were created
        bool success;
        const auto verifyBox = [&manager, &success](const QString &id, int numSources) {
            success = false;
            auto boxIt = std::find_if(manager.begin(), manager.end(),
                            [&id](const UnifiedMailboxManager::Entry &e) {
                                return e.second->id() == id;
                            });
            QVERIFY(boxIt != manager.end());
            const auto &box = boxIt->second;
            const auto sourceCollections = box->sourceCollections();
            QCOMPARE(sourceCollections.size(), numSources);
            for (auto source : sourceCollections) {
                auto col = collectionForId(source);
                QVERIFY(col.isValid());
                QVERIFY(col.hasAttribute<Akonadi::SpecialCollectionAttribute>());
                QCOMPARE(col.attribute<Akonadi::SpecialCollectionAttribute>()->collectionType(), id.toLatin1());
            }
            success = true;
        };
        verifyBox(Common::InboxBoxId, 2);
        QVERIFY(success);
        verifyBox(Common::SentBoxId, 2);
        QVERIFY(success);
        verifyBox(Common::DraftsBoxId, 1);
        QVERIFY(success);

        // Check boxes were written to config - we don't check the contents of
        // the group, testing UnifiedMailbox serialization is done in other tests
        QVERIFY(boxesGroup.groupList().size() == 3);
        QVERIFY(boxesGroup.hasGroup(Common::InboxBoxId));
        QVERIFY(boxesGroup.hasGroup(Common::SentBoxId));
        QVERIFY(boxesGroup.hasGroup(Common::DraftsBoxId));
    }

    void testAddingNewMailbox()
    {
        // Setup
        auto kcfg = KSharedConfig::openConfig(QString::fromUtf8(QTest::currentTestFunction()));
        const auto boxesGroup = kcfg->group("UnifiedMailboxes");
        UnifiedMailboxManager manager(kcfg);
        Akonadi::ChangeRecorder &recorder = manager.changeRecorder();

        // Nothing should be monitored as of now
        QVERIFY(recorder.collectionsMonitored().isEmpty());

        // Create a new unified mailbox and passit to the manager
        auto mailbox = createUnifiedMailbox(QStringLiteral("Test1"), QStringLiteral("Test 1"),
                                            { QStringLiteral("res1_inbox") });
        QVERIFY(mailbox);
        const auto sourceCol = mailbox->sourceCollections().toList().first();
        manager.insertBox(std::move(mailbox));

        // Now manager should have one unified mailbox and monitor all of its
        // source collections
        QCOMPARE(std::distance(manager.begin(), manager.end()), 1);
        QCOMPARE(recorder.collectionsMonitored().size(), 1);
        QCOMPARE(recorder.collectionsMonitored().at(0).id(), sourceCol);
        QVERIFY(manager.unifiedMailboxForSource(sourceCol) != nullptr);

        // But nothing should bne written in the config yet
        QVERIFY(!boxesGroup.groupList().contains(QLatin1String("Test1")));

        // Now write to the config file and check it's actually there - we don't test
        // the contents of the group, UnifiedMailbox serialization has its own test
        manager.saveBoxes();
        QVERIFY(boxesGroup.hasGroup(QLatin1String("Test1")));
    }

    void testRemoveMailbox()
    {
        // Setup
        auto kcfg = KSharedConfig::openConfig(QString::fromUtf8(QTest::currentTestFunction()));
        auto boxesGroup = kcfg->group("UnifiedMailboxes");
        auto mailbox = createUnifiedMailbox(QStringLiteral("Test1"), QStringLiteral("Test 1"),
                                            { QStringLiteral("res1_foo"), QStringLiteral("res2_foo") });
        QVERIFY(mailbox);
        auto group = boxesGroup.group(mailbox->id());
        mailbox->save(group);

        UnifiedMailboxManager manager(kcfg);
        Akonadi::ChangeRecorder &recorder = manager.changeRecorder();

        // Nothing should be monitored right now
        QVERIFY(recorder.collectionsMonitored().isEmpty());

        // Load the config
        bool loadingDone = false;
        manager.loadBoxes([&loadingDone]() { loadingDone = true; });
        QTRY_VERIFY_WITH_TIMEOUT(loadingDone, milliseconds(10s).count());

        // Now the box should be loaded and its source collections monitored
        QCOMPARE(std::distance(manager.begin(), manager.end()), 1);
        QCOMPARE(recorder.collectionsMonitored().count(), 2);
        const auto srcCols = mailbox->sourceCollections().toList();
        QCOMPARE(srcCols.count(), 2);
        QVERIFY(recorder.collectionsMonitored().contains(Akonadi::Collection(srcCols[0])));
        QVERIFY(recorder.collectionsMonitored().contains(Akonadi::Collection(srcCols[1])));

        // Now remove the box
        manager.removeBox(mailbox->id());

        // Manager should have no boxes and no source collections should be monitored
        QVERIFY(manager.begin() == manager.end());
        QVERIFY(recorder.collectionsMonitored().isEmpty());

        // But the box still exists in the config
        QVERIFY(boxesGroup.hasGroup(mailbox->id()));

        // Save the new state
        manager.saveBoxes();

        // And now it should be gone from the config file as well
        QVERIFY(!boxesGroup.hasGroup(mailbox->id()));
    }

    void testDiscoverBoxCollections()
    {
        // Setup
        auto kcfg = KSharedConfig::openConfig(QString::fromUtf8(QTest::currentTestFunction()));
        auto boxesGroup = kcfg->group("UnifiedMailboxes");
        UnifiedMailboxManager manager(kcfg);
        EntityDeleter deleter;
        const auto inbox = createUnifiedMailbox(Common::InboxBoxId, QStringLiteral("Inbox"),
                                            { QStringLiteral("res1_inbox"), QStringLiteral("res2_inbox") });
        auto boxGroup = boxesGroup.group(inbox->id());
        inbox->save(boxGroup);
        const auto sentBox = createUnifiedMailbox(Common::SentBoxId, QStringLiteral("Sent"),
                                            { QStringLiteral("res1_sent"), QStringLiteral("res2_sent") });
        boxGroup = boxesGroup.group(sentBox->id());
        sentBox->save(boxGroup);

        const Akonadi::Collection parentCol = collectionForRid(Common::AgentIdentifier);
        QVERIFY(parentCol.isValid());

        const auto inboxBoxCol = createCollection(Common::InboxBoxId, parentCol, deleter);
        QVERIFY(inboxBoxCol.isValid());

        const auto sentBoxCol = createCollection(Common::SentBoxId, parentCol, deleter);
        QVERIFY(sentBoxCol.isValid());

        // Load from config
        bool loadingDone = false;
        manager.loadBoxes([&loadingDone]() { loadingDone = true; });
        QTRY_VERIFY_WITH_TIMEOUT(loadingDone, milliseconds(10s).count());

        // Now the boxes should be loaded and we should be able to access them
        // by IDs of collections that represent them. The collections should also
        // be set for each box.
        auto box = manager.unifiedMailboxFromCollection(inboxBoxCol);
        QVERIFY(box != nullptr);
        QCOMPARE(box->collectionId(), inboxBoxCol.id());

        box = manager.unifiedMailboxFromCollection(sentBoxCol);
        QVERIFY(box != nullptr);
        QCOMPARE(box->collectionId(), sentBoxCol.id());
    }

    void testItemAddedToSourceCollection()
    {
        // Setup
        auto kcfg = KSharedConfig::openConfig(QString::fromUtf8(QTest::currentTestFunction()));
        UnifiedMailboxManager manager(kcfg);
        EntityDeleter deleter;

        const auto parentCol = collectionForRid(Common::AgentIdentifier);
        QVERIFY(parentCol.isValid());

        const auto inboxBoxCol = createCollection(Common::InboxBoxId, parentCol, deleter);
        QVERIFY(inboxBoxCol.isValid());

        // Load boxes - config is empty so this will create the default Boxes and
        // assign the Inboxes from Knuts to it
        bool loadingDone = true;
        manager.loadBoxes([&loadingDone]() { loadingDone = true; });
        QTRY_VERIFY_WITH_TIMEOUT(loadingDone, milliseconds(10s).count());

        // Now discover collections for the created boxes
        loadingDone = false;
        manager.discoverBoxCollections([&loadingDone]() { loadingDone = true; });
        QTRY_VERIFY_WITH_TIMEOUT(loadingDone, milliseconds(10s).count());

        // Get one of the source collections for Inbox
        const auto inboxSourceCol = collectionForRid(QStringLiteral("res1_inbox"));
        QVERIFY(inboxSourceCol.isValid());

        // Setup up a monitor to to be notified when an item gets linked into
        // the unified mailbox collection
        Akonadi::Monitor monitor;
        monitor.setCollectionMonitored(inboxBoxCol);
        QSignalSpy itemLinkedSignalSpy(&monitor, &Akonadi::Monitor::itemsLinked);
        QVERIFY(QSignalSpy(&monitor, &Akonadi::Monitor::monitorReady).wait());

        // Add a new Item into the source collection
        Akonadi::Item item;
        item.setMimeType(QStringLiteral("application/octet-stream"));
        item.setParentCollection(inboxSourceCol);
        item.setPayload(QByteArray{"Hello world!"});
        auto createItem = new Akonadi::ItemCreateJob(item, inboxSourceCol, this);
        AKVERIFYEXEC(createItem);
        item = createItem->item();
        deleter << item;

        // Then wait for ItemLinked notification as the Manager has linked the new Item
        // to the dest collection
        QTRY_COMPARE(itemLinkedSignalSpy.size(), 1);
        const auto linkedItems = itemLinkedSignalSpy.at(0).at(0).value<Akonadi::Item::List>();
        QCOMPARE(linkedItems.size(), 1);
        QCOMPARE(linkedItems.at(0), item);
        const auto linkedCol = itemLinkedSignalSpy.at(0).at(1).value<Akonadi::Collection>();
        QCOMPARE(linkedCol, inboxBoxCol);
    }

    void testItemMovedFromSourceCollection()
    {
        // Setup
        auto kcfg = KSharedConfig::openConfig(QString::fromUtf8(QTest::currentTestFunction()));
        UnifiedMailboxManager manager(kcfg);
        EntityDeleter deleter;

        const auto parentCol = collectionForRid(Common::AgentIdentifier);
        QVERIFY(parentCol.isValid());

        const auto inboxBoxCol = createCollection(Common::InboxBoxId, parentCol, deleter);
        QVERIFY(inboxBoxCol.isValid());

        // Load boxes - config is empty so this will create the default Boxes and
        // assign the Inboxes from Knuts to it
        bool loadingDone = true;
        manager.loadBoxes([&loadingDone]() { loadingDone = true; });
        QTRY_VERIFY_WITH_TIMEOUT(loadingDone, milliseconds(10s).count());

        // Now discover collections for the created boxes
        loadingDone = false;
        manager.discoverBoxCollections([&loadingDone]() { loadingDone = true; });
        QTRY_VERIFY_WITH_TIMEOUT(loadingDone, milliseconds(10s).count());

        // Get one of the source collections for Inbox
        const auto inboxSourceCol = collectionForRid(QStringLiteral("res1_inbox"));
        QVERIFY(inboxSourceCol.isValid());

        // Setup up a monitor to to be notified when an item gets linked into
        // the unified mailbox collection
        Akonadi::Monitor monitor;
        monitor.setCollectionMonitored(inboxBoxCol);
        QSignalSpy itemLinkedSignalSpy(&monitor, &Akonadi::Monitor::itemsLinked);
        QSignalSpy itemUnlinkedSignalSpy(&monitor, &Akonadi::Monitor::itemsUnlinked);
        QVERIFY(QSignalSpy(&monitor, &Akonadi::Monitor::monitorReady).wait());

        // Add a new Item into the source collection
        Akonadi::Item item;
        item.setMimeType(QStringLiteral("application/octet-stream"));
        item.setParentCollection(inboxSourceCol);
        item.setPayload(QByteArray{"Hello world!"});
        auto createItem = new Akonadi::ItemCreateJob(item, inboxSourceCol, this);
        AKVERIFYEXEC(createItem);
        item = createItem->item();
        deleter << item;

        // Waity for the item to be linked
        QTRY_COMPARE(itemLinkedSignalSpy.size(), 1);

        const auto destinationCol = collectionForRid(QStringLiteral("res1_foo"));
        QVERIFY(destinationCol.isValid());

        // Now move the Item to an unmonitored collection
        auto move = new Akonadi::ItemMoveJob(item, destinationCol, this);
        AKVERIFYEXEC(move);

        QTRY_COMPARE(itemUnlinkedSignalSpy.size(), 1);
        const auto unlinkedItems = itemUnlinkedSignalSpy.at(0).at(0).value<Akonadi::Item::List>();
        QCOMPARE(unlinkedItems.size(), 1);
        QCOMPARE(unlinkedItems.first(), item);
        const auto unlinkedCol = itemUnlinkedSignalSpy.at(0).at(1).value<Akonadi::Collection>();
        QCOMPARE(unlinkedCol, inboxBoxCol);
    }

    void testItemMovedBetweenSourceCollections()
    {
        // Setup
        auto kcfg = KSharedConfig::openConfig(QString::fromUtf8(QTest::currentTestFunction()));
        UnifiedMailboxManager manager(kcfg);
        EntityDeleter deleter;

        const auto parentCol = collectionForRid(Common::AgentIdentifier);
        QVERIFY(parentCol.isValid());

        const auto inboxBoxCol = createCollection(Common::InboxBoxId, parentCol, deleter);
        QVERIFY(inboxBoxCol.isValid());

        const auto draftsBoxCol = createCollection(Common::DraftsBoxId, parentCol, deleter);
        QVERIFY(draftsBoxCol.isValid());

        // Load boxes - config is empty so this will create the default Boxes and
        // assign the Inboxes from Knuts to it
        bool loadingDone = true;
        manager.loadBoxes([&loadingDone]() { loadingDone = true; });
        QTRY_VERIFY_WITH_TIMEOUT(loadingDone, milliseconds(10s).count());

        // Now discover collections for the created boxes
        loadingDone = false;
        manager.discoverBoxCollections([&loadingDone]() { loadingDone = true; });
        QTRY_VERIFY_WITH_TIMEOUT(loadingDone, milliseconds(10s).count());

        // Get one of the source collections for Inbox and Drafts
        const auto inboxSourceCol = collectionForRid(QStringLiteral("res1_inbox"));
        QVERIFY(inboxSourceCol.isValid());
        const auto draftsSourceCol = collectionForRid(QStringLiteral("res1_drafts"));
        QVERIFY(draftsSourceCol.isValid());


        // Setup up a monitor to to be notified when an item gets linked into
        // the unified mailbox collection
        Akonadi::Monitor monitor;
        monitor.setCollectionMonitored(inboxBoxCol);
        monitor.setCollectionMonitored(draftsBoxCol);
        QSignalSpy itemLinkedSignalSpy(&monitor, &Akonadi::Monitor::itemsLinked);
        QSignalSpy itemUnlinkedSignalSpy(&monitor, &Akonadi::Monitor::itemsUnlinked);
        QVERIFY(QSignalSpy(&monitor, &Akonadi::Monitor::monitorReady).wait());

        // Add a new Item into the source Inbox collection
        Akonadi::Item item;
        item.setMimeType(QStringLiteral("application/octet-stream"));
        item.setParentCollection(inboxSourceCol);
        item.setPayload(QByteArray{"Hello world!"});
        auto createItem = new Akonadi::ItemCreateJob(item, inboxSourceCol, this);
        AKVERIFYEXEC(createItem);
        item = createItem->item();
        deleter << item;

        // Waity for the item to be linked
        QTRY_COMPARE(itemLinkedSignalSpy.size(), 1);
        itemLinkedSignalSpy.clear();

        // Now move the Item to another Unified mailbox's source collection
        auto move = new Akonadi::ItemMoveJob(item, draftsSourceCol, this);
        AKVERIFYEXEC(move);

        QTRY_COMPARE(itemUnlinkedSignalSpy.size(), 1);
        const auto unlinkedItems = itemUnlinkedSignalSpy.at(0).at(0).value<Akonadi::Item::List>();
        QCOMPARE(unlinkedItems.size(), 1);
        QCOMPARE(unlinkedItems.first(), item);
        const auto unlinkedCol = itemUnlinkedSignalSpy.at(0).at(1).value<Akonadi::Collection>();
        QCOMPARE(unlinkedCol, inboxBoxCol);

        QTRY_COMPARE(itemLinkedSignalSpy.size(), 1);
        const auto linkedItems = itemLinkedSignalSpy.at(0).at(0).value<Akonadi::Item::List>();
        QCOMPARE(linkedItems.size(), 1);
        QCOMPARE(linkedItems.first(), item);
        const auto linkedCol = itemLinkedSignalSpy.at(0).at(1).value<Akonadi::Collection>();
        QCOMPARE(linkedCol, draftsBoxCol);
    }

    void testSourceCollectionRemoved()
    {
        // Setup
        auto kcfg = KSharedConfig::openConfig(QString::fromUtf8(QTest::currentTestFunction()));
        UnifiedMailboxManager manager(kcfg);
        auto &changeRecorder = manager.changeRecorder();
        QSignalSpy crRemovedSpy(&changeRecorder, &Akonadi::Monitor::collectionRemoved);
        EntityDeleter deleter;

        const auto parentCol = collectionForRid(Common::AgentIdentifier);
        QVERIFY(parentCol.isValid());

        const auto inboxBoxCol = createCollection(Common::InboxBoxId, parentCol, deleter);
        QVERIFY(inboxBoxCol.isValid());

        // Load boxes - config is empty so this will create the default Boxes and
        // assign the Inboxes from Knuts to it
        bool loadingDone = true;
        manager.loadBoxes([&loadingDone]() { loadingDone = true; });
        QTRY_VERIFY_WITH_TIMEOUT(loadingDone, milliseconds(10s).count());

        // Now discover collections for the created boxes
        loadingDone = false;
        manager.discoverBoxCollections([&loadingDone]() { loadingDone = true; });
        QTRY_VERIFY_WITH_TIMEOUT(loadingDone, milliseconds(10s).count());

        auto inboxSourceCol = collectionForRid(QStringLiteral("res1_inbox"));
        QVERIFY(inboxSourceCol.isValid());
        auto delJob = new Akonadi::CollectionDeleteJob(inboxSourceCol, this);
        AKVERIFYEXEC(delJob);

        // Wait for the change recorder to be notified
        QVERIFY(crRemovedSpy.wait());
        crRemovedSpy.clear();
        // and then wait a little bit more to give the Manager time to process the event
        QTest::qWait(0);

        auto inboxBox = manager.unifiedMailboxFromCollection(inboxBoxCol);
        QVERIFY(inboxBox);
        QVERIFY(!inboxBox->sourceCollections().contains(inboxSourceCol.id()));
        QVERIFY(!changeRecorder.collectionsMonitored().contains(inboxSourceCol));
        QVERIFY(!manager.unifiedMailboxForSource(inboxSourceCol.id()));

        // Lets removed the other source collection now, that should remove the unified box completely
        inboxSourceCol = collectionForRid(QStringLiteral("res2_inbox"));
        QVERIFY(inboxSourceCol.isValid());
        delJob = new Akonadi::CollectionDeleteJob(inboxSourceCol, this);
        AKVERIFYEXEC(delJob);

        // Wait for the change recorder once again
        QVERIFY(crRemovedSpy.wait());
        QTest::qWait(0);

        QVERIFY(!manager.unifiedMailboxFromCollection(inboxBoxCol));
        QVERIFY(!changeRecorder.collectionsMonitored().contains(inboxSourceCol));
        QVERIFY(!manager.unifiedMailboxForSource(inboxSourceCol.id()));
    }

    void testSpecialSourceCollectionCreated()
    {
        // TODO: this does not work yet: we only monitor collections that we are
        // intersted in, we don't monitor other collections
    }

    void testSpecialSourceCollectionDemoted()
    {
        // Setup
        auto kcfg = KSharedConfig::openConfig(QString::fromUtf8(QTest::currentTestFunction()));
        UnifiedMailboxManager manager(kcfg);
        auto &changeRecorder = manager.changeRecorder();
        QSignalSpy crChangedSpy(&changeRecorder, qOverload<const Akonadi::Collection &, const QSet<QByteArray> &>(&Akonadi::Monitor::collectionChanged));
        EntityDeleter deleter;

        const auto parentCol = collectionForRid(Common::AgentIdentifier);
        QVERIFY(parentCol.isValid());

        const auto sentBoxCol = createCollection(Common::SentBoxId, parentCol, deleter);
        QVERIFY(sentBoxCol.isValid());

        // Load boxes - config is empty so this will create the default Boxes and
        // assign the Inboxes from Knuts to it
        bool loadingDone = true;
        manager.loadBoxes([&loadingDone]() { loadingDone = true; });
        QTRY_VERIFY_WITH_TIMEOUT(loadingDone, milliseconds(10s).count());

        // Now discover collections for the created boxes
        loadingDone = false;
        manager.discoverBoxCollections([&loadingDone]() { loadingDone = true; });
        QTRY_VERIFY_WITH_TIMEOUT(loadingDone, milliseconds(10s).count());

        auto sentSourceCol = collectionForRid(QStringLiteral("res1_sent"));
        QVERIFY(sentSourceCol.isValid());
        sentSourceCol.removeAttribute<Akonadi::SpecialCollectionAttribute>();
        auto modify = new Akonadi::CollectionModifyJob(sentSourceCol, this);
        AKVERIFYEXEC(modify);

        // Wait for the change recorder to be notified
        QVERIFY(crChangedSpy.wait());
        crChangedSpy.clear();
        // and then wait a little bit more to give the Manager time to process the event
        QTest::qWait(0);

        auto sourceBox = manager.unifiedMailboxFromCollection(sentBoxCol);
        QVERIFY(sourceBox);
        QVERIFY(!sourceBox->sourceCollections().contains(sentSourceCol.id()));
        QVERIFY(!changeRecorder.collectionsMonitored().contains(sentSourceCol));
        QVERIFY(!manager.unifiedMailboxForSource(sentSourceCol.id()));

        // Lets demote the other source collection now, that should remove the unified box completely
        sentSourceCol = collectionForRid(QStringLiteral("res2_sent"));
        QVERIFY(sentSourceCol.isValid());
        sentSourceCol.attribute<Akonadi::SpecialCollectionAttribute>()->setCollectionType("drafts");
        modify = new Akonadi::CollectionModifyJob(sentSourceCol, this);
        AKVERIFYEXEC(modify);

        // Wait for the change recorder once again
        QVERIFY(crChangedSpy.wait());
        QTest::qWait(0);

        // There's no more Sent unified box
        QVERIFY(!manager.unifiedMailboxFromCollection(sentBoxCol));

        // The collection is still monitored: it belongs to the Drafts special box now!
        QVERIFY(changeRecorder.collectionsMonitored().contains(sentSourceCol));
        QVERIFY(manager.unifiedMailboxForSource(sentSourceCol.id()));
    }

};

QTEST_AKONADIMAIN(UnifiedMailboxManagerTest)

#include "unifiedmailboxmanagertest.moc"
