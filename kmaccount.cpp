// KMail Account

#include <stdlib.h>
#include <unistd.h>

#include <qdir.h>
#include <qstrlist.h>
#include <qtstream.h>
#include <qfile.h>
#include <assert.h>

#include "kmaccount.h"
#include "kmacctmgr.h"
#include "kmacctfolder.h"


//-----------------------------------------------------------------------------
KMAccount::KMAccount(KMAcctMgr* aOwner, const char* aName)
{
  assert(aOwner != NULL);

  mOwner   = aOwner;
  mName    = aName;
  mFolder  = NULL;
  mCFile   = NULL;
  mCStream = NULL;
  mConfig  = NULL;
}


//-----------------------------------------------------------------------------
KMAccount::~KMAccount() 
{
  if (mConfig)  delete mConfig;
  if (mCStream) delete mCStream;
  if (mCFile)   delete mCFile;
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

  if (mCFile) return;

  acctPath = mOwner->basePath() + "/" + mName;
  mCFile   = new QFile(acctPath);
  mCFile->open(IO_ReadWrite);
  mCStream = new QTextStream(mCFile);
  mConfig  = new KConfig(mCStream);
}


//-----------------------------------------------------------------------------
void KMAccount::takeConfig(KConfig* aConfig, QFile* aCFile, 
			   QTextStream*  aCStream)
{
  assert(aConfig != NULL);
  assert(aCFile != NULL);
  assert(aCStream != NULL);

  mConfig  = aConfig;
  mCFile   = aCFile;
  mCStream = aCStream;
  readConfig();
}
