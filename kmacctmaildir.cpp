// kmacctmaildir.cpp

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <qfileinfo.h>
#include "kmacctmaildir.h"
#include "kmfoldermaildir.h"
#include "kmmessage.h"
#include "kmacctfolder.h"
#include "kmbroadcaststatus.h"

#include <kapplication.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#ifdef HAVE_PATHS_H
#include <paths.h>	/* defines _PATH_MAILDIR */
#endif

#undef None

//-----------------------------------------------------------------------------
KMAcctMaildir::KMAcctMaildir(KMAcctMgr* aOwner, const QString& aAccountName):
  base(aOwner, aAccountName)
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
  base::init();

  mLocation = getenv("MAIL");
  if (mLocation.isNull()) {
    mLocation = getenv("HOME");
    mLocation += "/Maildir/";
  }
}


//-----------------------------------------------------------------------------
void KMAcctMaildir::pseudoAssign( const KMAccount * a )
{
  base::pseudoAssign( a );

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
      emit finishedCheck(hasNewMail);
      emit newMailsProcessed(0);
      QString statusMsg = i18n("Transmission completed, no new messages");
      KMBroadcastStatus::instance()->setStatusMsg( statusMsg );
      return;
    }
  }

  KMFolderMaildir mailFolder(0, location());

  long num = 0;
  long i;
  int rc;
  KMMessage* msg;
  bool addedOk;

  if (!mFolder) {
    emit finishedCheck(hasNewMail);
    emit newMailsProcessed(-1);
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
    emit finishedCheck(hasNewMail);
    emit newMailsProcessed(-1);
  }

  mailFolder.setAutoCreateIndex(FALSE);

  rc = mailFolder.open();
  if (rc)
  {
    QString aStr = i18n("<qt>Cannot open folder <b>%1</b>.</qt>").arg( mailFolder.location() );
    KMessageBox::sorry(0, aStr);
    kdDebug(5006) << "cannot open folder " << mailFolder.location() << endl;
    emit finishedCheck(hasNewMail);
    emit newMailsProcessed(-1);
    KMBroadcastStatus::instance()->setStatusMsg( i18n( "Transmission failed." ));
    return;
  }

  if (mailFolder.isReadOnly()) { // mailFolder is locked
    mailFolder.close();
    emit finishedCheck(hasNewMail);
    emit newMailsProcessed(-1);
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

  KMBroadcastStatus::instance()->setStatusProgressEnable( "M" + mName, true );
  for (i=0; i<num; i++)
  {

    if (!addedOk) break;
    if (KMBroadcastStatus::instance()->abortRequested()) break;

    QString statusMsg = statusMsgStub.arg(i);
    KMBroadcastStatus::instance()->setStatusMsg( statusMsg );
    KMBroadcastStatus::instance()->setStatusProgressPercent( "M" + mName,
      (i*100) / num );

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
  KMBroadcastStatus::instance()->setStatusProgressEnable( "M" + mName, false );
  KMBroadcastStatus::instance()->reset();

  if (addedOk)
  {
    rc = mailFolder.expunge();
    if (rc)
      KMessageBox::information( 0,
                              i18n("<qt>Cannot remove mail from mailbox <b>%1</b>:<br>%2</qt>")
                                .arg(mailFolder.location()).arg(strerror(rc)));
    QString statusMsg;
    if ( num )
      statusMsg = i18n("Transmission completed, %n new message.",
		       "Transmission completed, %n new messages.", num);
    else
      statusMsg = i18n("Transmission completed, no new messages");
    KMBroadcastStatus::instance()->setStatusMsg( statusMsg );
  }
  // else warning is written already

  mailFolder.close();
  mFolder->close();
  mFolder->quiet(FALSE);

  emit finishedCheck(hasNewMail);
  emit newMailsProcessed(num);

  return;
}


//-----------------------------------------------------------------------------
void KMAcctMaildir::readConfig(KConfig& config)
{
  base::readConfig(config);
  mLocation = config.readEntry("Location", mLocation);
}


//-----------------------------------------------------------------------------
void KMAcctMaildir::writeConfig(KConfig& config)
{
  base::writeConfig(config);
  config.writeEntry("Location", mLocation);
}

//-----------------------------------------------------------------------------
void KMAcctMaildir::setLocation(const QString& aLocation)
{
  mLocation = aLocation;
}
