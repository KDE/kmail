// kmacctlocal.cpp

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmacctlocal.h"
#include "kmfoldermbox.h"
#include "kmacctfolder.h"
#include "broadcaststatus.h"
using KPIM::BroadcastStatus;
#include "progressmanager.h"
using KPIM::ProgressManager;

#include "kmfoldermgr.h"

#include <kapplication.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kconfig.h>

#include <qfileinfo.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

//-----------------------------------------------------------------------------
KMAcctLocal::KMAcctLocal(KMAcctMgr* aOwner, const QString& aAccountName, uint id):
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
QString KMAcctLocal::type(void) const
{
  return "local";
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
      kapp->processEvents();
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

  mMailFolder = new KMFolder( 0, location(), KMFolderTypeMbox );
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
	i18n("Preparing transmission from \"%1\"...").arg(mName));


  Q_ASSERT( !mMailCheckProgressItem );
  mMailCheckProgressItem = KPIM::ProgressManager::createProgressItem(
    "MailCheck" + mName,
    mName,
    i18n("Preparing transmission from \"%1\"...").arg(mName),
    false, // cannot be canceled
    false ); // no tls/ssl

  // run the precommand
  if (!runPrecommand(precommand()))
  {
    kdDebug(5006) << "cannot run precommand " << precommand() << endl;
    checkDone( mHasNewMail, CheckError );
    BroadcastStatus::instance()->setStatusMsg( i18n( "Running precommand failed." ));
    return false;
  }

  mMailFolder->setAutoCreateIndex(FALSE);

  const int rc = mMailFolder->open();
  if ( rc != 0 ) {
    QString aStr;
    aStr = i18n("Cannot open file:");
    aStr += mMailFolder->path()+"/"+mMailFolder->name();
    KMessageBox::sorry(0, aStr);
    kdDebug(5006) << "cannot open file " << mMailFolder->path() << "/"
      << mMailFolder->name() << endl;
    checkDone( mHasNewMail, CheckError );
    BroadcastStatus::instance()->setStatusMsg( i18n( "Transmission failed." ));
    return false;
  }

  if (!mboxStorage->isLocked()) {
    kdDebug(5006) << "mailFolder could not be locked" << endl;
    mMailFolder->close();
    checkDone( mHasNewMail, CheckError );
    QString errMsg = i18n( "Transmission failed: Could not lock %1." )
      .arg( mMailFolder->location() );
    BroadcastStatus::instance()->setStatusMsg( errMsg );
    return false;
  }

  mFolder->open();

  mNumMsgs = mMailFolder->count();

  mMailCheckProgressItem->setTotalItems( mNumMsgs );

  // prepare the static parts of the status message:
  mStatusMsgStub = i18n("Moving message %3 of %2 from %1.")
    .arg(mMailFolder->location()).arg( mNumMsgs );

  //BroadcastStatus::instance()->setStatusProgressEnable( "L" + mName, true );
  return true;
}


//-----------------------------------------------------------------------------
bool KMAcctLocal::fetchMsg()
{
  KMMessage* msg;

  /* This causes mail eating
  if (kmkernel->mailCheckAborted()) break; */

  const QString statusMsg = mStatusMsgStub.arg( mMsgsFetched );
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
    if( fileD0.open( IO_WriteOnly ) ) {
      QCString s = msg->asString();
      uint l = s.length();
      if ( l > 0 ) {
        QDataStream ds( &fileD0 );
        ds.writeRawBytes( s.data(), l );
      }
      fileD0.close();  // If data is 0 we just create a zero length file.
    }
#endif
    msg->setStatus(msg->headerField("Status").latin1(),
      msg->headerField("X-Status").latin1());
    msg->setEncryptionStateChar( msg->headerField( "X-KMail-EncryptionState" ).at(0) );
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
                                           "mailbox <b>%1</b>:<br>%2</qt>" )
                                     .arg( mMailFolder->location() )
                                     .arg( strerror( rc ) ) );
    }

    if( mMailCheckProgressItem ) { // do this only once...
      BroadcastStatus::instance()->setStatusMsgTransmissionCompleted( mName, mNumMsgs );
      mMailCheckProgressItem->setStatus(
        i18n( "Fetched 1 message from mailbox %1.",
              "Fetched %n messages from mailbox %1.",
              mNumMsgs ).arg( mMailFolder->location() ) );
      mMailCheckProgressItem->setComplete();
      mMailCheckProgressItem = 0;
    }
  }
  // else warning is written already

  mMailFolder->close();
  delete mMailFolder; mMailFolder = 0;

  mFolder->close();

  checkDone( mHasNewMail, CheckOK );
}


//-----------------------------------------------------------------------------
void KMAcctLocal::readConfig(KConfig& config)
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
void KMAcctLocal::writeConfig(KConfig& config)
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
