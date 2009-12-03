/*******************************************************************************
**
** Filename   : util
** Created on : 03 April, 2005
** Copyright  : (c) 2005 Till Adam
** Email      : <adam@kde.org>
**
*******************************************************************************/

/*******************************************************************************
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
**   It is distributed in the hope that it will be useful, but
**   WITHOUT ANY WARRANTY; without even the implied warranty of
**   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
**   General Public License for more details.
**
**   You should have received a copy of the GNU General Public License
**   along with this program; if not, write to the Free Software
**   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
**   In addition, as a special exception, the copyright holders give
**   permission to link the code of this program with any edition of
**   the Qt library by Trolltech AS, Norway (or with modified versions
**   of Qt that use the same license as Qt), and distribute linked
**   combinations including the two.  You must obey the GNU General
**   Public License in all respects for all of the code used other than
**   Qt.  If you modify this file, you may extend this exception to
**   your version of the file, but you are not obligated to do so.  If
**   you do not wish to do so, delete this exception statement from
**   your version.
**
*******************************************************************************/


#include "util.h"
#include <QTextCodec>
#include "messageviewer/stringutil.h"

#include <stdlib.h>
#include <kpimutils/email.h>
#include <kglobal.h>
#include <kascii.h>
#include <KCharsets>
#include "imapsettings.h"
#include <kimap/loginjob.h>

void KMail::Util::reconnectSignalSlotPair( QObject *src, const char *signal, QObject *dst, const char *slot )
{
  QObject::disconnect( src, signal, dst, slot );
  QObject::connect( src, signal, dst, slot );
}


size_t KMail::Util::crlf2lf( char* str, const size_t strLen )
{
    if ( !str || strLen == 0 )
        return 0;

    const char* source = str;
    const char* sourceEnd = source + strLen;

    // search the first occurrence of "\r\n"
    for ( ; source < sourceEnd - 1; ++source ) {
        if ( *source == '\r' && *( source + 1 ) == '\n' )
            break;
    }

    if ( source == sourceEnd - 1 ) {
        // no "\r\n" found
        return strLen;
    }

    // replace all occurrences of "\r\n" with "\n" (in place)
    char* target = const_cast<char*>( source ); // target points to '\r'
    ++source; // source points to '\n'
    for ( ; source < sourceEnd; ++source ) {
        if ( *source != '\r' || *( source + 1 ) != '\n' )
            * target++ = *source;
    }
    *target = '\0'; // terminate result
    return target - str;
}

QByteArray KMail::Util::lf2crlf( const QByteArray & src )
{
    QByteArray result;
    result.resize( 2*src.size() );  // maximal possible length

    QByteArray::ConstIterator s = src.begin();
    QByteArray::Iterator d = result.begin();
  // we use cPrev to make sure we insert '\r' only there where it is missing
    char cPrev = '?';
    const char* end = src.end();
    while ( s != end ) {
        if ( ('\n' == *s) && ('\r' != cPrev) )
            *d++ = '\r';
        cPrev = *s;
        *d++ = *s++;
    }
    result.truncate( d - result.begin() );
    return result;
}

bool KMail::Util::checkOverwrite( const KUrl &url, QWidget *w )
{
  if ( KIO::NetAccess::exists( url, KIO::NetAccess::DestinationSide, w ) ) {
    if ( KMessageBox::Cancel == KMessageBox::warningContinueCancel(
         w,
         i18n( "A file named \"%1\" already exists. "
             "Are you sure you want to overwrite it?", url.prettyUrl() ),
             i18n( "Overwrite File?" ),
                   KStandardGuiItem::overwrite() ) )
      return false;
  }
  return true;
}

#ifndef KMAIL_UNITTESTS
bool KMail::Util::validateAddresses( QWidget *parent, const QString &addresses )
{
  QString brokenAddress;

  QStringList distributionListEmpty;
  KPIMUtils::EmailParseResult errorCode =
    KPIMUtils::isValidAddressList( MessageViewer::StringUtil::expandAliases( addresses,distributionListEmpty ),
                                   brokenAddress );
  if ( !distributionListEmpty.isEmpty() ) {
    QString errorMsg = i18n( "Distribution list \"%1\" is empty, it cannot be used.", distributionListEmpty.join( ", " ) );
    KMessageBox::sorry( parent , errorMsg, i18n("Invalid Email Address") );
    return false;
  }
  if ( !( errorCode == KPIMUtils::AddressOk ||
          errorCode == KPIMUtils::AddressEmpty ) ) {
    QString errorMsg( "<qt><p><b>" + brokenAddress +
                      "</b></p><p>" +
                      KPIMUtils::emailParseResultToString( errorCode ) +
                      "</p></qt>" );
    KMessageBox::sorry( parent , errorMsg, i18n("Invalid Email Address") );
    return false;
  }
  return true;
}
#endif

#ifdef Q_WS_MACX
#include <QDesktopServices>
#endif

bool KMail::Util::handleUrlOnMac( const KUrl& url )
{
#ifdef Q_WS_MACX
  QDesktopServices::openUrl( url );
  return true;
#else
  Q_UNUSED( url );
  return false;
#endif
}

