/*
    Copyright (c) 2011 Tobias Koenig <tokoe@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "mailfilteragent.h"

#include "mailcommon/dbusoperators.h"
#include "dummykernel.h"
#include "filterlogdialog.h"
#include "filtermanager.h"
#include "mailfilteragentadaptor.h"
#include <AkonadiCore/Pop3ResourceAttribute>

#include <AkonadiCore/ServerManager>
#include <AkonadiCore/changerecorder.h>
#include <AkonadiCore/collectionfetchjob.h>
#include <AkonadiCore/collectionfetchscope.h>
#include <kdbusconnectionpool.h>
#include <AkonadiCore/itemfetchscope.h>
#include <Akonadi/KMime/MessageParts>
#include <Akonadi/KMime/MessageStatus>
#include <AkonadiCore/session.h>
#include <MailCommon/MailKernel>
#include <KLocalizedString>
#include <QIcon>
#include "mailfilteragent_debug.h"
#include <KIconLoader>
#include <KMime/Message>
#include <KNotification>
#include <KWindowSystem>
#include <AgentManager>
#include <ItemFetchJob>
#include <AttributeFactory>
#include <KConfigGroup>

#include <QVector>
#include <QTimer>
#include <KSharedConfig>

#include <kdelibs4configmigrator.h>

bool MailFilterAgent::isFilterableCollection(const Akonadi::Collection &collection) const
{
    if (!collection.contentMimeTypes().contains(KMime::Message::mimeType())) {
        return false;
    }

    return m_filterManager->hasAllFoldersFilter() || MailCommon::Kernel::folderIsInbox(collection);

    //TODO: check got filter attribute here
}

MailFilterAgent::MailFilterAgent(const QString &id)
    : Akonadi::AgentBase(id)
    , m_filterLogDialog(nullptr)
{
    Kdelibs4ConfigMigrator migrate(QStringLiteral("mailfilteragent"));
    migrate.setConfigFiles(QStringList() << QStringLiteral("akonadi_mailfilter_agentrc") << QStringLiteral("akonadi_mailfilter_agent.notifyrc"));
    migrate.migrate();

    Akonadi::AttributeFactory::registerAttribute<Akonadi::Pop3ResourceAttribute>();
    mMailFilterKernel = new DummyKernel(this);
    CommonKernel->registerKernelIf(mMailFilterKernel);   //register KernelIf early, it is used by the Filter classes
    CommonKernel->registerSettingsIf(mMailFilterKernel);   //SettingsIf is used in FolderTreeWidget

    m_filterManager = new FilterManager(this);

    connect(m_filterManager, &FilterManager::percent, this, &MailFilterAgent::emitProgress);
    connect(m_filterManager, &FilterManager::progressMessage, this, &MailFilterAgent::emitProgressMessage);

    Akonadi::Monitor *collectionMonitor = new Akonadi::Monitor(this);
    collectionMonitor->fetchCollection(true);
    collectionMonitor->ignoreSession(Akonadi::Session::defaultSession());
    collectionMonitor->collectionFetchScope().setAncestorRetrieval(Akonadi::CollectionFetchScope::All);
    collectionMonitor->setMimeTypeMonitored(KMime::Message::mimeType());

    connect(collectionMonitor, &Akonadi::Monitor::collectionAdded, this, &MailFilterAgent::mailCollectionAdded);
    connect(collectionMonitor, SIGNAL(collectionChanged(Akonadi::Collection)),
            this, SLOT(mailCollectionChanged(Akonadi::Collection)));

    connect(collectionMonitor, &Akonadi::Monitor::collectionRemoved, this, &MailFilterAgent::mailCollectionRemoved);
    connect(Akonadi::AgentManager::self(), &Akonadi::AgentManager::instanceRemoved, this, &MailFilterAgent::slotInstanceRemoved);

    QTimer::singleShot(0, this, &MailFilterAgent::initializeCollections);

    qDBusRegisterMetaType<QList<qint64> >();

    new MailFilterAgentAdaptor(this);

    KDBusConnectionPool::threadConnection().registerObject(QStringLiteral("/MailFilterAgent"), this, QDBusConnection::ExportAdaptors);
    QString service = QStringLiteral("org.freedesktop.Akonadi.MailFilterAgent");
    if (Akonadi::ServerManager::hasInstanceIdentifier()) {
        service += QLatin1Char('.') + Akonadi::ServerManager::instanceIdentifier();
    }

    KDBusConnectionPool::threadConnection().registerService(service);
    //Enabled or not filterlogdialog
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    if (config->hasGroup("FilterLog")) {
        KConfigGroup group(config, "FilterLog");
        if (group.readEntry("Enabled", false)) {
            const QPixmap pixmap = QIcon::fromTheme(QStringLiteral("view-filter")).pixmap(KIconLoader::SizeSmall, KIconLoader::SizeSmall);
            KNotification *notify = new KNotification(QStringLiteral("mailfilterlogenabled"));
            notify->setComponentName(QApplication::applicationDisplayName());
            notify->setPixmap(pixmap);
            notify->setText(i18nc("Notification when the filter log was enabled", "Mail Filter Log Enabled"));
            notify->sendEvent();
        }
    }

    changeRecorder()->itemFetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
    changeRecorder()->itemFetchScope().setCacheOnly(true);
    changeRecorder()->fetchCollection(true);
    changeRecorder()->setChangeRecordingEnabled(false);

    mProgressCounter = 0;
    mProgressTimer = new QTimer(this);
    connect(mProgressTimer, SIGNAL(timeout()), this, SLOT(emitProgress()));

    itemMonitor = new Akonadi::Monitor(this);
    itemMonitor->itemFetchScope().setFetchRemoteIdentification(true);
    itemMonitor->itemFetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
    connect(itemMonitor, &Akonadi::Monitor::itemChanged, this, &MailFilterAgent::slotItemChanged);
}

MailFilterAgent::~MailFilterAgent()
{
    delete m_filterLogDialog;
}

void MailFilterAgent::configure(WId windowId)
{
    Q_UNUSED(windowId);
}

void MailFilterAgent::initializeCollections()
{
    m_filterManager->readConfig();

    Akonadi::CollectionFetchJob *job = new Akonadi::CollectionFetchJob(Akonadi::Collection::root(),
                                                                       Akonadi::CollectionFetchJob::Recursive,
                                                                       this);
    job->fetchScope().setContentMimeTypes({ KMime::Message::mimeType() });
    connect(job, &Akonadi::CollectionFetchJob::result,
            this, &MailFilterAgent::initialCollectionFetchingDone);
}

void MailFilterAgent::initialCollectionFetchingDone(KJob *job)
{
    if (job->error()) {
        qCWarning(MAILFILTERAGENT_LOG) << job->errorString();
        return; //TODO: proper error handling
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
    QTimer::singleShot(2000, this, &MailFilterAgent::clearMessage);
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
        itemMonitor->setItemMonitored(item);
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
    itemMonitor->setItemMonitored(item, false);
    filterItem(item, item.parentCollection());
}

void MailFilterAgent::filterItem(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    MailCommon::SearchRule::RequiredPart requiredPart = m_filterManager->requiredPart(collection.resource());

    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob(item);
    connect(job, &Akonadi::ItemFetchJob::itemsReceived,
            this, &MailFilterAgent::itemsReceiviedForFiltering);
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

    //TODO: Error handling?
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
    m_filterManager->process(item, m_filterManager->requiredPart(resource), FilterManager::Inbound, true, resource);

    emitProgress(++mProgressCounter);

    mProgressTimer->start(1000);
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
    m_filterManager->mailCollectionRemoved(collection);
}

QString MailFilterAgent::createUniqueName(const QString &nameTemplate)
{
    return m_filterManager->createUniqueName(nameTemplate);
}

void MailFilterAgent::filterItems(const QList< qint64 > &itemIds, int filterSet)
{
    Akonadi::Item::List items;
    items.reserve(itemIds.count());
    for (qint64 id : itemIds) {
        items << Akonadi::Item(id);
    }

    m_filterManager->applyFilters(items, static_cast<FilterManager::FilterSet>(filterSet));
}

void MailFilterAgent::applySpecificFilters(const QList< qint64 > &itemIds, int requires, const QStringList &listFilters)
{
    Akonadi::Item::List items;
    items.reserve(itemIds.count());
    for (qint64 id : itemIds) {
        items << Akonadi::Item(id);
    }

    m_filterManager->applySpecificFilters(items, static_cast<MailCommon::SearchRule::RequiredPart>(requires), listFilters);
}

void MailFilterAgent::filterItem(qint64 item, int filterSet, const QString &resourceId)
{
    m_filterManager->filter(Akonadi::Item(item), static_cast<FilterManager::FilterSet>(filterSet), resourceId);
}

void MailFilterAgent::filter(qint64 item, const QString &filterIdentifier, const QString &resourceId)
{
    m_filterManager->filter(Akonadi::Item(item), filterIdentifier, resourceId);
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
    if (!m_filterLogDialog) {
        m_filterLogDialog = new FilterLogDialog(nullptr);
    }
    KWindowSystem::setMainWindow(m_filterLogDialog, windowId);
    m_filterLogDialog->show();
    m_filterLogDialog->raise();
    m_filterLogDialog->activateWindow();
    m_filterLogDialog->setModal(false);
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

QString MailFilterAgent::printCollectionMonitored()
{
    QString printDebugCollection;
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
    return printDebugCollection;
}

void MailFilterAgent::showConfigureDialog(qlonglong windowId)
{
    Q_UNUSED(windowId);
    //TODO
}

void MailFilterAgent::expunge(qint64 collectionId)
{
    mMailFilterKernel->expunge(collectionId, false);
}

void MailFilterAgent::slotInstanceRemoved(const Akonadi::AgentInstance &instance)
{
    m_filterManager->agentRemoved(instance.identifier());
}

AKONADI_AGENT_MAIN(MailFilterAgent)
