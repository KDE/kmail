/**
 *  kmacctcachedimap.cpp
 *
 *  Copyright (c) 2002-2004 Bo Thorsen <bo@klaralvdalens-datakonsult.se>
 *  Copyright (c) 2002-2003 Steffen Hansen <steffen@klaralvdalens-datakonsult.se>
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
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
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
#include "kmacctmgr.h"

#include <kio/passdlg.h>
#include <kio/scheduler.h>
#include <kio/slave.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kapplication.h>
#include <kconfig.h>


KMAcctCachedImap::KMAcctCachedImap( KMAcctMgr* aOwner,
				    const QString& aAccountName, uint id )
  : KMail::ImapAccountBase( aOwner, aAccountName, id ), mFolder( 0 ),
    mProgressDialogEnabled( true )
{
  // Never EVER set this for the cached IMAP account
  mAutoExpunge = false;
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
void KMAcctCachedImap::displayProgress()
{
   mIdle = false;
   mIdleTimer.start( 15000 );
  /*
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

  //100*done / mTotal );
  */
}

//-----------------------------------------------------------------------------
void KMAcctCachedImap::killAllJobs( bool disconnectSlave )
{
  //kdDebug(5006) << "killAllJobs: disconnectSlave=" << disconnectSlave << "  " << mapJobData.count() << " jobs in map." << endl;
  // Make list of folders to reset. This must be done last, since folderComplete
  // can trigger the next queued mail check already.
  QValueList<KMFolderCachedImap*> folderList;
  QMap<KIO::Job*, jobData>::Iterator it = mapJobData.begin();
  for (; it != mapJobData.end(); ++it) {
    if ((*it).parent)
      folderList << static_cast<KMFolderCachedImap*>((*it).parent->storage());
  }
  mapJobData.clear();

  // Clear the joblist. Make SURE to stop the job emitting "finished"
  for( QPtrListIterator<CachedImapJob> it( mJobList ); it.current(); ++it )
    it.current()->setPassiveDestructor( true );
  KMAccount::deleteFolderJobs();
  displayProgress();

  if ( disconnectSlave && slave() ) {
    KIO::Scheduler::disconnectSlave( slave() );
    mSlave = 0;
  }
  for( QValueList<KMFolderCachedImap*>::Iterator it = folderList.begin(); it != folderList.end(); ++it ) {
    KMFolderCachedImap *fld = *it;
    fld->resetSyncState();
    fld->setContentState(KMFolderCachedImap::imapNoInformation);
    fld->setSubfolderState(KMFolderCachedImap::imapNoInformation);
    fld->sendFolderComplete(FALSE);
  }
}

