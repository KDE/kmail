/*  -*- c++ -*-
    identitylistview.cpp

    KMail, the KDE mail client.
    Copyright (c) 2002 the KMail authors.
    See file AUTHORS for details

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, US
*/

#include "identitylistview.h"

#include "kmidentity.h"
#include "identitydrag.h"
#include "identitymanager.h"
#include "kmkernel.h"

#include <klocale.h> // i18n
#include <kiconloader.h> // SmallIcon

#include <cassert>

namespace KMail {

  //
  //
  // IdentityListViewItem
  //
  //

  IdentityListViewItem::IdentityListViewItem( IdentityListView * parent, const KMIdentity & ident )
    : KListViewItem( parent ), mUOID( ident.uoid() ) {
    init( ident );
  }

  IdentityListViewItem::IdentityListViewItem( IdentityListView * parent, QListViewItem * after, const KMIdentity & ident )
    : KListViewItem( parent, after ), mUOID( ident.uoid() ) {
    init( ident );
  }

  KMIdentity & IdentityListViewItem::identity() const {
    IdentityManager * im = kernel->identityManager();
    assert( im );
    return im->identityForUoid( uoid() );
  }

  void IdentityListViewItem::setIdentity( const KMIdentity & ident ) {
    mUOID = ident.uoid();
    init( ident );
  }

  void IdentityListViewItem::init( const KMIdentity & ident ) {
    setText( 0, ident.identityName() );
    setText( 1, ident.fullEmailAddr() );
  }

  //
  //
  // IdentityListView
  //
  //

  IdentityListView::IdentityListView( QWidget * parent, const char * name )
    : KListView( parent, name )
  {
    setDragEnabled( true );
    setAcceptDrops( true );
    setDropVisualizer( true );
    addColumn( i18n("Identity Name") );
    addColumn( i18n("Email Address") );
    setRootIsDecorated( false );
    setRenameable( 0 );
    setItemsRenameable( true );
    // setShowToolTips( true );
    setItemsMovable( true );
    setAllColumnsShowFocus( true );
    setSorting( -1 ); // disabled
    setSelectionModeExt( Single ); // ### Extended would be nicer...
  }

  bool IdentityListView::acceptDrag( QDropEvent * e ) const {
    return e->source() == viewport() || IdentityDrag::canDecode( e );
  }

  QDragObject * IdentityListView::dragObject() {
    IdentityListViewItem * item = dynamic_cast<IdentityListViewItem*>( currentItem() );
    if ( !item ) return 0;

    IdentityDrag * drag = new IdentityDrag( item->identity(), viewport() );
    drag->setPixmap( SmallIcon("identity") );
    return drag;
  }

}; // namespace KMail


#include "identitylistview.moc"
