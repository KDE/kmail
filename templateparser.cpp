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
#include <kmime_util.h>
#include <kglobal.h>
#include <kprocess.h>
#include <qregexp.h>
#include <qfile.h>
#include <kmessagebox.h>
#include <kshell.h>
#include <qfileinfo.h>

#include "kmmessage.h"
#include "kmmsgbase.h"
#include "kmfolder.h"
#include "templatesconfiguration_kfg.h"
#include "globalsettings_base.h"

#include "templateparser.h"

#define QUOTERE "\"(.*)\""

TemplateParser::TemplateParser( KMMessage *amsg, const Mode amode,
                                const QString aselection,
                                bool asmartQuote, bool anoQuote,
                                bool aallowDecryption, bool aselectionIsBody ) :
  mode( amode ), selection( aselection ),
  smartQuote( asmartQuote ), noQuote( anoQuote ),
  allowDecryption( aallowDecryption ), selectionIsBody( aselectionIsBody ),
  debug( false ), quoteString( "> " )
{
  msg = amsg;
};

int TemplateParser::parseQuotes( const QString prefix, const QString str,
                                 QString &quote ) const
{
  int pos = prefix.length();
  int len;
  int str_len = str.length();
  QChar qc = '"';
  QChar prev = 0;

  pos++;
  len = pos;

  while ( pos < str_len ) {
    QChar c = str[pos];

    pos++;
    len++;

    if ( prev ) {
      quote.append( c );
      prev = 0;
    } else {
      if ( c == '\\' ) {
        prev = c;
      } else if ( c == qc ) {
        break;
      } else {
        quote.append( c );
      };
    };
  };

  return len;
};

QString TemplateParser::getFName( QString str )
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
      };
    };
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
      };
    };
  } else {
    unsigned int i;
    for ( i = 0; i < str.length(); ++i ) {
      QChar c = str[i];
      if ( c.isLetterOrNumber() ) {
        res.append( c );
      } else {
        break;
      };
    };
  };
  return res;
};

QString TemplateParser::getLName( QString str )
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
      };
    };
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
        };
      };
    };
  };
  return res;
};

