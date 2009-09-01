// KMail Account

#include "kmaccount.h"

#include "accountmanager.h"
using KMail::AccountManager;
#include "kmacctfolder.h"
#include "kmfoldermgr.h"
#include "kmfiltermgr.h"
#include "messagesender.h"
#include "kmmessage.h"
#include "broadcaststatus.h"
using KPIM::BroadcastStatus;
#include "kmfoldercachedimap.h"

#include "progressmanager.h"
using KPIM::ProgressItem;
using KPIM::ProgressManager;

#include <kpimidentities/identitymanager.h>
#include <kpimidentities/identity.h>

using KMail::FolderJob;

#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kstringhandler.h>

#include <QList>
#include <QEventLoop>
#include <QByteArray>

#include <cstdlib>
#include <unistd.h>
#include <cerrno>

#include <cassert>

//----------------------
#include "kmaccount.moc"

//-----------------------------------------------------------------------------
KMPrecommand::KMPrecommand(const QString &precommand, QObject *parent)
  : QObject( parent ), mPrecommand( precommand )
{
  BroadcastStatus::instance()->setStatusMsg(
      i18n("Executing precommand %1", precommand ));

  mPrecommandProcess.setShellCommand(precommand);

  connect(&mPrecommandProcess, SIGNAL(finished(int, QProcess::ExitStatus)),
          SLOT(precommandExited(int, QProcess::ExitStatus)));
}

//-----------------------------------------------------------------------------
KMPrecommand::~KMPrecommand()
{
}


//-----------------------------------------------------------------------------
bool KMPrecommand::start()
{
  mPrecommandProcess.start();
  const bool ok = mPrecommandProcess.waitForStarted();
  if ( !ok )
    KMessageBox::error( 0, i18n("Could not execute precommand '%1'.",
                        mPrecommand) );
  return ok;
}


//-----------------------------------------------------------------------------
void KMPrecommand::precommandExited(int exitCode, QProcess::ExitStatus)
{
  if (exitCode != 0)
    KMessageBox::error(0, i18n("The precommand exited with code %1:\n%2",
       exitCode, strerror(exitCode)));
  emit finished(exitCode == 0);
}


//-----------------------------------------------------------------------------
KMAccount::KMAccount(AccountManager* aOwner, const QString& aName, uint id)
  : KAccount( id, aName ),
    mTrash(KMKernel::self()->trashFolder()->idString()),
    mOwner(aOwner),
    mFolder(0),
    mTimer(0),
    mInterval(0),
    mExclude(false),
    mCheckingMail(false),
    mPrecommandSuccess(true),
    mUseDefaultIdentity(true),
    mHasInbox(false),
    mMailCheckProgressItem(0),
    mPrecommandEventLoop( 0 )
{
  assert(aOwner != 0);
  mIdentityId = kmkernel->identityManager()->defaultIdentity().uoid();

  connect( kmkernel->identityManager(), SIGNAL( changed() ),
           this, SLOT( slotIdentitiesChanged() ) );
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
  if ( (kmkernel && !kmkernel->shuttingDown()) && mFolder ) mFolder->removeAccount(this);
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
    //kDebug() << "aFolder == 0";
    mFolder = 0;
    return;
  }
  mFolder = (KMAcctFolder*)aFolder;
  if (addAccount) mFolder->addAccount(this);
}


//-----------------------------------------------------------------------------
void KMAccount::readConfig(KConfigGroup& config)
{
  QString folderName;
  mFolder = 0;
  folderName = config.readEntry("Folder");
  setCheckInterval(config.readEntry("check-interval", 0 ) );
  setTrash(config.readEntry("trash", kmkernel->trashFolder()->idString()));
  setCheckExclude(config.readEntry("check-exclude", false ) );
  setPrecommand(config.readPathEntry("precommand", QString()));

  uint defaultIdentity = kmkernel->identityManager()->defaultIdentity().uoid();
  setIdentityId( config.readEntry( "identity-id", defaultIdentity ) );
  mUseDefaultIdentity = config.readEntry( "use-default-identity", true );
  slotIdentitiesChanged();

  if ( !folderName.isEmpty() ) {
    setFolder (kmkernel->folderMgr()->findIdString( folderName ), true );
  }

  if (mInterval == 0)
    deinstallTimer();
  else
    installTimer();
}

