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

#include "teehtmlwriter.h"

#include <kdebug.h>

#include <qstring.h>

namespace KMail {

  TeeHtmlWriter::TeeHtmlWriter( HtmlWriter * writer1, HtmlWriter * writer2 )
    : HtmlWriter()
  {
    mWriters.setAutoDelete( true );
    if ( writer1 )
      mWriters.append( writer1 );
    if ( writer2 )
      mWriters.append( writer2 );
  }

  TeeHtmlWriter::~TeeHtmlWriter() {
  }

  void TeeHtmlWriter::addHtmlWriter( HtmlWriter * writer ) {
    if ( writer )
      mWriters.append( writer );
  }

  void TeeHtmlWriter::begin() {
    for ( QPtrListIterator<HtmlWriter> it( mWriters ) ; it.current() ; ++it )
      it.current()->begin();
  }

  void TeeHtmlWriter::end() {
    for ( QPtrListIterator<HtmlWriter> it( mWriters ) ; it.current() ; ++it )
      it.current()->end();
  }

  void TeeHtmlWriter::reset() {
    for ( QPtrListIterator<HtmlWriter> it( mWriters ) ; it.current() ; ++it )
      it.current()->reset();
  }

  void TeeHtmlWriter::write( const QString & str ) {
    for ( QPtrListIterator<HtmlWriter> it( mWriters ) ; it.current() ; ++it )
      it.current()->write( str );
  }

  void TeeHtmlWriter::queue( const QString & str ) {
    for ( QPtrListIterator<HtmlWriter> it( mWriters ) ; it.current() ; ++it )
      it.current()->queue( str );
  }

  void TeeHtmlWriter::flush() {
    for ( QPtrListIterator<HtmlWriter> it( mWriters ) ; it.current() ; ++it )
      it.current()->flush();
  }

}; // namespace KMail
