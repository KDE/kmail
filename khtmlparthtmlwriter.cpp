/*  -*- c++ -*-
    khtmlparthtmlwriter.cpp

    KMail, the KDE mail client.
    Copyright (c) 2003 Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, US
*/

#include "khtmlparthtmlwriter.h"

#include "kmreaderwin.h"

#include <khtml_part.h>
#include <kurl.h>

#include <qstring.h>

#include <cassert>

namespace KMail {

  KHtmlPartHtmlWriter::KHtmlPartHtmlWriter( KMReaderWin * readerWin )
    : HtmlWriter(),
      mReaderWin( readerWin )
  {
    assert( readerWin );
  }

  KHtmlPartHtmlWriter::~KHtmlPartHtmlWriter() {

  }

  void KHtmlPartHtmlWriter::begin() {
    mReaderWin->mViewer->begin( KURL( "file:/" ) );
  }

  void KHtmlPartHtmlWriter::end() {
    mReaderWin->mViewer->end();
  }

  //void KHtmlPartHtmlWriter::escapeAndWrite( const QString & str ) {
  //  write( escapeHTML( str ) );
  //}

  void KHtmlPartHtmlWriter::write( const QString & str ) {
    mReaderWin->mViewer->write( str );
  }

  //void KHtmlPartHtmlWriter::escapeAndQueue( const QString & str ) {
  //  queue( escapeHTML( str ) );
  //}

  void KHtmlPartHtmlWriter::queue( const QString & str ) {
    mReaderWin->queueHtml( str );
  }

  void KHtmlPartHtmlWriter::flush() {
    mReaderWin->sendNextHtmlChunk();
  }

#if 0
  QString KHtmlPartHtmlWriter::escapeHTML( const QString & str ) {
    QString result = str;
    result.replace( '&', "&amp;" );
    result.replace( '<', "&lt;" );
    result.replace( '>', "&gt;" );
    result.replace( '"', "&quot;" );
    result.replace( '\'', "&apos;" );
    return result;
  }
#endif

}; // namespace KMail
