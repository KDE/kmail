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

    } else {
        qDebug()<<"FollowupReminderCreateJob info not valid ";
        deleteLater();
        return;
    }
}



