/**
 * kmfoldercachedimap.cpp
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

#include <errno.h>

#include "kmfoldercachedimap.h"
#include "undostack.h"
#include "kmfoldermgr.h"
#include "kmacctcachedimap.h"
#include "kmacctmgr.h"
#include "imapprogressdialog.h"
using KMail::CachedImapJob;
using KMail::ImapAccountBase;

#include <kapplication.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>
#include <kio/scheduler.h>
#include <kconfig.h>
#include <qbuffer.h>
#include <qfile.h>

#define UIDCACHE_VERSION 1


KMFolderCachedImap::KMFolderCachedImap(KMFolderDir* aParent, const QString& aName)
 : KMFolderCachedImapInherited(aParent, aName), mSyncState(SYNC_STATE_INITIAL),
   mContentState(imapNoInformation),
   mSubfolderState(imapNoInformation), mIsSelected(FALSE), mCheckFlags(TRUE), mAccount(NULL),
   mLastUid(0), mIsConnected(false), mFolderRemoved(false)
{
  KConfig* config = kapp->config();
  KConfigGroupSaver saver(config, "Folder-" + idString());
  if (mImapPath.isEmpty()) mImapPath = config->readEntry("ImapPath");
  if (aName == "INBOX" && mImapPath == "/INBOX/")
  {
    //mIsSystemFolder = TRUE;
    //mLabel = i18n("inbox");
  }
  mNoContent = config->readBoolEntry("NoContent", FALSE);
  mReadOnly = config->readBoolEntry("ReadOnly", FALSE);

  connect( this, SIGNAL( listMessagesComplete() ), this, SLOT( serverSyncInternal() ) );

  setUidValidity("");
  mLastUid=0;
  readUidCache();

  mProgress = 0;
}

KMFolderCachedImap::~KMFolderCachedImap()
{
  if( !mFolderRemoved ) {
    // Only write configuration when the folder haven't been deleted
    KConfig* config = kapp->config();
    KConfigGroupSaver saver(config, "Folder-" + idString());
    config->writeEntry("UidValidity", mUidValidity); /* unused */
    config->writeEntry("lastUid", mLastUid); /* unused */
    config->writeEntry("ImapPath", mImapPath);
    config->writeEntry("NoContent", mNoContent);
    config->writeEntry("ReadOnly", mReadOnly);

    writeUidCache();
  }

  if (kernel->undoStack()) kernel->undoStack()->folderDestroyed(this);
}

int KMFolderCachedImap::remove()
{
  mFolderRemoved = true;
  return KMFolderCachedImapInherited::remove();
}

QString KMFolderCachedImap::uidCacheLocation() const
{
  QString sLocation(path());
  if (!sLocation.isEmpty()) sLocation += '/';
  return sLocation + '.' + dotEscape(fileName()) + ".uidcache";
}

int KMFolderCachedImap::readUidCache()
{
  QFile uidcache( uidCacheLocation() );
  if( uidcache.open( IO_ReadOnly ) ) {
    char buf[1024];
    int len = uidcache.readLine( buf, sizeof(buf) );
    if( len > 0 ) {
      int cacheVersion;
      sscanf( buf, "# KMail-UidCache V%d\n",  &cacheVersion );
      if( cacheVersion == UIDCACHE_VERSION ) {
	len = uidcache.readLine( buf, sizeof(buf) );
	if( len > 0 ) {
	  setUidValidity( QString::fromLocal8Bit( buf).stripWhiteSpace() );
	  len = uidcache.readLine( buf, sizeof(buf) );
	  if( len > 0 ) {
	    mLastUid = QString::fromLocal8Bit( buf).stripWhiteSpace().toULong();
	    return 0;
	  }
	}
      }
    }
  }
  return -1;
}

int KMFolderCachedImap::writeUidCache()
{
  QFile uidcache( uidCacheLocation() );
  if( uidcache.open( IO_WriteOnly ) ) {
    QTextStream str( &uidcache );
    str << "# KMail-UidCache V" << UIDCACHE_VERSION << endl;
    str << uidValidity() << endl;
    str << lastUid() << endl;
    uidcache.flush();
    fsync( uidcache.handle() ); /* this is probably overkill */
    uidcache.close();
    return 0;
  } else {
    return errno; /* does QFile set errno? */
  }
}

int KMFolderCachedImap::create(bool imap)
{
  int rc = KMFolderCachedImapInherited::create(imap);
  mLastUid = 0;
  mUidValidity = "";
  if( !rc ) return writeUidCache();
  return rc;
}

void KMFolderCachedImap::reloadUidMap()
{
  uidMap.clear();
  uidRevMap.clear();
  open();
  for( int i = 0; i < count(); ++i ) {
    bool unget = !isMessage(i);
    bool ok;
    KMMessage *msg = getMsg(i);
    if( !msg ) continue;
    ulong uid = msg->headerField("X-UID").toULong(&ok);
    if (unget) unGetMsg(i);
    if( ok ) {
      uidMap.insert( uid, i );
      uidRevMap.insert( i, uid );
    }
  }
  close();
}

/* Reimplemented from KMFolderMaildir */
KMMessage* KMFolderCachedImap::take(int idx)
{
  QMap<int,ulong>::Iterator it = uidRevMap.find(idx);
  if( it != uidRevMap.end() ) {
    uidMap.remove( it.data() );
    uidRevMap.remove( idx );
  }
  return KMFolderCachedImapInherited::take(idx);
}

// Add a message without clearing it's X-UID field.
int KMFolderCachedImap::addMsgInternal(KMMessage* msg, int* index_return)
{
  bool ok;
  int idx_return;

  // Add the message
  ulong uid = msg->headerField("X-UID").toULong(&ok);
  int rc = KMFolderCachedImapInherited::addMsg(msg, &idx_return);
  if( index_return ) *index_return = idx_return;

  // Put it in the uid maps
  if( ok && !rc && idx_return >= 0 ) {
    uidMap.insert( uid, idx_return );
    uidRevMap.insert( idx_return, uid );
    if( uid > mLastUid ) mLastUid = uid;
  }

  return rc;
}

