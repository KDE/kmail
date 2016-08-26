//
/* Copyright: before 2012: missing, see KMail copyrights
 * Copyright (C) 2012 Andras Mantia <amantia@kde.org> *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#include "filtermanager.h"

#include <AkonadiCore/agentmanager.h>
#include <AkonadiCore/changerecorder.h>
#include <AkonadiCore/collectionfetchjob.h>
#include <AkonadiCore/collectionfetchscope.h>
#include <AkonadiCore/itemfetchjob.h>
#include <AkonadiCore/itemfetchscope.h>
#include <AkonadiCore/itemmodifyjob.h>
#include <AkonadiCore/itemmovejob.h>
#include <AkonadiCore/itemdeletejob.h>
#include <Akonadi/KMime/MessageParts>
#include <kconfig.h>
#include <kconfiggroup.h>
#include "mailfilteragent_debug.h"
#include <KLocalizedString>
#include <KNotification>
#include <QIcon>
#include <KIconLoader>
#include <kmime/kmime_message.h>
#include <MailCommon/FilterImporterExporter>
#include <MailCommon/FilterLog>
#include <MailCommon/MailFilter>
#include <MailCommon/MailUtil>

#include <QTimer>

// other headers
#include <algorithm>
#include <boost/bind.hpp>
#include <errno.h>
#include <KSharedConfig>
#include <QLocale>

using namespace MailCommon;

class FilterManager::Private
{
public:
    Private(FilterManager *qq)
        : q(qq),
          mRequiredPartsBasedOnAll(SearchRule::Envelope),
          mInboundFiltersExist(false),
          mTotalProgressCount(0),
          mCurrentProgressCount(0)
    {
        pixmapNotification = QIcon::fromTheme(QStringLiteral("view-filter")).pixmap(KIconLoader::SizeSmall, KIconLoader::SizeSmall);
    }

    void itemsFetchJobForFilterDone(KJob *job);
    void itemFetchJobForFilterDone(KJob *job);
    void moveJobResult(KJob *);
    void modifyJobResult(KJob *);
    void deleteJobResult(KJob *);
    void slotItemsFetchedForFilter(const Akonadi::Item::List &items);
    void showNotification(const QString &errorMsg, const QString &jobErrorString);

    bool isMatching(const Akonadi::Item &item, const MailCommon::MailFilter *filter);
    void beginFiltering(const Akonadi::Item &item) const;
    void endFiltering(const Akonadi::Item &item) const;
    bool atLeastOneFilterAppliesTo(const QString &accountId) const;
    bool atLeastOneIncomingFilterAppliesTo(const QString &accountId) const;
    FilterManager *q;
    QList<MailCommon::MailFilter *> mFilters;
    QMap<QString, SearchRule::RequiredPart> mRequiredParts;
    SearchRule::RequiredPart mRequiredPartsBasedOnAll;
    QPixmap pixmapNotification;
    bool mInboundFiltersExist;
    int mTotalProgressCount;
    int mCurrentProgressCount;
};

void FilterManager::Private::slotItemsFetchedForFilter(const Akonadi::Item::List &items)
{
    FilterManager::FilterSet filterSet = FilterManager::Inbound;
    if (q->sender()->property("filterSet").isValid()) {
        filterSet = static_cast<FilterManager::FilterSet>(q->sender()->property("filterSet").toInt());
    }

    QList<MailFilter *> listMailFilters;
    if (q->sender()->property("listFilters").isValid()) {
        const QStringList listFilters = q->sender()->property("listFilters").toStringList();
        //TODO improve it
        foreach (const QString &filterId, listFilters) {
            foreach (MailCommon::MailFilter *filter, mFilters) {
                if (filter->identifier() == filterId) {
                    listMailFilters << filter;
                    break;
                }
            }
        }
    }

    if (listMailFilters.isEmpty()) {
        listMailFilters = mFilters;
    }

    bool needsFullPayload = q->sender()->property("needsFullPayload").toBool();

    foreach (const Akonadi::Item &item, items) {
        ++mCurrentProgressCount;

        if ((mTotalProgressCount > 0) && (mCurrentProgressCount != mTotalProgressCount)) {
            const QString statusMsg =
                i18n("Filtering message %1 of %2", mCurrentProgressCount, mTotalProgressCount);
            Q_EMIT q->progressMessage(statusMsg);
            Q_EMIT q->percent(mCurrentProgressCount * 100 / mTotalProgressCount);
        } else {
            Q_EMIT q->percent(0);
        }

        const bool filterResult = q->process(listMailFilters, item, needsFullPayload, filterSet);

        if (mCurrentProgressCount == mTotalProgressCount) {
            mTotalProgressCount = 0;
            mCurrentProgressCount = 0;
        }

        if (!filterResult) {
            Q_EMIT q->filteringFailed(item);
            // something went horribly wrong (out of space?)
            //CommonKernel->emergencyExit( i18n( "Unable to process messages: " ) + QString::fromLocal8Bit( strerror( errno ) ) );
        }
    }
}

void FilterManager::Private::itemsFetchJobForFilterDone(KJob *job)
{
    if (job->error()) {
        qCCritical(MAILFILTERAGENT_LOG) << "Error while fetching items. " << job->error() << job->errorString();
    }
}

void FilterManager::Private::itemFetchJobForFilterDone(KJob *job)
{
    if (job->error()) {
        qCCritical(MAILFILTERAGENT_LOG) << "Error while fetching item. " << job->error() << job->errorString();
        return;
    }

    const Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob *>(job);

    const Akonadi::Item::List items = fetchJob->items();
    if (items.isEmpty()) {
        qCCritical(MAILFILTERAGENT_LOG) << "Error while fetching item: item not found";
        return;
    }

    const QString resourceId = fetchJob->property("resourceId").toString();
    bool needsFullPayload = q->requiredPart(resourceId) != SearchRule::Envelope;

    if (job->property("filterId").isValid()) {
        const QString filterId = job->property("filterId").toString();

        // find correct filter object
        MailCommon::MailFilter *wantedFilter = Q_NULLPTR;
        foreach (MailCommon::MailFilter *filter, mFilters) {
            if (filter->identifier() == filterId) {
                wantedFilter = filter;
                break;
            }
        }

        if (!wantedFilter) {
            qCCritical(MAILFILTERAGENT_LOG) << "Cannot find filter object with id" << filterId;
            return;
        }

        if (!q->process(items.first(), needsFullPayload, wantedFilter)) {
            Q_EMIT q->filteringFailed(items.first());
        }
    } else {
        const FilterManager::FilterSet set = static_cast<FilterManager::FilterSet>(job->property("filterSet").toInt());

        if (!q->process(items.first(), needsFullPayload, set, !resourceId.isEmpty(), resourceId)) {
            Q_EMIT q->filteringFailed(items.first());
        }
    }
}

void FilterManager::Private::moveJobResult(KJob *job)
{
    if (job->error()) {
        const Akonadi::ItemMoveJob *movejob = qobject_cast<Akonadi::ItemMoveJob *>(job);
        if (movejob) {
            qCCritical(MAILFILTERAGENT_LOG) << "Error while moving items. " << job->error() << job->errorString()
                                            << " to destinationCollection.id() :" << movejob->destinationCollection().id();
        } else {
            qCCritical(MAILFILTERAGENT_LOG) << "Error while moving items. " << job->error() << job->errorString();
        }
        //Laurent: not real info and when we have 200 errors it's very long to click all the time on ok.
        showNotification(i18n("Error applying mail filter move"), job->errorString());
    }
}

void FilterManager::Private::deleteJobResult(KJob *job)
{
    if (job->error()) {
        qCCritical(MAILFILTERAGENT_LOG) << "Error while delete items. " << job->error() << job->errorString();
        showNotification(i18n("Error applying mail filter delete"), job->errorString());
    }
}

void FilterManager::Private::modifyJobResult(KJob *job)
{
    if (job->error()) {
        qCCritical(MAILFILTERAGENT_LOG) << "Error while modifying items. " << job->error() << job->errorString();
        showNotification(i18n("Error applying mail filter modifications"), job->errorString());
    }
}

void FilterManager::Private::showNotification(const QString &errorMsg, const QString &jobErrorString)
{
    KNotification *notify = new KNotification(QStringLiteral("mailfilterjoberror"));
    notify->setComponentName(QStringLiteral("akonadi_mailfilter_agent"));
    notify->setPixmap(pixmapNotification);
    notify->setText(errorMsg + QLatin1Char('\n') + jobErrorString);
    notify->sendEvent();
}

bool FilterManager::Private::isMatching(const Akonadi::Item &item, const MailCommon::MailFilter *filter)
{
    bool result = false;
    if (FilterLog::instance()->isLogging()) {
        QString logText(i18n("<b>Evaluating filter rules:</b> "));
        logText.append(filter->pattern()->asString());
        FilterLog::instance()->add(logText, FilterLog::PatternDescription);
    }

    if (filter->pattern()->matches(item)) {
        if (FilterLog::instance()->isLogging()) {
            FilterLog::instance()->add(i18n("<b>Filter rules have matched.</b>"),
                                       FilterLog::PatternResult);
        }

        result = true;
    }

    return result;
}

void FilterManager::Private::beginFiltering(const Akonadi::Item &item) const
{
    if (FilterLog::instance()->isLogging()) {
        FilterLog::instance()->addSeparator();
        if (item.hasPayload<KMime::Message::Ptr>()) {
            KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();
            const QString subject = msg->subject()->asUnicodeString();
            const QString from = msg->from()->asUnicodeString();
            const QDateTime dateTime = msg->date()->dateTime();
            const QString date = QLocale().toString(dateTime, QLocale::LongFormat);
            const QString logText(i18n("<b>Begin filtering on message \"%1\" from \"%2\" at \"%3\" :</b>",
                                       subject, from, date));
            FilterLog::instance()->add(logText, FilterLog::PatternDescription);
        }
    }
}

void FilterManager::Private::endFiltering(const Akonadi::Item &/*item*/) const
{
}

