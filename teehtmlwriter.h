/*  -*- c++ -*-
    teehtmlwriter.h

    KMail, the KDE mail client.
    Copyright (c) 2003 Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, US
*/

#ifndef __KMAIL_TEEHTMLWRITER_H__
#define __KMAIL_TEEHTMLWRITER_H__

#include "interfaces/htmlwriter.h"

#include <qptrlist.h>

class QString;

namespace KMail {


  /** @short A @ref HtmlWriter that dispatches all calls to a list of other @ref HtmlWriters
      @author Marc Mutz <mutz@kde.org>
  **/
  class TeeHtmlWriter : public KMail::HtmlWriter {
  public:
    TeeHtmlWriter( KMail::HtmlWriter * writer1=0, KMail::HtmlWriter * writer2=0  );
    virtual ~TeeHtmlWriter();

    void addHtmlWriter( KMail::HtmlWriter * writer );

    //
    // HtmlWriter Interface
    //
    void begin();
    void end();
    void reset();
    void write( const QString & str );
    void queue( const QString & str );
    void flush();

  private:
    /** We own the HtmlWriters added to us! */
    QPtrList<KMail::HtmlWriter> mWriters;
  };

}; // namespace KMail

#endif // __KMAIL_TEEHTMLWRITER_H__
