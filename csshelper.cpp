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

#include <config.h>

#include "csshelper.h"

#include "kmkernel.h"

#include <kconfig.h>
#include <kglobalsettings.h>
#include <kdebug.h>
#include <kglobal.h>

#include <qcolor.h>
#include <qfont.h>
#include <qstring.h>
#include <qapplication.h>

#include <cassert>

namespace KMail {

  class CSSHelper::Private {
    friend class CSSHelper;
  public:
    Private() {}
    ~Private() {}

    bool operator==( const Private & other ) const;
    bool operator!=( const Private & other ) const {
      return !operator==( other );
    }

    void readColorConfig();

    // returns CSS rules specific to the print media type
    QString printCssDefinitions() const;

    // returns CSS rules specific to the screen media type
    QString screenCssDefinitions( const CSSHelper * helper, bool fixed ) const;

    // returns CSS rules common to both screen and print media types
    QString commonCssDefinitions() const;

    QFont bodyFont( bool fixed, bool print=false ) const {
      return fixed ? mFixedFont : print ? mPrintFont : mBodyFont ;
    }
    int fontSize( bool fixed, bool print=false ) const {
      return bodyFont( fixed, print ).pointSize();
    }

    QString quoteFontTag( int level ) const;

  private:
    QFont mBodyFont, mPrintFont, mFixedFont;
    QFont mQuoteFont[3];
    QColor mQuoteColor[3];
    bool mRecycleQuoteColors;
    bool mBackingPixmapOn;
    QString mBackingPixmapStr;
    QColor c1, c2, c3, c4;
    // colors for PGP (Frame, Header, Body)
    QColor cPgpOk1F, cPgpOk1H, cPgpOk1B,
      cPgpOk0F, cPgpOk0H, cPgpOk0B,
      cPgpWarnF, cPgpWarnH, cPgpWarnB,
      cPgpErrF, cPgpErrH, cPgpErrB,
      cPgpEncrF, cPgpEncrH, cPgpEncrB;
    // color of frame of warning preceding the source of HTML messages
    QColor cHtmlWarning;
  };

  bool CSSHelper::Private::operator==( const Private & other ) const {
    for ( int i = 0 ; i < 3 ; ++i )
      if ( mQuoteFont[i] != other.mQuoteFont[i] ||
	   mQuoteColor[i] != other.mQuoteColor[i] )
	return false;
    return // eeek!
      mBodyFont == other.mBodyFont &&
      mPrintFont == other.mPrintFont &&
      mFixedFont == other.mFixedFont &&
      mRecycleQuoteColors == other.mRecycleQuoteColors &&
      mBackingPixmapOn == other.mBackingPixmapOn &&
      mBackingPixmapStr == other.mBackingPixmapStr &&
      c1 == other.c1 && c2 == other.c2 && c3 == other.c3 && c4 == other.c4 &&
      cHtmlWarning == other.cHtmlWarning &&
      cPgpOk1F == other.cPgpOk1F && cPgpOk1H == other.cPgpOk1H && cPgpOk1B == other.cPgpOk1B &&
      cPgpOk0F == other.cPgpOk0F && cPgpOk0H == other.cPgpOk0H && cPgpOk0B == other.cPgpOk0B &&
      cPgpWarnF == other.cPgpWarnF && cPgpWarnH == other.cPgpWarnH && cPgpWarnB == other.cPgpWarnB &&
      cPgpErrF == other.cPgpErrF && cPgpErrH == other.cPgpErrH && cPgpErrB == other.cPgpErrB &&
      cPgpEncrF == other.cPgpEncrF && cPgpEncrH == other.cPgpEncrH && cPgpEncrB == other.cPgpEncrB ;
    }

  namespace {
    // some QColor manipulators that hide the ugly QColor API w.r.t. HSV:
    inline QColor darker( const QColor & c ) {
      int h, s, v;
      c.hsv( &h, &s, &v );
      return QColor( h, s, v*4/5, QColor::Hsv );
    }

    inline QColor desaturate( const QColor & c ) {
      int h, s, v;
      c.hsv( &h, &s, &v );
      return QColor( h, s/8, v, QColor::Hsv );
    }

