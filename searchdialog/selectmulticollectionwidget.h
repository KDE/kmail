/*
  Copyright (c) 2013 Montel Laurent <montel@kde.org>

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

#ifndef SELECTMULTICOLLECTIONWIDGET_H
#define SELECTMULTICOLLECTIONWIDGET_H

#include <QWidget>
#include <Akonadi/Collection>
#include <QModelIndex>
class QItemSelectionModel;
namespace Akonadi {
class CollectionModel;
class CollectionView;
class RecursiveCollectionFilterProxyModel;
}

class SelectMultiCollectionWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SelectMultiCollectionWidget(const QList<Akonadi::Collection::Id> &selectedCollection, QWidget *parent=0);
    ~SelectMultiCollectionWidget();

    QList<Akonadi::Collection::Id> selectedCollection(const QModelIndex &parent) const;

private Q_SLOTS:
    void slotCollectionsInserted(const QModelIndex &parent, int start, int end);

private:
    QList<Akonadi::Collection::Id> (mListCollection);
    QItemSelectionModel *mSelectionModel;
    Akonadi::CollectionModel *mCollectionModel;
    Akonadi::CollectionView *mCollectionView;
    Akonadi::RecursiveCollectionFilterProxyModel *mCollectionFilter;
};

#endif // SELECTMULTICOLLECTIONWIDGET_H
