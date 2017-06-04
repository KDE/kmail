/*
  Copyright (c) 2014-2017 Montel Laurent <montel@kde.org>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

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
    ~CreateTaskJob();

    void start() override;
private Q_SLOTS:
    void itemFetchJobDone(KJob *job);

    void slotModifyItemDone(KJob *job);
private:
    void fetchItems();
    Akonadi::Item::List mListItem;
};

#endif // CREATETASKJOB_H