void TemplateParser::process( KMMessage *aorig_msg, KMFolder *afolder )
{
  orig_msg = aorig_msg;
  folder = afolder;
  QString tmpl = findTemplate();
  QString body;
  int tmpl_len = tmpl.length();

  // kdDebug() << "Mode " << mode << ", template found: " << tmpl << endl;

  bool dnl = false;
  for ( int i = 0; i < tmpl_len; ++i ) {
    QChar c = tmpl[i];
    // kdDebug() << "Next char: " << c << endl;
    if ( c == '%' ) {
      QString cmd = tmpl.mid( i + 1 );

      if ( cmd.startsWith( "-" ) ) {
        // dnl
        kdDebug() << "Command: -" << endl;
        dnl = true;
        i += 1;

      } else if ( cmd.startsWith( "REM=" ) ) {
        // comments
        kdDebug() << "Command: REM=" << endl;
        QString q;
        int len = parseQuotes( "REM=", cmd, q );
        i += len;

      } else if ( cmd.startsWith( "INSERT=" ) ) {
        // insert content of specified file as is
        kdDebug() << "Command: INSERT=" << endl;
        QString q;
        int len = parseQuotes( "INSERT=", cmd, q );
        i += len;
        QString path = KShell::tildeExpand( q );
        QFileInfo finfo( path );
        if (finfo.isRelative() ) {
          path = KShell::homeDir( "" );
          path += '/';
          path += q;
        };
        QFile file( path );
        if ( file.open( IO_ReadOnly ) ) {
          QByteArray content = file.readAll();
          QString str = QString::fromLocal8Bit( content, content.size() );
          body.append( str );
        } else if ( debug ) {
          KMessageBox::error( 0,
                              i18n( "Can`t insert content from file %1: %2" ).
                              arg( path ).arg( file.errorString() ) );
        };

      } else if ( cmd.startsWith( "SYSTEM=" ) ) {
        // insert content of specified file as is
        kdDebug() << "Command: SYSTEM=" << endl;
        QString q;
        int len = parseQuotes( "SYSTEM=", cmd, q );
        i += len;
        QString pipe_cmd = q;
        QString str = pipe( pipe_cmd, "" );
        body.append( str );

      } else if ( cmd.startsWith( "PUT=" ) ) {
        // insert content of specified file as is
        kdDebug() << "Command: PUT=" << endl;
        QString q;
        int len = parseQuotes( "PUT=", cmd, q );
        i += len;
        QString path = KShell::tildeExpand( q );
        QFileInfo finfo( path );
        if (finfo.isRelative() ) {
          path = KShell::homeDir( "" );
          path += '/';
          path += q;
        };
        QFile file( path );
        if ( file.open( IO_ReadOnly ) ) {
          QByteArray content = file.readAll();
          body.append( QString::fromLocal8Bit( content, content.size() ) );
        } else if ( debug ) {
          KMessageBox::error( 0,
                              i18n( "Can`t insert content from file %1: %2").
                              arg( path ).arg(file.errorString() ));
        };

      } else if ( cmd.startsWith( "QUOTEPIPE=" ) ) {
        // pipe message body throw command and insert it as quotation
        kdDebug() << "Command: QUOTEPIPE=" << endl;
        QString q;
        int len = parseQuotes( "QUOTEPIPE=", cmd, q );
        i += len;
        QString pipe_cmd = q;
        if ( orig_msg && !noQuote ) {
          QString str = pipe( pipe_cmd, selection );
          QString quote = orig_msg->asQuotedString( "", quoteString, str,
                                                    smartQuote, allowDecryption );
          body.append( quote );
        };

      } else if ( cmd.startsWith( "QUOTE" ) ) {
        kdDebug() << "Command: QUOTE" << endl;
        i += strlen( "QUOTE" );
        if ( orig_msg && !noQuote ) {
          QString quote = orig_msg->asQuotedString( "", quoteString, selection,
                                                    smartQuote, allowDecryption );
          body.append( quote );
        };

      } else if ( cmd.startsWith( "QHEADERS" ) ) {
        kdDebug() << "Command: QHEADERS" << endl;
        i += strlen( "QHEADERS" );
        if ( orig_msg && !noQuote ) {
          QString quote = orig_msg->asQuotedString( "", quoteString,
                                                    orig_msg->headerAsSendableString(),
                                                    smartQuote, false );
          body.append( quote );
        };

      } else if ( cmd.startsWith( "HEADERS" ) ) {
        kdDebug() << "Command: HEADERS" << endl;
        i += strlen( "HEADERS" );
        if ( orig_msg && !noQuote ) {
          QString str = orig_msg->headerAsSendableString();
          body.append( str );
        };

      } else if ( cmd.startsWith( "TEXTPIPE=" ) ) {
        // pipe message body throw command and insert it as is
        kdDebug() << "Command: TEXTPIPE=" << endl;
        QString q;
        int len = parseQuotes( "TEXTPIPE=", cmd, q );
        i += len;
        QString pipe_cmd = q;
        if ( orig_msg ) {
          QString str = pipe(pipe_cmd, orig_msg->body() );
          body.append( str );
        };

      } else if ( cmd.startsWith( "MSGPIPE=" ) ) {
        // pipe full message throw command and insert result as is
        kdDebug() << "Command: MSGPIPE=" << endl;
        QString q;
        int len = parseQuotes( "MSGPIPE=", cmd, q );
        i += len;
        QString pipe_cmd = q;
        if ( orig_msg ) {
          QString str = pipe(pipe_cmd, orig_msg->asString() );
          body.append( str );
        };

      } else if ( cmd.startsWith( "BODYPIPE=" ) ) {
        // pipe message body generated so far throw command and insert result as is
        kdDebug() << "Command: BODYPIPE=" << endl;
        QString q;
        int len = parseQuotes( "BODYPIPE=", cmd, q );
        i += len;
        QString pipe_cmd = q;
        QString str = pipe( pipe_cmd, body );
        body.append( str );

      } else if ( cmd.startsWith( "CLEARPIPE=" ) ) {
        // pipe message body generated so far throw command and
        // insert result as is replacing current body
        kdDebug() << "Command: CLEARPIPE=" << endl;
        QString q;
        int len = parseQuotes( "CLEARPIPE=", cmd, q );
        i += len;
        QString pipe_cmd = q;
        QString str = pipe( pipe_cmd, body );
        body = str;
        msg->setCursorPos( 0 );

      } else if ( cmd.startsWith( "TEXT" ) ) {
        kdDebug() << "Command: TEXT" << endl;
        i += strlen( "TEXT" );
        if ( orig_msg ) {
          QString quote = orig_msg->body();
          body.append( quote );
        };

      } else if ( cmd.startsWith( "OTEXTSIZE" ) ) {
        kdDebug() << "Command: OTEXTSIZE" << endl;
        i += strlen( "OTEXTSIZE" );
        if ( orig_msg ) {
          QString str = QString( "%1" ).arg( orig_msg->body().length() );
          body.append( str );
        };

      } else if ( cmd.startsWith( "OTEXT" ) ) {
        kdDebug() << "Command: OTEXT" << endl;
        i += strlen( "OTEXT" );
        if ( orig_msg ) {
          QString quote = orig_msg->body();
          body.append( quote );
        };

      } else if ( cmd.startsWith( "TOADDR" ) ) {
        kdDebug() << "Command: TOADDR" << endl;
        i += strlen( "TOADDR" );
        QString str = msg->to();
        body.append( str );

      } else if ( cmd.startsWith( "TONAME" ) ) {
        kdDebug() << "Command: TONAME" << endl;
        i += strlen( "TONAME" );
        QString str = msg->toStrip();
        body.append( str );

      } else if ( cmd.startsWith( "TOFNAME" ) ) {
        kdDebug() << "Command: TOFNAME" << endl;
        i += strlen( "TOFNAME" );
        QString str = msg->toStrip();
        body.append( getFName( str ) );

      } else if ( cmd.startsWith( "TOLNAME" ) ) {
        kdDebug() << "Command: TOLNAME" << endl;
        i += strlen( "TOLNAME" );
        QString str = msg->toStrip();
        body.append( getLName( str ) );

      } else if ( cmd.startsWith( "TOLIST" ) ) {
        kdDebug() << "Command: TOLIST" << endl;
        i += strlen( "TOLIST" );
        QString str = msg->to();
        body.append( str );

      } else if ( cmd.startsWith( "FROMADDR" ) ) {
        kdDebug() << "Command: FROMADDR" << endl;
        i += strlen( "FROMADDR" );
        QString str = msg->from();
        body.append( str );

      } else if ( cmd.startsWith( "FROMNAME" ) ) {
        kdDebug() << "Command: FROMNAME" << endl;
        i += strlen( "FROMNAME" );
        QString str = msg->fromStrip();
        body.append( str );

      } else if ( cmd.startsWith( "FROMFNAME" ) ) {
        kdDebug() << "Command: FROMFNAME" << endl;
        i += strlen( "FROMFNAME" );
        QString str = msg->fromStrip();
        body.append( getFName( str ) );

      } else if ( cmd.startsWith( "FROMLNAME" ) ) {
        kdDebug() << "Command: FROMLNAME" << endl;
        i += strlen( "FROMLNAME" );
        QString str = msg->fromStrip();
        body.append( getLName( str ) );

      } else if ( cmd.startsWith( "FULLSUBJECT" ) ) {
        kdDebug() << "Command: FULLSUBJECT" << endl;
        i += strlen( "FULLSUBJECT" );
        QString str = msg->subject();
        body.append( str );

      } else if ( cmd.startsWith( "FULLSUBJ" ) ) {
        kdDebug() << "Command: FULLSUBJ" << endl;
        i += strlen( "FULLSUBJ" );
        QString str = msg->subject();
        body.append( str );

      } else if ( cmd.startsWith( "MSGID" ) ) {
        kdDebug() << "Command: MSGID" << endl;
        i += strlen( "MSGID" );
        QString str = msg->id();
        body.append( str );

      } else if ( cmd.startsWith( "OHEADER=" ) ) {
        // insert specified content of header from original message
        kdDebug() << "Command: OHEADER=" << endl;
        QString q;
        int len = parseQuotes( "OHEADER=", cmd, q );
        i += len;
        if ( orig_msg ) {
          QString hdr = q;
          QString str = orig_msg->headerFields(hdr.local8Bit() ).join( ", " );
          body.append( str );
        };

      } else if ( cmd.startsWith( "HEADER=" ) ) {
        // insert specified content of header from current message
        kdDebug() << "Command: HEADER=" << endl;
        QString q;
        int len = parseQuotes( "HEADER=", cmd, q );
        i += len;
        QString hdr = q;
        QString str = msg->headerFields(hdr.local8Bit() ).join( ", " );
        body.append( str );

      } else if ( cmd.startsWith( "HEADER( " ) ) {
        // insert specified content of header from current message
        kdDebug() << "Command: HEADER( " << endl;
        QRegExp re = QRegExp( "^HEADER\\((.+)\\)" );
        re.setMinimal( true );
        int res = re.search( cmd );
        if ( res != 0 ) {
          // something wrong
          i += strlen( "HEADER( " );
        } else {
          i += re.matchedLength();
          QString hdr = re.cap( 1 );
          QString str = msg->headerFields( hdr.local8Bit() ).join( ", " );
          body.append( str );
        };

      } else if ( cmd.startsWith( "OTOADDR" ) ) {
        kdDebug() << "Command: OTOADDR" << endl;
        i += strlen( "OTOADDR" );
        if ( orig_msg ) {
          QString str = orig_msg->to();
          body.append( str );
        };

      } else if ( cmd.startsWith( "OTONAME" ) ) {
        kdDebug() << "Command: OTONAME" << endl;
        i += strlen( "OTONAME" );
        if ( orig_msg ) {
          QString str = orig_msg->toStrip();
          body.append( str );
        };

      } else if ( cmd.startsWith( "OTOFNAME" ) ) {
        kdDebug() << "Command: OTOFNAME" << endl;
        i += strlen( "OTOFNAME" );
        if ( orig_msg ) {
          QString str = orig_msg->toStrip();
          body.append( getFName( str ) );
        };

      } else if ( cmd.startsWith( "OTOLNAME" ) ) {
        kdDebug() << "Command: OTOLNAME" << endl;
        i += strlen( "OTOLNAME" );
        if ( orig_msg ) {
          QString str = orig_msg->toStrip();
          body.append( getLName( str ) );
        };

      } else if ( cmd.startsWith( "OTOLIST" ) ) {
        kdDebug() << "Command: OTOLIST" << endl;
        i += strlen( "OTOLIST" );
        if ( orig_msg ) {
          QString str = orig_msg->to();
          body.append( str );
        };

      } else if ( cmd.startsWith( "OTO" ) ) {
        kdDebug() << "Command: OTO" << endl;
        i += strlen( "OTO" );
        if ( orig_msg ) {
          QString str = orig_msg->to();
          body.append( str );
        };

      } else if ( cmd.startsWith( "OFROMADDR" ) ) {
        kdDebug() << "Command: OFROMADDR" << endl;
        i += strlen( "OFROMADDR" );
        if ( orig_msg ) {
          QString str = orig_msg->from();
          body.append( str );
        };

      } else if ( cmd.startsWith( "OFROMNAME" ) ) {
        kdDebug() << "Command: OFROMNAME" << endl;
        i += strlen( "OFROMNAME" );
        if ( orig_msg ) {
          QString str = orig_msg->fromStrip();
          body.append( str );
        };

      } else if ( cmd.startsWith( "OFROMFNAME" ) ) {
        kdDebug() << "Command: OFROMFNAME" << endl;
        i += strlen( "OFROMFNAME" );
        if ( orig_msg ) {
          QString str = orig_msg->fromStrip();
          body.append( getFName( str ) );
        };

      } else if ( cmd.startsWith( "OFROMLNAME" ) ) {
        kdDebug() << "Command: OFROMLNAME" << endl;
        i += strlen( "OFROMLNAME" );
        if ( orig_msg ) {
          QString str = orig_msg->fromStrip();
          body.append( getLName( str ) );
        };

      } else if ( cmd.startsWith( "OFULLSUBJECT" ) ) {
        kdDebug() << "Command: OFULLSUBJECT" << endl;
        i += strlen( "OFULLSUBJECT" );
        if ( orig_msg ) {
          QString str = orig_msg->subject();
          body.append( str );
        };

      } else if ( cmd.startsWith( "OFULLSUBJ" ) ) {
        kdDebug() << "Command: OFULLSUBJ" << endl;
        i += strlen( "OFULLSUBJ" );
        if ( orig_msg ) {
          QString str = orig_msg->subject();
          body.append( str );
        };

      } else if ( cmd.startsWith( "OMSGID" ) ) {
        kdDebug() << "Command: OMSGID" << endl;
        i += strlen( "OMSGID" );
        if ( orig_msg ) {
          QString str = orig_msg->id();
          body.append( str );
        };

      } else if ( cmd.startsWith( "DATEEN" ) ) {
        kdDebug() << "Command: DATEEN" << endl;
        i += strlen( "DATEEN" );
        QDateTime date = QDateTime::currentDateTime();
        KLocale locale( "C" );
        QString str = QString( "%1, %2 %3, %4" )
          .arg(locale.calendar()->weekDayName(date.date(), false) )
          .arg(locale.calendar()->monthName(date.date(), false) )
          .arg(date.date().day() )
          .arg(date.date().year() );
        body.append( str );

      } else if ( cmd.startsWith( "DATESHORT" ) ) {
        kdDebug() << "Command: DATESHORT" << endl;
        i += strlen( "DATESHORT" );
        QDateTime date = QDateTime::currentDateTime();
        QString str = date.date().toString(Qt::LocalDate);
        body.append( str );

      } else if ( cmd.startsWith( "DATE" ) ) {
        kdDebug() << "Command: DATE" << endl;
        i += strlen( "DATE" );
        QDateTime date = QDateTime::currentDateTime();
        QString str = date.date().toString(Qt::TextDate);
        body.append( str );

      } else if ( cmd.startsWith( "DOW" ) ) {
        kdDebug() << "Command: DOW" << endl;
        i += strlen( "DOW" );
        QDateTime date = QDateTime::currentDateTime();
        QString str = QDate::longDayName(date.date().dayOfWeek() );
        body.append( str );

      } else if ( cmd.startsWith( "TIMELONGEN" ) ) {
        kdDebug() << "Command: TIMELONGEN" << endl;
        i += strlen( "TIMELONGEN" );
        QDateTime date = QDateTime::currentDateTime();
        QString str = date.time().toString( "hh:mm:ss AP" );
        body.append( str );

      } else if ( cmd.startsWith( "TIMELONG" ) ) {
        kdDebug() << "Command: TIMELONG" << endl;
        i += strlen( "TIMELONG" );
        QDateTime date = QDateTime::currentDateTime();
        QString str = date.time().toString(Qt::TextDate);
        body.append( str );

      } else if ( cmd.startsWith( "TIME" ) ) {
        kdDebug() << "Command: TIME" << endl;
        i += strlen( "TIME" );
        QDateTime date = QDateTime::currentDateTime();
        QString str = date.time().toString(Qt::LocalDate);
        body.append( str );

      } else if ( cmd.startsWith( "ODATEEN" ) ) {
        kdDebug() << "Command: ODATEEN" << endl;
        i += strlen( "ODATEEN" );
        if ( orig_msg ) {
          QDateTime date;
          date.setTime_t( orig_msg->date() );
          KLocale locale( "C");
          QString str = QString( "%1, %2 %3, %4")
            .arg(locale.calendar()->weekDayName(date.date(), false) )
            .arg(locale.calendar()->monthName(date.date(), false) )
            .arg(date.date().day() )
            .arg(date.date().year() );
          body.append( str );
        };

      } else if ( cmd.startsWith( "ODATESHORT") ) {
        kdDebug() << "Command: ODATESHORT" << endl;
        i += strlen( "ODATESHORT");
        if ( orig_msg ) {
          QDateTime date;
          date.setTime_t( orig_msg->date() );
          QString str = date.date().toString(Qt::LocalDate);
          body.append( str );
        };

      } else if ( cmd.startsWith( "ODATE") ) {
        kdDebug() << "Command: ODATE" << endl;
        i += strlen( "ODATE");
        if ( orig_msg ) {
          QDateTime date;
          date.setTime_t( orig_msg->date() );
          QString str = date.date().toString(Qt::TextDate);
          body.append( str );
        };

      } else if ( cmd.startsWith( "ODOW") ) {
        kdDebug() << "Command: ODOW" << endl;
        i += strlen( "ODOW");
        if ( orig_msg ) {
          QDateTime date;
          date.setTime_t( orig_msg->date() );
          QString str = QDate::longDayName(date.date().dayOfWeek() );
          body.append( str );
        };

      } else if ( cmd.startsWith( "OTIMELONGEN") ) {
        kdDebug() << "Command: OTIMELONGEN" << endl;
        i += strlen( "OTIMELONGEN");
        if ( orig_msg ) {
          QDateTime date;
          date.setTime_t( orig_msg->date() );
          QString str = date.time().toString( "hh:mm:ss AP");
          body.append( str );
        };

      } else if ( cmd.startsWith( "OTIMELONG") ) {
        kdDebug() << "Command: OTIMELONG" << endl;
        i += strlen( "OTIMELONG");
        if ( orig_msg ) {
          QDateTime date;
          date.setTime_t( orig_msg->date() );
          QString str = date.time().toString(Qt::TextDate);
          body.append( str );
        };

      } else if ( cmd.startsWith( "OTIME") ) {
        kdDebug() << "Command: OTIME" << endl;
        i += strlen( "OTIME");
        if ( orig_msg ) {
          QDateTime date;
          date.setTime_t( orig_msg->date() );
          QString str = date.time().toString( Qt::LocalDate );
          body.append( str );
        };

      } else if ( cmd.startsWith( "BLANK" ) ) {
        // do nothing
        kdDebug() << "Command: BLANK" << endl;
        i += strlen( "BLANK" );

      } else if ( cmd.startsWith( "NOP" ) ) {
        // do nothing
        kdDebug() << "Command: NOP" << endl;
        i += strlen( "NOP" );

      } else if ( cmd.startsWith( "CLEAR" ) ) {
        // clear body buffer; not too useful yet
        kdDebug() << "Command: CLEAR" << endl;
        i += strlen( "CLEAR" );
        body = "";
        msg->setCursorPos( 0 );

      } else if ( cmd.startsWith( "DEBUGOFF" ) ) {
        // turn off debug
        kdDebug() << "Command: DEBUGOFF" << endl;
        i += strlen( "DEBUGOFF" );
        debug = false;

      } else if ( cmd.startsWith( "DEBUG" ) ) {
        // turn on debug
        kdDebug() << "Command: DEBUG" << endl;
        i += strlen( "DEBUG" );
        debug = true;

      } else if ( cmd.startsWith( "CURSOR" ) ) {
        // turn on debug
        kdDebug() << "Command: CURSOR" << endl;
        i += strlen( "CURSOR" );
        msg->setCursorPos( body.length() );

      } else {
        // wrong command, do nothing
        body.append( c );
      };

    } else if ( dnl && ( c == '\n' || c == '\r') ) {
      // skip
      if ( ( c == '\n' && tmpl[i + 1] == '\r' ) ||
           ( c == '\r' && tmpl[i + 1] == '\n' ) ) {
        // skip one more
        i += 1;
      };
      dnl = false;
    } else {
      body.append( c );
    };
  };

  // kdDebug() << "Message body: " << body << endl;

  msg->setBody( body.utf8() );
};

