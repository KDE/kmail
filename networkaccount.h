/** -*- c++ -*-
 * networkaccount.h
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


#ifndef __KMAIL_NETWORKACCOUNT_H__
#define __KMAIL_NETWORKACCOUNT_H__

#include "kmaccount.h"

#include "sieveconfig.h"

#include <qstring.h>

class KMAcctMgr;
class KConfig/*Base*/;
class KURL;
namespace KIO {
  class Slave;
  class MetaData;
}

namespace KMail {

  class NetworkAccount : public KMAccount {
    Q_OBJECT
  protected:
    NetworkAccount( KMAcctMgr * parent, const QString & name, uint id );
  public:
    virtual ~NetworkAccount();

    /** Set the config options to a decent state */
    virtual void init();

    /** A weak assignment operator */
    virtual void pseudoAssign( const KMAccount * a );

    /** User login name */
    QString login() const { return mLogin; }
    virtual void setLogin( const QString & login );

    /** User password */
    QString passwd() const;
    virtual void setPasswd( const QString & passwd, bool storeInConfig=false );

    /**
     * Set the password to "" (empty string)
     */
    virtual void clearPasswd();

    /** authentication method */
    QString auth() const { return mAuth; }
    virtual void setAuth( const QString & auth );

    /** @return whether to store the password in the config file */
    bool storePasswd() const { return mStorePasswd; }
    virtual void setStorePasswd( bool store );

    /** Server hostname */
    QString host() const { return mHost; }
    virtual void setHost( const QString & host );

    /** Server port number */
    unsigned short int port() const { return mPort; }
    virtual void setPort( unsigned short int port );

    /** @return whether to use SSL */
    bool useSSL() const { return mUseSSL; }
    virtual void setUseSSL( bool use );

    /** @return whether to use TLS */
    bool useTLS() const { return mUseTLS; }
    virtual void setUseTLS( bool use );

    /** @return the @see SieveConfig for this account */
    KMail::SieveConfig sieveConfig() const { return mSieveConfig; }
    virtual void setSieveConfig( const KMail::SieveConfig & config );

    /** Configure the slave by adding to the meta data map */
    virtual KIO::MetaData slaveConfig() const;

    virtual void readConfig( /*const*/ KConfig/*Base*/ & config );
    virtual void writeConfig( KConfig/*Base*/ & config ) /*const*/;

    /** @return an URL for this account */
    virtual KURL getUrl() const;

    /** @return the KIO slave for this account */
    KIO::Slave * slave() const { return mSlave; }

    /** Kill all jobs that are currently in progress */
    virtual void killAllJobs( bool disconnectSlave = false ) = 0;

    /** Read password from wallet, used for on-demand wallet opening */
    void readPassword();

  protected:
    virtual QString protocol() const = 0;
    virtual unsigned short int defaultPort() const = 0;

  protected:
    KMail::SieveConfig mSieveConfig;
    KIO::Slave * mSlave;
    QString mLogin, mPasswd, mAuth, mHost;
    unsigned short int mPort;
    bool mStorePasswd : 1;
    bool mUseSSL : 1;
    bool mUseTLS : 1;
    bool mAskAgain : 1;
    bool mPasswdDirty, mStorePasswdInConfig;

  };

} // namespace KMail

#endif // __KMAIL_NETWORKACCOUNT_H__
