/**
 * kmacctimap.cpp
 *
 * Copyright (c) 2000 Michael Haeckel <Michael@Haeckel.Net>
 *
 * This file is based on kmacctexppop.cpp by Don Sanders
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "kmacctimap.moc"

#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>

#include <kmfolderimap.h>
#include <kmmessage.h>
#include <kconfig.h>
#include <qpushbutton.h>
#include <kapp.h>

#include "kmacctimap.h"
#include "kmglobal.h"
#include "kmacctfolder.h"
#include <kprocess.h>
#include <qtooltip.h>

//-----------------------------------------------------------------------------
KMAcctImap::KMAcctImap(KMAcctMgr* aOwner, const QString& aAccountName):
  KMAcctImapInherited(aOwner, aAccountName)
{
  init();
}


//-----------------------------------------------------------------------------
KMAcctImap::~KMAcctImap()
{
}


//-----------------------------------------------------------------------------
const char* KMAcctImap::type(void) const
{
  return "imap";
}

//-----------------------------------------------------------------------------
void KMAcctImap::init(void)
{
  mHost   = "";
  struct servent *serv = getservbyname("imap-4", "tcp");
  if (serv) {
    mPort = ntohs(serv->s_port);
  } else {
    mPort = 143;
  }
  mLogin  = "";
  mPasswd = "";
  mAuth = "*";
  mStorePasswd = FALSE;
  mPrefix = "/";
  mAutoExpunge = TRUE;
  mHiddenFolders = FALSE;
  mUseSSL = FALSE;
  mUseTLS = FALSE;
}

//-----------------------------------------------------------------------------
void KMAcctImap::pseudoAssign(KMAccount* account)
{
  assert(account->type() == "imap");
  KMAcctImap *acct = static_cast<KMAcctImap*>(account);
  setName(acct->name());
  setCheckInterval( 0 );
  setCheckExclude( TRUE );
  setFolder(acct->folder());
  setHost(acct->host());
  setPort(acct->port());
  setPrefix(acct->prefix());
  setLogin(acct->login());
  setAuth(acct->auth());
  setAutoExpunge(acct->autoExpunge());
  setHiddenFolders(acct->hiddenFolders());
  setStorePasswd(acct->storePasswd());
  setPasswd(acct->passwd(), acct->storePasswd());
  setUseSSL(acct->useSSL());
  setUseTLS(acct->useTLS());
}

//-----------------------------------------------------------------------------
void KMAcctImap::readConfig(KConfig& config)
{
  KMAcctImapInherited::readConfig(config);

  mLogin = config.readEntry("login", "");
  mStorePasswd = config.readNumEntry("store-passwd", FALSE);
  if (mStorePasswd) mPasswd = config.readEntry("passwd");
  else mPasswd = "";
  mHost = config.readEntry("host");
  mPort = config.readNumEntry("port");
  mAuth = config.readEntry("auth", "*");
  mPrefix = config.readEntry("prefix", "/");
  mAutoExpunge = config.readBoolEntry("auto-expunge", TRUE);
  mHiddenFolders = config.readBoolEntry("hidden-folders", FALSE);
  mUseSSL = config.readBoolEntry("use-ssl", FALSE);
  mUseTLS = config.readBoolEntry("use-tls", FALSE);
}


//-----------------------------------------------------------------------------
void KMAcctImap::writeConfig(KConfig& config)
{
  KMAcctImapInherited::writeConfig(config);

  config.writeEntry("login", mLogin);
  config.writeEntry("store-passwd", mStorePasswd);
  if (mStorePasswd) config.writeEntry("passwd", mPasswd);
  else config.writeEntry("passwd", "");

  config.writeEntry("host", mHost);
  config.writeEntry("port", static_cast<int>(mPort));
  config.writeEntry("auth", mAuth);
  config.writeEntry("prefix", mPrefix);
  config.writeEntry("auto-expunge", mAutoExpunge);
  config.writeEntry("hidden-folders", mHiddenFolders);
  config.writeEntry("use-ssl", mUseSSL);
  config.writeEntry("use-tls", mUseTLS);
}


//-----------------------------------------------------------------------------
QString KMAcctImap::encryptStr(const QString &aStr) const
{
  unsigned int i, val;
  unsigned int len = aStr.length();
  QCString result;
  result.resize(len+1);

  for (i=0; i<len; i++)
  {
    val = aStr[i] - ' ';
    val = (255-' ') - val;
    result[i] = (char)(val + ' ');
  }
  result[i] = '\0';

  return result;
}


//-----------------------------------------------------------------------------
QString KMAcctImap::decryptStr(const QString &aStr) const
{
  return encryptStr(aStr);
}


//-----------------------------------------------------------------------------
void KMAcctImap::setStorePasswd(bool b)
{
  mStorePasswd = b;
}


//-----------------------------------------------------------------------------
void KMAcctImap::setUseSSL(bool b)
{
  mUseSSL = b;
}


//-----------------------------------------------------------------------------
void KMAcctImap::setUseTLS(bool b)
{
  mUseTLS = b;
}


//-----------------------------------------------------------------------------
void KMAcctImap::setLogin(const QString& aLogin)
{
  mLogin = aLogin;
}


//-----------------------------------------------------------------------------
QString KMAcctImap::passwd(void) const
{
  return decryptStr(mPasswd);
}


//-----------------------------------------------------------------------------
void KMAcctImap::setPasswd(const QString& aPasswd, bool aStoreInConfig)
{
  mPasswd = encryptStr(aPasswd);
  mStorePasswd = aStoreInConfig;
}


//-----------------------------------------------------------------------------
void KMAcctImap::setHost(const QString& aHost)
{
  mHost = aHost;
}


//-----------------------------------------------------------------------------
void KMAcctImap::setPort(unsigned short int aPort)
{
  mPort = aPort;
}


//-----------------------------------------------------------------------------
void KMAcctImap::setPrefix(const QString& aPrefix)
{
  mPrefix = aPrefix;
  mPrefix.replace(QRegExp("[%*\"]"), "");
  if (mPrefix.isEmpty() || mPrefix.at(0) != '/') mPrefix = '/' + mPrefix;
  if (mPrefix.at(mPrefix.length() - 1) != '/') mPrefix += '/';
}


//-----------------------------------------------------------------------------
void KMAcctImap::setAutoExpunge(bool aAutoExpunge)
{
  mAutoExpunge = aAutoExpunge;
}

//-----------------------------------------------------------------------------
void KMAcctImap::setHiddenFolders(bool aHiddenFolders)
{
  mHiddenFolders = aHiddenFolders;
}


//-----------------------------------------------------------------------------
void KMAcctImap::setAuth(const QString& aAuth)
{
  mAuth = aAuth;
}
