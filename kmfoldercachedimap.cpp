#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>

#include "kmfoldercachedimap.h"
#include "kmfolderimap.h"
#include "kmundostack.h"
#include "kmfoldermgr.h"
#include "kmmessage.h"
#include "kmacctcachedimap.h"
#include "kmacctmgr.h"
#include "imapprogressdialog.h"

#include <kapplication.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>
#include <kio/global.h>
#include <kio/scheduler.h>
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

  prog = 0;
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
  // kdDebug() << "KMFolderCachedImap::removeMsg(" << idx << ", " << imapQuiet << ")" << endl;

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

  new KMCachedImapJob( aName, KMCachedImapJob::tRenameFolder, this );
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
  // kdDebug() << "KMFolderCachedImap::lastUid("<<name()<<") = " << mLastUid << endl;
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
  // kdDebug() << "KMFolderCachedImap::serverSync(), imapPath()=" << imapPath() << ", path()="
  //    << path() << " name()="<< name() << endl;

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
//   kdDebug() << "KMFolderCachedImap::serverSyncInternal(), " << name()
// 	    << " state = " << state2String(mSyncState) << endl;

  // Don't let the states continue into the next one. Instead use sync(); break;
  // to get the debug output
  switch( mSyncState ) {
  case SYNC_STATE_INITIAL:
    prog = 0;
    emit statusMsg( i18n("%1: Synchronizing").arg(name()) );
    emit newState( name(), prog, i18n("Syncronizing"));
    // emit syncState( SYNC_STATE_INITIAL, prog /*dummy value*/ );

    mSyncState = SYNC_STATE_CHECK_UIDVALIDITY;
    open();
    serverSyncInternal();
    break;

  case SYNC_STATE_CHECK_UIDVALIDITY:
    mSyncState = SYNC_STATE_CREATE_SUBFOLDERS;
    checkUidValidity();
    break;

  case SYNC_STATE_CREATE_SUBFOLDERS:
    mSyncState = SYNC_STATE_PUT_MESSAGES;
    createNewFolders();
    break;
  case SYNC_STATE_PUT_MESSAGES:
    mSyncState = SYNC_STATE_LIST_SUBFOLDERS;
    uploadNewMessages();
    break;
  case SYNC_STATE_LIST_SUBFOLDERS:
    mSyncState = SYNC_STATE_LIST_SUBFOLDERS2;
    prog += 10;
    emit statusMsg( i18n("%1: Retrieving folderlist").arg(name()) );
    emit newState( name(), prog, i18n("Retrieving folderlist"));
    if( !listDirectory() ) {
      mSyncState = SYNC_STATE_INITIAL;
      KMessageBox::error(0, i18n("Error during listDirectory()"));
    }
    break;
  case SYNC_STATE_LIST_SUBFOLDERS2:
    mSyncState = SYNC_STATE_DELETE_SUBFOLDERS;
    prog += 10;
    emit newState( name(), prog, i18n("Retrieving subfolders"));
    listDirectory2();
    break;
  case SYNC_STATE_DELETE_SUBFOLDERS:
    mSyncState = SYNC_STATE_LIST_MESSAGES;
    emit syncState( SYNC_STATE_DELETE_SUBFOLDERS, foldersForDeletionOnServer.count() );
    prog += 10;
    if( !foldersForDeletionOnServer.isEmpty() ) {
      emit statusMsg( i18n("%1: Deleting folders %2 from server").arg(name())
		      .arg( foldersForDeletionOnServer.join(", ") ) );
      emit newState( name(), prog, i18n("Deleting folders from server"));
      KMCachedImapJob* job = new KMCachedImapJob( foldersForDeletionOnServer,
						  KMCachedImapJob::tDeleteFolders, this );
      connect( job, SIGNAL( finished() ), this, SLOT( serverSyncInternal() ) );
    } else {
      emit newState( name(), prog, i18n("No folders to delete from server"));
      serverSyncInternal();
    }
    break;

  case SYNC_STATE_LIST_MESSAGES:
    mSyncState = SYNC_STATE_DELETE_MESSAGES;
    prog += 10;
    emit statusMsg( i18n("%1: Retrieving messagelist").arg(name()) );
    emit newState( name(), prog, i18n("Retrieving messagelist"));
    // emit syncState( SYNC_STATE_LIST_MESSAGES, foldersForDeletionOnServer.count() );
    listMessages();
    break;
  case SYNC_STATE_DELETE_MESSAGES:
    mSyncState = SYNC_STATE_EXPUNGE_MESSAGES;
    if( deleteMessages() ) {
    // Fine, we will continue with the next state
    } else {
      // No messages to delete, skip to GET_MESSAGES
      prog += 10;
      emit newState( name(), prog, i18n("No messages to delete..."));
      mSyncState = SYNC_STATE_GET_MESSAGES;
      serverSyncInternal();
    }
    break;
  case SYNC_STATE_EXPUNGE_MESSAGES:
    mSyncState = SYNC_STATE_GET_MESSAGES;
    {
      prog += 10;
      emit statusMsg( i18n("%1: Expunging deleted messages").arg(name()) );
      emit newState( name(), prog, i18n("Expunging deleted messages"));
      KMCachedImapJob *job = new KMCachedImapJob( QString::null,
						  KMCachedImapJob::tExpungeFolder, this );
      connect( job, SIGNAL( finished() ), this, SLOT( serverSyncInternal() ) );
    }
    break;
  case SYNC_STATE_GET_MESSAGES:
    mSyncState = SYNC_STATE_HANDLE_INBOX;
    {
      prog += 10;
      //emit syncState( SYNC_STATE_GET_MESSAGES, uidsForDownload.count() );
      if( !uidsForDownload.isEmpty() ) {
	emit statusMsg( i18n("%1: Retrieving new messages").arg(name()) );
	emit newState( name(), prog, i18n("Retrieving new messages"));
	KMCachedImapJob *job = new KMCachedImapJob( uidsForDownload,
						    KMCachedImapJob::tGetMessage,
						    this, flagsForDownload );
	connect( job, SIGNAL( finished() ), this, SLOT( serverSyncInternal() ) );
	uidsForDownload.clear();
	flagsForDownload.clear();
      } else {
	// kdDebug() << "No new messages from server(" << imapPath() << "):" << endl;
	emit newState( name(), prog, i18n("No new messages from server"));
	serverSyncInternal();
      }
    }
    break;
  case SYNC_STATE_HANDLE_INBOX:
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
    serverSyncInternal();
    break;

  case SYNC_STATE_FIND_SUBFOLDERS:
    {
      emit newState( name(), prog, i18n("Updating cache file"));

      // last state got new messages, update cache file
      writeUidCache();

      mSyncState = SYNC_STATE_SYNC_SUBFOLDERS;
      mSubfoldersForSync.clear();
      mCurrentSubfolder = 0;
      if( child() ) {
	KMFolderNode *node = child()->first();
	while( node ) {
	  if( !node->isDir() ) {
	    // kdDebug() << "child folder " << node->name() << " is a "
            // << node->className() << endl;
	    if ( static_cast<KMFolderCachedImap*>(node)->imapPath() != "" )
	      // Only sync folders that have been accepted by the server
	      mSubfoldersForSync << static_cast<KMFolderCachedImap*>(node);
	  }
	  node = child()->next();
	}
      }
    }
    serverSyncInternal();
    break;

  case SYNC_STATE_SYNC_SUBFOLDERS:
    {
      if( mCurrentSubfolder ) {
	disconnect( mCurrentSubfolder, SIGNAL( folderComplete(KMFolderCachedImap*, bool) ),
		    this, SLOT( serverSyncInternal() ) );
	mCurrentSubfolder = 0;
      }

      prog += 10;
      //emit syncState( SYNC_STATE_SYNC_SUBFOLDERS, mSubfoldersForSync.count() );
      emit newState( name(), prog, i18n("Synchronization done"));

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

	// kdDebug() << "Sync'ing subfolder " << mCurrentSubfolder->imapPath() << endl;
	assert( mCurrentSubfolder->imapPath() != "" );
	mCurrentSubfolder->setAccount( account() );
	mCurrentSubfolder->serverSync();
      }
    }
    break;

  default:
    kdDebug() << "KMFolderCachedImap::serverSyncInternal() WARNING: no such state "
	      << mSyncState << endl;
  }
  // kdDebug() << "KMFolderCachedImap::serverSyncInternal() done"<<endl;
}

