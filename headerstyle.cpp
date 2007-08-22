/*  -*- c++ -*-
    headerstyle.cpp

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

#include "headerstyle.h"

#include "headerstrategy.h"
#include "kmkernel.h"
#include <kpimutils/linklocator.h>
using KPIMUtils::LinkLocator;
#include "kmmessage.h"
#include "spamheaderanalyzer.h"
#include "globalsettings.h"

#include <kpimutils/email.h>
#include <libkdepim/kxface.h>
using namespace KPIM;

#include <mimelib/string.h>
#include <mimelib/field.h>
#include <mimelib/headers.h>

#include <kdebug.h>
#include <kconfiggroup.h>
#include <klocale.h>
#include <kglobal.h>
#include <kimproxy.h>
#include <kabc/stdaddressbook.h>
#include <kabc/addresseelist.h>
#include <kcodecs.h>
#include <QDateTime>
#include <QBuffer>
#include <QBitmap>
#include <QImage>
#include <QApplication>
#include <QRegExp>

namespace KMail {

  //
  // Convenience functions:
  //
  static inline QString directionOf( const QString & str ) {
    return str.isRightToLeft() ? "rtl" : "ltr" ;
  }

#if 0
  // Converts to html. Changes URLs into href's, escapes HTML special
  // chars and inserts the result into an <div> or <span> tag with
  // "dir" set to "rtl" or "ltr" depending on the direction of @p str.
  static QString convertToHtmlBlock( const QString & str, bool useSpan=false ) {
    QString dir = directionOf( str );
    QString format = "<%1 dir=\"%3\">%4</%2>";
    return format.arg( useSpan ? "span" : "div" )
                 .arg( useSpan ? "span" : "div" )
                 .arg( dir )
                 .arg( LinkLocator::convertToHtml( str ) );
  }
#endif

  // ### tmp wrapper to make kmreaderwin code working:
  static QString strToHtml( const QString & str,
                            int flags = LinkLocator::PreserveSpaces ) {
    return LinkLocator::convertToHtml( str, flags );
  }

  //
  // BriefHeaderStyle
  //   Show everything in a single line, don't show header field names.
  //

  class BriefHeaderStyle : public HeaderStyle {
    friend class ::KMail::HeaderStyle;
  protected:
    BriefHeaderStyle() : HeaderStyle() {}
    virtual ~BriefHeaderStyle() {}

  public:
    const char * name() const { return "brief"; }
    const HeaderStyle * next() const { return plain(); }
    const HeaderStyle * prev() const { return fancy(); }

    QString format( const KMMessage * message, const HeaderStrategy * strategy,
                    const QString & vCardName, bool printing ) const;
  };

  QString BriefHeaderStyle::format( const KMMessage * message,
                                    const HeaderStrategy * strategy,
                                    const QString & vCardName, bool printing ) const {
    if ( !message ) return QString();
    if ( !strategy )
      strategy = HeaderStrategy::brief();

    // The direction of the header is determined according to the direction
    // of the application layout.

    QString dir = QApplication::isRightToLeft() ? "rtl" : "ltr" ;

    // However, the direction of the message subject within the header is
    // determined according to the contents of the subject itself. Since
    // the "Re:" and "Fwd:" prefixes would always cause the subject to be
    // considered left-to-right, they are ignored when determining its
    // direction.

    QString subjectDir;
    if (!message->subject().isEmpty())
      subjectDir = directionOf( message->cleanSubject() );
    else
      subjectDir = directionOf( i18n("No Subject") );

    // Prepare the date string (when printing always use the localized date)
    QString dateString;
    if( printing ) {
      QDateTime dateTime;
      KLocale * locale = KGlobal::locale();
      dateTime.setTime_t( message->date() );
      dateString = locale->formatDateTime( dateTime );
    } else {
      dateString = message->dateStr();
    }

    QString headerStr = "<div class=\"header\" dir=\"" + dir + "\">\n";

    if ( strategy->showHeader( "subject" ) )
      headerStr += "<div dir=\"" + subjectDir + "\">\n"
                   "<b style=\"font-size:130%\">" +
                           strToHtml( message->subject() ) +
                           "</b></div>\n";

    QStringList headerParts;

    if ( strategy->showHeader( "from" ) ) {
      QString fromStr = message->from();
      if ( fromStr.isEmpty() ) // no valid email in from, maybe just a name
        fromStr = message->fromStrip(); // let's use that
      QString fromPart = KMMessage::emailAddrAsAnchor( fromStr, true );
      if ( !vCardName.isEmpty() )
        fromPart += "&nbsp;&nbsp;<a href=\"" + vCardName + "\">" + i18n("[vCard]") + "</a>";
      headerParts << fromPart;
    }

    if ( strategy->showHeader( "cc" ) && !message->cc().isEmpty() )
      headerParts << i18n("CC: ") + KMMessage::emailAddrAsAnchor( message->cc(), true );

    if ( strategy->showHeader( "bcc" ) && !message->bcc().isEmpty() )
      headerParts << i18n("BCC: ") + KMMessage::emailAddrAsAnchor( message->bcc(), true );

    if ( strategy->showHeader( "date" ) )
      headerParts << strToHtml(message->dateShortStr());

    // remove all empty (modulo whitespace) entries and joins them via ", \n"
    headerStr += " (" + headerParts.filter( QRegExp( "\\S" ) ).join( ",\n" ) + ')';

    headerStr += "</div>\n";

    // ### iterate over the rest of strategy->headerToDisplay() (or
    // ### all headers if DefaultPolicy == Display) (elsewhere, too)
    return headerStr;
  }

  //
  // PlainHeaderStyle:
  //   show every header field on a line by itself,
  //   show subject larger
  //

  class PlainHeaderStyle : public HeaderStyle {
    friend class ::KMail::HeaderStyle;
  protected:
    PlainHeaderStyle() : HeaderStyle() {}
    virtual ~PlainHeaderStyle() {}

  public:
    const char * name() const { return "plain"; }
    const HeaderStyle * next() const { return fancy(); }
    const HeaderStyle * prev() const { return brief(); }

    QString format( const KMMessage * message, const HeaderStrategy * strategy,
                    const QString & vCardName, bool printing ) const;

  private:
    QString formatAllMessageHeaders( const KMMessage * message ) const;
  };

  QString PlainHeaderStyle::format( const KMMessage * message,
                                    const HeaderStrategy * strategy,
                                    const QString & vCardName, bool printing ) const {
    if ( !message ) return QString();
    if ( !strategy )
      strategy = HeaderStrategy::rich();

    // The direction of the header is determined according to the direction
    // of the application layout.

    QString dir = ( QApplication::isRightToLeft() ? "rtl" : "ltr" );

    // However, the direction of the message subject within the header is
    // determined according to the contents of the subject itself. Since
    // the "Re:" and "Fwd:" prefixes would always cause the subject to be
    // considered left-to-right, they are ignored when determining its
    // direction.

    QString subjectDir;
    if (!message->subject().isEmpty())
      subjectDir = directionOf( message->cleanSubject() );
    else
      subjectDir = directionOf( i18n("No Subject") );

    // Prepare the date string (when printing always use the localized date)
    QString dateString;
    if( printing ) {
      QDateTime dateTime;
      KLocale* locale = KGlobal::locale();
      dateTime.setTime_t( message->date() );
      dateString = locale->formatDateTime( dateTime );
    }
    else {
      dateString = message->dateStr();
    }

    QString headerStr;

    if ( strategy->headersToDisplay().isEmpty()
         && strategy->defaultPolicy() == HeaderStrategy::Display ) {
      // crude way to emulate "all" headers - Note: no strings have
      // i18n(), so direction should always be ltr.
      headerStr= QString("<div class=\"header\" dir=\"ltr\">");
      headerStr += formatAllMessageHeaders( message );
      return headerStr + "</div>";
    }

    headerStr = QString("<div class=\"header\" dir=\"%1\">").arg(dir);

    //case HdrLong:
    if ( strategy->showHeader( "subject" ) )
      headerStr += QString("<div dir=\"%1\"><b style=\"font-size:130%\">" +
                           strToHtml(message->subject()) + "</b></div>\n")
                        .arg(subjectDir);

    if ( strategy->showHeader( "date" ) )
      headerStr.append(i18n("Date: ") + strToHtml(dateString)+"<br>\n");

#if 0
    // Get Instant Messaging presence
    QString presence;
    QString kabcUid;
    if ( strategy->showHeader( "status" ) )
    {
      KABC::AddressBook *addressBook = KABC::StdAddressBook::self( true );
      KABC::AddresseeList addresses = addressBook->findByEmail( KPIM::getFirstEmailAddress( message->from() ) );
      ::KIMProxy *imProxy = KMKernel::self()->imProxy();
      kabcUid = addresses[0].uid();
      presence = imProxy->presenceString( kabcUid );
    }
#endif

    if ( strategy->showHeader( "from" ) ) {
      QString fromStr = message->from();
      if ( fromStr.isEmpty() ) // no valid email in from, maybe just a name
        fromStr = message->fromStrip(); // let's use that
      headerStr.append(i18n("From: ") +
          KMMessage::emailAddrAsAnchor( fromStr, false) );
      if ( !vCardName.isEmpty() )
        headerStr.append("&nbsp;&nbsp;<a href=\"" + vCardName +
              "\">" + i18n("[vCard]") + "</a>" );
#if 0
      if ( !presence.isEmpty() && strategy->showHeader( "status" ) )
        headerStr.append("&nbsp;&nbsp;(<span name=\"presence-" + kabcUid + "\">" + presence + "</span>)" );
#endif

      if ( strategy->showHeader( "organization" )
          && !message->headerField("Organization").isEmpty())
        headerStr.append("&nbsp;&nbsp;(" +
              strToHtml(message->headerField("Organization")) + ')');
      headerStr.append("<br>\n");
    }

    if ( strategy->showHeader( "to" ) )
      headerStr.append(i18n("To: ")+
                       KMMessage::emailAddrAsAnchor(message->to(),false) + "<br>\n");

    if ( strategy->showHeader( "cc" ) && !message->cc().isEmpty() )
      headerStr.append(i18n("CC: ")+
                       KMMessage::emailAddrAsAnchor(message->cc(),false) + "<br>\n");

    if ( strategy->showHeader( "bcc" ) && !message->bcc().isEmpty() )
      headerStr.append(i18n("BCC: ")+
                       KMMessage::emailAddrAsAnchor(message->bcc(),false) + "<br>\n");

    if ( strategy->showHeader( "reply-to" ) && !message->replyTo().isEmpty())
      headerStr.append(i18n("Reply to: ")+
                     KMMessage::emailAddrAsAnchor(message->replyTo(),false) + "<br>\n");

    headerStr += "</div>\n";

    return headerStr;
  }

  QString PlainHeaderStyle::formatAllMessageHeaders( const KMMessage * message ) const {
    const DwHeaders & headers = message->headers();
    QString result;

    for ( const DwField * field = headers.FirstField() ; field ; field = field->Next() ) {
      result += ( field->FieldNameStr() + ": " ).c_str();
      result += strToHtml( field->FieldBodyStr().c_str() );
      result += "<br>\n";
    }

    return result;
  }

  //
  // FancyHeaderStyle:
  //   Like PlainHeaderStyle, but with slick frames and background colours.
  //

  class FancyHeaderStyle : public HeaderStyle {
    friend class ::KMail::HeaderStyle;
  protected:
    FancyHeaderStyle() : HeaderStyle() {}
    virtual ~FancyHeaderStyle() {}

  public:
    const char * name() const { return "fancy"; }
    const HeaderStyle * next() const { return brief(); }
    const HeaderStyle * prev() const { return plain(); }

    QString format( const KMMessage * message, const HeaderStrategy * strategy,
                    const QString & vCardName, bool printing ) const;
    static QString imgToDataUrl( const QImage & image,
                                 const char *fmt = "PNG" );

  private:
    static QString drawSpamMeter( double percent, double confidence,
        const QString & filterHeader, const QString & confidenceHeader );

  };

  QString FancyHeaderStyle::drawSpamMeter( double percent, double confidence,
                                           const QString & filterHeader, const QString & confidenceHeader )
  {
    QImage meterBar( 20, 1, QImage::Format_Indexed8 );
    meterBar.setNumColors( 24 );

    const unsigned short gradient[20][3] = {
      {   0, 255,   0 },
      {  27, 254,   0 },
      {  54, 252,   0 },
      {  80, 250,   0 },
      { 107, 249,   0 },
      { 135, 247,   0 },
      { 161, 246,   0 },
      { 187, 244,   0 },
      { 214, 242,   0 },
      { 241, 241,   0 },
      { 255, 228,   0 },
      { 255, 202,   0 },
      { 255, 177,   0 },
      { 255, 151,   0 },
      { 255, 126,   0 },
      { 255, 101,   0 },
      { 255,  76,   0 },
      { 255,  51,   0 },
      { 255,  25,   0 },
      { 255,   0,   0 }
    };
    meterBar.setColor( 21, qRgb( 255, 255, 255 ) );
    meterBar.setColor( 22, qRgb( 170, 170, 170 ) );
    if ( percent < 0 ) // grey is for errors
      meterBar.fill( 22 );
    else {
      meterBar.fill( 21 );
      int max = qMin( 20, static_cast<int>( percent ) / 5 );
      for ( int i = 0; i < max; ++i ) {
        meterBar.setColor( i+1, qRgb( gradient[i][0], gradient[i][1],
                                      gradient[i][2] ) );
        meterBar.setPixel( i, 0, i+1 );
      }
    }

    QString titleText;
    QString confidenceString;
    if ( confidence >= 0.0 ) {
      confidenceString = QString::number( confidence ) + "% &nbsp;";
      titleText = i18n("%1% probability of being spam with confidence %3%.\n\n"
                       "Full report:\nProbability=%2\nConfidence=%4",
                       percent, filterHeader, confidence, confidenceHeader );
    }
    else
      titleText = i18n("%1% probability of being spam.\n\n"
                       "Full report:\n%2",
                       percent, filterHeader );
    return QString("<img src=\"%1\" width=\"%2\" height=\"%3\" style=\"border: 1px solid black;\" title=\"%4\"> &nbsp;")
      .arg( imgToDataUrl( meterBar, "PPM" ), QString::number( 20 ),
            QString::number( 5 ), titleText ) + confidenceString;
  }


  QString FancyHeaderStyle::format( const KMMessage * message,
                                    const HeaderStrategy * strategy,
                                    const QString & vCardName, bool printing ) const {
    if ( !message ) return QString();
    if ( !strategy )
      strategy = HeaderStrategy::rich();

    KConfigGroup configReader( KMKernel::config(), "Reader" );

    // ### from kmreaderwin begin
    // The direction of the header is determined according to the direction
    // of the application layout.

    QString dir = ( QApplication::isRightToLeft() ? "rtl" : "ltr" );
    QString headerStr = QString("<div class=\"fancy header\" dir=\"%1\">\n").arg(dir);

    // However, the direction of the message subject within the header is
    // determined according to the contents of the subject itself. Since
    // the "Re:" and "Fwd:" prefixes would always cause the subject to be
    // considered left-to-right, they are ignored when determining its
    // direction.

    QString subjectDir;
    if ( !message->subject().isEmpty() )
      subjectDir = directionOf( message->cleanSubject() );
    else
      subjectDir = directionOf( i18n("No Subject") );

    // Prepare the date string (when printing always use the localized date)
    QString dateString;
    if( printing ) {
      QDateTime dateTime;
      KLocale* locale = KGlobal::locale();
      dateTime.setTime_t( message->date() );
      dateString = locale->formatDateTime( dateTime );
    }
    else {
      dateString = message->dateStr();
    }

    // Spam header display.
    // If the spamSpamStatus config value is true then we look for headers
    // from a few spam filters and try to create visually meaningful graphics
    // out of the spam scores.

    QString spamHTML;

    if ( configReader.readEntry( "showSpamStatus", true ) ) {
      SpamScores scores = SpamHeaderAnalyzer::getSpamScores( message );
      for ( SpamScoresIterator it = scores.begin(); it != scores.end(); ++it )
        spamHTML += (*it).agent() + ' ' +
                    drawSpamMeter( (*it).score(), (*it).confidence(), (*it).spamHeader(), (*it).confidenceHeader() );
    }

    QString userHTML;
    QString presence;

    // IM presence and kabc photo

    ::KIMProxy *imProxy = KMKernel::self()->imProxy();
    QString kabcUid;
    KABC::AddressBook *addressBook = KABC::StdAddressBook::self( true );
    KABC::Addressee::List addresses = addressBook->findByEmail( KPIMUtils::firstEmailAddress( message->from() ) );

    QString photoURL;
    int photoWidth = 60;
    int photoHeight = 60;
    if( addresses.count() == 1 )
    {
      // kabcUid is embedded in im: URIs to indicate which IM contact to message
      kabcUid = addresses[0].uid();

      if ( imProxy->initialize() ) {
          // im status
#ifdef __GNUC__
#warning Port KIMProxy usage!
#endif
/*          presence = imProxy->presenceString( kabcUid );
          if ( !presence.isEmpty() )
          {
            QString presenceIcon = QString::fromLatin1( " <img src=\"%1\"/>" )
                .arg( imgToDataUrl( imProxy->presenceIcon( kabcUid ).toImage() ) );
            presence += presenceIcon;
          }*/
      }
      // picture
      if ( addresses[0].photo().isIntern() )
      {
        // get photo data and convert to data: url
        //kDebug( 5006 ) <<"INTERNAL photo found";
        QImage photo = addresses[0].photo().data();
        if ( !photo.isNull() )
        {
          photoWidth = photo.width();
          photoHeight = photo.height();
          // scale below 60, otherwise it can get way too large
          if ( photoHeight > 60 ) {
            double ratio = ( double )photoHeight / ( double )photoWidth;
            photoHeight = 60;
            photoWidth = (int)( 60 / ratio );
            photo = photo.scaled( photoWidth, photoHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
          }
          photoURL = imgToDataUrl( photo );
        }
      }
      else
      {
        //kDebug( 5006 ) <<"URL found";
        photoURL = addresses[0].photo().url();
        if ( photoURL.startsWith('/') )
          photoURL.prepend( "file:" );
      }
    }
    else // TODO: find a usable one
    {
      kDebug( 5006 ) <<"Multiple / No addressees matched email address; Count is" << addresses.count();
      userHTML = "&nbsp;";
    }

    if( photoURL.isEmpty() ) {
      // no photo, look for a Face header
      QString faceheader = message->headerField( "Face" );
      if ( !faceheader.isEmpty() ) {
        QImage faceimage;

        kDebug( 5006 ) <<"Found Face: header";

        QByteArray facestring = faceheader.toUtf8();
        // Spec says header should be less than 998 bytes
        // Face: is 5 characters
        if ( facestring.length() < 993 ) {
          QByteArray facearray;
          KCodecs::base64Decode( facestring, facearray );

          QImage faceimage;
          if ( faceimage.loadFromData( facearray, "png" ) ) {
            // Spec says image must be 48x48 pixels
            if ( ( 48 == faceimage.width() ) && ( 48 == faceimage.height() ) ) {
              photoURL = imgToDataUrl( faceimage );
              photoWidth = 48;
              photoHeight = 48;
            } else {
              kDebug( 5006 ) <<"Face: header image is" << faceimage.width() <<"by" << faceimage.height() <<"not 48x48 Pixels";
            }
          } else {
            kDebug( 5006 ) <<"Failed to load decoded png from Face: header";
          }
        } else {
          kDebug( 5006 ) <<"Face: header too long at" << facestring.length();
        }
      }
    }

    if( photoURL.isEmpty() )
    {
      // no photo, look for a X-Face header
      QString xfaceURL;
      QString xfhead = message->headerField( "X-Face" );
      if ( !xfhead.isEmpty() )
      {
        KXFace xf;
        photoURL = imgToDataUrl( xf.toImage( xfhead ) );
        photoWidth = 48;
        photoHeight = 48;

      }
    }

    if( !photoURL.isEmpty() )
    {
        //kDebug( 5006 ) <<"Got a photo:" << photoURL;
      userHTML = QString("<img src=\"%1\" width=\"%2\" height=\"%3\">")
          .arg( photoURL ).arg( photoWidth ).arg( photoHeight );
      if ( presence.isEmpty() ) {
        userHTML = QString("<div class=\"senderpic\">") + userHTML + "</div>";
      } else {
        userHTML = QString( "<div class=\"senderpic\">"
            "<a href=\"im:%1\">%2<div class=\"senderstatus\">"
            "<span name=\"presence-%3\">%4</span></div></a>"
            "</div>" ).arg( kabcUid )
            .arg( userHTML )
             .arg( kabcUid )
            .arg( presence );
      }
    } else {
       // we don't have a photo, just show presence, if we have it
      if ( !presence.isEmpty() )
        userHTML = QString( "<a href=\"im:%1\"><div class=\"senderstatus\">"
            "<span name=\"presence-%2\">%3</span></div></a>" )
            .arg( kabcUid )
            .arg( kabcUid )
            .arg( presence );
    }
