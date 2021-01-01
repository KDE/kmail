/*
   SPDX-FileCopyrightText: 2016-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef CHECKINDEXINGJOB_H
#define CHECKINDEXINGJOB_H

#include <QObject>
#include <AkonadiCore/Collection>
namespace Akonadi {
namespace Search {
namespace PIM {
class IndexedItems;
}
}
}
class KJob;
class CheckIndexingJob : public QObject
{
    Q_OBJECT
public:
    explicit CheckIndexingJob(Akonadi::Search::PIM::IndexedItems *indexedItems, QObject *parent = nullptr);
    ~CheckIndexingJob();

    void setCollection(const Akonadi::Collection &col);

    void start();

Q_SIGNALS:
    void finished(Akonadi::Collection::Id id, bool needToReindex);

private:
    Q_DISABLE_COPY(CheckIndexingJob)
    void slotCollectionPropertiesFinished(KJob *job);
    void askForNextCheck(quint64 id, bool needToReindex = false);
    Akonadi::Collection mCollection;
    Akonadi::Search::PIM::IndexedItems *const mIndexedItems;
};

#endif // CHECKINDEXINGJOB_H
