/*
  Copyright (c) 2014 Montel Laurent <montel@kde.org>

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

#include "followupremindercreatejob.h"
#include "agents/followupreminderagent/followupreminderutil.h"
#include <KCalCore/Todo>
#include <KLocalizedString>
#include <AkonadiCore/ItemCreateJob>

FollowupReminderCreateJob::FollowupReminderCreateJob(QObject *parent)
    : KJob(parent),
      mInfo(new FollowUpReminder::FollowUpReminderInfo)
{

}

FollowupReminderCreateJob::~FollowupReminderCreateJob()
{
    delete mInfo;
}

void FollowupReminderCreateJob::setFollowUpReminderDate(const QDate &date)
{
    mInfo->setFollowUpReminderDate(date);
}

void FollowupReminderCreateJob::setOriginalMessageItemId(Akonadi::Entity::Id value)
{
    mInfo->setOriginalMessageItemId(value);
}

void FollowupReminderCreateJob::setMessageId(const QString &messageId)
{
    mInfo->setMessageId(messageId);
}

void FollowupReminderCreateJob::setTo(const QString &to)
{
    mInfo->setTo(to);
}

void FollowupReminderCreateJob::setSubject(const QString &subject)
{
    mInfo->setSubject(subject);
}

void FollowupReminderCreateJob::setCollectionToDo(const Akonadi::Collection &collection)
{
    mCollection = collection;
}

void FollowupReminderCreateJob::start()
{
    if (mInfo->isValid()) {
        if (mCollection.isValid()) {
            KCalCore::Todo::Ptr todo(new KCalCore::Todo);
            todo->setSummary(i18n("Wait answer from \"%1\" send to \"%2\"").arg(mInfo->subject()).arg(mInfo->to()));
            Akonadi::Item newTodoItem;
            newTodoItem.setMimeType(KCalCore::Todo::todoMimeType());
            newTodoItem.setPayload<KCalCore::Todo::Ptr>(todo);

            Akonadi::ItemCreateJob *createJob = new Akonadi::ItemCreateJob(newTodoItem, mCollection);
            connect(createJob, &Akonadi::ItemCreateJob::result, this, &FollowupReminderCreateJob::slotCreateNewTodo);
        } else {
            writeFollowupReminderInfo();
        }
    } else {
        qDebug()<<"FollowupReminderCreateJob info not valid ";
        Q_EMIT emitResult();
        return;
    }
}

void FollowupReminderCreateJob::slotCreateNewTodo(KJob *job)
{
    if (job->error()) {
        qDebug() << "Error during create new Todo " << job->errorString();
    } else {
        Akonadi::ItemCreateJob *createJob = qobject_cast<Akonadi::ItemCreateJob *>(job);
        mInfo->setTodoId(createJob->item().id());
    }
    writeFollowupReminderInfo();
}

void FollowupReminderCreateJob::writeFollowupReminderInfo()
{
    FollowUpReminder::FollowUpReminderUtil::writeFollowupReminderInfo(FollowUpReminder::FollowUpReminderUtil::defaultConfig(), mInfo, true);
    Q_EMIT emitResult();
}
