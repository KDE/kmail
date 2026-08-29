/*
  SPDX-FileCopyrightText: 2014-2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#include "createtaskjob.h"
#include "attributes/taskattribute.h"
#include <Akonadi/MessageStatus>

#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>

#include <Akonadi/ItemModifyJob>

#include "kmail_debug.h"

CreateTaskJob::CreateTaskJob(const Akonadi::Item::List &items, QObject *parent)
    : KJob(parent)
    , mListItem(items)
{
}

CreateTaskJob::~CreateTaskJob() = default;

void CreateTaskJob::start()
{
    if (mListItem.isEmpty()) {
        emitResult();
        return;
    }
    fetchItems();
}

void CreateTaskJob::fetchItems()
{
    auto job = new Akonadi::ItemFetchJob(mListItem, this);
    job->fetchScope().fetchFullPayload(true);
    job->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
    job->fetchScope().fetchAttribute<TaskAttribute>();
    connect(job, &Akonadi::ItemFetchJob::result, this, &CreateTaskJob::itemFetchJobDone);
}

void CreateTaskJob::itemFetchJobDone(KJob *job)
{
    if (job->error()) {
        qCDebug(KMAIL_LOG) << job->errorString();
        emitResult();
        deleteLater();
        return;
    }
    auto fetchjob = qobject_cast<Akonadi::ItemFetchJob *>(job);
    const Akonadi::Item::List lst = fetchjob->items();
    if (lst.isEmpty()) {
        emitResult();
        deleteLater();
        return;
    }

    const Akonadi::MessageStatus toActStatus = Akonadi::MessageStatus::statusToAct();
    const Akonadi::Item::Flag toActFlag = *toActStatus.statusFlags().cbegin();

    // Toggle actions on threads toggle the whole thread
    // depending on the state of the parent.
    const Akonadi::Item first = lst.at(0);
    Akonadi::MessageStatus pStatus;
    pStatus.setStatusFromFlags(first.flags());
    const bool parentStatus = (pStatus & toActStatus);

    Akonadi::Item::List itemsToModify;
    itemsToModify.reserve(lst.size());
    for (const Akonadi::Item &it : lst) {
        // qCDebug(KMAIL_LOG)<<" item ::"<<tmpItem;
        if (it.isValid()) {
            Akonadi::MessageStatus itemStatus;
            itemStatus.setStatusFromFlags(it.flags());
            if (const bool myStatus = (itemStatus & toActStatus); myStatus != parentStatus) {
                continue;
            }
        }
        if (Akonadi::Item item(it); item.hasFlag(toActFlag)) {
            item.clearFlag(toActFlag);
            itemsToModify.push_back(item);
            if (item.hasAttribute<TaskAttribute>()) {
                // Change todo as done.
                item.removeAttribute<TaskAttribute>();
            }
        } else {
            item.setFlag(toActFlag);
            itemsToModify.push_back(std::move(item));
            // TODO add TaskAttribute();
        }
    }

    if (itemsToModify.isEmpty()) {
        slotModifyItemDone(nullptr);
    } else {
        auto modifyJob = new Akonadi::ItemModifyJob(itemsToModify, this);
        modifyJob->disableRevisionCheck();
        modifyJob->setIgnorePayload(true);
        connect(modifyJob, &Akonadi::ItemModifyJob::result, this, &CreateTaskJob::slotModifyItemDone);
    }
}

void CreateTaskJob::slotModifyItemDone(KJob *job)
{
    if (job && job->error()) {
        qCDebug(KMAIL_LOG) << " error " << job->errorString();
    }
    deleteLater();
}

#include "moc_createtaskjob.cpp"
