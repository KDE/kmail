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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmacctimap.h"
using KMail::SieveConfig;

#include "kmbroadcaststatus.h"
#include "kmfoldertree.h"
#include "kmfoldermgr.h"
#include "kmfolderimap.h"
#include "kmmainwin.h"
#include "imapjob.h"
using KMail::ImapJob;
#include "progressmanager.h"
using KMail::ProgressItem;
using KMail::ProgressManager;
#include <kio/scheduler.h>
#include <kio/slave.h>
#include <kmessagebox.h>
#include <kdebug.h>


//-----------------------------------------------------------------------------
KMAcctImap::KMAcctImap(KMAcctMgr* aOwner, const QString& aAccountName, uint id):
  KMail::ImapAccountBase(aOwner, aAccountName, id),
  mCountRemainChecks( 0 )
{
  mFolder = 0;
  mOpenFolders.setAutoDelete(true);
  connect(kmkernel->imapFolderMgr(), SIGNAL(changed()),
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
  ImapAccountBase::pseudoAssign( a );
}

//-----------------------------------------------------------------------------
void KMAcctImap::setImapFolder(KMFolderImap *aFolder)
{
  mFolder = aFolder;
  mFolder->setImapPath(mPrefix);
}


//-----------------------------------------------------------------------------

bool KMAcctImap::handleError( int errorCode, const QString &errorMsg, KIO::Job* job, const QString& context, bool abortSync )
{
  /* TODO check where to handle this one better. */
  if ( errorCode == KIO::ERR_DOES_NOT_EXIST ) {
    // folder is gone, so reload the folderlist
    if ( mFolder )
      mFolder->listDirectory();
    return true;
  }
  return ImapAccountBase::handleError( errorCode, errorMsg, job, context, abortSync );
}


//-----------------------------------------------------------------------------
void KMAcctImap::killAllJobs( bool disconnectSlave )
{
  QMap<KIO::Job*, jobData>::Iterator it = mapJobData.begin();
  for ( ; it != mapJobData.end(); ++it)
  {
    QPtrList<KMMessage> msgList = (*it).msgList;
    QPtrList<KMMessage>::Iterator it2 = msgList.begin();
    for ( ; it2 != msgList.end(); ++it2 ) {
       KMMessage *msg = *it2;
       if ( msg->transferInProgress() ) {
          kdDebug(5006) << "KMAcctImap::killAllJobs - resetting mail" << endl;
          msg->setTransferInProgress( false );
       }
    }
    if ((*it).parent)
    {
      // clear folder state
      KMFolderImap *fld = static_cast<KMFolderImap*>((*it).parent->storage());
      fld->setCheckingValidity(false);
      fld->setContentState(KMFolderImap::imapNoInformation);
      fld->setSubfolderState(KMFolderImap::imapNoInformation);
      fld->sendFolderComplete(FALSE);
      fld->removeJobs();
    }
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
  if (!msg) return;
  QPtrListIterator<ImapJob> it( mJobList );
  while ( it.current() )
  {
    ImapJob *job = it.current();
    ++it;
    if ( job->msgList().findRef( msg ) != -1 )
    {
      if ( job->mJob )
        removeJob( job->mJob );
      mJobList.remove( job );
      job->kill();
    }
  }
}

//-----------------------------------------------------------------------------
void KMAcctImap::ignoreJobsForFolder( KMFolder* folder )
{
  QPtrListIterator<ImapJob> it( mJobList );
  while ( it.current() )
  {
    ImapJob *job = it.current();
    ++it;
    if ( !job->msgList().isEmpty() && job->msgList().first()->parent() == folder )
    {
      if ( job->mJob )
        removeJob( job->mJob );
      mJobList.remove( job );
      job->kill();
    }
  }
}

//-----------------------------------------------------------------------------
void KMAcctImap::removeSlaveJobsForFolder( KMFolder* folder )
{
  // Make sure the folder is not referenced in any kio slave jobs
  QMap<KIO::Job*, jobData>::Iterator it = mapJobData.begin();
  while ( it != mapJobData.end() ) {
     QMap<KIO::Job*, jobData>::Iterator i = it;
     it++;
     if ( (*i).parent ) {
        if ( (*i).parent == folder ) {
           mapJobData.remove(i);
        }
     }
  }
}

//-----------------------------------------------------------------------------
void KMAcctImap::cancelMailCheck()
{
  // Make list of folders to reset, like in killAllJobs
  QValueList<KMFolderImap*> folderList;
  QMap<KIO::Job*, jobData>::Iterator it = mapJobData.begin();
  for (; it != mapJobData.end(); ++it) {
    if ( (*it).cancellable && (*it).parent ) {
      folderList << static_cast<KMFolderImap*>((*it).parent->storage());
    }
  }
  // Kill jobs
  // FIXME
  // ImapAccountBase::cancelMailCheck();
  killAllJobs( true );
  // emit folderComplete, this is important for
  // KMAccount::checkingMail() to be reset, in case we restart checking mail later.
  for( QValueList<KMFolderImap*>::Iterator it = folderList.begin(); it != folderList.end(); ++it ) {
    KMFolderImap *fld = *it;
    fld->sendFolderComplete(FALSE);
  }
}

//-----------------------------------------------------------------------------
void KMAcctImap::processNewMail(bool interactive)
{
  if (!mFolder || !mFolder->folder() || !mFolder->folder()->child() ||
      makeConnection() == ImapAccountBase::Error)
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
  // Ok, we're really checking, get a progress item;
  mMailCheckProgressItem =
    ProgressManager::createProgressItem( "MailCheckAccount" + name(),
          i18n("Checking account: " ) + name() );

  mMailCheckProgressItem->setTotalItems( mMailCheckFolders.count() );
  connect ( mMailCheckProgressItem,
            SIGNAL( progressItemCanceled( ProgressItem*) ),
            this,
            SLOT( slotMailCheckCanceled() ) );

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
  bool gotError = false;
  // then check for new mails
  for (it = mMailCheckFolders.begin(); it != mMailCheckFolders.end(); it++)
  {
    KMFolder *folder = *it;
    if (folder && !folder->noContent())
    {
      KMFolderImap *imapFolder = static_cast<KMFolderImap*>(folder->storage());
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
          bool ok = imapFolder->processNewMail(interactive);
          if (!ok)
          {
            // there was an error so cancel
            mCountRemainChecks--;
            gotError = true;
          }
        }
      }
    }
  } // end for
  if ( gotError )
    slotUpdateFolderList();
}

