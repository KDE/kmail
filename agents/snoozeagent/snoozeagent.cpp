/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozeagent.h"
#include "snoozeagent_debug.h"
#include "snoozeagentadaptor.h"
#include "snoozeagentsettings.h"
#include "snoozeinfo.h"
#include "snoozemanager.h"
#include "snoozeutil.h"

#include <MailCommon/SnoozeAttribute>

#include <Akonadi/AttributeFactory>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/Monitor>
#include <Akonadi/ServerManager>
#include <Akonadi/Session>
#include <KLocalizedString>
#include <KMime/Message>

#include <QDBusConnection>
#include <QTimer>

using namespace Qt::Literals::StringLiterals;

SnoozeAgent::SnoozeAgent(const QString &id)
    : Akonadi::ResourceWidgetBase(id)
    , mManager(new SnoozeManager(this))
    , mMonitor(new Akonadi::Monitor(this))
{
    setAgentName(i18n("Snooze Agent"));
    Akonadi::AttributeFactory::registerAttribute<MailCommon::SnoozeAttribute>();

    new SnoozeAgentAdaptor(this);
    QDBusConnection::sessionBus().registerObject(u"/SnoozeAgent"_s, this, QDBusConnection::ExportAdaptors);
    const QString service = Akonadi::ServerManager::self()->agentServiceName(Akonadi::ServerManager::Agent, u"akonadi_snooze_agent"_s);
    QDBusConnection::sessionBus().registerService(service);

    mMonitor->setObjectName("SnoozeAgentMonitor"_L1);
    mMonitor->setMimeTypeMonitored(KMime::Message::mimeType());
    mMonitor->itemFetchScope().setCacheOnly(true);
    mMonitor->itemFetchScope().setFetchModificationTime(false);
    mMonitor->itemFetchScope().fetchFullPayload(false);
    mMonitor->ignoreSession(Akonadi::Session::defaultSession());
    connect(mMonitor, &Akonadi::Monitor::itemsRemoved, this, &SnoozeAgent::slotItemsRemoved);
    connect(mMonitor, &Akonadi::Monitor::itemsMoved, this, &SnoozeAgent::slotItemsMoved);

    connect(mManager, &SnoozeManager::needUpdateConfigDialogBox, this, &SnoozeAgent::needUpdateConfigDialogBox);
    connect(this, &Akonadi::ResourceBase::collectionTreeSynchronized, this, &SnoozeAgent::discoverSnoozeCollection);

    setNeedsNetwork(false);

    QTimer::singleShot(0, this, &SnoozeAgent::slotStartAgent);
}

SnoozeAgent::~SnoozeAgent() = default;

void SnoozeAgent::slotStartAgent()
{
    mAgentInitialized = true;
    if (isOnline() && enabledAgent()) {
        mManager->load();
        synchronizeCollectionTree();
    }
}

void SnoozeAgent::setEnableAgent(bool enabled)
{
    if (SnoozeAgentSettings::self()->enabled() == enabled) {
        return;
    }
    SnoozeAgentSettings::self()->setEnabled(enabled);
    SnoozeAgentSettings::self()->save();
    if (enabled) {
        mManager->load();
    } else {
        mManager->stopAll();
    }
    // Creates the "Snoozed" collection, or deletes it when disabled.
    synchronizeCollectionTree();
}

bool SnoozeAgent::enabledAgent() const
{
    return SnoozeAgentSettings::self()->enabled();
}

void SnoozeAgent::reload()
{
    if (enabledAgent()) {
        mManager->load(true);
    }
}

void SnoozeAgent::snoozeItem(qint64 itemId, qint64 wakeUpDateTime, const QString &subject, const QString &from)
{
    auto snoozeInfo = new Snooze::SnoozeInfo();
    snoozeInfo->setFrom(from);
    snoozeInfo->setSubject(subject);
    snoozeInfo->setWakeUpDateTime(QDateTime::fromSecsSinceEpoch(wakeUpDateTime, QTimeZone::UTC));
    snoozeInfo->setItemId(itemId);
    mManager->addSnooze(snoozeInfo);
}

void SnoozeAgent::cancelSnooze(qint64 itemId)
{
    mManager->cancelSnooze(itemId);
}

void SnoozeAgent::wakeUpNow(qint64 itemId)
{
    mManager->wakeUpNow(itemId);
}

void SnoozeAgent::slotItemsRemoved(const Akonadi::Item::List &items)
{
    // The manager keeps its list and the config in sync itself: no reload needed.
    for (const Akonadi::Item &item : items) {
        (void)mManager->itemRemoved(item.id());
    }
}

void SnoozeAgent::slotItemsMoved(const Akonadi::Item::List &items)
{
    // Moving a snoozed message by hand is an explicit "I am dealing with it
    // now": cancel the snooze rather than letting it pop back up later.
    for (const Akonadi::Item &item : items) {
        if (mManager->isSnoozed(item.id())) {
            mManager->cancelSnooze(item.id());
        }
    }
}

void SnoozeAgent::doSetOnline(bool online)
{
    if (mAgentInitialized) {
        if (online) {
            reload();
        } else {
            mManager->stopAll();
        }
    }
}

QString SnoozeAgent::printDebugInfo() const
{
    return mManager->printDebugInfo();
}

void SnoozeAgent::retrieveCollections()
{
    if (!enabledAgent()) {
        collectionsRetrieved({});
        return;
    }

    Akonadi::Collection collection;
    collection.setName(identifier());
    collection.setRemoteId(identifier());
    collection.setParentCollection(Akonadi::Collection::root());
    collection.setContentMimeTypes({KMime::Message::mimeType()});
    collection.setRights(Akonadi::Collection::CanChangeItem | Akonadi::Collection::CanDeleteItem);
    collection.setVirtual(true);
    auto displayAttr = collection.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Collection::AddIfMissing);
    displayAttr->setDisplayName(i18n("Snoozed"));
    displayAttr->setIconName(u"chronometer"_s);
    collectionsRetrieved({collection});
}

void SnoozeAgent::retrieveItems(const Akonadi::Collection &collection)
{
    Q_UNUSED(collection)
    // The content is made of links managed by SnoozeItemJob/SnoozeWakeUpJob: nothing to retrieve.
    itemsRetrievedIncremental({}, {});
}

bool SnoozeAgent::retrieveItems(const Akonadi::Item::List &items, [[maybe_unused]] const QSet<QByteArray> &parts)
{
    qCWarning(SNOOZEAGENT_LOG) << "retrieveItems() called for" << items.count() << "items but we can't own any items! This is a bug in Akonadi";
    return false;
}

void SnoozeAgent::discoverSnoozeCollection()
{
    auto job = new Akonadi::CollectionFetchJob(Akonadi::Collection::root(), Akonadi::CollectionFetchJob::FirstLevel, this);
    job->fetchScope().setResource(identifier());
    connect(job, &Akonadi::CollectionFetchJob::result, this, [job]() {
        const Akonadi::Collection::List collections = job->collections();
        const Akonadi::Collection::Id id = collections.isEmpty() ? -1 : collections.first().id();
        if (id != Snooze::SnoozeUtil::snoozeCollectionId()) {
            Snooze::SnoozeUtil::setSnoozeCollectionId(id);
        }
    });
}

AKONADI_RESOURCE_MAIN(SnoozeAgent)

#include "moc_snoozeagent.cpp"