    inline QColor fixValue( const QColor & c, int newV ) {
      int h, s, v;
      c.hsv( &h, &s, &v );
      return QColor( h, s, newV, QColor::Hsv );
    }

    inline int getValueOf( const QColor & c ) {
      int h, s, v;
      c.hsv( &h, &s, &v );
      return v;
    }
  }

  void CSSHelper::Private::readColorConfig() {
    KConfig * config = KMKernel::config();

    KConfigGroup reader( config, "Reader" );
    KConfigGroup fonts( config, "Fonts" );
    KConfigGroup pixmaps( config, "Pixmaps" );

    c1 = QApplication::palette().active().text();
    c2 = KGlobalSettings::linkColor();
    c3 = KGlobalSettings::visitedLinkColor();
    c4 = QApplication::palette().active().base();
    cHtmlWarning = QColor( 0xFF, 0x40, 0x40 ); // warning frame color: light red

    // The default colors are also defined in configuredialog.cpp
    cPgpEncrH = QColor( 0x00, 0x80, 0xFF ); // light blue
    cPgpOk1H  = QColor( 0x40, 0xFF, 0x40 ); // light green
    cPgpOk0H  = QColor( 0xFF, 0xFF, 0x40 ); // light yellow
    cPgpWarnH = QColor( 0xFF, 0xFF, 0x40 ); // light yellow
    cPgpErrH  = Qt::red;

    for ( int i = 0 ; i < 3 ; ++i )
      mQuoteColor[i] = QColor( 0x00, 0x80 - i * 0x10, 0x00 ); // shades of green
    mRecycleQuoteColors = reader.readBoolEntry( "RecycleQuoteColors", false );

    if ( !reader.readBoolEntry( "defaultColors", true ) ) {
      c1 = reader.readColorEntry("ForegroundColor",&c1);
      c2 = reader.readColorEntry("LinkColor",&c2);
      c3 = reader.readColorEntry("FollowedColor",&c3);
      c4 = reader.readColorEntry("BackgroundColor",&c4);
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

    // determine the frame and body color for PGP messages from the header color
    // if the header color equals the background color then the other colors are
    // also set to the background color (-> old style PGP message viewing)
    // else
    // the brightness of the frame is set to 4/5 of the brightness of the header
    // and in case of a light background color
    // the saturation of the body is set to 1/8 of the saturation of the header
    // while in case of a dark background color
    // the value of the body is set to the value of the background color

    // Check whether the user uses a light color scheme
    const int vBG = getValueOf( c4 );
    const bool lightBG = vBG >= 128;
    if ( cPgpOk1H == c4 ) {
      cPgpOk1F = c4;
      cPgpOk1B = c4;
    } else {
      cPgpOk1F= darker( cPgpOk1H );
      cPgpOk1B = lightBG ? desaturate( cPgpOk1H ) : fixValue( cPgpOk1H, vBG );
    }
    if ( cPgpOk0H == c4 ) {
      cPgpOk0F = c4;
      cPgpOk0B = c4;
    } else {
      cPgpOk0F = darker( cPgpOk0H );
      cPgpOk0B = lightBG ? desaturate( cPgpOk0H ) : fixValue( cPgpOk0H, vBG );
    }
    if ( cPgpWarnH == c4 ) {
      cPgpWarnF = c4;
      cPgpWarnB = c4;
    } else {
      cPgpWarnF = darker( cPgpWarnH );
      cPgpWarnB = lightBG ? desaturate( cPgpWarnH ) : fixValue( cPgpWarnH, vBG );
    }
    if ( cPgpErrH == c4 ) {
      cPgpErrF = c4;
      cPgpErrB = c4;
    } else {
      cPgpErrF = darker( cPgpErrH );
      cPgpErrB = lightBG ? desaturate( cPgpErrH ) : fixValue( cPgpErrH, vBG );
    }
    if ( cPgpEncrH == c4 ) {
      cPgpEncrF = c4;
      cPgpEncrB = c4;
    } else {
      cPgpEncrF = darker( cPgpEncrH );
      cPgpEncrB = lightBG ? desaturate( cPgpEncrH ) : fixValue( cPgpEncrH, vBG );
    }

    QFont defaultFont = KGlobalSettings::generalFont();
    if ( fonts.readBoolEntry( "defaultFonts", true ) ) {
      mBodyFont = mPrintFont = defaultFont;
      mFixedFont = KGlobalSettings::fixedFont();
      defaultFont.setItalic( true );
      for ( int i = 0 ; i < 3 ; ++i )
	mQuoteFont[i] = defaultFont;
    } else {
      mBodyFont = fonts.readFontEntry(  "body-font",  &defaultFont);
      mPrintFont = fonts.readFontEntry( "print-font", &defaultFont);
      mFixedFont = fonts.readFontEntry( "fixed-font", &defaultFont);
      defaultFont.setItalic( true );
      for ( int i = 0 ; i < 3 ; ++i ) {
	const QString key = QString( "quote%1-font" ).arg( i+1 );
	mQuoteFont[i] = fonts.readFontEntry( key, &defaultFont );
      }
    }

    mBackingPixmapStr = pixmaps.readPathEntry("Readerwin");
    mBackingPixmapOn = !mBackingPixmapStr.isEmpty();
  }

  CSSHelper::CSSHelper( const QPaintDeviceMetrics & pdm, QObject * parent, const char * name )
    : ConfigManager( parent, name ),
      d( 0 ), s( 0 ), mMetrics( pdm )
  {
    d = new Private();
    d->readColorConfig();
  }

  CSSHelper::~CSSHelper() {
    kdWarning( hasPendingChanges(), 5006 )
      << "CSSHelper: There were uncommitted changes!" << endl;
    delete d; d = 0;
    delete s; s = 0;
  }

  void CSSHelper::commit() {
    // not yet implemented
  }

  void CSSHelper::rollback() {
    delete s; s = 0;
  }

  bool CSSHelper::hasPendingChanges() const {
    assert( d );
    return s && *s != *d ;
  }

  QString CSSHelper::cssDefinitions( bool fixed ) const {
    assert( d );
    return
      d->commonCssDefinitions()
      +
      "@media screen {\n\n"
      +
      d->screenCssDefinitions( this, fixed )
      +
      "}\n"
      "@media print {\n\n"
      +
      d->printCssDefinitions()
      +
      "}\n";
  }

  QString CSSHelper::htmlHead( bool fixed ) const {
    return
      "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n"
      "<html><head><title></title></head>\n"
      +
      QString( fixed ? "<body class\"fixedfont\">\n" : "<body>\n" );
  }

  QString CSSHelper::Private::quoteFontTag( int level ) const {
    if ( level < 0 )
      level = 0;
    static const int numQuoteLevels = sizeof mQuoteFont / sizeof *mQuoteFont ;
    const int effectiveLevel = mRecycleQuoteColors
      ? level % numQuoteLevels + 1
      : kMin( level + 1, numQuoteLevels ) ;
    return QString( "<div class=\"quotelevel%1\">" ).arg( effectiveLevel );
  }

  QString CSSHelper::quoteFontTag( int level ) const {
    assert( d );
    return d->quoteFontTag( level );
  }

  QString CSSHelper::nonQuotedFontTag() const {
    return "<div class=\"noquote\">";
  }

  QFont CSSHelper::bodyFont( bool fixed, bool print ) const {
    assert( d );
    return d->bodyFont( fixed, print );
  }

  namespace {
    int pointsToPixel( const QPaintDeviceMetrics & metrics, int pointSize ) {
      return ( pointSize * metrics.logicalDpiY() + 36 ) / 72 ;
    }
  }

  QString CSSHelper::Private::printCssDefinitions() const {
    const QString headerFont = QString( "  font-family: \"%1\" ! important;\n"
					"  font-size: %2pt ! important;\n" )
                           .arg( mPrintFont.family() )
                           .arg( mPrintFont.pointSize() );
    const QColorGroup & cg = QApplication::palette().active();

    QString quoteCSS;
    if ( mPrintFont.italic() )
      quoteCSS += "  font-style: italic ! important;\n";
    if ( mPrintFont.bold() )
      quoteCSS += "  font-weight: bold ! important;\n";
    if ( !quoteCSS.isEmpty() )
      quoteCSS = "div.noquote {\n" + quoteCSS + "}\n\n";

    return
      QString( "body {\n"
	       "  font-family: \"%1\" ! important;\n"
	       "  font-size: %2pt ! important;\n"
	       "  color: #000000 ! important;\n"
	       "  background-color: #ffffff ! important\n"
	       "}\n\n" )
      .arg( mPrintFont.family(),
	    QString::number( mPrintFont.pointSize() ) )
      +
      QString( "tr.textAtmH,\n"
	       "tr.rfc822H,\n"
	       "tr.encrH,\n"
	       "tr.signOkKeyOkH,\n"
	       "tr.signOkKeyBadH,\n"
	       "tr.signWarnH,\n"
	       "tr.signErrH,\n"
	       "div.header {\n"
	       "%1"
	       "}\n\n"

	       "div.fancy.header > div {\n"
	       "  background-color: %2 ! important;\n"
	       "  color: %3 ! important;\n"
	       "  padding: 4px ! important;\n"
	       "  border: solid %3 1px ! important;\n"
	       "}\n\n"

	       "div.fancy.header > div a[href] { color: %3 ! important; }\n\n"

	       "div.fancy.header > table.outer{\n"
	       "  background-color: %2 ! important;\n"
	       "  color: %3 ! important;\n"
	       "  border-bottom: solid %3 1px ! important;\n"
	       "  border-left: solid %3 1px ! important;\n"
	       "  border-right: solid %3 1px ! important;\n"
	       "}\n\n"

               "div.spamheader {\n"
               "  display:none ! important;\n"
               "}\n\n"

	       "div.htmlWarn {\n"
	       "  border: 2px solid #ffffff ! important;\n"
	       "}\n\n"

               "div.senderpic{\n"
               "  font-size:0.8em ! important;\n"
               "  border:1px solid black ! important;\n"
               "  background-color:%2 ! important;\n"
               "}\n\n"

               "div.senderstatus{\n"
               "  text-align:center ! important;\n"
               "}\n\n"
            )
      .arg( headerFont,
	    cg.background().name(),
	    cg.foreground().name() )
      + quoteCSS;
  }

  QString CSSHelper::Private::screenCssDefinitions( const CSSHelper * helper, bool fixed ) const {
    const QString fgColor = c1.name();
    const QString bgColor = c4.name();
    const QString linkColor = c2.name();
    const QString headerFont = QString("  font-family: \"%1\" ! important;\n"
				       "  font-size: %2px ! important;\n")
      .arg( mBodyFont.family() )
      .arg( pointsToPixel( helper->mMetrics, mBodyFont.pointSize() ) );
    const QString background = ( mBackingPixmapOn
                         ? QString( "  background-image:url(file://%1) ! important;\n" )
                           .arg( mBackingPixmapStr )
                         : QString( "  background-color: %1 ! important;\n" )
                           .arg( bgColor ) );
    const QString bodyFontSize = QString::number( pointsToPixel( helper->mMetrics, fontSize( fixed ) ) ) + "px" ;
    const QColorGroup & cg = QApplication::palette().active();

    QString quoteCSS;
    if ( bodyFont( fixed ).italic() )
      quoteCSS += "  font-style: italic ! important;\n";
    if ( bodyFont( fixed ).bold() )
      quoteCSS += "  font-weight: bold ! important;\n";
    if ( !quoteCSS.isEmpty() )
      quoteCSS = "div.noquote {\n" + quoteCSS + "}\n\n";

    for ( int i = 0 ; i < 3 ; ++i ) {
      quoteCSS += QString( "div.quotelevel%1 {\n"
			   "  color: %2 ! important;\n" )
	.arg( QString::number(i+1), mQuoteColor[i].name() );
      if ( mQuoteFont[i].italic() )
	quoteCSS += "  font-style: italic ! important;\n";
      if ( mQuoteFont[i].bold() )
	quoteCSS += "  font-weight: bold ! important;\n";
      quoteCSS += "}\n\n";
    }

    return
      QString( "body {\n"
	       "  font-family: \"%1\" ! important;\n"
	       "  font-size: %2 ! important;\n"
	       "  color: %3 ! important;\n"
	       "%4"
	       "}\n\n" )
      .arg( bodyFont( fixed ).family(),
	    bodyFontSize,
	    fgColor,
	    background )
      +
      QString( "a {\n"
	       "  color: %1 ! important;\n"
	       "  text-decoration: none ! important;\n"
	       "}\n\n"

	       "table.textAtm { background-color: %2 ! important; }\n\n"

	       "tr.textAtmH {\n"
	       "  background-color: %3 ! important;\n"
	       "%4"
	       "}\n\n"

	       "tr.textAtmB {\n"
	       "  background-color: %3 ! important;\n"
	       "}\n\n"

	       "table.rfc822 {\n"
	       "  background-color: %3 ! important;\n"
	       "}\n\n"

	       "tr.rfc822H {\n"
	       "%4"
	       "}\n\n" )
      .arg( linkColor, fgColor, bgColor, headerFont )
      +
      QString( "table.encr {\n"
	       "  background-color: %1 ! important;\n"
	       "}\n\n"

	       "tr.encrH {\n"
	       "  background-color: %2 ! important;\n"
	       "%3"
	       "}\n\n"

	       "tr.encrB { background-color: %4 ! important; }\n\n" )
      .arg( cPgpEncrF.name(),
	    cPgpEncrH.name(),
	    headerFont,
	    cPgpEncrB.name() )
      +
      QString( "table.signOkKeyOk {\n"
	       "  background-color: %1 ! important;\n"
	       "}\n\n"

	       "tr.signOkKeyOkH {\n"
	       "  background-color: %2 ! important;\n"
	       "%3"
	       "}\n\n"

	       "tr.signOkKeyOkB { background-color: %4 ! important; }\n\n" )
      .arg( cPgpOk1F.name(),
	    cPgpOk1H.name(),
	    headerFont,
	    cPgpOk1B.name() )
      +
      QString( "table.signOkKeyBad {\n"
	       "  background-color: %1 ! important;\n"
	       "}\n\n"

	       "tr.signOkKeyBadH {\n"
	       "  background-color: %2 ! important;\n"
	       "%3"
	       "}\n\n"

	       "tr.signOkKeyBadB { background-color: %4 ! important; }\n\n" )
      .arg( cPgpOk0F.name(),
	    cPgpOk0H.name(),
	    headerFont,
	    cPgpOk0B.name() )
      +
      QString( "table.signWarn {\n"
	       "  background-color: %1 ! important;\n"
	       "}\n\n"

	       "tr.signWarnH {\n"
	       "  background-color: %2 ! important;\n"
	       "%3"
	       "}\n\n"

	       "tr.signWarnB { background-color: %4 ! important; }\n\n" )
      .arg( cPgpWarnF.name(),
	    cPgpWarnH.name(),
	    headerFont,
	    cPgpWarnB.name() )
      +
      QString( "table.signErr {\n"
	       "  background-color: %1 ! important;\n"
	       "}\n\n"

	       "tr.signErrH {\n"
	       "  background-color: %2 ! important;\n"
	       "%3"
	       "}\n\n"

	       "tr.signErrB { background-color: %4 ! important; }\n\n" )
      .arg( cPgpErrF.name(),
	    cPgpErrH.name(),
	    headerFont,
	    cPgpErrB.name() )
      +
      QString( "div.htmlWarn {\n"
	       "  border: 2px solid %1 ! important;\n"
	       "}\n\n" )
      .arg( cHtmlWarning.name() )
      +
      QString( "div.header {\n"
	       "%1"
	       "}\n\n"

	       "div.fancy.header > div {\n"
	       "  background-color: %2 ! important;\n"
	       "  color: %3 ! important;\n"
	       "  border: solid %4 1px ! important;\n"
	       "}\n\n"

	       "div.fancy.header > div a[href] { color: %3 ! important; }\n\n"

	       "div.fancy.header > div a[href]:hover { text-decoration: underline ! important; }\n\n"

               "div.fancy.header > div.spamheader {\n"
               "  background-color: #cdcdcd ! important;\n"
               "  border-top: 0px ! important;\n"
               "  padding: 3px ! important;\n"
               "  color: black ! important;\n"
               "  font-weight: bold ! important;\n"
               "  font-size: smaller ! important;\n"
               "}\n\n"

	       "div.fancy.header > table.outer {\n"
	       "  background-color: %5 ! important;\n"
	       "  color: %4 ! important;\n"
	       "  border-bottom: solid %4 1px ! important;\n"
	       "  border-left: solid %4 1px ! important;\n"
	       "  border-right: solid %4 1px ! important;\n"
	       "}\n\n"

               "div.senderpic{\n"
               "  padding: 0px ! important;\n"
               "  font-size:0.8em ! important;\n"
               "  border:1px solid black ! important;\n"
               // FIXME: InfoBackground crashes KHTML
               //"  background-color:InfoBackground ! important;\n"
               "  background-color:%5 ! important;\n"
               "}\n\n"

               "div.senderstatus{\n"
               "  text-align:center ! important;\n"
               "}\n\n"
               )

      .arg( headerFont )
      .arg( cg.highlight().name(),
	    cg.highlightedText().name(),
	    cg.foreground().name(),
	    cg.background().name() )
      + quoteCSS;
  }

  QString CSSHelper::Private::commonCssDefinitions() const {
    return
      "div.header {\n"
      "  margin-bottom: 10pt ! important;\n"
      "}\n\n"

      "table.textAtm {\n"
      "  margin-top: 10pt ! important;\n"
      "  margin-bottom: 10pt ! important;\n"
      "}\n\n"

      "tr.textAtmH,\n"
      "tr.textAtmB,\n"
      "tr.rfc822B {\n"
      "  font-weight: normal ! important;\n"
      "}\n\n"

      "tr.rfc822H,\n"
      "tr.encrH,\n"
      "tr.signOkKeyOkH,\n"
      "tr.signOkKeyBadH,\n"
      "tr.signWarnH,\n"
      "tr.signErrH {\n"
      "  font-weight: bold ! important;\n"
      "}\n\n"

      "tr.textAtmH td,\n"
      "tr.textAtmB td {\n"
      "  padding: 3px ! important;\n"
      "}\n\n"

      "table.rfc822 {\n"
      "  width: 100% ! important;\n"
      "  border: solid 1px black ! important;\n"
      "  margin-top: 10pt ! important;\n"
      "  margin-bottom: 10pt ! important;\n"
      "}\n\n"

      "table.textAtm,\n"
      "table.encr,\n"
      "table.signWarn,\n"
      "table.signErr,\n"
      "table.signOkKeyBad,\n"
      "table.signOkKeyOk,\n"
      "div.fancy.header table {\n"
      "  width: 100% ! important;\n"
      "  border-width: 0px ! important;\n"
      "}\n\n"

      "div.htmlWarn {\n"
      "  margin: 0px 5% ! important;\n"
      "  padding: 10px ! important;\n"
      "  text-align: left ! important;\n"
      "}\n\n"

      "div.fancy.header > div {\n"
      "  font-weight: bold ! important;\n"
      "  padding: 4px ! important;\n"
      "}\n\n"

      "div.fancy.header table {\n"
      "  padding: 2px ! important;\n" // ### khtml bug: this is ignored
      "  text-align: left ! important\n"
      "}\n\n"

      "div.fancy.header table th {\n"
      "  padding: 0px ! important;\n"
      "  white-space: nowrap ! important;\n"
      "  border-spacing: 0px ! important;\n"
      "  text-align: left ! important;\n"
      "  vertical-align: top ! important;\n"
      "}\n\n"

      "div.fancy.header table td {\n"
      "  padding: 0px ! important;\n"
      "  border-spacing: 0px ! important;\n"
      "  text-align: left ! important;\n"
      "  vertical-align: top ! important;\n"
      "  width: 100% ! important;\n"
      "}\n\n"
      ;
  }

} // namespace KMail

