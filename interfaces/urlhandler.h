/*  -*- c++ -*-
    interfaces/urlhandler.h

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

#ifndef __KMAIL_INTERFACES_URLHANDLER_H__
#define __KMAIL_INTERFACES_URLHANDLER_H__

class KURL;

class QString;
class QPoint;
class KMReaderWin;

namespace KMail {
  /**
   * @short An interface to reader link handlers
   * @author Marc Mutz <mutz@kde.org>
   *
   * The @see KMReaderWin parameters are temporary until such time as
   * the Memento-store is in place.
   */
  class URLHandler {
  public:
    virtual ~URLHandler() {}

    /** Called when LMB-clicking on a link in the reader. Should start
	processing equivalent to "opening" the link.

	@return true if the click was handled by this URLHandler,
	false otherwise.
    */
    virtual bool handleClick( const KURL & url, KMReaderWin * w ) const = 0;
    /** Called when RMB-clicking on a link in the reader. Should show
	a context menu at the specified point with the specified
	widget as parent.

	@return true if the right-click was handled by this
	URLHandler, false otherwise.
    */
    virtual bool handleContextMenuRequest( const KURL & url, const QPoint & p, KMReaderWin * w ) const = 0;
    /** Called when hovering over a link.

	@return a string to be shown in the status bar while hovering
	over this link.
    */
    virtual QString statusBarMessage( const KURL & url, KMReaderWin * w ) const = 0;
  };

} // namespace KMail

#endif // __KMAIL_INTERFACES_URLHANDLER_H__