KMime::Message::Ptr KMail::Util::message( const Akonadi::Item & item )
{
  if ( !item.hasPayload<KMime::Message::Ptr>() ) {
    kWarning() << "Payload is not a MessagePtr!";
    return KMime::Message::Ptr();
  }
  return item.payload<boost::shared_ptr<KMime::Message> >();
}

QByteArray toUsAscii(const QString& _str, bool *ok)
{
  bool all_ok =true;
  QString result = _str;
  int len = result.length();
  for (int i = 0; i < len; i++)
    if (result.at(i).unicode() >= 128) {
      result[i] = '?';
      all_ok = false;
    }
  if (ok)
    *ok = all_ok;
  return result.toLatin1();
}

const QTextCodec* codecForName(const QByteArray& _str)
{
  // cberzan: kill this
  if (_str.isEmpty())
    return 0;
  QByteArray codec = _str;
  kAsciiToLower(codec.data()); // TODO cberzan: I don't think this is needed anymore
                               // (see kdelibs/kdecore/localization/kcharsets.cpp:
                               // it already toLowers stuff).
  return KGlobal::charsets()->codecForName(codec);
}

//-----------------------------------------------------------------------------
QByteArray autoDetectCharset(const QByteArray &_encoding, const QStringList &encodingList, const QString &text)
{
    QStringList charsets = encodingList;
    if (!_encoding.isEmpty())
    {
       QString currentCharset = QString::fromLatin1(_encoding);
       charsets.removeAll(currentCharset);
       charsets.prepend(currentCharset);
    }

    // cberzan: moved this crap to CodecManager ==================================
    QStringList::ConstIterator it = charsets.constBegin();
    for (; it != charsets.constEnd(); ++it)
    {
       QByteArray encoding = (*it).toLatin1();
       if (encoding == "locale")
       {
         encoding = KMKernel::self()->networkCodec()->name();
         kAsciiToLower(encoding.data());
       }
       if (text.isEmpty())
         return encoding;
       if (encoding == "us-ascii") {
         bool ok;
         (void) toUsAscii(text, &ok);
         if (ok)
            return encoding;
       }
       else
       {
         const QTextCodec *codec = codecForName(encoding);
         if (!codec) {
           kDebug() << "Auto-Charset: Something is wrong and I can not get a codec. [" << encoding <<"]";
         } else {
           if (codec->canEncode(text))
              return encoding;
         }
       }
    }
    // cberzan: till here =====================================================
    return 0;
}


KUrl KMail::Util::findSieveUrlForAccount( OrgKdeAkonadiImapSettingsInterface *a) {
  assert( a );
  if ( !a->sieveSupport() )
    return KUrl();
  if ( a->sieveReuseConfig() ) {
    // assemble Sieve url from the settings of the account:
    KUrl u;
    u.setProtocol( "sieve" );
    QString server;
    QDBusReply<QString> reply = a->imapServer();
    if ( reply.isValid() ) {
      server = reply;
      server = server.section( ':', 0, 0 );
    } else {
      return KUrl();
    }
    u.setHost( server );
    u.setUser( a->userName() );
#if 0
    u.setPass( a->password() );
#endif
    u.setPort( a->sievePort() );
    QString authStr;
    switch( a->authentication() ) {
    case KIMAP::LoginJob::ClearText:
      authStr = "PLAIN";
      break;
    case KIMAP::LoginJob::Login:
      authStr = "LOGIN";
      break;
    case KIMAP::LoginJob::Plain:
      authStr = "PLAIN";
      break;
    case KIMAP::LoginJob::CramMD5:
      authStr = "CRAM-MD5";
      break;
    case KIMAP::LoginJob::DigestMD5:
      authStr = "DIGEST-MD5";
      break;
    case KIMAP::LoginJob::GSSAPI:
      authStr = "GSSAPI";
      break;
    case KIMAP::LoginJob::Anonymous:
      authStr = "ANONYMOUS";
      break;
    default:
      authStr = "PLAIN";
      break;
    }
    u.addQueryItem( "x-mech", authStr );
    if ( a->safety() == ( int )( KIMAP::LoginJob::Unencrypted ))
      u.addQueryItem( "x-allow-unencrypted", "true" );
    u.setFileName( a->sieveVacationFilename() );
    return u;
  } else {
    KUrl u( a->sieveAlternateUrl() );
    if ( u.protocol().toLower() == "sieve" && (  a->safety() == ( int )( KIMAP::LoginJob::Unencrypted ) ) && u.queryItem("x-allow-unencrypted").isEmpty() )
      u.addQueryItem( "x-allow-unencrypted", "true" );
    u.setFileName( a->sieveVacationFilename() );
    return u;
  }
}

OrgKdeAkonadiImapSettingsInterface *KMail::Util::createImapSettingsInterface( const QString &ident )
{
  OrgKdeAkonadiImapSettingsInterface *iface = new OrgKdeAkonadiImapSettingsInterface("org.freedesktop.Akonadi.Resource." + ident, "/Settings", QDBusConnection::sessionBus() );
  return iface;
}
