/*   -*- mode: C++; c-file-style: "gnu" -*-
 *   kmail: KDE mail client
 *   This file: Copyright (C) 2006 Dmitry Morozhnikov <dmiceman@mail.ru>
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
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */


#include <qstring.h>
#include <qdatetime.h>
#include <klocale.h>
#include <kcalendarsystem.h>
#include <kglobal.h>
#include <kprocess.h>
#include <qregexp.h>
#include <qfile.h>
#include <kmessagebox.h>
#include <kshell.h>
#include <qfileinfo.h>
#include <qdir.h>

#include "kmmessage.h"
#include "kmmsgbase.h"
#include "kmfolder.h"
#include "templatesconfiguration.h"
#include "templatesconfiguration_kfg.h"
#include "customtemplates_kfg.h"
#include "globalsettings_base.h"
#include "kmkernel.h"
#include <kpimidentities/identity.h>
#include <kpimidentities/identitymanager.h>

#include "templateparser.h"

namespace KMail {

static const int PipeTimeout = 15 * 1000;

TemplateParser::TemplateParser( KMMessage *amsg, const Mode amode,
                                const QString &aselection,
                                bool asmartQuote, bool anoQuote,
                                bool aallowDecryption, bool aselectionIsBody ) :
  mMode( amode ), mFolder( 0 ), mIdentity( 0 ), mSelection( aselection ),
  mSmartQuote( asmartQuote ), mNoQuote( anoQuote ),
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
  QChar qc = '"';
  QChar prev = QChar::null;

  pos++;
  len = pos;

