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

KMime::Message::Ptr KMail::Util::message( const Akonadi::Item & item )
{
  if ( !item.hasPayload<KMime::Message::Ptr>() ) {
    kWarning() << "Payload is not a MessagePtr!";
    return KMime::Message::Ptr();
  }
  return item.payload<boost::shared_ptr<KMime::Message> >();
}

KUrl KMail::Util::findSieveUrlForAccount( OrgKdeAkonadiImapSettingsInterface *a, const QString& ident) {
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

    QDBusInterface resourceSettings( QString("org.freedesktop.Akonadi.Resource.")+ident,"/Settings", "org.kde.Akonadi.Imap.Wallet" );

    QString pwd;
    QDBusReply<QString> replyPass = resourceSettings.call( "password" );
    if ( replyPass.isValid() ) {
      pwd = replyPass;
    }
    u.setPass( pwd );
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
  return new OrgKdeAkonadiImapSettingsInterface("org.freedesktop.Akonadi.Resource." + ident, "/Settings", QDBusConnection::sessionBus() );
}


void KMail::Util::launchAccountWizard( QWidget *w )
{
  QStringList lst;
  lst.append( "--type" );
  lst.append( "message/rfc822" );

  if( !QProcess::startDetached("accountwizard", lst ) )
    KMessageBox::error( w, i18n( "Could not start the account wizard. "
                                 "Please check your installation." ),
                        i18n( "Unable to start account wizard" ) );

}

