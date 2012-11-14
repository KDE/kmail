/*
 * This file is part of KMail.
 * Copyright (c) 2011 Laurent Montel <montel@kde.org>
 * 
 * Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>
 *
 * Parts based on KMail code by:
 * Copyright (c) 2003 Ingo Kloecker <kloecker@kde.org>
 * Copyright (c) 2007 Thomas McGuire <Thomas.McGuire@gmx.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "attachmentview.h"

#include "messagecomposer/attachmentmodel.h"
#include "kmkernel.h"

#include <QContextMenuEvent>
#include <QHeaderView>
#include <QKeyEvent>
#include <QSortFilterProxyModel>

#include <KDebug>
#include <KConfigGroup>

#include <messagecore/attachmentpart.h>
#include <boost/shared_ptr.hpp>
using MessageCore::AttachmentPart;

using namespace KMail;

class KMail::AttachmentView::Private
{
  public:
    Message::AttachmentModel *model;
};

AttachmentView::AttachmentView( Message::AttachmentModel *model, QWidget *parent )
  : QTreeView( parent )
  , d( new Private )
{
  d->model = model;
  connect( model, SIGNAL(encryptEnabled(bool)), this, SLOT(setEncryptEnabled(bool)) );
  connect( model, SIGNAL(signEnabled(bool)), this, SLOT(setSignEnabled(bool)) );

  QSortFilterProxyModel *sortModel = new QSortFilterProxyModel( this );
  sortModel->setSortCaseSensitivity( Qt::CaseInsensitive );
  sortModel->setSourceModel( model );
  setModel( sortModel );
  connect( sortModel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(hideIfEmpty()) );
  connect( sortModel, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(hideIfEmpty()) );
  connect( sortModel, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(selectNewAttachment()) );

  setRootIsDecorated( false );
  setUniformRowHeights( true );
  setSelectionMode( QAbstractItemView::ExtendedSelection );
  setDragDropMode( QAbstractItemView::DragDrop );
  setDropIndicatorShown( false );
  setSortingEnabled( true );

  header()->setResizeMode( QHeaderView::Interactive );
  header()->setStretchLastSection( false );
  restoreHeaderState();
  setColumnWidth( 0, 200 );
}

AttachmentView::~AttachmentView()
{
  saveHeaderState();
  delete d;
}

void AttachmentView::restoreHeaderState()
{
  KConfigGroup grp( KMKernel::self()->config(), "AttachmentView" );
  header()->restoreState( grp.readEntry( "State", QByteArray() ) );
}

void AttachmentView::saveHeaderState()
{
  KConfigGroup grp( KMKernel::self()->config(), "AttachmentView" );
  grp.writeEntry( "State", header()->saveState() );
  grp.sync();
}

void AttachmentView::contextMenuEvent( QContextMenuEvent *event )
{
  Q_UNUSED( event );
  emit contextMenuRequested();
}

void AttachmentView::keyPressEvent( QKeyEvent *event )
{
  if( event->key() == Qt::Key_Delete ) {
    // Indexes are based on row numbers, and row numbers change when items are deleted.
    // Therefore, first we need to make a list of AttachmentParts to delete.
    AttachmentPart::List toRemove;
    foreach( const QModelIndex &index, selectionModel()->selectedRows() ) {
      AttachmentPart::Ptr part = model()->data(
          index, Message::AttachmentModel::AttachmentPartRole ).value<AttachmentPart::Ptr>();
      toRemove.append( part );
    }
    foreach( const AttachmentPart::Ptr &part, toRemove ) {
      d->model->removeAttachment( part );
    }
  }
}

void AttachmentView::dragEnterEvent( QDragEnterEvent *event )
{
  if( event->source() == this ) {
    // Ignore drags from ourselves.
    event->ignore();
  } else {
    QTreeView::dragEnterEvent( event );
  }
}

void AttachmentView::setEncryptEnabled( bool enabled )
{
  setColumnHidden( Message::AttachmentModel::EncryptColumn, !enabled );
}

void AttachmentView::setSignEnabled( bool enabled )
{
  setColumnHidden( Message::AttachmentModel::SignColumn, !enabled );
}

void AttachmentView::hideIfEmpty()
{
  setVisible( model()->rowCount() > 0 );
  emit modified(true);
}

void AttachmentView::selectNewAttachment()
{
  if ( selectionModel()->selectedRows().isEmpty() ) {
    selectionModel()->select( selectionModel()->currentIndex(),
                              QItemSelectionModel::Select | QItemSelectionModel::Rows );
  }
}

void AttachmentView::startDrag( Qt::DropActions supportedActions )
{
  Q_UNUSED( supportedActions );

  const QModelIndexList selection = selectionModel()->selectedRows();
  if( !selection.isEmpty() ) {
    QMimeData *mimeData = model()->mimeData( selection );
    QDrag *drag = new QDrag( this );
    drag->setMimeData( mimeData );
    drag->exec( Qt::CopyAction );
  }
}

#include "attachmentview.moc"
