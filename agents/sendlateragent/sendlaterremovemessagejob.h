/*
   SPDX-FileCopyrightText: 2013-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SENDLATERREMOVEMESSAGEJOB_H
#define SENDLATERREMOVEMESSAGEJOB_H

#include <QObject>
#include <Item>

class KJob;

class SendLaterRemoveMessageJob : public QObject
{
    Q_OBJECT
public:
    explicit SendLaterRemoveMessageJob(const QVector<Akonadi::Item::Id> &listItem, QObject *parent = nullptr);
    ~SendLaterRemoveMessageJob();

    void start();

private:
    Q_DISABLE_COPY(SendLaterRemoveMessageJob)
    void slotItemDeleteDone(KJob *job);
    void deleteItem();
    QVector<Akonadi::Item::Id> mListItems;
    int mIndex = 0;
};

#endif // SENDLATERREMOVEMESSAGEJOB_H
