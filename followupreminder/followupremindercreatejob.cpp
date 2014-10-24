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
#include <akonadi/itemcreatejob.h>

FollowupReminderCreateJob::FollowupReminderCreateJob(QObject *parent)
    : QObject(parent),
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

void FollowupReminderCreateJob::start()
{
    if (mInfo->isValid()) {
        KCalCore::Todo::Ptr todo( new KCalCore::Todo );
        todo->setSummary(i18n("Wait answer from \"%1\" send to \"%2\"").arg(mInfo->subject()).arg(mInfo->to()));
        Akonadi::Item newTodoItem;
        newTodoItem.setMimeType( KCalCore::Todo::todoMimeType() );
        newTodoItem.setPayload<KCalCore::Todo::Ptr>( todo );

        //Port collection Akonadi::ItemCreateJob *createJob = new Akonadi::ItemCreateJob(newTodoItem, mCollection);
        //Port collection connect(createJob, SIGNAL(result(KJob*)), this, SLOT(slotCreateNewTodo(KJob*)));

    } else {
        qDebug()<<"FollowupReminderCreateJob info not valid ";
        deleteLater();
        return;
    }
}

void FollowupReminderCreateJob::slotCreateNewTodo(KJob *job)
{
    if ( job->error() ) {
        qDebug() << "Error during create new Todo "<<job->errorString();
    } else {
        Akonadi::ItemCreateJob *createJob = qobject_cast<Akonadi::ItemCreateJob *>(job);
        mInfo->setTodoId(createJob->item().id());
    }
    FollowUpReminder::FollowUpReminderUtil::writeFollowupReminderInfo(FollowUpReminder::FollowUpReminderUtil::defaultConfig(), mInfo, true);
}
