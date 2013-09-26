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

#include "selectmulticollectionwidget.h"

#include <Akonadi/CollectionView>
#include <Akonadi/CollectionModel>
#include <Akonadi/RecursiveCollectionFilterProxyModel>
#include <Akonadi/CollectionFilterProxyModel>

#include <KCheckableProxyModel>
#include <KLineEdit>
#include <KLocale>

#include <QVBoxLayout>

SelectMultiCollectionWidget::SelectMultiCollectionWidget(const QList<Akonadi::Collection::Id> &selectedCollection, QWidget *parent)
    : QWidget(parent),
      mListCollection(selectedCollection)
{
    QVBoxLayout *vbox = new QVBoxLayout;
    setLayout(vbox);


    mCollectionModel = new Akonadi::CollectionModel(this);
    connect(mCollectionModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(slotCollectionsInserted(QModelIndex,int,int)));

    mSelectionModel = new QItemSelectionModel(mCollectionModel, this);

    KCheckableProxyModel *checkableProxy = new KCheckableProxyModel(this);
    checkableProxy->setSourceModel(mCollectionModel);
    checkableProxy->setSelectionModel(mSelectionModel);

    mCollectionFilter = new Akonadi::RecursiveCollectionFilterProxyModel(this);
    mCollectionFilter->addContentMimeTypeInclusionFilter(QLatin1String("message/rfc822"));
    mCollectionFilter->setSourceModel(checkableProxy);
    mCollectionFilter->setDynamicSortFilter(true);
    mCollectionFilter->setFilterCaseSensitivity(Qt::CaseInsensitive);

    KLineEdit *searchLine = new KLineEdit(this);
    searchLine->setPlaceholderText(i18n("Search..."));
    searchLine->setClearButtonShown(true);
    connect(searchLine, SIGNAL(textChanged(QString)),
            this, SLOT(slotSetCollectionFilter(QString)));

    vbox->addWidget(searchLine);

    mCollectionView = new Akonadi::CollectionView(this);
    mCollectionView->setModel(mCollectionFilter);
    mCollectionView->setAlternatingRowColors(true);
    vbox->addWidget(mCollectionView);
}

SelectMultiCollectionWidget::~SelectMultiCollectionWidget()
{
}

void SelectMultiCollectionWidget::slotCollectionsInserted(const QModelIndex &parent, int start, int end)
{
    if (!mListCollection.isEmpty()) {
        for (int i = start; i <= end; ++i) {
            const QModelIndex index = mCollectionModel->index(i, 0, parent);
            if (!index.isValid()) {
                continue;
            }
            const Akonadi::Collection collection = index.data(Akonadi::CollectionModel::CollectionRole).value<Akonadi::Collection>();
            if (mListCollection.contains(collection.id()))
                mSelectionModel->select(index, QItemSelectionModel::Select);
        }
    }
    mCollectionView->expandAll();
}

QList<Akonadi::Collection> SelectMultiCollectionWidget::selectedCollection(const QModelIndex &parent) const
{
    QList<Akonadi::Collection> lst;

    for (int i = 0; i < mCollectionModel->rowCount(parent); ++i) {
        const QModelIndex index = mCollectionModel->index(i, 0, parent);
        if (mCollectionModel->hasChildren(index)) {
            lst.append(selectedCollection(index));
        }

        const bool selected = mSelectionModel->isSelected(index);
        if (selected) {
            const Akonadi::Collection collection = index.data(Akonadi::CollectionModel::CollectionRole).value<Akonadi::Collection>();
            lst << collection;
        }

    }
    return lst;
}

void SelectMultiCollectionWidget::slotSetCollectionFilter(const QString &filter)
{
    mCollectionFilter->setSearchPattern(filter);
    mCollectionView->expandAll();
}



#include "selectmulticollectionwidget.moc"
