/*
  Copyright (c) 2014-2016 Montel Laurent <montel@kde.org>

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

#ifndef FOLLOWUPREMINDERSHOWMESSAGEJOB_H
#define FOLLOWUPREMINDERSHOWMESSAGEJOB_H

#include <AkonadiCore/Item>
#include <QObject>

class FollowUpReminderShowMessageJob : public QObject
{
    Q_OBJECT
public:
    explicit FollowUpReminderShowMessageJob(Akonadi::Item::Id id, QObject *parent = Q_NULLPTR);
    ~FollowUpReminderShowMessageJob();

    void start();

private:
    Akonadi::Item::Id mId;

};

#endif // FOLLOWUPREMINDERSHOWMESSAGEJOB_H
