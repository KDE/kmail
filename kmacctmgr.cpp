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
    warning("Directory\n"+mBasePath+"\ndoes not exist.\n\n"
	    "KMail will create it now.");
    // dir.mkdir(mBasePath, TRUE);
    // Stephan: mkdir without right permissions is dangerous
    // and is for sure a port from Windows ;)
    mkdir(mBasePath.data(), 0700);
  }

  if (!dir.cd(mBasePath, TRUE))
  {
    warning("Cannot enter directory '" + mBasePath + "'.\n");
    return FALSE;
  }

  if (!(list=(QStrList*)dir.entryList()))
  {
    warning("Directory '"+mBasePath+"' is unreadable.\n");
    return FALSE;
  }

  for (acctName=list->first(); acctName; acctName=list->next())
  {
    acctPath = mBasePath+"/"+acctName;
    cFile   = new QFile(acctPath);
    cFile->open(IO_ReadWrite);
    cStream = new QTextStream(cFile);
    config  = new KConfig(cStream);

    act = create(config->readEntry("type"), acctName);
    act->takeConfig(config, cFile, cStream);
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

  else
    warning("cannot access account '" + aName + 
	    "' which is of unknown type '" + aType + "'.");

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

  assert(!aName.isEmpty());

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
    warning("Cannot enter directory\n" + mBasePath);
    return FALSE;
  }

  if (!dir.rename(acct->name(), newName, FALSE))
  {
    warning("Cannot rename\n" + mBasePath + "/" + acct->name() +
	    "\nto\n" + mBasePath + "/" + newName + "'.\n");
    return FALSE;
  }

  return TRUE;
}


//-----------------------------------------------------------------------------
bool KMAcctMgr::remove(KMAccount* acct)
{
  QDir dir;
  QString acctName = acct->name();

  assert(acct != NULL);

  if (!dir.cd(mBasePath))
  {
    warning(QString(nls->translate("Cannot enter directory")) + QString("\n")
	                   + mBasePath);
    return FALSE;
  }

  mAcctList.remove(acct);
  delete acct;
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
