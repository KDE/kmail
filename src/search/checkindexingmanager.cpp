/*
   Copyright (C) 2016 Montel Laurent <montel@kde.org>

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
#include <MailCommon/MailUtil>
#include <PimCommon/PimUtil>
#include <QTimer>
#include <AkonadiCore/entityhiddenattribute.h>

CheckIndexingManager::CheckIndexingManager(QObject *parent)
    : QObject(parent),
      mIndex(0),
      mIsReady(true)
{
    mTimer = new QTimer(this);
    mTimer->setSingleShot(true);
    mTimer->setInterval(5 * 1000); //5 secondes
    connect(mTimer, &QTimer::timeout, this, &CheckIndexingManager::checkNextCollection);
}

CheckIndexingManager::~CheckIndexingManager()
{

}

void CheckIndexingManager::start(QAbstractItemModel *collectionModel)
{
    if(mIsReady) {
        mIndex = 0;
        mListCollection.clear();
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

void CheckIndexingManager::createJob()
{
    CheckIndexingJob *job = new CheckIndexingJob(this);
    job->setCollection(mListCollection.at(mIndex));
    connect(job, &CheckIndexingJob::finished, this, &CheckIndexingManager::slotRestartTimer);
    job->start();
}

void CheckIndexingManager::checkNextCollection()
{
    if (mIndex < mListCollection.count()) {
        createJob();
    }
}

void CheckIndexingManager::slotRestartTimer()
{
    mIndex++;
    if (mIndex < mListCollection.count()) {
        mTimer->start();
    } else {
        mIsReady = true;
        mIndex = 0;
        mListCollection.clear();
    }
}

bool CheckIndexingManager::isReady() const
{
    return mIsReady;
}

void CheckIndexingManager::initializeCollectionList(QAbstractItemModel *model, const QModelIndex &parentIndex)
{

    const int rowCount = model->rowCount(parentIndex);
    for (int row = 0; row < rowCount; ++row) {
        const QModelIndex index = model->index(row, 0, parentIndex);
        const Akonadi::Collection collection =
            model->data(
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
        mListCollection.append(collection);
        if (model->rowCount(index) > 0) {
            initializeCollectionList(model, index);
        }
    }
}