/* find new messages (messages without a UID) */
QValueList<KMMessage*> KMFolderCachedImap::findNewMessages()
{
  //kdDebug() << "KMFolderCachedImap::findNewMessages(), message count is " << count() << endl;
  QValueList<KMMessage*> result;
  for( int i = 0; i < count(); ++i ) {
    bool unget = !isMessage(i);
    KMMessage *msg = getMsg(i);
    if( !msg ) continue; /* what goes on if getMsg() returns 0? */
    if( msg->headerField("X-UID").isEmpty() ) {
      result << msg;
    } else {
      if (unget) unGetMsg(i);
    }
  }
  return result;
}

/* Upload new messages to server */
void KMFolderCachedImap::uploadNewMessages()
{
  QValueList<KMMessage*> newMsgs = findNewMessages();
  emit syncState( SYNC_STATE_PUT_MESSAGES, newMsgs.count() );
  prog += 10;
  if( !newMsgs.isEmpty() ) {
    emit statusMsg( i18n("%1: Uploading messages to server").arg(name()) );

    emit newState( i18n("%1").arg(name()) , prog, i18n("Uploading messages to server"));
    KMCachedImapJob *job = new KMCachedImapJob( newMsgs, KMCachedImapJob::tPutMessage, this );
    connect( job, SIGNAL( finished() ), this, SLOT( serverSyncInternal() ) );
  } else {
    emit newState( i18n("%1").arg(name()) , prog, i18n("No messages to upload to server"));

    serverSyncInternal();
  }
}

