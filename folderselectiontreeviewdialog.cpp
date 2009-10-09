/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2009 Montel Laurent <montel@kde.org>

  KMail is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  KMail is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "folderselectiontreeviewdialog.h"
#include <QVBoxLayout>
#include "folderselectiontreeview.h"
#include <akonadi/collection.h>
#include <akonadi/entitytreemodel.h>

FolderSelectionTreeViewDialog::FolderSelectionTreeViewDialog( QWidget *parent )
  :KDialog( parent )
{
  QWidget *widget = mainWidget();
  QVBoxLayout *layout = new QVBoxLayout( widget );
  treeview = new FolderSelectionTreeView( this );
  layout->addWidget( treeview );
  enableButton( KDialog::Ok, false );
  connect(treeview->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(slotSelectionChanged()));
}

FolderSelectionTreeViewDialog::~FolderSelectionTreeViewDialog()
{
}

void FolderSelectionTreeViewDialog::slotSelectionChanged()
{
  enableButton(KDialog::Ok, treeview->selectionModel()->selectedIndexes().count() > 0);
}

void FolderSelectionTreeViewDialog::setSelectionMode( QAbstractItemView::SelectionMode mode )
{
  treeview->setSelectionMode( mode );
}

QAbstractItemView::SelectionMode FolderSelectionTreeViewDialog::selectionMode() const
{
  return treeview->selectionMode();
}


Akonadi::Collection FolderSelectionTreeViewDialog::selectedCollection() const
{
  if ( treeview->selectionMode() == QAbstractItemView::SingleSelection ) {
    const QModelIndex index = treeview->currentIndex();
    if ( index.isValid() )
      return index.model()->data( index, Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();
  }

  return Akonadi::Collection();
}

Akonadi::Collection::List FolderSelectionTreeViewDialog::selectedCollections() const
{
  Akonadi::Collection::List collections;
  const QItemSelectionModel *selectionModel = treeview->selectionModel();
  const QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
  foreach ( const QModelIndex &index, selectedIndexes ) {
    if ( index.isValid() ) {
      const Akonadi::Collection collection = index.model()->data( index, Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();
      if ( collection.isValid() )
        collections.append( collection );
    }
  }

  return collections;
}

#include "folderselectiontreeviewdialog.moc"