void KMAccount::readTimerConfig()
{
  // Re-reads and checks check-interval value and deinstalls timer incase check-interval
  // for mail check is disabled.
  // Or else, the mail sync goes into a infinite loop (kolab/issue2607)
  if (mInterval == 0)
    deinstallTimer();
  else
    installTimer();
}

//-----------------------------------------------------------------------------
void KMAccount::writeConfig(KConfigGroup& config)
{
  // ID, Name
  KAccount::writeConfig(config);

  config.writeEntry("Type", KAccount::nameForType( type() ) );
  config.writeEntry("Folder", mFolder ? mFolder->idString() : QString());
  config.writeEntry("check-interval", mInterval);
  config.writeEntry("check-exclude", mExclude);
  config.writePathEntry("precommand", mPrecommand);
  config.writeEntry("trash", mTrash);
  config.writeEntry( "use-default-identity", mUseDefaultIdentity );
  config.writeEntry( "identity-id", mIdentityId );
}


//-----------------------------------------------------------------------------
void KMAccount::sendReceipt(KMMessage* aMsg)
{
  bool sendReceipts;

  KConfigGroup cfg( KMKernel::config(), "General" );

  sendReceipts = cfg.readEntry("send-receipts", false );
  if (!sendReceipts) return;

  KMMessage *newMsg = aMsg->createDeliveryReceipt();
  if (newMsg) {
    mReceipts.append(newMsg);
    QTimer::singleShot( 0, this, SLOT( sendReceipts() ) );
  }
}


