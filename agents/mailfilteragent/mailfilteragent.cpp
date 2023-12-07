/*
    SPDX-FileCopyrightText: 2011 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "mailfilteragent.h"

#include "dummykernel.h"
#include "filterlogdialog.h"
#include "filtermanager.h"
#include "mailfilteragentadaptor.h"
#include <MailCommon/DBusOperators>

#include "mailfilteragent_debug.h"
#include <Akonadi/AgentManager>
#include <Akonadi/AttributeFactory>
#include <Akonadi/ChangeRecorder>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/MessageParts>
#include <Akonadi/MessageStatus>
#include <Akonadi/Pop3ResourceAttribute>
#include <Akonadi/ServerManager>
#include <Akonadi/Session>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KMime/Message>
#include <KNotification>
#include <KWindowSystem>
#include <MailCommon/MailKernel>
#include <QDBusConnection>

#include <KSharedConfig>
#include <QTimer>

#include <chrono>

using namespace std::chrono_literals;
bool MailFilterAgent::isFilterableCollection(const Akonadi::Collection &collection) const
{
    if (!collection.contentMimeTypes().contains(KMime::Message::mimeType())) {
        return false;
    }

    return mFilterManager->hasAllFoldersFilter() || MailCommon::Kernel::folderIsInbox(collection);

    // TODO: check got filter attribute here
}

MailFilterAgent::MailFilterAgent(const QString &id)
    : Akonadi::AgentBase(id)
    , mProgressTimer(new QTimer(this))
{
    qRegisterMetaType<QList<qint64>>("QList<qint64>");
    Akonadi::AttributeFactory::registerAttribute<Akonadi::Pop3ResourceAttribute>();
    mMailFilterKernel = new DummyKernel(this);
    CommonKernel->registerKernelIf(mMailFilterKernel); // register KernelIf early, it is used by the Filter classes
    CommonKernel->registerSettingsIf(mMailFilterKernel); // SettingsIf is used in FolderTreeWidget

    // Initialize it after registring CommonKernel otherwise it crashs!
    mFilterManager = new FilterManager(this);

    connect(mFilterManager, &FilterManager::percent, this, &MailFilterAgent::emitProgress);
    connect(mFilterManager, &FilterManager::progressMessage, this, &MailFilterAgent::emitProgressMessage);

    auto collectionMonitor = new Akonadi::Monitor(this);
    collectionMonitor->setObjectName(QLatin1StringView("MailFilterCollectionMonitor"));
    collectionMonitor->fetchCollection(true);
    collectionMonitor->ignoreSession(Akonadi::Session::defaultSession());
    collectionMonitor->collectionFetchScope().setAncestorRetrieval(Akonadi::CollectionFetchScope::All);
    collectionMonitor->setMimeTypeMonitored(KMime::Message::mimeType());

    connect(collectionMonitor, &Akonadi::Monitor::collectionAdded, this, &MailFilterAgent::mailCollectionAdded);
    connect(collectionMonitor, qOverload<const Akonadi::Collection &>(&Akonadi::Monitor::collectionChanged), this, &MailFilterAgent::mailCollectionChanged);

    connect(collectionMonitor, &Akonadi::Monitor::collectionRemoved, this, &MailFilterAgent::mailCollectionRemoved);
    connect(Akonadi::AgentManager::self(), &Akonadi::AgentManager::instanceRemoved, this, &MailFilterAgent::slotInstanceRemoved);

    QTimer::singleShot(0, this, &MailFilterAgent::initializeCollections);

    qDBusRegisterMetaType<QList<qint64>>();

    new MailFilterAgentAdaptor(this);

    QDBusConnection::sessionBus().registerObject(QStringLiteral("/MailFilterAgent"), this, QDBusConnection::ExportAdaptors);

    const QString service = Akonadi::ServerManager::self()->agentServiceName(Akonadi::ServerManager::Agent, QStringLiteral("akonadi_mailfilter_agent"));

    QDBusConnection::sessionBus().registerService(service);
    // Enabled or not filterlogdialog
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    if (config->hasGroup(QLatin1String("FilterLog"))) {
        KConfigGroup group(config, QStringLiteral("FilterLog"));
        if (group.readEntry("Enabled", false)) {
            auto notify = new KNotification(QStringLiteral("mailfilterlogenabled"));
            notify->setComponentName(QApplication::applicationDisplayName());
            notify->setIconName(QStringLiteral("view-filter"));
            notify->setText(i18nc("Notification when the filter log was enabled", "Mail Filter Log Enabled"));
            notify->sendEvent();
        }
    }

    changeRecorder()->itemFetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
    changeRecorder()->itemFetchScope().setCacheOnly(true);
    changeRecorder()->fetchCollection(true);
    changeRecorder()->setChangeRecordingEnabled(false);

    connect(mProgressTimer, &QTimer::timeout, this, [this]() {
        emitProgress();
    });

    mItemMonitor = new Akonadi::Monitor(this);
    mItemMonitor->setObjectName(QLatin1StringView("MailFilterItemMonitor"));
    mItemMonitor->itemFetchScope().setFetchRemoteIdentification(true);
    mItemMonitor->itemFetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
    connect(mItemMonitor, &Akonadi::Monitor::itemChanged, this, &MailFilterAgent::slotItemChanged);
}

MailFilterAgent::~MailFilterAgent()
{
    delete mFilterLogDialog;
}

void MailFilterAgent::configure(WId windowId)
{
    Q_UNUSED(windowId)
}

void MailFilterAgent::initializeCollections()
{
    mFilterManager->readConfig();

    auto job = new Akonadi::CollectionFetchJob(Akonadi::Collection::root(), Akonadi::CollectionFetchJob::Recursive, this);
    job->fetchScope().setContentMimeTypes({KMime::Message::mimeType()});
    connect(job, &Akonadi::CollectionFetchJob::result, this, &MailFilterAgent::initialCollectionFetchingDone);
}

void MailFilterAgent::initialCollectionFetchingDone(KJob *job)
{
    if (job->error()) {
        qCWarning(MAILFILTERAGENT_LOG) << job->errorString();
        return; // TODO: proper error handling
    }

    const auto fetchJob = qobject_cast<Akonadi::CollectionFetchJob *>(job);

    const auto pop3ResourceMap = MailCommon::Kernel::pop3ResourceTargetCollection();

    const auto lstCols = fetchJob->collections();
    for (const Akonadi::Collection &collection : lstCols) {
        if (isFilterableCollection(collection)) {
            changeRecorder()->setCollectionMonitored(collection, true);
        } else {
            for (auto pop3ColId : pop3ResourceMap) {
                if (collection.id() == pop3ColId) {
                    changeRecorder()->setCollectionMonitored(collection, true);
                    break;
                }
            }
        }
    }
    Q_EMIT status(AgentBase::Idle, i18n("Ready"));
    Q_EMIT percent(100);
    QTimer::singleShot(2s, this, &MailFilterAgent::clearMessage);
}

void MailFilterAgent::clearMessage()
{
    Q_EMIT status(AgentBase::Idle, QString());
}

void MailFilterAgent::itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    /* The monitor mimetype filter would override the collection filter, therefor we have to check
     * for the mimetype of the item here.
     */
    if (item.mimeType() != KMime::Message::mimeType()) {
        qCDebug(MAILFILTERAGENT_LOG) << "MailFilterAgent::itemAdded called for a non-message item!";
        return;
    }

    if (item.remoteId().isEmpty()) {
        mItemMonitor->setItemMonitored(item);
    } else {
        filterItem(item, collection);
    }
}