/* Reimplemented from KMFolderMaildir */
int KMFolderCachedImap::addMsg(KMMessage* msg, int* index_return)
{
  assert(msg);

  // Strip the IMAP UID
  msg->removeHeaderField( "X-UID" );

  // Add it to storage
  return addMsgInternal( msg, index_return );
}


/* Reimplemented from KMFolderMaildir */
void KMFolderCachedImap::removeMsg(int idx, bool imapQuiet)
{
  // kdDebug(5006) << "KMFolderCachedImap::removeMsg(" << idx << ", " << imapQuiet << ")" << endl;

  // Remove it from the maps
  QMap<int,ulong>::Iterator it = uidRevMap.find(idx);
  if( it != uidRevMap.end() ) {
    ulong uid = it.data();
    uidMap.remove( uid );
    uidRevMap.remove( idx );
  }

  // Remove it from disk
  KMFolderCachedImapInherited::removeMsg(idx,imapQuiet);

  // TODO (Bo): Shouldn't this be "emit changed();"?
  kernel->imapFolderMgr()->contentsChanged();
}

bool KMFolderCachedImap::canRemoveFolder() const {
  // If this has subfolders it can't be removed
  if( child() != 0 && child()->count() > 0 )
    return false;

#if 0
  // No special condition here, so let base class decide
  return KMFolderCachedImapInherited::canRemoveFolder();
#endif
  return true;
}

/* Reimplemented from KMFolderDir */
int KMFolderCachedImap::rename(const QString& aName, KMFolderDir *aParent) {
  assert( aParent == 0 );
  Q_UNUSED( aParent );

  if ( aName == name() )
    // Stupid user trying to rename it to it's old name :)
    return 0;

  if( mSyncState != SYNC_STATE_INITIAL ) {
    KMessageBox::error( 0, i18n("You can't rename a folder when a sync is in progress") );
    return -1;
  }

  if( account() == 0 ) {
    QString err = i18n("You must synchronize with the server before renaming IMAP folders.");
    KMessageBox::error( 0, err );
    return -1;
  }

  CachedImapJob *job = new CachedImapJob( aName, CachedImapJob::tRenameFolder, this );
  job->start();
  return 0;
}

void KMFolderCachedImap::setLastUid( ulong uid ) {
  mLastUid = uid;
  writeUidCache();
}

ulong KMFolderCachedImap::lastUid() {
  if( mLastUid )
    return mLastUid;

  //reloadUidMap();
  open();
  if (count() > 0)
  {
    bool unget = !isMessage(count() - 1);
    KMMessage *msg = getMsg(count() - 1);
    mLastUid = msg->headerField("X-UID").toULong();
    if (unget) unGetMsg(count() - 1);
  }
  close();
  // kdDebug(5006) << "KMFolderCachedImap::lastUid("<<name()<<") = " << mLastUid << endl;
  return mLastUid;
}

KMMsgBase* KMFolderCachedImap::findByUID( ulong uid )
{
  QMap<ulong,int>::Iterator it = uidMap.find( uid );
  if( it != uidMap.end() ) return getMsgBase( *it );
  else return NULL;
}

// This finds and sets the proper account for this folder if it has not been done
KMAcctCachedImap *KMFolderCachedImap::account()
{
  if( (KMAcctCachedImap *)mAccount == 0 ) {
    // Find the account
    mAccount = static_cast<KMAcctCachedImap *>( kernel->acctMgr()->find( name() ) );
  }

  return mAccount;
}

void KMFolderCachedImap::serverSync()
{
  if( mSyncState != SYNC_STATE_INITIAL ) {
    if( KMessageBox::warningYesNo( 0, i18n("Folder %1 is not in initial sync state (state was %2). Do you want to reset\nit to initial sync state and sync anyway?" ).arg( imapPath() ).arg( mSyncState ) ) == KMessageBox::Yes ) {
      mSyncState = SYNC_STATE_INITIAL;
    } else return;
  }

  assert( account() );

  reloadUidMap();
  kdDebug(5006) << "KMFolderCachedImap::serverSync(), imapPath()=" << imapPath() << ", path()="
            << path() << " name()="<< name() << endl;

  // Connect to the imap progress dialog
  if( mIsConnected != mAccount->isProgressDialogEnabled() ) {
    if( !mIsConnected )
      connect( this, SIGNAL( newState( const QString&, int, const QString& ) ),
	       account()->imapProgressDialog(),
	       SLOT( syncState( const QString&, int, const QString& ) ) );
    else
      disconnect( this, SIGNAL( newState( const QString&, int, const QString& ) ),
	       account()->imapProgressDialog(),
	       SLOT( syncState( const QString&, int, const QString& ) ) );
    mIsConnected = mAccount->isProgressDialogEnabled();
  }

  serverSyncInternal();
}

QString KMFolderCachedImap::state2String( int state ) const
{
  switch( state ) {
  case SYNC_STATE_INITIAL:           return "SYNC_STATE_INITIAL";
  case SYNC_STATE_PUT_MESSAGES:      return "SYNC_STATE_PUT_MESSAGES";
  case SYNC_STATE_CREATE_SUBFOLDERS: return "SYNC_STATE_CREATE_SUBFOLDERS";
  case SYNC_STATE_LIST_SUBFOLDERS:   return "SYNC_STATE_LIST_SUBFOLDERS";
  case SYNC_STATE_LIST_SUBFOLDERS2:  return "SYNC_STATE_LIST_SUBFOLDERS2";
  case SYNC_STATE_DELETE_SUBFOLDERS: return "SYNC_STATE_DELETE_SUBFOLDERS";
  case SYNC_STATE_LIST_MESSAGES:     return "SYNC_STATE_LIST_MESSAGES";
  case SYNC_STATE_DELETE_MESSAGES:   return "SYNC_STATE_DELETE_MESSAGES";
  case SYNC_STATE_GET_MESSAGES:      return "SYNC_STATE_GET_MESSAGES";
  case SYNC_STATE_FIND_SUBFOLDERS:   return "SYNC_STATE_FIND_SUBFOLDERS";
  case SYNC_STATE_SYNC_SUBFOLDERS:   return "SYNC_STATE_SYNC_SUBFOLDERS";
  case SYNC_STATE_EXPUNGE_MESSAGES:  return "SYNC_STATE_EXPUNGE_MESSAGES";
  case SYNC_STATE_HANDLE_INBOX:      return "SYNC_STATE_HANDLE_INBOX";
  case SYNC_STATE_CHECK_UIDVALIDITY: return "SYNC_STATE_CHECK_UIDVALIDITY";
  default:                           return "Unknown state";
  }
}


