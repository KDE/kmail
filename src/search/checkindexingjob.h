/*
   Copyright (C) 2016 Montel Laurent <montel@kde.org>

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

#ifndef CHECKINDEXINGJOB_H
#define CHECKINDEXINGJOB_H

#include <QObject>
#include <AkonadiCore/Collection>
namespace Akonadi
{
namespace Search
{
namespace PIM
{
class IndexedItems;
}
}
}
class KJob;
class CheckIndexingJob : public QObject
{
    Q_OBJECT
public:
    explicit CheckIndexingJob(Akonadi::Search::PIM::IndexedItems *indexedItems, QObject *parent = Q_NULLPTR);
    ~CheckIndexingJob();

    void setCollection(const Akonadi::Collection &col);

    void start();

Q_SIGNALS:
    void finished(Akonadi::Collection::Id id, bool needToReindex);

private Q_SLOTS:
    void slotCollectionPropertiesFinished(KJob *job);
private:
    void askForNextCheck(quint64 id, bool needToReindex = false);
    Akonadi::Collection mCollection;
    Akonadi::Search::PIM::IndexedItems *mIndexedItems;
};

#endif // CHECKINDEXINGJOB_H
