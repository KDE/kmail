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

#include "kmacctimap.h"
using KMail::SieveConfig;

#include "kmbroadcaststatus.h"
#include "kmfoldertree.h"
#include "kmfoldermgr.h"
#include "kmfiltermgr.h"
#include "kmmainwin.h"
#include "imapjob.h"
using KMail::ImapJob;

#include <kmfolderimap.h>
#include <kio/scheduler.h>
#include <kio/slave.h>
#include <kmessagebox.h>
#include <kdebug.h>


//-----------------------------------------------------------------------------
KMAcctImap::KMAcctImap(KMAcctMgr* aOwner, const QString& aAccountName):
  KMail::ImapAccountBase(aOwner, aAccountName)
{
  mFolder = 0;
  mOpenFolders.setAutoDelete(true);
  connect(KMBroadcastStatus::instance(), SIGNAL(signalAbortRequested()),
          this, SLOT(slotAbortRequested()));
  connect(&mIdleTimer, SIGNAL(timeout()), SLOT(slotIdleTimeout()));
  KIO::Scheduler::connect(
    SIGNAL(slaveError(KIO::Slave *, int, const QString &)),
    this, SLOT(slotSlaveError(KIO::Slave *, int, const QString &)));
  connect(kernel->imapFolderMgr(), SIGNAL(changed()),
      this, SLOT(slotUpdateFolderList()));	
}


//-----------------------------------------------------------------------------
KMAcctImap::~KMAcctImap()
{
  killAllJobs( true );
}


//-----------------------------------------------------------------------------
QString KMAcctImap::type() const
{
  return "imap";
}

//-----------------------------------------------------------------------------
void KMAcctImap::pseudoAssign( const KMAccount * a ) {
  mIdleTimer.stop();
  killAllJobs( true );
  if (mFolder)
  {
    mFolder->setContentState(KMFolderImap::imapNoInformation);
    mFolder->setSubfolderState(KMFolderImap::imapNoInformation);
  }
  base::pseudoAssign( a );
}

//-----------------------------------------------------------------------------
void KMAcctImap::setImapFolder(KMFolderImap *aFolder)
{
  mFolder = aFolder;
  mFolder->setImapPath(mPrefix);
}


//-----------------------------------------------------------------------------
int KMAcctImap::tempOpenFolder(KMFolder *folder)
{
  int rc = folder->open();
  if (rc) return rc;
  mOpenFolders.append(new QGuardedPtr<KMFolder>(folder));
  return 0;
}


//-----------------------------------------------------------------------------
void KMAcctImap::slotSlaveError(KIO::Slave *aSlave, int errorCode,
  const QString &errorMsg)
{
  if (aSlave != mSlave) return;
  if (errorCode == KIO::ERR_SLAVE_DIED) slaveDied();
  if (errorCode == KIO::ERR_COULD_NOT_LOGIN && !mStorePasswd) mAskAgain = TRUE;
  if (errorCode == KIO::ERR_DOES_NOT_EXIST)
  {
    // folder is gone, so reload the folderlist
    if (mFolder) mFolder->listDirectory();
    return;
  }
  // check if we still display an error
  killAllJobs();
  if ( !mErrorDialogIsActive )
  {
    mErrorDialogIsActive = true;
    if ( KMessageBox::messageBox(kernel->mainWin(), KMessageBox::Error,
          KIO::buildErrorString(errorCode, errorMsg),
          i18n("Error")) == KMessageBox::Ok )
    {
      mErrorDialogIsActive = false;
    }
  } else
    kdDebug(5006) << "suppressing error:" << errorMsg << endl;
}


