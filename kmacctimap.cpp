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

#include "broadcaststatus.h"
using KPIM::BroadcastStatus;
#include "kmfoldertree.h"
#include "kmfoldermgr.h"
#include "kmfolderimap.h"
#include "kmmainwin.h"
#include "folderstorage.h"
#include "imapjob.h"
using KMail::ImapJob;
using KMail::ImapAccountBase;
#include "progressmanager.h"
using KPIM::ProgressItem;
using KPIM::ProgressManager;
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
  mNoopTimer.start( 60000 ); // // send a noop every minute
  mOpenFolders.setAutoDelete(true);
  connect(kmkernel->imapFolderMgr(), SIGNAL(changed()),
      this, SLOT(slotUpdateFolderList()));
  connect(&mErrorTimer, SIGNAL(timeout()), SLOT(slotResetConnectionError()));
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
    if ( (*it).progressItem )
    {
      (*it).progressItem->setComplete();
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
    checkDone( false, CheckOK ); // returned 0 new messages
    mCountRemainChecks = 0;
  }
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
    if ( job->msgList().first() == msg )
    {
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
  kdDebug() << "processNewMail " << mCheckingSingleFolder << ",status="<<makeConnection()<<endl;
  if (!mFolder || !mFolder->folder() || !mFolder->folder()->child() ||
      makeConnection() == ImapAccountBase::Error)
  {
    mCountRemainChecks = 0;
    mCheckingSingleFolder = false;
    checkDone( false, CheckError );
    return;
  }
  // if necessary then initialize the list of folders which should be checked
  if( mMailCheckFolders.isEmpty() )
  {
    slotUpdateFolderList();
    // if no folders should be checked then the check is finished
    if( mMailCheckFolders.isEmpty() )
    {
      checkDone( false, CheckOK );
      mCheckingSingleFolder = false;
      return;
    }
  }
  // Ok, we're really checking, get a progress item;
  Q_ASSERT( !mMailCheckProgressItem );
  mMailCheckProgressItem =
    ProgressManager::createProgressItem(
        "MailCheckAccount" + name(),
        i18n("Checking account: " ) + name(),
        QString::null, // status
        true, // can be canceled
        useSSL() || useTLS() );

  mMailCheckProgressItem->setTotalItems( mMailCheckFolders.count() );
  connect ( mMailCheckProgressItem,
            SIGNAL( progressItemCanceled( KPIM::ProgressItem*) ),
            this,
            SLOT( slotMailCheckCanceled() ) );

  QValueList<QGuardedPtr<KMFolder> >::Iterator it;
  // first get the current count of unread-messages
  mCountRemainChecks = 0;
  mCountUnread = 0;
  mUnreadBeforeCheck.clear();
  for (it = mMailCheckFolders.begin(); it != mMailCheckFolders.end(); ++it)
  {
    KMFolder *folder = *it;
    if (folder && !folder->noContent())
    {
      mUnreadBeforeCheck[folder->idString()] = folder->countUnread();
    }
  }
  bool gotError = false;
  // then check for new mails
  for (it = mMailCheckFolders.begin(); it != mMailCheckFolders.end(); ++it)
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
            if ( mMailCheckProgressItem ) {
              mMailCheckProgressItem->incCompletedItems();
              mMailCheckProgressItem->updateProgress();
            }
          }
        }
      }
    }
  } // end for
  if ( gotError )
    slotUpdateFolderList();
  // for the case the account is down and all folders report errors
  if ( mCountRemainChecks == 0 )
  {
    mCountLastUnread = 0; // => mCountUnread - mCountLastUnread == new count
    ImapAccountBase::postProcessNewMail();
    mUnreadBeforeCheck.clear();
    mCheckingSingleFolder = false;
  }
}

//-----------------------------------------------------------------------------
void KMAcctImap::postProcessNewMail(KMFolderImap* folder, bool)
{
  disconnect(folder, SIGNAL(folderComplete(KMFolderImap*, bool)),
      this, SLOT(postProcessNewMail(KMFolderImap*, bool)));
  postProcessNewMail(static_cast<KMFolder*>(folder->folder()));
}

void KMAcctImap::postProcessNewMail( KMFolder * folder ) 
{
  disconnect( folder->storage(), SIGNAL(numUnreadMsgsChanged(KMFolder*)),
              this, SLOT(postProcessNewMail(KMFolder*)) );

  if ( mMailCheckProgressItem ) {
    mMailCheckProgressItem->incCompletedItems();
    mMailCheckProgressItem->updateProgress();
    mMailCheckProgressItem->setStatus( folder->prettyURL() + i18n(" completed") );
  }
  mCountRemainChecks--;

  // count the unread messages
  const QString folderId = folder->idString();
  int newInFolder = folder->countUnread();
  if ( mUnreadBeforeCheck.find( folderId ) != mUnreadBeforeCheck.end() )
    newInFolder -= mUnreadBeforeCheck[folderId];
  if ( newInFolder > 0 ) {
    addToNewInFolder( folderId, newInFolder );
    mCountUnread += newInFolder;
  }
  if (mCountRemainChecks == 0)
  {
    // all checks are done
    mCountLastUnread = 0; // => mCountUnread - mCountLastUnread == new count
    // when we check only one folder (=selected) and we have new mails
    // then do not display a summary as the normal status message is better
    bool showStatus = ( mCheckingSingleFolder && mCountUnread > 0 ) ? false : true;
    ImapAccountBase::postProcessNewMail( showStatus );
    mUnreadBeforeCheck.clear();
    mCheckingSingleFolder = false;
  }
}

//-----------------------------------------------------------------------------
void KMAcctImap::slotUpdateFolderList()
{
  if ( !mFolder || !mFolder->folder() || !mFolder->folder()->child() )
  {
    kdWarning(5006) << "KMAcctImap::slotUpdateFolderList return" << endl;
    return;
  }
  QStringList strList;
  mMailCheckFolders.clear();
  kmkernel->imapFolderMgr()->createFolderList(&strList, &mMailCheckFolders,
    mFolder->folder()->child(), QString::null, false);
  // the new list
  QValueList<QGuardedPtr<KMFolder> > includedFolders;
  // check for excluded folders
  QValueList<QGuardedPtr<KMFolder> >::Iterator it;
  for (it = mMailCheckFolders.begin(); it != mMailCheckFolders.end(); ++it)
  {
    KMFolderImap* folder = static_cast<KMFolderImap*>(((KMFolder*)(*it))->storage());
    if (folder->includeInMailCheck())
      includedFolders.append(*it);
  }
  mMailCheckFolders = includedFolders;
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
}

//-----------------------------------------------------------------------------
void KMAcctImap::slotMailCheckCanceled()
{
  if( mMailCheckProgressItem )
    mMailCheckProgressItem->setComplete();
  cancelMailCheck();
}

//-----------------------------------------------------------------------------
FolderStorage* const KMAcctImap::rootFolder() const
{
  return mFolder;
}

ImapAccountBase::ConnectionState KMAcctImap::makeConnection() 
{
  if ( mSlaveConnectionError )
  {
    mErrorTimer.start(100, true); // Clear error flag
    return Error;
  }
  return ImapAccountBase::makeConnection();
}

void KMAcctImap::slotResetConnectionError()
{
  mSlaveConnectionError = false;
  kdDebug(5006) << k_funcinfo << endl;
}

#include "kmacctimap.moc"
