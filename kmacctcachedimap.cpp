/**
 * kmacctcachedimap.cpp
 *
 * Copyright (c) 2002-2003 Bo Thorsen <bo@klaralvdalens-datakonsult.se>
 * Copyright (c) 2002-2003 Steffen Hansen <steffen@klaralvdalens-datakonsult.se>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmacctcachedimap.h"
using KMail::SieveConfig;

#include "imapprogressdialog.h"
using KMail::IMAPProgressDialog;

#include "kmbroadcaststatus.h"
#include "kmfoldertree.h"
#include "kmfoldermgr.h"
#include "kmfiltermgr.h"
#include "kmfoldercachedimap.h"
#include "kmmainwin.h"
#include "kmkernel.h"

#include <kio/passdlg.h>
#include <kio/scheduler.h>
#include <kio/slave.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kapplication.h>
#include <kconfig.h>


KMAcctCachedImap::KMAcctCachedImap( KMAcctMgr* aOwner,
				    const QString& aAccountName )
  : KMail::ImapAccountBase( aOwner, aAccountName ), mFolder( 0 ),
    mProgressDialogEnabled( true ), mSyncActive( false )
{
  // Never EVER set this for the cached IMAP account
  mAutoExpunge = false;

  connect(KMBroadcastStatus::instance(), SIGNAL(signalAbortRequested()),
          this, SLOT(slotAbortRequested()));
  connect(&mIdleTimer, SIGNAL(timeout()), SLOT(slotIdleTimeout()));
}


//-----------------------------------------------------------------------------
KMAcctCachedImap::~KMAcctCachedImap()
{
  killAllJobs( true );
  delete mProgressDlg;
}


//-----------------------------------------------------------------------------
QString KMAcctCachedImap::type() const
{
  return "cachedimap";
}

void KMAcctCachedImap::init() {
  ImapAccountBase::init();

  setProgressDialogEnabled( true );
}

//-----------------------------------------------------------------------------
void KMAcctCachedImap::pseudoAssign( const KMAccount * a ) {
  mIdleTimer.stop();
  killAllJobs( true );
  if (mFolder)
  {
    mFolder->setContentState(KMFolderCachedImap::imapNoInformation);
    mFolder->setSubfolderState(KMFolderCachedImap::imapNoInformation);
  }

  setProgressDialogEnabled(static_cast<const KMAcctCachedImap*>(a)->isProgressDialogEnabled());

  ImapAccountBase::pseudoAssign( a );
}

void KMAcctCachedImap::setPrefixHook() {
  if ( mFolder ) mFolder->setImapPath( prefix() );
}

//-----------------------------------------------------------------------------
void KMAcctCachedImap::setImapFolder(KMFolderCachedImap *aFolder)
{
  mFolder = aFolder;
  mFolder->setImapPath(mPrefix);
  mFolder->setAccount( this );
}


//-----------------------------------------------------------------------------
void KMAcctCachedImap::setAutoExpunge( bool /*aAutoExpunge*/ )
{
  // Never EVER set this for the cached IMAP account
  mAutoExpunge = false;
}

//-----------------------------------------------------------------------------
void KMAcctCachedImap::slotSlaveError(KIO::Slave *aSlave, int errorCode,
  const QString &errorMsg)
{
  if (aSlave != mSlave) return;
  if (errorCode == KIO::ERR_SLAVE_DIED) slaveDied();
  if (errorCode == KIO::ERR_COULD_NOT_LOGIN) mAskAgain = TRUE;

  // Note: HEAD has a killAllJobs() call here

  // check if we still display an error
  if ( !mErrorDialogIsActive )
  {
    mErrorDialogIsActive = true;
    KMessageBox::messageBox(kapp->activeWindow(), KMessageBox::Error,
          KIO::buildErrorString(errorCode, errorMsg),
          i18n("Error"));
    mErrorDialogIsActive = false;
  } else
    kdDebug(5006) << "suppressing error:" << errorMsg << endl;

  mSyncActive = false;
}