  while ( pos < str_len ) {
    QChar c = str[pos];

    pos++;
    len++;

    if ( !prev.isNull() ) {
      quote.append( c );
      prev = QChar::null;
    } else {
      if ( c == '\\' ) {
        prev = c;
      } else if ( c == qc ) {
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
    // kDebugug() << "Next char: " << c;
    if ( c == '%' ) {
      QString cmd = tmpl.mid( i + 1 );

      if ( cmd.startsWith( "-" ) ) {
        // dnl
        kDebug(5006) <<"Command: -";
        dnl = true;
        i += 1;

      } else if ( cmd.startsWith( "REM=" ) ) {
        // comments
        kDebug(5006) <<"Command: REM=";
        QString q;
        int len = parseQuotes( "REM=", cmd, q );
        i += len;

      } else if ( cmd.startsWith( "INSERT=" ) ) {
        // insert content of specified file as is
        kDebug(5006) <<"Command: INSERT=";
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
        if ( file.open( IO_ReadOnly ) ) {
          QByteArray content = file.readAll();
          QString str = QString::fromLocal8Bit( content, content.size() );
          body.append( str );
        } else if ( mDebug ) {
          KMessageBox::error( 0,
                              i18n( "Cannot insert content from file %1: %2",
                                    path, file.errorString() ) );
        }

      } else if ( cmd.startsWith( "SYSTEM=" ) ) {
        // insert content of specified file as is
        kDebug(5006) <<"Command: SYSTEM=";
        QString q;
        int len = parseQuotes( "SYSTEM=", cmd, q );
        i += len;
        QString pipe_cmd = q;
        QString str = pipe( pipe_cmd, "" );
        body.append( str );

      } else if ( cmd.startsWith( "PUT=" ) ) {
        // insert content of specified file as is
        kDebug(5006) <<"Command: PUT=";
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
        if ( file.open( IO_ReadOnly ) ) {
          QByteArray content = file.readAll();
          body.append( QString::fromLocal8Bit( content, content.size() ) );
        } else if ( mDebug ) {
          KMessageBox::error( 0,
                              i18n( "Cannot insert content from file %1: %2",
                                    path, file.errorString() ));
        }

      } else if ( cmd.startsWith( "QUOTEPIPE=" ) ) {
        // pipe message body through command and insert it as quotation
        kDebug(5006) <<"Command: QUOTEPIPE=";
        QString q;
        int len = parseQuotes( "QUOTEPIPE=", cmd, q );
        i += len;
        QString pipe_cmd = q;
        if ( mOrigMsg && !mNoQuote ) {
          QString str = pipe( pipe_cmd, mSelection );
          QString quote = mOrigMsg->asQuotedString( "", mQuoteString, str,
                                                    mSmartQuote, mAllowDecryption );
          body.append( quote );
        }

      } else if ( cmd.startsWith( "QUOTE" ) ) {
        kDebug(5006) <<"Command: QUOTE";
        i += strlen( "QUOTE" );
        if ( mOrigMsg && !mNoQuote ) {
          QString quote = mOrigMsg->asQuotedString( "", mQuoteString, mSelection,
                                                    mSmartQuote, mAllowDecryption );
          body.append( quote );
        }

      } else if ( cmd.startsWith( "QHEADERS" ) ) {
        kDebug(5006) <<"Command: QHEADERS";
        i += strlen( "QHEADERS" );
        if ( mOrigMsg && !mNoQuote ) {
          QString quote = mOrigMsg->asQuotedString( "", mQuoteString,
                                                    mOrigMsg->headerAsSendableString(),
                                                    mSmartQuote, false );
          body.append( quote );
        }

      } else if ( cmd.startsWith( "HEADERS" ) ) {
        kDebug(5006) <<"Command: HEADERS";
        i += strlen( "HEADERS" );
        if ( mOrigMsg && !mNoQuote ) {
          QString str = mOrigMsg->headerAsSendableString();
          body.append( str );
        }

      } else if ( cmd.startsWith( "TEXTPIPE=" ) ) {
        // pipe message body through command and insert it as is
        kDebug(5006) <<"Command: TEXTPIPE=";
        QString q;
        int len = parseQuotes( "TEXTPIPE=", cmd, q );
        i += len;
        QString pipe_cmd = q;
        if ( mOrigMsg ) {
          QString str = pipe(pipe_cmd, mSelection );
          body.append( str );
        }

      } else if ( cmd.startsWith( "MSGPIPE=" ) ) {
        // pipe full message through command and insert result as is
        kDebug(5006) <<"Command: MSGPIPE=";
        QString q;
        int len = parseQuotes( "MSGPIPE=", cmd, q );
        i += len;
        QString pipe_cmd = q;
        if ( mOrigMsg ) {
          QString str = pipe(pipe_cmd, mOrigMsg->asString() );
          body.append( str );
        }

      } else if ( cmd.startsWith( "BODYPIPE=" ) ) {
        // pipe message body generated so far through command and insert result as is
        kDebug(5006) <<"Command: BODYPIPE=";
        QString q;
        int len = parseQuotes( "BODYPIPE=", cmd, q );
        i += len;
        QString pipe_cmd = q;
        QString str = pipe( pipe_cmd, body );
        body.append( str );

      } else if ( cmd.startsWith( "CLEARPIPE=" ) ) {
        // pipe message body generated so far through command and
        // insert result as is replacing current body
        kDebug(5006) <<"Command: CLEARPIPE=";
        QString q;
        int len = parseQuotes( "CLEARPIPE=", cmd, q );
        i += len;
        QString pipe_cmd = q;
        QString str = pipe( pipe_cmd, body );
        body = str;
        mMsg->setCursorPos( 0 );

      } else if ( cmd.startsWith( "TEXT" ) ) {
        kDebug(5006) <<"Command: TEXT";
        i += strlen( "TEXT" );
        if ( mOrigMsg ) {
          QString quote = mOrigMsg->asPlainText( false, mAllowDecryption );
          body.append( quote );
        }

      } else if ( cmd.startsWith( "OTEXTSIZE" ) ) {
        kDebug(5006) <<"Command: OTEXTSIZE";
        i += strlen( "OTEXTSIZE" );
        if ( mOrigMsg ) {
          QString str = QString( "%1" ).arg( mOrigMsg->body().length() );
          body.append( str );
        }

      } else if ( cmd.startsWith( "OTEXT" ) ) {
        kDebug(5006) <<"Command: OTEXT";
        i += strlen( "OTEXT" );
        if ( mOrigMsg ) {
          QString quote = mOrigMsg->asPlainText( false, mAllowDecryption );
          body.append( quote );
        }

      } else if ( cmd.startsWith( "CCADDR" ) ) {
        kDebug(5006) <<"Command: CCADDR";
        i += strlen( "CCADDR" );
        QString str = mMsg->cc();
        body.append( str );

      } else if ( cmd.startsWith( "CCNAME" ) ) {
        kDebug(5006) <<"Command: CCNAME";
        i += strlen( "CCNAME" );
        QString str = mMsg->ccStrip();
        body.append( str );

      } else if ( cmd.startsWith( "CCFNAME" ) ) {
        kDebug(5006) <<"Command: CCFNAME";
        i += strlen( "CCFNAME" );
        QString str = mMsg->ccStrip();
        body.append( getFName( str ) );

      } else if ( cmd.startsWith( "CCLNAME" ) ) {
        kDebug(5006) <<"Command: CCLNAME";
        i += strlen( "CCLNAME" );
        QString str = mMsg->ccStrip();
        body.append( getLName( str ) );

      } else if ( cmd.startsWith( "TOADDR" ) ) {
        kDebug(5006) <<"Command: TOADDR";
        i += strlen( "TOADDR" );
        QString str = mMsg->to();
        body.append( str );

      } else if ( cmd.startsWith( "TONAME" ) ) {
        kDebug(5006) <<"Command: TONAME";
        i += strlen( "TONAME" );
        QString str = mMsg->toStrip();
        body.append( str );

      } else if ( cmd.startsWith( "TOFNAME" ) ) {
        kDebug(5006) <<"Command: TOFNAME";
        i += strlen( "TOFNAME" );
        QString str = mMsg->toStrip();
        body.append( getFName( str ) );

      } else if ( cmd.startsWith( "TOLNAME" ) ) {
        kDebug(5006) <<"Command: TOLNAME";
        i += strlen( "TOLNAME" );
        QString str = mMsg->toStrip();
        body.append( getLName( str ) );

      } else if ( cmd.startsWith( "TOLIST" ) ) {
        kDebug(5006) <<"Command: TOLIST";
        i += strlen( "TOLIST" );
        QString str = mMsg->to();
        body.append( str );

      } else if ( cmd.startsWith( "FROMADDR" ) ) {
        kDebug(5006) <<"Command: FROMADDR";
        i += strlen( "FROMADDR" );
        QString str = mMsg->from();
        body.append( str );

      } else if ( cmd.startsWith( "FROMNAME" ) ) {
        kDebug(5006) <<"Command: FROMNAME";
        i += strlen( "FROMNAME" );
        QString str = mMsg->fromStrip();
        body.append( str );

      } else if ( cmd.startsWith( "FROMFNAME" ) ) {
        kDebug(5006) <<"Command: FROMFNAME";
        i += strlen( "FROMFNAME" );
        QString str = mMsg->fromStrip();
        body.append( getFName( str ) );

      } else if ( cmd.startsWith( "FROMLNAME" ) ) {
        kDebug(5006) <<"Command: FROMLNAME";
        i += strlen( "FROMLNAME" );
        QString str = mMsg->fromStrip();
        body.append( getLName( str ) );

      } else if ( cmd.startsWith( "FULLSUBJECT" ) ) {
        kDebug(5006) <<"Command: FULLSUBJECT";
        i += strlen( "FULLSUBJECT" );
        QString str = mMsg->subject();
        body.append( str );

      } else if ( cmd.startsWith( "FULLSUBJ" ) ) {
        kDebug(5006) <<"Command: FULLSUBJ";
        i += strlen( "FULLSUBJ" );
        QString str = mMsg->subject();
        body.append( str );

      } else if ( cmd.startsWith( "MSGID" ) ) {
        kDebug(5006) <<"Command: MSGID";
        i += strlen( "MSGID" );
        QString str = mMsg->id();
        body.append( str );

      } else if ( cmd.startsWith( "OHEADER=" ) ) {
        // insert specified content of header from original message
        kDebug(5006) <<"Command: OHEADER=";
        QString q;
        int len = parseQuotes( "OHEADER=", cmd, q );
        i += len;
        if ( mOrigMsg ) {
          QString hdr = q;
          QString str = mOrigMsg->headerFields(hdr.toLocal8Bit() ).join( ", " );
          body.append( str );
        }

      } else if ( cmd.startsWith( "HEADER=" ) ) {
        // insert specified content of header from current message
        kDebug(5006) <<"Command: HEADER=";
        QString q;
        int len = parseQuotes( "HEADER=", cmd, q );
        i += len;
        QString hdr = q;
        QString str = mMsg->headerFields(hdr.toLocal8Bit() ).join( ", " );
        body.append( str );

      } else if ( cmd.startsWith( "HEADER( " ) ) {
        // insert specified content of header from current message
        kDebug(5006) <<"Command: HEADER(";
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

      } else if ( cmd.startsWith( "OCCADDR" ) ) {
        kDebug(5006) <<"Command: OCCADDR";
        i += strlen( "OCCADDR" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->cc();
          body.append( str );
        }

      } else if ( cmd.startsWith( "OCCNAME" ) ) {
        kDebug(5006) <<"Command: OCCNAME";
        i += strlen( "OCCNAME" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->ccStrip();
          body.append( str );
        }

      } else if ( cmd.startsWith( "OCCFNAME" ) ) {
        kDebug(5006) <<"Command: OCCFNAME";
        i += strlen( "OCCFNAME" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->ccStrip();
          body.append( getFName( str ) );
        }

      } else if ( cmd.startsWith( "OCCLNAME" ) ) {
        kDebug(5006) <<"Command: OCCLNAME";
        i += strlen( "OCCLNAME" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->ccStrip();
          body.append( getLName( str ) );
        }

      } else if ( cmd.startsWith( "OTOADDR" ) ) {
        kDebug(5006) <<"Command: OTOADDR";
        i += strlen( "OTOADDR" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->to();
          body.append( str );
        }

      } else if ( cmd.startsWith( "OTONAME" ) ) {
        kDebug(5006) <<"Command: OTONAME";
        i += strlen( "OTONAME" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->toStrip();
          body.append( str );
        }

      } else if ( cmd.startsWith( "OTOFNAME" ) ) {
        kDebug(5006) <<"Command: OTOFNAME";
        i += strlen( "OTOFNAME" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->toStrip();
          body.append( getFName( str ) );
        }

      } else if ( cmd.startsWith( "OTOLNAME" ) ) {
        kDebug(5006) <<"Command: OTOLNAME";
        i += strlen( "OTOLNAME" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->toStrip();
          body.append( getLName( str ) );
        }

      } else if ( cmd.startsWith( "OTOLIST" ) ) {
        kDebug(5006) <<"Command: OTOLIST";
        i += strlen( "OTOLIST" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->to();
          body.append( str );
        }

      } else if ( cmd.startsWith( "OTO" ) ) {
        kDebug(5006) <<"Command: OTO";
        i += strlen( "OTO" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->to();
          body.append( str );
        }

      } else if ( cmd.startsWith( "OFROMADDR" ) ) {
        kDebug(5006) <<"Command: OFROMADDR";
        i += strlen( "OFROMADDR" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->from();
          body.append( str );
        }

      } else if ( cmd.startsWith( "OFROMNAME" ) ) {
        kDebug(5006) <<"Command: OFROMNAME";
        i += strlen( "OFROMNAME" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->fromStrip();
          body.append( str );
        }

      } else if ( cmd.startsWith( "OFROMFNAME" ) ) {
        kDebug(5006) <<"Command: OFROMFNAME";
        i += strlen( "OFROMFNAME" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->fromStrip();
          body.append( getFName( str ) );
        }

      } else if ( cmd.startsWith( "OFROMLNAME" ) ) {
        kDebug(5006) <<"Command: OFROMLNAME";
        i += strlen( "OFROMLNAME" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->fromStrip();
          body.append( getLName( str ) );
        }

      } else if ( cmd.startsWith( "OFULLSUBJECT" ) ) {
        kDebug(5006) <<"Command: OFULLSUBJECT";
        i += strlen( "OFULLSUBJECT" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->subject();
          body.append( str );
        }

      } else if ( cmd.startsWith( "OFULLSUBJ" ) ) {
        kDebug(5006) <<"Command: OFULLSUBJ";
        i += strlen( "OFULLSUBJ" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->subject();
          body.append( str );
        }

      } else if ( cmd.startsWith( "OMSGID" ) ) {
        kDebug(5006) <<"Command: OMSGID";
        i += strlen( "OMSGID" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->id();
          body.append( str );
        }

      } else if ( cmd.startsWith( "DATEEN" ) ) {
        kDebug(5006) <<"Command: DATEEN";
        i += strlen( "DATEEN" );
        QDateTime date = QDateTime::currentDateTime();
        KLocale locale( "C" );
        QString str = locale.formatDate( date.date(), KLocale::LongDate );
        body.append( str );

      } else if ( cmd.startsWith( "DATESHORT" ) ) {
        kDebug(5006) <<"Command: DATESHORT";
        i += strlen( "DATESHORT" );
        QDateTime date = QDateTime::currentDateTime();
        QString str = KGlobal::locale()->formatDate( date.date(), KLocale::ShortDate );
        body.append( str );

      } else if ( cmd.startsWith( "DATE" ) ) {
        kDebug(5006) <<"Command: DATE";
        i += strlen( "DATE" );
        QDateTime date = QDateTime::currentDateTime();
        QString str = KGlobal::locale()->formatDate( date.date(), KLocale::LongDate );
        body.append( str );

      } else if ( cmd.startsWith( "DOW" ) ) {
        kDebug(5006) <<"Command: DOW";
        i += strlen( "DOW" );
        QDateTime date = QDateTime::currentDateTime();
        QString str = KGlobal::locale()->calendar()->weekDayName( date.date(),
                      KCalendarSystem::LongDayName );
        body.append( str );

      } else if ( cmd.startsWith( "TIMELONGEN" ) ) {
        kDebug(5006) <<"Command: TIMELONGEN";
        i += strlen( "TIMELONGEN" );
        QDateTime date = QDateTime::currentDateTime();
        KLocale locale( "C");
        QString str = locale.formatTime( date.time(), true );
        body.append( str );

      } else if ( cmd.startsWith( "TIMELONG" ) ) {
        kDebug(5006) <<"Command: TIMELONG";
        i += strlen( "TIMELONG" );
        QDateTime date = QDateTime::currentDateTime();
        QString str = KGlobal::locale()->formatTime( date.time(), true );
        body.append( str );

      } else if ( cmd.startsWith( "TIME" ) ) {
        kDebug(5006) <<"Command: TIME";
        i += strlen( "TIME" );
        QDateTime date = QDateTime::currentDateTime();
        QString str = KGlobal::locale()->formatTime( date.time(), false );
        body.append( str );

      } else if ( cmd.startsWith( "ODATEEN" ) ) {
        kDebug(5006) <<"Command: ODATEEN";
        i += strlen( "ODATEEN" );
        if ( mOrigMsg ) {
          QDateTime date;
          date.setTime_t( mOrigMsg->date() );
          KLocale locale( "C");
          QString str = locale.formatDate( date.date(), KLocale::LongDate );
          body.append( str );
        }

      } else if ( cmd.startsWith( "ODATESHORT") ) {
        kDebug(5006) <<"Command: ODATESHORT";
        i += strlen( "ODATESHORT");
        if ( mOrigMsg ) {
          QDateTime date;
          date.setTime_t( mOrigMsg->date() );
          QString str = KGlobal::locale()->formatDate( date.date(), KLocale::ShortDate );
          body.append( str );
        }

      } else if ( cmd.startsWith( "ODATE") ) {
        kDebug(5006) <<"Command: ODATE";
        i += strlen( "ODATE");
        if ( mOrigMsg ) {
          QDateTime date;
          date.setTime_t( mOrigMsg->date() );
          QString str = KGlobal::locale()->formatDate( date.date(), KLocale::LongDate );
          body.append( str );
        }

      } else if ( cmd.startsWith( "ODOW") ) {
        kDebug(5006) <<"Command: ODOW";
        i += strlen( "ODOW");
        if ( mOrigMsg ) {
          QDateTime date;
          date.setTime_t( mOrigMsg->date() );
          QString str = KGlobal::locale()->calendar()->weekDayName( date.date(),
                        KCalendarSystem::LongDayName );
          body.append( str );
        }

      } else if ( cmd.startsWith( "OTIMELONGEN") ) {
        kDebug(5006) <<"Command: OTIMELONGEN";
        i += strlen( "OTIMELONGEN");
        if ( mOrigMsg ) {
          QDateTime date;
          date.setTime_t( mOrigMsg->date() );
          KLocale locale( "C");
          QString str = locale.formatTime( date.time(), true );
          body.append( str );
        }

      } else if ( cmd.startsWith( "OTIMELONG") ) {
        kDebug(5006) <<"Command: OTIMELONG";
        i += strlen( "OTIMELONG");
        if ( mOrigMsg ) {
          QDateTime date;
          date.setTime_t( mOrigMsg->date() );
          QString str = KGlobal::locale()->formatTime( date.time(), true );
          body.append( str );
        }

      } else if ( cmd.startsWith( "OTIME") ) {
        kDebug(5006) <<"Command: OTIME";
        i += strlen( "OTIME");
        if ( mOrigMsg ) {
          QDateTime date;
          date.setTime_t( mOrigMsg->date() );
          QString str = KGlobal::locale()->formatTime( date.time(), false );
          body.append( str );
        }

      } else if ( cmd.startsWith( "BLANK" ) ) {
        // do nothing
        kDebug(5006) <<"Command: BLANK";
        i += strlen( "BLANK" );

      } else if ( cmd.startsWith( "NOP" ) ) {
        // do nothing
        kDebug(5006) <<"Command: NOP";
        i += strlen( "NOP" );

      } else if ( cmd.startsWith( "CLEAR" ) ) {
        // clear body buffer; not too useful yet
        kDebug(5006) <<"Command: CLEAR";
        i += strlen( "CLEAR" );
        body = "";
        mMsg->setCursorPos( 0 );

      } else if ( cmd.startsWith( "DEBUGOFF" ) ) {
        // turn off debug
        kDebug(5006) <<"Command: DEBUGOFF";
        i += strlen( "DEBUGOFF" );
        mDebug = false;

      } else if ( cmd.startsWith( "DEBUG" ) ) {
        // turn on debug
        kDebug(5006) <<"Command: DEBUG";
        i += strlen( "DEBUG" );
        mDebug = true;

      } else if ( cmd.startsWith( "CURSOR" ) ) {
        // turn on debug
        kDebug(5006) <<"Command: CURSOR";
        i += strlen( "CURSOR" );
        mMsg->setCursorPos( body.length() );

      } else {
        // wrong command, do nothing
        body.append( c );
      }

    } else if ( dnl && ( c == '\n' || c == '\r') ) {
      // skip
      if ( ( c == '\n' && tmpl[i + 1] == '\r' ) ||
           ( c == '\r' && tmpl[i + 1] == '\n' ) ) {
        // skip one more
        i += 1;
      }
      dnl = false;
    } else {
      body.append( c );
    }
  }

  // kDebug(5006) <<"Message body:" << body;

  if ( mAppend ) {
    QByteArray msg_body = mMsg->body();
    msg_body.append( body.toUtf8() );
    mMsg->setBody( msg_body );
  } else {
    mMsg->setBodyFromUnicode( body );
  }
}

QString TemplateParser::findCustomTemplate( const QString &tmplName )
{
  CTemplates t( tmplName );
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

  // kDebug(5006) <<"Trying to find template for mode" << mode;

  QString tmpl;

  if ( !mFolder ) { // find folder message belongs to
    mFolder = mMsg->parent();
    if ( !mFolder ) {
      if ( mOrigMsg ) {
        mFolder = mOrigMsg->parent();
      }
      if ( !mFolder ) {
        kDebug(5006) <<"Oops! No folder for message";
      }
    }
  }
  kDebug(5006) <<"Folder found:" << mFolder;

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
        kDebug(5006) <<"Unknown message mode:" << mMode;
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
      kDebug(5006) <<"Oops! No identity for message";
    }
  }
  kDebug(5006) <<"Identity found:" << mIdentity;

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
      kDebug(5006) <<"Unknown message mode:" << mMode;
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
    kDebug(5006) <<"Unknown message mode:" << mMode;
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
    process.write( buf.toAscii() );
    if ( process.waitForBytesWritten( PipeTimeout ) ) {
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
