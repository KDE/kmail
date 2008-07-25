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

    mForegroundColor = KColorScheme( QPalette::Active ).foreground().color();
    mBackgroundColor = KColorScheme( QPalette::Active ).background().color();
    if ( !reader.readEntry( "defaultColors", true ) ) {
      mLinkColor =
        reader.readEntry( "LinkColor", mLinkColor );
      mVisitedLinkColor =
        reader.readEntry( "FollowedColor", mVisitedLinkColor );
      cPgpEncrH =
        reader.readEntry( "PGPMessageEncr", cPgpEncrH );
      cPgpOk1H  =
        reader.readEntry( "PGPMessageOkKeyOk", cPgpOk1H );
      cPgpOk0H  =
        reader.readEntry( "PGPMessageOkKeyBad", cPgpOk0H );
      cPgpWarnH =
        reader.readEntry( "PGPMessageWarn", cPgpWarnH );
      cPgpErrH  =
        reader.readEntry( "PGPMessageErr", cPgpErrH );
      cHtmlWarning =
        reader.readEntry( "HTMLWarningColor", cHtmlWarning );
      for ( int i = 0 ; i < 3 ; ++i ) {
        const QString key = "QuotedText" + QString::number( i+1 );
        mQuoteColor[i] = reader.readEntry( key, mQuoteColor[i] );
      }
    }

    if ( !fonts.readEntry( "defaultFonts", true ) ) {
      mBodyFont = fonts.readEntry(  "body-font",  mBodyFont );
      mPrintFont = fonts.readEntry( "print-font", mPrintFont );
      mFixedFont = fonts.readEntry( "fixed-font", mFixedFont );
      mFixedPrintFont = mFixedFont; // FIXME when we have a separate fixed print font
      QFont defaultFont = mBodyFont;
      defaultFont.setItalic( true );
      for ( int i = 0 ; i < 3 ; ++i ) {
        const QString key = QString( "quote%1-font" ).arg( i+1 );
        mQuoteFont[i] = fonts.readEntry( key, defaultFont );
      }
    }

    mShrinkQuotes = GlobalSettings::self()->shrinkQuotes();

    mBackingPixmapStr = pixmaps.readPathEntry("Readerwin", QString());
    mBackingPixmapOn = !mBackingPixmapStr.isEmpty();

    recalculatePGPColors();
  }
} // namespace KMail

