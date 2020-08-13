/*
  SPDX-FileCopyrightText: 2014-2020 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#ifndef CREATETASKJOB_H
#define CREATETASKJOB_H

#include <KJob>
#include <AkonadiCore/Item>

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
    Akonadi::Item::List mListItem;
};

#endif // CREATETASKJOB_H
