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
    kdDebug() << "KMAccount::setFolder() : aFolder == NULL" << endl;
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
    folder = (KMAcctFolder*)kernel->folderMgr()->find(folderName);
    if (folder)
    {
      mFolder = folder;
      mFolder->addAccount(this);
    }
    else kdDebug() << "Cannot find folder `" << (const char*)folderName << "' for account `" << (const char*)mName << "'." << endl;
  }
}


//-----------------------------------------------------------------------------
void KMAccount::writeConfig(KConfig& config)
{
  config.writeEntry("Type", type());
  config.writeEntry("Name", mName);
  config.writeEntry("Folder", mFolder ? mFolder->name() : QString::null);
  config.writeEntry("check-interval", mInterval);
  config.writeEntry("check-exclude", mExclude);
  config.writeEntry("precommand", mPrecommand);
}


//-----------------------------------------------------------------------------
void KMAccount::sendReceipt(KMMessage* aMsg, const QString &aReceiptTo)
{
  KMMessage* newMsg = new KMMessage;
  QString str, receiptTo;

  KConfig* cfg = kapp->config();
  bool sendReceipts;

  cfg->setGroup("General");
  sendReceipts = cfg->readBoolEntry("send-receipts", false);
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
  // Conversion to latin1 is correct here as Mail headers should contain
  // ascii only
  newMsg->setBody(str.latin1());
  newMsg->setAutomaticFields();

  mReceipts.append(newMsg);
  mReceiptTimer.start(0,true);
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
  processResult = kernel->filterMgr()->process(aMsg);
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
	 i18n( QString("Executing precommand ") + precommand ));

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
      //kdDebug() << "KMAccount::runPrecommand: arg " << i << " = " << args[i] << endl;
      precommandProcess << args[i];
    }

  KMAccountPrivate priv;

  connect(&precommandProcess, SIGNAL(processExited(KProcess *)),
          &priv, SLOT(precommandExited(KProcess *)));

  kdDebug() << "Running precommand " << precommand << endl;
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
    kdDebug() << "precommand exited " << p->exitStatus() << endl;
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