bool FilterManager::Private::atLeastOneFilterAppliesTo(const QString &accountId) const
{
    foreach (const MailCommon::MailFilter *filter, mFilters) {
        if (filter->applyOnAccount(accountId)) {
            return true;
        }
    }

    return false;
}

bool FilterManager::Private::atLeastOneIncomingFilterAppliesTo(const QString &accountId) const
{
    foreach (const MailCommon::MailFilter *filter, mFilters) {
        if (filter->applyOnInbound() && filter->applyOnAccount(accountId)) {
            return true;
        }
    }

    return false;
}

FilterManager::FilterManager(QObject *parent)
    : QObject(parent), d(new Private(this))
{
    readConfig();
}

FilterManager::~FilterManager()
{
    clear();

    delete d;
}

void FilterManager::clear()
{
    qDeleteAll(d->mFilters);
    d->mFilters.clear();
}

void FilterManager::readConfig()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig(); // use akonadi_mailfilter_agentrc
    config->reparseConfiguration();
    clear();

    QStringList emptyFilters;
    d->mFilters = FilterImporterExporter::readFiltersFromConfig(config, emptyFilters);
    d->mRequiredParts.clear();

    d->mRequiredPartsBasedOnAll = SearchRule::Envelope;
    if (!d->mFilters.isEmpty()) {
        Akonadi::AgentInstance::List agents = Akonadi::AgentManager::self()->instances();
        foreach (const Akonadi::AgentInstance &agent, agents) {
            const QString id = agent.identifier();

            QList<MailFilter *>::const_iterator it = std::max_element(d->mFilters.constBegin(), d->mFilters.constEnd(),
                    boost::bind(&MailCommon::MailFilter::requiredPart, _1, id)
                    < boost::bind(&MailCommon::MailFilter::requiredPart, _2, id));
            d->mRequiredParts[id] = (*it)->requiredPart(id);
            d->mRequiredPartsBasedOnAll = qMax(d->mRequiredPartsBasedOnAll, d->mRequiredParts[id]);
        }
    }
    // check if at least one filter is to be applied on inbound mail
    d->mInboundFiltersExist = std::find_if(d->mFilters.constBegin(), d->mFilters.constEnd(),
                                           boost::bind(&MailCommon::MailFilter::applyOnInbound, _1)) != d->mFilters.constEnd();

    Q_EMIT filterListUpdated();
}

