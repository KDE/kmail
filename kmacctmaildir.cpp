// kmacctmaildir.cpp
#include "kmacctmaildir.h"
#include <config-kmail.h>
#include "kmfoldermaildir.h"
#include "kmacctfolder.h"
#include "broadcaststatus.h"
using KPIM::BroadcastStatus;
#include "progressmanager.h"
using KPIM::ProgressManager;

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

#ifdef HAVE_PATHS_H
#include <paths.h>       /* defines _PATH_MAILDIR */
#endif

#undef None

//-----------------------------------------------------------------------------
KMAcctMaildir::KMAcctMaildir(AccountManager* aOwner, const QString& aAccountName, uint id):
  KMAccount(aOwner, aAccountName, id)
{
}


//-----------------------------------------------------------------------------
KMAcctMaildir::~KMAcctMaildir()
{
  mLocation = "";
}


//-----------------------------------------------------------------------------
void KMAcctMaildir::init() {
  KMAccount::init();

  mLocation = qgetenv("MAIL");
  if (mLocation.isNull()) {
    mLocation = qgetenv("HOME");
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

  KMFolder mailFolder(0, location(), KMFolderTypeMaildir,
                              false /* no index */, false /* don't export sernums */);

  long num = 0;
  int rc;
  KMMessage* msg;
  bool addedOk;

  if (!mFolder) {
    checkDone( hasNewMail, CheckError );
    BroadcastStatus::instance()->setStatusMsg( i18n( "Transmission failed." ));
    return;
  }

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
    kDebug() << "cannot run precommand" << precommand();
    checkDone( hasNewMail, CheckError );
    BroadcastStatus::instance()->setStatusMsg( i18n( "Transmission failed." ));
    return;
  }

  rc = mailFolder.open( "acctmaildirMail" );
  if (rc)
  {
    QString aStr = i18n("<qt>Cannot open folder <b>%1</b>.</qt>", mailFolder.location() );
    KMessageBox::sorry(0, aStr);
    kDebug() << "cannot open folder" << mailFolder.location();
    checkDone( hasNewMail, CheckError );
    BroadcastStatus::instance()->setStatusMsg( i18n( "Transmission failed." ));
    return;
  }

  mFolder->open( "acctmaildirFold" );


  num = mailFolder.count();

  addedOk = true;
  t.start();

  mMailCheckProgressItem->setTotalItems( num );

  for (long i=0; i<num; i++)
  {

    if( kmkernel->mailCheckAborted() ) {
      BroadcastStatus::instance()->setStatusMsg( i18n("Transmission aborted.") );
      num = i;
      addedOk = false;
    }
    if (!addedOk) break;

    QString statusMsg = i18n( "Moving message %1 of %2 from %3.",
                              i, num, mailFolder.location() );
    mMailCheckProgressItem->incCompletedItems();
    mMailCheckProgressItem->updateProgress();
    mMailCheckProgressItem->setStatus( statusMsg );

    msg = mailFolder.take(0);
    if (msg)
    {
      msg->setStatus(msg->headerField("Status").toLatin1(),
                     msg->headerField("X-Status").toLatin1());
      if ( !msg->headerField( "X-KMail-EncryptionState" ).isEmpty() )
        msg->setEncryptionStateChar( msg->headerField( "X-KMail-EncryptionState" ).at(0) );
      if ( !msg->headerField( "X-KMail-SignatureState" ).isEmpty() )
        msg->setSignatureStateChar( msg->headerField( "X-KMail-SignatureState" ).at(0));

      addedOk = processNewMsg(msg);
      if (addedOk)
        hasNewMail = true;
    }

    if (t.elapsed() >= 200) { //hardwired constant
      qApp->processEvents();
      t.start();
    }

  }

  if( mMailCheckProgressItem ) { // do this only once...
    BroadcastStatus::instance()->setStatusMsgTransmissionCompleted( num );
    mMailCheckProgressItem->setStatus(
      i18np( "Fetched 1 message from maildir folder %2.",
            "Fetched %1 messages from maildir folder %2.",
            num, mailFolder.location() ) );

    mMailCheckProgressItem->setComplete();
    mMailCheckProgressItem = 0;
  }
  if (addedOk)
  {
    BroadcastStatus::instance()->setStatusMsgTransmissionCompleted( mName, num );
  }
  // else warning is written already

  mailFolder.close( "acctmaildirMail" );
  mFolder->close( "acctmaildirFold" );

  checkDone( hasNewMail, CheckOK );

  return;
}


//-----------------------------------------------------------------------------
void KMAcctMaildir::readConfig(KConfigGroup& config)
{
  KMAccount::readConfig(config);
  mLocation = config.readPathEntry("Location", mLocation);
}


//-----------------------------------------------------------------------------
void KMAcctMaildir::writeConfig(KConfigGroup& config)
{
  KMAccount::writeConfig(config);
  config.writePathEntry("Location", mLocation);
}

//-----------------------------------------------------------------------------
void KMAcctMaildir::setLocation(const QString& aLocation)
{
  mLocation = aLocation;
}
