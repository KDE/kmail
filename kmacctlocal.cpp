// kmacctlocal.cpp

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmacctlocal.h"
#include "kmfoldermbox.h"
#include "kmmessage.h"
#include "kmacctfolder.h"
#include "kmbroadcaststatus.h"
#include "kmfoldermgr.h"

#include <kapplication.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>

#include <qfileinfo.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#ifdef HAVE_PATHS_H
#include <paths.h>	/* defines _PATH_MAILDIR */
#endif

#ifndef _PATH_MAILDIR
#define _PATH_MAILDIR "/var/spool/mail"
#endif
#undef None

//-----------------------------------------------------------------------------
KMAcctLocal::KMAcctLocal(KMAcctMgr* aOwner, const QString& aAccountName):
  base(aOwner, aAccountName)
{
  mLock = procmail_lockfile;
}


//-----------------------------------------------------------------------------
KMAcctLocal::~KMAcctLocal()
{
  mLocation = "";
}


//-----------------------------------------------------------------------------
QString KMAcctLocal::type(void) const
{
  return "local";
}


//-----------------------------------------------------------------------------
void KMAcctLocal::init() {
  base::init();

  mLocation = getenv("MAIL");
  if (mLocation.isNull()) {
    mLocation = _PATH_MAILDIR;
    mLocation += '/';
    mLocation += getenv("USER");
  }
  setProcmailLockFileName("");
}


//-----------------------------------------------------------------------------
void KMAcctLocal::pseudoAssign( const KMAccount * a )
{
  base::pseudoAssign( a );

  const KMAcctLocal * l = dynamic_cast<const KMAcctLocal*>( a );
  if ( !l ) return;

  setLocation( l->location() );
  setLockType( l->lockType() );
  setProcmailLockFileName( l->procmailLockFileName() );
}

//-----------------------------------------------------------------------------
void KMAcctLocal::processNewMail(bool)
{
  QTime t;
  hasNewMail = false;

  if ( precommand().isEmpty() ) {
    QFileInfo fi( location() );
    if ( fi.size() == 0 ) {
      QString statusMsg = i18n("Transmission complete. No new messages.");
      KMBroadcastStatus::instance()->setStatusMsg( statusMsg );
      checkDone(hasNewMail, 0);
      return;
    }
  }

  KMFolderMbox mailFolder(0, location());
  mailFolder.setLockType( mLock );
  if ( mLock == procmail_lockfile)
    mailFolder.setProcmailLockFileName( mProcmailLockFileName );

  long num = 0;
  long i;
  int rc;
  KMMessage* msg;
  bool addedOk;

  if (!mFolder) {
    checkDone(hasNewMail, -1);
    KMBroadcastStatus::instance()->setStatusMsg( i18n( "Transmission failed." ));
    return;
  }

  KMBroadcastStatus::instance()->reset();
  KMBroadcastStatus::instance()->setStatusMsg(
	i18n("Preparing transmission from \"%1\"...").arg(mName));

  // run the precommand
  if (!runPrecommand(precommand()))
  {
    kdDebug(5006) << "cannot run precommand " << precommand() << endl;
    checkDone(hasNewMail, -1);
  }

  mailFolder.setAutoCreateIndex(FALSE);

  rc = mailFolder.open();
  if (rc)
  {
    QString aStr;
    aStr = i18n("Cannot open file:");
    aStr += mailFolder.path()+"/"+mailFolder.name();
    KMessageBox::sorry(0, aStr);
    kdDebug(5006) << "cannot open file " << mailFolder.path() << "/"
      << mailFolder.name() << endl;
    checkDone(hasNewMail, -1);
    KMBroadcastStatus::instance()->setStatusMsg( i18n( "Transmission failed." ));
    return;
  }

  if (mailFolder.isReadOnly()) { // mailFolder is locked
    kdDebug(5006) << "mailFolder could not be locked" << endl;
    mailFolder.close();
    checkDone(hasNewMail, -1);
    QString errMsg = i18n( "Transmission failed: Could not lock %1." )
      .arg( mailFolder.location() );
    KMBroadcastStatus::instance()->setStatusMsg( errMsg );
    return;
  }

  mFolder->quiet(TRUE);
  mFolder->open();

  num = mailFolder.count();

  addedOk = true;
  t.start();

  // prepare the static parts of the status message:
  QString statusMsgStub = i18n("Moving message %3 of %2 from %1.")
    .arg(mailFolder.location()).arg(num);

  KMBroadcastStatus::instance()->setStatusProgressEnable( "L" + mName, true );
  for (i=0; i<num; i++)
  {

    if (!addedOk) break;

    /* This causes mail eating
    if (KMBroadcastStatus::instance()->abortRequested()) break; */

    QString statusMsg = statusMsgStub.arg(i);
    KMBroadcastStatus::instance()->setStatusMsg( statusMsg );
    KMBroadcastStatus::instance()->setStatusProgressPercent( "L" + mName,
      (i*100) / num );

    msg = mailFolder.take(0);
    if (msg)
    {
/*
QFile fileD0( "testdat_xx-0-0" );
if( fileD0.open( IO_WriteOnly ) ) {
    if( data ) {
    QDataStream ds( &fileD0 );
    ds.writeRawBytes( msg->asString(), msg->asString.length() );
    }
    fileD0.close();  // If data is 0 we just create a zero length file.
}
*/
      msg->setStatus(msg->headerField("Status").latin1(),
        msg->headerField("X-Status").latin1());
      msg->setEncryptionStateChar( msg->headerField( "X-KMail-EncryptionState" ).at(0) );
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
  KMBroadcastStatus::instance()->setStatusProgressEnable( "L" + mName, false );
  KMBroadcastStatus::instance()->reset();

  if (addedOk)
  {
    kernel->folderMgr()->syncAllFolders();
    rc = mailFolder.expunge();
    if (rc)
      KMessageBox::information( 0, i18n("<qt>Cannot remove mail from mailbox <b>%1</b>:<br>%2</qt>").arg(mailFolder.location()).arg(strerror(rc)));
    QString statusMsg;
    if ( num )
      statusMsg = i18n("Transmission complete. %n new message.",
		       "Transmission complete. %n new messages.", num);
    else
      statusMsg = i18n("Transmission complete. No new messages.");
    KMBroadcastStatus::instance()->setStatusMsg( statusMsg );
  }
  // else warning is written already

  mailFolder.close();
  mFolder->close();
  mFolder->quiet(FALSE);

  checkDone(hasNewMail, num);

  return;
}


//-----------------------------------------------------------------------------
void KMAcctLocal::readConfig(KConfig& config)
{
  base::readConfig(config);
  mLocation = config.readEntry("Location", mLocation);
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
    mLock = None;
  else mLock = FCNTL;
}


//-----------------------------------------------------------------------------
void KMAcctLocal::writeConfig(KConfig& config)
{
  base::writeConfig(config);

  config.writeEntry("Location", mLocation);

  QString st = "fcntl";
  if (mLock == procmail_lockfile) st = "procmail_lockfile";
  else if (mLock == mutt_dotlock) st = "mutt_dotlock";
  else if (mLock == mutt_dotlock_privileged) st = "mutt_dotlock_privileged";
  else if (mLock == None) st = "none";
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

void
KMAcctLocal::setProcmailLockFileName(QString s)
{
  if (!s.isEmpty())
    mProcmailLockFileName = s;
  else
    mProcmailLockFileName = mLocation + ".lock";
}

