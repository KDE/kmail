/*  -*- c++ -*-
    identitylistview.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2002 Marc Mutz <mutz@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

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

  void IdentityListViewItem::redisplay() {
    init( identity() );
  }

  void IdentityListViewItem::init( const KMIdentity & ident ) {
    if ( ident.isDefault() )
      // Add "(Default)" to the end of the default identity's name:
      setText( 0, i18n("%1: identity name. Used in the config "
		       "dialog, section Identity, to indicate the "
		       "default identity", "%1 (Default)")
	       .arg( ident.identityName() ) );
    else
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
    setFullWidth( true );
    setDragEnabled( true );
    setAcceptDrops( true );
    setDropVisualizer( true );
    addColumn( i18n("Identity Name") );
    addColumn( i18n("Email Address") );
    setRootIsDecorated( false );
    setRenameable( 0 );
    setItemsRenameable( true );
    // setShowToolTips( true );
    setItemsMovable( false );
    setAllColumnsShowFocus( true );
    setSorting( -1 ); // disabled
    setSelectionModeExt( Single ); // ### Extended would be nicer...
  }

  void IdentityListView::rename( QListViewItem * i, int col ) {
    if ( col == 0 && isRenameable( col ) ) {
      IdentityListViewItem * item = dynamic_cast<IdentityListViewItem*>( i );
      if ( item ) {
	KMIdentity & ident = item->identity();
	if ( ident.isDefault() )
	  item->setText( 0, ident.identityName() );
      }
    }
    base::rename( i, col );
  }

  bool IdentityListView::acceptDrag( QDropEvent * e ) const {
    // disallow moving:
    return e->source() != viewport() && IdentityDrag::canDecode( e );
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
