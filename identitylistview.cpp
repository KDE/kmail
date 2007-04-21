/*  -*- c++ -*-
    identitylistview.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2002 Marc Mutz <mutz@kde.org>
                  2007 Mathias Soeken <msoeken@tzi.de>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "identitylistview.h"

#include <libkpimidentities/identitymanager.h>
#include <libkpimidentities/identity.h>
#include "kmkernel.h"

#include <KLocale> // i18n
#include <KIconLoader> // SmallIcon

#include <QDropEvent>
#include <QHeaderView>

namespace KMail {

  //
  //
  // IdentityListViewItem
  //
  //

  IdentityListViewItem::IdentityListViewItem( IdentityListView *parent, 
                                              const KPIM::Identity &ident )
    : QTreeWidgetItem( parent ), mUOID( ident.uoid() ) 
  {
    init( ident );
  }

  IdentityListViewItem::IdentityListViewItem( IdentityListView *parent, 
                                              QTreeWidgetItem *after,
                                              const KPIM::Identity &ident )
    : QTreeWidgetItem( parent, after ), mUOID( ident.uoid() ) 
  {
    init( ident );
  }

  KPIM::Identity & IdentityListViewItem::identity() const 
  {
    KPIM::IdentityManager *im = kmkernel->identityManager();
    Q_ASSERT( im );
    return im->modifyIdentityForUoid( mUOID );
  }

  void IdentityListViewItem::setIdentity( const KPIM::Identity &ident ) 
  {
    mUOID = ident.uoid();
    init( ident );
  }

  void IdentityListViewItem::redisplay() 
  {
    init( identity() );
  }

  void IdentityListViewItem::init( const KPIM::Identity &ident ) 
  {
    if ( ident.isDefault() ) {
      // Add "(Default)" to the end of the default identity's name:
      setText( 0, i18nc("%1: identity name. Used in the config "
                        "dialog, section Identity, to indicate the "
                        "default identity", "%1 (Default)",
                        ident.identityName() ) );
    } else {
      setText( 0, ident.identityName() );
    }
    setText( 1, ident.fullEmailAddr() );
  }

  //
  //
  // IdentityListView
  //
  //

  IdentityListView::IdentityListView( QWidget *parent )
    : QTreeWidget( parent )
  {
    setDragEnabled( true );
    setAcceptDrops( true );
    setHeaderLabels( QStringList() << i18n("Identity Name") << i18n("Email Address") );
    setRootIsDecorated( false );
    header()->setMovable( false );
    setAllColumnsShowFocus( true );
    setSortingEnabled( false ); // disabled
    setSelectionMode( SingleSelection ); // ### Extended would be nicer...

    setContextMenuPolicy( Qt::CustomContextMenu );
    connect( this, SIGNAL(customContextMenuRequested(const QPoint&)),
             this, SLOT(slotCustomContextMenuRequested(const QPoint&)) );
  }

  void IdentityListView::editItem( QTreeWidgetItem *item, int column )
  {
    if (column == 0 && item) {
      IdentityListViewItem *lvItem = dynamic_cast<IdentityListViewItem*>( item );
      if (lvItem) {
        KPIM::Identity& ident = lvItem->identity();
        if (ident.isDefault()) {
          lvItem->setText( 0, ident.identityName() );
        }
      }

      Qt::ItemFlags oldFlags = item->flags();
      item->setFlags( oldFlags | Qt::ItemIsEditable );
      QTreeWidget::editItem( item, 0 );
      item->setFlags( oldFlags );
    }
  }

  void IdentityListView::commitData( QWidget *editor )
  {
    kDebug() << "after editing" << endl;

    if (selectedItems().size() > 0) {
      IdentityListViewItem *item = dynamic_cast<IdentityListViewItem*>( selectedItems()[0] );

      QLineEdit *edit = dynamic_cast<QLineEdit*>( editor );
      if (edit) {
        QString text = edit->text();
        emit rename( item, text );
      }
    }
  }

  void IdentityListView::slotCustomContextMenuRequested( const QPoint &pos )
  {
    kDebug() << "position: " << pos << endl;
    QTreeWidgetItem *item = itemAt( pos );
    kDebug() << "item: " << item << endl;
    if (item) {
      IdentityListViewItem *lvItem = dynamic_cast<IdentityListViewItem*>( item );
      if (lvItem) {
        emit contextMenu( lvItem, viewport()->mapToGlobal(pos) );
      }
    } else {
      emit contextMenu( 0L, viewport()->mapToGlobal(pos) );
    }
  }

  bool IdentityListView::acceptDrag( QDropEvent *e ) const {
    // disallow moving:
    return e->source() != viewport() && KPIM::Identity::canDecode( e->mimeData() );
  }

  Q3DragObject *IdentityListView::dragObject() {
    IdentityListViewItem * item = dynamic_cast<IdentityListViewItem*>( currentItem() );
    if ( !item ) return 0;

    return 0;
#ifdef __GNUC__
#warning enable the QDrag-based code once the list view does no longer derive from Q3ListView...
#endif
    QDrag *drag = new QDrag( viewport() );
    QMimeData *md = new QMimeData;
    drag->setMimeData( md );
    item->identity().populateMimeData( md );
    drag->setPixmap( SmallIcon("identity") );
    //return drag;
  }

} // namespace KMail


#include "identitylistview.moc"