// While the server synchronization is running, mSyncState will hold
// the state that should be executed next
void KMFolderCachedImap::serverSyncInternal()
{
  switch( mSyncState ) {
  case SYNC_STATE_INITIAL:
  {
    mProgress = 0;
    emit statusMsg( i18n("%1: Synchronizing").arg(name()) );
    emit newState( name(), mProgress, i18n("Syncronizing"));

    open();

    // Connect to the server (i.e. prepare the slave)
    ImapAccountBase::ConnectionState cs = mAccount->makeConnection();
    if ( cs == ImapAccountBase::Error ) // cancelled by user, or slave can't start
    {
        kdDebug(5006) << "makeConnection said Error, aborting." << endl;
        // We stop here. We're already in SYNC_STATE_INITIAL for the next time.
        emit folderComplete(this, FALSE);
        break;
    } else if ( cs == ImapAccountBase::Connecting )
    {
        kdDebug(5006) << "makeConnection said Connecting, waiting for signal." << endl;
        // We'll wait for the connectionResult signal from the account.
        connect( mAccount, SIGNAL( connectionResult(int) ),
                 this, SLOT( slotConnectionResult(int) ) );
        break;
    } else // Connected
    {
        kdDebug(5006) << "makeConnection said Connected, proceeding." << endl;
        mSyncState = SYNC_STATE_CHECK_UIDVALIDITY;
        // Fall through to next state
    }
  }
  case SYNC_STATE_CHECK_UIDVALIDITY:
    mSyncState = SYNC_STATE_CREATE_SUBFOLDERS;
    if( !noContent() ) {
      // TODO (Bo): How can we obtain the UID validity on a noContent folder?
      // TODO (Bo): Is it perhaps not necessary to do so?
      checkUidValidity();
      break;
    }
    // Else carry on

  case SYNC_STATE_CREATE_SUBFOLDERS:
    mSyncState = SYNC_STATE_PUT_MESSAGES;
    createNewFolders();
    break;

  case SYNC_STATE_PUT_MESSAGES:
    mSyncState = SYNC_STATE_LIST_SUBFOLDERS;
    if( !noContent() ) {
      uploadNewMessages();
      break;
    }
    // Else carry on

  case SYNC_STATE_LIST_SUBFOLDERS:
    mSyncState = SYNC_STATE_LIST_SUBFOLDERS2;
    mProgress += 10;
    emit statusMsg( i18n("%1: Retrieving folderlist").arg(name()) );
    emit newState( name(), mProgress, i18n("Retrieving folderlist"));
    if( !listDirectory() ) {
      mSyncState = SYNC_STATE_INITIAL;
      KMessageBox::error(0, i18n("Error during listDirectory()"));
    }
    break;

  case SYNC_STATE_LIST_SUBFOLDERS2:
    mSyncState = SYNC_STATE_DELETE_SUBFOLDERS;
    mProgress += 10;
    emit newState( name(), mProgress, i18n("Retrieving subfolders"));
    listDirectory2();
    break;

  case SYNC_STATE_DELETE_SUBFOLDERS:
    mSyncState = SYNC_STATE_LIST_MESSAGES;
    emit syncState( SYNC_STATE_DELETE_SUBFOLDERS, foldersForDeletionOnServer.count() );
    if( !foldersForDeletionOnServer.isEmpty() ) {
      emit statusMsg( i18n("%1: Deleting folders %2 from server").arg(name())
		      .arg( foldersForDeletionOnServer.join(", ") ) );
      emit newState( name(), mProgress, i18n("Deleting folders from server"));
      CachedImapJob* job = new CachedImapJob( foldersForDeletionOnServer,
						  CachedImapJob::tDeleteFolders, this );
      connect( job, SIGNAL( finished() ), this, SLOT( serverSyncInternal() ) );
      job->start();
      break;
    } else
      emit newState( name(), mProgress, i18n("No folders to delete from server"));
      // Carry on

  case SYNC_STATE_LIST_MESSAGES:
    mSyncState = SYNC_STATE_DELETE_MESSAGES;
    mProgress += 10;
    if( !noContent() ) {
      emit statusMsg( i18n("%1: Retrieving messagelist").arg(name()) );
      emit newState( name(), mProgress, i18n("Retrieving messagelist"));
      // emit syncState( SYNC_STATE_LIST_MESSAGES, foldersForDeletionOnServer.count() );
      listMessages();
      break;
    }
    // Else carry on

  case SYNC_STATE_DELETE_MESSAGES:
    mSyncState = SYNC_STATE_EXPUNGE_MESSAGES;
    if( !noContent() ) {
      if( deleteMessages() ) {
	// Fine, we will continue with the next state
      } else {
	// No messages to delete, skip to GET_MESSAGES
	emit newState( name(), mProgress, i18n("No messages to delete..."));
	mSyncState = SYNC_STATE_GET_MESSAGES;
	serverSyncInternal();
      }
      break;
    }
    // Else carry on

  case SYNC_STATE_EXPUNGE_MESSAGES:
    mSyncState = SYNC_STATE_GET_MESSAGES;
    if( !noContent() ) {
      mProgress += 10;
      emit statusMsg( i18n("%1: Expunging deleted messages").arg(name()) );
      emit newState( name(), mProgress, i18n("Expunging deleted messages"));
      CachedImapJob *job = new CachedImapJob( QString::null,
						  CachedImapJob::tExpungeFolder, this );
      connect( job, SIGNAL( finished() ), this, SLOT( serverSyncInternal() ) );
      job->start();
      break;
    }
    // Else carry on

  case SYNC_STATE_GET_MESSAGES:
    mSyncState = SYNC_STATE_HANDLE_INBOX;
    if( !noContent() ) {
      //emit syncState( SYNC_STATE_GET_MESSAGES, mMsgsForDownload.count() );
      if( !mMsgsForDownload.isEmpty() ) {
	emit statusMsg( i18n("%1: Retrieving new messages").arg(name()) );
	emit newState( name(), mProgress, i18n("Retrieving new messages"));
	CachedImapJob *job = new CachedImapJob( mMsgsForDownload,
                                                CachedImapJob::tGetMessage,
                                                this );
	connect( job, SIGNAL( progress(unsigned long, unsigned long) ),
                 this, SLOT( slotProgress(unsigned long, unsigned long) ) );
	connect( job, SIGNAL( finished() ), this, SLOT( serverSyncInternal() ) );
        job->start();
	mMsgsForDownload.clear();
	break;
      } else {
	emit newState( name(), mProgress, i18n("No new messages from server"));
      }
    }
    // Else carry on

  case SYNC_STATE_HANDLE_INBOX:
    // Wrap up the 'download emails' stage (which has a 20% span)
    mProgress += 20;
    //kdDebug() << name() << ": +20 -> " << mProgress << "%" << endl;

    mSyncState = SYNC_STATE_FIND_SUBFOLDERS;
#if 0
    if( imapPath() == "/INBOX/" && kernel->groupware().isEnabled() ) {
      // Here we need to move messages from INBOX to the "real" inbox
      KMFolderNode* node = child()->hasNamedFolder( kernel->groupware().folderName( KFolderTreeItem::Inbox ) );
      if( node && !node->isDir() ) {
	KMFolder* inboxFolder = static_cast<KMFolder*>(node);
	open();
	inboxFolder->open();
	mAccount->setFolder( inboxFolder );
	while( count() > 0) {
	  KMMessage* m = getMsg(0);
	  inboxFolder->moveMsg(m);
	  mAccount->processNewMsg(m);
	}
	compact();
	close();
	inboxFolder->close();
	// Delete from INBOX
	if( deleteMessages() ) {
	  break;
	}
      }
    }
#endif
    // Carry on

  case SYNC_STATE_FIND_SUBFOLDERS:
    {
      emit newState( name(), mProgress, i18n("Updating cache file"));

      // last state got new messages, update cache file
      writeUidCache();

      mSyncState = SYNC_STATE_SYNC_SUBFOLDERS;
      mSubfoldersForSync.clear();
      mCurrentSubfolder = 0;
      if( child() ) {
	KMFolderNode *node = child()->first();
	while( node ) {
	  if( !node->isDir() ) {
            //kdDebug(5006) << "##### child folder " << node->name() << " is a "
            //           << node->className() << endl;
            if ( !static_cast<KMFolderCachedImap*>(node)->imapPath().isEmpty() )
	      // Only sync folders that have been accepted by the server
	      mSubfoldersForSync << static_cast<KMFolderCachedImap*>(node);
	  }
	  node = child()->next();
	}
      }
    }

    // All done for this folder.
    mProgress = 100; // all done
    emit newState( name(), mProgress, i18n("Synchronization done"));
    mAccount->displayProgress();
    // Carry on

  case SYNC_STATE_SYNC_SUBFOLDERS:
    {
      if( mCurrentSubfolder ) {
	disconnect( mCurrentSubfolder, SIGNAL( folderComplete(KMFolderCachedImap*, bool) ),
		    this, SLOT( serverSyncInternal() ) );
	mCurrentSubfolder = 0;
      }

      if( mSubfoldersForSync.isEmpty() ) {
	mSyncState = SYNC_STATE_INITIAL;
	emit statusMsg( i18n("%1: Synchronization done").arg(name()) );
	emit folderComplete( this, TRUE );
	close();
      } else {
	mCurrentSubfolder = mSubfoldersForSync.front();
	mSubfoldersForSync.pop_front();
	connect( mCurrentSubfolder, SIGNAL( folderComplete(KMFolderCachedImap*, bool) ),
		 this, SLOT( serverSyncInternal() ) );

	// kdDebug(5006) << "Sync'ing subfolder " << mCurrentSubfolder->imapPath() << endl;
	assert( !mCurrentSubfolder->imapPath().isEmpty() );
	mCurrentSubfolder->setAccount( account() );
	mCurrentSubfolder->serverSync();
      }
    }
    break;

  default:
    kdDebug(5006) << "KMFolderCachedImap::serverSyncInternal() WARNING: no such state "
	      << mSyncState << endl;
  }
}

