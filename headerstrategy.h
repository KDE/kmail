/*  -*- c++ -*-
    headerstrategy.h

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

} // namespace KMail

#endif // __KMAIL_HEADERSTRATEGY_H__
