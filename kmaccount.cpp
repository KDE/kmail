// KMail Account

#include <stdlib.h>
#include <unistd.h>

#include <qdir.h>
#include <qstrlist.h>
#include <qtstream.h>
#include <qfile.h>
#include <assert.h>
#include <kconfig.h>
#include <qobject.h>

#include "kmacctmgr.h"
#include "kmacctfolder.h"
#include "kmaccount.h"
#include "kmaccount.moc"


//-----------------------------------------------------------------------------
KMAccount::KMAccount(KMAcctMgr* aOwner, const char* aName)
{
  initMetaObject();
  assert(aOwner != NULL);

  mOwner   = aOwner;
  mName    = aName;
  mFolder  = NULL;
  mConfig  = NULL;
}


//-----------------------------------------------------------------------------
KMAccount::~KMAccount() 
{
  if (mConfig)  delete mConfig;
  if (mFolder) mFolder->removeAccount(this);
}


//-----------------------------------------------------------------------------
void KMAccount::setName(const QString& aName)
{
  mOwner->rename(this, aName);
  mName = aName;
}


//-----------------------------------------------------------------------------
void KMAccount::setFolder(KMAcctFolder* aFolder)
{
  mFolder = aFolder;
}


//-----------------------------------------------------------------------------
void KMAccount::openConfig(void)
{
  QString acctPath;

  if (mConfig) return;

  acctPath = mOwner->basePath() + "/" + mName;
  mConfig  = new KConfig(acctPath);
}


//-----------------------------------------------------------------------------
void KMAccount::takeConfig(KConfig* aConfig)
{
  assert(aConfig != NULL);

  mConfig  = aConfig;
  readConfig();
}