QString TemplateParser::findTemplate()
{

  // kdDebug() << "Trying to find template for mode " << mode << endl;

  QString tmpl;

  if ( !folder ) {
    folder = msg->parent();
    if ( !folder ) {
      if ( orig_msg ) {
        folder = orig_msg->parent();
      };
      if ( !folder ) {
        kdDebug() << "Oops! No folder for message" << endl;
        return "";
      };
    };
  };

  QString fid = folder->idString();
  Templates fconf( fid );
  if ( fconf.useCustomTemplates() ) {
    switch( mode ) {
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
      kdDebug() << "Unknown message mode: " << mode << endl;
      return "";
    };
    quoteString = fconf.quoteString();
    if ( !tmpl.isEmpty() ) {
      return tmpl;
    };
  };
  switch( mode ) {
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
    kdDebug() << "Unknown message mode: " << mode << endl;
    return "";
  };
  quoteString = GlobalSettings::self()->quoteString();
  return tmpl;
};

QString TemplateParser::pipe( const QString cmd, const QString buf )
{
  pipe_out = "";
  pipe_err = "";
  pipe_rc = 0;

  KProcess proc;
  QCString data = buf.local8Bit();

  // kdDebug() << "Command data: " << data << endl;

  proc << KShell::splitArgs( cmd, KShell::TildeExpand );
  proc.setUseShell( true );
  connect( &proc, SIGNAL( receivedStdout( KProcess *, char *, int ) ),
           this, SLOT( onReceivedStdout( KProcess *, char *, int ) ) );
  connect( &proc, SIGNAL( receivedStderr( KProcess *, char *, int ) ),
           this, SLOT( onReceivedStderr( KProcess *, char *, int ) ) );
  connect( &proc, SIGNAL( wroteStdin( KProcess * ) ),
           this, SLOT( onWroteStdin( KProcess * ) ) );

  if ( proc.start( KProcess::NotifyOnExit, KProcess::All ) ) {

    bool pipe_filled = proc.writeStdin( data, data.length() );
    if ( pipe_filled ) {
      proc.closeStdin();

      bool exited = proc.wait( PipeTimeout );
      if ( exited ) {

        if ( proc.normalExit() ) {

          pipe_rc = proc.exitStatus();
          if ( pipe_rc != 0 && debug ) {
            if ( pipe_err.isEmpty() ) {
              KMessageBox::error( 0,
                                  i18n( "Pipe command exit with status %1: %2").
                                  arg( pipe_rc ).arg( cmd ) );
            } else {
              KMessageBox::detailedError( 0,
                                          i18n( "Pipe command exit with status %1: %2" ).
                                          arg( pipe_rc ).arg( cmd ), pipe_err );
            };
          };

        } else {

          pipe_rc = -( proc.exitSignal() );
          if ( pipe_rc != 0 && debug ) {
            if ( pipe_err.isEmpty() ) {
              KMessageBox::error( 0,
                                  i18n( "Pipe command killed by signal %1: %2" ).
                                  arg( -(pipe_rc) ).arg( cmd ) );
            } else {
              KMessageBox::detailedError( 0,
                                          i18n( "Pipe command killed by signal %1: %2" ).
                                          arg( -(pipe_rc) ).arg( cmd ), pipe_err );
            };
          };
        };

      } else {
        // process does not exited after TemplateParser::PipeTimeout seconds, kill it
        proc.kill();
        proc.detach();
        if ( debug ) {
          KMessageBox::error( 0,
                              i18n( "Pipe command does not finished within %1 seconds: %2" ).
                              arg( PipeTimeout ).arg( cmd ) );
        };
      };

    } else {
      // can`t write to stdin of process
      proc.kill();
      proc.detach();
      if ( debug ) {
        if ( pipe_err.isEmpty() ) {
          KMessageBox::error( 0,
                              i18n( "Can`t write to process stdin: %1" ).arg( cmd ) );
        } else {
          KMessageBox::detailedError( 0,
                                      i18n( "Can`t write to process stdin: %1" ).
                                      arg( cmd ), pipe_err );
        };
      };
    };

  } else if ( debug ) {
    KMessageBox::error( 0,
                        i18n( "Can`t start pipe command from template: %1" ).
                        arg( cmd ) );
  };

  return pipe_out;
};

void TemplateParser::onProcessExited( KProcess *proc )
{
  Q_UNUSED( proc );
  // do nothing for now
};

void TemplateParser::onReceivedStdout( KProcess *proc, char *buffer, int buflen )
{
  Q_UNUSED( proc );
  pipe_out += QString::fromLocal8Bit( buffer, buflen );
};

void TemplateParser::onReceivedStderr( KProcess *proc, char *buffer, int buflen )
{
  Q_UNUSED( proc );
  pipe_err += QString::fromLocal8Bit( buffer, buflen );
};

void TemplateParser::onWroteStdin( KProcess *proc )
{
  proc->closeStdin();
};

#include "templateparser.moc"
