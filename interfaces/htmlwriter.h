/*  -*- c++ -*-
    interfaces/htmlwriter.h

    KMail, the KDE mail client.
    Copyright (c) 2003 Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, US
*/

#ifndef __KMAIL_INTERFACES_HTMLWRITER_H__
#define __KMAIL_INTERFACES_HTMLWRITER_H__

class QString;

namespace KMail {

  class HtmlWriter {
  public:
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