/* Connected to the imap account's connectionResult signal.
   Emitted when the slave connected or failed to connect.
*/
void KMFolderCachedImap::slotConnectionResult( int errorCode )
{
    disconnect( mAccount, SIGNAL( connectionResult(int) ),
                this, SLOT( slotConnectionResult(int) ) );
    if ( !errorCode ) // success
    {
        mSyncState = SYNC_STATE_CHECK_UIDVALIDITY;
        serverSyncInternal();
    }
    else // error (error message already shown by the account)
    {
        emit folderComplete(this, FALSE);
    }
}

/* find new messages (messages without a UID) */
QPtrList<KMMessage> KMFolderCachedImap::findNewMessages()
{
  //kdDebug(5006) << "KMFolderCachedImap::findNewMessages(), message count is " << count() << endl;
  QPtrList<KMMessage> result;
  for( int i = 0; i < count(); ++i ) {
    bool unget = !isMessage(i);
    KMMessage *msg = getMsg(i);
    if( !msg ) continue; /* what goes on if getMsg() returns 0? */
    if( msg->headerField("X-UID").isEmpty() ) {
      result.append( msg );
    } else {
      if (unget) unGetMsg(i);
    }
  }
  return result;
}

/* Upload new messages to server */
void KMFolderCachedImap::uploadNewMessages()
{
  QPtrList<KMMessage> newMsgs = findNewMessages();
  emit syncState( SYNC_STATE_PUT_MESSAGES, newMsgs.count() );
  mProgress += 10;
  //kdDebug() << name() << ": +10 (uploadNewMessages) -> " << mProgress << "%" << endl;
  if( !newMsgs.isEmpty() ) {
    emit statusMsg( i18n("%1: Uploading messages to server").arg(name()) );

    emit newState( name(), mProgress, i18n("Uploading messages to server"));
    CachedImapJob *job = new CachedImapJob( newMsgs, CachedImapJob::tPutMessage, this );
    connect( job, SIGNAL( finished() ), this, SLOT( serverSyncInternal() ) );
    job->start();
  } else {
    emit newState( name(), mProgress, i18n("No messages to upload to server"));

    serverSyncInternal();
  }
}

