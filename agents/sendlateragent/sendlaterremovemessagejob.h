/*
   SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Item>
#include <QObject>

class KJob;

class SendLaterRemoveMessageJob : public QObject
{
    Q_OBJECT
public:
    explicit SendLaterRemoveMessageJob(const QVector<Akonadi::Item::Id> &listItem, QObject *parent = nullptr);
    ~SendLaterRemoveMessageJob() override;

    void start();

private:
    Q_DISABLE_COPY(SendLaterRemoveMessageJob)
    void slotItemDeleteDone(KJob *job);
    void deleteItem();
    const QVector<Akonadi::Item::Id> mListItems;
    int mIndex = 0;
};