/* Upload new folders to server */
void KMFolderCachedImap::createNewFolders()
{
  QValueList<KMFolderCachedImap*> newFolders = findNewFolders();
  //emit syncState( SYNC_STATE_CREATE_SUBFOLDERS, newFolders.count() );
   prog += 10;
  if( !newFolders.isEmpty() ) {
    emit statusMsg( i18n("%1: Creating subfolders on server").arg(name()) );
    emit newState( i18n("%1").arg(name()) , prog, i18n("Creating subfolders on server"));
    connect( new KMCachedImapJob( newFolders, KMCachedImapJob::tAddSubfolders, this ),
	     SIGNAL( finished() ), this, SLOT( serverSyncInternal() ) );
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
	  kdDebug() << "KMFolderCachedImap::findNewFolders(): ARGH!!! " << node->name()
		    << " is not an IMAP folder. It is a " << node->className() << endl;
	  node = child()->next();
	  assert(0);
	}
	KMFolderCachedImap* folder = static_cast<KMFolderCachedImap*>(node);
	if( folder->imapPath() == "" ) newFolders << folder;
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
    // kdDebug() << "looking at " << it.key() << " in cache " << imapPath() << endl;
    if( !uidsOnServer.contains( it.key() ) ) {
      // kdDebug() << "Uid" << it.key() << "(idx="<<it.data()<< ") not present on server" << endl;
      msgsForDeletion.append( mMsgList[it.data()] );
    }
  }

  if( !msgsForDeletion.isEmpty() ) {
    emit statusMsg( i18n("%1: Deleting removed messages from cache").arg(name()) );
    open();
    for( KMMsgBase *msg = msgsForDeletion.first(); msg; msg = msgsForDeletion.next() )
      KMFolder::removeMsg( msg );
    compact();
    close();

    // It is quite possible that the list have been resized, so we need to rebuild
    // the entire uid map
    reloadUidMap();
  }

  //emit syncState( SYNC_STATE_DELETE_MESSAGES, uidsForDeletionOnServer.count() );

  prog += 10;
   emit newState( i18n("%1").arg(name()) , prog, i18n("Deleting removed messages from server"));

  /* Delete messages from the server that we dont have anymore */
  if( !uidsForDeletionOnServer.isEmpty() ) {
    emit statusMsg( i18n("%1: Deleting removed messages from server").arg(name()) );
    QStringList sets = makeSets( uidsForDeletionOnServer, true );
    uidsForDeletionOnServer.clear();
    if( sets.count() > 1 ) {
      KMessageBox::error( 0, i18n("The number of messages scheduled for deletion is too large") );
    }
    //kdDebug() << "Deleting " << sets.front() << " from sever folder " << imapPath() << endl;
    KMCachedImapJob *job = new KMCachedImapJob( sets.front(), KMCachedImapJob::tDeleteMessage,
						this );
    connect( job, SIGNAL( finished() ), this, SLOT( serverSyncInternal() ) );
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
    prog += 10;
    emit newState( i18n("%1").arg(name()) , prog, i18n("Checking folder validity"));
    emit statusMsg( i18n("%1: Checking folder validity").arg(name()) );
    KMCachedImapJob *job = new KMCachedImapJob( KMCachedImapJob::tCheckUidValidity, this );
    connect( job, SIGNAL( finished() ), this, SLOT( serverSyncInternal() ) );
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
  uidsForDownload.clear();
  flagsForDownload.clear();
  KURL url = mAccount->getUrl();
  url.setPath(imapPath() + ";UID=1:*;SECTION=ENVELOPE");
  KMAcctCachedImap::jobData jd( url.url(), this );

  KIO::SimpleJob *newJob = KIO::get(url, FALSE, FALSE);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), newJob);
  mAccount->mapJobData.insert(newJob, jd);

  connect( newJob, SIGNAL( result( KIO::Job* ) ),
	   this, SLOT( slotGetLastMessagesResult( KIO::Job* ) ) );
  connect( newJob, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
	   this, SLOT( slotGetMessagesData( KIO::Job* , const QByteArray& ) ) );
  mAccount->displayProgress();
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


