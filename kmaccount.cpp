// KMail Account

#include <stdlib.h>
#include <unistd.h>

#include <qdir.h>
#include <qstrlist.h>
#include <qtstream.h>
#include <qfile.h>
#include <assert.h>
#include <kconfig.h>
#include <klocale.h>

#include "kmacctmgr.h"
#include "kmacctfolder.h"
#include "kmaccount.h"
#include "kmglobal.h"
#include "kmfoldermgr.h"

//----------------------
#include "kmaccount.moc"

//-----------------------------------------------------------------------------
KMAccount::KMAccount(KMAcctMgr* aOwner, const char* aName)
{
  initMetaObject();
  assert(aOwner != NULL);

  mOwner   = aOwner;
  mName    = aName;
  mFolder  = NULL;
}


//-----------------------------------------------------------------------------
KMAccount::~KMAccount() 
{
  if (!shuttingDown && mFolder) mFolder->removeAccount(this);
}


//-----------------------------------------------------------------------------
void KMAccount::setName(const QString& aName)
{
  mName = aName;
}


//-----------------------------------------------------------------------------
void KMAccount::setFolder(KMFolder* aFolder)
{
  mFolder = (KMAcctFolder*)aFolder;
  debug("setFolder of %s to %s", (const char*)name(),
	aFolder ? (const char*)aFolder->name() : "(null)");
}


//-----------------------------------------------------------------------------
void KMAccount::readConfig(KConfig& config)
{
  KMAcctFolder* folder;
  QString folderName;

  mFolder = NULL;
  mName   = config.readEntry("Name", nls->translate("Unnamed"));
  folderName = config.readEntry("Folder", "");

  if (!folderName.isEmpty())
  {
    folder = (KMAcctFolder*)folderMgr->find(folderName);
    if (folder) 
    {
      mFolder = folder;
      mFolder->addAccount(this);
    }
    else debug("Cannot find folder `%s' for account `%s'.", 
	       (const char*)folderName, (const char*)mName);
  }
}


//-----------------------------------------------------------------------------
void KMAccount::writeConfig(KConfig& config)
{
  config.writeEntry("Type", type());
  config.writeEntry("Name", mName);
  config.writeEntry("Folder", mFolder ? (const char*)mFolder->name() : "");
}

