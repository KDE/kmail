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

#include "csshelper.h"
#include "kmkernel.h"

#include "globalsettings.h"

#include <kconfig.h>

#include <qcolor.h>
#include <qfont.h>

namespace KMail {

  CSSHelper::CSSHelper( const QPaintDeviceMetrics &pdm ) :
    KPIM::CSSHelper( pdm )
  {
    KConfig * config = KMKernel::config();

    KConfigGroup reader( config, "Reader" );
    KConfigGroup fonts( config, "Fonts" );
    KConfigGroup pixmaps( config, "Pixmaps" );

    mRecycleQuoteColors = reader.readBoolEntry( "RecycleQuoteColors", false );

    if ( !reader.readBoolEntry( "defaultColors", true ) ) {
      mForegroundColor = reader.readColorEntry("ForegroundColor",&mForegroundColor);
      mLinkColor = reader.readColorEntry("LinkColor",&mLinkColor);
      mVisitedLinkColor = reader.readColorEntry("FollowedColor",&mVisitedLinkColor);
      mBackgroundColor = reader.readColorEntry("BackgroundColor",&mBackgroundColor);
      cPgpEncrH = reader.readColorEntry( "PGPMessageEncr", &cPgpEncrH );
      cPgpOk1H  = reader.readColorEntry( "PGPMessageOkKeyOk", &cPgpOk1H );
      cPgpOk0H  = reader.readColorEntry( "PGPMessageOkKeyBad", &cPgpOk0H );
      cPgpWarnH = reader.readColorEntry( "PGPMessageWarn", &cPgpWarnH );
      cPgpErrH  = reader.readColorEntry( "PGPMessageErr", &cPgpErrH );
      cHtmlWarning = reader.readColorEntry( "HTMLWarningColor", &cHtmlWarning );
      for ( int i = 0 ; i < 3 ; ++i ) {
        const QString key = "QuotedText" + QString::number( i+1 );
        mQuoteColor[i] = reader.readColorEntry( key, &mQuoteColor[i] );
      }
    }

    if ( !fonts.readBoolEntry( "defaultFonts", true ) ) {
      mBodyFont = fonts.readFontEntry(  "body-font",  &mBodyFont);
      mPrintFont = fonts.readFontEntry( "print-font", &mPrintFont);
      mFixedFont = fonts.readFontEntry( "fixed-font", &mFixedFont);
      mFixedPrintFont = mFixedFont; // FIXME when we have a separate fixed print font
      QFont defaultFont = mBodyFont;
      defaultFont.setItalic( true );
      for ( int i = 0 ; i < 3 ; ++i ) {
        const QString key = QString( "quote%1-font" ).arg( i+1 );
        mQuoteFont[i] = fonts.readFontEntry( key, &defaultFont );
      }
    }

    mShrinkQuotes = GlobalSettings::self()->shrinkQuotes();

    mBackingPixmapStr = pixmaps.readPathEntry("Readerwin");
    mBackingPixmapOn = !mBackingPixmapStr.isEmpty();

    recalculatePGPColors();
  }
} // namespace KMail