void KMFolderCachedImap::slotGetMessagesData(KIO::Job * job, const QByteArray & data)
{
  QMap<KIO::Job *, KMAcctCachedImap::jobData>::Iterator it = mAccount->mapJobData.find(job);
  if (it == mAccount->mapJobData.end())
    return;

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
      // kdDebug() << "KMFolderCachedImap::slotGetMessagesData() : folder "<<name()<<" already has msg="<<msg->headerField("Subject") << ", UID="<<uid << ", lastUid = " << mLastUid << endl;
      /* If this message UID is not present locally, then it must
	 have been deleted by the user, so we delete it on the
	 server also.
      */
      KMMsgBase *existingMessage = findByUID(uid);
      if( !existingMessage ) {
	// kdDebug() << "message with uid " << uid << " is gone from local cache. Must be deleted on server!!!" << endl;
	uidsForDeletionOnServer << uid;
      } else {
	/* The message is OK, update flags */
	existingMessage->setStatus( flagsToStatus( flags ) );
      }
      delete msg;
    } else {
      uidsForDownload << uid;
      flagsForDownload << flags;
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
      (void)new KMCachedImapJob( msgList, KMCachedImapJob::tGetMessage, this );
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
  QMap<KIO::Job *, KMAcctCachedImap::jobData>::Iterator it =
    mAccount->mapJobData.find(job);
  if (it == mAccount->mapJobData.end()) return;
  assert(it != mAccount->mapJobData.end());
  if (job->error())
  {
    mAccount->slotSlaveError( mAccount->slave(), job->error(),
        job->errorText() );
    mContentState = imapNoInformation;
    emit folderComplete(this, FALSE);
  } else if (lastSet) mContentState = imapFinished;
  if (lastSet) quiet(FALSE);
  if (mAccount->slave()) mAccount->mapJobData.remove(it);
  mAccount->displayProgress();
  if (!job->error() && lastSet) {
    emit listMessagesComplete();
    //emit folderComplete(this, TRUE);
  }
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
  // kdDebug() << "KMFolderCachedImap::setAccount( " << aAccount->name() << " )" << endl;
  assert( aAccount && aAccount->isA("KMAcctCachedImap") );
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
bool KMFolderCachedImap::listDirectory() {
  // kdDebug() << "KMFolderCachedImap::listDirectory " << "imapPath() = "
  //    << imapPath() << " mAccount->prefix() = " << mAccount->prefix() << endl;
  reloadUidMap();

  mSubfolderState = imapInProgress;
  KURL url = mAccount->getUrl();
  url.setPath(imapPath() + ";TYPE="
	      + (mAccount->onlySubscribedFolders() ? "LSUB" : "LIST"));
  KMAcctCachedImap::jobData jd( url.url(), this );
  mSubfolderNames.clear();
  mSubfolderPaths.clear();
  mSubfolderMimeTypes.clear();

  // kdDebug() << "listDirectory(): listing url " << url.url() << endl;
  if (!mAccount->makeConnection())
    return FALSE;

  KIO::SimpleJob *job = KIO::listDir(url, FALSE);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), job);
  mAccount->mapJobData.insert(job, jd);
  connect(job, SIGNAL(result(KIO::Job *)),
          this, SLOT(slotListResult(KIO::Job *)));
  connect(job, SIGNAL(entries(KIO::Job *, const KIO::UDSEntryList &)),
          this, SLOT(slotListEntries(KIO::Job *, const KIO::UDSEntryList &)));
  mAccount->displayProgress();

  return TRUE;
}