/* Upload new folders to server */
void KMFolderCachedImap::createNewFolders()
{
  QValueList<KMFolderCachedImap*> newFolders = findNewFolders();
  //emit syncState( SYNC_STATE_CREATE_SUBFOLDERS, newFolders.count() );
  mProgress += 10;
  //kdDebug() << name() << ": +10 (createNewFolders) -> " << mProgress << "%" << endl;
  if( !newFolders.isEmpty() ) {
    emit statusMsg( i18n("%1: Creating subfolders on server").arg(name()) );
    emit newState( name(), mProgress, i18n("Creating subfolders on server"));
    CachedImapJob *job = new CachedImapJob( newFolders, CachedImapJob::tAddSubfolders, this );
    connect( job, SIGNAL( finished() ), this, SLOT( serverSyncInternal() ) );
    job->start();
  } else {
    serverSyncInternal();
  }
}

QValueList<KMFolderCachedImap*> KMFolderCachedImap::findNewFolders()
{
  QValueList<KMFolderCachedImap*> newFolders;
  if( child() ) {
    KMFolderNode *node = child()->first();
    while( node ) {
      if( !node->isDir() ) {
	if( !node->isA("KMFolderCachedImap") ) {
	  kdDebug(5006) << "KMFolderCachedImap::findNewFolders(): ARGH!!! " << node->name()
		    << " is not an IMAP folder. It is a " << node->className() << endl;
	  node = child()->next();
	  assert(0);
	}
	KMFolderCachedImap* folder = static_cast<KMFolderCachedImap*>(node);
	if( folder->imapPath().isEmpty() ) newFolders << folder;
      }
      node = child()->next();
    }
  }
  return newFolders;
}

bool KMFolderCachedImap::deleteMessages()
{
  /* Delete messages from cache that are gone from the server */
  QPtrList<KMMsgBase> msgsForDeletion;
  reloadUidMap();
  // It is not possible to just go over all indices and remove them one by one
  // because the index list can get resized under us. So use msg pointers instead
  for( QMap<ulong,int>::Iterator it = uidMap.begin(); it != uidMap.end(); ++it ) {
    // kdDebug(5006) << "looking at " << it.key() << " in cache " << imapPath() << endl;
    if( !uidsOnServer.contains( it.key() ) ) {
      // kdDebug(5006) << "Uid" << it.key() << "(idx="<<it.data()<< ") not present on server" << endl;
      msgsForDeletion.append( mMsgList[it.data()] );
    }
  }

  if( !msgsForDeletion.isEmpty() ) {
    emit statusMsg( i18n("%1: Deleting removed messages from cache").arg(name()) );
    open();
    for( KMMsgBase *msg = msgsForDeletion.first(); msg; msg = msgsForDeletion.next() ) {
      int idx = find(msg);
      assert( idx != -1);
      removeMsg(idx);
    }
    compact();
    close();

    // It is quite possible that the list have been resized, so we need to rebuild
    // the entire uid map
    reloadUidMap();
  }

  //emit syncState( SYNC_STATE_DELETE_MESSAGES, uidsForDeletionOnServer.count() );

  mProgress += 10;
  //kdDebug() << name() << ": +10 (deleteMessages) -> " << mProgress << "%" << endl;
  emit newState( name(), mProgress, i18n("Deleting removed messages from server"));

  /* Delete messages from the server that we dont have anymore */
  if( !uidsForDeletionOnServer.isEmpty() ) {
    emit statusMsg( i18n("%1: Deleting removed messages from server").arg(name()) );
    QStringList sets = makeSets( uidsForDeletionOnServer, true );
    uidsForDeletionOnServer.clear();
    if( sets.count() > 1 ) {
      KMessageBox::error( 0, i18n("The number of messages scheduled for deletion is too large") );
    }
    //kdDebug(5006) << "Deleting " << sets.front() << " from sever folder " << imapPath() << endl;
    CachedImapJob *job = new CachedImapJob( sets.front(), CachedImapJob::tDeleteMessage,
						this );
    connect( job, SIGNAL( finished() ), this, SLOT( serverSyncInternal() ) );
    job->start();
    return true;
  } else {
    return false;
  }
}

void KMFolderCachedImap::checkUidValidity() {
  // IMAP root folders doesn't seem to have a UID validity setting.
  // Also, don't try the uid validity on new folders
  if( imapPath().isEmpty() || imapPath() == "/" )
    // Just proceed
    serverSyncInternal();
  else {
    mProgress += 10;
    //kdDebug() << name() << ": +10 (checkUidValidity) -> " << mProgress << "%" << endl;
    emit newState( name(), mProgress, i18n("Checking folder validity"));
    emit statusMsg( i18n("%1: Checking folder validity").arg(name()) );
    CachedImapJob *job = new CachedImapJob( FolderJob::tCheckUidValidity, this );
    connect( job, SIGNAL( finished() ), this, SLOT( serverSyncInternal() ) );
    job->start();
  }
}

