/*  -*- c++ -*-
    identitydrag.h

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

#ifndef __KMAIL_IDENTITYDRAG_H__
#define __KMAIL_IDENTITYDRAG_H__

#include "kmidentity.h"

#include <qdragobject.h> // is a qobject and a qmimesource

namespace KMail {

  /** @short A @ref QDragObject for @ref KMIdentity
      @author Marc Mutz <mutz@kde.org>
  **/
  class IdentityDrag : public QDragObject {
    Q_OBJECT
  public:
    IdentityDrag( const KMIdentity & ident,
		  QWidget * dragSource=0, const char * name=0 );

  public:
    virtual ~IdentityDrag() {}

    const char * format( int i ) const; // reimp. QMimeSource
    QByteArray encodedData( const char * mimetype ) const; // dto.

    static bool canDecode( const QMimeSource * e );
    static bool decode( const QMimeSource * e, KMIdentity & ident );

  protected:
    KMIdentity mIdent;
  };

}; // namespace KMail

#endif // __KMAIL_IDENTITYDRAG_H__
