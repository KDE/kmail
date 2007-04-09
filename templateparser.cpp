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

#include <config.h>

#include <qstring.h>
#include <qdatetime.h>
#include <klocale.h>
#include <kcalendarsystem.h>
#include <kglobal.h>
#include <k3process.h>
#include <qregexp.h>
#include <qfile.h>
#include <kmessagebox.h>
#include <kshell.h>
#include <qfileinfo.h>

#include "kmmessage.h"
#include "kmmsgbase.h"
#include "kmfolder.h"
#include "templatesconfiguration.h"
#include "templatesconfiguration_kfg.h"
#include "customtemplates_kfg.h"
#include "globalsettings_base.h"
#include "kmkernel.h"
#include <libkpimidentities/identity.h>
#include <libkpimidentities/identitymanager.h>

#include "templateparser.h"

namespace KMail {

static const int PipeTimeout = 15;

TemplateParser::TemplateParser( KMMessage *amsg, const Mode amode,
                                const QString aselection,
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
  if ( ( sep_pos = str.find( '@' ) ) > 0 ) {
    int i;
    for ( i = (sep_pos - 1); i >= 0; --i ) {
      QChar c = str[i];
      if ( c.isLetterOrNumber() ) {
        res.prepend( c );
      } else {
        break;
      }
    }
  } else if ( ( sep_pos = str.find(',') ) > 0 ) {
    unsigned int i;
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
    unsigned int i;
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
  if ( ( sep_pos = str.find(',') ) > 0 ) {
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
    if ( ( sep_pos = str.find( ' ' ) ) > 0 ) {
      unsigned int i;
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
    // kDebugug() << "Next char: " << c << endl;
    if ( c == '%' ) {
      QString cmd = tmpl.mid( i + 1 );

      if ( cmd.startsWith( "-" ) ) {
        // dnl
        kDebug() << "Command: -" << endl;
        dnl = true;
        i += 1;

      } else if ( cmd.startsWith( "REM=" ) ) {
        // comments
        kDebug() << "Command: REM=" << endl;
        QString q;
        int len = parseQuotes( "REM=", cmd, q );
        i += len;

      } else if ( cmd.startsWith( "INSERT=" ) ) {
        // insert content of specified file as is
        kDebug() << "Command: INSERT=" << endl;
        QString q;
        int len = parseQuotes( "INSERT=", cmd, q );
        i += len;
        QString path = KShell::tildeExpand( q );
        QFileInfo finfo( path );
        if (finfo.isRelative() ) {
          path = KShell::homeDir( "" );
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
        kDebug() << "Command: SYSTEM=" << endl;
        QString q;
        int len = parseQuotes( "SYSTEM=", cmd, q );
        i += len;
        QString pipe_cmd = q;
        QString str = pipe( pipe_cmd, "" );
        body.append( str );

      } else if ( cmd.startsWith( "PUT=" ) ) {
        // insert content of specified file as is
        kDebug() << "Command: PUT=" << endl;
        QString q;
        int len = parseQuotes( "PUT=", cmd, q );
        i += len;
        QString path = KShell::tildeExpand( q );
        QFileInfo finfo( path );
        if (finfo.isRelative() ) {
          path = KShell::homeDir( "" );
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
        // pipe message body throw command and insert it as quotation
        kDebug() << "Command: QUOTEPIPE=" << endl;
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
        kDebug() << "Command: QUOTE" << endl;
        i += strlen( "QUOTE" );
        if ( mOrigMsg && !mNoQuote ) {
          QString quote = mOrigMsg->asQuotedString( "", mQuoteString, mSelection,
                                                    mSmartQuote, mAllowDecryption );
          body.append( quote );
        }

      } else if ( cmd.startsWith( "QHEADERS" ) ) {
        kDebug() << "Command: QHEADERS" << endl;
        i += strlen( "QHEADERS" );
        if ( mOrigMsg && !mNoQuote ) {
          QString quote = mOrigMsg->asQuotedString( "", mQuoteString,
                                                    mOrigMsg->headerAsSendableString(),
                                                    mSmartQuote, false );
          body.append( quote );
        }

      } else if ( cmd.startsWith( "HEADERS" ) ) {
        kDebug() << "Command: HEADERS" << endl;
        i += strlen( "HEADERS" );
        if ( mOrigMsg && !mNoQuote ) {
          QString str = mOrigMsg->headerAsSendableString();
          body.append( str );
        }

      } else if ( cmd.startsWith( "TEXTPIPE=" ) ) {
        // pipe message body throw command and insert it as is
        kDebug() << "Command: TEXTPIPE=" << endl;
        QString q;
        int len = parseQuotes( "TEXTPIPE=", cmd, q );
        i += len;
        QString pipe_cmd = q;
        if ( mOrigMsg ) {
          QString str = pipe(pipe_cmd, mSelection );
          body.append( str );
        }

      } else if ( cmd.startsWith( "MSGPIPE=" ) ) {
        // pipe full message throw command and insert result as is
        kDebug() << "Command: MSGPIPE=" << endl;
        QString q;
        int len = parseQuotes( "MSGPIPE=", cmd, q );
        i += len;
        QString pipe_cmd = q;
        if ( mOrigMsg ) {
          QString str = pipe(pipe_cmd, mOrigMsg->asString() );
          body.append( str );
        }

      } else if ( cmd.startsWith( "BODYPIPE=" ) ) {
        // pipe message body generated so far throw command and insert result as is
        kDebug() << "Command: BODYPIPE=" << endl;
        QString q;
        int len = parseQuotes( "BODYPIPE=", cmd, q );
        i += len;
        QString pipe_cmd = q;
        QString str = pipe( pipe_cmd, body );
        body.append( str );

      } else if ( cmd.startsWith( "CLEARPIPE=" ) ) {
        // pipe message body generated so far throw command and
        // insert result as is replacing current body
        kDebug() << "Command: CLEARPIPE=" << endl;
        QString q;
        int len = parseQuotes( "CLEARPIPE=", cmd, q );
        i += len;
        QString pipe_cmd = q;
        QString str = pipe( pipe_cmd, body );
        body = str;
        mMsg->setCursorPos( 0 );

      } else if ( cmd.startsWith( "TEXT" ) ) {
        kDebug() << "Command: TEXT" << endl;
        i += strlen( "TEXT" );
        if ( mOrigMsg ) {
          QString quote = mOrigMsg->asPlainText( false, mAllowDecryption );
          body.append( quote );
        }

      } else if ( cmd.startsWith( "OTEXTSIZE" ) ) {
        kDebug() << "Command: OTEXTSIZE" << endl;
        i += strlen( "OTEXTSIZE" );
        if ( mOrigMsg ) {
          QString str = QString( "%1" ).arg( mOrigMsg->body().length() );
          body.append( str );
        }

      } else if ( cmd.startsWith( "OTEXT" ) ) {
        kDebug() << "Command: OTEXT" << endl;
        i += strlen( "OTEXT" );
        if ( mOrigMsg ) {
          QString quote = mOrigMsg->asPlainText( false, mAllowDecryption );
          body.append( quote );
        }

      } else if ( cmd.startsWith( "CCADDR" ) ) {
        kDebug() << "Command: CCADDR" << endl;
        i += strlen( "CCADDR" );
        QString str = mMsg->cc();
        body.append( str );

      } else if ( cmd.startsWith( "CCNAME" ) ) {
        kDebug() << "Command: CCNAME" << endl;
        i += strlen( "CCNAME" );
        QString str = mMsg->ccStrip();
        body.append( str );

      } else if ( cmd.startsWith( "CCFNAME" ) ) {
        kDebug() << "Command: CCFNAME" << endl;
        i += strlen( "CCFNAME" );
        QString str = mMsg->ccStrip();
        body.append( getFName( str ) );

      } else if ( cmd.startsWith( "CCLNAME" ) ) {
        kDebug() << "Command: CCLNAME" << endl;
        i += strlen( "CCLNAME" );
        QString str = mMsg->ccStrip();
        body.append( getLName( str ) );

      } else if ( cmd.startsWith( "TOADDR" ) ) {
        kDebug() << "Command: TOADDR" << endl;
        i += strlen( "TOADDR" );
        QString str = mMsg->to();
        body.append( str );

      } else if ( cmd.startsWith( "TONAME" ) ) {
        kDebug() << "Command: TONAME" << endl;
        i += strlen( "TONAME" );
        QString str = mMsg->toStrip();
        body.append( str );

      } else if ( cmd.startsWith( "TOFNAME" ) ) {
        kDebug() << "Command: TOFNAME" << endl;
        i += strlen( "TOFNAME" );
        QString str = mMsg->toStrip();
        body.append( getFName( str ) );

      } else if ( cmd.startsWith( "TOLNAME" ) ) {
        kDebug() << "Command: TOLNAME" << endl;
        i += strlen( "TOLNAME" );
        QString str = mMsg->toStrip();
        body.append( getLName( str ) );

      } else if ( cmd.startsWith( "TOLIST" ) ) {
        kDebug() << "Command: TOLIST" << endl;
        i += strlen( "TOLIST" );
        QString str = mMsg->to();
        body.append( str );

      } else if ( cmd.startsWith( "FROMADDR" ) ) {
        kDebug() << "Command: FROMADDR" << endl;
        i += strlen( "FROMADDR" );
        QString str = mMsg->from();
        body.append( str );

      } else if ( cmd.startsWith( "FROMNAME" ) ) {
        kDebug() << "Command: FROMNAME" << endl;
        i += strlen( "FROMNAME" );
        QString str = mMsg->fromStrip();
        body.append( str );

      } else if ( cmd.startsWith( "FROMFNAME" ) ) {
        kDebug() << "Command: FROMFNAME" << endl;
        i += strlen( "FROMFNAME" );
        QString str = mMsg->fromStrip();
        body.append( getFName( str ) );

      } else if ( cmd.startsWith( "FROMLNAME" ) ) {
        kDebug() << "Command: FROMLNAME" << endl;
        i += strlen( "FROMLNAME" );
        QString str = mMsg->fromStrip();
        body.append( getLName( str ) );

      } else if ( cmd.startsWith( "FULLSUBJECT" ) ) {
        kDebug() << "Command: FULLSUBJECT" << endl;
        i += strlen( "FULLSUBJECT" );
        QString str = mMsg->subject();
        body.append( str );

      } else if ( cmd.startsWith( "FULLSUBJ" ) ) {
        kDebug() << "Command: FULLSUBJ" << endl;
        i += strlen( "FULLSUBJ" );
        QString str = mMsg->subject();
        body.append( str );

      } else if ( cmd.startsWith( "MSGID" ) ) {
        kDebug() << "Command: MSGID" << endl;
        i += strlen( "MSGID" );
        QString str = mMsg->id();
        body.append( str );

      } else if ( cmd.startsWith( "OHEADER=" ) ) {
        // insert specified content of header from original message
        kDebug() << "Command: OHEADER=" << endl;
        QString q;
        int len = parseQuotes( "OHEADER=", cmd, q );
        i += len;
        if ( mOrigMsg ) {
          QString hdr = q;
          QString str = mOrigMsg->headerFields(hdr.local8Bit() ).join( ", " );
          body.append( str );
        }

      } else if ( cmd.startsWith( "HEADER=" ) ) {
        // insert specified content of header from current message
        kDebug() << "Command: HEADER=" << endl;
        QString q;
        int len = parseQuotes( "HEADER=", cmd, q );
        i += len;
        QString hdr = q;
        QString str = mMsg->headerFields(hdr.local8Bit() ).join( ", " );
        body.append( str );

      } else if ( cmd.startsWith( "HEADER( " ) ) {
        // insert specified content of header from current message
        kDebug() << "Command: HEADER( " << endl;
        QRegExp re = QRegExp( "^HEADER\\((.+)\\)" );
        re.setMinimal( true );
        int res = re.search( cmd );
        if ( res != 0 ) {
          // something wrong
          i += strlen( "HEADER( " );
        } else {
          i += re.matchedLength();
          QString hdr = re.cap( 1 );
          QString str = mMsg->headerFields( hdr.local8Bit() ).join( ", " );
          body.append( str );
        }

      } else if ( cmd.startsWith( "OCCADDR" ) ) {
        kDebug() << "Command: OCCADDR" << endl;
        i += strlen( "OCCADDR" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->cc();
          body.append( str );
        }

      } else if ( cmd.startsWith( "OCCNAME" ) ) {
        kDebug() << "Command: OCCNAME" << endl;
        i += strlen( "OCCNAME" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->ccStrip();
          body.append( str );
        }

      } else if ( cmd.startsWith( "OCCFNAME" ) ) {
        kDebug() << "Command: OCCFNAME" << endl;
        i += strlen( "OCCFNAME" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->ccStrip();
          body.append( getFName( str ) );
        }

      } else if ( cmd.startsWith( "OCCLNAME" ) ) {
        kDebug() << "Command: OCCLNAME" << endl;
        i += strlen( "OCCLNAME" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->ccStrip();
          body.append( getLName( str ) );
        }

      } else if ( cmd.startsWith( "OTOADDR" ) ) {
        kDebug() << "Command: OTOADDR" << endl;
        i += strlen( "OTOADDR" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->to();
          body.append( str );
        }

      } else if ( cmd.startsWith( "OTONAME" ) ) {
        kDebug() << "Command: OTONAME" << endl;
        i += strlen( "OTONAME" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->toStrip();
          body.append( str );
        }

      } else if ( cmd.startsWith( "OTOFNAME" ) ) {
        kDebug() << "Command: OTOFNAME" << endl;
        i += strlen( "OTOFNAME" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->toStrip();
          body.append( getFName( str ) );
        }

      } else if ( cmd.startsWith( "OTOLNAME" ) ) {
        kDebug() << "Command: OTOLNAME" << endl;
        i += strlen( "OTOLNAME" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->toStrip();
          body.append( getLName( str ) );
        }

      } else if ( cmd.startsWith( "OTOLIST" ) ) {
        kDebug() << "Command: OTOLIST" << endl;
        i += strlen( "OTOLIST" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->to();
          body.append( str );
        }

      } else if ( cmd.startsWith( "OTO" ) ) {
        kDebug() << "Command: OTO" << endl;
        i += strlen( "OTO" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->to();
          body.append( str );
        }

      } else if ( cmd.startsWith( "OFROMADDR" ) ) {
        kDebug() << "Command: OFROMADDR" << endl;
        i += strlen( "OFROMADDR" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->from();
          body.append( str );
        }

      } else if ( cmd.startsWith( "OFROMNAME" ) ) {
        kDebug() << "Command: OFROMNAME" << endl;
        i += strlen( "OFROMNAME" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->fromStrip();
          body.append( str );
        }

      } else if ( cmd.startsWith( "OFROMFNAME" ) ) {
        kDebug() << "Command: OFROMFNAME" << endl;
        i += strlen( "OFROMFNAME" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->fromStrip();
          body.append( getFName( str ) );
        }

      } else if ( cmd.startsWith( "OFROMLNAME" ) ) {
        kDebug() << "Command: OFROMLNAME" << endl;
        i += strlen( "OFROMLNAME" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->fromStrip();
          body.append( getLName( str ) );
        }

      } else if ( cmd.startsWith( "OFULLSUBJECT" ) ) {
        kDebug() << "Command: OFULLSUBJECT" << endl;
        i += strlen( "OFULLSUBJECT" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->subject();
          body.append( str );
        }

      } else if ( cmd.startsWith( "OFULLSUBJ" ) ) {
        kDebug() << "Command: OFULLSUBJ" << endl;
        i += strlen( "OFULLSUBJ" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->subject();
          body.append( str );
        }

      } else if ( cmd.startsWith( "OMSGID" ) ) {
        kDebug() << "Command: OMSGID" << endl;
        i += strlen( "OMSGID" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->id();
          body.append( str );
        }

      } else if ( cmd.startsWith( "DATEEN" ) ) {
        kDebug() << "Command: DATEEN" << endl;
        i += strlen( "DATEEN" );
        QDateTime date = QDateTime::currentDateTime();
        KLocale locale( "C" );
        QString str = locale.formatDate( date.date(), KLocale::LongDate );
        body.append( str );

      } else if ( cmd.startsWith( "DATESHORT" ) ) {
        kDebug() << "Command: DATESHORT" << endl;
        i += strlen( "DATESHORT" );
        QDateTime date = QDateTime::currentDateTime();
        QString str = KGlobal::locale()->formatDate( date.date(), KLocale::ShortDate );
        body.append( str );

      } else if ( cmd.startsWith( "DATE" ) ) {
        kDebug() << "Command: DATE" << endl;
        i += strlen( "DATE" );
        QDateTime date = QDateTime::currentDateTime();
        QString str = KGlobal::locale()->formatDate( date.date(), KLocale::LongDate );
        body.append( str );

      } else if ( cmd.startsWith( "DOW" ) ) {
        kDebug() << "Command: DOW" << endl;
        i += strlen( "DOW" );
        QDateTime date = QDateTime::currentDateTime();
        QString str = KGlobal::locale()->calendar()->weekDayName( date.date(), false );
        body.append( str );

      } else if ( cmd.startsWith( "TIMELONGEN" ) ) {
        kDebug() << "Command: TIMELONGEN" << endl;
        i += strlen( "TIMELONGEN" );
        QDateTime date = QDateTime::currentDateTime();
        KLocale locale( "C");
        QString str = locale.formatTime( date.time(), true );
        body.append( str );

      } else if ( cmd.startsWith( "TIMELONG" ) ) {
        kDebug() << "Command: TIMELONG" << endl;
        i += strlen( "TIMELONG" );
        QDateTime date = QDateTime::currentDateTime();
        QString str = KGlobal::locale()->formatTime( date.time(), true );
        body.append( str );

      } else if ( cmd.startsWith( "TIME" ) ) {
        kDebug() << "Command: TIME" << endl;
        i += strlen( "TIME" );
        QDateTime date = QDateTime::currentDateTime();
        QString str = KGlobal::locale()->formatTime( date.time(), false );
        body.append( str );

      } else if ( cmd.startsWith( "ODATEEN" ) ) {
        kDebug() << "Command: ODATEEN" << endl;
        i += strlen( "ODATEEN" );
        if ( mOrigMsg ) {
          QDateTime date;
          date.setTime_t( mOrigMsg->date() );
          KLocale locale( "C");
          QString str = locale.formatDate( date.date(), KLocale::LongDate );
          body.append( str );
        }

      } else if ( cmd.startsWith( "ODATESHORT") ) {
        kDebug() << "Command: ODATESHORT" << endl;
        i += strlen( "ODATESHORT");
        if ( mOrigMsg ) {
          QDateTime date;
          date.setTime_t( mOrigMsg->date() );
          QString str = KGlobal::locale()->formatDate( date.date(), KLocale::ShortDate );
          body.append( str );
        }

      } else if ( cmd.startsWith( "ODATE") ) {
        kDebug() << "Command: ODATE" << endl;
        i += strlen( "ODATE");
        if ( mOrigMsg ) {
          QDateTime date;
          date.setTime_t( mOrigMsg->date() );
          QString str = KGlobal::locale()->formatDate( date.date(), KLocale::LongDate );
          body.append( str );
        }

      } else if ( cmd.startsWith( "ODOW") ) {
        kDebug() << "Command: ODOW" << endl;
        i += strlen( "ODOW");
        if ( mOrigMsg ) {
          QDateTime date;
          date.setTime_t( mOrigMsg->date() );
          QString str = KGlobal::locale()->calendar()->weekDayName( date.date(), false );
          body.append( str );
        }

      } else if ( cmd.startsWith( "OTIMELONGEN") ) {
        kDebug() << "Command: OTIMELONGEN" << endl;
        i += strlen( "OTIMELONGEN");
        if ( mOrigMsg ) {
          QDateTime date;
          date.setTime_t( mOrigMsg->date() );
          KLocale locale( "C");
          QString str = locale.formatTime( date.time(), true );
          body.append( str );
        }

      } else if ( cmd.startsWith( "OTIMELONG") ) {
        kDebug() << "Command: OTIMELONG" << endl;
        i += strlen( "OTIMELONG");
        if ( mOrigMsg ) {
          QDateTime date;
          date.setTime_t( mOrigMsg->date() );
          QString str = KGlobal::locale()->formatTime( date.time(), true );
          body.append( str );
        }

      } else if ( cmd.startsWith( "OTIME") ) {
        kDebug() << "Command: OTIME" << endl;
        i += strlen( "OTIME");
        if ( mOrigMsg ) {
          QDateTime date;
          date.setTime_t( mOrigMsg->date() );
          QString str = KGlobal::locale()->formatTime( date.time(), false );
          body.append( str );
        }

      } else if ( cmd.startsWith( "BLANK" ) ) {
        // do nothing
        kDebug() << "Command: BLANK" << endl;
        i += strlen( "BLANK" );

      } else if ( cmd.startsWith( "NOP" ) ) {
        // do nothing
        kDebug() << "Command: NOP" << endl;
        i += strlen( "NOP" );

      } else if ( cmd.startsWith( "CLEAR" ) ) {
        // clear body buffer; not too useful yet
        kDebug() << "Command: CLEAR" << endl;
        i += strlen( "CLEAR" );
        body = "";
        mMsg->setCursorPos( 0 );

      } else if ( cmd.startsWith( "DEBUGOFF" ) ) {
        // turn off debug
        kDebug() << "Command: DEBUGOFF" << endl;
        i += strlen( "DEBUGOFF" );
        mDebug = false;

      } else if ( cmd.startsWith( "DEBUG" ) ) {
        // turn on debug
        kDebug() << "Command: DEBUG" << endl;
        i += strlen( "DEBUG" );
        mDebug = true;

      } else if ( cmd.startsWith( "CURSOR" ) ) {
        // turn on debug
        kDebug() << "Command: CURSOR" << endl;
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

  // kDebug() << "Message body: " << body << endl;

  if ( mAppend ) {
    QByteArray msg_body = mMsg->body();
    msg_body.append( body.utf8() );
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

  // kDebug() << "Trying to find template for mode " << mode << endl;

  QString tmpl;

  if ( !mFolder ) { // find folder message belongs to
    mFolder = mMsg->parent();
    if ( !mFolder ) {
      if ( mOrigMsg ) {
        mFolder = mOrigMsg->parent();
      }
      if ( !mFolder ) {
        kDebug(5006) << "Oops! No folder for message" << endl;
      }
    }
  }
  kDebug(5006) << "Folder found: " << mFolder << endl;

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
        kDebug(5006) << "Unknown message mode: " << mMode << endl;
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
      kDebug(5006) << "Oops! No identity for message" << endl;
    }
  }
  kDebug(5006) << "Identity found: " << mIdentity << endl;

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
      kDebug(5006) << "Unknown message mode: " << mMode << endl;
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
    kDebug(5006) << "Unknown message mode: " << mMode << endl;
    return "";
  }

  mQuoteString = GlobalSettings::self()->quoteString();
  return tmpl;
}

QString TemplateParser::pipe( const QString &cmd, const QString &buf )
{
  mPipeOut = "";
  mPipeErr = "";
  mPipeRc = 0;

  K3Process proc;
  QByteArray data = buf.local8Bit();

  // kDebug() << "Command data: " << data << endl;

  proc << KShell::splitArgs( cmd, KShell::TildeExpand );
  proc.setUseShell( true );
  connect( &proc, SIGNAL( receivedStdout( K3Process *, char *, int ) ),
           this, SLOT( onReceivedStdout( K3Process *, char *, int ) ) );
  connect( &proc, SIGNAL( receivedStderr( K3Process *, char *, int ) ),
           this, SLOT( onReceivedStderr( K3Process *, char *, int ) ) );
  connect( &proc, SIGNAL( wroteStdin( K3Process * ) ),
           this, SLOT( onWroteStdin( K3Process * ) ) );

  if ( proc.start( K3Process::NotifyOnExit, K3Process::All ) ) {

    bool pipe_filled = proc.writeStdin( data, data.length() );
    if ( pipe_filled ) {
      proc.closeStdin();

      bool exited = proc.wait( PipeTimeout );
      if ( exited ) {

        if ( proc.normalExit() ) {

          mPipeRc = proc.exitStatus();
          if ( mPipeRc != 0 && mDebug ) {
            if ( mPipeErr.isEmpty() ) {
              KMessageBox::error( 0,
                                  i18n( "Pipe command exit with status %1: %2",
                                        mPipeRc, cmd ) );
            } else {
              KMessageBox::detailedError( 0,
                                          i18n( "Pipe command exit with status %1: %2",
                                                mPipeRc, cmd ), mPipeErr );
            }
          }

        } else {

          mPipeRc = -( proc.exitSignal() );
          if ( mPipeRc != 0 && mDebug ) {
            if ( mPipeErr.isEmpty() ) {
              KMessageBox::error( 0,
                                  i18n( "Pipe command killed by signal %1: %2",
                                        -(mPipeRc), cmd ) );
            } else {
              KMessageBox::detailedError( 0,
                                          i18n( "Pipe command killed by signal %1: %2",
                                                -(mPipeRc), cmd ), mPipeErr );
            }
          }
        }

      } else {
        // process does not exited after TemplateParser::PipeTimeout seconds, kill it
        proc.kill();
        proc.detach();
        if ( mDebug ) {
          KMessageBox::error( 0,
                              i18n( "Pipe command did not finish within %1 seconds: %2",
                              PipeTimeout, cmd ) );
        }
      }

    } else {
      // can`t write to stdin of process
      proc.kill();
      proc.detach();
      if ( mDebug ) {
        if ( mPipeErr.isEmpty() ) {
          KMessageBox::error( 0,
                              i18n( "Cannot write to process stdin: %1", cmd ) );
        } else {
          KMessageBox::detailedError( 0,
                                      i18n( "Cannot write to process stdin: %1",
                                            cmd ), mPipeErr );
        }
      }
    }

  } else if ( mDebug ) {
    KMessageBox::error( 0,
                        i18n( "Cannot start pipe command from template: %1",
                              cmd ) );
  }

  return mPipeOut;
}

void TemplateParser::onProcessExited( K3Process *proc )
{
  Q_UNUSED( proc );
  // do nothing for now
}

void TemplateParser::onReceivedStdout( K3Process *proc, char *buffer, int buflen )
{
  Q_UNUSED( proc );
  mPipeOut += QString::fromLocal8Bit( buffer, buflen );
}

void TemplateParser::onReceivedStderr( K3Process *proc, char *buffer, int buflen )
{
  Q_UNUSED( proc );
  mPipeErr += QString::fromLocal8Bit( buffer, buflen );
}

void TemplateParser::onWroteStdin( K3Process *proc )
{
  proc->closeStdin();
}

} // namespace KMail

#include "templateparser.moc"
