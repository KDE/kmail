/*
   SPDX-FileCopyrightText: 2013-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Item>
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
    void slotItemDeleteDone(KJob *job);
    void removeMessageItem();
    const QVector<Akonadi::Item::Id> mListItems;
    int mIndex = 0;
};
