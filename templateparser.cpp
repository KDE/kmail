/*   -*- mode: C++; c-file-style: "gnu" -*-
 *   kmail: KDE mail client
 *   Copyright (C) 2006 Dmitry Morozhnikov <dmiceman@mail.ru>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "templateparser.h"
#include "kmmessage.h"
#include "kmmsgbase.h"
#include "kmfolder.h"
#include "templatesconfiguration.h"
#include "templatesconfiguration_kfg.h"
#include "customtemplates_kfg.h"
#include "globalsettings_base.h"
#include "kmkernel.h"
#include "partNode.h"
#include "attachmentcollector.h"

#include <mimelib/bodypart.h>

#include <kpimidentities/identity.h>
#include <kpimidentities/identitymanager.h>

#include <klocale.h>
#include <kcalendarsystem.h>
#include <kglobal.h>
#include <kprocess.h>
#include <kmessagebox.h>
#include <kshell.h>

#include <QString>
#include <QDateTime>
#include <QRegExp>
#include <QFile>
#include <QFileInfo>
#include <QDir>

namespace KMail {

static const int PipeTimeout = 15 * 1000;

TemplateParser::TemplateParser( KMMessage *amsg, const Mode amode,
                                const QString &aselection,
                                bool asmartQuote, bool aallowDecryption,
                                bool aselectionIsBody ) :
  mMode( amode ), mFolder( 0 ), mIdentity( 0 ), mSelection( aselection ),
  mSmartQuote( asmartQuote ),
  mAllowDecryption( aallowDecryption ), mSelectionIsBody( aselectionIsBody ),
  mDebug( false ), mQuoteString( "> " ), mAppend( false )
{
  mMsg = amsg;
}

int TemplateParser::parseQuotes( const QString &prefix, const QString &str,
                                 QString &quote ) const
{
  int pos = prefix.length();
  int len;
  int str_len = str.length();

  // Also allow the german lower double-quote sign as quote separator, not only
  // the standard ASCII quote ("). This fixes bug 166728.
  QList< QChar > quoteChars;
  quoteChars.append( '"' );
  quoteChars.append( 0x201C );

  QChar prev( QChar::Null );

  pos++;
  len = pos;

  while ( pos < str_len ) {
    QChar c = str[pos];

    pos++;
    len++;

    if ( !prev.isNull() ) {
      quote.append( c );
      prev = QChar::Null;
    } else {
      if ( c == '\\' ) {
        prev = c;
      } else if ( quoteChars.contains( c ) ) {
        break;
      } else {
        quote.append( c );
      }
    }
  }

  return len;
}

QString TemplateParser::getFName( const QString &str )
{
  // simple logic:
  // if there is ',' in name, than format is 'Last, First'
  // else format is 'First Last'
  // last resort -- return 'name' from 'name@domain'
  int sep_pos;
  QString res;
  if ( ( sep_pos = str.indexOf( '@' ) ) > 0 ) {
    int i;
    for ( i = (sep_pos - 1); i >= 0; --i ) {
      QChar c = str[i];
      if ( c.isLetterOrNumber() ) {
        res.prepend( c );
      } else {
        break;
      }
    }
  } else if ( ( sep_pos = str.indexOf(',') ) > 0 ) {
    int i;
    bool begin = false;
    for ( i = sep_pos; i < str.length(); ++i ) {
      QChar c = str[i];
      if ( c.isLetterOrNumber() ) {
        begin = true;
        res.append( c );
      } else if ( begin ) {
        break;
      }
    }
  } else {
    int i;
    for ( i = 0; i < str.length(); ++i ) {
      QChar c = str[i];
      if ( c.isLetterOrNumber() ) {
        res.append( c );
      } else {
        break;
      }
    }
  }
  return res;
}

QString TemplateParser::getLName( const QString &str )
{
  // simple logic:
  // if there is ',' in name, than format is 'Last, First'
  // else format is 'First Last'
  int sep_pos;
  QString res;
  if ( ( sep_pos = str.indexOf(',') ) > 0 ) {
    int i;
    for ( i = sep_pos; i >= 0; --i ) {
      QChar c = str[i];
      if ( c.isLetterOrNumber() ) {
        res.prepend( c );
      } else {
        break;
      }
    }
  } else {
    if ( ( sep_pos = str.indexOf( ' ' ) ) > 0 ) {
      int i;
      bool begin = false;
      for ( i = sep_pos; i < str.length(); ++i ) {
        QChar c = str[i];
        if ( c.isLetterOrNumber() ) {
          begin = true;
          res.append( c );
        } else if ( begin ) {
          break;
        }
      }
    }
  }
  return res;
}

void TemplateParser::process( KMMessage *aorig_msg, KMFolder *afolder, bool append )
{
  mAppend = append;
  mOrigMsg = aorig_msg;
  mFolder = afolder;
  QString tmpl = findTemplate();
  return processWithTemplate( tmpl );
}

void TemplateParser::process( const QString &tmplName, KMMessage *aorig_msg,
                              KMFolder *afolder, bool append )
{
  mAppend = append;
  mOrigMsg = aorig_msg;
  mFolder = afolder;
  QString tmpl = findCustomTemplate( tmplName );
  return processWithTemplate( tmpl );
}

void TemplateParser::processWithTemplate( const QString &tmpl )
{
  QString body;
  int tmpl_len = tmpl.length();
  bool dnl = false;
  for ( int i = 0; i < tmpl_len; ++i ) {
    QChar c = tmpl[i];
    // kDebug() << "Next char: " << c;
    if ( c == '%' ) {
      QString cmd = tmpl.mid( i + 1 );

      if ( cmd.startsWith( '-' ) ) {
        // dnl
        kDebug() <<"Command: -";
        dnl = true;
        i += 1;

      } else if ( cmd.startsWith( QLatin1String("REM=") ) ) {
        // comments
        kDebug() <<"Command: REM=";
        QString q;
        int len = parseQuotes( "REM=", cmd, q );
        i += len;

      } else if ( cmd.startsWith( QLatin1String("INSERT=") ) ) {
        // insert content of specified file as is
        kDebug() <<"Command: INSERT=";
        QString q;
        int len = parseQuotes( "INSERT=", cmd, q );
        i += len;
        QString path = KShell::tildeExpand( q );
        QFileInfo finfo( path );
        if (finfo.isRelative() ) {
          path = QDir::homePath();
          path += '/';
          path += q;
        }
        QFile file( path );
        if ( file.open( QIODevice::ReadOnly ) ) {
          QByteArray content = file.readAll();
          QString str = QString::fromLocal8Bit( content, content.size() );
          body.append( str );
        } else if ( mDebug ) {
          KMessageBox::error( 0,
                              i18nc( "@info:status", "Cannot insert content from file %1: %2",
                                    path, file.errorString() ) );
        }

      } else if ( cmd.startsWith( QLatin1String("SYSTEM=") ) ) {
        // insert content of specified file as is
        kDebug() <<"Command: SYSTEM=";
        QString q;
        int len = parseQuotes( "SYSTEM=", cmd, q );
        i += len;
        QString pipe_cmd = q;
        QString str = pipe( pipe_cmd, "" );
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("PUT=") ) ) {
        // insert content of specified file as is
        kDebug() <<"Command: PUT=";
        QString q;
        int len = parseQuotes( "PUT=", cmd, q );
        i += len;
        QString path = KShell::tildeExpand( q );
        QFileInfo finfo( path );
        if (finfo.isRelative() ) {
          path = QDir::homePath();
          path += '/';
          path += q;
        }
        QFile file( path );
        if ( file.open( QIODevice::ReadOnly ) ) {
          QByteArray content = file.readAll();
          body.append( QString::fromLocal8Bit( content, content.size() ) );
        } else if ( mDebug ) {
          KMessageBox::error( 0,
                              i18nc( "@info:status", "Cannot insert content from file %1: %2",
                                    path, file.errorString() ));
        }

      } else if ( cmd.startsWith( QLatin1String("QUOTEPIPE=") ) ) {
        // pipe message body through command and insert it as quotation
        kDebug() <<"Command: QUOTEPIPE=";
        QString q;
        int len = parseQuotes( "QUOTEPIPE=", cmd, q );
        i += len;
        QString pipe_cmd = q;
        if ( mOrigMsg ) {
          QString str =
              pipe( pipe_cmd, mOrigMsg->asPlainText( mSmartQuote, mAllowDecryption ) );
          QString quote = mOrigMsg->asQuotedString( mQuoteString, str,
                                                    mSmartQuote, mAllowDecryption );
          if ( quote.endsWith( '\n' ) )
            quote.chop( 1 );
          body.append( quote );
        }

      } else if ( cmd.startsWith( QLatin1String("QUOTE") ) ) {
        kDebug() <<"Command: QUOTE";
        i += strlen( "QUOTE" );
        if ( mOrigMsg ) {
          QString quote = mOrigMsg->asQuotedString( mQuoteString, mSelection,
                                                    mSmartQuote, mAllowDecryption );
          if ( quote.endsWith( '\n' ) )
            quote.chop( 1 );
          body.append( quote );
        }

      } else if ( cmd.startsWith( QLatin1String("QHEADERS") ) ) {
        kDebug() <<"Command: QHEADERS";
        i += strlen( "QHEADERS" );
        if ( mOrigMsg ) {
          QString quote = mOrigMsg->asQuotedString( mQuoteString,
                                                    mOrigMsg->headerAsSendableString(),
                                                    mSmartQuote, false );
          if ( quote.endsWith( '\n' ) )
               quote.chop( 1 );
          body.append( quote );
        }

      } else if ( cmd.startsWith( QLatin1String("HEADERS") ) ) {
        kDebug() <<"Command: HEADERS";
        i += strlen( "HEADERS" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->headerAsSendableString();
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("TEXTPIPE=") ) ) {
        // pipe message body through command and insert it as is
        kDebug() <<"Command: TEXTPIPE=";
        QString q;
        int len = parseQuotes( "TEXTPIPE=", cmd, q );
        i += len;
        QString pipe_cmd = q;
        if ( mOrigMsg ) {
          QString str =
              pipe(pipe_cmd, mOrigMsg->asPlainText( mSmartQuote, mAllowDecryption ) );
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("MSGPIPE=") ) ) {
        // pipe full message through command and insert result as is
        kDebug() <<"Command: MSGPIPE=";
        QString q;
        int len = parseQuotes( "MSGPIPE=", cmd, q );
        i += len;
        QString pipe_cmd = q;
        if ( mOrigMsg ) {
          QString str = pipe(pipe_cmd, mOrigMsg->asString() );
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("BODYPIPE=") ) ) {
        // pipe message body generated so far through command and insert result as is
        kDebug() <<"Command: BODYPIPE=";
        QString q;
        int len = parseQuotes( "BODYPIPE=", cmd, q );
        i += len;
        QString pipe_cmd = q;
        QString str = pipe( pipe_cmd, body );
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("CLEARPIPE=") ) ) {
        // pipe message body generated so far through command and
        // insert result as is replacing current body
        kDebug() <<"Command: CLEARPIPE=";
        QString q;
        int len = parseQuotes( "CLEARPIPE=", cmd, q );
        i += len;
        QString pipe_cmd = q;
        QString str = pipe( pipe_cmd, body );
        body = str;
        mMsg->setCursorPos( 0 );

      } else if ( cmd.startsWith( QLatin1String("TEXT") ) ) {
        kDebug() <<"Command: TEXT";
        i += strlen( "TEXT" );
        if ( mOrigMsg ) {
          QString quote = mOrigMsg->asPlainText( false, mAllowDecryption );
          body.append( quote );
        }

      } else if ( cmd.startsWith( QLatin1String("OTEXTSIZE") ) ) {
        kDebug() <<"Command: OTEXTSIZE";
        i += strlen( "OTEXTSIZE" );
        if ( mOrigMsg ) {
          QString str = QString( "%1" ).arg( mOrigMsg->body().length() );
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OTEXT") ) ) {
        kDebug() <<"Command: OTEXT";
        i += strlen( "OTEXT" );
        if ( mOrigMsg ) {
          QString quote = mOrigMsg->asPlainText( false, mAllowDecryption );
          body.append( quote );
        }

      } else if ( cmd.startsWith( QLatin1String("CCADDR") ) ) {
        kDebug() <<"Command: CCADDR";
        i += strlen( "CCADDR" );
        QString str = mMsg->cc();
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("CCNAME") ) ) {
        kDebug() <<"Command: CCNAME";
        i += strlen( "CCNAME" );
        QString str = mMsg->ccStrip();
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("CCFNAME") ) ) {
        kDebug() <<"Command: CCFNAME";
        i += strlen( "CCFNAME" );
        QString str = mMsg->ccStrip();
        body.append( getFName( str ) );

      } else if ( cmd.startsWith( QLatin1String("CCLNAME") ) ) {
        kDebug() <<"Command: CCLNAME";
        i += strlen( "CCLNAME" );
        QString str = mMsg->ccStrip();
        body.append( getLName( str ) );

      } else if ( cmd.startsWith( QLatin1String("TOADDR") ) ) {
        kDebug() <<"Command: TOADDR";
        i += strlen( "TOADDR" );
        QString str = mMsg->to();
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("TONAME") ) ) {
        kDebug() <<"Command: TONAME";
        i += strlen( "TONAME" );
        QString str = mMsg->toStrip();
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("TOFNAME") ) ) {
        kDebug() <<"Command: TOFNAME";
        i += strlen( "TOFNAME" );
        QString str = mMsg->toStrip();
        body.append( getFName( str ) );

      } else if ( cmd.startsWith( QLatin1String("TOLNAME") ) ) {
        kDebug() <<"Command: TOLNAME";
        i += strlen( "TOLNAME" );
        QString str = mMsg->toStrip();
        body.append( getLName( str ) );

      } else if ( cmd.startsWith( QLatin1String("TOLIST") ) ) {
        kDebug() <<"Command: TOLIST";
        i += strlen( "TOLIST" );
        QString str = mMsg->to();
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("FROMADDR") ) ) {
        kDebug() <<"Command: FROMADDR";
        i += strlen( "FROMADDR" );
        QString str = mMsg->from();
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("FROMNAME") ) ) {
        kDebug() <<"Command: FROMNAME";
        i += strlen( "FROMNAME" );
        QString str = mMsg->fromStrip();
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("FROMFNAME") ) ) {
        kDebug() <<"Command: FROMFNAME";
        i += strlen( "FROMFNAME" );
        QString str = mMsg->fromStrip();
        body.append( getFName( str ) );

      } else if ( cmd.startsWith( QLatin1String("FROMLNAME") ) ) {
        kDebug() <<"Command: FROMLNAME";
        i += strlen( "FROMLNAME" );
        QString str = mMsg->fromStrip();
        body.append( getLName( str ) );

      } else if ( cmd.startsWith( QLatin1String("FULLSUBJECT") ) ) {
        kDebug() <<"Command: FULLSUBJECT";
        i += strlen( "FULLSUBJECT" );
        QString str = mMsg->subject();
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("FULLSUBJ") ) ) {
        kDebug() <<"Command: FULLSUBJ";
        i += strlen( "FULLSUBJ" );
        QString str = mMsg->subject();
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("MSGID") ) ) {
        kDebug() <<"Command: MSGID";
        i += strlen( "MSGID" );
        QString str = mMsg->id();
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("OHEADER=") ) ) {
        // insert specified content of header from original message
        kDebug() <<"Command: OHEADER=";
        QString q;
        int len = parseQuotes( "OHEADER=", cmd, q );
        i += len;
        if ( mOrigMsg ) {
          QString hdr = q;
          QString str = mOrigMsg->headerFields(hdr.toLocal8Bit() ).join( ", " );
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("HEADER=") ) ) {
        // insert specified content of header from current message
        kDebug() <<"Command: HEADER=";
        QString q;
        int len = parseQuotes( "HEADER=", cmd, q );
        i += len;
        QString hdr = q;
        QString str = mMsg->headerFields(hdr.toLocal8Bit() ).join( ", " );
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("HEADER( ") ) ) {
        // insert specified content of header from current message
        kDebug() <<"Command: HEADER(";
        QRegExp re = QRegExp( "^HEADER\\((.+)\\)" );
        re.setMinimal( true );
        int res = re.indexIn( cmd );
        if ( res != 0 ) {
          // something wrong
          i += strlen( "HEADER( " );
        } else {
          i += re.matchedLength();
          QString hdr = re.cap( 1 );
          QString str = mMsg->headerFields( hdr.toLocal8Bit() ).join( ", " );
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OCCADDR") ) ) {
        kDebug() <<"Command: OCCADDR";
        i += strlen( "OCCADDR" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->cc();
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OCCNAME") ) ) {
        kDebug() <<"Command: OCCNAME";
        i += strlen( "OCCNAME" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->ccStrip();
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OCCFNAME") ) ) {
        kDebug() <<"Command: OCCFNAME";
        i += strlen( "OCCFNAME" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->ccStrip();
          body.append( getFName( str ) );
        }

      } else if ( cmd.startsWith( QLatin1String("OCCLNAME") ) ) {
        kDebug() <<"Command: OCCLNAME";
        i += strlen( "OCCLNAME" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->ccStrip();
          body.append( getLName( str ) );
        }

      } else if ( cmd.startsWith( QLatin1String("OTOADDR") ) ) {
        kDebug() <<"Command: OTOADDR";
        i += strlen( "OTOADDR" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->to();
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OTONAME") ) ) {
        kDebug() <<"Command: OTONAME";
        i += strlen( "OTONAME" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->toStrip();
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OTOFNAME") ) ) {
        kDebug() <<"Command: OTOFNAME";
        i += strlen( "OTOFNAME" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->toStrip();
          body.append( getFName( str ) );
        }

      } else if ( cmd.startsWith( QLatin1String("OTOLNAME") ) ) {
        kDebug() <<"Command: OTOLNAME";
        i += strlen( "OTOLNAME" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->toStrip();
          body.append( getLName( str ) );
        }

      } else if ( cmd.startsWith( QLatin1String("OTOLIST") ) ) {
        kDebug() <<"Command: OTOLIST";
        i += strlen( "OTOLIST" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->to();
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OTO") ) ) {
        kDebug() <<"Command: OTO";
        i += strlen( "OTO" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->to();
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OFROMADDR") ) ) {
        kDebug() <<"Command: OFROMADDR";
        i += strlen( "OFROMADDR" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->from();
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OFROMNAME") ) ) {
        kDebug() <<"Command: OFROMNAME";
        i += strlen( "OFROMNAME" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->fromStrip();
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OFROMFNAME") ) ) {
        kDebug() <<"Command: OFROMFNAME";
        i += strlen( "OFROMFNAME" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->fromStrip();
          body.append( getFName( str ) );
        }

      } else if ( cmd.startsWith( QLatin1String("OFROMLNAME") ) ) {
        kDebug() <<"Command: OFROMLNAME";
        i += strlen( "OFROMLNAME" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->fromStrip();
          body.append( getLName( str ) );
        }

      } else if ( cmd.startsWith( QLatin1String("OFULLSUBJECT") ) ) {
        kDebug() <<"Command: OFULLSUBJECT";
        i += strlen( "OFULLSUBJECT" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->subject();
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OFULLSUBJ") ) ) {
        kDebug() <<"Command: OFULLSUBJ";
        i += strlen( "OFULLSUBJ" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->subject();
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OMSGID") ) ) {
        kDebug() <<"Command: OMSGID";
        i += strlen( "OMSGID" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->id();
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("DATEEN") ) ) {
        kDebug() <<"Command: DATEEN";
        i += strlen( "DATEEN" );
        QDateTime date = QDateTime::currentDateTime();
        KLocale locale( "C" );
        QString str = locale.formatDate( date.date(), KLocale::LongDate );
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("DATESHORT") ) ) {
        kDebug() <<"Command: DATESHORT";
        i += strlen( "DATESHORT" );
        QDateTime date = QDateTime::currentDateTime();
        QString str = KGlobal::locale()->formatDate( date.date(), KLocale::ShortDate );
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("DATE") ) ) {
        kDebug() <<"Command: DATE";
        i += strlen( "DATE" );
        QDateTime date = QDateTime::currentDateTime();
        QString str = KGlobal::locale()->formatDate( date.date(), KLocale::LongDate );
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("DOW") ) ) {
        kDebug() <<"Command: DOW";
        i += strlen( "DOW" );
        QDateTime date = QDateTime::currentDateTime();
        QString str = KGlobal::locale()->calendar()->weekDayName( date.date(),
                      KCalendarSystem::LongDayName );
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("TIMELONGEN") ) ) {
        kDebug() <<"Command: TIMELONGEN";
        i += strlen( "TIMELONGEN" );
        QDateTime date = QDateTime::currentDateTime();
        KLocale locale( "C");
        QString str = locale.formatTime( date.time(), true );
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("TIMELONG") ) ) {
        kDebug() <<"Command: TIMELONG";
        i += strlen( "TIMELONG" );
        QDateTime date = QDateTime::currentDateTime();
        QString str = KGlobal::locale()->formatTime( date.time(), true );
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("TIME") ) ) {
        kDebug() <<"Command: TIME";
        i += strlen( "TIME" );
        QDateTime date = QDateTime::currentDateTime();
        QString str = KGlobal::locale()->formatTime( date.time(), false );
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("ODATEEN") ) ) {
        kDebug() <<"Command: ODATEEN";
        i += strlen( "ODATEEN" );
        if ( mOrigMsg ) {
          QDateTime date;
          date.setTime_t( mOrigMsg->date() );
          KLocale locale( "C");
          QString str = locale.formatDate( date.date(), KLocale::LongDate );
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("ODATESHORT")) ) {
        kDebug() <<"Command: ODATESHORT";
        i += strlen( "ODATESHORT");
        if ( mOrigMsg ) {
          QDateTime date;
          date.setTime_t( mOrigMsg->date() );
          QString str = KGlobal::locale()->formatDate( date.date(), KLocale::ShortDate );
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("ODATE")) ) {
        kDebug() <<"Command: ODATE";
        i += strlen( "ODATE");
        if ( mOrigMsg ) {
          QDateTime date;
          date.setTime_t( mOrigMsg->date() );
          QString str = KGlobal::locale()->formatDate( date.date(), KLocale::LongDate );
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("ODOW")) ) {
        kDebug() <<"Command: ODOW";
        i += strlen( "ODOW");
        if ( mOrigMsg ) {
          QDateTime date;
          date.setTime_t( mOrigMsg->date() );
          QString str = KGlobal::locale()->calendar()->weekDayName( date.date(),
                        KCalendarSystem::LongDayName );
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OTIMELONGEN")) ) {
        kDebug() <<"Command: OTIMELONGEN";
        i += strlen( "OTIMELONGEN");
        if ( mOrigMsg ) {
          QDateTime date;
          date.setTime_t( mOrigMsg->date() );
          KLocale locale( "C");
          QString str = locale.formatTime( date.time(), true );
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OTIMELONG")) ) {
        kDebug() <<"Command: OTIMELONG";
        i += strlen( "OTIMELONG");
        if ( mOrigMsg ) {
          QDateTime date;
          date.setTime_t( mOrigMsg->date() );
          QString str = KGlobal::locale()->formatTime( date.time(), true );
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OTIME")) ) {
        kDebug() <<"Command: OTIME";
        i += strlen( "OTIME");
        if ( mOrigMsg ) {
          QDateTime date;
          date.setTime_t( mOrigMsg->date() );
          QString str = KGlobal::locale()->formatTime( date.time(), false );
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("BLANK") ) ) {
        // do nothing
        kDebug() <<"Command: BLANK";
        i += strlen( "BLANK" );

      } else if ( cmd.startsWith( QLatin1String("NOP") ) ) {
        // do nothing
        kDebug() <<"Command: NOP";
        i += strlen( "NOP" );

      } else if ( cmd.startsWith( QLatin1String("CLEAR") ) ) {
        // clear body buffer; not too useful yet
        kDebug() <<"Command: CLEAR";
        i += strlen( "CLEAR" );
        body = "";
        mMsg->setCursorPos( 0 );

      } else if ( cmd.startsWith( QLatin1String("DEBUGOFF") ) ) {
        // turn off debug
        kDebug() <<"Command: DEBUGOFF";
        i += strlen( "DEBUGOFF" );
        mDebug = false;

      } else if ( cmd.startsWith( QLatin1String("DEBUG") ) ) {
        // turn on debug
        kDebug() <<"Command: DEBUG";
        i += strlen( "DEBUG" );
        mDebug = true;

      } else if ( cmd.startsWith( QLatin1String("CURSOR") ) ) {
        // turn on debug
        kDebug() <<"Command: CURSOR";
        i += strlen( "CURSOR" );
        mMsg->setCursorPos( body.length() );

      } else {
        // wrong command, do nothing
        body.append( c );
      }

    } else if ( dnl && ( c == '\n' || c == '\r') ) {
      // skip
      if ( ( tmpl.size() > i+1 ) &&
           ( ( c == '\n' && tmpl[i + 1] == '\r' ) ||
             ( c == '\r' && tmpl[i + 1] == '\n' ) ) ) {
        // skip one more
        i += 1;
      }
      dnl = false;
    } else {
      body.append( c );
    }
  }

  addProcessedBodyToMessage( body );
}


void TemplateParser::addProcessedBodyToMessage( const QString &body )
{
  if ( mAppend ) {

    // ### What happens here if the body is multipart or in some way encoded?
    QByteArray msg_body = mMsg->body();
    msg_body.append( body.toUtf8() );
    mMsg->setBody( msg_body );
  }
  else {

    // Get the attachments of the original mail
    partNode *root = partNode::fromMessage( mMsg );
    KMail::AttachmentCollector ac;
    ac.setDiveIntoEncryptions( true );
    ac.setDiveIntoSignatures( true );
    ac.setDiveIntoMessages( false );
    ac.collectAttachmentsFrom( root );

    // Now, delete the old content and set the new content, which
    // is either only the new text or the new text with some attachments.
    mMsg->deleteBodyParts();

    // Set To and CC from the template
    if ( mMode == Forward ) {
      if ( !mTo.isEmpty() ) {
        mMsg->setTo( mMsg->to() + ',' + mTo );
      }
      if ( !mCC.isEmpty() )
        mMsg->setCc( mMsg->cc() + ',' + mCC );
    }

    // If we have no attachment, simply create a text/plain part and
    // set the processed template text as the body
    if ( ac.attachments().empty() ) {
      mMsg->headers().ContentType().FromString( DwString() ); // to get rid of old boundary
      mMsg->headers().ContentType().Parse();
      mMsg->headers().ContentType().SetType( DwMime::kTypeText );
      mMsg->headers().ContentType().SetSubtype( DwMime::kSubtypePlain );
      mMsg->headers().Assemble();
      mMsg->setBodyFromUnicode( body );
      mMsg->assembleIfNeeded();
    }

    // If we have some attachments, create a multipart/mixed mail and
    // add the normal body as well as the attachments
    else
    {
      mMsg->headers().ContentType().SetType( DwMime::kTypeMultipart );
      mMsg->headers().ContentType().SetSubtype( DwMime::kSubtypeMixed );
      mMsg->headers().ContentType().CreateBoundary( 0 );

      KMMessagePart textPart;
      textPart.setBodyFromUnicode( body );
      mMsg->addDwBodyPart( mMsg->createDWBodyPart( &textPart ) );
      mMsg->assembleIfNeeded();

      foreach( const partNode *attachment, ac.attachments() ) {

        // When adding this body part, make sure to _not_ add the next bodypart
        // as well, which mimelib would do, therefore creating a mail with many
        // duplicate attachments (so many that KMail runs out of memory, in fact).
        // Body::AddBodyPart is very misleading here...
        attachment->dwPart()->SetNext( 0 );

        mMsg->addDwBodyPart( attachment->dwPart() );
        mMsg->assembleIfNeeded();
      }
    }
  }
}
QString TemplateParser::findCustomTemplate( const QString &tmplName )
{
  CTemplates t( tmplName );
  mTo = t.to();
  mCC = t.cC();
  QString content = t.content();
  if ( !content.isEmpty() ) {
    return content;
  } else {
    return findTemplate();
  }
}

QString TemplateParser::findTemplate()
{
  // import 'Phrases' if it not done yet
  if ( !GlobalSettings::self()->phrasesConverted() ) {
    TemplatesConfiguration::importFromPhrases();
  }

  // kDebug() <<"Trying to find template for mode" << mode;

  QString tmpl;

  if ( !mFolder ) { // find folder message belongs to
    mFolder = mMsg->parent();
    if ( !mFolder ) {
      if ( mOrigMsg ) {
        mFolder = mOrigMsg->parent();
      }
      if ( !mFolder ) {
        kDebug() <<"Oops! No folder for message";
      }
    }
  }
  kDebug() <<"Folder found:" << mFolder;

  if ( mFolder )  // only if a folder was found
  {
    QString fid = mFolder->idString();
    Templates fconf( fid );
    if ( fconf.useCustomTemplates() ) {   // does folder use custom templates?
      switch( mMode ) {
      case NewMessage:
        tmpl = fconf.templateNewMessage();
        break;
      case Reply:
        tmpl = fconf.templateReply();
        break;
      case ReplyAll:
        tmpl = fconf.templateReplyAll();
        break;
      case Forward:
        tmpl = fconf.templateForward();
        break;
      default:
        kDebug() <<"Unknown message mode:" << mMode;
        return "";
      }
      mQuoteString = fconf.quoteString();
      if ( !tmpl.isEmpty() ) {
        return tmpl;  // use folder-specific template
      }
    }
  }

  if ( !mIdentity ) { // find identity message belongs to
    mIdentity = mMsg->identityUoid();
    if ( !mIdentity && mOrigMsg ) {
      mIdentity = mOrigMsg->identityUoid();
    }
    mIdentity = kmkernel->identityManager()->identityForUoidOrDefault( mIdentity ).uoid();
    if ( !mIdentity ) {
      kDebug() <<"Oops! No identity for message";
    }
  }
  kDebug() <<"Identity found:" << mIdentity;

  QString iid;
  if ( mIdentity ) {
    iid = QString("IDENTITY_%1").arg( mIdentity );	// templates ID for that identity
  }
  else {
    iid = "IDENTITY_NO_IDENTITY"; // templates ID for no identity
  }

  Templates iconf( iid );
  if ( iconf.useCustomTemplates() ) { // does identity use custom templates?
    switch( mMode ) {
    case NewMessage:
      tmpl = iconf.templateNewMessage();
      break;
    case Reply:
      tmpl = iconf.templateReply();
      break;
    case ReplyAll:
      tmpl = iconf.templateReplyAll();
      break;
    case Forward:
      tmpl = iconf.templateForward();
      break;
    default:
      kDebug() <<"Unknown message mode:" << mMode;
      return "";
    }
    mQuoteString = iconf.quoteString();
    if ( !tmpl.isEmpty() ) {
      return tmpl;  // use identity-specific template
    }
  }

  switch( mMode ) { // use the global template
  case NewMessage:
    tmpl = GlobalSettings::self()->templateNewMessage();
    break;
  case Reply:
    tmpl = GlobalSettings::self()->templateReply();
    break;
  case ReplyAll:
    tmpl = GlobalSettings::self()->templateReplyAll();
    break;
  case Forward:
    tmpl = GlobalSettings::self()->templateForward();
    break;
  default:
    kDebug() <<"Unknown message mode:" << mMode;
    return "";
  }

  mQuoteString = GlobalSettings::self()->quoteString();
  return tmpl;
}

QString TemplateParser::pipe( const QString &cmd, const QString &buf )
{
  KProcess process;
  bool success, finished;

  process.setOutputChannelMode( KProcess::SeparateChannels );
  process.setShellCommand( cmd );
  process.start();
  if ( process.waitForStarted( PipeTimeout ) ) {
    if ( !buf.isEmpty() )
      process.write( buf.toAscii() );
    if ( buf.isEmpty() || process.waitForBytesWritten( PipeTimeout ) ) {
      if ( !buf.isEmpty() )
        process.closeWriteChannel();
      if ( process.waitForFinished( PipeTimeout ) ) {
        success = ( process.exitStatus() == QProcess::NormalExit );
        finished = true;
      }
      else {
        finished = false;
        success = false;
      }
    }
    else {
      success = false;
      finished = false;
    }

    // The process has started, but did not finish in time. Kill it.
    if ( !finished )
      process.kill();
  }
  else
    success = false;

  if ( !success && mDebug )
    KMessageBox::error( 0, i18nc( "@info",
                                  "Pipe command <command>%1</command> failed.",
                                  cmd ) );

  if ( success )
    return process.readAllStandardOutput();
  else
    return QString();
}

} // namespace KMail

#include "templateparser.moc"
