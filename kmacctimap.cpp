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

void KMAcctImap::setPrefixHook() {
  if ( mFolder ) mFolder->setImapPath( prefix() );
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
void KMAcctImap::initJobData(jobData &jd)
{
  jd.total = 1;
  jd.done = 0;
  jd.parent = 0;
  jd.quiet = FALSE;
  jd.inboxOnly = FALSE;
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
void KMAcctImap::displayProgress()
{
  if (mProgressEnabled == mapJobData.isEmpty())
  {
    mProgressEnabled = !mapJobData.isEmpty();
    KMBroadcastStatus::instance()->setStatusProgressEnable( "I" + mName,
      mProgressEnabled );
    if (!mProgressEnabled)
    {
      QPtrListIterator<QGuardedPtr<KMFolder> > it(mOpenFolders);
      for ( it.toFirst() ; it.current() ; ++it )
        if (*it) (*(*it))->close();
      mOpenFolders.clear();
    }
  }
  mIdle = FALSE;
  if (mapJobData.isEmpty())
    mIdleTimer.start(15000);
  else
    mIdleTimer.stop();
  int total = 0, done = 0;
  for (QMap<KIO::Job*, jobData>::Iterator it = mapJobData.begin();
    it != mapJobData.end(); ++it)
  {
    total += (*it).total;
    done += (*it).done;
  }
  if (total == 0)
  {
    mTotal = 0;
    return;
  }
  if (total > mTotal) mTotal = total;
  done += mTotal - total;
  KMBroadcastStatus::instance()->setStatusProgressPercent( "I" + mName,
    100*done / mTotal );
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
      KMFolderImap *fld = (*it).parent;
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
    mCountRemainChecks = 0;
    emit finishedCheck(false);
  }
  displayProgress();

  if ( disconnectSlave && slave() ) {
    KIO::Scheduler::disconnectSlave( slave() );
    mSlave = 0;
  }
}

//-----------------------------------------------------------------------------
void KMAcctImap::ignoreJobsForMessage( KMMessage*  )
{
    /* TODO: doesn't yet compile because kmfolderimap.h needs to be merged (coolo)
  KMImapJob *job;
  for (KMFolderJob *it = mJobList.first(); it;
       it = mJobList.next()) {
    if ((*it).msgList().first() == msg) {
      job = dynamic_cast<KMImapJob*>(it);
      mapJobData.remove( job->mJob );
      mJobList.remove( job );
      delete job;
      break;
    }
  }
    */
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
  emit newMailsProcessed(-1);
  if (!mFolder || !mFolder->child() ||
      !makeConnection())
  {
    mCountRemainChecks = 0;
    emit finishedCheck(false);
    return;
  }
  QStringList strList;
  QValueList<QGuardedPtr<KMFolder> > folderList;
  kernel->imapFolderMgr()->createFolderList(&strList, &folderList,
    mFolder->child(), QString::null, false);
  QValueList<QGuardedPtr<KMFolder> >::Iterator it;
  // first get the current count of unread-messages
  mCountRemainChecks = 0;
  mCountLastUnread = 0;
  for (it = folderList.begin(); it != folderList.end(); ++it)
  {
    KMFolder *folder = *it;
    if (folder && !folder->noContent())
    {
      mCountLastUnread += folder->countUnread();
    }
  }
  // then check for new mails
  for (it = folderList.begin(); it != folderList.end(); ++it)
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

#include "kmacctimap.moc"