void MailFilterAgent::slotItemChanged(const Akonadi::Item &item)
{
    if (item.remoteId().isEmpty()) {
        return;
    }

    // now we have the remoteId
    mItemMonitor->setItemMonitored(item, false);
    filterItem(item, item.parentCollection());
}

void MailFilterAgent::filterItem(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    MailCommon::SearchRule::RequiredPart requiredPart = mFilterManager->requiredPart(collection.resource());

    auto job = new Akonadi::ItemFetchJob(item);
    connect(job, &Akonadi::ItemFetchJob::itemsReceived, this, &MailFilterAgent::itemsReceiviedForFiltering);
    if (requiredPart == MailCommon::SearchRule::CompleteMessage) {
        job->fetchScope().fetchFullPayload();
    } else if (requiredPart == MailCommon::SearchRule::Header) {
        job->fetchScope().fetchPayloadPart(Akonadi::MessagePart::Header, true);
    } else {
        job->fetchScope().fetchPayloadPart(Akonadi::MessagePart::Envelope, true);
    }
    job->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
    job->fetchScope().fetchAttribute<Akonadi::Pop3ResourceAttribute>();
    job->setProperty("resource", collection.resource());

    // TODO: Error handling?
}

void MailFilterAgent::itemsReceiviedForFiltering(const Akonadi::Item::List &items)
{
    if (items.isEmpty()) {
        qCDebug(MAILFILTERAGENT_LOG) << "MailFilterAgent::itemsReceiviedForFiltering items is empty!";
        return;
    }

    Akonadi::Item item = items.first();
    /*
     * happens when item no longer exists etc, and queue compression didn't happen yet
     */
    if (!item.hasPayload()) {
        qCDebug(MAILFILTERAGENT_LOG) << "MailFilterAgent::itemsReceiviedForFiltering item has no payload!";
        return;
    }

    Akonadi::MessageStatus status;
    status.setStatusFromFlags(item.flags());
    if (status.isRead() || status.isSpam() || status.isIgnored()) {
        return;
    }

    QString resource = sender()->property("resource").toString();
    const Akonadi::Pop3ResourceAttribute *pop3ResourceAttribute = item.attribute<Akonadi::Pop3ResourceAttribute>();
    if (pop3ResourceAttribute) {
        resource = pop3ResourceAttribute->pop3AccountName();
    }

    emitProgressMessage(i18n("Filtering in %1", Akonadi::AgentManager::self()->instance(resource).name()));
    if (!mFilterManager->process(item, mFilterManager->requiredPart(resource), FilterManager::Inbound, true, resource)) {
        qCWarning(MAILFILTERAGENT_LOG) << "Impossible to process mails";
    }

    emitProgress(++mProgressCounter);

    mProgressTimer->start(1s);
}