void FilterManager::mailCollectionRemoved(const Akonadi::Collection &collection)
{
    QList<MailCommon::MailFilter *>::const_iterator end(d->mFilters.constEnd());
    for (QList<MailCommon::MailFilter *>::const_iterator it = d->mFilters.constBegin();
            it != end; ++it) {
        (*it)->folderRemoved(collection, Akonadi::Collection());
    }
}

void FilterManager::agentRemoved(const QString &identifier)
{
    QList<MailCommon::MailFilter *>::const_iterator end(d->mFilters.constEnd());
    for (QList<MailCommon::MailFilter *>::const_iterator it = d->mFilters.constBegin();
            it != end; ++it) {
        (*it)->agentRemoved(identifier);
    }
}

void FilterManager::filter(const Akonadi::Item &item, FilterManager::FilterSet set, const QString &resourceId)
{
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob(item, this);
    job->setProperty("filterSet", static_cast<int>(set));
    job->setProperty("resourceId", resourceId);
    SearchRule::RequiredPart requestedPart = requiredPart(resourceId);
    if (requestedPart == SearchRule::CompleteMessage) {
        job->fetchScope().fetchFullPayload(true);
    } else if (requestedPart == SearchRule::Header) {
        job->fetchScope().fetchPayloadPart(Akonadi::MessagePart::Header, true);
    } else {
        job->fetchScope().fetchPayloadPart(Akonadi::MessagePart::Envelope, true);
    }
    job->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);

    connect(job, SIGNAL(result(KJob*)), SLOT(itemFetchJobForFilterDone(KJob*)));
}

