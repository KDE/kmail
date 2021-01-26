/*
   SPDX-FileCopyrightText: 2016-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "checkindexingjob.h"
#include "kmail_debug.h"
#include <AkonadiSearch/PIM/indexeditems.h>

#include <AkonadiCore/CollectionFetchJob>
#include <AkonadiCore/CollectionFetchScope>
#include <AkonadiCore/CollectionStatistics>

#include <PimCommon/PimUtil>

CheckIndexingJob::CheckIndexingJob(Akonadi::Search::PIM::IndexedItems *indexedItems, QObject *parent)
    : QObject(parent)
    , mIndexedItems(indexedItems)
{
}

CheckIndexingJob::~CheckIndexingJob() = default;

void CheckIndexingJob::askForNextCheck(quint64 id, bool needToReindex)
{
    Q_EMIT finished(id, needToReindex);
    deleteLater();
}

void CheckIndexingJob::setCollection(const Akonadi::Collection &col)
{
    mCollection = col;
}

void CheckIndexingJob::start()
{
    if (mCollection.isValid()) {
        auto fetch = new Akonadi::CollectionFetchJob(mCollection, Akonadi::CollectionFetchJob::Base);
        fetch->fetchScope().setIncludeStatistics(true);
        connect(fetch, &KJob::result, this, &CheckIndexingJob::slotCollectionPropertiesFinished);
    } else {
        qCWarning(KMAIL_LOG) << "Collection was not valid";
        askForNextCheck(-1);
    }
}

void CheckIndexingJob::slotCollectionPropertiesFinished(KJob *job)
{
    auto fetch = qobject_cast<Akonadi::CollectionFetchJob *>(job);
    Q_ASSERT(fetch);
    if (fetch->collections().isEmpty()) {
        qCWarning(KMAIL_LOG) << "No collection fetched";
        askForNextCheck(-1);
        return;
    }

    mCollection = fetch->collections().constFirst();
    const qlonglong result = mIndexedItems->indexedItems(mCollection.id());
    bool needToReindex = false;
    qCDebug(KMAIL_LOG) << "name :" << mCollection.name() << " mCollection.statistics().count() " << mCollection.statistics().count()
                       << "stats.value(mCollection.id())" << result;
    if (mCollection.statistics().count() != result) {
        needToReindex = true;
        qCDebug(KMAIL_LOG) << "Reindex collection :"
                           << "name :" << mCollection.name();
    }
    askForNextCheck(mCollection.id(), needToReindex);
}
