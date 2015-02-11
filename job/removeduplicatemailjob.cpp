/*
  Copyright (c) 2014-2015 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "removeduplicatemailjob.h"

#include "progresswidget/progressmanager.h"
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
    connect(job, SIGNAL(finished(KJob*)), this, SLOT(slotRemoveDuplicatesDone(KJob*)));
    connect(job, SIGNAL(description(KJob*,QString,QPair<QString,QString>,QPair<QString,QString>)), this, SLOT(slotRemoveDuplicatesUpdate(KJob*,QString)));
    connect(item, SIGNAL(progressItemCanceled(KPIM::ProgressItem*)), this, SLOT(slotRemoveDuplicatesCanceled(KPIM::ProgressItem*)));
}

void RemoveDuplicateMailJob::slotRemoveDuplicatesDone(KJob *job)
{
    KPIM::ProgressItem *item = job->property("ProgressItem").value<KPIM::ProgressItem *>();
    if (item) {
        item->setComplete();
        item->setStatus(i18n("Done"));
        item = Q_NULLPTR;
    }
    if (job->error() && job->error() != KJob::KilledJobError) {
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
