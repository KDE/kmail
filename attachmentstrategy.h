/*  -*- c++ -*-
    attachmentstrategy.h

    KMail, the KDE mail client.
    Copyright (c) 2003 Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, US
*/

#ifndef __KMAIL_ATTACHMENTSTRATEGY_H__
#define __KMAIL_ATTACHMENTSTRATEGY_H__

class QString;

namespace KMail {

  class AttachmentStrategy {
  protected:
    AttachmentStrategy();
    virtual ~AttachmentStrategy();

  public:
    //
    // Factory methods:
    //
    enum Type { Iconic, Smart, Inlined, Hidden };

    static const AttachmentStrategy * create( Type type );
    static const AttachmentStrategy * create( const QString & type );

    static const AttachmentStrategy * iconic();
    static const AttachmentStrategy * smart();
    static const AttachmentStrategy * inlined();
    static const AttachmentStrategy * hidden();

    virtual const char * name() const = 0;
    virtual const AttachmentStrategy * next() const = 0;
    virtual const AttachmentStrategy * prev() const = 0;
  };

}; // namespace KMail

#endif // __KMAIL_ATTACHMENTSTRATEGY_H__