void MailFilterAgent::mailCollectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &)
{
    if (isFilterableCollection(collection)) {
        changeRecorder()->setCollectionMonitored(collection, true);
    }
}

void MailFilterAgent::mailCollectionChanged(const Akonadi::Collection &collection)
{
    changeRecorder()->setCollectionMonitored(collection, isFilterableCollection(collection));
}

void MailFilterAgent::mailCollectionRemoved(const Akonadi::Collection &collection)
{
    changeRecorder()->setCollectionMonitored(collection, false);
    mFilterManager->mailCollectionRemoved(collection);
}

QString MailFilterAgent::createUniqueName(const QString &nameTemplate)
{
    return mFilterManager->createUniqueName(nameTemplate);
}

void MailFilterAgent::filterItems(const QList<qint64> &itemIds, int filterSet)
{
    qDebug() << " MailFilterAgent::filterItems ";
    Akonadi::Item::List items;
    items.reserve(itemIds.count());
    for (qint64 id : itemIds) {
        items << Akonadi::Item(id);
    }

    mFilterManager->applyFilters(items, static_cast<FilterManager::FilterSet>(filterSet));
}

void MailFilterAgent::filterCollections(const QList<qint64> &collections, int filterSet)
{
    for (qint64 id : collections) {
        auto ifj = new Akonadi::ItemFetchJob{Akonadi::Collection{id}, this};
        ifj->setDeliveryOption(Akonadi::ItemFetchJob::EmitItemsInBatches);
        connect(ifj, &Akonadi::ItemFetchJob::itemsReceived, this, [this, filterSet](const Akonadi::Item::List &items) {
            mFilterManager->applyFilters(items, static_cast<FilterManager::FilterSet>(filterSet));
        });
    }
}

