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

#include <khtml_part.h>
#include <khtmlview.h>
#include <kurl.h>

#include <qstring.h>

#include <cassert>

namespace KMail {

  KHtmlPartHtmlWriter::KHtmlPartHtmlWriter( KHTMLPart * part,
					    QObject * parent, const char * name )
    : QObject( parent, name ), HtmlWriter(),
      mHtmlPart( part )
  {
    assert( part );
    connect( &mHtmlTimer, SIGNAL(timeout()), SLOT(slotWriteNextHtmlChunk()) );
  }

  KHtmlPartHtmlWriter::~KHtmlPartHtmlWriter() {

  }

  void KHtmlPartHtmlWriter::begin() {
    mHtmlPart->begin( KURL( "file:/" ) );
  }

  void KHtmlPartHtmlWriter::end() {
    mHtmlPart->end();
  }

  void KHtmlPartHtmlWriter::reset() {
    if ( mHtmlTimer.isActive() ) {
      mHtmlTimer.stop();
      end();
    }
    mHtmlQueue.clear();
  }

  void KHtmlPartHtmlWriter::write( const QString & str ) {
    mHtmlPart->write( str );
  }

  void KHtmlPartHtmlWriter::queue( const QString & str ) {
    static const uint chunksize = 16384;
    for ( uint pos = 0 ; pos < str.length() ; pos += chunksize )
      mHtmlQueue.push_back( str.mid( pos, chunksize ) );
  }

  void KHtmlPartHtmlWriter::flush() {
    slotWriteNextHtmlChunk();
  }

  void KHtmlPartHtmlWriter::slotWriteNextHtmlChunk() {
    if ( mHtmlQueue.empty() ) {
      end();
      mHtmlPart->view()->viewport()->setUpdatesEnabled( true );
      mHtmlPart->view()->setUpdatesEnabled( true );
      mHtmlPart->view()->viewport()->repaint( false );
    } else {
      write( mHtmlQueue.front() );
      mHtmlQueue.pop_front();
      mHtmlTimer.start( 0, true );
    }
  }

  

}; // namespace KMail

#include "khtmlparthtmlwriter.moc"
