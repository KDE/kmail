// KMail Account
#include <config.h>

#include "kmaccount.h"

#include "kmacctmgr.h"
#include "kmacctfolder.h"
#include "kmfoldermgr.h"
#include "kmfiltermgr.h"
#include "kmsender.h"
#include "kmkernel.h"
#include "broadcaststatus.h"
using KPIM::BroadcastStatus;
#include "kmfoldercachedimap.h"

#include "progressmanager.h"
using KPIM::ProgressItem;
using KPIM::ProgressManager;

using KMail::FolderJob;

#include <kapplication.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kconfig.h>

#include <qeventloop.h>

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <assert.h>

//----------------------
#include "kmaccount.moc"

//-----------------------------------------------------------------------------
KMPrecommand::KMPrecommand(const QString &precommand, QObject *parent)
  : QObject(parent), mPrecommand(precommand)
{
  BroadcastStatus::instance()->setStatusMsg(
      i18n("Executing precommand %1").arg(precommand ));

  mPrecommandProcess.setUseShell(true);
  mPrecommandProcess << precommand;

  connect(&mPrecommandProcess, SIGNAL(processExited(KProcess *)),
          SLOT(precommandExited(KProcess *)));
}

//-----------------------------------------------------------------------------
KMPrecommand::~KMPrecommand()
{
}


//-----------------------------------------------------------------------------
bool KMPrecommand::start()
{
  bool ok = mPrecommandProcess.start( KProcess::NotifyOnExit );
  if (!ok) KMessageBox::error(0, i18n("Could not execute precommand '%1'.")
    .arg(mPrecommand));
  return ok;
}


//-----------------------------------------------------------------------------
void KMPrecommand::precommandExited(KProcess *p)
{
  int exitCode = p->normalExit() ? p->exitStatus() : -1;
  if (exitCode)
    KMessageBox::error(0, i18n("The precommand exited with code %1:\n%2")
      .arg(exitCode).arg(strerror(exitCode)));
  emit finished(!exitCode);
}


//-----------------------------------------------------------------------------
KMAccount::KMAccount(KMAcctMgr* aOwner, const QString& aName, uint id)
  : KAccount( id, aName ),
    mTrash(KMKernel::self()->trashFolder()->idString()),
    mOwner(aOwner),
    mFolder(0),
    mTimer(0),
    mInterval(0),
    mExclude(false),
    mCheckingMail(false),
    mPrecommandSuccess(true),
    mHasInbox(false),
    mMailCheckProgressItem(0)
{
  assert(aOwner != 0);

  connect(&mReceiptTimer,SIGNAL(timeout()),SLOT(sendReceipts()));
}

void KMAccount::init() {
  mTrash = kmkernel->trashFolder()->idString();
  mExclude = false;
  mInterval = 0;
  mNewInFolder.clear();
}

//-----------------------------------------------------------------------------
KMAccount::~KMAccount()
{
  if (!kmkernel->shuttingDown() && mFolder) mFolder->removeAccount(this);
  if (mTimer) deinstallTimer();
}


//-----------------------------------------------------------------------------
void KMAccount::setName(const QString& aName)
{
  mName = aName;
}


//-----------------------------------------------------------------------------
void KMAccount::clearPasswd()
{
}


//-----------------------------------------------------------------------------
void KMAccount::setFolder(KMFolder* aFolder, bool addAccount)
{
  if(!aFolder) {
    //kdDebug(5006) << "KMAccount::setFolder() : aFolder == 0" << endl;
    mFolder = 0;
    return;
  }
  mFolder = (KMAcctFolder*)aFolder;
  if (addAccount) mFolder->addAccount(this);
}


//-----------------------------------------------------------------------------
void KMAccount::readConfig(KConfig& config)
{
  QString folderName;
  mFolder = 0;
  folderName = config.readEntry("Folder");
  setCheckInterval(config.readNumEntry("check-interval", 0));
  setTrash(config.readEntry("trash", kmkernel->trashFolder()->idString()));
  setCheckExclude(config.readBoolEntry("check-exclude", false));
  setPrecommand(config.readPathEntry("precommand"));

  if (!folderName.isEmpty())
  {
    setFolder(kmkernel->folderMgr()->findIdString(folderName), true);
  }
}


