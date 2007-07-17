// kmacctlocal.cpp

#include "kmacctlocal.h"
#include "kmfoldermbox.h"
#include "kmacctfolder.h"
#include "broadcaststatus.h"
//Added by qt3to4:
using KPIM::BroadcastStatus;
#include "progressmanager.h"
using KPIM::ProgressManager;

#include "kmfoldermgr.h"

#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kconfiggroup.h>

#include <QFileInfo>
#include <QTime>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

//-----------------------------------------------------------------------------
KMAcctLocal::KMAcctLocal(AccountManager* aOwner, const QString& aAccountName, uint id):
  KMAccount(aOwner, aAccountName, id), mHasNewMail( false ),
  mProcessingNewMail( false ), mAddedOk( true ), mNumMsgs( 0 ),
  mMsgsFetched( 0 ), mMailFolder( 0 )
{
  mLock = procmail_lockfile;
}


//-----------------------------------------------------------------------------
KMAcctLocal::~KMAcctLocal()
{
}


//-----------------------------------------------------------------------------
void KMAcctLocal::init() {
  KMAccount::init();
}


//-----------------------------------------------------------------------------
void KMAcctLocal::pseudoAssign( const KMAccount * a )
{
  KMAccount::pseudoAssign( a );

  const KMAcctLocal * l = dynamic_cast<const KMAcctLocal*>( a );
  if ( !l ) return;

  setLocation( l->location() );
  setLockType( l->lockType() );
  setProcmailLockFileName( l->procmailLockFileName() );
}

//-----------------------------------------------------------------------------
void KMAcctLocal::processNewMail(bool)
{
  if ( mProcessingNewMail )
    return;

  mHasNewMail = false;
  mProcessingNewMail = true;

  if ( !preProcess() ) {
    mProcessingNewMail = false;
    return;
  }

  QTime t;
  t.start();

  for ( mMsgsFetched = 0; mMsgsFetched < mNumMsgs; ++mMsgsFetched )
  {
    if ( !fetchMsg() )
      break;

    if (t.elapsed() >= 200) { //hardwired constant
      qApp->processEvents();
      t.start();
    }
  }

  postProcess();
  mProcessingNewMail = false;
}


//-----------------------------------------------------------------------------
bool KMAcctLocal::preProcess()
{
  if ( precommand().isEmpty() ) {
    QFileInfo fi( location() );
    if ( fi.size() == 0 ) {
      BroadcastStatus::instance()->setStatusMsgTransmissionCompleted( mName, 0 );
      checkDone( mHasNewMail, CheckOK );
      return false;
    }
  }

  mMailFolder = new KMFolder( 0, location(), KMFolderTypeMbox,
                              false /* no index */, false /* don't export sernums */ );
  KMFolderMbox* mboxStorage =
    static_cast<KMFolderMbox*>(mMailFolder->storage());
  mboxStorage->setLockType( mLock );
  if ( mLock == procmail_lockfile)
    mboxStorage->setProcmailLockFileName( mProcmailLockFileName );

  if (!mFolder) {
    checkDone( mHasNewMail, CheckError );
    BroadcastStatus::instance()->setStatusMsg( i18n( "Transmission failed." ));
    return false;
  }

  //BroadcastStatus::instance()->reset();
  BroadcastStatus::instance()->setStatusMsg(
	i18n("Preparing transmission from \"%1\"...", mName));


  Q_ASSERT( !mMailCheckProgressItem );
  mMailCheckProgressItem = KPIM::ProgressManager::createProgressItem(
    "MailCheck" + mName,
    mName,
    i18n("Preparing transmission from \"%1\"...", mName),
    false, // cannot be canceled
    false ); // no tls/ssl

  // run the precommand
  if (!runPrecommand(precommand()))
  {
    kDebug(5006) << "cannot run precommand " << precommand() << endl;
    checkDone( mHasNewMail, CheckError );
    BroadcastStatus::instance()->setStatusMsg( i18n( "Running precommand failed." ));
    return false;
  }

  const int rc = mMailFolder->open( "acctlocalMail" );
  if ( rc != 0 ) {
    QString aStr;
    aStr = i18n("Cannot open file:");
    aStr += mMailFolder->path() + '/' + mMailFolder->name();
    KMessageBox::sorry(0, aStr);
    kDebug(5006) << "cannot open file " << mMailFolder->path() << "/"
      << mMailFolder->name() << endl;
    checkDone( mHasNewMail, CheckError );
    BroadcastStatus::instance()->setStatusMsg( i18n( "Transmission failed." ));
    return false;
  }

  if (!mboxStorage->isLocked()) {
    kDebug(5006) << "mailFolder could not be locked" << endl;
    mMailFolder->close( "acctlocalMail" );
    checkDone( mHasNewMail, CheckError );
    QString errMsg = i18n( "Transmission failed: Could not lock %1." ,
        mMailFolder->location() );
    BroadcastStatus::instance()->setStatusMsg( errMsg );
    return false;
  }

  mFolder->open( "acctlocalFold" );

  mNumMsgs = mMailFolder->count();

  mMailCheckProgressItem->setTotalItems( mNumMsgs );

  //BroadcastStatus::instance()->setStatusProgressEnable( 'L' + mName, true );
  return true;
}