void FilterManager::filter(const Akonadi::Item &item, const QString &filterId, const QString &resourceId)
{
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob(item, this);
    job->setProperty("filterId", filterId);

    SearchRule::RequiredPart requestedPart = requiredPart(resourceId);
    if (requestedPart == SearchRule::CompleteMessage) {
        job->fetchScope().fetchFullPayload(true);
    } else if (requestedPart == SearchRule::Header) {
        job->fetchScope().fetchPayloadPart(Akonadi::MessagePart::Header, true);
    } else {
        job->fetchScope().fetchPayloadPart(Akonadi::MessagePart::Envelope, true);
    }

    job->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);

    connect(job, SIGNAL(result(KJob*)), SLOT(itemFetchJobForFilterDone(KJob*)));
}

bool FilterManager::process(const Akonadi::Item &item, bool needsFullPayload, const MailFilter *filter)
{
    if (!filter) {
        return false;
    }

    if (!filter->isEnabled()) {
        return true;
    }

    if (!item.hasPayload<KMime::Message::Ptr>()) {
        qCCritical(MAILFILTERAGENT_LOG) << "Filter is null or item doesn't have correct payload.";
        return false;
    }

    bool stopIt = false;
    bool applyOnOutbound = false;
    if (d->isMatching(item, filter)) {
        // do the actual filtering stuff
        d->beginFiltering(item);

        ItemContext context(item, needsFullPayload);

        if (filter->execActions(context, stopIt, applyOnOutbound) == MailCommon::MailFilter::CriticalError) {
            return false;
        }

        d->endFiltering(item);

        if (!processContextItem(context)) {
            return false;
        }

    }

    return true;
}