void KMFolderCachedImap::slotListResult(KIO::Job * job)
{
  QMap<KIO::Job *, KMAcctCachedImap::jobData>::Iterator it = mAccount->mapJobData.find(job);
  if (it == mAccount->mapJobData.end()) {
    kdDebug() << "could not find job!?!?!" << endl;
    serverSyncInternal(); /* HACK^W Fix: we should at least try to keep going */
    return;
  }

  if (job->error())
    mAccount->slotSlaveError( mAccount->slave(), job->error(), job->errorText() );

  mSubfolderState = imapFinished;
  if (mAccount->slave()) mAccount->mapJobData.remove(it);

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
	  if (folder->uidValidity() == "") {
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
	// This is an old folder that is deleted locally. We need to schedule a deletion
	// on the server. TODO: Actually do it!
	// kdDebug() << "unlink( " << QFile::encodeName( uidCacheFile ) << " )" << endl;
	unlink( QFile::encodeName( uidCacheFile ) );
	foldersForDeletionOnServer << mSubfolderPaths[i];
      } else {
	// Create new folder
	// kdDebug() << "Creating new KMFolderCachedImap folder with name "
	// << mSubfolderNames[i] << endl;
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
      // kdDebug() << "node " << node->name() << " is a " << node->className() << endl;
      if( node->isA("KMFolderCachedImap") )
	folder = static_cast<KMFolderCachedImap*>(node);
    }

    if (folder && folder->imapPath() == "") {
      // kdDebug() << "folder("<<folder->name()<<")->imapPath()=" << folder->imapPath()
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
  // kdDebug() << "KMFolderCachedImap::slotListEntries("<<name()<<")" << endl;
  QMap<KIO::Job *, KMAcctCachedImap::jobData>::Iterator it =
    mAccount->mapJobData.find(job);
  if (it == mAccount->mapJobData.end()) return;

  QString name;
  KURL url;
  QString mimeType;
  for (KIO::UDSEntryList::ConstIterator udsIt = uds.begin();
    udsIt != uds.end(); udsIt++)
  {
    // kdDebug() << "slotListEntries start" << endl;
    mimeType = QString::null;

    // Find the info on this subfolder
    for (KIO::UDSEntry::ConstIterator eIt = (*udsIt).begin();
      eIt != (*udsIt).end(); eIt++)
    {
      //kdDebug() << "slotListEntries got type " << (*eIt).m_uds << " str " << (*eIt).m_str << endl;
      if ((*eIt).m_uds == KIO::UDS_NAME)
        name = (*eIt).m_str;
      else if ((*eIt).m_uds == KIO::UDS_URL)
        url = KURL((*eIt).m_str, 106); // utf-8
      else if ((*eIt).m_uds == KIO::UDS_MIME_TYPE)
        mimeType = (*eIt).m_str;
    }

    // kdDebug() << "slotListEntries end. mimetype = " << mimeType
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
  QMap<KIO::Job *, KMAcctCachedImap::jobData>::Iterator it = mAccount->mapJobData.find(job);
  if (it == mAccount->mapJobData.end()) return;
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



KMCachedImapJob::KMCachedImapJob( const QValueList<ulong>& uids, JobType type,
				  KMFolderCachedImap* folder, const QValueList<int>& flags )
  : QObject( 0, "cached_imap_job" ), mFolder( folder ), mUidList( uids ), mFlags( flags ),
    mMsg(0)
{
  init( type );
}

KMCachedImapJob::KMCachedImapJob( const QValueList<KMMessage*>& msgs, JobType type,
				  KMFolderCachedImap* folder )
  : QObject( 0,"cached_imap_job" ), mFolder( folder ), mMsgList( msgs ), mMsg(0)
{
  init( type );
}

KMCachedImapJob::KMCachedImapJob( const QValueList<KMFolderCachedImap*>& fList,
				  JobType type, KMFolderCachedImap* folder )
  : QObject( 0, "cached_imap_job" ), mFolder( folder ), mFolderList( fList ), mMsg(0)
{
  init( type );
}

KMCachedImapJob::KMCachedImapJob( const QString& uids, JobType type,
				  KMFolderCachedImap* folder )
  : QObject( 0, "cached_imap_job" ), mFolder(folder), mMsg(0), mString(uids)
{
  assert( folder );
  init( type );
}

KMCachedImapJob::KMCachedImapJob( const QStringList& folderpaths, JobType type,
				  KMFolderCachedImap* folder )
  : QObject( 0, "cached_imap_job" ), mFolder( folder ), mFolderPathList( folderpaths ), mMsg(0)
{
  assert( folder );
  init( type );
}

KMCachedImapJob::KMCachedImapJob( JobType type, KMFolderCachedImap* folder )
  : QObject( 0, "cached_imap_job" ), mFolder( folder ), mMsg( 0 ), mJob( 0 )
{
  assert( folder );
  init( type );
}

KMCachedImapJob::~KMCachedImapJob()
{
  mAccount->displayProgress();
  if( mJob ) {
    // kdDebug() << "~KMCachedImapJob(): Removing jobdata from mapJobData" << endl;
    mAccount->mapJobData.remove(mJob);
  }

  /* // TODO(steffen): Handle transferinpro...
     if ( !(*it).msgList.isEmpty() ) {
     for ( KMMessage* msg = (*it).msgList.first(); msg; msg = (*it).msgList.next() )
     msg->setTransferInProgress(false);
     }
  */
  //if( mMsg ) mMsg->setTransferInProgress(false);
  mAccount->displayProgress();

  // kdDebug() << "~KMCachedImapJob(): Removing this from joblist" << endl;
  mAccount->mJobList.remove(this);

  if( !mPassiveDestructor )
    emit finished();
}

void KMCachedImapJob::init( JobType type )
{
  mType = type;

  if( !mFolder ) {
    if( !mMsgList.isEmpty() ) {
      mFolder = static_cast<KMFolderCachedImap*>(mMsgList.front()->parent());
    }
  }
  assert( mFolder );
  mAccount = mFolder->account();
  assert( mAccount != 0 );
  if( !mAccount->makeConnection() ) {
    // No connection to the IMAP server
    kdDebug() << "mAccount->makeConnection() failed" << endl;
    mPassiveDestructor = true;
    delete this;
    return;
  } else
    mPassiveDestructor = false;

  // All necessary conditions have been met. Register this job
  mAccount->mJobList.append(this);

  switch( mType ) {
  case tGetMessage:       slotGetNextMessage();     break;
  case tPutMessage:       slotPutNextMessage();     break;
  case tDeleteMessage:    deleteMessages(mString);  break;
  case tExpungeFolder:    expungeFolder();          break;
  case tAddSubfolders:    slotAddNextSubfolder();   break;
  case tDeleteFolders:    slotDeleteNextFolder();   break;
  case tCheckUidValidity: checkUidValidity();       break;
  case tRenameFolder:     renameFolder(mString);    break;
  default:
    assert( 0 );
  }
}

void KMCachedImapJob::deleteMessages( const QString& uids )
{
  KURL url = mAccount->getUrl();
  url.setPath( mFolder->imapPath() + QString::fromLatin1(";UID=%1").arg(uids) );

  KIO::SimpleJob *job = KIO::file_delete( url, FALSE );
  KIO::Scheduler::assignJobToSlave( mAccount->slave(), job );
  KMAcctCachedImap::jobData jd;
  mAccount->mapJobData.insert( job, jd );
  connect( job, SIGNAL( result(KIO::Job *) ), this, SLOT( slotDeleteResult(KIO::Job *) ) );
  mAccount->displayProgress();
}

void KMCachedImapJob::expungeFolder()
{
  KURL url = mAccount->getUrl();
  // Special URL that means EXPUNGE
  url.setPath( mFolder->imapPath() + QString::fromLatin1(";UID=*") );

  KIO::SimpleJob *job = KIO::file_delete( url, FALSE );
  KIO::Scheduler::assignJobToSlave( mAccount->slave(), job );
  KMAcctCachedImap::jobData jd( url.url() );
  mAccount->mapJobData.insert( job, jd );
  connect( job, SIGNAL( result(KIO::Job *) ), this, SLOT( slotDeleteResult(KIO::Job *) ) );
  mAccount->displayProgress();
}

void KMCachedImapJob::slotDeleteResult( KIO::Job * job )
{
  QMap<KIO::Job *, KMAcctCachedImap::jobData>::Iterator it = mAccount->mapJobData.find(job);
  if (it != mAccount->mapJobData.end())
    // What?
    if (mAccount->slave()) mAccount->mJobList.remove(this);

  if (job->error())
    mAccount->slotSlaveError( mAccount->slave(), job->error(), job->errorText() );

  delete this;
}

void KMCachedImapJob::slotGetNextMessage(KIO::Job * job)
{
  if (job) {
    if (job->error()) {
      mAccount->slotSlaveError( mAccount->slave(), job->error(), job->errorText() );
      delete this;
      return;
    }

    QMap<KIO::Job *, KMAcctCachedImap::jobData>::Iterator it = mAccount->mapJobData.find(job);
    if ( it == mAccount->mapJobData.end() ) {
      delete this;
      return;
    }

    if ((*it).data.size() > 0) {
      QString uid = mMsg->headerField("X-UID");
      (*it).data.resize((*it).data.size() + 1);
      (*it).data[(*it).data.size() - 1] = '\0';
      mMsg->fromString(QCString((*it).data));
      //int idx = mFolder->find(mMsg);
      //if( idx >= 0 ) mFolder->take(idx);
      //else kdDebug() << "weird, message not in folder!?!" << endl;
      mMsg->setHeaderField("X-UID",uid);

      mMsg->setTransferInProgress( FALSE );
      mMsg->setComplete( TRUE );
      mFolder->addMsgInternal(mMsg);
      emit messageRetrieved(mMsg);
      /*mFolder->unGetMsg(idx);*/ // Is this OK? /steffen
    } else {
      emit messageRetrieved(NULL);
    }
    mMsg = NULL;

    if (mAccount->slave()) mAccount->mapJobData.remove(it);
    mAccount->displayProgress();
  }

  if( mUidList.isEmpty() ) {
    //emit messageRetrieved(mMsg);
    if (mAccount->slave()) mAccount->mJobList.remove(this);
    delete this;
    return;
  }

  mUid = mUidList.front(); mUidList.pop_front();
  if( mFlags.isEmpty() ) mFlag = -1;
  else {
    mFlag = mFlags.front(); mFlags.pop_front();
  }
  mMsg = new KMMessage;
  mMsg->setHeaderField("X-UID",QString::number(mUid));
  if( mFlag > 0 ) mMsg->setStatus( KMFolderCachedImap::flagsToStatus(mFlag) );
  KURL url = mAccount->getUrl();
  url.setPath(mFolder->imapPath() + QString(";UID=%1").arg(mUid));

  KMAcctCachedImap::jobData jd;
  mMsg->setTransferInProgress(TRUE);
  KIO::SimpleJob *simpleJob = KIO::get(url, FALSE, FALSE);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), simpleJob);
  mJob = simpleJob;
  mAccount->mapJobData.insert(mJob, jd);
  connect(mJob, SIGNAL(result(KIO::Job *)),
          this, SLOT(slotGetNextMessage(KIO::Job *)));
  connect(mJob, SIGNAL(data(KIO::Job *, const QByteArray &)),
          mFolder, SLOT(slotSimpleData(KIO::Job *, const QByteArray &)));
  mAccount->displayProgress();
}

void KMCachedImapJob::slotPutNextMessage()
{
  if( mMsgList.isEmpty() ) {
    mAccount->mJobList.remove(this);
    delete this;
    return;
  }

  mMsg = mMsgList.front(); mMsgList.pop_front();
  assert( mMsg );

  KURL url = mAccount->getUrl();
  url.setPath(mFolder->imapPath() + ";SECTION="
	      + QString::fromLatin1(KMFolderCachedImap::statusToFlags(mMsg->status())));
  KMAcctCachedImap::jobData jd( url.url() );

  QCString cstr(mMsg->asString());
  int a = cstr.find("\nX-UID: ");
  int b = cstr.find('\n', a);
  if (a != -1 && b != -1 && cstr.find("\n\n") > a) cstr.remove(a, b-a);
  mData.resize(cstr.length() + cstr.contains('\n'));
  unsigned int i = 0;
  for( char *ch = cstr.data(); *ch; ch++ ) {
    if ( *ch == '\n' ) {
      mData.at(i) = '\r';
      i++;
    }
    mData.at(i) = *ch; i++;
  }
  jd.data = mData;

  mMsg->setTransferInProgress(TRUE);
  KIO::SimpleJob *simpleJob = KIO::put(url, 0, FALSE, FALSE, FALSE);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), simpleJob);
  mJob = simpleJob;
  mAccount->mapJobData.insert(mJob, jd);
  connect( mJob, SIGNAL( result(KIO::Job *) ), SLOT( slotPutMessageResult(KIO::Job *) ) );
  connect( mJob, SIGNAL( dataReq(KIO::Job *, QByteArray &) ),
	   SLOT( slotPutMessageDataReq(KIO::Job *, QByteArray &) ) );
  connect( mJob, SIGNAL( data(KIO::Job *, const QByteArray &) ),
	   mFolder, SLOT( slotSimpleData(KIO::Job *, const QByteArray &) ) );
  mAccount->displayProgress();
}

