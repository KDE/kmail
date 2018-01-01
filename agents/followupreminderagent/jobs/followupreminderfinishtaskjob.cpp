/*
   Copyright (C) 2014-2018 Montel Laurent <montel@kde.org>

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

#include "followupreminderfinishtaskjob.h"
#include "FollowupReminder/FollowUpReminderInfo"
#include <AkonadiCore/Item>
#include <AkonadiCore/ItemFetchJob>
#include <AkonadiCore/ItemModifyJob>
#include "followupreminderagent_debug.h"
#include <KCalCore/Todo>

FollowUpReminderFinishTaskJob::FollowUpReminderFinishTaskJob(Akonadi::Item::Id id, QObject *parent)
    : QObject(parent)
    , mTodoId(id)
{
}

FollowUpReminderFinishTaskJob::~FollowUpReminderFinishTaskJob()
{
}

void FollowUpReminderFinishTaskJob::start()
{
    if (mTodoId != -1) {
        closeTodo();
    } else {
        Q_EMIT finishTaskFailed();
        deleteLater();
    }
}

void FollowUpReminderFinishTaskJob::closeTodo()
{
    Akonadi::Item item(mTodoId);
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob(item, this);
    connect(job, &Akonadi::ItemFetchJob::result, this, &FollowUpReminderFinishTaskJob::slotItemFetchJobDone);
}

void FollowUpReminderFinishTaskJob::slotItemFetchJobDone(KJob *job)
{
    if (job->error()) {
        qCWarning(FOLLOWUPREMINDERAGENT_LOG) << job->errorString();
        Q_EMIT finishTaskFailed();
        deleteLater();
        return;
    }

    const Akonadi::Item::List lst = qobject_cast<Akonadi::ItemFetchJob *>(job)->items();
    if (lst.count() == 1) {
        const Akonadi::Item item = lst.first();
        if (!item.hasPayload<KCalCore::Todo::Ptr>()) {
            qCDebug(FOLLOWUPREMINDERAGENT_LOG) << " item is not a todo.";
            Q_EMIT finishTaskFailed();
            deleteLater();
            return;
        }
        KCalCore::Todo::Ptr todo = item.payload<KCalCore::Todo::Ptr>();
        todo->setCompleted(true);
        Akonadi::Item updateItem = item;
        updateItem.setPayload<KCalCore::Todo::Ptr>(todo);

        Akonadi::ItemModifyJob *job = new Akonadi::ItemModifyJob(updateItem);
        connect(job, &Akonadi::ItemModifyJob::result, this, &FollowUpReminderFinishTaskJob::slotItemModifiedResult);
    } else {
        qCWarning(FOLLOWUPREMINDERAGENT_LOG) << " Found item different from 1: " << lst.count();
        Q_EMIT finishTaskFailed();
        deleteLater();
        return;
    }
}

void FollowUpReminderFinishTaskJob::slotItemModifiedResult(KJob *job)
{
    if (job->error()) {
        qCWarning(FOLLOWUPREMINDERAGENT_LOG) << job->errorString();
        Q_EMIT finishTaskFailed();
    } else {
        Q_EMIT finishTaskDone();
    }
    deleteLater();
}