//-----------------------------------------------------------------------------
void KMAcctCachedImap::displayProgress()
{
  if (mProgressEnabled == mapJobData.isEmpty())
  {
    mProgressEnabled = !mapJobData.isEmpty();
    KMBroadcastStatus::instance()->setStatusProgressEnable( "I" + mName,
      mProgressEnabled );
    if (!mProgressEnabled) kmkernel->filterMgr()->deref(true);
  }
  mIdle = FALSE;
  if( mapJobData.isEmpty() )
    mIdleTimer.start(0);
  else
    mIdleTimer.stop();
  int total = 0, done = 0;
  // This is a loop, but it seems that we can currently have only one job at a time in this map.
  for (QMap<KIO::Job*, jobData>::Iterator it = mapJobData.begin();
    it != mapJobData.end(); ++it)
  {
    total += (*it).total; // always ==1 (in kmfoldercachedimap.cpp)
    if( (*it).parent )
      done += static_cast<KMFolderCachedImap*>((*it).parent)->progress();
    else
      done += (*it).done;
  }
  if (total == 0) // can't happen
  {
    mTotal = 0;
    return;
  }
  //if (total > mTotal) mTotal = total;
  //done += mTotal - total;
  KMBroadcastStatus::instance()->setStatusProgressPercent( "I" + mName,
     done / total );
     //  100*done / mTotal );
}


//-----------------------------------------------------------------------------
void KMAcctCachedImap::slotIdleTimeout()
{
  if (/*mIdle*/true) // STEFFEN: Hacked this to always disconnect
  {
    if (mSlave) KIO::Scheduler::disconnectSlave(mSlave);
    mSlave = NULL;
    mIdleTimer.stop();
  } else {
    if (mSlave)
    {
      QByteArray packedArgs;
      QDataStream stream( packedArgs, IO_WriteOnly);

      stream << (int) 'N';

      KIO::SimpleJob *job = KIO::special(getUrl(), packedArgs, FALSE);
      KIO::Scheduler::assignJobToSlave(mSlave, job);
      connect(job, SIGNAL(result(KIO::Job *)),
        this, SLOT(slotSimpleResult(KIO::Job *)));
    }
    else mIdleTimer.stop();
  }
}


//-----------------------------------------------------------------------------
void KMAcctCachedImap::slotAbortRequested()
{
  killAllJobs();
}


//-----------------------------------------------------------------------------
void KMAcctCachedImap::killAllJobs( bool disconnectSlave )
{
  QMap<KIO::Job*, jobData>::Iterator it = mapJobData.begin();
  for (it = mapJobData.begin(); it != mapJobData.end(); ++it)
    if ((*it).parent)
    {
      KMFolderCachedImap *fld = static_cast<KMFolderCachedImap*>((*it).parent);
      fld->resetSyncState();
      fld->setContentState(KMFolderCachedImap::imapNoInformation);
      fld->setSubfolderState(KMFolderCachedImap::imapNoInformation);
      fld->sendFolderComplete(FALSE);
    }
  if (mSlave && mapJobData.begin() != mapJobData.end())
  {
    mSlave->kill();
    mSlave = 0;
  }
  mapJobData.clear();

  // Clear the joblist. Make SURE to stop the job emitting "finished"
  for( QPtrListIterator<CachedImapJob> it( mJobList ); it.current(); ++it )
    it.current()->setPassiveDestructor( true );
  mJobList.setAutoDelete(true);
  mJobList.clear();
  mJobList.setAutoDelete(false);
  displayProgress();

  if ( disconnectSlave && slave() ) {
    KIO::Scheduler::disconnectSlave( slave() );
    mSlave = 0;
  }

  // Finally allow new syncs to proceed
  mSyncActive = false;
}


//-----------------------------------------------------------------------------
void KMAcctCachedImap::killJobsForItem(KMFolderTreeItem * fti)
{
  QMap<KIO::Job *, jobData>::Iterator it = mapJobData.begin();
  while (it != mapJobData.end())
  {
    if (it.data().parent == fti->folder())
    {
      killAllJobs();
      break;
    }
    else ++it;
  }
}


