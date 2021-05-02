/*
   SPDX-FileCopyrightText: 2016-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "checkindexingmanager.h"
#include "checkindexingjob.h"
#include "kmail_debug.h"
#include <AkonadiCore/CachePolicy>
#include <AkonadiCore/EntityTreeModel>
#include <AkonadiCore/ServerManager>
#include <AkonadiCore/entityhiddenattribute.h>
#include <AkonadiSearch/PIM/indexeditems.h>
#include <KConfigGroup>
#include <KSharedConfig>
#include <MailCommon/MailUtil>
#include <PimCommon/PimUtil>
#include <PimCommonAkonadi/MailUtil>
#include <QDBusInterface>
#include <QTimer>

CheckIndexingManager::CheckIndexingManager(Akonadi::Search::PIM::IndexedItems *indexer, QObject *parent)
    : QObject(parent)
    , mIndexedItems(indexer)
    , mTimer(new QTimer(this))
{
    mTimer->setSingleShot(true);
    mTimer->setInterval(5 * 1000); // 5 secondes
    connect(mTimer, &QTimer::timeout, this, &CheckIndexingManager::checkNextCollection);
}

CheckIndexingManager::~CheckIndexingManager()
{
    callToReindexCollection();
    const KSharedConfig::Ptr cfg = KSharedConfig::openConfig(QStringLiteral("kmailsearchindexingrc"));
    KConfigGroup grp = cfg->group(QStringLiteral("General"));
    grp.writeEntry(QStringLiteral("collectionsIndexed"), mCollectionsIndexed);
}

void CheckIndexingManager::start(QAbstractItemModel *collectionModel)
{
    if (mIsReady) {
        const KSharedConfig::Ptr cfg = KSharedConfig::openConfig(QStringLiteral("kmailsearchindexingrc"));
        KConfigGroup grp = cfg->group(QStringLiteral("General"));
        const QDateTime lastDateTime = grp.readEntry(QStringLiteral("lastCheck"), QDateTime());
        // Check each 7 days
        QDateTime today = QDateTime::currentDateTime();
        if (!lastDateTime.isValid() || today > lastDateTime.addDays(7)) {
            mIndex = 0;
            mListCollection.clear();
            mCollectionsIndexed = grp.readEntry(QStringLiteral("collectionsIndexed"), QList<qint64>());
            if (collectionModel) {
                initializeCollectionList(collectionModel);
                if (!mListCollection.isEmpty()) {
                    qCDebug(KMAIL_LOG) << "Number of collection to check " << mListCollection.count();
                    mIsReady = false;
                    mTimer->start();
                }
            }
        }
    }
}

void CheckIndexingManager::createJob()
{
    auto job = new CheckIndexingJob(mIndexedItems, this);
    job->setCollection(mListCollection.at(mIndex));
    connect(job, &CheckIndexingJob::finished, this, &CheckIndexingManager::indexingFinished);
    job->start();
}

void CheckIndexingManager::checkNextCollection()
{
    if (mIndex < mListCollection.count()) {
        createJob();
    }
}

void CheckIndexingManager::callToReindexCollection()
{
    if (!mCollectionsNeedToBeReIndexed.isEmpty()) {
        QDBusInterface interfaceAkonadiIndexer(PimCommon::MailUtil::indexerServiceName(),
                                               QStringLiteral("/"),
                                               QStringLiteral("org.freedesktop.Akonadi.Indexer"));
        if (interfaceAkonadiIndexer.isValid()) {
            qCDebug(KMAIL_LOG) << "Reindex collections :" << mCollectionsIndexed;
            interfaceAkonadiIndexer.asyncCall(QStringLiteral("reindexCollections"), QVariant::fromValue(mCollectionsNeedToBeReIndexed));
        }
    }
}

void CheckIndexingManager::indexingFinished(qint64 index, bool reindexCollection)
{
    if (index != -1) {
        if (!mCollectionsIndexed.contains(index)) {
            mCollectionsIndexed.append(index);
        }
    }
    if (reindexCollection) {
        if (!mCollectionsNeedToBeReIndexed.contains(index)) {
            mCollectionsNeedToBeReIndexed.append(index);
        }
        if (mCollectionsNeedToBeReIndexed.count() > 30) {
            callToReindexCollection();
            mCollectionsNeedToBeReIndexed.clear();
        }
    }
    mIndex++;
    if (mIndex < mListCollection.count()) {
        mTimer->start();
    } else {
        mIsReady = true;
        mIndex = 0;
        callToReindexCollection();
        mListCollection.clear();
        mCollectionsNeedToBeReIndexed.clear();

        const KSharedConfig::Ptr cfg = KSharedConfig::openConfig(QStringLiteral("kmailsearchindexingrc"));
        KConfigGroup grp = cfg->group(QStringLiteral("General"));
        grp.writeEntry(QStringLiteral("lastCheck"), QDateTime::currentDateTime());
        grp.deleteEntry(QStringLiteral("collectionsIndexed"));
        grp.sync();
    }
}

void CheckIndexingManager::initializeCollectionList(QAbstractItemModel *model, const QModelIndex &parentIndex)
{
    const int rowCount = model->rowCount(parentIndex);
    for (int row = 0; row < rowCount; ++row) {
        const QModelIndex index = model->index(row, 0, parentIndex);
        const auto collection = model->data(index, Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();

        if (!collection.isValid() || MailCommon::Util::isVirtualCollection(collection)) {
            continue;
        }
        if (collection.hasAttribute<Akonadi::EntityHiddenAttribute>()) {
            continue;
        }
        if (PimCommon::Util::isImapResource(collection.resource()) && !collection.cachePolicy().localParts().contains(QLatin1String("RFC822"))) {
            continue;
        }
        if (!mCollectionsIndexed.contains(collection.id())) {
            mListCollection.append(collection);
        }
        if (model->rowCount(index) > 0) {
            initializeCollectionList(model, index);
        }
    }
}
