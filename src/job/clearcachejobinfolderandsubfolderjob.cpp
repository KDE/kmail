/*
  SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#include "clearcachejobinfolderandsubfolderjob.h"
#include "kmail_debug.h"
#include <Akonadi/ClearCacheFoldersJob>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <KLocalizedString>
#include <KMessageBox>
#include <Libkdepim/ProgressManager>

ClearCacheJobInFolderAndSubFolderJob::ClearCacheJobInFolderAndSubFolderJob(QObject *parent, QWidget *parentWidget)
    : QObject(parent)
    , mParentWidget(parentWidget)
{
}

ClearCacheJobInFolderAndSubFolderJob::~ClearCacheJobInFolderAndSubFolderJob() = default;

void ClearCacheJobInFolderAndSubFolderJob::start()
{
    if (mTopLevelCollection.isValid()) {
        auto fetchJob = new Akonadi::CollectionFetchJob(mTopLevelCollection, Akonadi::CollectionFetchJob::Recursive, this);
        fetchJob->fetchScope().setAncestorRetrieval(Akonadi::CollectionFetchScope::All);
        connect(fetchJob, &Akonadi::CollectionFetchJob::result, this, [this](KJob *job) {
            if (job->error()) {
                qCWarning(KMAIL_LOG) << job->errorString();
                slotFetchCollectionFailed();
            } else {
                auto fetch = static_cast<Akonadi::CollectionFetchJob *>(job);
                slotFetchCollectionDone(fetch->collections());
            }
        });
    } else {
        qCDebug(KMAIL_LOG()) << "Invalid toplevel collection";
        deleteLater();
    }
}

void ClearCacheJobInFolderAndSubFolderJob::setTopLevelCollection(const Akonadi::Collection &topLevelCollection)
{
    mTopLevelCollection = topLevelCollection;
}

void ClearCacheJobInFolderAndSubFolderJob::slotFetchCollectionFailed()
{
    qCDebug(KMAIL_LOG()) << "Fetch toplevel collection failed";
    deleteLater();
}

void ClearCacheJobInFolderAndSubFolderJob::slotFetchCollectionDone(const Akonadi::Collection::List &list)
{
    Akonadi::Collection::List lst;
    for (const Akonadi::Collection &collection : list) {
        if (collection.isValid()) {
            if (collection.rights() & Akonadi::Collection::CanDeleteItem) {
                lst.append(collection);
            }
        }
    }
    if (lst.isEmpty()) {
        deleteLater();
    } else {
        KPIM::ProgressItem *item = KPIM::ProgressManager::createProgressItem(i18n("Clear Akonadi Cache"));
        item->setUsesBusyIndicator(true);
        item->setCryptoStatus(KPIM::ProgressItem::Unknown);

        auto job = new Akonadi::ClearCacheFoldersJob(lst, this);
        job->setProperty("ProgressItem", QVariant::fromValue(item));
        item->setProperty("ClearCacheFoldersJob", QVariant::fromValue(qobject_cast<Akonadi::Job *>(job)));
        connect(job, &Akonadi::ClearCacheFoldersJob::clearCacheDone, this, &ClearCacheJobInFolderAndSubFolderJob::clearCacheDone);
        connect(job, &Akonadi::ClearCacheFoldersJob::finished, this, [this, job](bool success) {
            // TODO use it
            Q_UNUSED(success)

            slotFinished(job);
        });
        // connect(job, &Akonadi::ClearCacheFoldersJob::description, this, &ClearCacheJobInFolderAndSubFolderJob::slotClearAkonadiCacheUpdate);
        connect(item, &KPIM::ProgressItem::progressItemCanceled, this, &ClearCacheJobInFolderAndSubFolderJob::slotClearAkonadiCacheCanceled);
        job->start();
    }
}

void ClearCacheJobInFolderAndSubFolderJob::slotFinished(Akonadi::ClearCacheFoldersJob *job)
{
    auto item = job->property("ProgressItem").value<KPIM::ProgressItem *>();
    if (item) {
        item->setComplete();
        item->setStatus(i18n("Done"));
        item = nullptr;
    }
    deleteLater();
}

void ClearCacheJobInFolderAndSubFolderJob::slotClearAkonadiCacheUpdate(Akonadi::ClearCacheFoldersJob *job, const QString &description)
{
    auto item = job->property("ProgressItem").value<KPIM::ProgressItem *>();
    if (item) {
        item->setStatus(description);
    }
}

void ClearCacheJobInFolderAndSubFolderJob::slotClearAkonadiCacheCanceled(KPIM::ProgressItem *item)
{
    auto job = item->property("ClearCacheFoldersJob").value<Akonadi::ClearCacheFoldersJob *>();
    if (job) {
        job->setCanceled(true);
    }

    item->setComplete();
    item = nullptr;
    deleteLater();
}

#include "moc_clearcachejobinfolderandsubfolderjob.cpp"
