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

#include "foldertreeview.h"
#include <kdebug.h>
#include <QDebug>
#include <KLocale>
#include <akonadi/entitytreemodel.h>
#include <akonadi/collectionstatistics.h>
FolderTreeView::FolderTreeView(QWidget *parent )
  : Akonadi::EntityTreeView( parent )
{
  init();
}


FolderTreeView::FolderTreeView(KXMLGUIClient *xmlGuiClient, QWidget *parent )
  :Akonadi::EntityTreeView( xmlGuiClient, parent )
{
  init();
}


FolderTreeView::~FolderTreeView()
{
}

void FolderTreeView::init()
{
}

void FolderTreeView::selectModelIndex( const QModelIndex & index )
{
  if ( index.isValid() ) {
    clearSelection();
    scrollTo( index );
    setCurrentIndex(index);
  }
}


void FolderTreeView::slotFocusNextFolder()
{
  QModelIndex nextFolder = selectNextFolder( currentIndex() );

  if ( nextFolder.isValid() ) {
    expand( nextFolder );
    selectModelIndex( nextFolder );
  }
}

QModelIndex FolderTreeView::selectNextFolder( const QModelIndex & current )
{
  QModelIndex below;
  if ( current.isValid() ) {
    model()->fetchMore( current );
    if ( model()->hasChildren( current ) ) {
      expand( current );
      below = indexBelow( current );
    } else if ( current.row() < model()->rowCount( model()->parent( current ) ) -1 ) {
      below = model()->index( current.row()+1, current.column(), model()->parent( current ) );
    } else {
      below = indexBelow( current );
    }
  }
  return below;
}

void FolderTreeView::slotFocusPrevFolder()
{
  QModelIndex current = currentIndex();
  if ( current.isValid() ) {
    QModelIndex above = indexAbove( current );
    selectModelIndex( above );
  }
}

void FolderTreeView::selectNextUnreadFolder()
{
  kDebug()<<"Need to implement  FolderTreeView::selectNextUnreadFolder() ";
  QModelIndex current = selectNextFolder( currentIndex() );
  while ( current.isValid() ) {
    QModelIndex nextIndex;
    if ( isUnreadFolder( current,nextIndex,FolderTreeView::Next ) ) {
      selectModelIndex( current );
      return;
    }
    current = nextIndex;
  }

  current = rootIndex();
  kDebug()<<" current :"<<current;
  while ( current.isValid() ) {
    QModelIndex nextIndex;
    if ( isUnreadFolder( current,nextIndex, FolderTreeView::Next ) ) {
      selectModelIndex( current );
      return;
    }
    current = nextIndex;
  }
}

bool FolderTreeView::isUnreadFolder( const QModelIndex & current, QModelIndex &index, FolderTreeView::Move move )
{
  if ( current.isValid() ) {

    if ( move == FolderTreeView::Next )
      index = selectNextFolder( current );
    else if ( move == FolderTreeView::Previous )
      index = indexAbove( current );

    if ( index.isValid() ) {
      Akonadi::Collection collection = index.model()->data( current, Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();
      if ( collection.isValid() ) {
        if ( collection.statistics().unreadCount()>0 ) {
          return true;
        }
      }
    }
  }
  return false;
}

void FolderTreeView::selectPrevUnreadFolder()
{
  kDebug()<<" Need to implement FolderTreeView::selectPrevUnreadFolder()";

  QModelIndex current = indexAbove( currentIndex() );
  while ( current.isValid() ) {
    QModelIndex nextIndex;
    if ( isUnreadFolder( current,nextIndex,FolderTreeView::Previous ) ) {
      selectModelIndex( current );
      return;
    }
    current = nextIndex;
  }
  //TODO start at the end.
}

Akonadi::Collection FolderTreeView::currentFolder()
{
  QModelIndex current = currentIndex();
  if ( current.isValid() ) {
    Akonadi::Collection collection = current.model()->data( current, Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();
    return collection;
  }
  return Akonadi::Collection();
}

#include "foldertreeview.moc"
