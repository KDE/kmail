// kmacctpop.cpp

#include "kmacctpop.h"
#include <assert.h>


//-----------------------------------------------------------------------------
KMAcctPop::KMAcctPop(KMAcctMgr* aOwner, const char* aAccountName):
  KMAcctPopInherited(aOwner, aAccountName)
{
  mStorePasswd = FALSE;
  mPort = 0;
}


//-----------------------------------------------------------------------------
KMAcctPop::~KMAcctPop()
{
  writeConfig();
}


//-----------------------------------------------------------------------------
const char* KMAcctPop::type(void) const
{
  return "pop";
}


//-----------------------------------------------------------------------------
bool KMAcctPop::processNewMail(void)
{
  return FALSE;
}


//-----------------------------------------------------------------------------
void KMAcctPop::readConfig(void)
{
  mLogin = mConfig->readEntry("login");
  mStorePasswd = mConfig->readNumEntry("store-passwd");
  if (mStorePasswd) mPasswd = mConfig->readEntry("passwd");
  else mPasswd="?";
  mHost = mConfig->readEntry("host");
  mPort = mConfig->readNumEntry("port");
  mProtocol = mConfig->readNumEntry("protocol");
}


//-----------------------------------------------------------------------------
void KMAcctPop::writeConfig(void)
{
  mConfig->writeEntry("login", mLogin);
  if (mStorePasswd) mConfig->writeEntry("password", mPasswd);
  else mConfig->writeEntry("passwd", "");
  mConfig->writeEntry("store-passwd", mStorePasswd);
  mConfig->writeEntry("host", mHost);
  mConfig->writeEntry("port", mPort);
  mConfig->writeEntry("protocol", mProtocol);
  mConfig->sync();
}


//-----------------------------------------------------------------------------
void KMAcctPop::setLogin(const QString& aLogin)
{
  mLogin = aLogin;
}


//-----------------------------------------------------------------------------
void KMAcctPop::setPasswd(const QString& aPasswd, bool aStoreInConfig)
{
  mPasswd = aPasswd;
  mStorePasswd = aStoreInConfig;
}


//-----------------------------------------------------------------------------
void KMAcctPop::setHost(const QString& aHost)
{
  mHost = aHost;
}


//-----------------------------------------------------------------------------
void KMAcctPop::setPort(int aPort)
{
  mPort = aPort;
}


//-----------------------------------------------------------------------------
void KMAcctPop::setProtocol(short aProtocol)
{
  assert(aProtocol==2 || aProtocol==3);
  mProtocol = aProtocol;
}


