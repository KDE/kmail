/*  -*- c++ -*-
    urlhandlermanager.h

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2003 Marc Mutz <mutz@kde.org>

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

#ifndef __KMAIL_URLHANDLERMANAGER_H__
#define __KMAIL_URLHANDLERMANAGER_H__

#include <qvaluevector.h>

class KURL;

class QString;
class QPoint;
class KMReaderWin;

namespace KMail {

  class URLHandler;

  namespace Interface {
    class BodyPartURLHandler;
  }

  /**
   * @short Singleton to manage the list of @see URLHandlers
   * @author Marc Mutz <mutz@kde.org>
   */
  class URLHandlerManager {
    static URLHandlerManager * self;

    URLHandlerManager();
  public:
    ~URLHandlerManager();

    static URLHandlerManager * instance() {
      if ( !self )
	self = new URLHandlerManager();
      return self;
    }

    void registerHandler( const URLHandler * handler );
    void unregisterHandler( const URLHandler * handler );

    void registerHandler( const Interface::BodyPartURLHandler * handler );
    void unregisterHandler( const Interface::BodyPartURLHandler * handler );

    bool handleClick( const KURL & url, KMReaderWin * w=0 ) const;
    bool handleContextMenuRequest( const KURL & url, const QPoint & p, KMReaderWin * w=0 ) const;
    QString statusBarMessage( const KURL & url, KMReaderWin * w=0 ) const;

  private:
    typedef QValueVector<const URLHandler*> HandlerList;
    HandlerList mHandlers;
    class BodyPartURLHandlerManager;
    BodyPartURLHandlerManager * mBodyPartURLHandlerManager;
  };

} // namespace KMail

#endif // __KMAIL_URLHANDLERMANAGER_H__

