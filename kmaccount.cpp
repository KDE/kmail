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

#include <klocale.h>
#include <kprocess.h>
#include <kmessagebox.h>
#include <kdebug.h>

#include "kmacctmgr.h"
#include "kmacctfolder.h"
#include "kmaccount.h"
#include "kmglobal.h"
#include "kmfoldermgr.h"
#include "kmfiltermgr.h"
#include "kmsender.h"
#include "kmmessage.h"
#include "kmbroadcaststatus.h"

//----------------------
#include "kmaccount.moc"

//-----------------------------------------------------------------------------
KMAccount::KMAccount(KMAcctMgr* aOwner, const QString& aName)
{
  assert(aOwner != NULL);

  mOwner  = aOwner;
  mName   = aName;
  mFolder = NULL;
  mTimer  = NULL;
  mInterval = 0;
  mExclude = false;
  mCheckingMail = FALSE;
  connect(&mReceiptTimer,SIGNAL(timeout()),SLOT(sendReceipts()));
}


//-----------------------------------------------------------------------------
KMAccount::~KMAccount()
{
  if (!kernel->shuttingDown() && mFolder) mFolder->removeAccount(this);
  if (mTimer) deinstallTimer();
}


//-----------------------------------------------------------------------------
void KMAccount::setName(const QString& aName)
{
  mName = (aName.isEmpty()) ? i18n("Unnamed") : aName;
}


//-----------------------------------------------------------------------------
void KMAccount::clearPasswd()
{
}


//-----------------------------------------------------------------------------
void KMAccount::setFolder(KMFolder* aFolder)
{
  if(!aFolder)
    {
    kdDebug(5006) << "KMAccount::setFolder() : aFolder == NULL" << endl;
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
  setPrecommand(config.readEntry("precommand"));

  if (!folderName.isEmpty())
  {
    folder = (KMAcctFolder*)kernel->folderMgr()->findIdString(folderName);
    if (folder)
    {
      mFolder = folder;
      mFolder->addAccount(this);
    }
    else kdDebug(5006) << "Cannot find folder `" << folderName << "' for account `" << mName << "'." << endl;
  }
}


//-----------------------------------------------------------------------------
void KMAccount::writeConfig(KConfig& config)
{
  config.writeEntry("Type", type());
  config.writeEntry("Name", mName);
  config.writeEntry("Folder", mFolder ? mFolder->idString() : QString::null);
  config.writeEntry("check-interval", mInterval);
  config.writeEntry("check-exclude", mExclude);
  config.writeEntry("precommand", mPrecommand);
}


//-----------------------------------------------------------------------------
void KMAccount::sendReceipt(KMMessage* aMsg)
{
  KConfig* cfg = kapp->config();
  bool sendReceipts;

  KConfigGroupSaver saver(cfg, "General");

  sendReceipts = cfg->readBoolEntry("send-receipts", false);
  if (!sendReceipts) return;

  KMMessage *newMsg = aMsg->createDeliveryReceipt();
  if (newMsg) {
    mReceipts.append(newMsg);
    mReceiptTimer.start(0,true);
  }
}


//-----------------------------------------------------------------------------
bool KMAccount::processNewMsg(KMMessage* aMsg)
{
  int rc, processResult;

  assert(aMsg != NULL);

  // checks whether we should send delivery receipts
  // and sends them.
  sendReceipt(aMsg);

  // Set status of new messages that are marked as old to read, otherwise
  // the user won't see which messages newly arrived.
  if (aMsg->status()==KMMsgStatusOld)
    aMsg->setStatus(KMMsgStatusUnread);  // -sanders
  //    aMsg->setStatus(KMMsgStatusRead);
  else aMsg->setStatus(KMMsgStatusNew);

  // 0==processed ok, 1==processing failed, 2==critical error, abort!
  processResult = kernel->filterMgr()->process(aMsg,KMFilterMgr::Inbound);
  if (processResult == 2) {
    perror("Critical error: Unable to collect mail (out of space?)");
    KMessageBox::information(0,(i18n("Critical error: "
      "Unable to collect mail (out of space?)")));
    return false;
  }
  else if (processResult == 1)
  {
    kernel->filterMgr()->tempOpenFolder(mFolder);
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

bool KMAccount::runPrecommand(const QString &precommand)
{
  KProcess precommandProcess;

  // Run the pre command if there is one
  if ( precommand.isEmpty() )
    return true;

  KMBroadcastStatus::instance()->setStatusMsg(
      i18n("Executing precommand %1").arg(precommand ));

  QStringList args;
  // Tokenize on space
  int left = 0;
  QString parseString = precommand;
  left = parseString.find(' ', 0, false);
  while ((left <= (int)parseString.length()) && (left != -1))
  {
    args << parseString.left(left);
    parseString = parseString.right(parseString.length() - (left+1));
    left = parseString.find(' ', 0, false);
  }
  args << parseString;

  for (unsigned int i = 0; i < args.count(); i++)
    {
      //kdDebug(5006) << "KMAccount::runPrecommand: arg " << i << " = " << args[i] << endl;
      precommandProcess << args[i];
    }

  KMAccountPrivate priv;

  connect(&precommandProcess, SIGNAL(processExited(KProcess *)),
          &priv, SLOT(precommandExited(KProcess *)));

  kdDebug(5006) << "Running precommand " << precommand << endl;
  precommandProcess.start( KProcess::NotifyOnExit );

  kapp->enter_loop();

  if (!precommandProcess.normalExit() || precommandProcess.exitStatus() != 0)
      return false;

  return true;
}

KMAccountPrivate::KMAccountPrivate( QObject *p ) :
    QObject( p ) {}

void  KMAccountPrivate::precommandExited(KProcess *p)
{
    kdDebug(5006) << "precommand exited " << p->exitStatus() << endl;
    kapp->exit_loop();
}

//-----------------------------------------------------------------------------
void KMAccount::mailCheck()
{
 if (mCheckingMail) return;
 mCheckingMail = TRUE;
 kernel->acctMgr()->singleCheckMail(this,false);
 mCheckingMail = FALSE;
}

//-----------------------------------------------------------------------------
void KMAccount::sendReceipts()
{
  // re-entrant
  QValueList<KMMessage*> receipts;
  QValueList<KMMessage*>::Iterator it;
  for(it = mReceipts.begin(); it != mReceipts.end(); ++it)
    receipts.append(*it);
  mReceipts.clear();

  for(it = receipts.begin(); it != receipts.end(); ++it)
    kernel->msgSender()->send(*it);  //might process events
}