//-----------------------------------------------------------------------------
void KMAcctImap::slotIdleTimeout()
{
  if (mIdle)
  {
    if (mSlave) KIO::Scheduler::disconnectSlave(mSlave);
    mSlave = 0;
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
void KMAcctImap::slotAbortRequested()
{
  killAllJobs();
}


//-----------------------------------------------------------------------------
void KMAcctImap::killAllJobs( bool disconnectSlave )
{
  QMap<KIO::Job*, jobData>::Iterator it = mapJobData.begin();
  for (it = mapJobData.begin(); it != mapJobData.end(); ++it)
    if ((*it).parent)
    {
      // clear folder state
      KMFolderImap *fld = static_cast<KMFolderImap*>((*it).parent);
      fld->setContentState(KMFolderImap::imapNoInformation);
      fld->setSubfolderState(KMFolderImap::imapNoInformation);
      fld->sendFolderComplete(FALSE);
      fld->quiet(FALSE);
    }
  if (mSlave && mapJobData.begin() != mapJobData.end())
  {
    mSlave->kill();
    mSlave = 0;
  }
  // remove the jobs
  mapJobData.clear();
  KMAccount::deleteFolderJobs();
  // make sure that no new-mail-check is blocked
  if (mCountRemainChecks > 0)
  {
    checkDone(false, 0);
    mCountRemainChecks = 0;
  }
  displayProgress();

  if ( disconnectSlave && slave() ) {
    KIO::Scheduler::disconnectSlave( slave() );
    mSlave = 0;
  }
}

//-----------------------------------------------------------------------------
void KMAcctImap::ignoreJobsForMessage( KMMessage* msg )
{
  ImapJob *job;
  for ( QPtrListIterator<ImapJob> it( mJobList ); it; ++it ) {
    if ( it.current()->msgList().first() == msg) {
      job = dynamic_cast<ImapJob*>( it.current() );
      mapJobData.remove( job->mJob );
      mJobList.remove( job );
      delete job;
      break;
    }
  }
}

//-----------------------------------------------------------------------------
void KMAcctImap::killJobsForItem(KMFolderTreeItem * fti)
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
void KMAcctImap::slotSimpleResult(KIO::Job * job)
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
void KMAcctImap::processNewMail(bool interactive)
{
  if (!mFolder || !mFolder->child() ||
      !makeConnection())
  {
    mCountRemainChecks = 0;
    checkDone(false, 0);
    return;
  }
  // if necessary then initialize the list of folders which should be checked
  if( mMailCheckFolders.isEmpty() ) 
  {
    slotUpdateFolderList();
    // if no folders should be checked then the check is finished
    if( mMailCheckFolders.isEmpty() )
    {
      checkDone(false, 0);
    }
  }
  QValueList<QGuardedPtr<KMFolder> >::Iterator it;
  // first get the current count of unread-messages
  mCountRemainChecks = 0;
  mCountLastUnread = 0;
  for (it = mMailCheckFolders.begin(); it != mMailCheckFolders.end(); it++)
  {
    KMFolder *folder = *it;
    if (folder && !folder->noContent())
    {
      mCountLastUnread += folder->countUnread();
    }
  }
  // then check for new mails
  for (it = mMailCheckFolders.begin(); it != mMailCheckFolders.end(); it++)
  {
    KMFolder *folder = *it;
    if (folder && !folder->noContent())
    {
      KMFolderImap *imapFolder = static_cast<KMFolderImap*>(folder);
      if (imapFolder->getContentState() != KMFolderImap::imapInProgress)
      {
        // connect the result-signals for new-mail-notification
        mCountRemainChecks++;
        if (imapFolder->isSelected()) {
          connect(imapFolder, SIGNAL(folderComplete(KMFolderImap*, bool)),
              this, SLOT(postProcessNewMail(KMFolderImap*, bool)));
          imapFolder->getFolder();
        }
        else {
          connect(imapFolder, SIGNAL(numUnreadMsgsChanged(KMFolder*)),
              this, SLOT(postProcessNewMail(KMFolder*)));
          imapFolder->processNewMail(interactive);
        }
      }
    }
  }
}

//-----------------------------------------------------------------------------
void KMAcctImap::postProcessNewMail(KMFolderImap* folder, bool)
{
  disconnect(folder, SIGNAL(folderComplete(KMFolderImap*, bool)),
      this, SLOT(postProcessNewMail(KMFolderImap*, bool)));
  postProcessNewMail(static_cast<KMFolder*>(folder));
}

//-----------------------------------------------------------------------------
void KMAcctImap::slotUpdateFolderList()
{
  if (!mFolder || !mFolder->child() || !makeConnection())
    return; 
  QStringList strList;
  mMailCheckFolders.clear();
  kernel->imapFolderMgr()->createFolderList(&strList, &mMailCheckFolders,
    mFolder->child(), QString::null, false);
  // the new list
  QValueList<QGuardedPtr<KMFolder> > includedFolders;
  // check for excluded folders
  QValueList<QGuardedPtr<KMFolder> >::Iterator it;
  for (it = mMailCheckFolders.begin(); it != mMailCheckFolders.end(); it++)
  {
    KMFolderImap* folder = static_cast<KMFolderImap*>((KMFolder*)(*it));
    if (folder->includeInMailCheck())
      includedFolders.append(*it);
  }
  mMailCheckFolders = includedFolders;
}

//-----------------------------------------------------------------------------
void KMAcctImap::setPrefixHook() {
  if ( mFolder ) mFolder->setImapPath( prefix() );
}

#include "kmacctimap.moc"
