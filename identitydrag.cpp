/*  -*- c++ -*-
    identitydrag.cpp

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

#include "identitydrag.h"

#include <qdatastream.h>

namespace KMail {

  static const char kmailIdentityMimeType[] = "application/x-kmail-identity-drag";

  IdentityDrag::IdentityDrag( const KMIdentity & ident,
			      QWidget * dragSource, const char * name )
    : QDragObject( dragSource, name ), mIdent( ident )
  {

  }

  const char * IdentityDrag::format( int i ) const {
    if ( i == 0 )
      return kmailIdentityMimeType;
    else
      return 0;
  }

  QByteArray IdentityDrag::encodedData( const char * mimetype ) const {
    QByteArray a;

    if ( !qstrcmp( mimetype, kmailIdentityMimeType ) ) {
      QDataStream s( a, IO_WriteOnly );
      s << mIdent;
    }

    return a;
  }

  bool IdentityDrag::canDecode( const QMimeSource * e ) {
    // ### feel free to add vCard and other stuff here and in decode...
    return e->provides( kmailIdentityMimeType );
  }

  bool IdentityDrag::decode( const QMimeSource * e, KMIdentity & i ) {

    if ( e->provides( kmailIdentityMimeType ) ) {
      QDataStream s( e->encodedData( kmailIdentityMimeType ), IO_ReadOnly );
      s >> i;
      return true;
    }

    return false;
  }

};

#include "identitydrag.moc"
