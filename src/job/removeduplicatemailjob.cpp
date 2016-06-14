/*
   Copyright (C) 2014-2016 Montel Laurent <montel@kde.org>

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

#include "removeduplicatemailjob.h"

#include "libkdepim/progressmanager.h"
#include <KLocalizedString>
#include <KMessageBox>

#include <AkonadiCore/Collection>
#include <akonadi/kmime/removeduplicatesjob.h>
#include <AkonadiCore/EntityTreeModel>

#include <QItemSelectionModel>
Q_DECLARE_METATYPE(KPIM::ProgressItem *)
Q_DECLARE_METATYPE(Akonadi::Job *)

RemoveDuplicateMailJob::RemoveDuplicateMailJob(QItemSelectionModel *selectionModel, QWidget *widget, QObject *parent)
    : QObject(parent),
      mParent(widget),
      mSelectionModel(selectionModel)
{

}

RemoveDuplicateMailJob::~RemoveDuplicateMailJob()
{

}

void RemoveDuplicateMailJob::start()
{
    KPIM::ProgressItem *item = KPIM::ProgressManager::createProgressItem(i18n("Removing duplicates"));
    item->setUsesBusyIndicator(true);
    item->setCryptoStatus(KPIM::ProgressItem::Unknown);

    QModelIndexList indexes = mSelectionModel->selectedIndexes();
    Akonadi::Collection::List collections;

    Q_FOREACH (const QModelIndex &index, indexes) {
        const Akonadi::Collection collection = index.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
        if (collection.isValid()) {
            collections << collection;
        }
    }

    Akonadi::RemoveDuplicatesJob *job = new Akonadi::RemoveDuplicatesJob(collections, this);
    job->setProperty("ProgressItem", QVariant::fromValue(item));
    item->setProperty("RemoveDuplicatesJob", QVariant::fromValue(qobject_cast<Akonadi::Job *>(job)));
    connect(job, &KJob::finished, this, &RemoveDuplicateMailJob::slotRemoveDuplicatesDone);
    connect(job, &KJob::description, this, &RemoveDuplicateMailJob::slotRemoveDuplicatesUpdate);
    connect(item, &KPIM::ProgressItem::progressItemCanceled, this, &RemoveDuplicateMailJob::slotRemoveDuplicatesCanceled);
}

void RemoveDuplicateMailJob::slotRemoveDuplicatesDone(KJob *job)
{
    KPIM::ProgressItem *item = job->property("ProgressItem").value<KPIM::ProgressItem *>();
    if (item) {
        item->setComplete();
        item->setStatus(i18n("Done"));
        item = Q_NULLPTR;
    }
    if (job && (job->error() != KJob::KilledJobError)) {
        KMessageBox::error(mParent, job->errorText(), i18n("Error while removing duplicates"));
    }
    deleteLater();
}

void RemoveDuplicateMailJob::slotRemoveDuplicatesCanceled(KPIM::ProgressItem *item)
{
    Akonadi::Job *job = item->property("RemoveDuplicatesJob").value<Akonadi::Job *>();
    if (job) {
        job->kill(KJob::Quietly);
    }

    item->setComplete();
    item = Q_NULLPTR;
    deleteLater();
}

void RemoveDuplicateMailJob::slotRemoveDuplicatesUpdate(KJob *job, const QString &description)
{
    KPIM::ProgressItem *item = job->property("ProgressItem").value<KPIM::ProgressItem *>();
    if (item) {
        item->setStatus(description);
    }
}
