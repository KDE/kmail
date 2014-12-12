/*
  Copyright (c) 2014 Montel Laurent <montel@kde.org>

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

#include "createtaskjob.h"
#include "attributes/taskattribute.h"
#include <Akonadi/KMime/MessageStatus>

#include <AkonadiCore/ItemFetchJob>
#include <AkonadiCore/ItemFetchScope>

#include <AkonadiCore/ItemModifyJob>

#include "kmail_debug.h"

CreateTaskJob::CreateTaskJob(const Akonadi::Item::List &items, QObject *parent)
    : KJob(parent),
      mListItem(items)
{
}

CreateTaskJob::~CreateTaskJob()
{

}

void CreateTaskJob::start()
{
    if (mListItem.isEmpty()) {
        Q_EMIT emitResult();
        return;
    }
    fetchItems();
}

void CreateTaskJob::fetchItems()
{
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob(mListItem, this);
    job->fetchScope().fetchFullPayload(true);
    job->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
    job->fetchScope().fetchAttribute<TaskAttribute>();
    connect(job, &Akonadi::ItemFetchJob::result, this, &CreateTaskJob::itemFetchJobDone);
}

void CreateTaskJob::itemFetchJobDone(KJob *job)
{
    if (job->error()) {
        qCDebug(KMAIL_LOG) << job->errorString();
        Q_EMIT emitResult();
        return;
    }
    Akonadi::ItemFetchJob *fetchjob = qobject_cast<Akonadi::ItemFetchJob *>(job);
    const Akonadi::Item::List lst = fetchjob->items();
    if (lst.isEmpty()) {
        Q_EMIT emitResult();
        return;
    }

    bool parentStatus = false;
    // Toggle actions on threads toggle the whole thread
    // depending on the state of the parent.
    const Akonadi::Item first = lst.first();
    Akonadi::MessageStatus pStatus;
    pStatus.setStatusFromFlags(first.flags());
    if (pStatus & Akonadi::MessageStatus::statusToAct()) {
        parentStatus = true;
    } else {
        parentStatus = false;
    }

    Akonadi::Item::List itemsToModify;
    foreach (const Akonadi::Item &it, lst) {
        //qCDebug(KMAIL_LOG)<<" item ::"<<tmpItem;
        if (it.isValid()) {
            bool myStatus;
            Akonadi::MessageStatus itemStatus;
            itemStatus.setStatusFromFlags(it.flags());
            if (itemStatus & Akonadi::MessageStatus::statusToAct()) {
                myStatus = true;
            } else {
                myStatus = false;
            }
            if (myStatus != parentStatus) {
                continue;
            }
        }
        Akonadi::Item item(it);
        const Akonadi::Item::Flag flag = *(Akonadi::MessageStatus::statusToAct().statusFlags().begin());
        if (item.hasFlag(flag)) {
            item.clearFlag(flag);
            itemsToModify.push_back(item);
            if (item.hasAttribute<TaskAttribute>()) {
                //Change todo as done.
                item.removeAttribute<TaskAttribute>();
            }
        } else {
            item.setFlag(flag);
            itemsToModify.push_back(item);
            //TODO add TaskAttribute();
        }
    }

    if (itemsToModify.isEmpty()) {
        slotModifyItemDone(0);
    } else {
        Akonadi::ItemModifyJob *modifyJob = new Akonadi::ItemModifyJob(itemsToModify, this);
        modifyJob->disableRevisionCheck();
        modifyJob->setIgnorePayload(true);
        connect(modifyJob, &Akonadi::ItemModifyJob::result, this, &CreateTaskJob::slotModifyItemDone);
    }
}

void CreateTaskJob::slotModifyItemDone(KJob *job)
{
    //TODO
    if (job && job->error()) {
        qCDebug(KMAIL_LOG) << " error " << job->errorString();
    }
    deleteLater();
}
