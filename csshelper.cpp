/*  -*- mode: C++; c-file-style: "gnu" -*-
    csshelper.cpp

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

#include "config.h"

#include "csshelper.h"
#include "kmkernel.h"

#include "globalsettings.h"

#include <kconfig.h>
#include <kconfiggroup.h>

#include <QColor>
#include <QFont>

namespace KMail {

  CSSHelper::CSSHelper( const QPaintDevice *pd ) :
    KPIM::CSSHelper( pd )
  {
    KConfig * config = KMKernel::config();

    KConfigGroup reader( config, "Reader" );
    KConfigGroup fonts( config, "Fonts" );
    KConfigGroup pixmaps( config, "Pixmaps" );

    mRecycleQuoteColors = reader.readEntry( "RecycleQuoteColors", false );

    if ( !reader.readEntry( "defaultColors", true ) ) {
      mForegroundColor =
          reader.readEntry( "ForegroundColor", QVariant( &mForegroundColor ) ).value<QColor>();
      mLinkColor =
          reader.readEntry( "LinkColor", QVariant( &mLinkColor ) ).value<QColor>();
      mVisitedLinkColor =
          reader.readEntry( "FollowedColor", QVariant( &mVisitedLinkColor ) ).value<QColor>();
      mBackgroundColor =
          reader.readEntry( "BackgroundColor", QVariant( &mBackgroundColor ) ).value<QColor>();
      cPgpEncrH =
          reader.readEntry( "PGPMessageEncr", QVariant( &cPgpEncrH ) ).value<QColor>();
      cPgpOk1H  =
          reader.readEntry( "PGPMessageOkKeyOk", QVariant( &cPgpOk1H ) ).value<QColor>();
      cPgpOk0H  =
          reader.readEntry( "PGPMessageOkKeyBad", QVariant( &cPgpOk0H ) ).value<QColor>();
      cPgpWarnH =
          reader.readEntry( "PGPMessageWarn", QVariant( &cPgpWarnH ) ).value<QColor>();
      cPgpErrH  =
          reader.readEntry( "PGPMessageErr", QVariant( &cPgpErrH ) ).value<QColor>();
      cHtmlWarning =
          reader.readEntry( "HTMLWarningColor", QVariant( &cHtmlWarning ) ).value<QColor>();
      for ( int i = 0 ; i < 3 ; ++i ) {
        const QString key = "QuotedText" + QString::number( i+1 );
        mQuoteColor[i] = reader.readEntry( key, QVariant( &mQuoteColor[i] ) ).value<QColor>();
      }
    }

    if ( !fonts.readEntry( "defaultFonts", true ) ) {
      mBodyFont = fonts.readEntry(  "body-font",  QVariant( &mBodyFont ) ).value<QFont>();
      mPrintFont = fonts.readEntry( "print-font", QVariant( &mPrintFont ) ).value<QFont>();
      mFixedFont = fonts.readEntry( "fixed-font", QVariant( &mFixedFont ) ).value<QFont>();
      mFixedPrintFont = mFixedFont; // FIXME when we have a separate fixed print font
      QFont defaultFont = mBodyFont;
      defaultFont.setItalic( true );
      for ( int i = 0 ; i < 3 ; ++i ) {
        const QString key = QString( "quote%1-font" ).arg( i+1 );
        mQuoteFont[i] = fonts.readEntry( key, QVariant( defaultFont ) ).value<QFont>();
      }
    }

    mShrinkQuotes = GlobalSettings::self()->shrinkQuotes();

    mBackingPixmapStr = pixmaps.readPathEntry("Readerwin");
    mBackingPixmapOn = !mBackingPixmapStr.isEmpty();

    recalculatePGPColors();
  }
} // namespace KMail

