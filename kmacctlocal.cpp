// kmacctlocal.cpp

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <qdatetime.h>
#include <qfileinfo.h>
#include "kmacctlocal.h"
#include "kmfolder.h"
#include "kmmessage.h"
#include "kmacctfolder.h"
#include "kmglobal.h"
#include "kmbroadcaststatus.h"
#include "kmfoldermgr.h"

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

#ifndef _PATH_MAILDIR
#define _PATH_MAILDIR "/var/spool/mail"
#endif
#undef None

//-----------------------------------------------------------------------------
KMAcctLocal::KMAcctLocal(KMAcctMgr* aOwner, const QString& aAccountName):
  KMAcctLocalInherited(aOwner, aAccountName)
{
  mLock = procmail_lockfile;
}


//-----------------------------------------------------------------------------
KMAcctLocal::~KMAcctLocal()
{
  mLocation = "";
}


//-----------------------------------------------------------------------------
const char* KMAcctLocal::type(void) const
{
  return "local";
}


//-----------------------------------------------------------------------------
void KMAcctLocal::init(void)
{
  mLocation = getenv("MAIL");
  if (mLocation.isNull()) {
    mLocation = _PATH_MAILDIR;
    mLocation += "/";
    mLocation += getenv("USER");
  }
  setProcmailLockFileName("");
}


//-----------------------------------------------------------------------------
void KMAcctLocal::pseudoAssign(KMAccount *account)
{
  assert(account->type() == "local");
  KMAcctLocal *acct = static_cast<KMAcctLocal*>(account);
  setName(acct->name());
  setLocation(acct->location());
  setCheckInterval(acct->checkInterval());
  setCheckExclude(acct->checkExclude());
  setLockType(acct->lockType());
  setProcmailLockFileName(acct->procmailLockFileName());
  setFolder(acct->folder());
  setPrecommand(acct->precommand());
}

//-----------------------------------------------------------------------------
void KMAcctLocal::processNewMail(bool)
{
  QTime t;
  hasNewMail = false;

  if ( precommand().isEmpty() ) {
    QFileInfo fi( location() );
    if ( fi.size() == 0 ) {
      emit finishedCheck(hasNewMail);
      return;
    }
  }

  KMFolder mailFolder(NULL, location());
  mailFolder.setLockType( mLock );
  if ( mLock == procmail_lockfile)
    mailFolder.setProcmailLockFileName( mProcmailLockFileName );

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
    aStr = i18n("Cannot open file:");
    aStr += mailFolder.path()+"/"+mailFolder.name();
    KMessageBox::sorry(0, aStr);
    kdDebug(5006) << "cannot open file " << mailFolder.path() << "/"
      << mailFolder.name() << endl;
    emit finishedCheck(hasNewMail);
    KMBroadcastStatus::instance()->setStatusMsg( i18n( "Transmission completed." ));
    return;
  }

  if (mailFolder.isReadOnly()) { // mailFolder is locked
    kdDebug(5006) << "mailFolder could not be locked" << endl;
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
      /*
      if (msg->parent()) {
	  int count = msg->parent()->count();
	  if (count != 1 && msg->parent()->operator[](count - 1) == msg)
	      msg->parent()->unGetMsg(count - 1);
      }
      */
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
    kernel->folderMgr()->syncAllFolders();
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
void KMAcctLocal::readConfig(KConfig& config)
{
  KMAcctLocalInherited::readConfig(config);
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
  KMAcctLocalInherited::writeConfig(config);

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

