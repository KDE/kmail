// KMail Account Manager

#include "kmacctmgr.h"
#include "kmacctlocal.h"
#include "kmacctpop.h"
#include "kmacctexppop.h"
#include "kmglobal.h"
#include "kbusyptr.h"
#include "kmfiltermgr.h"

#include <qlabel.h>

#include <assert.h>
#include <kconfig.h>
#include <kapp.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream.h>
#include <klocale.h>
#include <kmessagebox.h>


//-----------------------------------------------------------------------------
KMAcctMgr::KMAcctMgr(const char* aBasePath): KMAcctMgrInherited()
{
  assert(aBasePath != NULL);
  mAcctList.setAutoDelete(TRUE);
  setBasePath(aBasePath);
  mAccountIt = new QListIterator<KMAccount>(mAcctList);
  checking = false;
  lastAccountChecked = 0;
}


//-----------------------------------------------------------------------------
KMAcctMgr::~KMAcctMgr()
{
  delete mAccountIt;
  writeConfig(FALSE);
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
}


//-----------------------------------------------------------------------------
void KMAcctMgr::writeConfig(bool withSync)
{
  KConfig* config = kapp->config();
  KMAccount* acct;
  QString groupName;
  int i;

  config->setGroup("General");
  config->writeEntry("accounts", mAcctList.count());

  for (i=1,acct=mAcctList.first(); acct; acct=mAcctList.next(),i++)
  {
    groupName.sprintf("Account %d", i);
    config->setGroup(groupName);
    acct->writeConfig(*config);
  }
  if (withSync) config->sync();
}


//-----------------------------------------------------------------------------
void KMAcctMgr::readConfig(void)
{
  KConfig* config = kapp->config();
  KMAccount* acct;
  QString groupName, acctType, acctName;
  int i, num;

  mAcctList.clear();

  config->setGroup("General");
  num = config->readNumEntry("accounts", 0);

  for (i=1; i<=num; i++)
  {
    groupName.sprintf("Account %d", i);
    config->setGroup(groupName);
    acctType = config->readEntry("Type");
    acctName = config->readEntry("Name");
    acct = create(acctType, acctName);
    if (!acct) continue;
    add(acct);
    acct->readConfig(*config);
  }
}


//-----------------------------------------------------------------------------
void KMAcctMgr::singleCheckMail(KMAccount *account, bool _interactive)
{
  newMailArrived = false;
  interactive = _interactive;

  if (checking)
    return;

  if (account->folder() == 0)
  {
    QString tmp; //Unsafe
    tmp = i18n("Account %1 has no mailbox defined!\n"
 	        "Mail checking aborted\n"
	        "Check your account settings!")
		.arg(account->name());
    KMessageBox::information(0,tmp);
    return;
  }

  checking = true;

  debug ("checking mail, server busy");
  kernel->serverReady (false);

  mAccountIt->toLast(); 
  ++(*mAccountIt);

  lastAccountChecked = account;
  connect( account, SIGNAL(finishedCheck(bool)),
	   this, SLOT(processNextAccount(bool)) );
  account->processNewMail(interactive);
}


//-----------------------------------------------------------------------------
KMAccount* KMAcctMgr::create(const QString aType, const QString aName) 
{
  KMAccount* act = NULL;

  if (stricmp(aType,"local")==0) 
    act = new KMAcctLocal(this, aName);

  else if (stricmp(aType,"pop")==0) 
    act = new KMAcctPop(this, aName);

  else if (stricmp(aType,"experimental pop")==0) 
    act = new KMAcctExpPop(this, aName);

  else if (stricmp(aType,"advanced pop")==0) 
    act = new KMAcctExpPop(this, aName);

  if (act) 
    act->setFolder(kernel->inboxFolder());

  return act;
}


//-----------------------------------------------------------------------------
void KMAcctMgr::add(KMAccount *account) 
{
  if (account)
    mAcctList.append(account);
}


//-----------------------------------------------------------------------------
KMAccount* KMAcctMgr::find(const QString aName) 
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
bool KMAcctMgr::remove(KMAccount* acct)
{
  //assert(acct != NULL);
  if(!acct)
    return FALSE;
  mAcctList.remove(acct);
  return TRUE;
}

//-----------------------------------------------------------------------------
void KMAcctMgr::checkMail(bool _interactive)
{
  newMailArrived = false;
  interactive = _interactive;

  if (checking)
    return;

  if (mAcctList.isEmpty())
  {
    KMessageBox::information(0,i18n("You need to add an account in the network\n"
				    "section of the settings in order to\n"
				    "receive mail."));
    return;
  }

  checking = true;

  kernel->serverReady (false);
  
  mAccountIt->toFirst(); 
  lastAccountChecked = 0;
  processNextAccount(false);
}

void KMAcctMgr::processNextAccount(bool _newMail)
{
  KMAccount *cur = mAccountIt->current();
  newMailArrived |= _newMail;
  if (lastAccountChecked)
    disconnect( lastAccountChecked, SIGNAL(finishedCheck(bool)),
		this, SLOT(processNextAccount(bool)) );

  if (!cur) {
    kernel->filterMgr()->cleanup();
    debug ("checked mail, server ready");
    kernel->serverReady (true);
    checking = false;
    emit checkedMail(newMailArrived);
    return;
  }
  
  connect( cur, SIGNAL(finishedCheck(bool)),
	   this, SLOT(processNextAccount(bool)) );

  lastAccountChecked = cur;
  ++(*mAccountIt);

  if (cur->folder() == 0)
    {
      QString tmp; 
      tmp = i18n("Account %1 has no mailbox defined!\n"
		 "Mail checking aborted\n"
		 "Check your account settings!")
	.arg(cur->name());
      KMessageBox::information(0,tmp);
      processNextAccount(false);
    }
  else   if (cur->checkExclude())
    {
      // Account excluded from mail check.
      processNextAccount(false);
    }
  else cur->processNewMail(interactive);
}


//-----------------------------------------------------------------------------
QStringList  KMAcctMgr::getAccounts() {
  
  KMAccount *cur;
  QStringList strList;
  for (cur=mAcctList.first(); cur; cur=mAcctList.next()) {
    strList.append(cur->name());
  }

  return strList;

}

//-----------------------------------------------------------------------------
void KMAcctMgr::intCheckMail(int item, bool _interactive) {

  KMAccount* cur;
  newMailArrived = false;
  interactive = _interactive;

  if (checking)
    return;

  if (mAcctList.isEmpty())
  {
    KMessageBox::information(0,i18n("You need to add an account in the network"
				    "\n"
				    "section of the settings in order to\n"
				    "receive mail."));
    return;
  }

  int x = 0;
  cur = mAcctList.first();
  for(x=0; x < item; x++)
  {
    cur=mAcctList.next();
  }

  if (cur->folder() == 0)
  {
    QString tmp; 
    tmp = i18n("Account %1 has no mailbox defined!\n"
                     "Mail checking aborted\n"
                     "Check your account settings!")
		.arg(cur->name());
    KMessageBox::information(0,tmp);
    return;
  }

  checking = true;

  debug ("checking mail, server busy");
  kernel->serverReady (false);

  mAccountIt->toLast(); 
  ++(*mAccountIt);

  lastAccountChecked = cur;
  connect( cur, SIGNAL(finishedCheck(bool)),
	   this, SLOT(processNextAccount(bool)) );
  cur->processNewMail(interactive);
}


//-----------------------------------------------------------------------------
#include "kmacctmgr.moc"
