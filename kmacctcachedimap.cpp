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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
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

#include "kmacctcachedimap.h"
using KMail::SieveConfig;

#include "kmfoldertree.h"
#include "kmfoldermgr.h"
#include "kmfiltermgr.h"
#include "kmfoldercachedimap.h"
#include "kmmainwin.h"
#include "accountmanager.h"
using KMail::AccountManager;
#include "progressmanager.h"

#include <kio/passworddialog.h>
#include <kio/scheduler.h>
#include <kio/slave.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kconfiggroup.h>

#include <QList>

KMAcctCachedImap::KMAcctCachedImap( AccountManager* aOwner,
				    const QString& aAccountName, uint id )
  : KMail::ImapAccountBase( aOwner, aAccountName, id ), mFolder( 0 ),
    mAnnotationCheckPassed(false),
    mGroupwareType( GroupwareKolab ),
    mSentCustomLoginCommand(false)
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

//-----------------------------------------------------------------------------
void KMAcctCachedImap::setImapFolder(KMFolderCachedImap *aFolder)
{
  mFolder = aFolder;
  mFolder->setImapPath( "/" );
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
  //kDebug(5006) <<"killAllJobs: disconnectSlave=" << disconnectSlave << mapJobData.count() <<" jobs in map.";
  QList<KMFolderCachedImap*> folderList = killAllJobsInternal( disconnectSlave );
  for( QList<KMFolderCachedImap*>::Iterator it = folderList.begin(); it != folderList.end(); ++it ) {
    KMFolderCachedImap *fld = *it;
    fld->resetSyncState();
    fld->setContentState(KMFolderCachedImap::imapNoInformation);
    fld->setSubfolderState(KMFolderCachedImap::imapNoInformation);
    fld->sendFolderComplete(false);
  }
}

//-----------------------------------------------------------------------------
// Common between killAllJobs and the destructor - which shouldn't call sendFolderComplete
QList<KMFolderCachedImap*> KMAcctCachedImap::killAllJobsInternal( bool disconnectSlave )
{
  // Make list of folders to reset. This must be done last, since folderComplete
  // can trigger the next queued mail check already.
  QList<KMFolderCachedImap*> folderList;
  QMap<KJob*, jobData>::Iterator it = mapJobData.begin();
  for (; it != mapJobData.end(); ++it) {
    if ((*it).parent)
      folderList << static_cast<KMFolderCachedImap*>((*it).parent->storage());
    // Kill the job - except if it's the one that already died and is calling us
    if ( mSlave && !it.key()->error() ) {
      it.key()->kill();
      mSlave = 0; // killing a job, kills the slave
    }
  }
  mapJobData.clear();

  // Clear the joblist. Make SURE to stop the job emitting "finished"
  QList<CachedImapJob*>::const_iterator jt;
  for( jt = mJobList.constBegin(); jt != mJobList.constEnd(); ++jt )
    (*jt)->setPassiveDestructor( true );
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
  QList<KMFolderCachedImap*> folderList;
  QMap<KJob*, jobData>::Iterator it = mapJobData.begin();
  for (; it != mapJobData.end(); ++it) {
    if ( (*it).cancellable && (*it).parent )
      folderList << static_cast<KMFolderCachedImap*>((*it).parent->storage());
  }
  // Kill jobs
  ImapAccountBase::cancelMailCheck();
  // Reset sync states and emit folderComplete, this is important for
  // KMAccount::checkingMail() to be reset, in case we restart checking mail later.
  for( QList<KMFolderCachedImap*>::Iterator it = folderList.begin(); it != folderList.end(); ++it ) {
    KMFolderCachedImap *fld = *it;
    fld->resetSyncState();
    fld->setContentState(KMFolderCachedImap::imapNoInformation);
    fld->setSubfolderState(KMFolderCachedImap::imapNoInformation);
    fld->sendFolderComplete(false);
  }
}

