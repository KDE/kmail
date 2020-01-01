/*
   Copyright (C) 2014-2020 Laurent Montel <montel@kde.org>

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

#ifndef FOLLOWUPREMINDERFINISHTASKJOB_H
#define FOLLOWUPREMINDERFINISHTASKJOB_H

#include <QObject>
#include <AkonadiCore/Item>
class KJob;
class FollowUpReminderFinishTaskJob : public QObject
{
    Q_OBJECT
public:
    explicit FollowUpReminderFinishTaskJob(Akonadi::Item::Id id, QObject *parent = nullptr);
    ~FollowUpReminderFinishTaskJob();

    void start();

Q_SIGNALS:
    void finishTaskDone();
    void finishTaskFailed();

private:
    Q_DISABLE_COPY(FollowUpReminderFinishTaskJob)
    void slotItemFetchJobDone(KJob *job);
    void slotItemModifiedResult(KJob *job);
    void closeTodo();
    Akonadi::Item::Id mTodoId;
};

#endif // FOLLOWUPREMINDERFINISHTASKJOB_H