/* This will only list the messages in a folder.
   No directory listing done*/
void KMFolderCachedImap::listMessages() {
  if( imapPath() == "/" ) {
    // Don't list messages on the root folder
    serverSyncInternal();
    return;
  }

  if( !mAccount->makeConnection() ) {
    emit listMessagesComplete();
    //emit folderComplete(this, FALSE);
    return;
  }
  uidsOnServer.clear();
  uidsForDeletionOnServer.clear();
  mMsgsForDownload.clear();
  KURL url = mAccount->getUrl();
  url.setPath(imapPath() + ";UID=1:*;SECTION=ENVELOPE");
  ImapAccountBase::jobData jd( url.url(), this );

  KIO::SimpleJob *newJob = KIO::get(url, FALSE, FALSE);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), newJob);
  mAccount->insertJob(newJob, jd);

  connect( newJob, SIGNAL( result( KIO::Job* ) ),
	   this, SLOT( slotGetLastMessagesResult( KIO::Job* ) ) );
  connect( newJob, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
	   this, SLOT( slotGetMessagesData( KIO::Job* , const QByteArray& ) ) );
}

void KMFolderCachedImap::slotGetLastMessagesResult(KIO::Job * job)
{
  getMessagesResult(job, true);
}


//-----------------------------------------------------------------------------
void KMFolderCachedImap::slotGetMessagesResult(KIO::Job * job)
{
  getMessagesResult(job, false);
}

// All this should be moved to CachedImapJob obviously...
void KMFolderCachedImap::slotGetMessagesData(KIO::Job * job, const QByteArray & data)
{
  KMAcctCachedImap::JobIterator it = mAccount->findJob(job);
  if ( it == mAccount->jobsEnd() ) { // Shouldn't happen
    kdDebug(5006) << "could not find job!?!?!" << endl;
    serverSyncInternal(); /* HACK^W Fix: we should at least try to keep going */
    return;
  }
  (*it).cdata += QCString(data, data.size() + 1);
  int pos = (*it).cdata.find("\r\n--IMAPDIGEST");
  if (pos > 0) {
    int p = (*it).cdata.find("\r\nX-uidValidity:");
    if (p != -1)
      setUidValidity((*it).cdata.mid(p + 17, (*it).cdata.find("\r\n", p+1) - p - 17));
    (*it).cdata.remove(0, pos);
  }
  pos = (*it).cdata.find("\r\n--IMAPDIGEST", 1);

  int flags;
  while (pos >= 0) {
    KMMessage *msg = new KMMessage;
    msg->fromString((*it).cdata.mid(16, pos - 16));
    flags = msg->headerField("X-Flags").toInt();
    bool ok;
    ulong uid = msg->headerField("X-UID").toULong(&ok);
    if( ok ) uidsOnServer.append( uid );
    if ( /*flags & 8 ||*/ uid <= lastUid()) {
      // kdDebug(5006) << "KMFolderCachedImap::slotGetMessagesData() : folder "<<name()<<" already has msg="<<msg->headerField("Subject") << ", UID="<<uid << ", lastUid = " << mLastUid << endl;
      /* If this message UID is not present locally, then it must
	 have been deleted by the user, so we delete it on the
	 server also.
      */
      KMMsgBase *existingMessage = findByUID(uid);
      if( !existingMessage ) {
	// kdDebug(5006) << "message with uid " << uid << " is gone from local cache. Must be deleted on server!!!" << endl;
	uidsForDeletionOnServer << uid;
      } else {
	/* The message is OK, update flags */
	existingMessage->setStatus( flagsToStatus( flags ) );
      }
      delete msg;
    } else {
      ulong size = msg->headerField("X-Length").toULong();
      mMsgsForDownload << KMail::CachedImapJob::MsgForDownload(uid, flags, size);
#if 0
      msg->setStatus(flagsToStatus(flags));
      open();
      //KMFolderCachedImapInherited::addMsg(msg, NULL);
      msg->setComplete( FALSE );
      addMsgInternal(msg, NULL);
      //sync();
      //if (count() > 1) unGetMsg(count() - 1);
      //mLastUid = uid;
      close();
#endif
      /*
      QValueList<KMMessage*> msgList;
      msgList << msg;
      (void)new CachedImapJob( msgList, CachedImapJob::tGetMessage, this );
      */
/*      if ((*it).total > 20 &&
        ((*it).done + 1) * 5 / (*it).total > (*it).done * 5 / (*it).total)
      {
        quiet(FALSE);
        quiet(TRUE);
      } */
    }
    (*it).cdata.remove(0, pos);
    (*it).done++;
    pos = (*it).cdata.find("\r\n--IMAPDIGEST", 1);
    mAccount->displayProgress();
  }
}

void KMFolderCachedImap::getMessagesResult(KIO::Job * job, bool lastSet)
{
  KMAcctCachedImap::JobIterator it = mAccount->findJob(job);
  if ( it == mAccount->jobsEnd() ) { // Shouldn't happen
    kdDebug(5006) << "could not find job!?!?!" << endl;
    serverSyncInternal(); /* HACK^W Fix: we should at least try to keep going */
    return;
  }
  if (job->error())
  {
    mAccount->slotSlaveError( mAccount->slave(), job->error(),
        job->errorText() );
    mContentState = imapNoInformation;
    emit folderComplete(this, FALSE);
  } else if (lastSet) mContentState = imapFinished;
  if (lastSet) quiet(FALSE);
  mAccount->removeJob(it);
  if (!job->error() && lastSet) {
    emit listMessagesComplete();
    //emit folderComplete(this, TRUE);
  }
}

void KMFolderCachedImap::slotProgress(unsigned long done, unsigned long total)
{
  //kdDebug() << "KMFolderCachedImap::slotProgress done=" << done << " total=" << total << "=> progress=" << mProgress + ( 20 * done ) / total << endl;
  // Progress info while retrieving new emails
  // (going from mProgress to mProgress+20)
  emit newState( name(), mProgress + (20 * done) / total, QString::null);
}