//-----------------------------------------------------------------------------
void KMAcctImap::postProcessNewMail(KMFolderImap* folder, bool)
{
  disconnect(folder, SIGNAL(folderComplete(KMFolderImap*, bool)),
      this, SLOT(postProcessNewMail(KMFolderImap*, bool)));
  postProcessNewMail(static_cast<KMFolder*>(folder->folder()));
}

void KMAcctImap::postProcessNewMail( KMFolder * folder ) {

  disconnect( folder->storage(), SIGNAL(numUnreadMsgsChanged(KMFolder*)),
              this, SLOT(postProcessNewMail(KMFolder*)) );

  if ( mMailCheckProgressItem ) {
    mMailCheckProgressItem->incCompletedItems();
    mMailCheckProgressItem->updateProgress();
    mMailCheckProgressItem->setStatus( folder->prettyURL() + i18n(" completed") );
  }
  mCountRemainChecks--;

  // count the unread messages
  mCountUnread += folder->countUnread();
  if (mCountRemainChecks == 0)
  {
    // all checks are done
    ImapAccountBase::postProcessNewMail();
  }
}

//-----------------------------------------------------------------------------
void KMAcctImap::slotUpdateFolderList()
{
  if (!mFolder || !mFolder->folder() || !mFolder->folder()->child() ||
      makeConnection() != ImapAccountBase::Connected)
    return;
  QStringList strList;
  mMailCheckFolders.clear();
  kmkernel->imapFolderMgr()->createFolderList(&strList, &mMailCheckFolders,
    mFolder->folder()->child(), QString::null, false);
  // the new list
  QValueList<QGuardedPtr<KMFolder> > includedFolders;
  // check for excluded folders
  QValueList<QGuardedPtr<KMFolder> >::Iterator it;
  for (it = mMailCheckFolders.begin(); it != mMailCheckFolders.end(); it++)
  {
    KMFolderImap* folder = static_cast<KMFolderImap*>(((KMFolder*)(*it))->storage());
    if (folder->includeInMailCheck())
      includedFolders.append(*it);
  }
  mMailCheckFolders = includedFolders;
}

//-----------------------------------------------------------------------------
void KMAcctImap::listDirectory(QString path, ListType subscription,
    bool secondStep, KMFolder* parent, bool reset)
{
  ImapAccountBase::listDirectory( path, subscription, secondStep, parent, reset );
}

//-----------------------------------------------------------------------------
void KMAcctImap::listDirectory()
{
  mFolder->listDirectory();
}

//-----------------------------------------------------------------------------
void KMAcctImap::setPrefixHook() {
  if ( mFolder ) mFolder->setImapPath( prefix() );
}

//-----------------------------------------------------------------------------
void KMAcctImap::readConfig(KConfig& config)
{
  ImapAccountBase::readConfig( config );
  if ( checkExclude() ) {
    disconnect(kmkernel->imapFolderMgr(), SIGNAL(changed()),
        this, SLOT(slotUpdateFolderList()));
  }
}

void KMAcctImap::slotMailCheckCanceled()
{
  mMailCheckProgressItem->setComplete();
  cancelMailCheck();
}

#include "kmacctimap.moc"
