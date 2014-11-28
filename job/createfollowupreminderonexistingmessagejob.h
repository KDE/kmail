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

#ifndef CREATEFOLLOWUPREMINDERONEXISTINGMESSAGEJOB_H
#define CREATEFOLLOWUPREMINDERONEXISTINGMESSAGEJOB_H

#include <QObject>
#include <Akonadi/Collection>
#include <QDate>
#include <akonadi/item.h>

class CreateFollowupReminderOnExistingMessageJob : public QObject
{
    Q_OBJECT
public:
    explicit CreateFollowupReminderOnExistingMessageJob(QObject *parent=0);
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
