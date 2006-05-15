/*  -*- c++ -*-
    htmlstatusbar.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2002 Ingo Kloecker <kloecker@kde.org>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "htmlstatusbar.h"

#ifndef KMAIL_TESTING
#include "kmkernel.h"
#else
#include <kapplication.h>
#endif

#include <klocale.h>
#include <kconfig.h>

#include <qcolor.h>
#include <qstring.h>

KMail::HtmlStatusBar::HtmlStatusBar( QWidget * parent, const char * name, WFlags f )
  : QLabel( parent, name, f ),
    mMode( Normal )
{
  setAlignment( AlignHCenter|AlignTop );
  // Don't force a minimum height to the reader widget
  setSizePolicy( QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Ignored ) );
  upd();
}

KMail::HtmlStatusBar::~HtmlStatusBar() {}

void KMail::HtmlStatusBar::upd() {
  setEraseColor( bgColor() );
  setPaletteForegroundColor( fgColor() );
  setText( message() );
}

void KMail::HtmlStatusBar::setNormalMode() {
  setMode( Normal );
}

void KMail::HtmlStatusBar::setHtmlMode() {
  setMode( Html );
}

void KMail::HtmlStatusBar::setNeutralMode() {
  setMode( Neutral );
}

void KMail::HtmlStatusBar::setMode( Mode m ) {
  if ( m == mode() )
    return;
  mMode = m;
  upd();
}

QString KMail::HtmlStatusBar::message() const {
  switch ( mode() ) {
  case Html: // bold: "HTML Message"
    return i18n( "<qt><b><br>H<br>T<br>M<br>L<br> "
		 "<br>M<br>e<br>s<br>s<br>a<br>g<br>e</b></qt>" );
  case Normal: // normal: "No HTML Message"
    return i18n( "<qt><br>N<br>o<br> "
		 "<br>H<br>T<br>M<br>L<br> "
		 "<br>M<br>e<br>s<br>s<br>a<br>g<br>e</qt>" );
  default:
  case Neutral:
    return QString::null;
  }
}

namespace {
  inline KConfig * config() {
#ifndef KMAIL_TESTING
    return KMKernel::config();
#else
    return kApp->config();
#endif
  }
}

QColor KMail::HtmlStatusBar::fgColor() const {
  KConfigGroup conf( config(), "Reader" );
  switch ( mode() ) {
  case Html:
    return conf.readColorEntry( "ColorbarForegroundHTML", &Qt::white );
  case Normal:
    return conf.readColorEntry( "ColorbarForegroundPlain", &Qt::black );
  default:
  case Neutral:
    return Qt::black;
  }
}

QColor KMail::HtmlStatusBar::bgColor() const {
  KConfigGroup conf( config(), "Reader" );

  switch ( mode() ) {
  case Html:
    return conf.readColorEntry( "ColorbarBackgroundHTML", &Qt::black );
  case Normal:
    return conf.readColorEntry( "ColorbarBackgroundPlain", &Qt::lightGray );
  default:
  case Neutral:
    return Qt::white;
  }
}

#include "htmlstatusbar.moc"