//-----------------------------------------------------------------------------
bool KMAcctLocal::fetchMsg()
{
  KMMessage* msg;

  /* This causes mail eating
  if (kmkernel->mailCheckAborted()) break; */

  const QString statusMsg = i18n( "Moving message %1 of %2 from %3.",
                                  mMsgsFetched, mNumMsgs,
                                  mMailFolder->location() );
  //BroadcastStatus::instance()->setStatusMsg( statusMsg );
  mMailCheckProgressItem->incCompletedItems();
  mMailCheckProgressItem->updateProgress();
  mMailCheckProgressItem->setStatus( statusMsg );

  msg = mMailFolder->take(0);
  if (msg)
  {
#if 0
    // debug code, don't remove
    QFile fileD0( "testdat_xx-0-0" );
    if( fileD0.open( QIODevice::WriteOnly ) ) {
      Q3CString s = msg->asString();
      uint l = s.length();
      if ( l > 0 ) {
        QDataStream ds( &fileD0 );
        ds.writeRawData( s.data(), l );
      }
      fileD0.close();  // If data is 0 we just create a zero length file.
    }
#endif
    msg->setStatus( msg->headerField( "Status" ).toLatin1(),
                    msg->headerField( "X-Status" ).toLatin1());
    if ( !msg->headerField( "X-KMail-EncryptionState" ).isEmpty() )
      msg->setEncryptionStateChar( msg->headerField( "X-KMail-EncryptionState" ).at(0) );
    if ( !msg->headerField( "X-KMail-SignatureState" ).isEmpty() )
      msg->setSignatureStateChar( msg->headerField( "X-KMail-SignatureState" ).at(0));
    msg->setComplete(true);
    msg->updateAttachmentState();

    mAddedOk = processNewMsg(msg);

    if (mAddedOk)
      mHasNewMail = true;

    return mAddedOk;
  }
  return true;
}


//-----------------------------------------------------------------------------
void KMAcctLocal::postProcess()
{
  if (mAddedOk)
  {
    kmkernel->folderMgr()->syncAllFolders();
    const int rc = mMailFolder->expunge();
    if ( rc != 0 ) {
      KMessageBox::queuedMessageBox( 0, KMessageBox::Information,
                                     i18n( "<qt>Cannot remove mail from "
                                           "mailbox <b>%1</b>:<br>%2</qt>" ,
                                       mMailFolder->location() ,
                                       strerror( rc ) ) );
    }

    if( mMailCheckProgressItem ) { // do this only once...
      BroadcastStatus::instance()->setStatusMsgTransmissionCompleted( mName, mNumMsgs );
      mMailCheckProgressItem->setStatus(
        i18np( "Fetched 1 message from mailbox %2.",
              "Fetched %1 messages from mailbox %2.",
              mNumMsgs, mMailFolder->location() ) );
      mMailCheckProgressItem->setComplete();
      mMailCheckProgressItem = 0;
    }
  }
  // else warning is written already

  mMailFolder->close( "acctLocalMail" );
  delete mMailFolder; mMailFolder = 0;

  mFolder->close( "acctlocalFold" );

  checkDone( mHasNewMail, CheckOK );
}


//-----------------------------------------------------------------------------
void KMAcctLocal::readConfig(KConfigGroup& config)
{
  KMAccount::readConfig(config);
  mLocation = config.readPathEntry("Location", mLocation);
  QString locktype = config.readEntry("LockType", "procmail_lockfile" );

  if( locktype == "procmail_lockfile" ) {
    mLock = procmail_lockfile;
    mProcmailLockFileName = config.readEntry("ProcmailLockFile",
      mLocation + ".lock");
  } else if( locktype == "mutt_dotlock" )
    mLock = mutt_dotlock;
  else if( locktype == "mutt_dotlock_privileged" )
    mLock = mutt_dotlock_privileged;
  else if( locktype == "none" )
    mLock = lock_none;
  else mLock = FCNTL;
}


//-----------------------------------------------------------------------------
void KMAcctLocal::writeConfig(KConfigGroup& config)
{
  KMAccount::writeConfig(config);

  config.writePathEntry("Location", mLocation);

  QString st = "fcntl";
  if (mLock == procmail_lockfile) st = "procmail_lockfile";
  else if (mLock == mutt_dotlock) st = "mutt_dotlock";
  else if (mLock == mutt_dotlock_privileged) st = "mutt_dotlock_privileged";
  else if (mLock == lock_none) st = "none";
  config.writeEntry("LockType", st);

  if (mLock == procmail_lockfile) {
    config.writeEntry("ProcmailLockFile", mProcmailLockFileName);
  }

}


//-----------------------------------------------------------------------------
void KMAcctLocal::setLocation(const QString& aLocation)
{
    mLocation = aLocation;
}

void KMAcctLocal::setProcmailLockFileName(const QString& s)
{
    mProcmailLockFileName = s;
}