//-----------------------------------------------------------------------------
void KMCachedImapJob::slotPutMessageDataReq(KIO::Job *job, QByteArray &data)
{
  QMap<KIO::Job *, KMAcctCachedImap::jobData>::Iterator it = mAccount->mapJobData.find(job);
  if (it == mAccount->mapJobData.end()) {
    delete this;
    return;
  }
  if ((*it).data.size() - (*it).offset > 0x8000) {
    data.duplicate((*it).data.data() + (*it).offset, 0x8000);
    (*it).offset += 0x8000;
  } else if ((*it).data.size() - (*it).offset > 0) {
    data.duplicate((*it).data.data() + (*it).offset, (*it).data.size() - (*it).offset);
    (*it).offset = (*it).data.size();
  } else
    data.resize(0);
}


//-----------------------------------------------------------------------------
void KMCachedImapJob::slotPutMessageResult(KIO::Job *job)
{
  QMap<KIO::Job *, KMAcctCachedImap::jobData>::Iterator it = mAccount->mapJobData.find(job);
  if ( it == mAccount->mapJobData.end() ) {
    delete this;
    return;
  }

  if ( job->error() ) {
    QStringList errors = job->detailedErrorStrings();
    QString myError = "<qt><p><b>" + i18n("Error while uploading message")
      + "</b></p><p>" + i18n("Could not upload the message %1 on the server from folder %2 with URL %3.").arg((*it).items[0]).arg(mFolder->name()).arg((*it).url)
      + "</p><p>" + i18n("This could be because you don't have permission to do this. The error message from the server communication is here:") + "</p>";
    KMessageBox::error( 0, myError + errors[1] + '\n' + errors[2], errors[0] );
    if (mAccount->slave())
      mAccount->mapJobData.remove(it);
    delete this;
    return;
  } else {
    // kdDebug() << "resulting data \"" << QCString((*it).data) << "\"" << endl;
    emit messageStored(mMsg);
    int i;
    if( ( i = mFolder->find(mMsg) ) != -1 ) {
      mFolder->quiet( TRUE );
      mFolder->removeMsg(i);
      mFolder->quiet( FALSE );
    }
    mMsg = NULL;
  }
  if (mAccount->slave()) mAccount->mapJobData.remove(it);
  mAccount->displayProgress();
  /*
  if (mAccount->slave()) mAccount->mJobList.remove(this);
  delete this;
  */
  slotPutNextMessage();
}