//-----------------------------------------------------------------------------
bool KMAccount::processNewMsg(KMMessage* aMsg)
{
  int rc, processResult;

  assert(aMsg != 0);

  // Save this one for readding
  KMFolderCachedImap* parent = 0;
  if( type() == KAccount::DImap )
    parent = static_cast<KMFolderCachedImap*>( aMsg->storage() );

  // checks whether we should send delivery receipts
  // and sends them.
  sendReceipt(aMsg);

  // Set status of new messages that are marked as old to read, otherwise
  // the user won't see which messages newly arrived.
  // This is only valid for pop accounts and produces wrong stati for imap.
  if ( type() != KAccount::DImap && type() != KAccount::Imap ) {
    if ( aMsg->status().isOld() )
      aMsg->setStatus( MessageStatus::statusUnread() );  // -sanders
    //    aMsg->setStatus( MessageStatus::statusRead() );
    else
      aMsg->setStatus( MessageStatus::statusNew() );
  }
/*
QFile fileD0( "testdat_xx-kmaccount-0" );
if( fileD0.open( QIODevice::WriteOnly ) ) {
    QDataStream ds( &fileD0 );
    ds.writeRawData( aMsg->asString(), aMsg->asString().length() );
    fileD0.close();  // If data is 0 we just create a zero length file.
}
*/
  // 0==message moved; 1==processing ok, no move; 2==critical error, abort!

  processResult = kmkernel->filterMgr()->process(aMsg,KMFilterMgr::Inbound,true,id());
  if (processResult == 2) {
    kError() << "Critical error: Unable to collect mail (out of space?)";
    KMessageBox::information(0,(i18n("Critical error: "
      "Unable to collect mail: ")) + QString::fromLocal8Bit(strerror(errno)));
    return false;
  }
  else if (processResult == 1)
  {
    if( type() == KAccount::DImap )
      ; // already done by caller: parent->addMsgInternal( aMsg, false );
    else {
      // TODO: Perhaps it would be best, if this if was handled by a virtual
      // method, so the if( !dimap ) above could die?
      kmkernel->filterMgr()->tempOpenFolder(mFolder);
      rc = mFolder->addMsg(aMsg);
/*
QFile fileD0( "testdat_xx-kmaccount-1" );
if( fileD0.open( QIODevice::WriteOnly ) ) {
    QDataStream ds( &fileD0 );
    ds.writeRawData( aMsg->asString(), aMsg->asString().length() );
    fileD0.close();  // If data is 0 we just create a zero length file.
}
*/
      if (rc) {
        kError() << "Failed to add message!";
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
    folderId = ( type() == KAccount::DImap ) ? parent->folder()->idString()
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
    mInterval = 0;
  else
    mInterval = aInterval;
  // Don't call installTimer from here! See #117935.
}

int KMAccount::checkInterval() const
{
  if ( mInterval <= 0 )
    return mInterval;
  return qMax( mInterval, GlobalSettings::self()->minimumCheckInterval() );
}

//----------------------------------------------------------------------------
void KMAccount::deleteFolderJobs()
{
  qDeleteAll( mJobList );
  mJobList.clear();
}

//----------------------------------------------------------------------------
void KMAccount::ignoreJobsForMessage( KMMessage* msg )
{
  //FIXME: remove, make folders handle those
  QList<FolderJob*>::iterator it;
  for( it = mJobList.begin(); it != mJobList.end(); ++it ) {
    if ( (*it)->msgList().first() == msg) {
      FolderJob *job = (*it);
      it = mJobList.erase( it );
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
  mTimer->start( checkInterval() * 60000 );
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

  // Don't do anything if there is a running pre-command
  if ( mPrecommandEventLoop != 0 )
    return true;

  KMPrecommand precommandProcess(precommand, this);

  BroadcastStatus::instance()->setStatusMsg(
      i18n("Executing precommand %1", precommand ));

  connect(&precommandProcess, SIGNAL(finished(bool)),
          SLOT(precommandExited(bool)));

  kDebug() << "Running precommand" << precommand;
  if ( !precommandProcess.start() )
    return false;

  // Start an event loop. This makes sure GUI events are still processed while
  // the precommand is running (which may take a while).
  // The exec call will block until the event loop is exited, which happens in
  // precommandExited().
  mPrecommandEventLoop = new QEventLoop();
  mPrecommandEventLoop->exec();
  delete mPrecommandEventLoop;
  mPrecommandEventLoop = 0;

  return mPrecommandSuccess;
}

//-----------------------------------------------------------------------------
void KMAccount::precommandExited(bool success)
{
  Q_ASSERT( mPrecommandEventLoop != 0 );
  mPrecommandSuccess = success;
  mPrecommandEventLoop->exit();
}

//-----------------------------------------------------------------------------
void KMAccount::slotIdentitiesChanged()
{
  // Fall back to the default identity if the one used currently is invalid
  if ( kmkernel->identityManager()->identityForUoid( mIdentityId ).isNull() ) {
    mIdentityId = kmkernel->identityManager()->defaultIdentity().uoid();
  }
}

//-----------------------------------------------------------------------------
void KMAccount::mailCheck()
{
  if (mTimer)
    mTimer->stop();

  if ( kmkernel ) {
    AccountManager *acctmgr = kmkernel->acctMgr();
    if ( acctmgr ) {
      acctmgr->singleCheckMail( this, false );
    }
  }
}

//-----------------------------------------------------------------------------
void KMAccount::sendReceipts()
{
  QList<KMMessage*>::Iterator it;
  for(it = mReceipts.begin(); it != mReceipts.end(); ++it)
    kmkernel->msgSender()->send(*it); //might process events
  mReceipts.clear();
}

//-----------------------------------------------------------------------------
QString KMAccount::importPassword(const QString &aStr)
{
  unsigned int i, val;
  unsigned int len = aStr.length();
  QByteArray result;
  result.resize(len);

  for (i=0; i<len; i++)
  {
    val = aStr[i].toLatin1() - ' ';
    val = (255-' ') - val;
    result[i] = (char)(val + ' ');
  }
  result[i] = '\0';

  return KStringHandler::obscure(result);
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
  setIdentityId( a->identityId() );
  setUseDefaultIdentity( a->useDefaultIdentity() );
}

//-----------------------------------------------------------------------------
void KMAccount::checkDone( bool newmail, CheckStatus status )
{
    setCheckingMail( false );
  // Reset the timeout for automatic mailchecking. The user might have
  // triggered the check manually.
  if (mTimer)
    mTimer->start( checkInterval() * 60000 );
  if ( mMailCheckProgressItem ) {
    mMailCheckProgressItem->setComplete(); // that will delete it
    mMailCheckProgressItem = 0;
  }

  emit newMailsProcessed( mNewInFolder );
  emit finishedCheck( newmail, status );
  mNewInFolder.clear();
}

uint KMAccount::identityId() const
{
  if ( mUseDefaultIdentity )
    return kmkernel->identityManager()->defaultIdentity().uoid();
  else
    return mIdentityId;
}

//-----------------------------------------------------------------------------
void KMAccount::addToNewInFolder( const QString &folderId, int num )
{
  if ( mNewInFolder.find( folderId ) == mNewInFolder.end() )
    mNewInFolder[folderId] = num;
  else
    mNewInFolder[folderId] += num;
}
