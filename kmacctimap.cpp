/**
 * kmacctimap.cpp
 *
 * Copyright (c) 2000-2002 Michael Haeckel <haeckel@kde.org>
 *
 * This file is based on popaccount.cpp by Don Sanders
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#include "kmacctimap.h"

using KMail::SieveConfig;

#include "kmmessage.h"
#include "broadcaststatus.h"
using KPIM::BroadcastStatus;
#include "kmfoldertree.h"
#include "kmfoldermgr.h"
#include "kmfolderimap.h"
#include "kmmainwin.h"
#include "kmmsgdict.h"
#include "kmfilter.h"
#include "kmfiltermgr.h"
#include "folderstorage.h"
#include "imapjob.h"
#include "actionscheduler.h"
using KMail::ActionScheduler;
using KMail::ImapJob;
using KMail::ImapAccountBase;
#include "progressmanager.h"
using KPIM::ProgressItem;
using KPIM::ProgressManager;

#include <kio/scheduler.h>
#include <kio/slave.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <qtextdocument.h>

#include <QList>

#include <errno.h>

//-----------------------------------------------------------------------------
KMAcctImap::KMAcctImap(AccountManager* aOwner, const QString& aAccountName, uint id):
  KMail::ImapAccountBase(aOwner, aAccountName, id),
  mCountRemainChecks( 0 )
{
  mFolder = 0;
  mScheduler = 0;
  mNoopTimer.start( 60000 ); // // send a noop every minute
  connect(kmkernel->imapFolderMgr(), SIGNAL(changed()),
      this, SLOT(slotUpdateFolderList()));
  mErrorTimer.setSingleShot( true );
  connect(&mErrorTimer, SIGNAL(timeout()), SLOT(slotResetConnectionError()));

  QString serNumUri = KStandardDirs::locateLocal( "data", "kmail/unfiltered." +
				   QString("%1").arg(KAccount::id()) );
  KConfig config( serNumUri );
  QStringList serNums = config.readEntry( "unfiltered" , QStringList() );

  for ( QStringList::ConstIterator it = serNums.begin();
	it != serNums.end(); ++it ) {
      mFilterSerNums.append( (*it).toUInt() );
      mFilterSerNumsToSave.insert( *it, 1 );
    }
}


//-----------------------------------------------------------------------------
KMAcctImap::~KMAcctImap()
{
  killAllJobs( true );

  QString serNumUri = KStandardDirs::locateLocal( "data", "kmail/unfiltered." +
				   QString("%1").arg(KAccount::id()) );
  KConfig config( serNumUri );
  QStringList serNums;
  QHashIterator<QString, int> it( mFilterSerNumsToSave );
  while ( it.hasNext() ) {
    serNums.append( it.key() );
  }
  config.writeEntry( "unfiltered", serNums );
  qDeleteAll( mOpenFolders );
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
  mFolder->setImapPath( "/" );
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
  QMap<KJob*, jobData>::Iterator it = mapJobData.begin();
  for ( ; it != mapJobData.end(); ++it)
  {
    QList<KMMessage*> msgList = (*it).msgList;
    QList<KMMessage*>::Iterator it2 = msgList.begin();
    for ( ; it2 != msgList.end(); ++it2 ) {
       KMMessage *msg = *it2;
       if ( msg->transferInProgress() ) {
          kDebug(5006) << "KMAcctImap::killAllJobs - resetting mail" << endl;
          msg->setTransferInProgress( false );
       }
    }
    if ((*it).parent)
    {
      // clear folder state
      KMFolderImap *fld = static_cast<KMFolderImap*>((*it).parent->storage());
      fld->setCheckingValidity(false);
      fld->quiet(false);
      fld->setContentState(KMFolderImap::imapNoInformation);
      fld->setSubfolderState(KMFolderImap::imapNoInformation);
      fld->sendFolderComplete(false);
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
  // KMAccount::deleteFolderJobs(); doesn't work here always, it deletes jobs from
  // its own mJobList instead of our mJobList...
  KMAccount::deleteFolderJobs();
  foreach ( ImapJob *job, mJobList ) {
    job->kill();
  }
  mJobList.clear();
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
  QList<ImapJob*>::const_iterator it;
  for ( it = mJobList.begin(); (*it) && it != mJobList.constEnd(); ++it )
  {
    ImapJob *job = (*it);
    if ( job->msgList().first() == msg )
    {
      job->kill();
    }
    ++it;
  }
}

//-----------------------------------------------------------------------------
void KMAcctImap::ignoreJobsForFolder( KMFolder* folder )
{
  QList<ImapJob*>::const_iterator it;
  for ( it = mJobList.begin(); (*it) && it != mJobList.constEnd(); ++it )
  {
    ImapJob *job = (*it);
    if ( !job->msgList().isEmpty() && job->msgList().first()->parent() == folder )
    {
      job->kill();
    }
    ++it;
  }
}

//-----------------------------------------------------------------------------
void KMAcctImap::removeSlaveJobsForFolder( KMFolder* folder )
{
  // Make sure the folder is not referenced in any kio slave jobs
  QMap<KJob*, jobData>::Iterator it = mapJobData.begin();
  while ( it != mapJobData.end() ) {
     QMap<KJob*, jobData>::Iterator i = it;
     it++;
     if ( (*i).parent ) {
        if ( (*i).parent == folder ) {
           mapJobData.erase(i);
        }
     }
  }
}

//-----------------------------------------------------------------------------
void KMAcctImap::cancelMailCheck()
{
  // Make list of folders to reset, like in killAllJobs
  QList<KMFolderImap*> folderList;
  QMap<KJob*, jobData>::Iterator it = mapJobData.begin();
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
  for( QList<KMFolderImap*>::Iterator it = folderList.begin(); it != folderList.end(); ++it ) {
    KMFolderImap *fld = *it;
    fld->sendFolderComplete(false);
  }
}

//-----------------------------------------------------------------------------
void KMAcctImap::processNewMail(bool interactive)
{
  kDebug(5006) << "processNewMail " << mCheckingSingleFolder << ",status="<<makeConnection()<<endl;
  if ( !mFolder || !mFolder->folder() || !mFolder->folder()->child() ||
       makeConnection() == ImapAccountBase::Error ) {
    // checks for mCountRemainChecks
    checkDone( false, CheckError );
    mCountRemainChecks = 0;
    mCheckingSingleFolder = false;
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
        i18n( "Checking account: %1", Qt::escape( name() ) ),
        QString(), // status
        true, // can be canceled
        useSSL() || useTLS() );

  mMailCheckProgressItem->setTotalItems( mMailCheckFolders.count() );
  connect ( mMailCheckProgressItem,
            SIGNAL( progressItemCanceled( KPIM::ProgressItem*) ),
            this,
            SLOT( slotMailCheckCanceled() ) );

  QList<QPointer<KMFolder> >::Iterator it;
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
  for ( it = mMailCheckFolders.begin(); it != mMailCheckFolders.end(); ++it ) {
    KMFolder *folder = *it;
    if ( folder && !folder->noContent() ) {
      KMFolderImap *imapFolder = static_cast<KMFolderImap*>(folder->storage());
      if ( imapFolder->getContentState() != KMFolderImap::imapListingInProgress
        && imapFolder->getContentState() != KMFolderImap::imapDownloadInProgress )
      {
        // connect the result-signals for new-mail-notification
        mCountRemainChecks++;

        if ( imapFolder->isSelected() ) {
          connect(imapFolder, SIGNAL(folderComplete(KMFolderImap*, bool)),
              this, SLOT(postProcessNewMail(KMFolderImap*, bool)));
          imapFolder->getFolder();
        } else if ( kmkernel->filterMgr()->atLeastOneIncomingFilterAppliesTo( id() ) &&
                    imapFolder->folder()->isSystemFolder() &&
                    imapFolder->imapPath() == "/INBOX/" ) {
          // will be closed in the folderSelected slot
          imapFolder->open( "acctimap" );
          // first get new headers before we select the folder
          imapFolder->setSelected( true );
          connect( imapFolder, SIGNAL( folderComplete( KMFolderImap*, bool ) ),
                   this, SLOT( slotFolderSelected( KMFolderImap*, bool) ) );
          imapFolder->getFolder();
        } else {
          connect(imapFolder, SIGNAL(numUnreadMsgsChanged(KMFolder*)),
                  this, SLOT(postProcessNewMail(KMFolder*)));
          bool ok = imapFolder->processNewMail(interactive); // this removes the local kmfolderimap if its imapPath is somehow empty, and removing it calls createFolderList, invalidating mMailCheckFolders, and causing a crash
          if ( !ok ) {
            // there was an error so cancel
            mCountRemainChecks--;
            gotError = true;
            if ( mMailCheckProgressItem ) {
              mMailCheckProgressItem->incCompletedItems();
              mMailCheckProgressItem->updateProgress();
            }
            // since the list of folders might have been updated at this point, mMailCheckFolders may be invalid, so break
            break;
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
    mMailCheckProgressItem->setStatus( folder->prettyUrl() + i18n(" completed") );
  }
  mCountRemainChecks--;

  // count the unread messages
  const QString folderId = folder->idString();
  int newInFolder = folder->countUnread();
  if ( mUnreadBeforeCheck.contains( folderId )  )
    newInFolder -= mUnreadBeforeCheck[folderId];
  if ( newInFolder > 0 ) {
    addToNewInFolder( folderId, newInFolder );
    mCountUnread += newInFolder;
  }

  // Filter messages
  QListIterator<quint32> filterIt( mFilterSerNums );
  QList<quint32> inTransit;

  if (ActionScheduler::isEnabled() ||
      kmkernel->filterMgr()->atLeastOneOnlineImapFolderTarget()) {
    KMFilterMgr::FilterSet set = KMFilterMgr::Inbound;
    QList<KMFilter*> filters = kmkernel->filterMgr()->filters();
    if (!mScheduler) {
	mScheduler = new KMail::ActionScheduler( set, filters );
	mScheduler->setAccountId( id() );
	connect( mScheduler, SIGNAL(filtered(quint32)), this, SLOT(slotFiltered(quint32)) );
    } else {
	mScheduler->setFilterList( filters );
    }
  }

  while ( filterIt.hasNext() ) {
    int idx = -1;
    KMFolder *folder = 0;
    KMMessage *msg = 0;
    quint32 sernum = filterIt.next();
    KMMsgDict::instance()->getLocation( sernum, &folder, &idx );
    // It's possible that the message has been deleted or moved into a
    // different folder, or that the serNum is stale
    if ( !folder ) {
      mFilterSerNumsToSave.remove( QString( "%1" ).arg( sernum ) );
      continue;
    }

    KMFolderImap *imapFolder = dynamic_cast<KMFolderImap*>(folder->storage());
    if (!imapFolder ||
	!imapFolder->folder()->isSystemFolder() ||
        !(imapFolder->imapPath() == "/INBOX/") ) { // sanity checking
      mFilterSerNumsToSave.remove( QString( "%1" ).arg( sernum ) );
      continue;
    }

    if (idx != -1) {

      msg = folder->getMsg( idx );
      if (!msg) { // sanity checking
        mFilterSerNumsToSave.remove( QString( "%1" ).arg( sernum ) );
        continue;
      }

      if (ActionScheduler::isEnabled() ||
	  kmkernel->filterMgr()->atLeastOneOnlineImapFolderTarget()) {
	mScheduler->execFilters( msg );
      } else {
	if (msg->transferInProgress()) {
	  inTransit.append( sernum );
	  continue;
	}
	msg->setTransferInProgress(true);
	if ( !msg->isComplete() ) {
	  FolderJob *job = folder->createJob(msg);
	  connect(job, SIGNAL(messageRetrieved(KMMessage*)),
		  SLOT(slotFilterMsg(KMMessage*)));
	  job->start();
	} else {
	  mFilterSerNumsToSave.remove( QString( "%1" ).arg( sernum ) );
	  if (slotFilterMsg(msg) == 2) break;
	}
      }
    }
  }
  mFilterSerNums = inTransit;

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
void KMAcctImap::slotFiltered(quint32 serNum)
{
    mFilterSerNumsToSave.remove( QString( "%1" ).arg( serNum ) );
}

//-----------------------------------------------------------------------------
void KMAcctImap::slotUpdateFolderList()
{
  if ( !mFolder || !mFolder->folder() || !mFolder->folder()->child() )
  {
    kWarning(5006) << "KMAcctImap::slotUpdateFolderList return" << endl;
    return;
  }
  QStringList strList;
  mMailCheckFolders.clear();
  kmkernel->imapFolderMgr()->createFolderList(&strList, &mMailCheckFolders,
    mFolder->folder()->child(), QString(), false);
  // the new list
  QList<QPointer<KMFolder> > includedFolders;
  // check for excluded folders
  QList<QPointer<KMFolder> >::Iterator it;
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
void KMAcctImap::readConfig(KConfigGroup& config)
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
FolderStorage* KMAcctImap::rootFolder() const
{
  return mFolder;
}

ImapAccountBase::ConnectionState KMAcctImap::makeConnection()
{
  if ( mSlaveConnectionError )
  {
    mErrorTimer.start( 100 ); // Clear error flag
    return Error;
  }
  return ImapAccountBase::makeConnection();
}

void KMAcctImap::slotResetConnectionError()
{
  mSlaveConnectionError = false;
  kDebug(5006) << k_funcinfo << endl;
}

void KMAcctImap::slotFolderSelected( KMFolderImap* folder, bool )
{
  folder->setSelected( false );
  disconnect( folder, SIGNAL( folderComplete( KMFolderImap*, bool ) ),
	      this, SLOT( slotFolderSelected( KMFolderImap*, bool) ) );
  postProcessNewMail( static_cast<KMFolder*>(folder->folder()) );
  folder->close( "acctimap" );
}

void KMAcctImap::execFilters(quint32 serNum)
{
  if ( !kmkernel->filterMgr()->atLeastOneFilterAppliesTo( id() ) ) return;
  if ( mFilterSerNums.contains( serNum ) )
      return;

  mFilterSerNums.append( serNum );
  mFilterSerNumsToSave.insert( QString( "%1" ).arg( serNum ), 1 );
}

int KMAcctImap::slotFilterMsg( KMMessage *msg )
{
  if ( !msg ) {
    // messageRetrieved(0) is always possible
    return -1;
  }
  msg->setTransferInProgress(false);
  quint32 serNum = msg->getMsgSerNum();
  if ( serNum )
    mFilterSerNumsToSave.remove( QString( "%1" ).arg( serNum ) );

  int filterResult = kmkernel->filterMgr()->process(msg,
						    KMFilterMgr::Inbound,
						    true,
						    id() );
  if (filterResult == 2) {
    // something went horribly wrong (out of space?)
    kmkernel->emergencyExit( i18n("Unable to process messages: " ) + QString::fromLocal8Bit(strerror(errno)));
    return 2;
  }
  if (msg->parent()) { // unGet this msg
    int idx = -1;
    KMFolder * p = 0;
    KMMsgDict::instance()->getLocation( msg, &p, &idx );
    assert( p == msg->parent() ); assert( idx >= 0 );
    p->unGetMsg( idx );
  }

  return filterResult;
}

#include "kmacctimap.moc"