KMMsgStatus KMFolderCachedImap::flagsToStatus(int flags, bool newMsg)
{
  if (flags & 4) return KMMsgStatusFlag;
  if (flags & 2) return KMMsgStatusReplied;
  if (flags & 1) return KMMsgStatusOld;
  return (newMsg) ? KMMsgStatusNew : KMMsgStatusUnread;
}

QCString KMFolderCachedImap::statusToFlags(KMMsgStatus status)
{
  QCString flags = "";
  switch (status)
  {
    case KMMsgStatusNew:
    case KMMsgStatusUnread:
      break;
    case KMMsgStatusDeleted:
      flags = "\\DELETED";
      break;
    case KMMsgStatusReplied:
      flags = "\\SEEN \\ANSWERED";
      break;
    case KMMsgStatusFlag:
      flags = "\\SEEN \\FLAGGED";
      break;
    default:
      flags = "\\SEEN";
  }
  return flags;
}


void KMFolderCachedImap::setAccount(KMAcctCachedImap *aAccount)
{
  // kdDebug(5006) << "KMFolderCachedImap::setAccount( " << aAccount->name() << " )" << endl;
  // kdDebug(5006) << "classtype: " << protocol() << endl;
  assert( aAccount && aAccount->isA("KMAcctCachedImap") && this->protocol() == "cachedimap" );
  mAccount = aAccount;
  if( imapPath()=="/" ) aAccount->setFolder(this);
  if (!mChild) return;
#if 0
  KMFolderNode* node;
  for (node = mChild->first(); node; node = mChild->next())
  {
    if (!node->isDir())
      static_cast<KMFolderCachedImap*>(node)->setAccount(aAccount);
  }
#endif
}


// This synchronizes the subfolders with the server
bool KMFolderCachedImap::listDirectory()
{
  kdDebug(5006) << "KMFolderCachedImap::listDirectory " << "imapPath() = "
              << imapPath() << " mAccount->prefix() = " << mAccount->prefix() << kdBacktrace(10);
  reloadUidMap();

  mSubfolderState = imapInProgress;
  KURL url = mAccount->getUrl();
  url.setPath(imapPath() + ";TYPE="
	      + (mAccount->onlySubscribedFolders() ? "LSUB" : "LIST"));
  ImapAccountBase::jobData jd( url.url(), this );
  mSubfolderNames.clear();
  mSubfolderPaths.clear();
  mSubfolderMimeTypes.clear();

  // kdDebug(5006) << "listDirectory(): listing url " << url.url() << endl;
  if (!mAccount->makeConnection())
    return FALSE;

  KIO::SimpleJob *job = KIO::listDir(url, FALSE);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), job);
  mAccount->insertJob(job, jd);
  connect(job, SIGNAL(result(KIO::Job *)),
          this, SLOT(slotListResult(KIO::Job *)));
  connect(job, SIGNAL(entries(KIO::Job *, const KIO::UDSEntryList &)),
          this, SLOT(slotListEntries(KIO::Job *, const KIO::UDSEntryList &)));

  return TRUE;
}

void KMFolderCachedImap::slotListResult(KIO::Job * job)
{
  KMAcctCachedImap::JobIterator it = mAccount->findJob(job);
  if ( it == mAccount->jobsEnd() ) { // Shouldn't happen
    kdDebug(5006) << "could not find job!?!?!" << endl;
    serverSyncInternal(); /* HACK^W Fix: we should at least try to keep going */
    return;
  }

  if (job->error())
    mAccount->slotSlaveError( mAccount->slave(), job->error(), job->errorText() );

  mSubfolderState = imapFinished;
  mAccount->removeJob(it);

  if (!job->error()) {
    kernel->imapFolderMgr()->quiet(TRUE);
    createChildFolder();

    // Find all subfolders present on disk but not on the server
    KMFolderCachedImap *folder;
    KMFolderNode *node = mChild->first();
    QValueList<KMFolderCachedImap*> subfolders;
    while (node) {
      if (!node->isDir() ) {
	if( mSubfolderNames.findIndex(node->name()) == -1) {
	  // This subfolder isn't present on disk
	  kdDebug(5006) << node->name() << " isn't on the server." << endl;

	  folder = static_cast<KMFolderCachedImap*>(node);
	  if (folder->uidValidity().isEmpty()) {
	    // This folder doesn't have a uidValidity setting yet, so assume
	    // it's a new one the user made. Add it on the server
	    subfolders.append(folder);
	    node = mChild->next();
	  } else {
	    // The folder have a uidValidity setting, so it has been on the
	    // server before. Delete it locally.
	    KMFolderNode *n = mChild->next();
	    kernel->imapFolderMgr()->remove(static_cast<KMFolder*>(node));
	    node = n;
	  }
	} else {
	  // the folder was on the server too
	  node = mChild->next();
	}
      } else {
	// The folder was not a dir
	node = mChild->next();
      }
    }
    mAccount->displayProgress();
    serverSyncInternal();
  }
}


