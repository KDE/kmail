// KMail Account Manager

#include "kmacctmgr.h"
#include "kmacctmaildir.h"
#include "kmacctlocal.h"
#include "kmacctexppop.h"
#include "kmacctimap.h"
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
#include <kdebug.h>

//-----------------------------------------------------------------------------
KMAcctMgr::KMAcctMgr(): KMAcctMgrInherited()
{
  mAcctList.setAutoDelete(TRUE);
  mAccountIt = new QPtrListIterator<KMAccount>(mAcctList);
  mAcctChecking = new QPtrList<KMAccount>();
  checking = false;
  lastAccountChecked = 0;
}


//-----------------------------------------------------------------------------
KMAcctMgr::~KMAcctMgr()
{
  delete mAccountIt;
  delete mAcctChecking;
  writeConfig(FALSE);
  mAcctList.clear();
}


//-----------------------------------------------------------------------------
void KMAcctMgr::writeConfig(bool withSync)
{
  KConfig* config = kapp->config();
  KMAccount* acct;
  QString groupName;
  int i;

  KConfigGroupSaver saver(config, "General");
  config->writeEntry("accounts", mAcctList.count());

  for (i=1,acct=mAcctList.first(); acct; acct=mAcctList.next(),i++)
  {
    groupName.sprintf("Account %d", i);
    KConfigGroupSaver saver(config, groupName);
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
  *mAccountIt = mAcctList;

  KConfigGroupSaver saver(config, "General");
  num = config->readNumEntry("accounts", 0);

  for (i=1; i<=num; i++)
  {
    groupName.sprintf("Account %d", i);
    KConfigGroupSaver saver(config, groupName);
    acctType = config->readEntry("Type");
    // Provide backwards compatibility
    if (acctType == "advanced pop" || acctType == "experimental pop")
      acctType = "pop";
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

  if (!mAcctChecking->contains(account)) mAcctChecking->append(account);

  if (checking) {
    return;
  }

//   if (account->folder() == 0)
//   {
//     QString tmp; //Unsafe
//     tmp = i18n("Account %1 has no mailbox defined!\n"
//  	        "Mail checking aborted\n"
// 	        "Check your account settings!")
// 		.arg(account->name());
//     KMessageBox::information(0,tmp);
//     return;
//   }

  checking = true;

  kdDebug(5006) << "checking mail, server busy" << endl;
  kernel->serverReady (false);
  lastAccountChecked = 0;

  processNextCheck(false);

//   mAccountIt->toLast();
//   ++(*mAccountIt);

//   lastAccountChecked = account;
//   connect( account, SIGNAL(finishedCheck(bool)),
// 	   this, SLOT(processNextCheck(bool)) );
//   account->processNewMail(interactive);
}

void KMAcctMgr::processNextCheck(bool _newMail)
{
  KMAccount *curAccount = 0; 
  newMailArrived |= _newMail;

  if (lastAccountChecked)
    disconnect( lastAccountChecked, SIGNAL(finishedCheck(bool)),
		this, SLOT(processNextCheck(bool)) );

  if (mAcctChecking->isEmpty() || 
      (((curAccount = mAcctChecking->take(0)) == lastAccountChecked))) {
    kernel->filterMgr()->cleanup();
    kdDebug(5006) << "checked mail, server ready" << endl;
    kernel->serverReady (true);
    checking = false;
    emit checkedMail(newMailArrived, FALSE);
    return;
  }

  connect( curAccount, SIGNAL(finishedCheck(bool)),
	   this, SLOT(processNextCheck(bool)) );

  lastAccountChecked = curAccount;

  if (curAccount->folder() == 0)
    {
      QString tmp; //Unsafe
      tmp = i18n("Account %1 has no mailbox defined!\n"
		 "Mail checking aborted\n"
		 "Check your account settings!")
	         .arg(curAccount->name());
      KMessageBox::information(0,tmp);
      processNextCheck(false);
    }

  kdDebug(5006) << "processing next mail check, server busy" << endl;

  curAccount->processNewMail(interactive);
}

//-----------------------------------------------------------------------------
KMAccount* KMAcctMgr::create(const QString &aType, const QString &aName)
{
  KMAccount* act = NULL;

  if (aType == "local")
    act = new KMAcctLocal(this, aName);

  if (aType == "maildir")
    act = new KMAcctMaildir(this, aName);

  else if (aType == "pop")
    act = new KMAcctExpPop(this, aName);

  else if (aType == "imap")
    act = new KMAcctImap(this, aName);

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
KMAccount* KMAcctMgr::find(const QString &aName)
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
    kdDebug(5006) << "checked mail, server ready" << endl;
    kernel->serverReady (true);
    checking = false;
    emit checkedMail(newMailArrived, TRUE);
    return;
  }

  connect( cur, SIGNAL(finishedCheck(bool)),
	   this, SLOT(processNextAccount(bool)) );

  lastAccountChecked = cur;
  ++(*mAccountIt);

  if (cur->type() != "imap" && cur->folder() == 0)
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
QStringList  KMAcctMgr::getAccounts(bool noImap) {

  KMAccount *cur;
  QStringList strList;
  for (cur=mAcctList.first(); cur; cur=mAcctList.next()) {
    if (!noImap || cur->type() != "imap") strList.append(cur->name());
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
  while (cur)
  {
    x++;
    if (x > item) break;
    cur=mAcctList.next();
  }

  if (cur->type() != "imap" && cur->folder() == 0)
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

  kdDebug(5006) << "checking mail, server busy" << endl;
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
