/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "removeduplicatemailjob.h"

#include <AkonadiCore/Collection>
#include <AkonadiCore/EntityTreeModel>
#include <KLocalizedString>
#include <KMessageBox>
#include <Libkdepim/ProgressManager>
#include <akonadi/kmime/removeduplicatesjob.h>

#include <QItemSelectionModel>
Q_DECLARE_METATYPE(KPIM::ProgressItem *)
Q_DECLARE_METATYPE(Akonadi::Job *)

RemoveDuplicateMailJob::RemoveDuplicateMailJob(QItemSelectionModel *selectionModel, QWidget *widget, QObject *parent)
    : QObject(parent)
    , mParent(widget)
    , mSelectionModel(selectionModel)
{
}

RemoveDuplicateMailJob::~RemoveDuplicateMailJob() = default;

void RemoveDuplicateMailJob::start()
{
    KPIM::ProgressItem *item = KPIM::ProgressManager::createProgressItem(i18n("Removing duplicates"));
    item->setUsesBusyIndicator(true);
    item->setCryptoStatus(KPIM::ProgressItem::Unknown);

    const QModelIndexList indexes = mSelectionModel->selectedIndexes();
    Akonadi::Collection::List collections;

    for (const QModelIndex &index : indexes) {
        const auto collection = index.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
        if (collection.isValid()) {
            collections << collection;
        }
    }

    auto job = new Akonadi::RemoveDuplicatesJob(collections, this);
    job->setProperty("ProgressItem", QVariant::fromValue(item));
    item->setProperty("RemoveDuplicatesJob", QVariant::fromValue(job));
    connect(job, &KJob::finished, this, &RemoveDuplicateMailJob::slotRemoveDuplicatesDone);
    connect(job, &KJob::description, this, &RemoveDuplicateMailJob::slotRemoveDuplicatesUpdate);
    connect(item, &KPIM::ProgressItem::progressItemCanceled, this, &RemoveDuplicateMailJob::slotRemoveDuplicatesCanceled);
}

void RemoveDuplicateMailJob::slotRemoveDuplicatesDone(KJob *job)
{
    auto *item = job->property("ProgressItem").value<KPIM::ProgressItem *>();
    if (item) {
        item->setComplete();
        item->setStatus(i18n("Done"));
        item = nullptr;
    }
    if (job->error()) {
        KMessageBox::error(mParent, i18n("Error occurred during removing duplicate emails: \'%1\'", job->errorText()), i18n("Error while removing duplicates"));
    }
    deleteLater();
}

void RemoveDuplicateMailJob::slotRemoveDuplicatesCanceled(KPIM::ProgressItem *item)
{
    auto *job = item->property("RemoveDuplicatesJob").value<Akonadi::Job *>();
    if (job) {
        job->kill(KJob::Quietly);
    }

    item->setComplete();
    item = nullptr;
    deleteLater();
}

void RemoveDuplicateMailJob::slotRemoveDuplicatesUpdate(KJob *job, const QString &description)
{
    auto *item = job->property("ProgressItem").value<KPIM::ProgressItem *>();
    if (item) {
        item->setStatus(description);
    }
}
