/*
  SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#pragma once

#include <AkonadiCore/Item>
#include <KJob>

class CreateTaskJob : public KJob
{
    Q_OBJECT
public:
    explicit CreateTaskJob(const Akonadi::Item::List &items, QObject *parent = nullptr);
    ~CreateTaskJob() override;

    void start() override;

private:
    void itemFetchJobDone(KJob *job);

    void slotModifyItemDone(KJob *job);

    void fetchItems();
    const Akonadi::Item::List mListItem;
};

