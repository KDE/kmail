/**
 *  kmacctcachedimap.cpp
 *
 *  Copyright (c) 2002-2004 Bo Thorsen <bo@sonofthor.dk>
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

#include "kmfoldertree.h"
#include "kmfoldermgr.h"
#include "kmfiltermgr.h"
#include "kmfoldercachedimap.h"
#include "kmmainwin.h"
#include "kmmainwidget.h"
#include "kmkernel.h"
#include "kmacctmgr.h"
#include "progressmanager.h"

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
    mAnnotationCheckPassed(false)
{
  // Never EVER set this for the cached IMAP account
  mAutoExpunge = false;
}


//-----------------------------------------------------------------------------
KMAcctCachedImap::~KMAcctCachedImap()
{
  killAllJobsInternal( true );
}


//-----------------------------------------------------------------------------
QString KMAcctCachedImap::type() const
{
  return "cachedimap";
}

void KMAcctCachedImap::init() {
  ImapAccountBase::init();
}

//-----------------------------------------------------------------------------
void KMAcctCachedImap::pseudoAssign( const KMAccount * a ) {
  killAllJobs( true );
  if (mFolder)
  {
    mFolder->setContentState(KMFolderCachedImap::imapNoInformation);
    mFolder->setSubfolderState(KMFolderCachedImap::imapNoInformation);
  }
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
void KMAcctCachedImap::killAllJobs( bool disconnectSlave )
{
  //kdDebug(5006) << "killAllJobs: disconnectSlave=" << disconnectSlave << "  " << mapJobData.count() << " jobs in map." << endl;
  QValueList<KMFolderCachedImap*> folderList = killAllJobsInternal( disconnectSlave );
  for( QValueList<KMFolderCachedImap*>::Iterator it = folderList.begin(); it != folderList.end(); ++it ) {
    KMFolderCachedImap *fld = *it;
    fld->resetSyncState();
    fld->setContentState(KMFolderCachedImap::imapNoInformation);
    fld->setSubfolderState(KMFolderCachedImap::imapNoInformation);
    fld->sendFolderComplete(FALSE);
  }
}

//-----------------------------------------------------------------------------
// Common between killAllJobs and the destructor - which shouldn't call sendFolderComplete
QValueList<KMFolderCachedImap*> KMAcctCachedImap::killAllJobsInternal( bool disconnectSlave )
{
  // Make list of folders to reset. This must be done last, since folderComplete
  // can trigger the next queued mail check already.
  QValueList<KMFolderCachedImap*> folderList;
  QMap<KIO::Job*, jobData>::Iterator it = mapJobData.begin();
  for (; it != mapJobData.end(); ++it) {
    if ((*it).parent)
      folderList << static_cast<KMFolderCachedImap*>((*it).parent->storage());
    // Kill the job - except if it's the one that already died and is calling us
    if ( !it.key()->error() && mSlave ) {
      it.key()->kill();
      mSlave = 0; // killing a job, kills the slave
    }
  }
  mapJobData.clear();

  // Clear the joblist. Make SURE to stop the job emitting "finished"
  for( QPtrListIterator<CachedImapJob> it( mJobList ); it.current(); ++it )
    it.current()->setPassiveDestructor( true );
  KMAccount::deleteFolderJobs();

  if ( disconnectSlave && mSlave ) {
    KIO::Scheduler::disconnectSlave( mSlave );
    mSlave = 0;
  }
  return folderList;
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
    mMailCheckFolders.clear();
    mMailCheckFolders.append( mFoldersQueuedForChecking.front() );
    mFoldersQueuedForChecking.pop_front();
    if ( mFoldersQueuedForChecking.isEmpty() )
      disconnect( this, SIGNAL( finishedCheck( bool, CheckStatus ) ),
                  this, SLOT( slotCheckQueuedFolders() ) );

    kmkernel->acctMgr()->singleCheckMail(this, true);
    mMailCheckFolders.clear();
}

void KMAcctCachedImap::processNewMail( bool interactive )
{
  if ( !mFolder ) { // happens if this is a pseudo-account (from configuredialog)
    checkDone( false, CheckIgnored );
    return;
  }
  if ( mMailCheckFolders.isEmpty() )
   processNewMail( mFolder, interactive, true );
  else {
    KMFolder* f = mMailCheckFolders.front();
    mMailCheckFolders.pop_front();
    processNewMail( static_cast<KMFolderCachedImap *>( f->storage() ), interactive, false );
  }
}

void KMAcctCachedImap::processNewMail( KMFolderCachedImap* folder,
				       bool /* interactive */,
                                       bool recurse )
{
  // This should never be set for a cached IMAP account
  mAutoExpunge = false;
  mCountLastUnread = 0;
  mUnreadBeforeCheck.clear();
  // stop sending noops during sync, that will keep the connection open
  mNoopTimer.stop();

  Q_ASSERT( !mMailCheckProgressItem );
  mMailCheckProgressItem = KPIM::ProgressManager::createProgressItem(
    "MailCheck" + QString::number( id() ),
    folder->label(), // will be changed immediately in serverSync anyway
    QString::null,
    true, // can be cancelled
    useSSL() || useTLS() );
  connect( mMailCheckProgressItem, SIGNAL( progressItemCanceled( KPIM::ProgressItem* ) ),
           this, SLOT( slotProgressItemCanceled( KPIM::ProgressItem* ) ) );

  folder->setAccount(this);
  connect(folder, SIGNAL(folderComplete(KMFolderCachedImap*, bool)),
	  this, SLOT(postProcessNewMail(KMFolderCachedImap*, bool)));
  folder->serverSync( recurse );
}