//-----------------------------------------------------------------------------
void KMAcctCachedImap::killJobsForItem(KMFolderTreeItem * fti)
{
  QMap<KJob *, jobData>::Iterator it = mapJobData.begin();
  while (it != mapJobData.end())
  {
    if (it.value().parent == fti->folder())
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

void KMAcctCachedImap::processNewMail( bool /*interactive*/ )
{
  assert( mFolder );

  if ( mMailCheckFolders.isEmpty() )
    processNewMail( mFolder, true );
  else {
    KMFolder* f = mMailCheckFolders.front();
    mMailCheckFolders.pop_front();

    // Only check mail if the folder really exists, it might have been removed by the sync in
    // the meantime.
    if ( f ) {
      processNewMail( static_cast<KMFolderCachedImap *>( f->storage() ), !checkingSingleFolder() );
    }
  }
}

void KMAcctCachedImap::processNewMail( KMFolderCachedImap* folder,
                                       bool recurse )
{
  assert( folder );
  // This should never be set for a cached IMAP account
  mAutoExpunge = false;
  mCountLastUnread = 0;
  mUnreadBeforeCheck.clear();
  // stop sending noops during sync, that will keep the connection open
  mNoopTimer.stop();

  // reset namespace todo
  if ( folder == mFolder ) {
    QStringList nsToList = namespaces()[PersonalNS];
    QStringList otherNSToCheck = namespaces()[OtherUsersNS];
    otherNSToCheck += namespaces()[SharedNS];
    for ( QStringList::Iterator it = otherNSToCheck.begin();
          it != otherNSToCheck.end(); ++it ) {
      if ( (*it).isEmpty() ) {
        // empty namespaces are included in the "normal" listing
        // as the folders are created under the root folder
        nsToList += *it;
      }
    }
    folder->setNamespacesToList( nsToList );
  }

  Q_ASSERT( !mMailCheckProgressItem );
  mMailCheckProgressItem = KPIM::ProgressManager::createProgressItem(
    "MailCheck" + QString::number( id() ),
    folder->label(), // will be changed immediately in serverSync anyway
    QString(),
    true, // can be canceled
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

#if 0 // this opens a race: delete a folder during a sync (after the sync checked that folder), and it'll be forgotten...
    mDeletedFolders.clear();
#endif
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
    if ( mUnreadBeforeCheck.contains( folderId )  )
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

void KMAcctCachedImap::readConfig( KConfigGroup & config ) {
  ImapAccountBase::readConfig( config );
  // Apparently this method is only ever called once (from KMKernel::init) so this is ok
  mPreviouslyDeletedFolders = config.readEntry( "deleted-folders", QStringList() );
  mDeletedFolders.clear(); // but just in case...
  const QStringList oldPaths = config.readEntry( "renamed-folders-paths", QStringList() );
  const QStringList newNames = config.readEntry( "renamed-folders-names", QStringList() );
  QStringList::const_iterator it = oldPaths.begin();
  QStringList::const_iterator nameit = newNames.begin();
  for( ; it != oldPaths.end() && nameit != newNames.end(); ++it, ++nameit ) {
    addRenamedFolder( *it, QString(), *nameit );
  }
  mGroupwareType = (GroupwareType)config.readEntry( "groupwareType", (int)GroupwareKolab );
}

void KMAcctCachedImap::writeConfig( KConfigGroup & config ) {
  ImapAccountBase::writeConfig( config );
  config.writeEntry( "deleted-folders", mDeletedFolders + mPreviouslyDeletedFolders );
  config.writeEntry( "renamed-folders-paths", mRenamedFolders.keys() );
  const QList<RenamedFolder> values = mRenamedFolders.values();
  QStringList lstNames;
  QList<RenamedFolder>::const_iterator it = values.begin();
  for ( ; it != values.end() ; ++it )
    lstNames.append( (*it).mNewName );
  config.writeEntry( "renamed-folders-names", lstNames );
  config.writeEntry( "groupwareType", (int)mGroupwareType );
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
  QList<QPointer<KMFolder> > folderList;
  kmkernel->dimapFolderMgr()->createFolderList( &strList, &folderList,
                                                folder->folder()->child(), QString(),
                                                false );
  QList<QPointer<KMFolder> >::Iterator it;
  mCountLastUnread = 0;
  mUnreadBeforeCheck.clear();

  for( it = folderList.begin(); it != folderList.end(); ++it ) {
    KMFolder *f = *it;
    if( f && f->folderType() == KMFolderTypeCachedImap ) {
      KMFolderCachedImap *cfolder = static_cast<KMFolderCachedImap*>(f->storage());
      // This invalidates the folder completely
      cfolder->setUidValidity("INVALID");
      cfolder->writeUidCache();
    }
  }
  folder->setUidValidity("INVALID");
  folder->writeUidCache();

  processNewMailInFolder( folder->folder(), Recursive );
}

//-----------------------------------------------------------------------------
void KMAcctCachedImap::addDeletedFolder( KMFolder* folder )
{
  if ( !folder || folder->folderType() != KMFolderTypeCachedImap )
    return;
  KMFolderCachedImap* storage = static_cast<KMFolderCachedImap*>(folder->storage());
  addDeletedFolder( storage->imapPath() );
  kDebug(5006) << storage->imapPath();

  // Add all child folders too
  if( folder->child() ) {
    QList<KMFolderNode*>::const_iterator it;
    for ( it = folder->child()->constBegin();
        it != folder->child()->constEnd();
        ++it ) {
      KMFolderNode *node = (*it);
      if( node && !node->isDir() ) {
        addDeletedFolder( static_cast<KMFolder*>( node ) ); // recurse
      }
    }
  }
}

void KMAcctCachedImap::addDeletedFolder( const QString& imapPath )
{
  mDeletedFolders << imapPath;
}

QStringList KMAcctCachedImap::deletedFolderPaths( const QString& subFolderPath ) const
{
  QStringList lst;
  for ( QStringList::const_iterator it = mDeletedFolders.begin(); it != mDeletedFolders.end(); ++it ) {
    if ( (*it).startsWith( subFolderPath ) )
      // We must reverse the order, so that sub sub sub folders are deleted first
      lst.prepend( *it );
  }
  for ( QStringList::const_iterator it = mPreviouslyDeletedFolders.begin(); it != mPreviouslyDeletedFolders.end(); ++it ) {
    if ( (*it).startsWith( subFolderPath ) )
      lst.prepend( *it );
  }
  kDebug(5006) << "For" << subFolderPath <<" returning:" << lst;
  Q_ASSERT( !lst.isEmpty() );
  return lst;
}

bool KMAcctCachedImap::isDeletedFolder( const QString& subFolderPath ) const
{
  return mDeletedFolders.contains( subFolderPath ) ;
}

bool KMAcctCachedImap::isPreviouslyDeletedFolder( const QString& subFolderPath ) const
{
  return mPreviouslyDeletedFolders.contains( subFolderPath ) ;
}

void KMAcctCachedImap::removeDeletedFolder( const QString& subFolderPath )
{
  mDeletedFolders.removeAll( subFolderPath );
  mPreviouslyDeletedFolders.removeAll( subFolderPath );
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
    emit connectionResult( KIO::ERR_USER_CANCELED, QString() );
  }
}

FolderStorage* KMAcctCachedImap::rootFolder() const
{
  return mFolder;
}


QString KMAcctCachedImap::renamedFolder( const QString& imapPath ) const
{
  QMap<QString, RenamedFolder>::ConstIterator renit = mRenamedFolders.find( imapPath );
  if ( renit != mRenamedFolders.end() )
    return (*renit).mNewName;
  return QString();
}

#include "kmacctcachedimap.moc"