bool FilterManager::processContextItem(ItemContext context)
{
    const KMime::Message::Ptr msg = context.item().payload<KMime::Message::Ptr>();
    msg->assemble();

    const bool itemCanDelete = (MailCommon::Util::updatedCollection(context.item().parentCollection()).rights() & Akonadi::Collection::CanDeleteItem);
    if (context.deleteItem()) {
        if (itemCanDelete) {
            Akonadi::ItemDeleteJob *deleteJob = new Akonadi::ItemDeleteJob(context.item(), this);
            connect(deleteJob, SIGNAL(result(KJob*)), SLOT(deleteJobResult(KJob*)));
        } else {
            return false;
        }
    } else {
        if (context.moveTargetCollection().isValid() && context.item().storageCollectionId() != context.moveTargetCollection().id()) {
            if (itemCanDelete) {
                Akonadi::ItemMoveJob *moveJob = new Akonadi::ItemMoveJob(context.item(), context.moveTargetCollection(), this);
                connect(moveJob, SIGNAL(result(KJob*)), SLOT(moveJobResult(KJob*)));
            } else {
                return false;
            }
        }
        if (context.needsPayloadStore() || context.needsFlagStore()) {
            Akonadi::Item item = context.item();
            //the item might be in a new collection with a different remote id, so don't try to force on it
            //the previous remote id. Example: move to another collection on another resource => new remoteId, but our context.item()
            //remoteid still holds the old one. Without clearing it, we try to enforce that on the new location, which is
            //anything but good (and the server replies with "NO Only resources can modify remote identifiers"
            item.setRemoteId(QString());
            Akonadi::ItemModifyJob *modifyJob = new Akonadi::ItemModifyJob(item, this);
            modifyJob->disableRevisionCheck(); //no conflict handling for mails as no other process could change the mail body and we don't care about flag conflicts
            //The below is a safety check to ignore modifying payloads if it was not requested,
            //as in that case we might change the payload to an invalid one
            modifyJob->setIgnorePayload(!context.needsFullPayload());
            connect(modifyJob, SIGNAL(result(KJob*)), SLOT(modifyJobResult(KJob*)));
        }
    }

    return true;
}

bool FilterManager::process(const QList< MailFilter * > &mailFilters, const Akonadi::Item &item, bool needsFullPayload, FilterManager::FilterSet set, bool account, const QString &accountId)
{
    if (set == NoSet) {
        qCDebug(MAILFILTERAGENT_LOG) << "FilterManager: process() called with not filter set selected";
        return false;
    }

    if (!item.hasPayload<KMime::Message::Ptr>()) {
        qCCritical(MAILFILTERAGENT_LOG) << "Filter is null or item doesn't have correct payload.";
        return false;
    }

    bool stopIt = false;

    d->beginFiltering(item);

    ItemContext context(item, needsFullPayload);
    QList<MailCommon::MailFilter *>::const_iterator end(mailFilters.constEnd());

    const bool applyOnOutbound = ((set & Outbound) || (set & BeforeOutbound));

    for (QList<MailCommon::MailFilter *>::const_iterator it = mailFilters.constBegin();
            !stopIt && it != end; ++it) {
        if ((*it)->isEnabled()) {

            const bool inboundOk = ((set & Inbound) && (*it)->applyOnInbound());
            const bool outboundOk = ((set & Outbound) && (*it)->applyOnOutbound());
            const bool beforeOutboundOk = ((set & BeforeOutbound) && (*it)->applyBeforeOutbound());
            const bool explicitOk = ((set & Explicit) && (*it)->applyOnExplicit());
            const bool accountOk = (!account || (account && (*it)->applyOnAccount(accountId)));

            if ((inboundOk && accountOk) || outboundOk || beforeOutboundOk || explicitOk) {
                // filter is applicable

                if (d->isMatching(context.item(), *it)) {
                    // execute actions:
                    if ((*it)->execActions(context, stopIt, applyOnOutbound) == MailCommon::MailFilter::CriticalError) {
                        return false;
                    }
                }
            }
        }
    }

    d->endFiltering(item);
    if (!processContextItem(context)) {
        return false;
    }

    return true;
}

bool FilterManager::process(const Akonadi::Item &item,  bool needsFullPayload,
                            FilterSet set, bool account, const QString &accountId)
{
    return  process(d->mFilters, item, needsFullPayload, set, account, accountId);
}

