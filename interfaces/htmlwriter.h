/*  -*- c++ -*-
    interfaces/htmlwriter.h

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

#ifndef __KMAIL_INTERFACES_HTMLWRITER_H__
#define __KMAIL_INTERFACES_HTMLWRITER_H__

class QString;

namespace KMail {
  /**
   * @short An interface to HTML sinks
   * @author Marc Mutz <mutz@kde.org>
   *
   * Operate this interface in one and only one of the following two
   * modes:
   *
   * @sect Sync Mode
   *
   * In sync mode, use @ref #begin() to initiate a session, then @ref
   * #write() some chunks of HTML code and finally @ref #end() the session.
   *
   * @sect Async Mode
   *
   * In async mode, use @ref #begin() to initialize a session, then
   * @ref #queue() some chunks of HTML code and finally end()
   * the session by calling @ref #flush().
   *
   * Queued HTML code is fed to the html sink using a timer. For this
   * to work, control must return to the event loop so timer events
   * are delivered.
   *
   * @sect Combined mode
   *
   * You may combine the two modes in the following way only. Any
   * number of @ref #write() calls can precede @ref #queue() calls,
   * but once a chunk has been queued, you @em must @em not @ref
   * #write() more data, only @ref #queue() it.
   *
   * Naturally, whenever you queued data in a given session, that
   * session must be ended by calling @ref #flush(), not @ref #end().
   */
  class HtmlWriter {
  public:
    virtual ~HtmlWriter() {}
    /** Signal the begin of stuff to write */
    virtual void begin() = 0;
    /** Signal the end of stuff to write. */
    virtual void end() = 0;
    /** Stop all possibly pending processing in order to be able to
	call @ref #begin() again. */
    virtual void reset() = 0;
    /** Write out a chunk of text. No HTML escaping is performed. */
    virtual void write( const QString & str ) = 0;

    virtual void queue( const QString & str ) = 0;
    /** (Start) flushing internal buffers, if any. */
    virtual void flush() = 0;
  };

}; // namespace KMail

#endif // __KMAIL_INTERFACES_HTMLWRITER_H__