//-----------------------------------------------------------------------------
void KMAccount::writeConfig(KConfig& config)
{
  // ID, Name
  KAccount::writeConfig(config);

  config.writeEntry("Type", type());
  config.writeEntry("Folder", mFolder ? mFolder->idString() : QString::null);
  config.writeEntry("check-interval", mInterval);
  config.writeEntry("check-exclude", mExclude);
  config.writePathEntry("precommand", mPrecommand);
  config.writeEntry("trash", mTrash);
}


//-----------------------------------------------------------------------------
void KMAccount::sendReceipt(KMMessage* aMsg)
{
  KConfig* cfg = KMKernel::config();
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

  assert(aMsg != 0);

  // Save this one for readding
  KMFolderCachedImap* parent = 0;
  if( type() == "cachedimap" )
    parent = static_cast<KMFolderCachedImap*>( aMsg->storage() );

  // checks whether we should send delivery receipts
  // and sends them.
  sendReceipt(aMsg);

  // Set status of new messages that are marked as old to read, otherwise
  // the user won't see which messages newly arrived.
  // This is only valid for pop accounts and produces wrong stati for imap.
  if ( type() != "cachedimap" && type() != "imap" ) {
    if ( aMsg->isOld() )
      aMsg->setStatus(KMMsgStatusUnread);  // -sanders
    //    aMsg->setStatus(KMMsgStatusRead);
    else
      aMsg->setStatus(KMMsgStatusNew);
  }
/*
QFile fileD0( "testdat_xx-kmaccount-0" );
if( fileD0.open( IO_WriteOnly ) ) {
    QDataStream ds( &fileD0 );
    ds.writeRawBytes( aMsg->asString(), aMsg->asString().length() );
    fileD0.close();  // If data is 0 we just create a zero length file.
}
*/
  // 0==message moved; 1==processing ok, no move; 2==critical error, abort!

  processResult = kmkernel->filterMgr()->process(aMsg,KMFilterMgr::Inbound);
  if (processResult == 2) {
    perror("Critical error: Unable to collect mail (out of space?)");
    KMessageBox::information(0,(i18n("Critical error: "
      "Unable to collect mail: ")) + QString::fromLocal8Bit(strerror(errno)));
    return false;
  }
  else if (processResult == 1)
  {
    if( type() == "cachedimap" )
      ; // already done by caller: parent->addMsgInternal( aMsg, false );
    else {
      // TODO: Perhaps it would be best, if this if was handled by a virtual
      // method, so the if( !dimap ) above could die?
      kmkernel->filterMgr()->tempOpenFolder(mFolder);
      rc = mFolder->addMsg(aMsg);
/*
QFile fileD0( "testdat_xx-kmaccount-1" );
if( fileD0.open( IO_WriteOnly ) ) {
    QDataStream ds( &fileD0 );
    ds.writeRawBytes( aMsg->asString(), aMsg->asString().length() );
    fileD0.close();  // If data is 0 we just create a zero length file.
}
*/
      if (rc) {
        perror("failed to add message");
        KMessageBox::information(0, i18n("Failed to add message:\n") +
                                 QString(strerror(rc)));
        return false;
      }
      int count = mFolder->count();
      // If count == 1, the message is immediately displayed
      if (count != 1) mFolder->unGetMsg(count - 1);
    }
  }

  // Count number of new messages for each folder
  QString folderId;
  if ( processResult == 1 ) {
    folderId = ( type() == "cachedimap" ) ? parent->folder()->idString()
                                          : mFolder->idString();
  }
  else {
    folderId = aMsg->parent()->idString();
  }
  addToNewInFolder( folderId, 1 );

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

//----------------------------------------------------------------------------
void KMAccount::deleteFolderJobs()
{
  mJobList.setAutoDelete(true);
  mJobList.clear();
  mJobList.setAutoDelete(false);
}

//----------------------------------------------------------------------------
void KMAccount::ignoreJobsForMessage( KMMessage* msg )
{
  //FIXME: remove, make folders handle those
  for( QPtrListIterator<FolderJob> it(mJobList); it.current(); ++it ) {
    if ( it.current()->msgList().first() == msg) {
      FolderJob *job = it.current();
      mJobList.remove( job );
      delete job;
      break;
    }
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
  delete mTimer;
  mTimer = 0;
}

//-----------------------------------------------------------------------------
bool KMAccount::runPrecommand(const QString &precommand)
{
  // Run the pre command if there is one
  if ( precommand.isEmpty() )
    return true;

  KMPrecommand precommandProcess(precommand, this);

  BroadcastStatus::instance()->setStatusMsg(
      i18n("Executing precommand %1").arg(precommand ));

  connect(&precommandProcess, SIGNAL(finished(bool)),
          SLOT(precommandExited(bool)));

  kdDebug(5006) << "Running precommand " << precommand << endl;
  if (!precommandProcess.start()) return false;

  kapp->eventLoop()->enterLoop();

  return mPrecommandSuccess;
}

//-----------------------------------------------------------------------------
void KMAccount::precommandExited(bool success)
{
  mPrecommandSuccess = success;
  kapp->eventLoop()->exitLoop();
}

//-----------------------------------------------------------------------------
void KMAccount::mailCheck()
{
  if (mTimer)
    mTimer->stop();
  kmkernel->acctMgr()->singleCheckMail(this, false);
}

//-----------------------------------------------------------------------------
void KMAccount::sendReceipts()
{
  QValueList<KMMessage*>::Iterator it;
  for(it = mReceipts.begin(); it != mReceipts.end(); ++it)
    kmkernel->msgSender()->send(*it); //might process events
  mReceipts.clear();
}

//-----------------------------------------------------------------------------
QString KMAccount::encryptStr(const QString &aStr)
{
  QString result;
  for (uint i = 0; i < aStr.length(); i++)
    result += (aStr[i].unicode() < 0x20) ? aStr[i] :
      QChar(0x1001F - aStr[i].unicode());
  return result;
}

//-----------------------------------------------------------------------------
QString KMAccount::importPassword(const QString &aStr)
{
  unsigned int i, val;
  unsigned int len = aStr.length();
  QCString result;
  result.resize(len+1);

  for (i=0; i<len; i++)
  {
    val = aStr[i] - ' ';
    val = (255-' ') - val;
    result[i] = (char)(val + ' ');
  }
  result[i] = '\0';

  return encryptStr(result);
}

void KMAccount::invalidateIMAPFolders()
{
  // Default: Don't do anything. The IMAP account will handle it
}

void KMAccount::pseudoAssign( const KMAccount * a ) {
  if ( !a ) return;

  setName( a->name() );
  setId( a->id() );
  setCheckInterval( a->checkInterval() );
  setCheckExclude( a->checkExclude() );
  setFolder( a->folder() );
  setPrecommand( a->precommand() );
  setTrash( a->trash() );
}

//-----------------------------------------------------------------------------
void KMAccount::checkDone( bool newmail, CheckStatus status )
{
  mCheckingMail = false;
  // Reset the timeout for automatic mailchecking. The user might have
  // triggered the check manually.
  if (mTimer)
    mTimer->start(mInterval*60000);
  if ( mMailCheckProgressItem ) {
    mMailCheckProgressItem->setComplete(); // that will delete it
    mMailCheckProgressItem = 0;
  }

  emit newMailsProcessed( mNewInFolder );
  emit finishedCheck( newmail, status );
  mNewInFolder.clear();
}

//-----------------------------------------------------------------------------
void KMAccount::addToNewInFolder( QString folderId, int num )
{
  if ( mNewInFolder.find( folderId ) == mNewInFolder.end() )
    mNewInFolder[folderId] = num;
  else
    mNewInFolder[folderId] += num;
}
