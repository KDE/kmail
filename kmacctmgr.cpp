// KMail Account Manager

#include "kmacctmgr.h"
#include "kmacctlocal.h"
#include "kmacctpop.h"
#include "kmglobal.h"

#include <assert.h>
#include <qdir.h>
#include <qfile.h>
#include <qmsgbox.h>
#include <Kconfig.h>
#include <klocale.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

//-----------------------------------------------------------------------------
KMAcctMgr::KMAcctMgr(const char* aBasePath): KMAcctMgrInherited()
{
  assert(aBasePath != NULL);

  mAcctList.setAutoDelete(TRUE);

  setBasePath(aBasePath);
}


//-----------------------------------------------------------------------------
KMAcctMgr::~KMAcctMgr()
{
  KMAccount* cur;

  for (cur=mAcctList.first(); cur; cur=mAcctList.next())
    cur->writeConfig();

  mAcctList.clear();
}


//-----------------------------------------------------------------------------
void KMAcctMgr::setBasePath(const char* aBasePath)
{
  assert(aBasePath != NULL);

  if (aBasePath[0] == '~')
  {
    mBasePath = QDir::homeDirPath();
    mBasePath.append("/");
    mBasePath.append(aBasePath+1);
  }
  else mBasePath = aBasePath;

  mBasePath.detach();
  reload();
}


//-----------------------------------------------------------------------------
bool KMAcctMgr::reload(void)
{
  QDir         dir;
  QStrList*    list;  // Qt1.2 has non-const QList::first()  X-)
                      // I understand why, but that does not make it better.
  QString      acctName, acctPath;
  KConfig*     config;
  QFile*       cFile;
  QTextStream* cStream;
  KMAccount*   act;

  mAcctList.clear();

  dir.setFilter(QDir::Files | QDir::Hidden);
  dir.setNameFilter("*");
  
  dir.setPath(mBasePath);
  if (!dir.exists())
  {
    warning(nls->translate("Directory\n%s\ndoes not exist.\n\n"
	    "KMail will create it now."), (const char*)mBasePath);
    // dir.mkdir(mBasePath, TRUE);
    // Stephan: mkdir without right permissions is dangerous
    // and is for sure a port from Windows ;)
    mkdir(mBasePath.data(), 0700);
  }

  if (!dir.cd(mBasePath, TRUE))
  {
    warning(nls->translate("Cannot enter directory:\n%s"), 
	    (const char*)mBasePath);
    return FALSE;
  }

  if (!(list=(QStrList*)dir.entryList()))
  {
    warning(nls->translate("Directory is unreadable:\n%s"), 
	    (const char*)mBasePath);
    return FALSE;
  }

  for (acctName=list->first(); acctName; acctName=list->next())
  {
    acctPath = mBasePath+"/"+acctName;
    cFile   = new QFile(acctPath);
    cFile->open(IO_ReadWrite);
    cStream = new QTextStream(cFile);
    config  = new KConfig(cStream);

    config->setGroup("Account");
    act = create(config->readEntry("type"), acctName);
    if (act) act->takeConfig(config, cFile, cStream);
    else
    {
      warning(nls->translate("Cannot read configuration of account '%s'\n"
			     "from broken file %s\nAccount type: %s\n"),
	      (const char*)acctName, acctPath.data(), 
	      (const char*)config->readEntry("type"));

      delete config;
      delete cStream;
      delete cFile;

      unlink(mBasePath+"/"+acctName);
    }
  }

  return TRUE;
}


//-----------------------------------------------------------------------------
KMAccount* KMAcctMgr::create(const QString& aType, const QString& aName) 
{
  KMAccount* act = NULL;

  if (aType == "local") 
    act = new KMAcctLocal(this, aName);

  else if (aType == "pop") 
    act = new KMAcctPop(this, aName);

  if (act) 
  {
    act->openConfig();
    mAcctList.append(act);
  }
  return act;
}


//-----------------------------------------------------------------------------
KMAccount* KMAcctMgr::find(const QString& aName) 
{
  KMAccount* cur;

  if (aName.isEmpty()) return NULL;

  for (cur=mAcctList.first(); cur; cur=mAcctList.next())
  {
    if (cur->name() == aName) return cur;
  }

  return NULL;
}


//-----------------------------------------------------------------------------
KMAccount* KMAcctMgr::first(void)
{
  return mAcctList.first();
}


//-----------------------------------------------------------------------------
KMAccount* KMAcctMgr::next(void)
{
  return mAcctList.next();
}


//-----------------------------------------------------------------------------
bool KMAcctMgr::rename(const KMAccount* acct, const char* newName) 
{
  QDir dir;

  assert(acct != NULL);
  assert(newName != NULL);

  if (!dir.cd(mBasePath))
  {
    warning(nls->translate("Cannot enter directory:\n%s"), 
	    (const char*)mBasePath);
    return FALSE;
  }

  if (!dir.rename(acct->name(), newName, FALSE))
  {
    warning(nls->translate("Cannot rename account file\n%s to %s\nin directory %s"),
	    (const char*)acct->name(), newName, (const char*)mBasePath);
    return FALSE;
  }

  return TRUE;
}


//-----------------------------------------------------------------------------
bool KMAcctMgr::remove(KMAccount* acct)
{
  QDir dir;
  QString acctName;

  assert(acct != NULL);

  acctName = acct->name();

  if (!dir.cd(mBasePath))
  {
    warning(nls->translate("Cannot enter directory:\n%s"), (const char*)mBasePath);
    return FALSE;
  }

  mAcctList.remove(acct);
   dir.remove(acctName);

  return TRUE;
}


//-----------------------------------------------------------------------------
bool KMAcctMgr::checkMail(void)
{
  KMAccount* cur;
  bool hasNewMail = FALSE;

  for (cur=mAcctList.first(); cur; cur=mAcctList.next())
  {
    if (cur->processNewMail())
    {
      hasNewMail = TRUE;
      emit newMail(cur);
    }
  }
  return hasNewMail;
}


//-----------------------------------------------------------------------------
#include "kmacctmgr.moc"
