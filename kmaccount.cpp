// KMail Account

#include <stdlib.h>
#include <unistd.h>

#include <qdir.h>
#include <qstrlist.h>
#include <qtextstream.h>
#include <qfile.h>
#include <assert.h>
#include <kconfig.h>
#include <kapp.h>
#include <qregexp.h>

#include "kmacctmgr.h"
#include "kmacctfolder.h"
#include "kmaccount.h"
#include "kmglobal.h"
#include "kmfoldermgr.h"
#include "kmfiltermgr.h"
#include "kmsender.h"
#include "kmmessage.h"
#include <klocale.h>
#include <kmessagebox.h>

//----------------------
#include "kmaccount.moc"

//-----------------------------------------------------------------------------
KMAccount::KMAccount(KMAcctMgr* aOwner, const char* aName)
{
  initMetaObject();
  assert(aOwner != NULL);

  mOwner  = aOwner;
  mName   = aName;
  mFolder = NULL;
  mTimer  = NULL;
  mInterval = 0;
  mExclude = false;
  mCheckingMail = FALSE;
}


//-----------------------------------------------------------------------------
KMAccount::~KMAccount() 
{
  if (!shuttingDown && mFolder) mFolder->removeAccount(this);
  if (mTimer) deinstallTimer();
}


//-----------------------------------------------------------------------------
void KMAccount::setName(const QString& aName)
{
  mName = aName;
}


//-----------------------------------------------------------------------------
void KMAccount::setFolder(KMFolder* aFolder)
{
  if(!aFolder) 
    {
    debug("KMAccount::setFolder() : aFolder == NULL");
    mFolder = NULL;
    return;
    }
  mFolder = (KMAcctFolder*)aFolder;
}


//-----------------------------------------------------------------------------
void KMAccount::readConfig(KConfig& config)
{
  KMAcctFolder* folder;
  QString folderName;

  mFolder = NULL;
  mName   = config.readEntry("Name", i18n("Unnamed"));
  folderName = config.readEntry("Folder", "");
  setCheckInterval(config.readNumEntry("check-interval", 0));
  setCheckExclude(config.readBoolEntry("check-exclude", false));

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
  config.writeEntry("check-interval", mInterval);
  config.writeEntry("check-exclude", mExclude);
}


//-----------------------------------------------------------------------------
void KMAccount::sendReceipt(KMMessage* aMsg, const QString aReceiptTo) const
{
  KMMessage* newMsg = new KMMessage;
  QString str, receiptTo;

  KConfig* cfg = app->config();
  bool sendReceipts;

  cfg->setGroup("General");
  sendReceipts = cfg->readBoolEntry("send-receipts", true);
  if (!sendReceipts) return;

  receiptTo = aReceiptTo;
  receiptTo.replace(QRegExp("\\n"),"");

  newMsg->initHeader();
  newMsg->setTo(receiptTo);
  newMsg->setSubject(i18n("Receipt: ") + aMsg->subject());

  str  = "Your message was successfully delivered.";
  str += "\n\n---------- Message header follows ----------\n";
  str += aMsg->headerAsString();
  str += "--------------------------------------------\n";
  newMsg->setBody(str);
  newMsg->setAutomaticFields();

  msgSender->send(newMsg);
}


//-----------------------------------------------------------------------------
bool KMAccount::processNewMsg(KMMessage* aMsg)
{
  QString receiptTo;
  int rc, processResult;

  assert(aMsg != NULL);

  receiptTo = aMsg->headerField("Return-Receipt-To");
  if (!receiptTo.isEmpty()) sendReceipt(aMsg, receiptTo);

  // Set status of new messages that are marked as old to read, otherwise
  // the user won't see which messages newly arrived.
  if (aMsg->status()==KMMsgStatusOld)
    aMsg->setStatus(KMMsgStatusUnread);  // -sanders
  //    aMsg->setStatus(KMMsgStatusRead);
  else aMsg->setStatus(KMMsgStatusNew);

  // 0==processed ok, 1==processing failed, 2==critical error, abort!
  processResult = filterMgr->process(aMsg);
  if (processResult == 2) {
    perror("Critical error: Unable to collect mail (out of space?)");
    KMessageBox::information(0,(i18n("Critical error: "
      "Unable to collect mail (out of space?)")));
    return false;
  }
  else if (processResult == 1)
  {
    rc = mFolder->addMsg(aMsg);
    if (rc) {
      perror("failed to add message");
      KMessageBox::information(0, i18n("Failed to add message:\n") +
			       QString(strerror(rc)));
      return false;
    }
    else return true;
  }
  // What now -  are we owner or not?
  return true; //Everything's fine - message has been added by filter  }
}


//-----------------------------------------------------------------------------
void KMAccount::setCheckInterval(int aInterval)
{
  if (aInterval <= 0)
  {
    mInterval = 0;
    deinstallTimer();
  }
  else
  {
    mInterval = aInterval;
    installTimer();
  }
}

//-----------------------------------------------------------------------------
void KMAccount::setCheckExclude(bool aExclude)
{
  mExclude = aExclude;
}


//-----------------------------------------------------------------------------
void KMAccount::installTimer()
{
  if (mInterval <= 0) return;
  if(!mTimer)
  {
    mTimer = new QTimer();
    connect(mTimer,SIGNAL(timeout()),SLOT(mailCheck()));
  }
  else
  {
    mTimer->stop();
  }   
  mTimer->start(mInterval*60000);
}


//-----------------------------------------------------------------------------
void KMAccount::deinstallTimer()
{
  if(mTimer) {
    mTimer->stop();
    disconnect(mTimer);
    delete mTimer;
    mTimer = NULL;
  }
}


//-----------------------------------------------------------------------------
void KMAccount::mailCheck()
{
 if (mCheckingMail) return;
 mCheckingMail = TRUE;
 acctMgr->singleCheckMail(this,false);
 mCheckingMail = FALSE;
}
