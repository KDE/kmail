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

#include "headerstyle.h"

#include "headerstrategy.h"
#include "linklocator.h"
#include "kmmessage.h"


#include <mimelib/string.h>
#include <mimelib/field.h>
#include <mimelib/headers.h>

#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>

#include <qdatetime.h>
#include <qapplication.h>
#include <qregexp.h>

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
  static QString strToHtml( const QString & str, bool preserveBlanks=true ) {
    return LinkLocator::convertToHtml( str, preserveBlanks );
  }

  //
  // BriefHeaderStyle
  //   Show everything in a single line, don't show header field names.
  //

  class BriefHeaderStyle : public HeaderStyle {
    friend class HeaderStyle;
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
    if ( !message ) return QString::null;
    if ( !strategy )
      strategy = HeaderStrategy::brief();

    // The direction of the header is determined according to the direction
    // of the application layout.

    QString dir = QApplication::reverseLayout() ? "rtl" : "ltr" ;

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
      QString fromPart = KMMessage::emailAddrAsAnchor( message->from(), true );
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
    headerStr += " (" + headerParts.grep( QRegExp( "\\S" ) ).join( ",\n" ) + ')';

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
    friend class HeaderStyle;
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
    if ( !message ) return QString::null;
    if ( !strategy )
      strategy = HeaderStrategy::rich();

    // The direction of the header is determined according to the direction
    // of the application layout.

    QString dir = ( QApplication::reverseLayout() ? "rtl" : "ltr" );

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

    QString headerStr = QString("<div class=\"header\" dir=\"%1\">").arg(dir);

    if ( strategy->headersToDisplay().isEmpty()
	 && strategy->defaultPolicy() == HeaderStrategy::Display ) {
      // crude way to emulate "all" headers:
      headerStr += formatAllMessageHeaders( message );
      return headerStr + "</div>";
    }

    //case HdrLong:
    if ( strategy->showHeader( "subject" ) )
      headerStr += QString("<div dir=\"%1\"><b style=\"font-size:130%\">" +
			   strToHtml(message->subject()) + "</b></div>\n")
                        .arg(subjectDir);

    if ( strategy->showHeader( "date" ) )
      headerStr.append(i18n("Date: ") + strToHtml(dateString)+"<br>\n");

    if ( strategy->showHeader( "from" ) ) {
      headerStr.append(i18n("From: ") +
		       KMMessage::emailAddrAsAnchor(message->from(),FALSE));
      if ( !vCardName.isEmpty() )
	headerStr.append("&nbsp;&nbsp;<a href=\"" + vCardName +
			 "\">" + i18n("[vCard]") + "</a>" );
      if ( strategy->showHeader( "organization" )
	   && !message->headerField("Organization").isEmpty())
	headerStr.append("&nbsp;&nbsp;(" +
			 strToHtml(message->headerField("Organization")) + ")");
      headerStr.append("<br>\n");
    }

    if ( strategy->showHeader( "to" ) )
      headerStr.append(i18n("To: ")+
		       KMMessage::emailAddrAsAnchor(message->to(),FALSE) + "<br>\n");

    if ( strategy->showHeader( "cc" ) && !message->cc().isEmpty() )
      headerStr.append(i18n("CC: ")+
                       KMMessage::emailAddrAsAnchor(message->cc(),FALSE) + "<br>\n");

    if ( strategy->showHeader( "bcc" ) && !message->bcc().isEmpty() )
      headerStr.append(i18n("BCC: ")+
                       KMMessage::emailAddrAsAnchor(message->bcc(),FALSE) + "<br>\n");

    if ( strategy->showHeader( "reply-to" ) && !message->replyTo().isEmpty())
      headerStr.append(i18n("Reply to: ")+
                     KMMessage::emailAddrAsAnchor(message->replyTo(),FALSE) + "<br>\n");

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
    friend class HeaderStyle;
  protected:
    FancyHeaderStyle() : HeaderStyle() {}
    virtual ~FancyHeaderStyle() {}
    
  public:
    const char * name() const { return "fancy"; }
    const HeaderStyle * next() const { return brief(); }
    const HeaderStyle * prev() const { return plain(); }

    QString format( const KMMessage * message, const HeaderStrategy * strategy,
		    const QString & vCardName, bool printing ) const;
  };

  QString FancyHeaderStyle::format( const KMMessage * message,
				    const HeaderStrategy * strategy,
				    const QString & vCardName, bool printing ) const {
    if ( !message ) return QString::null;
    if ( !strategy )
      strategy = HeaderStrategy::rich();

    // ### from kmreaderwin begin
    // The direction of the header is determined according to the direction
    // of the application layout.

    QString dir = ( QApplication::reverseLayout() ? "rtl" : "ltr" );
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

    //case HdrFancy:
    // the subject line and box below for details
    if ( strategy->showHeader( "subject" ) )
      headerStr += QString("<div dir=\"%1\">%2</div>\n")
                        .arg(subjectDir)
		        .arg(message->subject().isEmpty()?
			     i18n("No Subject") :
			     strToHtml(message->subject()));
    headerStr += "<table>\n";

    // from line
    // the mailto: URLs can contain %3 etc., therefore usage of multiple
    // QString::arg is not possible
    if ( strategy->showHeader( "from" ) )
      headerStr += QString("<tr><th>%1</th>\n"
			   "<td>")
                         .arg(i18n("From: "))
                 + KMMessage::emailAddrAsAnchor(message->from(),FALSE)
                 + ( !vCardName.isEmpty() ? "&nbsp;&nbsp;<a href=\"" + vCardName + "\">"
                                + i18n("[vCard]") + "</a>"
                              : QString("") )
                 + ( message->headerField("Organization").isEmpty()
                              ? QString("")
                              : "&nbsp;&nbsp;("
                                + strToHtml(message->headerField("Organization"))
                                + ")")
                 + "</td></tr>\n";
    // to line
    if ( strategy->showHeader( "to" ) )
      headerStr.append(QString("<tr><th>%1</th>\n"
			       "<td>%2</td></tr>\n")
                            .arg(i18n("To: "))
                            .arg(KMMessage::emailAddrAsAnchor(message->to(),FALSE)));

    // cc line, if any
    if ( strategy->showHeader( "cc" ) && !message->cc().isEmpty())
      headerStr.append(QString("<tr><th>%1</th>\n"
			       "<td>%2</td></tr>\n")
                              .arg(i18n("CC: "))
                              .arg(KMMessage::emailAddrAsAnchor(message->cc(),FALSE)));

    // Bcc line, if any
    if ( strategy->showHeader( "bcc" ) && !message->bcc().isEmpty())
      headerStr.append(QString("<tr><th>%1</th>\n"
			       "<td>%2</td></tr>\n")
                              .arg(i18n("BCC: "))
                              .arg(KMMessage::emailAddrAsAnchor(message->bcc(),FALSE)));

    if ( strategy->showHeader( "date" ) )
      headerStr.append(QString("<tr><th>%1</th>\n"
			       "<td dir=\"%2\">%3</td></tr>\n")
                            .arg(i18n("Date: "))
		            .arg( directionOf( message->dateStr() ) )
                            .arg(strToHtml(dateString)));
    headerStr.append("</table>\n");

    headerStr += "</div>\n\n";

    return headerStr;
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
    kdFatal( 5006 ) << "HeaderStyle::create(): Unknown header style ( type == "
		    << (int)type << " ) requested!" << endl;
    return 0; // make compiler happy
  }

  const HeaderStyle * HeaderStyle::create( const QString & type ) {
    QString lowerType = type.lower();
    if ( lowerType == "brief" ) return brief();
    if ( lowerType == "plain" )  return plain();
    //if ( lowerType == "fancy" ) return fancy(); // not needed, see below
    // don't kdFatal here, b/c the strings are user-provided
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