QString FilterManager::createUniqueName(const QString &name) const
{
    QString uniqueName = name;

    int counter = 0;
    bool found = true;

    while (found) {
        found = false;
        foreach (const MailCommon::MailFilter *filter, d->mFilters) {
            if (!filter->name().compare(uniqueName)) {
                found = true;
                ++counter;
                uniqueName = name;
                uniqueName += QLatin1String(" (") + QString::number(counter)
                              + QLatin1String(")");
                break;
            }
        }
    }

    return uniqueName;
}

MailCommon::SearchRule::RequiredPart FilterManager::requiredPart(const QString &id) const
{
    if (id.isEmpty()) {
        return d->mRequiredPartsBasedOnAll;
    }
    return d->mRequiredParts.contains(id) ? d->mRequiredParts[id] : SearchRule::Envelope;
}

#ifndef NDEBUG
void FilterManager::dump() const
{
    foreach (const MailCommon::MailFilter *filter, d->mFilters) {
        qCDebug(MAILFILTERAGENT_LOG) << filter->asString();
    }
}
#endif

void FilterManager::applySpecificFilters(const Akonadi::Item::List &selectedMessages, SearchRule::RequiredPart requiredPart, const QStringList &listFilters)
{
    Q_EMIT progressMessage(i18n("Filtering messages"));
    d->mTotalProgressCount = selectedMessages.size();
    d->mCurrentProgressCount = 0;

    Akonadi::ItemFetchJob *itemFetchJob = new Akonadi::ItemFetchJob(selectedMessages, this);
    if (requiredPart == SearchRule::CompleteMessage) {
        itemFetchJob->fetchScope().fetchFullPayload(true);
    } else if (requiredPart == SearchRule::Header) {
        itemFetchJob->fetchScope().fetchPayloadPart(Akonadi::MessagePart::Header, true);
    } else {
        itemFetchJob->fetchScope().fetchPayloadPart(Akonadi::MessagePart::Envelope, true);
    }

    itemFetchJob->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
    itemFetchJob->setProperty("listFilters", QVariant::fromValue(listFilters));
    itemFetchJob->setProperty("needsFullPayload", requiredPart != SearchRule::Envelope);

    connect(itemFetchJob, SIGNAL(itemsReceived(Akonadi::Item::List)),
            this, SLOT(slotItemsFetchedForFilter(Akonadi::Item::List)));
    connect(itemFetchJob, SIGNAL(result(KJob*)),
            SLOT(itemsFetchJobForFilterDone(KJob*)));
}

void FilterManager::applyFilters(const Akonadi::Item::List &selectedMessages, FilterSet filterSet)
{
    Q_EMIT progressMessage(i18n("Filtering messages"));
    d->mTotalProgressCount = selectedMessages.size();
    d->mCurrentProgressCount = 0;

    Akonadi::ItemFetchJob *itemFetchJob = new Akonadi::ItemFetchJob(selectedMessages, this);
    SearchRule::RequiredPart requiredParts = requiredPart(QString());
    if (requiredParts == SearchRule::CompleteMessage) {
        itemFetchJob->fetchScope().fetchFullPayload(true);
    } else if (requiredParts == SearchRule::Header) {
        itemFetchJob->fetchScope().fetchPayloadPart(Akonadi::MessagePart::Header, true);
    } else {
        itemFetchJob->fetchScope().fetchPayloadPart(Akonadi::MessagePart::Envelope, true);
    }

    itemFetchJob->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
    itemFetchJob->setProperty("filterSet", QVariant::fromValue(static_cast<int>(filterSet)));
    itemFetchJob->setProperty("needsFullPayload", requiredParts != SearchRule::Envelope);

    connect(itemFetchJob, SIGNAL(itemsReceived(Akonadi::Item::List)),
            this, SLOT(slotItemsFetchedForFilter(Akonadi::Item::List)));
    connect(itemFetchJob, SIGNAL(result(KJob*)),
            SLOT(itemsFetchJobForFilterDone(KJob*)));
}

#include "moc_filtermanager.cpp"
