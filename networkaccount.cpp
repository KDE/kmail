/** -*- c++ -*-
 * networkaccount.cpp
 *
 * Copyright (c) 2000-2002 Michael Haeckel <haeckel@kde.org>
 * Copyright (c) 2002 Marc Mutz <mutz@kde.org>
 *
 * This file is based on work on pop3 and imap account implementations
 * by Don Sanders <sanders@kde.org> and Michael Haeckel <haeckel@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */



#include "networkaccount.h"

#include <kconfig.h>
#include <kio/global.h>
using KIO::MetaData;

#include <climits>

namespace KMail {

  NetworkAccount::NetworkAccount( KMAcctMgr * parent, const QString & name )
    : base( parent, name ),
      mSlave( 0 ),
      mAuth( "*" ),
      mPort( 0 ),
      mStorePasswd( false ),
      mUseSSL( false ),
      mUseTLS( false ),
      mAskAgain( false )
  {

  }

  NetworkAccount::~NetworkAccount() {

  }

  void NetworkAccount::init() {
    base::init();

    mSieveConfig = SieveConfig();
    mLogin = QString::null;
    mPasswd = QString::null;
    mAuth = "*";
    mHost = QString::null;
    mPort = defaultPort();
    mStorePasswd = false;
    mUseSSL = false;
    mUseTLS = false;
    mAskAgain = false;
  }

  //
  //
  // Getters and Setters
  //
  //

  void NetworkAccount::setLogin( const QString & login ) {
    mLogin = login;
  }

  QString NetworkAccount::passwd() const {
    return decryptStr( mPasswd );
  }

  void NetworkAccount::setPasswd( const QString & passwd, bool storeInConfig ) {
    mPasswd = encryptStr( passwd );
    setStorePasswd( storeInConfig );
  }

  void NetworkAccount::clearPasswd() {
    setPasswd( "", false );
  }

  void NetworkAccount::setAuth( const QString & auth ) {
    mAuth = auth;
  }

  void NetworkAccount::setStorePasswd( bool store ) {
    mStorePasswd = store;
  }

  void NetworkAccount::setHost( const QString & host ) {
    mHost = host;
  }

  void NetworkAccount::setPort( unsigned short int port ) {
    mPort = port;
  }

  void NetworkAccount::setUseSSL( bool use ) {
    mUseSSL = use;
  }

  void NetworkAccount::setUseTLS( bool use ) {
    mUseTLS = use;
  }

  void NetworkAccount::setSieveConfig( const SieveConfig & config ) {
    mSieveConfig = config;
  }

  //
  //
  // read/write config
  //
  //

  void NetworkAccount::readConfig( /*const*/ KConfig/*Base*/ & config ) {
    base::readConfig( config );

    setLogin( config.readEntry( "login" ) );

    if ( config.readNumEntry( "store-passwd", false ) ) { // ### s/Num/Bool/
      QString encpasswd = config.readEntry( "pass" );
      if ( encpasswd.isEmpty() ) {
	encpasswd = config.readEntry( "passwd" );
	if ( !encpasswd.isEmpty() ) encpasswd = importPassword( encpasswd );
      }
      setPasswd( decryptStr( encpasswd ), true );
    } else
      setPasswd( "", false );
    
    setHost( config.readEntry( "host" ) );

    unsigned int port = config.readUnsignedNumEntry( "port", defaultPort() );
    if ( port > USHRT_MAX ) port = defaultPort();
    setPort( port );
    
    setAuth( config.readEntry( "auth", "*" ) );
    setUseSSL( config.readBoolEntry( "use-ssl", false ) );
    setUseTLS( config.readBoolEntry( "use-tls", false ) );
    
    mSieveConfig.readConfig( config );
  }

  void NetworkAccount::writeConfig( KConfig/*Base*/ & config ) /*const*/ {
    base::writeConfig( config );

    config.writeEntry( "login", login() );
    config.writeEntry( "store-passwd", storePasswd() );
    if ( storePasswd() ) config.writeEntry( "pass", mPasswd ); // NOT passwd()
    else config.writeEntry( "passwd", "" ); // ### ???? why two different keys?

    config.writeEntry( "host", host() );
    config.writeEntry( "port", static_cast<unsigned int>( port() ) );
    config.writeEntry( "auth", auth() );
    config.writeEntry( "use-ssl", useSSL() );
    config.writeEntry( "use-tls", useTLS() );

    mSieveConfig.writeConfig( config );
  }

  //
  //
  // Network processing
  //
  //

  KURL NetworkAccount::getUrl() const {
    KURL url;
    url.setProtocol( protocol() );
    url.setUser( login() );
    url.setPass( passwd() );
    url.setHost( host() );
    url.setPort( port() );
    return url;
  }

  MetaData NetworkAccount::slaveConfig() const {
    MetaData m;
    m.insert( "tls", useTLS() ? "on" : "off" );
    return m;
  }

  void NetworkAccount::pseudoAssign( const KMAccount * a ) {
    base::pseudoAssign( a );

    const NetworkAccount * n = dynamic_cast<const NetworkAccount*>( a );
    if ( !n ) return;

    setLogin( n->login() );
    setPasswd( n->passwd(), n->storePasswd() );
    setHost( n->host() );
    setPort( n->port() );
    setAuth( n->auth() );
    setUseSSL( n->useSSL() );
    setUseTLS( n->useTLS() );
    setSieveConfig( n->sieveConfig() );
  }

} // namespace KMail

#include "networkaccount.moc"
