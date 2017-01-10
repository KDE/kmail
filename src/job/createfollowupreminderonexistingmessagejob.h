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

#ifndef CREATEFOLLOWUPREMINDERONEXISTINGMESSAGEJOB_H
#define CREATEFOLLOWUPREMINDERONEXISTINGMESSAGEJOB_H

#include <QObject>
#include <AkonadiCore/Collection>
#include <QDate>
#include <AkonadiCore/Item>

class CreateFollowupReminderOnExistingMessageJob : public QObject
{
    Q_OBJECT
public:
    explicit CreateFollowupReminderOnExistingMessageJob(QObject *parent = nullptr);
    ~CreateFollowupReminderOnExistingMessageJob();

    void start();

    Akonadi::Collection collection() const;
    void setCollection(const Akonadi::Collection &collection);

    QDate date() const;
    void setDate(const QDate &date);

    Akonadi::Item messageItem() const;
    void setMessageItem(const Akonadi::Item &messageItem);

    bool canStart() const;

private Q_SLOTS:
    void itemFetchJobDone(KJob *job);
    void slotReminderDone(KJob *job);
private:
    void doStart();
    Akonadi::Collection mCollection;
    Akonadi::Item mMessageItem;
    QDate mDate;
};

#endif // CREATEFOLLOWUPREMINDERONEXISTINGMESSAGEJOB_H
