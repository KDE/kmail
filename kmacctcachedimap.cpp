/**
 * kmacctimap.cpp
 *
 * Copyright (c) 2000-2002 Michael Haeckel <haeckel@kde.org>
 *
 * This file is based on kmacctexppop.cpp by Don Sanders
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
#include "kmfolderimap.h"
#include "kmmainwin.h"
#include "cachedimapjob.h"

#include <kio/passdlg.h>
#include <kio/scheduler.h>
#include <kio/slave.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kstandarddirs.h>


KMAcctCachedImap::KMAcctCachedImap(KMAcctMgr* aOwner, const QString& aAccountName):
  KMail::ImapAccountBase(aOwner, aAccountName), mFolder(0),
  mProgressDialogEnabled(true)
{
  // Never EVER set this for the cached IMAP account
  mAutoExpunge = false;

  connect(KMBroadcastStatus::instance(), SIGNAL(signalAbortRequested()),
          this, SLOT(slotAbortRequested()));
  connect(&mIdleTimer, SIGNAL(timeout()), SLOT(slotIdleTimeout()));
  KIO::Scheduler::connect(SIGNAL(slaveError(KIO::Slave *, int, const QString &)),
			  this, SLOT(slotSlaveError(KIO::Slave *, int, const QString &)));
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
  base::init();

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

  base::pseudoAssign( a );
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
void KMAcctCachedImap::displayProgress()
{
  if (mProgressEnabled == mapJobData.isEmpty())
  {
    mProgressEnabled = !mapJobData.isEmpty();
    KMBroadcastStatus::instance()->setStatusProgressEnable( "I" + mName,
      mProgressEnabled );
    if (!mProgressEnabled) kernel->filterMgr()->cleanup();
  }
  mIdle = FALSE;
  if (mapJobData.isEmpty())  {
    //mIdleTimer.start(15000);
    mIdleTimer.start(0);
    kdDebug(5006) << "KMAcctCachedImap::displayProgress no more jobs, disconnecting slave" << endl;
  }
  else
    mIdleTimer.stop();
  int total = 0, done = 0;
  // This is a loop, but it seems that we can currently have only one job at a time in this map.
  for (QMap<KIO::Job*, jobData>::Iterator it = mapJobData.begin();
    it != mapJobData.end(); ++it)
  {
    total += (*it).total; // always ==1 (in kmfoldercachedimap.cpp)
    //done += (*it).done;
    Q_ASSERT( (*it).parent );
    if( (*it).parent )  {
      done += static_cast<KMFolderCachedImap*>((*it).parent)->progress();
    }
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
      fld->quiet(FALSE);
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
  QMap<KIO::Job *, jobData>::Iterator it = mapJobData.find(job);
  bool quiet = FALSE;
  if (it != mapJobData.end())
  {
    quiet = (*it).quiet;
    mapJobData.remove(it);
  }
  if (job->error())
  {
    if (!quiet) slotSlaveError(mSlave, job->error(),
        job->errorText() );
    if (job->error() == KIO::ERR_SLAVE_DIED) slaveDied();
  }
  displayProgress();
}


//-----------------------------------------------------------------------------
void KMAcctCachedImap::processNewMail(bool interactive)
{
  // This should never be set for a cached IMAP account
  mAutoExpunge = false;

  // This assertion must hold, otherwise we have to search for the root folder
  // assert(mFolder && mFolder->imapPath() == "/");

  if( interactive && isProgressDialogEnabled() ) {
    imapProgressDialog()->clear();
    imapProgressDialog()->show();
    imapProgressDialog()->raise();
  }

  mFolder->setAccount(this);
  mFolder->serverSync();
  checkDone(false, 0);
}

void KMAcctCachedImap::postProcessNewMail(KMFolderCachedImap* folder, bool)
{
  disconnect(folder, SIGNAL(folderComplete(KMFolderCachedImap*, bool)),
      this, SLOT(postProcessNewMail(KMFolderCachedImap*, bool)));
  postProcessNewMail(static_cast<KMFolder*>(folder));
}

//
//
// read/write config
//
//

void KMAcctCachedImap::readConfig( /*const*/ KConfig/*Base*/ & config ) {
  base::readConfig( config );
  setProgressDialogEnabled( config.readBoolEntry( "progressdialog", true ) );
}

void KMAcctCachedImap::writeConfig( KConfig/*Base*/ & config ) /*const*/ {
  base::writeConfig( config );
  config.writeEntry( "progressdialog", isProgressDialogEnabled() );
}

void KMAcctCachedImap::invalidateIMAPFolders()
{
  mFolder->setAccount(this);

  QStringList strList;
  QValueList<QGuardedPtr<KMFolder> > folderList;
  kernel->imapFolderMgr()->createFolderList(&strList, &folderList,
					    mFolder->child(), QString::null, false);
  QValueList<QGuardedPtr<KMFolder> >::Iterator it;
  mCountRemainChecks = 0;
  mCountLastUnread = 0;

  if( folderList.count() > 0 )
    for( it = folderList.begin(); it != folderList.end(); ++it ) {
      KMFolder *folder = *it;
      if( folder && folder->protocol() == "cachedimap" ) {
	KMFolderCachedImap *cfolder = static_cast<KMFolderCachedImap*>(folder);
	// This invalidates the folder completely
	cfolder->setUidValidity("INVALID");
	cfolder->writeUidCache();
      }
    }

  processNewMail(false);
}

IMAPProgressDialog* KMAcctCachedImap::imapProgressDialog() const
{
  if( !mProgressDlg ) {
    mProgressDlg = new IMAPProgressDialog(0);
  }
  return mProgressDlg;
}

#include "kmacctcachedimap.moc"
