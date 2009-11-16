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

#include "htmlstatusbar.h"

#ifndef KMAIL_TESTING
#include "kmkernel.h"
#else
#endif

#include <klocale.h>
#include <kconfig.h>
#include <kconfiggroup.h>

#include <QColor>
#include <QString>
#include <QLabel>
#include <QMouseEvent>

KMail::HtmlStatusBar::HtmlStatusBar( QWidget * parent, const char * name, Qt::WFlags f )
  : QLabel( parent, f ),
    mMode( Normal )
{
  setObjectName( name );
  setAlignment( Qt::AlignHCenter | Qt::AlignTop );
  // Don't force a minimum height to the reader widget
  setSizePolicy( QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Ignored ) );
  setAutoFillBackground( true );
  update();
}

KMail::HtmlStatusBar::~HtmlStatusBar() {}

void KMail::HtmlStatusBar::update() {
  QPalette pal = palette();
  pal.setColor( backgroundRole(), bgColor() );
  pal.setColor( foregroundRole(), fgColor() );
  setPalette( pal );
  setText( message() );
  setToolTip( toolTip() );
}

void KMail::HtmlStatusBar::setNormalMode() {
  setMode( Normal );
}

void KMail::HtmlStatusBar::setHtmlMode() {
  setMode( Html );
}

void KMail::HtmlStatusBar::setMultipartPlainMode()
{
  setMode( MultipartPlain );
}

void KMail::HtmlStatusBar::setMultipartHtmlMode()
{
  setMode( MultipartHtml );
}

void KMail::HtmlStatusBar::setMode( Mode m ) {
  if ( m == mode() )
    return;
  mMode = m;
  update();
}

void KMail::HtmlStatusBar::mousePressEvent( QMouseEvent * event )
{
  if ( event->button() == Qt::LeftButton )
  {
    emit clicked();
  }
}

QString KMail::HtmlStatusBar::message() const {
  switch ( mode() ) {
  case Html: // bold: "HTML Message"
  case MultipartHtml:
    return i18n( "<qt><b><br />H<br />T<br />M<br />L<br /> "
                 "<br />M<br />e<br />s<br />s<br />a<br />g<br />e</b></qt>" );
  case Normal: // normal: "No HTML Message"
    return i18n( "<qt><br />N<br />o<br /> "
                 "<br />H<br />T<br />M<br />L<br /> "
                 "<br />M<br />e<br />s<br />s<br />a<br />g<br />e</qt>" );
  case MultipartPlain: // normal: "Plain Message"
    return i18n( "<qt><br />P<br />l<br />a<br />i<br />n<br /> "
                 "<br />M<br />e<br />s<br />s<br />a<br />g<br />e<br /></qt>" );
  default:
    return QString();
  }
}

QString KMail::HtmlStatusBar::toolTip() const
{
  switch ( mode() )
  {
    case Html:
    case MultipartHtml:
    case MultipartPlain:
      return i18n( "Click to toggle between HTML and plain text." );
    default:
    case Normal:
      break;
  }

  return QString();
}

QColor KMail::HtmlStatusBar::fgColor() const {
  KConfigGroup conf( KMKernel::config(), "Reader" );
  QColor defaultColor, color;
  switch ( mode() ) {
  case Html:
  case MultipartHtml:
    defaultColor = Qt::white;
    color = defaultColor;
    if ( !GlobalSettings::self()->useDefaultColors() ) {
      color = conf.readEntry( "ColorbarForegroundHTML", defaultColor );
    }
    return color;
  case Normal:
  case MultipartPlain:
    defaultColor = Qt::black;
    color = defaultColor;
    if ( !GlobalSettings::self()->useDefaultColors() ) {
      color = conf.readEntry( "ColorbarForegroundPlain", defaultColor );
    }
    return color;
  default:
    return Qt::black;
  }
}

QColor KMail::HtmlStatusBar::bgColor() const {
  KConfigGroup conf( KMKernel::config(), "Reader" );

  QColor defaultColor, color;
  switch ( mode() ) {
  case Html:
  case MultipartHtml:
    defaultColor = Qt::black;
    color = defaultColor;
    if ( !GlobalSettings::self()->useDefaultColors() ) {
      color = conf.readEntry( "ColorbarBackgroundHTML", defaultColor );
    }
    return color;
  case Normal:
  case MultipartPlain:
    defaultColor = Qt::lightGray;
    color = defaultColor;
    if ( !GlobalSettings::self()->useDefaultColors() ) {
      color = conf.readEntry( "ColorbarBackgroundPlain", defaultColor );
    }
    return color;
  default:
    return Qt::white;
  }
}

#include "htmlstatusbar.moc"