void KMCachedImapJob::slotAddNextSubfolder(KIO::Job * job) {
  if (job) {
    QMap<KIO::Job *, KMAcctCachedImap::jobData>::Iterator it = mAccount->mapJobData.find(job);

    if ( job->error() && !(*it).parent->silentUpload() ) {
      QStringList errors = job->detailedErrorStrings();
      QString myError = "<qt><p><b>" + i18n("Error while uploading folder")
	+ "</b></p><p>" + i18n("Could not make the folder %1 on the server.").arg((*it).items[0])
	+ "</p><p>" + i18n("This could be because you don't have permission to do this or because the directory is already present on the server. The error message from the server communication is here:") + "</p>";
      // kdDebug() << "Error messages:\n 0: " << errors[0].latin1() << "\n 1: " << errors[1].latin1() << "\n 2: " << errors[2].latin1() << endl;
      KMessageBox::error( 0, myError + errors[1] + '\n' + errors[2], errors[0] );
    }
    (*it).parent->setSilentUpload( false );
    mAccount->mapJobData.remove(it);

    if( job->error() ) {
      delete this;
      return;
    }
  }

  if (mFolderList.isEmpty()) {
    // No more folders to add
    delete this;
    return;
  }

  KMFolderCachedImap *folder = mFolderList.front();
  mFolderList.pop_front();
  KURL url = mAccount->getUrl();
  url.setPath(mFolder->imapPath() + folder->name());

  KMAcctCachedImap::jobData jd( url.url(), folder );
  KIO::SimpleJob *simpleJob = KIO::mkdir(url);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), simpleJob);
  mJob = simpleJob;
  mAccount->mapJobData.insert(mJob, jd);
  connect( mJob, SIGNAL(result(KIO::Job *)), this, SLOT(slotAddNextSubfolder(KIO::Job *)) );
  mAccount->displayProgress();
}


void KMCachedImapJob::slotDeleteNextFolder( KIO::Job *job )
{
  if( job && job->error() ) {
    job->showErrorDialog( 0L  );
    mAccount->mJobList.remove(this);
    delete this;
    return;
  }

  if( mFolderPathList.isEmpty() ) {
    mAccount->mJobList.remove(this);
    delete this;
    return;
  }

  QString folderPath = mFolderPathList.front(); mFolderPathList.pop_front();
  KURL url = mAccount->getUrl();
  url.setPath(folderPath);
  KMAcctCachedImap::jobData jd;
  KIO::SimpleJob *simpleJob = KIO::file_delete(url, FALSE);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), simpleJob);
  mJob = simpleJob;
  mAccount->mapJobData.insert(mJob, jd);
  connect( mJob, SIGNAL( result(KIO::Job *) ), SLOT( slotDeleteNextFolder(KIO::Job *) ) );
  mAccount->displayProgress();
}

void KMCachedImapJob::checkUidValidity() {
  KURL url = mAccount->getUrl();
  url.setPath( mFolder->imapPath() + ";UID=0:0" );

  KMAcctCachedImap::jobData jd( url.url(), mFolder );

  KIO::SimpleJob *job = KIO::get( url, FALSE, FALSE );
  KIO::Scheduler::assignJobToSlave( mAccount->slave(), job );
  mAccount->mapJobData.insert( job, jd );
  connect( job, SIGNAL(result(KIO::Job *)), SLOT(slotCheckUidValidityResult(KIO::Job *)) );
  connect( job, SIGNAL(data(KIO::Job *, const QByteArray &)),
           mFolder, SLOT(slotSimpleData(KIO::Job *, const QByteArray &)));
  mAccount->displayProgress();
}

