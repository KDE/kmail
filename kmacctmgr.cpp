// KMail Account Manager

#include <config.h>
#include "kmacctmgr.h"
#include "kmacctmaildir.h"
#include "kmacctlocal.h"
#include "kmacctexppop.h"
#include "kmacctimap.h"
#include "kmbroadcaststatus.h"


#include <assert.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kapplication.h>

#include <qstringlist.h>
#include <qregexp.h>

//-----------------------------------------------------------------------------
KMAcctMgr::KMAcctMgr(): KMAcctMgrInherited()
{
  mAcctList.setAutoDelete(TRUE);
  checking = false;
  moreThanOneAccount = false;
  lastAccountChecked = 0;
  mTotalNewMailsArrived=0;
}


//-----------------------------------------------------------------------------
KMAcctMgr::~KMAcctMgr()
{
  writeConfig(FALSE);
}


//-----------------------------------------------------------------------------
void KMAcctMgr::writeConfig(bool withSync)
{
  KConfig* config = kapp->config();
  QString groupName;

  KConfigGroupSaver saver(config, "General");
  config->writeEntry("accounts", mAcctList.count());

  // first delete all account groups in the config file:
  QStringList accountGroups =
    config->groupList().grep( QRegExp( "Account \\d+" ) );
  for ( QStringList::Iterator it = accountGroups.begin() ;
	it != accountGroups.end() ; ++it )
    config->deleteGroup( *it );

  // now write new account groups:
  int i = 1;
  for ( QPtrListIterator<KMAccount> it(mAcctList) ;
	it.current() ; ++it, ++i ) {
    groupName.sprintf("Account %d", i);
    KConfigGroupSaver saver(config, groupName);
    (*it)->writeConfig(*config);
  }
  if (withSync) config->sync();
}


//-----------------------------------------------------------------------------
void KMAcctMgr::readConfig(void)
{
  KConfig* config = kapp->config();
  KMAccount* acct;
  QString acctType, acctName;
  QCString groupName;
  int i, num;

  mAcctList.clear();

  KConfigGroup general(config, "General");
  num = general.readNumEntry("accounts", 0);

  for (i=1; i<=num; i++)
  {
    groupName.sprintf("Account %d", i);
    KConfigGroupSaver saver(config, groupName);
    acctType = config->readEntry("Type");
    // Provide backwards compatibility
    if (acctType == "advanced pop" || acctType == "experimental pop")
      acctType = "pop";
    acctName = config->readEntry("Name");
    if (acctName.isEmpty()) acctName = i18n("Account %1").arg(i);
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

  if (!mAcctChecking.contains(account)) mAcctChecking.append(account);

  if (checking) {
    if (mAcctChecking.count() > 1) moreThanOneAccount = true;
    return;
  }

//   if (account->folder() == 0)
//   {
//     QString tmp; //Unsafe
//     tmp = i18n("Account %1 has no mailbox defined!\n"
//  	        "Mail checking aborted.\n"
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
}

void KMAcctMgr::processNextCheck(bool _newMail)
{
  KMAccount *curAccount = 0; 
  newMailArrived |= _newMail;

  if (lastAccountChecked)
    disconnect( lastAccountChecked, SIGNAL(finishedCheck(bool)),
		this, SLOT(processNextCheck(bool)) );

  if (mAcctChecking.isEmpty() || 
      (((curAccount = mAcctChecking.take(0)) == lastAccountChecked))) {
    kernel->filterMgr()->cleanup();
    kdDebug(5006) << "checked mail, server ready" << endl;
    kernel->serverReady (true);
    checking = false;
    if (mTotalNewMailsArrived > 0 && moreThanOneAccount)
      KMBroadcastStatus::instance()->setStatusMsg(
      i18n("Transmission completed, %n new message.",
           "Transmission completed, %n new messages.", mTotalNewMailsArrived));
    emit checkedMail(newMailArrived, interactive);
    moreThanOneAccount = false;
    mTotalNewMailsArrived = 0;
    return;
  }

  connect( curAccount, SIGNAL(finishedCheck(bool)),
	   this, SLOT(processNextCheck(bool)) );

  lastAccountChecked = curAccount;

  if (curAccount->type() != "imap" && curAccount->folder() == 0)
    {
      QString tmp; //Unsafe
      tmp = i18n("Account %1 has no mailbox defined!\n"
		 "Mail checking aborted.\n"
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
  {
    act->setFolder(kernel->inboxFolder());
    connect( act, SIGNAL(newMailsProcessed(int)),
	this, SLOT(addToTotalNewMailCount(int)) );
  }

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

  if (checking)
    return;

  if (mAcctList.isEmpty())
  {
    KMessageBox::information(0,i18n("You need to add an account in the network "
				    "section of the settings in order to "
				    "receive mail."));
    return;
  }

  mTotalNewMailsArrived=0;

  for ( QPtrListIterator<KMAccount> it(mAcctList) ;
	it.current() ; ++it ) 
  {
    if (!it.current()->checkExclude())
      singleCheckMail(it.current(), _interactive);
  }
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

  if (checking)
    return;

  if (mAcctList.isEmpty())
  {
    KMessageBox::information(0,i18n("You need to add an account in the network "
				    "section of the settings in order to "
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
                     "Mail checking aborted.\n"
                     "Check your account settings!")
		.arg(cur->name());
    KMessageBox::information(0,tmp);
    return;
  }

  singleCheckMail(cur, _interactive);
}


//-----------------------------------------------------------------------------
void KMAcctMgr::addToTotalNewMailCount(int newmails)
{
  if ( newmails==-1 ) mTotalNewMailsArrived=-1;
  if ( mTotalNewMailsArrived==-1 ) return;
  mTotalNewMailsArrived+=newmails;
}
//-----------------------------------------------------------------------------
#include "kmacctmgr.moc"
