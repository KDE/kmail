// KMail Account

#include <stdlib.h>
#include <unistd.h>

#include <qdir.h>
#include <qstrlist.h>
#include <qtstream.h>
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
}


//-----------------------------------------------------------------------------
void KMAccount::readConfig(KConfig& config)
{
  KMAcctFolder* folder;
  QString folderName;

  mFolder = NULL;
  mName   = config.readEntry("Name", i18n("Unnamed"));
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


//-----------------------------------------------------------------------------
void KMAccount::sendReceipt(KMMessage* aMsg, const QString aReceiptTo) const
{
  KMMessage* newMsg = new KMMessage;
  QString str, receiptTo;

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
void KMAccount::processNewMsg(KMMessage* aMsg)
{
  QString receiptTo;
  int rc;

  assert(aMsg != NULL);

  receiptTo = aMsg->headerField("Return-Receipt-To");
  if (!receiptTo.isEmpty()) sendReceipt(aMsg, receiptTo);

  if (filterMgr->process(aMsg))
  {
    rc = mFolder->addMsg(aMsg);
    if (rc) perror("failed to add message");
    if (rc) warning(i18n("Failed to add message:")+
		    '\n' + QString(strerror(rc)));
  }
}


//-----------------------------------------------------------------------------
void KMAccount::installTimer()
{
  if(!mTimer) {
    mTimer = new QTimer();
    connect(mTimer,SIGNAL(timeout()),SLOT(mailCheck()));
    connect(this,SIGNAL(requestCheck(KMAccount *)),
	      acctMgr,SLOT(singleCheckMail(KMAccount *)));
    printf("Starting new Timer with interval: %d\n",mInterval*1000*60);
    mTimer->start(mInterval*1000*60);
  }
  else {
    mTimer->stop();
    printf("Starting old Timer with interval: %d\n",mInterval*1000*60);
    mTimer->start(mInterval*1000*60);
  }   
}


//-----------------------------------------------------------------------------
void KMAccount::deinstallTimer()
{
  printf("Calling deinstallTimer()\n");
  if(mTimer) {
    mTimer->stop();
    disconnect(mTimer);
    disconnect(this);
    delete mTimer;
    mTimer = 0L;
  }
}


//-----------------------------------------------------------------------------
void KMAccount::mailCheck()
{
  printf("Emitting signal\n");
  emit requestCheck(this);
}


//-----------------------------------------------------------------------------
void KMAccount::stateChanged()
{
  printf("stateChanged called\n");
  if(timerRequested())
    installTimer();
  else
    deinstallTimer();
}
