/*  -*- c++ -*-
    headerstyle.h

    KMail, the KDE mail client.
    Copyright (c) 2003 Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, US
*/

#ifndef __KMAIL_HEADERSTYLE_H__
#define __KMAIL_HEADERSTYLE_H__

class QString;
class KMMessage;

namespace KMail {

  class HeaderStrategy;

  /** This class encapsulates the visual appearance of message
      headers. Together with @ref HeaderStrategy, which determines
      which of the headers present in the message be shown, it is
      responsible for the formatting of message headers.

      @short Encapsulates visual appearance of message headers.
      @author Marc Mutz <mutz@kde.org>
      @see HeaderStrategy
  **/
  class HeaderStyle {
  protected:
    HeaderStyle();
    virtual ~HeaderStyle();

  public:
    //
    // Factory methods:
    //
    enum Type { Brief, Plain, Fancy };

    static const HeaderStyle * create( Type type );
    static const HeaderStyle * create( const QString & type );

    static const HeaderStyle * brief();
    static const HeaderStyle * plain();
    static const HeaderStyle * fancy();

    //
    // Methods for handling the styles:
    //
    virtual const char * name() const = 0;
    virtual const HeaderStyle * next() const = 0;
    virtual const HeaderStyle * prev() const = 0;

    //
    // HeaderStyle interface:
    //
    virtual QString format( const KMMessage * message,
			    const KMail::HeaderStrategy * strategy,
			    const QString & vCardName,
			    bool printing=false ) const = 0;
  };

}; // namespace KMail

#endif // __KMAIL_HEADERSTYLE_H__
