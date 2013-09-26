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

#include <Akonadi/RecursiveCollectionFilterProxyModel>
#include <Akonadi/CollectionFilterProxyModel>

#include <Akonadi/ChangeRecorder>
#include <Akonadi/EntityTreeModel>
#include <Akonadi/EntityRightsFilterModel>
#include <KMime/Message>

#include <KCheckableProxyModel>
#include <KLineEdit>
#include <KLocale>

#include <QVBoxLayout>
#include <QTreeView>

SelectMultiCollectionWidget::SelectMultiCollectionWidget(const QList<Akonadi::Collection::Id> &selectedCollection, QWidget *parent)
    : QWidget(parent),
      mListCollection(selectedCollection)
{
    QVBoxLayout *vbox = new QVBoxLayout;
    setLayout(vbox);


    // Create a new change recorder.
    mChangeRecorder = new Akonadi::ChangeRecorder( this );
    mChangeRecorder->setMimeTypeMonitored( KMime::Message::mimeType() );

    mModel = new Akonadi::EntityTreeModel( mChangeRecorder, this );
    // Set the model to show only collections, not items.
    mModel->setItemPopulationStrategy( Akonadi::EntityTreeModel::NoItemPopulation );

    Akonadi::CollectionFilterProxyModel *mimeTypeProxy = new Akonadi::CollectionFilterProxyModel( this );
    mimeTypeProxy->setExcludeVirtualCollections( true );
    mimeTypeProxy->addMimeTypeFilters( QStringList() << KMime::Message::mimeType() );
    mimeTypeProxy->setSourceModel( mModel );



    // Create the Check proxy model.
    mSelectionModel = new QItemSelectionModel( mimeTypeProxy );
    mCheckProxy = new KCheckableProxyModel( this );
    mCheckProxy->setSelectionModel( mSelectionModel );
    mCheckProxy->setSourceModel( mimeTypeProxy );

    connect(mModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(slotCollectionsInserted(QModelIndex,int,int)));


    mCollectionFilter = new Akonadi::EntityRightsFilterModel(this);
    //mCollectionFilter->addContentMimeTypeInclusionFilter(QLatin1String("message/rfc822"));
    mCollectionFilter->setSourceModel(mCheckProxy);
    mCollectionFilter->setDynamicSortFilter(true);
    mCollectionFilter->setFilterCaseSensitivity(Qt::CaseInsensitive);


    KLineEdit *searchLine = new KLineEdit(this);
    searchLine->setPlaceholderText(i18n("Search..."));
    searchLine->setClearButtonShown(true);
    connect(searchLine, SIGNAL(textChanged(QString)),
            this, SLOT(slotSetCollectionFilter(QString)));

    vbox->addWidget(searchLine);


    mFolderView = new QTreeView;
    mFolderView->setAlternatingRowColors(true);

    vbox->addWidget(mFolderView);
}

SelectMultiCollectionWidget::~SelectMultiCollectionWidget()
{
}

void SelectMultiCollectionWidget::updateStatus(const QModelIndex &parent)
{
    const int nbCol = mCheckProxy->rowCount( parent );
    for ( int i = 0; i < nbCol; ++i ) {
        const QModelIndex child = mCheckProxy->index( i, 0, parent );

        const Akonadi::Collection col =
                mCheckProxy->data( child, Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();

        if (mListCollection.contains(col.id())) {
            mCheckProxy->setData( child, Qt::Checked, Qt::CheckStateRole );
        }
        updateStatus( child );
    }
}

void SelectMultiCollectionWidget::slotCollectionsInserted(const QModelIndex &, int, int)
{
    if (!mListCollection.isEmpty()) {
        updateStatus(QModelIndex());
    }
    mFolderView->expandAll();
}

QList<Akonadi::Collection> SelectMultiCollectionWidget::selectedCollection(const QModelIndex &parent) const
{
    QList<Akonadi::Collection> lst;

    const int nbCol = mCheckProxy->rowCount( parent );
    for ( int i = 0; i < nbCol; ++i ) {
      const QModelIndex child = mCheckProxy->index( i, 0, parent );

      const Akonadi::Collection col =
        mCheckProxy->data( child, Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();

      if (mCheckProxy->data( child, Qt::CheckStateRole ).value<int>())
          lst << col;
      lst << selectedCollection( child );
    }
    return lst;
}

void SelectMultiCollectionWidget::slotSetCollectionFilter(const QString &filter)
{
    mCollectionFilter->setFilterWildcard(filter);
    mFolderView->expandAll();
}



#include "selectmulticollectionwidget.moc"
