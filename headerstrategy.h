/*  -*- c++ -*-
    headerstrategy.h

    KMail, the KDE mail client.
    Copyright (c) 2003 Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, US
*/

#ifndef __KMAIL_HEADERSTRATEGY_H__
#define __KMAIL_HEADERSTRATEGY_H__

class QString;
class QStringList;

namespace KMail {

  class HeaderStrategy {
  protected:
    HeaderStrategy();
    virtual ~HeaderStrategy();

  public:
    //
    // Factory methods:
    //
    enum Type { All, Rich, Standard, Brief, Custom };

    static const HeaderStrategy * create( Type type );
    static const HeaderStrategy * create( const QString & type );

    static const HeaderStrategy * all();
    static const HeaderStrategy * rich();
    static const HeaderStrategy * standard();
    static const HeaderStrategy * brief();
    static const HeaderStrategy * custom();

    //
    // Methods for handling the strategies:
    //
    virtual const char * name() const = 0;
    virtual const HeaderStrategy * next() const = 0;
    virtual const HeaderStrategy * prev() const = 0;

    //
    // HeaderStrategy interface:
    //
    enum DefaultPolicy { Display, Hide };

    virtual QStringList headersToDisplay() const;
    virtual QStringList headersToHide() const;
    virtual DefaultPolicy defaultPolicy() const = 0;
    virtual bool showHeader( const QString & header ) const;
  };

}; // namespace KMail

#endif // __KMAIL_HEADERSTRATEGY_H__
