/*  -*- c++ -*-
    identitydrag.h

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

#ifndef __KMAIL_IDENTITYDRAG_H__
#define __KMAIL_IDENTITYDRAG_H__

#include <libkpimidentities/identity.h>

#include <qdragobject.h> // is a qobject and a qmimesource

namespace KMail {

  /** @short A @see QDragObject for @see KPIM::Identity
      @author Marc Mutz <mutz@kde.org>
  **/
  class IdentityDrag : public QDragObject {
    Q_OBJECT
  public:
    IdentityDrag( const KPIM::Identity & ident,
		  QWidget * dragSource=0, const char * name=0 );

  public:
    virtual ~IdentityDrag() {}

    const char * format( int i ) const; // reimp. QMimeSource
    QByteArray encodedData( const char * mimetype ) const; // dto.

    static bool canDecode( const QMimeSource * e );
    static bool decode( const QMimeSource * e, KPIM::Identity & ident );

  protected:
    KPIM::Identity mIdent;
  };

} // namespace KMail

#endif // __KMAIL_IDENTITYDRAG_H__
