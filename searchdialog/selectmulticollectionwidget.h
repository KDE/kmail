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
class EntityTreeModel;
class ChangeRecorder;
class EntityRightsFilterModel;
}
class QTreeView;
class KCheckableProxyModel;

class SelectMultiCollectionWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SelectMultiCollectionWidget(const QList<Akonadi::Collection::Id> &selectedCollection, QWidget *parent=0);
    explicit SelectMultiCollectionWidget(QWidget *parent=0);
    ~SelectMultiCollectionWidget();

    QList<Akonadi::Collection> selectedCollection(const QModelIndex &parent = QModelIndex()) const;

private Q_SLOTS:
    void slotCollectionsInserted(const QModelIndex &parent, int start, int end);
    void slotSetCollectionFilter(const QString &filter);

private:
    void initialize();
    void updateStatus(const QModelIndex &parent);
    QList<Akonadi::Collection::Id> mListCollection;
    QTreeView *mFolderView;
    QItemSelectionModel *mSelectionModel;
    Akonadi::EntityTreeModel *mModel;
    Akonadi::ChangeRecorder *mChangeRecorder;
    KCheckableProxyModel *mCheckProxy;
    Akonadi::EntityRightsFilterModel *mCollectionFilter;
};

#endif // SELECTMULTICOLLECTIONWIDGET_H
