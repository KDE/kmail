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
#include "templatesconfiguration.h"
#include "templatesconfiguration_kfg.h"
#include "customtemplates_kfg.h"
#include "globalsettings_base.h"
#include "kmkernel.h"
#include "attachmentcollector.h"
#include "stringutil.h"


#include "messagecore/stringutil.h"
#include "messageviewer/objecttreeparser.h"
#include "messageviewer/objecttreeemptysource.h"
#include "messageviewer/nodehelper.h"
#include "messagehelper.h"

#include <akonadi/collection.h>

#include <libkpgp/kpgpblock.h>

#include <kmime/kmime_message.h>
#include <kmime/kmime_content.h>

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
#include <QWebElement>

namespace KMail {

static const int PipeTimeout = 15 * 1000;

TemplateParser::TemplateParser( const KMime::Message::Ptr &amsg, const Mode amode,
                                bool asmartQuote ) :
  mMode( amode ), mFolder( 0 ), mIdentity( 0 ),
  mSmartQuote( asmartQuote ),
  mAllowDecryption( false ),
  mDebug( false ), mQuoteString( "> " ), mAppend( false ), mOrigRoot( 0 )
{
  mMsg = amsg;
}

void TemplateParser::setSelection( const QString &selection )
{
  mSelection = selection;
}

void TemplateParser::setAllowDecryption( const bool allowDecryption )
{
  mAllowDecryption = allowDecryption;
}

TemplateParser::~TemplateParser()
{
  delete mOrigRoot;
  mOrigRoot = 0;
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

void TemplateParser::process( const KMime::Message::Ptr &aorig_msg, const Akonadi::Collection & afolder, bool append )
{
  if( aorig_msg == 0 ) {
    kDebug() << "aorig_msg == 0!";
    return;
  }
  mAppend = append;
  mOrigMsg = aorig_msg;
  mFolder = afolder;
  QString tmpl = findTemplate();
  if( tmpl.isEmpty()) return;
  return processWithTemplate( tmpl );
}

void TemplateParser::process( const QString &tmplName, const KMime::Message::Ptr &aorig_msg,
                              const Akonadi::Collection &afolder, bool append )
{
  mAppend = append;
  mOrigMsg = aorig_msg;
  mFolder = afolder;
  QString tmpl = findCustomTemplate( tmplName );
  return processWithTemplate( tmpl );
}

void TemplateParser::processWithIdentity( uint uoid, const KMime::Message::Ptr &aorig_msg,
                                          const Akonadi::Collection &afolder, bool append )
{
  mIdentity = uoid;
  return process( aorig_msg, afolder, append );
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
        kDebug() << "Command: -";
        dnl = true;
        i += 1;

      } else if ( cmd.startsWith( QLatin1String("REM=") ) ) {
        // comments
        kDebug() << "Command: REM=";
        QString q;
        int len = parseQuotes( "REM=", cmd, q );
        i += len;

      } else if ( cmd.startsWith( QLatin1String("INSERT=") ) ) {
        // insert content of specified file as is
        kDebug() << "Command: INSERT=";
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
        kDebug() << "Command: SYSTEM=";
        QString q;
        int len = parseQuotes( "SYSTEM=", cmd, q );
        i += len;
        QString pipe_cmd = q;
        QString str = pipe( pipe_cmd, "" );
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("PUT=") ) ) {
        // insert content of specified file as is
        kDebug() << "Command: PUT=";
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
        kDebug() << "Command: QUOTEPIPE=";
        QString q;
        int len = parseQuotes( "QUOTEPIPE=", cmd, q );
        i += len;
        QString pipe_cmd = q;
        if ( mOrigMsg ) {
          QString str =
              pipe( pipe_cmd, messageText( false ) );
          QString quote = asQuotedString( mOrigMsg, mQuoteString, str,
                                          mSmartQuote, mAllowDecryption );
          if ( quote.endsWith( '\n' ) )
            quote.chop( 1 );
          body.append( quote );
        }

      } else if ( cmd.startsWith( QLatin1String("QUOTE") ) ) {
        kDebug() << "Command: QUOTE";
        i += strlen( "QUOTE" );
        if ( mOrigMsg ) {
          QString quote = asQuotedString( mOrigMsg, mQuoteString, messageText( true ),
                                                    mSmartQuote, mAllowDecryption );
          if ( quote.endsWith( '\n' ) )
            quote.chop( 1 );
          body.append( quote );
        }

      } else if ( cmd.startsWith( QLatin1String("QHEADERS") ) ) {
        kDebug() << "Command: QHEADERS";
        i += strlen( "QHEADERS" );
        if ( mOrigMsg ) {
          QString quote = asQuotedString( mOrigMsg, mQuoteString,
                                                    MessageHelper::headerAsSendableString( mOrigMsg ),
                                                    mSmartQuote, false );
          if ( quote.endsWith( '\n' ) )
               quote.chop( 1 );
          body.append( quote );
        }

      } else if ( cmd.startsWith( QLatin1String("HEADERS") ) ) {
        kDebug() << "Command: HEADERS";
        i += strlen( "HEADERS" );
        if ( mOrigMsg ) {
          QString str = MessageHelper::headerAsSendableString( mOrigMsg );
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("TEXTPIPE=") ) ) {
        // pipe message body through command and insert it as is
        kDebug() << "Command: TEXTPIPE=";
        QString q;
        int len = parseQuotes( "TEXTPIPE=", cmd, q );
        i += len;
        QString pipe_cmd = q;
        if ( mOrigMsg ) {
          QString str = pipe(pipe_cmd, messageText( false ) );
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("MSGPIPE=") ) ) {
        // pipe full message through command and insert result as is
        kDebug() << "Command: MSGPIPE=";
        QString q;
        int len = parseQuotes( "MSGPIPE=", cmd, q );
        i += len;
        QString pipe_cmd = q;
        if ( mOrigMsg ) {
          QString str = pipe(pipe_cmd, mOrigMsg->encodedContent() );
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("BODYPIPE=") ) ) {
        // pipe message body generated so far through command and insert result as is
        kDebug() << "Command: BODYPIPE=";
        QString q;
        int len = parseQuotes( "BODYPIPE=", cmd, q );
        i += len;
        QString pipe_cmd = q;
        QString str = pipe( pipe_cmd, body );
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("CLEARPIPE=") ) ) {
        // pipe message body generated so far through command and
        // insert result as is replacing current body
        kDebug() << "Command: CLEARPIPE=";
        QString q;
        int len = parseQuotes( "CLEARPIPE=", cmd, q );
        i += len;
        QString pipe_cmd = q;
        QString str = pipe( pipe_cmd, body );
        body = str;
        KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-KMail-CursorPos", mMsg.get(), QString::number( 0 ), "utf-8" );
        mMsg->setHeader( header );

      } else if ( cmd.startsWith( QLatin1String("TEXT") ) ) {
        kDebug() << "Command: TEXT";
        i += strlen( "TEXT" );
        if ( mOrigMsg ) {
          QString quote = messageText( false );
          body.append( quote );
        }

      } else if ( cmd.startsWith( QLatin1String("OTEXTSIZE") ) ) {
        kDebug() << "Command: OTEXTSIZE";
        i += strlen( "OTEXTSIZE" );
        if ( mOrigMsg ) {
          QString str = QString( "%1" ).arg( mOrigMsg->body().length() );
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OTEXT") ) ) {
        kDebug() << "Command: OTEXT";
        i += strlen( "OTEXT" );
        if ( mOrigMsg ) {
          QString quote = messageText( false );
          body.append( quote );
        }

      } else if ( cmd.startsWith( QLatin1String("OADDRESSEESADDR") ) ) {
        kDebug() << "Command: OADDRESSEESADDR";
        i += strlen( "OADDRESSEESADDR" );
        const QString to = mOrigMsg->to()->asUnicodeString();
        const QString cc = mOrigMsg->cc()->asUnicodeString();
        if ( !to.isEmpty() )
          body.append( i18n( "To:" ) + QLatin1Char( ' ' ) + to );
        if ( !to.isEmpty() && !cc.isEmpty() )
          body.append( QLatin1Char( '\n' ) );
        if ( !cc.isEmpty() )
          body.append( i18n( "CC:" ) + QLatin1Char( ' ' ) +  cc );

      } else if ( cmd.startsWith( QLatin1String("CCADDR") ) ) {
        kDebug() << "Command: CCADDR";
        i += strlen( "CCADDR" );
        QString str = mMsg->cc()->asUnicodeString();
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("CCNAME") ) ) {
        kDebug() << "Command: CCNAME";
        i += strlen( "CCNAME" );
        QString str = MessageHelper::ccStrip( mMsg );
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("CCFNAME") ) ) {
        kDebug() << "Command: CCFNAME";
        i += strlen( "CCFNAME" );
        QString str = MessageHelper::ccStrip( mMsg );
        body.append( getFName( str ) );

      } else if ( cmd.startsWith( QLatin1String("CCLNAME") ) ) {
        kDebug() << "Command: CCLNAME";
        i += strlen( "CCLNAME" );
        QString str = MessageHelper::ccStrip( mMsg );
        body.append( getLName( str ) );

      } else if ( cmd.startsWith( QLatin1String("TOADDR") ) ) {
        kDebug() << "Command: TOADDR";
        i += strlen( "TOADDR" );
        QString str = mMsg->to()->asUnicodeString();
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("TONAME") ) ) {
        kDebug() << "Command: TONAME";
        i += strlen( "TONAME" );
        QString str = MessageHelper::toStrip( mMsg );
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("TOFNAME") ) ) {
        kDebug() << "Command: TOFNAME";
        i += strlen( "TOFNAME" );
        QString str = MessageHelper::toStrip( mMsg );
        body.append( getFName( str ) );

      } else if ( cmd.startsWith( QLatin1String("TOLNAME") ) ) {
        kDebug() << "Command: TOLNAME";
        i += strlen( "TOLNAME" );
        QString str = MessageHelper::toStrip( mMsg );
        body.append( getLName( str ) );

      } else if ( cmd.startsWith( QLatin1String("TOLIST") ) ) {
        kDebug() << "Command: TOLIST";
        i += strlen( "TOLIST" );
        QString str = mMsg->to()->asUnicodeString();
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("FROMADDR") ) ) {
        kDebug() << "Command: FROMADDR";
        i += strlen( "FROMADDR" );
        QString str = mMsg->from()->asUnicodeString();
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("FROMNAME") ) ) {
        kDebug() << "Command: FROMNAME";
        i += strlen( "FROMNAME" );
        QString str = MessageHelper::fromStrip( mMsg );
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("FROMFNAME") ) ) {
        kDebug() << "Command: FROMFNAME";
        i += strlen( "FROMFNAME" );
        QString str = MessageHelper::fromStrip( mMsg );
        body.append( getFName( str ) );

      } else if ( cmd.startsWith( QLatin1String("FROMLNAME") ) ) {
        kDebug() << "Command: FROMLNAME";
        i += strlen( "FROMLNAME" );
        QString str = MessageHelper::fromStrip( mMsg );
        body.append( getLName( str ) );

      } else if ( cmd.startsWith( QLatin1String("FULLSUBJECT") ) ) {
        kDebug() << "Command: FULLSUBJECT";
        i += strlen( "FULLSUBJECT" );
        QString str = mMsg->subject()->asUnicodeString();
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("FULLSUBJ") ) ) {
        kDebug() << "Command: FULLSUBJ";
        i += strlen( "FULLSUBJ" );
        QString str = mMsg->subject()->asUnicodeString();
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("MSGID") ) ) {
        kDebug() << "Command: MSGID";
        i += strlen( "MSGID" );
        QString str = mMsg->messageID()->asUnicodeString();
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("OHEADER=") ) ) {
        // insert specified content of header from original message
        kDebug() << "Command: OHEADER=";
        QString q;
        int len = parseQuotes( "OHEADER=", cmd, q );
        i += len;
        if ( mOrigMsg ) {
          QString hdr = q;
          QString str = mOrigMsg->headerByType(hdr.toLocal8Bit() ) ? mOrigMsg->headerByType(hdr.toLocal8Bit() )->asUnicodeString() : "";
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("HEADER=") ) ) {
        // insert specified content of header from current message
        kDebug() << "Command: HEADER=";
        QString q;
        int len = parseQuotes( "HEADER=", cmd, q );
        i += len;
        QString hdr = q;
        QString str = mMsg->headerByType(hdr.toLocal8Bit() ) ? mMsg->headerByType(hdr.toLocal8Bit() )->asUnicodeString() : "";
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("HEADER( ") ) ) {
        // insert specified content of header from current message
        kDebug() << "Command: HEADER(";
        QRegExp re = QRegExp( "^HEADER\\((.+)\\)" );
        re.setMinimal( true );
        int res = re.indexIn( cmd );
        if ( res != 0 ) {
          // something wrong
          i += strlen( "HEADER( " );
        } else {
          i += re.matchedLength();
          QString hdr = re.cap( 1 );
          QString str = mMsg->headerByType( hdr.toLocal8Bit() ) ? mMsg->headerByType( hdr.toLocal8Bit() )->asUnicodeString() : "";
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OCCADDR") ) ) {
        kDebug() << "Command: OCCADDR";
        i += strlen( "OCCADDR" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->cc()->asUnicodeString();
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OCCNAME") ) ) {
        kDebug() << "Command: OCCNAME";
        i += strlen( "OCCNAME" );
        if ( mOrigMsg ) {
          QString str = MessageHelper::ccStrip( mOrigMsg );
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OCCFNAME") ) ) {
        kDebug() << "Command: OCCFNAME";
        i += strlen( "OCCFNAME" );
        if ( mOrigMsg ) {
          QString str = MessageHelper::ccStrip( mOrigMsg );
          body.append( getFName( str ) );
        }

      } else if ( cmd.startsWith( QLatin1String("OCCLNAME") ) ) {
        kDebug() << "Command: OCCLNAME";
        i += strlen( "OCCLNAME" );
        if ( mOrigMsg ) {
          QString str = MessageHelper::ccStrip( mOrigMsg );
          body.append( getLName( str ) );
        }

      } else if ( cmd.startsWith( QLatin1String("OTOADDR") ) ) {
        kDebug() << "Command: OTOADDR";
        i += strlen( "OTOADDR" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->to()->asUnicodeString();
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OTONAME") ) ) {
        kDebug() << "Command: OTONAME";
        i += strlen( "OTONAME" );
        if ( mOrigMsg ) {
          QString str = MessageHelper::toStrip( mOrigMsg );
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OTOFNAME") ) ) {
        kDebug() << "Command: OTOFNAME";
        i += strlen( "OTOFNAME" );
        if ( mOrigMsg ) {
          QString str = MessageHelper::toStrip( mOrigMsg );
          body.append( getFName( str ) );
        }

      } else if ( cmd.startsWith( QLatin1String("OTOLNAME") ) ) {
        kDebug() << "Command: OTOLNAME";
        i += strlen( "OTOLNAME" );
        if ( mOrigMsg ) {
          QString str = MessageHelper::toStrip( mOrigMsg );
          body.append( getLName( str ) );
        }

      } else if ( cmd.startsWith( QLatin1String("OTOLIST") ) ) {
        kDebug() << "Command: OTOLIST";
        i += strlen( "OTOLIST" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->to()->asUnicodeString();
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OTO") ) ) {
        kDebug() << "Command: OTO";
        i += strlen( "OTO" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->to()->asUnicodeString();
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OFROMADDR") ) ) {
        kDebug() << "Command: OFROMADDR";
        i += strlen( "OFROMADDR" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->from()->asUnicodeString();
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OFROMNAME") ) ) {
        kDebug() << "Command: OFROMNAME";
        i += strlen( "OFROMNAME" );
        if ( mOrigMsg ) {
          QString str = MessageHelper::fromStrip( mOrigMsg );
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OFROMFNAME") ) ) {
        kDebug() << "Command: OFROMFNAME";
        i += strlen( "OFROMFNAME" );
        if ( mOrigMsg ) {
          QString str = MessageHelper::fromStrip( mOrigMsg );
          body.append( getFName( str ) );
        }

      } else if ( cmd.startsWith( QLatin1String("OFROMLNAME") ) ) {
        kDebug() << "Command: OFROMLNAME";
        i += strlen( "OFROMLNAME" );
        if ( mOrigMsg ) {
          QString str = MessageHelper::fromStrip( mOrigMsg );
          body.append( getLName( str ) );
        }

      } else if ( cmd.startsWith( QLatin1String("OFULLSUBJECT") ) ) {
        kDebug() << "Command: OFULLSUBJECT";
        i += strlen( "OFULLSUBJECT" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->subject()->asUnicodeString();
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OFULLSUBJ") ) ) {
        kDebug() << "Command: OFULLSUBJ";
        i += strlen( "OFULLSUBJ" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->subject()->asUnicodeString();
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OMSGID") ) ) {
        kDebug() << "Command: OMSGID";
        i += strlen( "OMSGID" );
        if ( mOrigMsg ) {
          QString str = mOrigMsg->messageID()->asUnicodeString();
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("DATEEN") ) ) {
        kDebug() << "Command: DATEEN";
        i += strlen( "DATEEN" );
        QDateTime date = QDateTime::currentDateTime();
        KLocale locale( "C" );
        QString str = locale.formatDate( date.date(), KLocale::LongDate );
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("DATESHORT") ) ) {
        kDebug() << "Command: DATESHORT";
        i += strlen( "DATESHORT" );
        QDateTime date = QDateTime::currentDateTime();
        QString str = KGlobal::locale()->formatDate( date.date(), KLocale::ShortDate );
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("DATE") ) ) {
        kDebug() << "Command: DATE";
        i += strlen( "DATE" );
        QDateTime date = QDateTime::currentDateTime();
        QString str = KGlobal::locale()->formatDate( date.date(), KLocale::LongDate );
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("DOW") ) ) {
        kDebug() << "Command: DOW";
        i += strlen( "DOW" );
        QDateTime date = QDateTime::currentDateTime();
        QString str = KGlobal::locale()->calendar()->weekDayName( date.date(),
                      KCalendarSystem::LongDayName );
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("TIMELONGEN") ) ) {
        kDebug() << "Command: TIMELONGEN";
        i += strlen( "TIMELONGEN" );
        QDateTime date = QDateTime::currentDateTime();
        KLocale locale( "C");
        QString str = locale.formatTime( date.time(), true );
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("TIMELONG") ) ) {
        kDebug() << "Command: TIMELONG";
        i += strlen( "TIMELONG" );
        QDateTime date = QDateTime::currentDateTime();
        QString str = KGlobal::locale()->formatTime( date.time(), true );
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("TIME") ) ) {
        kDebug() << "Command: TIME";
        i += strlen( "TIME" );
        QDateTime date = QDateTime::currentDateTime();
        QString str = KGlobal::locale()->formatTime( date.time(), false );
        body.append( str );

      } else if ( cmd.startsWith( QLatin1String("ODATEEN") ) ) {
        kDebug() << "Command: ODATEEN";
        i += strlen( "ODATEEN" );
        if ( mOrigMsg ) {
          QDateTime date = mOrigMsg->date()->dateTime().dateTime();
          KLocale locale( "C");
          QString str = locale.formatDate( date.date(), KLocale::LongDate );
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("ODATESHORT")) ) {
        kDebug() << "Command: ODATESHORT";
        i += strlen( "ODATESHORT");
        if ( mOrigMsg ) {
          QDateTime date = mOrigMsg->date()->dateTime().dateTime();
          QString str = KGlobal::locale()->formatDate( date.date(), KLocale::ShortDate );
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("ODATE")) ) {
        kDebug() << "Command: ODATE";
        i += strlen( "ODATE");
        if ( mOrigMsg ) {
          QDateTime date = mOrigMsg->date()->dateTime().dateTime();
          QString str = KGlobal::locale()->formatDate( date.date(), KLocale::LongDate );
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("ODOW")) ) {
        kDebug() << "Command: ODOW";
        i += strlen( "ODOW");
        if ( mOrigMsg ) {
          QDateTime date = mOrigMsg->date()->dateTime().dateTime();
          QString str = KGlobal::locale()->calendar()->weekDayName( date.date(),
                        KCalendarSystem::LongDayName );
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OTIMELONGEN")) ) {
        kDebug() << "Command: OTIMELONGEN";
        i += strlen( "OTIMELONGEN");
        if ( mOrigMsg ) {
          QDateTime date = mOrigMsg->date()->dateTime().dateTime();
          KLocale locale( "C");
          QString str = locale.formatTime( date.time(), true );
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OTIMELONG")) ) {
        kDebug() << "Command: OTIMELONG";
        i += strlen( "OTIMELONG");
        if ( mOrigMsg ) {
          QDateTime date = mOrigMsg->date()->dateTime().dateTime();
          QString str = KGlobal::locale()->formatTime( date.time(), true );
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("OTIME")) ) {
        kDebug() << "Command: OTIME";
        i += strlen( "OTIME");
        if ( mOrigMsg ) {
          QDateTime date = mOrigMsg->date()->dateTime().dateTime();
          QString str = KGlobal::locale()->formatTime( date.time(), false );
          body.append( str );
        }

      } else if ( cmd.startsWith( QLatin1String("BLANK") ) ) {
        // do nothing
        kDebug() << "Command: BLANK";
        i += strlen( "BLANK" );

      } else if ( cmd.startsWith( QLatin1String("NOP") ) ) {
        // do nothing
        kDebug() << "Command: NOP";
        i += strlen( "NOP" );

      } else if ( cmd.startsWith( QLatin1String("CLEAR") ) ) {
        // clear body buffer; not too useful yet
        kDebug() << "Command: CLEAR";
        i += strlen( "CLEAR" );
        body = "";
        KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-KMail-CursorPos", mMsg.get(), QString::number( 0 ), "utf-8" );
        mMsg->setHeader( header );
      } else if ( cmd.startsWith( QLatin1String("DEBUGOFF") ) ) {
        // turn off debug
        kDebug() << "Command: DEBUGOFF";
        i += strlen( "DEBUGOFF" );
        mDebug = false;

      } else if ( cmd.startsWith( QLatin1String("DEBUG") ) ) {
        // turn on debug
        kDebug() << "Command: DEBUG";
        i += strlen( "DEBUG" );
        mDebug = true;

      } else if ( cmd.startsWith( QLatin1String("CURSOR") ) ) {
        // turn on debug
        kDebug() << "Command: CURSOR";
        i += strlen( "CURSOR" );
        KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-KMail-CursorPos", mMsg.get(), QString::number( body.length()-1 ), "utf-8" );
        mMsg->setHeader( header );
      } else if ( cmd.startsWith( QLatin1String( "SIGNATURE" ) ) ) {
        kDebug() << "Command: SIGNATURE";
        i += strlen( "SIGNATURE" );
        body.append( getSignature() );

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

QString TemplateParser::getSignature() const
{
  const KPIMIdentities::Identity &identity =
    kmkernel->identityManager()->identityForUoid( mIdentity );
  if ( identity.isNull() )
    return QString();

  KPIMIdentities::Signature signature = const_cast<KPIMIdentities::Identity&>
                                                  ( identity ).signature();
  if ( signature.type() == KPIMIdentities::Signature::Inlined &&
       signature.isInlinedHtml() ) {
    // templates don't support HTML; convert to plain text
    return signature.toPlainText();
  }
  else {
    return signature.rawText();
  }
}

QString TemplateParser::messageText( bool allowSelectionOnly )
{
  if ( !mSelection.isEmpty() && allowSelectionOnly )
    return mSelection;
  // FIXME
  // No selection text, therefore we need to parse the object tree ourselves to get
  //KMime::Content *root = parsedObjectTree();

  // ### temporary hack to uncrash reply/forward
  mOrigRoot = new KMime::Content;
  mOrigRoot->setContent( mOrigMsg->encodedContent() );
  mOrigRoot->parse();

  MessageViewer::EmptySource emptySource;
  MessageViewer::ObjectTreeParser otp(&emptySource); // all defaults are ok
  otp.setAllowAsync( false );
  otp.parseObjectTree( Akonadi::Item(), mOrigRoot );

  return asPlainTextFromObjectTree( mOrigMsg, mOrigRoot, &otp, mSmartQuote, mAllowDecryption );
}

KMime::Content* TemplateParser::parsedObjectTree()
{
  if ( mOrigRoot )
    return mOrigRoot;
  mOrigRoot = new KMime::Content;
  mOrigRoot->setContent( mMsg->encodedContent() );

  MessageViewer::EmptySource emptySource;
  MessageViewer::ObjectTreeParser otp(&emptySource); // all defaults are ok
  otp.parseObjectTree( Akonadi::Item(), mOrigRoot );

  return mOrigRoot;
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
    KMime::Content *root = parsedObjectTree();
    KMail::AttachmentCollector ac;
    ac.collectAttachmentsFrom( root );

    // Now, delete the old content and set the new content, which
    // is either only the new text or the new text with some attachments.
    KMime::Content::List parts = mMsg->contents();
    foreach ( KMime::Content *content, parts )
      mMsg->removeContent( content, true /*delete*/ );

    // Set To and CC from the template
    if ( !mTo.isEmpty() ) {
      mMsg->to()->fromUnicodeString( mMsg->to()->asUnicodeString() + ',' + mTo, "utf-8" );
    }
    if ( !mCC.isEmpty() )
      mMsg->cc()->fromUnicodeString( mMsg->cc()->asUnicodeString() + ',' + mCC, "utf-8" );

    // If we have no attachment, simply create a text/plain part and
    // set the processed template text as the body
    if ( ac.attachments().empty() || mMode != Forward ) {
      mMsg->contentType()->clear(); // to get rid of old boundary
      mMsg->contentType()->setMimeType( "text/plain" );
      mMsg->setBody( body.toUtf8() );
      mMsg->assemble();
    }

    // If we have some attachments, create a multipart/mixed mail and
    // add the normal body as well as the attachments
    else
    {
      const QByteArray boundary = KMime::multiPartBoundary();
      mMsg->contentType()->setMimeType( "multipart/mixed" );
      mMsg->contentType()->setBoundary( boundary );

      KMime::Content *textPart = new KMime::Content( mMsg.get() );
      textPart->contentType()->setMimeType( "text/plain" );
      textPart->fromUnicodeString( body );
      mMsg->addContent( textPart );

      int attachmentNumber = 1;
      foreach( KMime::Content *attachment, ac.attachments() ) {
        mMsg->addContent( attachment );
        // If the content type has no name or filename parameter, add one, since otherwise the name
        // would be empty in the attachment view of the composer, which looks confusing
        if ( attachment->contentType( false ) ) {
          if ( !attachment->contentType()->hasParameter( "name" ) &&
               !attachment->contentType()->hasParameter( "filename" ) ) {
            attachment->contentType()->setParameter( "name", i18n( "Attachment %1", attachmentNumber ) );
          }
        }
        attachmentNumber++;
      }
      mMsg->assemble();
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
  // kDebug() << "Trying to find template for mode" << mode;

  QString tmpl;
#if 0
  if ( !mFolder.isValid() ) { // find folder message belongs to
    mFolder = mMsg->parentCollection();
    if ( !mFolder.isValid() ) {
      if ( mOrigMsg ) {
        mFolder = mOrigMsg->parentCollection();
      }
      if ( !mFolder.isValid() ) {
        kDebug() << "Oops! No folder for message";
      }
    }
  }
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  kDebug() << "Folder found:" << mFolder;
  if ( mFolder.isValid() )  // only if a folder was found
  {
    QString fid = QString::number( mFolder.id() );
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
        kDebug() << "Unknown message mode:" << mMode;
        return "";
      }
      mQuoteString = fconf.quoteString();
      if ( !tmpl.isEmpty() ) {
        return tmpl;  // use folder-specific template
      }
    }
  }

  if ( !mIdentity ) { // find identity message belongs to
       kDebug() << "AKONADI PORT: verify Akonadi::Item() here  " << Q_FUNC_INFO;

    mIdentity = KMail::MessageHelper::identityUoid( Akonadi::Item(), mMsg );
    if ( !mIdentity && mOrigMsg ) {
      kDebug() << "AKONADI PORT: verify Akonadi::Item() here  " << Q_FUNC_INFO;
      mIdentity = KMail::MessageHelper::identityUoid( Akonadi::Item(), mOrigMsg );
    }
    mIdentity = kmkernel->identityManager()->identityForUoidOrDefault( mIdentity ).uoid();
    if ( !mIdentity ) {
      kDebug() << "Oops! No identity for message";
    }
  }
  kDebug() << "Identity found:" << mIdentity;

  QString iid;
  if ( mIdentity ) {
    iid = TemplatesConfiguration::configIdString( mIdentity );	// templates ID for that identity
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
      kDebug() << "Unknown message mode:" << mMode;
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
    kDebug() << "Unknown message mode:" << mMode;
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

void TemplateParser::parseTextStringFromContent( KMime::Content * root,
                                           QByteArray& parsedString,
                                           const QTextCodec*& codec,
                                           bool& isHTML ) const
{
  if ( !root )
    return;

  isHTML = false;
  KMime::Content * curNode = root->textContent();
  kDebug() << ( curNode ? "text part found!\n" : "sorry, no text node!\n" );
  if( curNode ) {
    isHTML = curNode->contentType()->isHTMLText();
    // now parse the TEXT message part we want to quote
    MessageViewer::EmptySource emptySource;
    MessageViewer::ObjectTreeParser otp( &emptySource, 0, 0, true, false, true );
    otp.parseObjectTree( Akonadi::Item(), curNode );
    parsedString = otp.rawReplyString();
    codec = otp.nodeHelper()->codec( curNode );
  }
}

QString TemplateParser::asPlainTextFromObjectTree( const KMime::Message::Ptr &msg, KMime::Content *root, MessageViewer::ObjectTreeParser *otp, bool aStripSignature,
                                              bool allowDecryption )
{
  Q_ASSERT( root );
  Q_ASSERT( otp->nodeHelper()->nodeProcessed( root ) );

  QByteArray parsedString;
  bool isHTML = false;
  const QTextCodec * codec = 0;

  if ( !root ) return QString();
  parseTextStringFromContent( root, parsedString, codec, isHTML );

#if 0 //TODO port to akonadi
  if ( mOverrideCodec || !codec )
    codec = otp->nodeHelper()->codec( msg );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  if ( parsedString.isEmpty() )
    return QString();

  bool clearSigned = false;
  QString result;

  // decrypt
  if ( allowDecryption ) {
    QList<Kpgp::Block> pgpBlocks;
    QList<QByteArray>  nonPgpBlocks;
    if ( Kpgp::Module::prepareMessageForDecryption( parsedString,
                                                    pgpBlocks,
                                                    nonPgpBlocks ) ) {
      // Only decrypt/strip off the signature if there is only one OpenPGP
      // block in the message
      if ( pgpBlocks.count() == 1 ) {
        Kpgp::Block &block = pgpBlocks.first();
        if ( block.type() == Kpgp::PgpMessageBlock ||
             block.type() == Kpgp::ClearsignedBlock ) {
          if ( block.type() == Kpgp::PgpMessageBlock ) {
            // try to decrypt this OpenPGP block
            block.decrypt();
          } else {
            // strip off the signature
            block.verify();
            clearSigned = true;
          }

          result = codec->toUnicode( nonPgpBlocks.first() )
              + codec->toUnicode( block.text() )
              + codec->toUnicode( nonPgpBlocks.last() );
        }
      }
    }
  }

  if ( result.isEmpty() ) {
    result = codec->toUnicode( parsedString );
    if ( result.isEmpty() )
      return result;
  }

  // html -> plaintext conversion, if necessary:
  if ( isHTML /* TODO port it && mDecodeHTML*/ ) {
    QWebElement doc = QWebElement();
    doc.prependInside( result );
    result = doc.toPlainText();
  }

  // strip the signature (footer):
  if ( aStripSignature )
    return MessageCore::StringUtil::stripSignature( result, clearSigned );
  else
    return result;
}


QString TemplateParser::asPlainText( const KMime::Message::Ptr &msg, bool aStripSignature, bool allowDecryption )
{
  if ( !msg )
    return QString();

  KMime::Content *root = new KMime::Content;
  root->setContent( msg->encodedContent() );
  root->parse();
  MessageViewer::EmptySource emptySource;
  MessageViewer::ObjectTreeParser otp(&emptySource);
  otp.parseObjectTree( Akonadi::Item(), root );
  QString result = asPlainTextFromObjectTree( msg, root, &otp, aStripSignature, allowDecryption );
  delete root;
  return result;
}



QString TemplateParser::asQuotedString( const KMime::Message::Ptr &msg, const QString& aIndentStr,
                                        const QString& selection /*.clear() */,
                                        bool aStripSignature /* = true */,
                                        bool allowDecryption /* = true */)
{
  if ( !msg )
    return QString();

  QString content = selection.isEmpty() ?
    asPlainText( msg, aStripSignature, allowDecryption ) : selection ;

  // Remove blank lines at the beginning:
  const int firstNonWS = content.indexOf( QRegExp( "\\S" ) );
  const int lineStart = content.lastIndexOf( '\n', firstNonWS );
  if ( lineStart >= 0 )
    content.remove( 0, static_cast<unsigned int>( lineStart ) );

  const QString indentStr = MessageCore::StringUtil::formatString( aIndentStr,
                                                                   msg->from()->asUnicodeString() );
  if ( kmkernel->smartQuote() && kmkernel->wordWrap() )
    content = MessageCore::StringUtil::smartQuote( content,
                                                   kmkernel->wrapCol() - indentStr.length() );
  content.replace( '\n', '\n' + indentStr );
  content.prepend( indentStr );
  content += '\n';

  return content;
}
} // namespace KMail

#include "templateparser.moc"