void KMCachedImapJob::slotCheckUidValidityResult(KIO::Job * job) {
  QMap<KIO::Job *, KMAcctCachedImap::jobData>::Iterator it = mAccount->mapJobData.find(job);
  if( it == mAccount->mapJobData.end() ) {
    delete this;
    return;
  }

  if( job->error() ) {
    job->showErrorDialog( 0 );
    mAccount->mJobList.remove( this );
    delete this;
    return;
  }

  // Check the uidValidity
  QCString cstr((*it).data.data(), (*it).data.size() + 1);
  int a = cstr.find("X-uidValidity: ");
  if (a < 0) {
    // Something is seriously rotten here! TODO: Tell the user that he has a problem
    kdDebug() << "No uidvalidity available for folder " << mFolder->name() << endl;
    return;
  }
  int b = cstr.find("\r\n", a);
  if ( (b - a - 15) >= 0 ) {
    QString uidv = cstr.mid(a + 15, b - a - 15);
    // kdDebug() << "New uidv = " << uidv << ", old uidv = " << mFolder->uidValidity()
    // << endl;
    if( mFolder->uidValidity() != "" && mFolder->uidValidity() != uidv ) {
      // kdDebug() << "Expunging the mailbox " << mFolder->name() << "!" << endl;
      mFolder->expunge();
      mFolder->setLastUid( 0 );
    }
  } else
    kdDebug() << "No uidvalidity available for folder " << mFolder->name() << endl;

#if 0
  // Set access control on the folder
  a = cstr.find("X-Access: ");
  if (a >= 0) {
    b = cstr.find("\r\n", a);
    QString access;
    if ( (b - a - 10) >= 0 ) access = cstr.mid(a + 10, b - a - 10);
    mReadOnly = access == "Read only";
  }
#endif

  mAccount->mapJobData.remove(it);
  delete this;
}


void KMCachedImapJob::renameFolder( const QString &newName ) {
  // Set the source URL
  KURL urlSrc = mAccount->getUrl();
  urlSrc.setPath( mFolder->imapPath() );

  // Set the destination URL - this is a bit trickier
  KURL urlDst = mAccount->getUrl();
  QString imapPath( mFolder->imapPath() );
  // Destination url = old imappath - oldname + new name
  imapPath.truncate( imapPath.length() - mFolder->name().length() - 1);
  imapPath += newName + '/';
  urlDst.setPath( imapPath );

  KMAcctCachedImap::jobData jd( newName, mFolder );
  jd.path = imapPath;

  KIO::SimpleJob *simpleJob = KIO::rename( urlSrc, urlDst, FALSE );
  KIO::Scheduler::assignJobToSlave( mAccount->slave(), simpleJob );
  mJob = simpleJob;
  mAccount->mapJobData.insert( mJob, jd );
  connect( mJob, SIGNAL(result(KIO::Job *)), SLOT(slotRenameFolderResult(KIO::Job *)) );
  mAccount->displayProgress();
}

static void renameChildFolders( KMFolderDir* dir, const QString& oldPath, const QString& newPath )
{
  if( dir ) {
    KMFolderNode *node = dir->first();
    while( node ) {
      if( !node->isDir() ) {	  
	KMFolderCachedImap* imapFolder = static_cast<KMFolderCachedImap*>(node);
	if ( imapFolder->imapPath() != "" )
	  // Only rename folders that have been accepted by the server
	  if( imapFolder->imapPath().find( oldPath ) == 0 ) {
	    QString p = imapFolder->imapPath();
	    p = p.mid( oldPath.length() );
	    p.prepend( newPath );
	    imapFolder->setImapPath( p );
	    renameChildFolders( imapFolder->child(), oldPath, newPath );
	  }
      }
      node = dir->next();
    }
  }
}

void KMCachedImapJob::slotRenameFolderResult( KIO::Job *job ) {
  QMap<KIO::Job *, KMAcctCachedImap::jobData>::Iterator it =
    mAccount->mapJobData.find(job);
  if( it == mAccount->mapJobData.end() ) {
    // This shouldn't happen??
    delete this;
    return;    
  }

  if( job->error() ) {
    job->showErrorDialog( 0 );
  } else {
    // Okay, the folder seems to be renamed on the folder. Now rename it on disk
    QString oldName = mFolder->name();
    QString oldPath = mFolder->imapPath();
    mFolder->setImapPath( (*it).path );
    mFolder->KMFolder::rename( (*it).url );

    if( oldPath.endsWith( "/" ) ) oldPath = oldPath.left( oldPath.length() -1 );
    QString newPath = mFolder->imapPath();
    if( newPath.endsWith( "/" ) ) newPath = newPath.left( newPath.length() -1 );
    renameChildFolders( mFolder->child(), oldPath, newPath );
    kernel->imapFolderMgr()->contentsChanged();
  }

  mAccount->mJobList.remove( this );
  delete this;
  return;
}

#include "kmfoldercachedimap.moc"