void KMAcctCachedImap::postProcessNewMail( KMFolderCachedImap* folder, bool )
{
  mNoopTimer.start( 60000 ); // send a noop every minute to avoid "connection broken" errors
  disconnect(folder, SIGNAL(folderComplete(KMFolderCachedImap*, bool)),
             this, SLOT(postProcessNewMail(KMFolderCachedImap*, bool)));
  mMailCheckProgressItem->setComplete();
  mMailCheckProgressItem = 0;

  if ( folder == mFolder ) {
    // We remove everything from the deleted folders list after a full sync.
    // Even if it fails (no permission), because on the next sync we want the folder to reappear,
    //  instead of the user being stuck with "can't delete" every time.
    // And we do it for _all_ deleted folders, even those that were deleted on the server in the first place (slotListResult).
    //  Otherwise this might have side effects much later (e.g. when regaining permissions to a folder we could see before)
    mDeletedFolders.clear();
    mPreviouslyDeletedFolders.clear();
  }

  KMail::ImapAccountBase::postProcessNewMail();
}

void KMAcctCachedImap::addUnreadMsgCount( const KMFolderCachedImap *folder,
                                          int countUnread )
{
  if ( folder->imapPath() != "/INBOX/" ) {
    // new mail in INBOX is processed with KMAccount::processNewMsg() and
    // therefore doesn't need to be counted here
    const QString folderId = folder->folder()->idString();
    int newInFolder = countUnread;
    if ( mUnreadBeforeCheck.find( folderId ) != mUnreadBeforeCheck.end() )
      newInFolder -= mUnreadBeforeCheck[folderId];
    if ( newInFolder > 0 )
      addToNewInFolder( folderId, newInFolder );
  }
  mCountUnread += countUnread;
}

void KMAcctCachedImap::addLastUnreadMsgCount( const KMFolderCachedImap *folder,
                                              int countLastUnread )
{
  mUnreadBeforeCheck[folder->folder()->idString()] = countLastUnread;
  mCountLastUnread += countLastUnread;
}

//
//
// read/write config
//
//

void KMAcctCachedImap::readConfig( /*const*/ KConfig/*Base*/ & config ) {
  ImapAccountBase::readConfig( config );
  // Apparently this method is only ever called once (from KMKernel::init) so this is ok
  mPreviouslyDeletedFolders = config.readListEntry( "deleted-folders" );
  mDeletedFolders.clear(); // but just in case...
  const QStringList oldPaths = config.readListEntry( "renamed-folders-paths" );
  const QStringList newNames = config.readListEntry( "renamed-folders-names" );
  QStringList::const_iterator it = oldPaths.begin();
  QStringList::const_iterator nameit = newNames.begin();
  for( ; it != oldPaths.end() && nameit != newNames.end(); ++it, ++nameit ) {
    addRenamedFolder( *it, QString::null, *nameit );
  }
}

void KMAcctCachedImap::writeConfig( KConfig/*Base*/ & config ) /*const*/ {
  ImapAccountBase::writeConfig( config );
  config.writeEntry( "deleted-folders", mDeletedFolders + mPreviouslyDeletedFolders );
  config.writeEntry( "renamed-folders-paths", mRenamedFolders.keys() );
  const QValueList<RenamedFolder> values = mRenamedFolders.values();
  QStringList lstNames;
  QValueList<RenamedFolder>::const_iterator it = values.begin();
  for ( ; it != values.end() ; ++it )
    lstNames.append( (*it).mNewName );
  config.writeEntry( "renamed-folders-names", lstNames );
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
  mUnreadBeforeCheck.clear();

  for( it = folderList.begin(); it != folderList.end(); ++it ) {
    KMFolder *f = *it;
    if( f && f->folderType() == KMFolderTypeCachedImap ) {
      KMFolderCachedImap *cfolder = static_cast<KMFolderCachedImap*>(f->storage());
      // This invalidates the folder completely
      cfolder->setUidValidity("INVALID");
      cfolder->writeUidCache();
      processNewMailSingleFolder( f );
    }
  }
  folder->setUidValidity("INVALID");
  folder->writeUidCache();

  processNewMailSingleFolder( folder->folder() );
}

//-----------------------------------------------------------------------------
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

void KMAcctCachedImap::addRenamedFolder( const QString& subFolderPath, const QString& oldLabel, const QString& newName )
{
  mRenamedFolders.insert( subFolderPath, RenamedFolder( oldLabel, newName ) );
}

void KMAcctCachedImap::removeRenamedFolder( const QString& subFolderPath )
{
  mRenamedFolders.remove( subFolderPath );
}

void KMAcctCachedImap::slotProgressItemCanceled( ProgressItem* )
{
  bool abortConnection = !mSlaveConnected;
  killAllJobs( abortConnection );
  if ( abortConnection ) {
    // If we were trying to connect, tell kmfoldercachedimap so that it moves on
    emit connectionResult( KIO::ERR_USER_CANCELED, QString::null );
  }
}

FolderStorage* const KMAcctCachedImap::rootFolder() const
{
  return mFolder;
}


QString KMAcctCachedImap::renamedFolder( const QString& imapPath ) const
{
  QMap<QString, RenamedFolder>::ConstIterator renit = mRenamedFolders.find( imapPath );
  if ( renit != mRenamedFolders.end() )
    return (*renit).mNewName;
  return QString::null;
}

#include "kmacctcachedimap.moc"
