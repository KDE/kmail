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
  if (mCFile)   delete mCFile;
  if (mCStream) delete mCStream;
  if (mConfig)  delete mConfig;
  if (mFolder) mFolder->removeAccount(this);
}


//-----------------------------------------------------------------------------
void KMAccount::setName(const QString& aName)
{
  mName = aName;
  mOwner->rename(this, aName);
}


//-----------------------------------------------------------------------------
void KMAccount::setFolder(KMAcctFolder* aFolder)
{
  mFolder = aFolder;
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


#ifdef BROKEN
//-----------------------------------------------------------------------------
void KMAccount::getLocation(QString *s)
{
  QString t=mConfig->readEntry("type");
  if (t=="inbox") {
    *s=config->readEntry("location");
  } else if (t=="pop3") {
    s->sprintf("{%s:%s/user=%s/service=pop3}",
	       (const char *)config->readEntry("host"),
	       (const char *)config->readEntry("port"),
	       (const char *)config->readEntry("login"));
  } else {
    s->sprintf("{%s:%s/user=%s/service=imap}%s",
	       (const char *)config->readEntry("host"),
	       (const char *)config->readEntry("port"),
	       (const char *)config->readEntry("mailbox"),
	       (const char *)config->readEntry("login"));
  }
}


//-----------------------------------------------------------------------------
void KMAccount::init(const QString& aLogin,const QString& aPasswd)
{
  config->writeEntry("login",aLogin);
  config->writeEntry("password",aPasswd);
}
#endif

