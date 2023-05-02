/*
  SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#include "clearcachejobinfolderandsubfolderjob.h"
#include "kmail_debug.h"
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/RemoveDuplicatesJob>
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

        auto job = new Akonadi::RemoveDuplicatesJob(lst, this);
        job->setProperty("ProgressItem", QVariant::fromValue(item));
        item->setProperty("RemoveDuplicatesJob", QVariant::fromValue(qobject_cast<Akonadi::Job *>(job)));
        connect(job, &Akonadi::RemoveDuplicatesJob::finished, this, &ClearCacheJobInFolderAndSubFolderJob::slotFinished);
        connect(job, &Akonadi::RemoveDuplicatesJob::description, this, &ClearCacheJobInFolderAndSubFolderJob::slotRemoveDuplicatesUpdate);
        connect(item, &KPIM::ProgressItem::progressItemCanceled, this, &ClearCacheJobInFolderAndSubFolderJob::slotRemoveDuplicatesCanceled);
    }
}

void ClearCacheJobInFolderAndSubFolderJob::slotFinished(KJob *job)
{
    auto item = job->property("ProgressItem").value<KPIM::ProgressItem *>();
    if (item) {
        item->setComplete();
        item->setStatus(i18n("Done"));
        item = nullptr;
    }
    if (job->error()) {
        qCDebug(KMAIL_LOG()) << " Error during remove duplicates " << job->errorString();
        KMessageBox::error(mParentWidget,
                           i18n("Error occurred during removing duplicate emails: \'%1\'", job->errorText()),
                           i18n("Error while removing duplicates"));
    }

    deleteLater();
}

void ClearCacheJobInFolderAndSubFolderJob::slotRemoveDuplicatesUpdate(KJob *job, const QString &description)
{
    auto item = job->property("ProgressItem").value<KPIM::ProgressItem *>();
    if (item) {
        item->setStatus(description);
    }
}

void ClearCacheJobInFolderAndSubFolderJob::slotRemoveDuplicatesCanceled(KPIM::ProgressItem *item)
{
    auto job = item->property("RemoveDuplicatesJob").value<Akonadi::Job *>();
    if (job) {
        job->kill(KJob::Quietly);
    }

    item->setComplete();
    item = nullptr;
    deleteLater();
}
