// kmacctmaildir.cpp

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <qdatetime.h>
#include <qfileinfo.h>
#include "kmacctmaildir.h"
#include "kmfoldermaildir.h"
#include "kmmessage.h"
#include "kmacctfolder.h"
#include "kmglobal.h"
#include "kmbroadcaststatus.h"

#include <kapp.h>
#include <kconfig.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>

#ifdef HAVE_PATHS_H
#include <paths.h>	/* defines _PATH_MAILDIR */
#endif

#undef None

//-----------------------------------------------------------------------------
KMAcctMaildir::KMAcctMaildir(KMAcctMgr* aOwner, const QString& aAccountName):
  KMAcctMaildirInherited(aOwner, aAccountName)
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
void KMAcctMaildir::init(void)
{
  mLocation = getenv("MAIL");
  if (mLocation.isNull()) {
    mLocation = getenv("HOME");
    mLocation += "/Maildir/";
  }
}


//-----------------------------------------------------------------------------
void KMAcctMaildir::pseudoAssign(KMAccount *account)
{
  assert(account->type() == "maildir");
  KMAcctMaildir *acct = static_cast<KMAcctMaildir*>(account);
  setName(acct->name());
  setLocation(acct->location());
  setCheckInterval(acct->checkInterval());
  setCheckExclude(acct->checkExclude());
  setFolder(acct->folder());
  setPrecommand(acct->precommand());
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
      return;
    }
  }

  KMFolderMaildir mailFolder(NULL, location());

  long num = 0;
  long i;
  int rc;
  KMMessage* msg;
  bool addedOk;

  if (!mFolder) {
    emit finishedCheck(hasNewMail);
    return;
  }

  KMBroadcastStatus::instance()->reset();
  KMBroadcastStatus::instance()->setStatusMsg(
	i18n("Preparing transmission from %1...").arg(mailFolder.name()));

  // run the precommand
  if (!runPrecommand(precommand()))
  {
    kdDebug(5006) << "cannot run precommand " << precommand() << endl;
    emit finishedCheck(hasNewMail);
  }

  mailFolder.setAutoCreateIndex(FALSE);

  rc = mailFolder.open();
  if (rc)
  {
    QString aStr;
    aStr = i18n("Cannot open folder:");
    aStr += mailFolder.path()+"/"+mailFolder.name();
    KMessageBox::sorry(0, aStr);
    kdDebug(5006) << "cannot open file " << mailFolder.path() << "/"
      << mailFolder.name() << endl;
    emit finishedCheck(hasNewMail);
    KMBroadcastStatus::instance()->setStatusMsg( i18n( "Transmission completed." ));
    return;
  }

  if (mailFolder.isReadOnly()) { // mailFolder is locked
    mailFolder.close();
    emit finishedCheck(hasNewMail);
    KMBroadcastStatus::instance()->setStatusMsg( i18n( "Transmission completed." ));
    return;
  }

  mFolder->quiet(TRUE);
  mFolder->open();


  num = mailFolder.count();

  addedOk = true;
  t.start();

  KMBroadcastStatus::instance()->setStatusProgressEnable( true );
  for (i=0; i<num; i++)
  {

    if (!addedOk) break;
    if (KMBroadcastStatus::instance()->abortRequested()) break;

    KMBroadcastStatus::instance()->setStatusMsg( i18n("Message ") +
			                QString("%1/%2").arg(i).arg(num) );
    KMBroadcastStatus::instance()->setStatusProgressPercent( (i*100) / num );

    msg = mailFolder.take(0);
    if (msg)
    {
      msg->setStatus(msg->headerField("Status").latin1(),
        msg->headerField("X-Status").latin1());
      addedOk = processNewMsg(msg);
      if (addedOk)
        hasNewMail = true;
    }

    if (t.elapsed() >= 200) { //hardwired constant
      kapp->processEvents();
      t.start();
    }

  }
  KMBroadcastStatus::instance()->setStatusProgressEnable( false );
  KMBroadcastStatus::instance()->reset();

  if (addedOk)
  {
    rc = mailFolder.expunge();
    if (rc)
      KMessageBox::information( 0, i18n("Cannot remove mail from\nmailbox `%1':\n%2").arg(mailFolder.location()).arg(strerror(rc)));
    KMBroadcastStatus::instance()->setStatusMsg( i18n( "Transmission completed." ));
  }
  // else warning is written already

  mailFolder.close();
  mFolder->close();
  mFolder->quiet(FALSE);

  emit finishedCheck(hasNewMail);

  return;
}


//-----------------------------------------------------------------------------
void KMAcctMaildir::readConfig(KConfig& config)
{
  KMAcctMaildirInherited::readConfig(config);
  mLocation = config.readEntry("Location", mLocation);
}


//-----------------------------------------------------------------------------
void KMAcctMaildir::writeConfig(KConfig& config)
{
  KMAcctMaildirInherited::writeConfig(config);

  config.writeEntry("Location", mLocation);
}

//-----------------------------------------------------------------------------
void KMAcctMaildir::setLocation(const QString& aLocation)
{
  mLocation = aLocation;
}
