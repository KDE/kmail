/*  -*- c++ -*-
    khtmlparthtmlwriter.cpp

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

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

#include <config.h>

#include "khtmlparthtmlwriter.h"

#include <kdebug.h>
#include <khtml_part.h>
#include <khtmlview.h>

#include <dom/dom_string.h>
#include <dom/html_document.h>
#include <dom/html_image.h>
#include <dom/html_misc.h>

#include <cassert>

namespace KMail {

  KHtmlPartHtmlWriter::KHtmlPartHtmlWriter( KHTMLPart * part,
					    QObject * parent, const char * name )
    : QObject( parent, name ), HtmlWriter(),
      mHtmlPart( part ), mState( Ended ), mHtmlTimer( 0, "mHtmlTimer" )
  {
    assert( part );
    connect( &mHtmlTimer, SIGNAL(timeout()), SLOT(slotWriteNextHtmlChunk()) );
  }

  KHtmlPartHtmlWriter::~KHtmlPartHtmlWriter() {

  }

  void KHtmlPartHtmlWriter::begin( const QString & css ) {
    if ( mState != Ended ) {
      kdWarning( 5006 ) << "KHtmlPartHtmlWriter: begin() called on non-ended session!" << endl;
      reset();
    }

    mEmbeddedPartMap.clear();

    // clear the widget:
    mHtmlPart->view()->setUpdatesEnabled( false );
    mHtmlPart->view()->viewport()->setUpdatesEnabled( false );
    static_cast<QScrollView *>(mHtmlPart->widget())->ensureVisible( 0, 0 );

    mHtmlPart->begin( KURL( "file:/" ) );
    if ( !css.isEmpty() )
      mHtmlPart->setUserStyleSheet( css );
    mState = Begun;
  }

  void KHtmlPartHtmlWriter::end() {
    kdWarning( mState != Begun, 5006 ) << "KHtmlPartHtmlWriter: end() called on non-begun or queued session!" << endl;
    mHtmlPart->end();

    resolveCidUrls();

    mHtmlPart->view()->viewport()->setUpdatesEnabled( true );
    mHtmlPart->view()->setUpdatesEnabled( true );
    mHtmlPart->view()->viewport()->repaint( false );
    mState = Ended;
  }

  void KHtmlPartHtmlWriter::reset() {
    if ( mState != Ended ) {
      mHtmlTimer.stop();
      mHtmlQueue.clear();
      mState = Begun; // don't run into end()'s warning
      end();
    }
    mState = Ended;
  }

  void KHtmlPartHtmlWriter::write( const QString & str ) {
    kdWarning( mState != Begun, 5006 ) << "KHtmlPartHtmlWriter: write() called in Ended or Queued state!" << endl;
    mHtmlPart->write( str );
  }

  void KHtmlPartHtmlWriter::queue( const QString & str ) {
    static const uint chunksize = 16384;
    for ( uint pos = 0 ; pos < str.length() ; pos += chunksize )
      mHtmlQueue.push_back( str.mid( pos, chunksize ) );
    mState = Queued;
  }

  void KHtmlPartHtmlWriter::flush() {
    slotWriteNextHtmlChunk();
  }

  void KHtmlPartHtmlWriter::slotWriteNextHtmlChunk() {
    if ( mHtmlQueue.empty() ) {
      mState = Begun; // don't run into end()'s warning
      end();
    } else {
      mHtmlPart->write( mHtmlQueue.front() );
      mHtmlQueue.pop_front();
      mHtmlTimer.start( 0, true );
    }
  }

  void KHtmlPartHtmlWriter::embedPart( const QCString & contentId,
                                       const QString & contentURL ) {
    mEmbeddedPartMap[QString(contentId)] = contentURL;
  }

  void KHtmlPartHtmlWriter::resolveCidUrls()
  {
    DOM::HTMLDocument document = mHtmlPart->htmlDocument();
    DOM::HTMLCollection images = document.images();
    for ( DOM::Node node = images.firstItem(); !node.isNull(); node = images.nextItem() ) {
      DOM::HTMLImageElement image( node );
      KURL url( image.src().string() );
      if ( url.protocol() == "cid" ) {
        EmbeddedPartMap::const_iterator it = mEmbeddedPartMap.find( url.path() );
        if ( it != mEmbeddedPartMap.end() ) {
          kdDebug(5006) << "Replacing " << url.prettyURL() << " by " << it.data() << endl;
          image.setSrc( it.data() );
        }
      }
    }
  }

} // namespace KMail

#include "khtmlparthtmlwriter.moc"