void KMFolderCachedImap::listDirectory2() {
  foldersForDeletionOnServer.clear();

  // Find all subfolders present on server but not on disk
  for (uint i = 0; i < mSubfolderNames.count(); i++) {
    KMFolderCachedImap *folder = 0;

    // Find the subdir, if already present
    KMFolderNode *node;
    for (node = mChild->first(); node; node = mChild->next())
      if (!node->isDir() && node->name() == mSubfolderNames[i]) break;

    if (!node) {
      // This folder is not present here
      QString uidCacheFile = path() + "/." + dotEscape(name()) + ".directory/."
	+ dotEscape(mSubfolderNames[i]) + ".uidcache";
      if( QFile::exists(uidCacheFile) ) {
	// This is an old folder that is deleted locally - delete it on the server
	unlink( QFile::encodeName( uidCacheFile ) );
	foldersForDeletionOnServer << mSubfolderPaths[i];
      } else {
	// This is a new folder, create the local cache
	folder = static_cast<KMFolderCachedImap*>
	  (mChild->createFolder(mSubfolderNames[i], false, KMFolderTypeCachedImap));
	if (folder) {
	  folder->close();
	  folder->setAccount(mAccount);
	  kernel->imapFolderMgr()->contentsChanged();
	} else {
	  kdDebug(5006) << "can't create folder " << mSubfolderNames[i] <<endl;
	}
      }
    } else {
      // kdDebug(5006) << "node " << node->name() << " is a " << node->className() << endl;
      if( node->isA("KMFolderCachedImap") )
	folder = static_cast<KMFolderCachedImap*>(node);
    }

    if (folder && folder->imapPath().isEmpty()) {
      // kdDebug(5006) << "folder("<<folder->name()<<")->imapPath()=" << folder->imapPath()
      // << "\nAssigning new imapPath " << mSubfolderPaths[i] << endl;
      // Write folder settings
      folder->setAccount(mAccount);
      folder->setNoContent(mSubfolderMimeTypes[i] == "inode/directory");
      folder->setImapPath(mSubfolderPaths[i]);
    }
  }

  kernel->imapFolderMgr()->quiet(FALSE);
  emit listComplete(this);
  serverSyncInternal();
}


//-----------------------------------------------------------------------------
void KMFolderCachedImap::slotListEntries(KIO::Job * job, const KIO::UDSEntryList & uds)
{
  // kdDebug(5006) << "KMFolderCachedImap::slotListEntries("<<name()<<")" << endl;
  KMAcctCachedImap::JobIterator it = mAccount->findJob(job);
  if (it == mAccount->jobsEnd()) return;

  QString name;
  KURL url;
  QString mimeType;
  for (KIO::UDSEntryList::ConstIterator udsIt = uds.begin();
    udsIt != uds.end(); udsIt++)
  {
    // kdDebug(5006) << "slotListEntries start" << endl;
    mimeType = QString::null;

    // Find the info on this subfolder
    for (KIO::UDSEntry::ConstIterator eIt = (*udsIt).begin();
      eIt != (*udsIt).end(); eIt++)
    {
      //kdDebug(5006) << "slotListEntries got type " << (*eIt).m_uds << " str " << (*eIt).m_str << endl;
      if ((*eIt).m_uds == KIO::UDS_NAME)
        name = (*eIt).m_str;
      else if ((*eIt).m_uds == KIO::UDS_URL)
        url = KURL((*eIt).m_str, 106); // utf-8
      else if ((*eIt).m_uds == KIO::UDS_MIME_TYPE)
        mimeType = (*eIt).m_str;
    }

    // kdDebug(5006) << "slotListEntries end. mimetype = " << mimeType
    //      << ", name = " << name << ", path = " << url.path() << endl;

    // If this was a subfolder, add it to the list
    if ((mimeType == "inode/directory" || mimeType == "message/digest"
        || mimeType == "message/directory")
        && name != ".." && (mAccount->hiddenFolders() || name.at(0) != '.'))
    {
      // Some servers send _lots_ of duplicates
      if (mSubfolderNames.findIndex(name) == -1) {
	mSubfolderNames.append(name);
	mSubfolderPaths.append(url.path());
	mSubfolderMimeTypes.append(mimeType);
      }
    }
  }
}

void KMFolderCachedImap::slotSimpleData(KIO::Job * job, const QByteArray & data)
{
  KMAcctCachedImap::JobIterator it = mAccount->findJob(job);
  if (it == mAccount->jobsEnd()) return;
  QBuffer buff((*it).data);
  buff.open(IO_WriteOnly | IO_Append);
  buff.writeBlock(data.data(), data.size());
  buff.close();
}


QStringList KMFolderCachedImap::makeSets(QStringList& uids, bool sort)
{
  QValueList<ulong> tmp;
  for ( QStringList::Iterator it = uids.begin(); it != uids.end(); ++it )
    tmp.append( (*it).toInt() );
  return makeSets(tmp, sort);
}

QStringList KMFolderCachedImap::makeSets(QValueList<ulong>& uids, bool sort)
{
  QStringList sets;
  QString set;

  if (uids.size() == 1)
  {
    sets.append(QString::number(uids.first()));
    return sets;
  }

  if (sort) qHeapSort(uids);

  ulong last = 0;
  // needed to make a uid like 124 instead of 124:124
  bool inserted = false;
  /* iterate over uids and build sets like 120:122,124,126:150 */
  for ( QValueList<ulong>::Iterator it = uids.begin(); it != uids.end(); ++it )
  {
    if (it == uids.begin() || set.isEmpty()) {
      set = QString::number(*it);
      inserted = true;
    } else
    {
      if (last+1 != *it)
      {
        // end this range
        if (inserted)
          set += ',' + QString::number(*it);
        else
          set += ':' + QString::number(last) + ',' + QString::number(*it);
        inserted = true;
        if (set.length() > 100)
        {
          // just in case the server has a problem with longer lines..
          sets.append(set);
          set = "";
        }
      } else {
        inserted = false;
      }
    }
    last = *it;
  }
  // last element
  if (!inserted)
    set += ':' + QString::number(uids.last());

  if (!set.isEmpty()) sets.append(set);

  return sets;
}

FolderJob*
KMFolderCachedImap::doCreateJob( KMMessage *msg, FolderJob::JobType jt, KMFolder *folder ) const
{
  QPtrList<KMMessage> msgList;
  msgList.append( msg );
  CachedImapJob *job = new CachedImapJob( msgList, jt, static_cast<KMFolderCachedImap*>( folder ) );
  return job;
}

FolderJob*
KMFolderCachedImap::doCreateJob( QPtrList<KMMessage>& msgList, const QString& sets,
                                 FolderJob::JobType jt, KMFolder *folder ) const
{
  //FIXME: how to handle sets here?
  Q_UNUSED( sets );
  CachedImapJob *job = new CachedImapJob( msgList, jt, static_cast<KMFolderCachedImap*>( folder ) );
  return job;
}

#include "kmfoldercachedimap.moc"
