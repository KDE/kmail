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

#ifndef FOLLOWUPREMINDERJOB_H
#define FOLLOWUPREMINDERJOB_H

#include <QObject>

#include <AkonadiCore/Item>

class FollowUpReminderJob : public QObject
{
    Q_OBJECT
public:
    explicit FollowUpReminderJob(QObject *parent = Q_NULLPTR);
    ~FollowUpReminderJob();

    void setItem(const Akonadi::Item &item);

    void start();

Q_SIGNALS:
    void finished(const QString &messageId, Akonadi::Item::Id id);

private Q_SLOTS:
    void slotItemFetchJobDone(KJob *job);

private:
    Akonadi::Item mItem;
};

#endif // FOLLOWUPREMINDERJOB_H
