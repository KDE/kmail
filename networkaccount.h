/* -*- c++ -*-
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#ifndef __KMAIL_NETWORKACCOUNT_H__
#define __KMAIL_NETWORKACCOUNT_H__

#include "kmaccount.h"

#include "sieveconfig.h"

#include <QString>

class AccountManager;
class KConfig/*Base*/;
class KUrl;
namespace KIO {
  class Slave;
  class MetaData;
}

namespace KMail {

  class NetworkAccount : public KMAccount {
    Q_OBJECT
  protected:
    NetworkAccount( AccountManager * parent, const QString & name, uint id );
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

    /** @return the SieveConfig for this account */
    KMail::SieveConfig sieveConfig() const { return mSieveConfig; }
    virtual void setSieveConfig( const KMail::SieveConfig & config );

    /** Configure the slave by adding to the meta data map */
    virtual KIO::MetaData slaveConfig() const;

    // This class is the reason that readConfig can't take a const KConfigGroup
    // (it deletes entries from it when migrating to kwallet),
    // and writeConfig can't be const (it sets member vars).
    virtual void readConfig( KConfigGroup & config );
    virtual void writeConfig( KConfigGroup & config );

    /** @return an URL for this account */
    virtual KUrl getUrl() const;

    /** @return the KIO slave for this account */
    KIO::Slave * slave() const { return mSlave; }

    /** Kill all jobs that are currently in progress */
    virtual void killAllJobs( bool disconnectSlave = false ) = 0;

    /** Read password from wallet, used for on-demand wallet opening */
    void readPassword();

    virtual bool mailCheckCanProceed() const;

    virtual void setCheckingMail( bool checking );

    /** Reset connection list for the account */
    static void resetConnectionList( NetworkAccount* acct );
  protected:
    virtual QString protocol() const = 0;
    virtual unsigned short int defaultPort() const = 0;

  protected:
    KMail::SieveConfig mSieveConfig;
    KIO::Slave * mSlave;
    QString mLogin, mPasswd, mAuth, mHost, mOldPassKey;
    unsigned short int mPort;
    bool mStorePasswd : 1;
    bool mUseSSL : 1;
    bool mUseTLS : 1;
    bool mAskAgain : 1;
    bool mPasswdDirty, mStorePasswdInConfig;
  };

} // namespace KMail

#endif // __KMAIL_NETWORKACCOUNT_H__