void MailFilterAgent::applySpecificFilters(const QList<qint64> &itemIds, int requiresPart, const QStringList &listFilters)
{
    Akonadi::Item::List items;
    items.reserve(itemIds.count());
    for (qint64 id : itemIds) {
        items << Akonadi::Item(id);
    }

    mFilterManager->applySpecificFilters(items, static_cast<MailCommon::SearchRule::RequiredPart>(requiresPart), listFilters);
}

void MailFilterAgent::applySpecificFiltersOnCollections(const QList<qint64> &colIds, const QStringList &listFilters, int filterSet)
{
    // TODO: Actually calculate this based on the listFilters' requirements
    const auto requiresParts = MailCommon::SearchRule::CompleteMessage;

    for (qint64 id : colIds) {
        auto ifj = new Akonadi::ItemFetchJob{Akonadi::Collection{id}, this};
        ifj->setDeliveryOption(Akonadi::ItemFetchJob::EmitItemsInBatches);
        connect(ifj, &Akonadi::ItemFetchJob::itemsReceived, this, [this, listFilters, filterSet](const Akonadi::Item::List &items) {
            mFilterManager->applySpecificFilters(items, requiresParts, listFilters, static_cast<FilterManager::FilterSet>(filterSet));
        });
    }
}

void MailFilterAgent::filterItem(qint64 item, int filterSet, const QString &resourceId)
{
    qDebug() << " MailFilterAgent::filterItem ";
    mFilterManager->filter(Akonadi::Item(item), static_cast<FilterManager::FilterSet>(filterSet), resourceId);
}

void MailFilterAgent::filter(qint64 item, const QString &filterIdentifier, const QString &resourceId)
{
    mFilterManager->filter(Akonadi::Item(item), filterIdentifier, resourceId);
}

void MailFilterAgent::reload()
{
    const Akonadi::Collection::List collections = changeRecorder()->collectionsMonitored();
    for (const Akonadi::Collection &collection : collections) {
        changeRecorder()->setCollectionMonitored(collection, false);
    }
    initializeCollections();
}

void MailFilterAgent::showFilterLogDialog(qlonglong windowId)
{
    if (!mFilterLogDialog) {
        mFilterLogDialog = new FilterLogDialog(nullptr);
        mFilterLogDialog->setAttribute(Qt::WA_NativeWindow, true);
    }
    KWindowSystem::setMainWindow(mFilterLogDialog->windowHandle(), windowId);
    mFilterLogDialog->show();
    mFilterLogDialog->raise();
    mFilterLogDialog->activateWindow();
    mFilterLogDialog->setModal(false);
}

void MailFilterAgent::emitProgress(int p)
{
    if (p == 0) {
        mProgressTimer->stop();
        Q_EMIT status(AgentBase::Idle, QString());
    }
    mProgressCounter = p;
    Q_EMIT percent(p);
}

void MailFilterAgent::emitProgressMessage(const QString &message)
{
    Q_EMIT status(AgentBase::Running, message);
}

QString MailFilterAgent::printCollectionMonitored() const
{
    QString printDebugCollection = QStringLiteral("Start print collection monitored\n");

    const Akonadi::Collection::List collections = changeRecorder()->collectionsMonitored();
    if (collections.isEmpty()) {
        printDebugCollection = QStringLiteral("No collection is monitored!");
    } else {
        for (const Akonadi::Collection &collection : collections) {
            if (!printDebugCollection.isEmpty()) {
                printDebugCollection += QLatin1Char('\n');
            }
            printDebugCollection += QStringLiteral("Collection name: %1\n").arg(collection.name());
            printDebugCollection += QStringLiteral("Collection id: %1\n").arg(collection.id());
        }
    }
    printDebugCollection += QStringLiteral("End print collection monitored\n");
    return printDebugCollection;
}

void MailFilterAgent::expunge(qint64 collectionId)
{
    mMailFilterKernel->expunge(collectionId, false);
}

void MailFilterAgent::slotInstanceRemoved(const Akonadi::AgentInstance &instance)
{
    mFilterManager->agentRemoved(instance.identifier());
}

AKONADI_AGENT_MAIN(MailFilterAgent)

#include "moc_mailfilteragent.cpp"
