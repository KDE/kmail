// kmacctmaildir.cpp

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <qfileinfo.h>
#include "kmacctmaildir.h"
#include "kmfoldermaildir.h"
#include "kmacctfolder.h"
#include "broadcaststatus.h"
using KPIM::BroadcastStatus;
#include "progressmanager.h"
using KPIM::ProgressManager;

#include <kapplication.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kconfig.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#ifdef HAVE_PATHS_H
#include <paths.h>	/* defines _PATH_MAILDIR */
#endif

#undef None

//-----------------------------------------------------------------------------
KMAcctMaildir::KMAcctMaildir(KMAcctMgr* aOwner, const QString& aAccountName, uint id):
  KMAccount(aOwner, aAccountName, id)
{
}


//-----------------------------------------------------------------------------
KMAcctMaildir::~KMAcctMaildir()
{
  mLocation = "";
}


//-----------------------------------------------------------------------------
QString KMAcctMaildir::type(void) const
{
  return "maildir";
}


//-----------------------------------------------------------------------------
void KMAcctMaildir::init() {
  KMAccount::init();

  mLocation = getenv("MAIL");
  if (mLocation.isNull()) {
    mLocation = getenv("HOME");
    mLocation += "/Maildir/";
  }
}


//-----------------------------------------------------------------------------
void KMAcctMaildir::pseudoAssign( const KMAccount * a )
{
  KMAccount::pseudoAssign( a );

  const KMAcctMaildir * m = dynamic_cast<const KMAcctMaildir*>( a );
  if ( !m ) return;

  setLocation( m->location() );
}

//-----------------------------------------------------------------------------
void KMAcctMaildir::processNewMail(bool)
{
  QTime t;
  hasNewMail = false;

  if ( precommand().isEmpty() ) {
    QFileInfo fi( location() );
    if ( !fi.exists() ) {
      checkDone( hasNewMail, CheckOK );
      BroadcastStatus::instance()->setStatusMsgTransmissionCompleted( mName, 0 );
      return;
    }
  }

  KMFolder mailFolder(0, location(), KMFolderTypeMaildir);

  long num = 0;
  long i;
  int rc;
  KMMessage* msg;
  bool addedOk;

  if (!mFolder) {
    checkDone( hasNewMail, CheckError );
    BroadcastStatus::instance()->setStatusMsg( i18n( "Transmission failed." ));
    return;
  }

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
    checkDone( hasNewMail, CheckError );
  }

  mailFolder.setAutoCreateIndex(FALSE);

  rc = mailFolder.open();
  if (rc)
  {
    QString aStr = i18n("<qt>Cannot open folder <b>%1</b>.</qt>").arg( mailFolder.location() );
    KMessageBox::sorry(0, aStr);
    kdDebug(5006) << "cannot open folder " << mailFolder.location() << endl;
    checkDone( hasNewMail, CheckError );
    BroadcastStatus::instance()->setStatusMsg( i18n( "Transmission failed." ));
    return;
  }

  mFolder->open();


  num = mailFolder.count();

  addedOk = true;
  t.start();

  // prepare the static parts of the status message:
  QString statusMsgStub = i18n("Moving message %3 of %2 from %1.")
    .arg(mailFolder.location()).arg(num);

  mMailCheckProgressItem->setTotalItems( num );

  for (i=0; i<num; i++)
  {

    if( kmkernel->mailCheckAborted() ) {
      BroadcastStatus::instance()->setStatusMsg( i18n("Transmission aborted.") );
      num = i;
      addedOk = false;
    }
    if (!addedOk) break;

    QString statusMsg = statusMsgStub.arg(i);
    mMailCheckProgressItem->incCompletedItems();
    mMailCheckProgressItem->updateProgress();
    mMailCheckProgressItem->setStatus( statusMsg );

    msg = mailFolder.take(0);
    if (msg)
    {
      msg->setStatus(msg->headerField("Status").latin1(),
        msg->headerField("X-Status").latin1());
      msg->setEncryptionStateChar( msg->headerField( "X-KMail-EncryptionState" ).at(0));
      msg->setSignatureStateChar( msg->headerField( "X-KMail-SignatureState" ).at(0));

      addedOk = processNewMsg(msg);
      if (addedOk)
        hasNewMail = true;
    }

    if (t.elapsed() >= 200) { //hardwired constant
      kapp->processEvents();
      t.start();
    }

  }

  if( mMailCheckProgressItem ) { // do this only once...
    BroadcastStatus::instance()->setStatusMsgTransmissionCompleted( num );
    mMailCheckProgressItem->setStatus(
      i18n( "Fetched 1 message from maildir folder %1.",
            "Fetched %n messages from maildir folder %1.",
            num ).arg(mailFolder.location() ) );

    mMailCheckProgressItem->setComplete();
    mMailCheckProgressItem = 0;
  }
  if (addedOk)
  {
    BroadcastStatus::instance()->setStatusMsgTransmissionCompleted( mName, num );
  }
  // else warning is written already

  mailFolder.close();
  mFolder->close();

  checkDone( hasNewMail, CheckOK );

  return;
}


//-----------------------------------------------------------------------------
void KMAcctMaildir::readConfig(KConfig& config)
{
  KMAccount::readConfig(config);
  mLocation = config.readPathEntry("Location", mLocation);
}


//-----------------------------------------------------------------------------
void KMAcctMaildir::writeConfig(KConfig& config)
{
  KMAccount::writeConfig(config);
  config.writePathEntry("Location", mLocation);
}

//-----------------------------------------------------------------------------
void KMAcctMaildir::setLocation(const QString& aLocation)
{
  mLocation = aLocation;
}
