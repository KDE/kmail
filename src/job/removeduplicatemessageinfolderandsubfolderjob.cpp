/*
  Copyright (c) 2015-2019 Montel Laurent <montel@kde.org>

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

#include "removeduplicatemessageinfolderandsubfolderjob.h"
#include <PimCommonAkonadi/FetchRecursiveCollectionsJob>
#include "kmail_debug.h"
#include <Akonadi/KMime/RemoveDuplicatesJob>
#include "libkdepim/progressmanager.h"
#include <KLocalizedString>
#include <KMessageBox>

RemoveDuplicateMessageInFolderAndSubFolderJob::RemoveDuplicateMessageInFolderAndSubFolderJob(QObject *parent, QWidget *parentWidget)
    : QObject(parent)
    , mParentWidget(parentWidget)
{
}

RemoveDuplicateMessageInFolderAndSubFolderJob::~RemoveDuplicateMessageInFolderAndSubFolderJob()
{
}

void RemoveDuplicateMessageInFolderAndSubFolderJob::start()
{
    if (mTopLevelCollection.isValid()) {
        PimCommon::FetchRecursiveCollectionsJob *fetchJob = new PimCommon::FetchRecursiveCollectionsJob(this);
        fetchJob->setTopCollection(mTopLevelCollection);
        connect(fetchJob, &PimCommon::FetchRecursiveCollectionsJob::fetchCollectionFailed, this, &RemoveDuplicateMessageInFolderAndSubFolderJob::slotFetchCollectionFailed);
        connect(fetchJob, &PimCommon::FetchRecursiveCollectionsJob::fetchCollectionFinished, this, &RemoveDuplicateMessageInFolderAndSubFolderJob::slotFetchCollectionDone);
        fetchJob->start();
    } else {
        qCDebug(KMAIL_LOG()) << "Invalid toplevel collection";
        deleteLater();
    }
}

void RemoveDuplicateMessageInFolderAndSubFolderJob::setTopLevelCollection(const Akonadi::Collection &topLevelCollection)
{
    mTopLevelCollection = topLevelCollection;
}

void RemoveDuplicateMessageInFolderAndSubFolderJob::slotFetchCollectionFailed()
{
    qCDebug(KMAIL_LOG()) << "Fetch toplevel collection failed";
    deleteLater();
}

void RemoveDuplicateMessageInFolderAndSubFolderJob::slotFetchCollectionDone(const Akonadi::Collection::List &list)
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
        KPIM::ProgressItem *item = KPIM::ProgressManager::createProgressItem(i18n("Removing duplicates"));
        item->setUsesBusyIndicator(true);
        item->setCryptoStatus(KPIM::ProgressItem::Unknown);

        Akonadi::RemoveDuplicatesJob *job = new Akonadi::RemoveDuplicatesJob(lst, this);
        job->setProperty("ProgressItem", QVariant::fromValue(item));
        item->setProperty("RemoveDuplicatesJob", QVariant::fromValue(qobject_cast<Akonadi::Job *>(job)));
        connect(job, &Akonadi::RemoveDuplicatesJob::finished, this, &RemoveDuplicateMessageInFolderAndSubFolderJob::slotFinished);
        connect(job, &Akonadi::RemoveDuplicatesJob::description, this, &RemoveDuplicateMessageInFolderAndSubFolderJob::slotRemoveDuplicatesUpdate);
        connect(item, &KPIM::ProgressItem::progressItemCanceled, this, &RemoveDuplicateMessageInFolderAndSubFolderJob::slotRemoveDuplicatesCanceled);
    }
}

void RemoveDuplicateMessageInFolderAndSubFolderJob::slotFinished(KJob *job)
{
    KPIM::ProgressItem *item = job->property("ProgressItem").value<KPIM::ProgressItem *>();
    if (item) {
        item->setComplete();
        item->setStatus(i18n("Done"));
        item = nullptr;
    }
    if (job->error()) {
        qCDebug(KMAIL_LOG()) << " Error during remove duplicates " << job->errorString();
        KMessageBox::error(mParentWidget, i18n("Error occurred during removing duplicate emails: \'%1\'", job->errorText()), i18n("Error while removing duplicates"));
    }

    deleteLater();
}

void RemoveDuplicateMessageInFolderAndSubFolderJob::slotRemoveDuplicatesUpdate(KJob *job, const QString &description)
{
    KPIM::ProgressItem *item = job->property("ProgressItem").value<KPIM::ProgressItem *>();
    if (item) {
        item->setStatus(description);
    }
}

void RemoveDuplicateMessageInFolderAndSubFolderJob::slotRemoveDuplicatesCanceled(KPIM::ProgressItem *item)
{
    Akonadi::Job *job = item->property("RemoveDuplicatesJob").value<Akonadi::Job *>();
    if (job) {
        job->kill(KJob::Quietly);
    }

    item->setComplete();
    item = nullptr;
    deleteLater();
}