//-----------------------------------------------------------------------------
void KMAcctCachedImap::slotSimpleResult(KIO::Job * job)
{
  JobIterator it = findJob( job );
  bool quiet = false;
  if (it != mapJobData.end())
  {
    quiet = (*it).quiet;
    removeJob(it);
  }
  if (job->error())
  {
    if (!quiet) slotSlaveError(mSlave, job->error(),
        job->errorText() );
    if (job->error() == KIO::ERR_SLAVE_DIED) slaveDied();
    mSyncActive = false;
  }
  displayProgress();
}


//-----------------------------------------------------------------------------
void KMAcctCachedImap::processNewMail( KMFolderCachedImap* folder,
				       bool interactive )
{
  // This should never be set for a cached IMAP account
  mAutoExpunge = false;

  // Guard against parallel runs
  if( mSyncActive ) {
    kdDebug(5006) << "Already processing new mail, won't start again\n";
    return;
  }
  mSyncActive = true;

  emit newMailsProcessed(-1);
  if( interactive && isProgressDialogEnabled() ) {
    imapProgressDialog()->clear();
    imapProgressDialog()->show();
    imapProgressDialog()->raise();
  }

  folder->setAccount(this);
  connect(folder, SIGNAL(folderComplete(KMFolderCachedImap*, bool)),
	  this, SLOT(postProcessNewMail(KMFolderCachedImap*, bool)));
  folder->serverSync( interactive && isProgressDialogEnabled() );
  checkDone(false, 0);
}

void KMAcctCachedImap::postProcessNewMail( KMFolderCachedImap* folder, bool )
{
  mSyncActive = false;
  emit finishedCheck(false);
  disconnect(folder, SIGNAL(folderComplete(KMFolderCachedImap*, bool)),
      this, SLOT(postProcessNewMail(KMFolderCachedImap*, bool)));
  //postProcessNewMail(static_cast<KMFolder*>(folder));
}

//
//
// read/write config
//
//

void KMAcctCachedImap::readConfig( /*const*/ KConfig/*Base*/ & config ) {
  ImapAccountBase::readConfig( config );
  setProgressDialogEnabled( config.readBoolEntry( "progressdialog", true ) );
}

void KMAcctCachedImap::writeConfig( KConfig/*Base*/ & config ) /*const*/ {
  ImapAccountBase::writeConfig( config );
  config.writeEntry( "progressdialog", isProgressDialogEnabled() );
}

void KMAcctCachedImap::invalidateIMAPFolders()
{
  invalidateIMAPFolders( mFolder );
}

void KMAcctCachedImap::invalidateIMAPFolders( KMFolderCachedImap* folder )
{
  folder->setAccount(this);

  QStringList strList;
  QValueList<QGuardedPtr<KMFolder> > folderList;
  kmkernel->dimapFolderMgr()->createFolderList( &strList, &folderList,
						folder->child(), QString::null,
						false );
  QValueList<QGuardedPtr<KMFolder> >::Iterator it;
  mCountRemainChecks = 0;
  mCountLastUnread = 0;

  if( folderList.count() > 0 )
    for( it = folderList.begin(); it != folderList.end(); ++it ) {
      KMFolder *folder = *it;
      if( folder && folder->folderType() == KMFolderTypeCachedImap ) {
	KMFolderCachedImap *cfolder = static_cast<KMFolderCachedImap*>(folder);
	// This invalidates the folder completely
	cfolder->setUidValidity("INVALID");
	cfolder->writeUidCache();
      }
    }
  folder->setUidValidity("INVALID");
  folder->writeUidCache();

  processNewMail(false);
}

//-----------------------------------------------------------------------------
void KMAcctCachedImap::listDirectory(QString path, bool onlySubscribed,
    bool secondStep, KMFolder* parent, bool reset)
{
  ImapAccountBase::listDirectory( path, onlySubscribed, secondStep, parent, reset );
}

//-----------------------------------------------------------------------------
void KMAcctCachedImap::listDirectory()
{
  mFolder->listDirectory();
}

IMAPProgressDialog* KMAcctCachedImap::imapProgressDialog() const
{
  if( !mProgressDlg ) {
    mProgressDlg = new IMAPProgressDialog( KMKernel::self()->mainWin() );
  }
  return mProgressDlg;
}

#include "kmacctcachedimap.moc"
