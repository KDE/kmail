/*
   SPDX-FileCopyrightText: 2016-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef CHECKINDEXINGMANAGER_H
#define CHECKINDEXINGMANAGER_H

#include <QObject>
#include <AkonadiCore/Collection>
#include <QAbstractItemModel>
namespace Akonadi {
namespace Search {
namespace PIM {
class IndexedItems;
}
}
}
class QTimer;
class CheckIndexingManager : public QObject
{
    Q_OBJECT
public:
    explicit CheckIndexingManager(Akonadi::Search::PIM::IndexedItems *indexer, QObject *parent = nullptr);
    ~CheckIndexingManager() override;

    void start(QAbstractItemModel *collectionModel);

private:
    Q_DISABLE_COPY(CheckIndexingManager)
    void checkNextCollection();

    void indexingFinished(qint64 index, bool reindexCollection);

    void initializeCollectionList(QAbstractItemModel *model, const QModelIndex &parentIndex = QModelIndex());
    void createJob();
    void callToReindexCollection();

    Akonadi::Search::PIM::IndexedItems *const mIndexedItems;
    Akonadi::Collection::List mListCollection;
    QTimer *mTimer = nullptr;
    QList<qint64> mCollectionsIndexed;
    QList<qint64> mCollectionsNeedToBeReIndexed;
    int mIndex = 0;
    bool mIsReady = true;
};

#endif // CHECKINDEXINGMANAGER_H
