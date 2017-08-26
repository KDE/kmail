/*
   Copyright (C) 2016-2017 Montel Laurent <montel@kde.org>

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

#include "checkindexingmanager.h"
#include "kmail_debug.h"
#include "checkindexingjob.h"
#include <AkonadiCore/EntityTreeModel>
#include <AkonadiCore/CachePolicy>
#include <KSharedConfig>
#include <KConfigGroup>
#include <AkonadiCore/ServerManager>
#include <MailCommon/MailUtil>
#include <PimCommon/PimUtil>
#include <PimCommonAkonadi/MailUtil>
#include <AkonadiSearch/PIM/indexeditems.h>
#include <QTimer>
#include <QDBusInterface>
#include <QDBusPendingCall>
#include <AkonadiCore/entityhiddenattribute.h>

CheckIndexingManager::CheckIndexingManager(Akonadi::Search::PIM::IndexedItems *indexer, QObject *parent)
    : QObject(parent)
    , mIndexedItems(indexer)
{
    mTimer = new QTimer(this);
    mTimer->setSingleShot(true);
    mTimer->setInterval(5 * 1000); //5 secondes
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
        //Check each 7 days
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
    CheckIndexingJob *job = new CheckIndexingJob(mIndexedItems, this);
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
        QString service = QStringLiteral("org.freedesktop.Akonadi.Indexer");
        if (Akonadi::ServerManager::hasInstanceIdentifier()) {
            service += QLatin1Char('.') + Akonadi::ServerManager::instanceIdentifier();
        }
        QDBusInterface interfaceBalooIndexer(PimCommon::MailUtil::indexerServiceName(), QStringLiteral("/"), service);
        if (interfaceBalooIndexer.isValid()) {
            qCDebug(KMAIL_LOG) << "Reindex collections :" << mCollectionsIndexed;
            interfaceBalooIndexer.asyncCall(QStringLiteral("reindexCollections"), QVariant::fromValue(mCollectionsNeedToBeReIndexed));
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
        const Akonadi::Collection collection
            = model->data(
            index, Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();

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