#if 0
    // Disabled 'Launch IM' link in headers - Will
    if ( imProxy->imAppsAvailable() )
      presence = "<a name=\"launchim\" href=\"kmail:startIMApp\">" + i18n("Launch IM") + "</a></span>";
    // do nothing - no im apps available, leave presence empty
    //presence = i18n( "DCOP/InstantMessenger not installed" );
    kDebug( 5006 ) <<"final presence: '" << presence <<"'";
#endif


    //case HdrFancy:
    // the subject line and box below for details
    if ( strategy->showHeader( "subject" ) ) {
      const int flags = LinkLocator::PreserveSpaces |
                        ( GlobalSettings::self()->showEmoticons() ?
                          LinkLocator::ReplaceSmileys : 0 );
      headerStr += QString("<div dir=\"%1\">%2</div>\n")
                        .arg(subjectDir)
                        .arg(message->subject().isEmpty()?
                             i18n("No Subject") :
                             strToHtml( message->subject(), flags ));
    }
    headerStr += "<table class=\"outer\"><tr><td width=\"100%\"><table>\n";
    //headerStr += "<table>\n";
    // from line
    // the mailto: URLs can contain %3 etc., therefore usage of multiple
    // QString::arg is not possible
    if ( strategy->showHeader( "from" ) ) {
      QString fromStr = message->from();
      if ( fromStr.isEmpty() ) // no valid email in from, maybe just a name
        fromStr = message->fromStrip(); // let's use that
      headerStr += QString("<tr><th>%1</th>\n"
                           "<td>")
                           .arg(i18n("From: "))
                 + KMMessage::emailAddrAsAnchor( fromStr, false )
                 + ( !message->headerField( "Resent-From" ).isEmpty() ? "&nbsp;"
                                + i18n("(resent from %1)",
                                    KMMessage::emailAddrAsAnchor(
                                    message->headerField( "Resent-From" ),false) )
                              : QString("") )
                 + ( !vCardName.isEmpty() ? "&nbsp;&nbsp;<a href=\"" + vCardName + "\">"
                                + i18n("[vCard]") + "</a>"
                              : QString("") )
#if 0
                 + ( ( !presence.isEmpty() )
                              ? "&nbsp;&nbsp;(<span name=\"presence-" + kabcUid + "\">" + presence + "</span>)"
                              : QString("") )
#endif
                 + ( message->headerField("Organization").isEmpty()
                              ? QString("")
                              : "&nbsp;&nbsp;("
                                + strToHtml(message->headerField("Organization"))
                                + ')')
                 + "</td></tr>\n";
    }
    // to line
    if ( strategy->showHeader( "to" ) )
      headerStr.append(QString("<tr><th>%1</th>\n"
                   "<td>%2</td></tr>\n")
                            .arg(i18n("To: "))
                            .arg(KMMessage::emailAddrAsAnchor(message->to(),false)));

    // cc line, if any
    if ( strategy->showHeader( "cc" ) && !message->cc().isEmpty())
      headerStr.append(QString("<tr><th>%1</th>\n"
                   "<td>%2</td></tr>\n")
                              .arg(i18n("CC: "))
                              .arg(KMMessage::emailAddrAsAnchor(message->cc(),false)));

    // Bcc line, if any
    if ( strategy->showHeader( "bcc" ) && !message->bcc().isEmpty())
      headerStr.append(QString("<tr><th>%1</th>\n"
                   "<td>%2</td></tr>\n")
                              .arg(i18n("BCC: "))
                              .arg(KMMessage::emailAddrAsAnchor(message->bcc(),false)));

    if ( strategy->showHeader( "date" ) )
      headerStr.append(QString("<tr><th>%1</th>\n"
                   "<td dir=\"%2\">%3</td></tr>\n")
                            .arg(i18n("Date: "))
                    .arg( directionOf( message->dateStr() ) )
                            .arg(strToHtml(dateString)));

    if ( GlobalSettings::self()->showUserAgent() ) {
      if ( strategy->showHeader( "user-agent" ) ) {
        if ( !message->headerField("User-Agent").isEmpty() ) {
          headerStr.append(QString("<tr><th>%1</th>\n"
                                   "<td>%2</td></tr>\n")
                           .arg(i18n("User-Agent: "))
                           .arg( strToHtml( message->headerField("User-Agent") ) ) );
        }
      }

      if ( strategy->showHeader( "x-mailer" ) ) {
        if ( !message->headerField("X-Mailer").isEmpty() ) {
          headerStr.append(QString("<tr><th>%1</th>\n"
                                   "<td>%2</td></tr>\n")
                           .arg(i18n("X-Mailer: "))
                           .arg( strToHtml( message->headerField("X-Mailer") ) ) );
        }
      }
    }

    // FIXME: Show status in synthetic header style field.  Decide whether this or current in brackets style is best and remove one.
    /*    if( strategy->showHeader( "status" ) )
      headerStr.append( QString( "<tr><th>%1</th>\n"
                                 "<td dir=\"%2\">%3</td></tr>\n")
                                    .arg(i18n("Sender status: "))
                                    .arg( directionOf( onlineStatus ) )
                                    .arg(onlineStatus));
    */
    headerStr.append(
          QString( "</table></td><td align=\"center\">%1</td></tr></table>\n" ).arg(userHTML) );

    if ( !spamHTML.isEmpty() )
      headerStr.append( QString( "<div class=\"spamheader\" dir=\"%1\"><b>%2</b>&nbsp;<span style=\"padding-left: 20px;\">%3</span></div>\n")
                        .arg( subjectDir, i18n("Spam Status:"), spamHTML ) );

    headerStr += "</div>\n\n";
    return headerStr;
  }

  QString FancyHeaderStyle::imgToDataUrl( const QImage &image, const char* fmt  )
  {
    QByteArray ba;
    QBuffer buffer( &ba );
    buffer.open( QIODevice::WriteOnly );
    image.save( &buffer, fmt );
    return QString::fromLatin1("data:image/%1;base64,%2")
           .arg( QString::fromLatin1( fmt ), QString::fromLatin1( KCodecs::base64Encode( ba ) ) );
  }

  //
  // HeaderStyle abstract base:
  //

  HeaderStyle::HeaderStyle() {

  }

  HeaderStyle::~HeaderStyle() {

  }

  const HeaderStyle * HeaderStyle::create( Type type ) {
    switch ( type ) {
    case Brief:  return brief();
    case Plain:  return plain();
    case Fancy:   return fancy();
    }
    kFatal( 5006 ) <<"HeaderStyle::create(): Unknown header style ( type =="
                    << (int)type << ") requested!";
    return 0; // make compiler happy
  }

  const HeaderStyle * HeaderStyle::create( const QString & type ) {
    QString lowerType = type.toLower();
    if ( lowerType == "brief" ) return brief();
    if ( lowerType == "plain" )  return plain();
    //if ( lowerType == "fancy" ) return fancy(); // not needed, see below
    // don't kFatal here, b/c the strings are user-provided
    // (KConfig), so fail gracefully to the default:
    return fancy();
  }

  static const HeaderStyle * briefStyle = 0;
  static const HeaderStyle * plainStyle = 0;
  static const HeaderStyle * fancyStyle = 0;

  const HeaderStyle * HeaderStyle::brief() {
    if ( !briefStyle )
      briefStyle = new BriefHeaderStyle();
    return briefStyle;
  }

  const HeaderStyle * HeaderStyle::plain() {
    if ( !plainStyle )
      plainStyle = new PlainHeaderStyle();
    return plainStyle;
  }

  const HeaderStyle * HeaderStyle::fancy() {
    if ( !fancyStyle )
      fancyStyle = new FancyHeaderStyle();
    return fancyStyle;
  }

} // namespace KMail
