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



#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "networkaccount.h"
#include "kmacctmgr.h"
#include "kmkernel.h"

#include <kconfig.h>
#include <kio/global.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kwallet.h>
using KIO::MetaData;
using KWallet::Wallet;

#include <climits>

namespace KMail {

  NetworkAccount::NetworkAccount( KMAcctMgr * parent, const QString & name, uint id )
    : KMAccount( parent, name, id ),
      mSlave( 0 ),
      mAuth( "*" ),
      mPort( 0 ),
      mStorePasswd( false ),
      mUseSSL( false ),
      mUseTLS( false ),
      mAskAgain( false ),
      mPasswdDirty( false )
  {

  }

  NetworkAccount::~NetworkAccount() {

  }

  void NetworkAccount::init() {
    KMAccount::init();

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
    if ( storePasswd() && mPasswd.isEmpty() )
      mOwner->readPasswords();
    return decryptStr( mPasswd );
  }

  void NetworkAccount::setPasswd( const QString & passwd, bool storeInConfig ) {
    if ( mPasswd != encryptStr( passwd ) ) {
      mPasswd = encryptStr( passwd );
      mPasswdDirty = true;
    }
    setStorePasswd( storeInConfig );
  }

  void NetworkAccount::clearPasswd() {
    setPasswd( "", false );
  }

  void NetworkAccount::setAuth( const QString & auth ) {
    mAuth = auth;
  }

  void NetworkAccount::setStorePasswd( bool store ) {
    if( mStorePasswd != store && store )
      mPasswdDirty = true;
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
    KMAccount::readConfig( config );

    setLogin( config.readEntry( "login" ) );

    if ( config.readNumEntry( "store-passwd", false ) ) { // ### s/Num/Bool/
      mStorePasswd = true;
      QString encpasswd = config.readEntry( "pass" );
      if ( encpasswd.isEmpty() ) {
        encpasswd = config.readEntry( "passwd" );
        if ( !encpasswd.isEmpty() ) encpasswd = importPassword( encpasswd );
      }

      if ( !encpasswd.isEmpty() ) {
        // migration to KWallet
        setPasswd( decryptStr( encpasswd ), true );
        config.deleteEntry( "pass" );
        config.deleteEntry( "passwd" );
        mPasswdDirty = true;
      } else {
        // read password if wallet is already open, otherwise defer to on-demand loading
        if ( Wallet::isOpen( Wallet::NetworkWallet() ) )
          readPassword();
      }

    } else {
      setPasswd( "", false );
    }

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
    KMAccount::writeConfig( config );

    config.writeEntry( "login", login() );
    config.writeEntry( "store-passwd", storePasswd() );

    // write password to the wallet if necessary
    if ( storePasswd() && mPasswdDirty ) {
      Wallet *wallet = kmkernel->wallet();
      if ( !wallet || wallet->writePassword( "account-" + QString::number(mId), passwd() ) ) {
        KMessageBox::information( 0, i18n("KWallet is not running. It is strongly recommend to use "
            "KWallet for managing your password"),
            i18n("KWallet is Not Running."), "KWalletWarning" );
        config.writeEntry( "pass", encryptStr( passwd() ) );
      }
    }

    // delete password from the wallet if password storage is disabled
    if (!storePasswd() && !Wallet::keyDoesNotExist(
        Wallet::NetworkWallet(), "kmail", "account-" + QString::number(mId))) {
      Wallet *wallet = kmkernel->wallet();
      if (wallet)
        wallet->removeEntry( "account-" + QString::number(mId) );
    }

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
    KMAccount::pseudoAssign( a );

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

  void NetworkAccount::readPassword() {
    if ( !storePasswd() )
      return;

    // ### workaround for broken Wallet::keyDoesNotExist() which returns wrong
    // results for new entries without closing and reopening the wallet
    if ( Wallet::isOpen( Wallet::NetworkWallet() ) )
    {
       Wallet *wallet = kmkernel->wallet();
       if (!wallet || !wallet->hasEntry( "account-" + QString::number(mId) ) )
         return;
    }
    else
    {
       if (Wallet::keyDoesNotExist( Wallet::NetworkWallet(), "kmail", "account-" + QString::number(mId) ) )
         return;
    }

    if ( kmkernel->wallet() ) {
      QString passwd;
      kmkernel->wallet()->readPassword( "account-" + QString::number(mId), passwd );
      setPasswd( passwd, true );
      mPasswdDirty = false;
    }
  }

} // namespace KMail

#include "networkaccount.moc"