//-----------------------------------------------------------------------------
void KMAcctCachedImap::cancelMailCheck()
{
  // Make list of folders to reset, like in killAllJobs
  QValueList<KMFolderCachedImap*> folderList;
  QMap<KIO::Job*, jobData>::Iterator it = mapJobData.begin();
  for (; it != mapJobData.end(); ++it) {
    if ( (*it).cancellable && (*it).parent )
      folderList << static_cast<KMFolderCachedImap*>((*it).parent->storage());
  }
  // Kill jobs
  ImapAccountBase::cancelMailCheck();
  // Reset sync states and emit folderComplete, this is important for
  // KMAccount::checkingMail() to be reset, in case we restart checking mail later.
  for( QValueList<KMFolderCachedImap*>::Iterator it = folderList.begin(); it != folderList.end(); ++it ) {
    KMFolderCachedImap *fld = *it;
    fld->resetSyncState();
    fld->setContentState(KMFolderCachedImap::imapNoInformation);
    fld->setSubfolderState(KMFolderCachedImap::imapNoInformation);
    fld->sendFolderComplete(FALSE);
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

// Reimplemented from ImapAccountBase because we only check one folder at a time
void KMAcctCachedImap::slotCheckQueuedFolders()
{
    disconnect (this, SIGNAL(finishedCheck(bool)),
          this, SLOT(slotCheckQueuedFolders()));

    mMailCheckFolders.clear();
    mMailCheckFolders.append( mFoldersQueuedForChecking.front() );
    mFoldersQueuedForChecking.pop_front();
    kmkernel->acctMgr()->singleCheckMail(this, true);
    mMailCheckFolders.clear();
}

void KMAcctCachedImap::processNewMail( bool interactive )
{
  if ( mMailCheckFolders.isEmpty() )
   processNewMail( mFolder, interactive, true );
  else {
    KMFolder* f = mMailCheckFolders.front();
    mMailCheckFolders.pop_front();
    processNewMail( static_cast<KMFolderCachedImap *>( f->storage() ), interactive, false );
  }
}

void KMAcctCachedImap::processNewMail( KMFolderCachedImap* folder,
				       bool interactive,
                                       bool recurse )
{
  // This should never be set for a cached IMAP account
  mAutoExpunge = false;
  mCountLastUnread = 0;

  if( interactive && isProgressDialogEnabled() ) {
    imapProgressDialog()->clear();
    imapProgressDialog()->show();
    imapProgressDialog()->raise();
  }

  folder->setAccount(this);
  connect(folder, SIGNAL(folderComplete(KMFolderCachedImap*, bool)),
	  this, SLOT(postProcessNewMail(KMFolderCachedImap*, bool)));
  folder->serverSync( interactive && isProgressDialogEnabled(), recurse );
}

void KMAcctCachedImap::postProcessNewMail( KMFolderCachedImap* folder, bool )
{
  disconnect(folder, SIGNAL(folderComplete(KMFolderCachedImap*, bool)),
             this, SLOT(postProcessNewMail(KMFolderCachedImap*, bool)));

  // We remove everything from the deleted folders list after a sync, unconditionally.
  // Even if it fails (no permission), because on the next sync we want the folder to reappear,
  //  instead of the user being stuck with "can't delete" every time.
  // And we do it for _all_ deleted folders, even those that were deleted on the server in the first place (slotListResult).
  //  Otherwise this might have side effects much later (e.g. when regaining permissions to a folder we could see before)
  mDeletedFolders.clear();
  mPreviouslyDeletedFolders.clear();

  KMail::ImapAccountBase::postProcessNewMail();
}

void KMAcctCachedImap::addUnreadMsgCount( int msgs )
{
  mCountUnread += msgs;
}

void KMAcctCachedImap::addLastUnreadMsgCount( int msgs )
{
  mCountLastUnread += msgs;
}

//
//
// read/write config
//
//

void KMAcctCachedImap::readConfig( /*const*/ KConfig/*Base*/ & config ) {
  ImapAccountBase::readConfig( config );
  setProgressDialogEnabled( config.readBoolEntry( "progressdialog", true ) );
  // Apparently this method is only ever called once (from KMKernel::init) so this is ok
  mPreviouslyDeletedFolders = config.readListEntry( "deleted-folders" );
  mDeletedFolders.clear(); // but just in case...
}

void KMAcctCachedImap::writeConfig( KConfig/*Base*/ & config ) /*const*/ {
  ImapAccountBase::writeConfig( config );
  config.writeEntry( "progressdialog", isProgressDialogEnabled() );
  config.writeEntry( "deleted-folders", mDeletedFolders + mPreviouslyDeletedFolders );
}

void KMAcctCachedImap::invalidateIMAPFolders()
{
  invalidateIMAPFolders( mFolder );
}

void KMAcctCachedImap::invalidateIMAPFolders( KMFolderCachedImap* folder )
{
  if( !folder || !folder->folder() )
    return;

  folder->setAccount(this);

  QStringList strList;
  QValueList<QGuardedPtr<KMFolder> > folderList;
  kmkernel->dimapFolderMgr()->createFolderList( &strList, &folderList,
						folder->folder()->child(), QString::null,
						false );
  QValueList<QGuardedPtr<KMFolder> >::Iterator it;
  mCountLastUnread = 0;

  if( folderList.count() > 0 )
    for( it = folderList.begin(); it != folderList.end(); ++it ) {
      KMFolder *folder = *it;
      if( folder && folder->folderType() == KMFolderTypeCachedImap ) {
        KMFolderCachedImap *cfolder = static_cast<KMFolderCachedImap*>(folder->storage());
        // This invalidates the folder completely
        cfolder->setUidValidity("INVALID");
        cfolder->writeUidCache();
      }
    }
  folder->setUidValidity("INVALID");
  folder->writeUidCache();

  if ( !checkingMail() ) {
    setCheckingMail( true );
    processNewMail( false );
  }
}

//-----------------------------------------------------------------------------
void KMAcctCachedImap::listDirectory(QString path, ListType subscription,
    bool secondStep, KMFolder* parent, bool reset)
{
  ImapAccountBase::listDirectory( path, subscription, secondStep, parent, reset );
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

void KMAcctCachedImap::addDeletedFolder( const QString& subFolderPath )
{
  mDeletedFolders.append( subFolderPath );
}

bool KMAcctCachedImap::isDeletedFolder( const QString& subFolderPath ) const
{
  return mDeletedFolders.find( subFolderPath ) != mDeletedFolders.end();
}

bool KMAcctCachedImap::isPreviouslyDeletedFolder( const QString& subFolderPath ) const
{
  return mPreviouslyDeletedFolders.find( subFolderPath ) != mPreviouslyDeletedFolders.end();
}

void KMAcctCachedImap::removeDeletedFolder( const QString& subFolderPath )
{
  mDeletedFolders.remove( subFolderPath );
  mPreviouslyDeletedFolders.remove( subFolderPath );
}

#include "kmacctcachedimap.moc"
