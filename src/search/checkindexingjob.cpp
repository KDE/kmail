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

#include "checkindexingjob.h"
#include "kmail_debug.h"

#include <AkonadiCore/CollectionFetchJob>
#include <AkonadiCore/CollectionFetchScope>
#include <AkonadiCore/CollectionStatistics>

#include <PimCommon/CollectionIndexStatusJob>

#include <QDBusInterface>
#include <QDebug>

CheckIndexingJob::CheckIndexingJob(QObject *parent)
    : QObject(parent)
{

}

CheckIndexingJob::~CheckIndexingJob()
{

}

void CheckIndexingJob::askForNextCheck()
{
    deleteLater();
    Q_EMIT finished();
}

void CheckIndexingJob::setCollection(const Akonadi::Collection &col)
{
    mCollection = col;
}

void CheckIndexingJob::start()
{
    if (mCollection.isValid()) {
        Akonadi::CollectionFetchJob *fetch = new Akonadi::CollectionFetchJob(mCollection,
                Akonadi::CollectionFetchJob::Base);
        fetch->fetchScope().setIncludeStatistics(true);
        connect(fetch, &KJob::result, this, &CheckIndexingJob::slotCollectionPropertiesFinished);
    } else {
        qCWarning(KMAIL_LOG) << "Collection was not valid";
        askForNextCheck();
    }
}

void CheckIndexingJob::slotCollectionPropertiesFinished(KJob *job)
{
    Akonadi::CollectionFetchJob *fetch = qobject_cast<Akonadi::CollectionFetchJob *>(job);
    Q_ASSERT(fetch);
    if (fetch->collections().isEmpty()) {
        qCWarning(KMAIL_LOG) << "No collection fetched";
        askForNextCheck();
        return;
    }

    mCollection = fetch->collections().first();
    PimCommon::CollectionIndexStatusJob *indexerJob = new PimCommon::CollectionIndexStatusJob(Akonadi::Collection::List() << mCollection, this);
    connect(indexerJob, &PimCommon::CollectionIndexStatusJob::finished, this, &CheckIndexingJob::indexerStatsFetchFinished);
    indexerJob->start();
}

void CheckIndexingJob::indexerStatsFetchFinished(KJob* job)
{
    if (job->error()) {
        qCWarning(KMAIL_LOG) << "CheckIndexingJob::indexerStatsFetchFinished error :" << job->errorString();
        askForNextCheck();
        return;
    }

    QMap<qint64, qint64> stats = qobject_cast<PimCommon::CollectionIndexStatusJob*>(job)->resultStats();
    qDebug()<<" stats "<< stats;
    if (mCollection.statistics().count() != stats.value(mCollection.id())) {
        QDBusInterface interfaceBalooIndexer(QStringLiteral("org.freedesktop.Akonadi.Agent.akonadi_indexing_agent"), QStringLiteral("/"), QStringLiteral("org.freedesktop.Akonadi.Indexer"));
        if (interfaceBalooIndexer.isValid()) {
            qCDebug(KMAIL_LOG) << "Reindex collection :"<< mCollection.id();
            interfaceBalooIndexer.call(QStringLiteral("reindexCollection"), (qlonglong)mCollection.id());
        }
    }
    askForNextCheck();
}
