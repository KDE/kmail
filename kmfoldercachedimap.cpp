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

#include "kmkernel.h"
#include "kmfoldercachedimap.h"
#include "undostack.h"
#include "kmfoldermgr.h"
#include "kmmessage.h"
#include "kmacctcachedimap.h"
#include "kmacctmgr.h"
#include "imapprogressdialog.h"
#include "kmgroupware.h"
#include "kmailicalifaceimpl.h"

using KMail::CachedImapJob;
using KMail::ImapAccountBase;

#include <kapplication.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kio/global.h>
#include <kio/scheduler.h>
#include <qbuffer.h>
#include <qfile.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qvaluelist.h>

#define UIDCACHE_VERSION 1


DImapTroubleShootDialog::DImapTroubleShootDialog( QWidget* parent,
						  const char* name )
  : KDialogBase( Plain, i18n( "Troubleshooting the IMAP cache" ),
		 Cancel | User1 | User2, Cancel, parent, name, true ),
    rc( Cancel )
{
  QFrame* page = plainPage();
  QVBoxLayout *topLayout = new QVBoxLayout( page, 0 );
  QString txt = i18n( "<p><b>Troubleshooting the IMAP cache.</b></p>"
		      "<p>If you have problems with synchronizing an IMAP "
		      "folder, you should first try rebuilding the index "
		      "file. This will take some time to rebuild, but will "
		      "not cause any problems.</p><p>If that is not enough, "
		      "you can try refreshing the IMAP cache. If you do this, "
		      "you will loose all your local changes for this folder "
		      "and all it's subfolders.</p>" );
  topLayout->addWidget( new QLabel( txt, page ) );
  enableButtonSeparator( true );

  setButtonText( User1, i18n( "Refresh &Cache" ) );
  setButtonText( User2, i18n( "Rebuild &Index" ) );

  connect( this, SIGNAL( user1Clicked () ), this, SLOT( slotRebuildCache() ) );
  connect( this, SIGNAL( user2Clicked () ), this, SLOT( slotRebuildIndex() ) );
}

int DImapTroubleShootDialog::run()
{
  DImapTroubleShootDialog d;
  d.exec();
  return d.rc;
}

void DImapTroubleShootDialog::slotRebuildCache()
{
  rc = User1;
  done( User1 );
}

void DImapTroubleShootDialog::slotRebuildIndex()
{
  rc = User2;
  done( User2 );
}


KMFolderCachedImap::KMFolderCachedImap( KMFolderDir* aParent,
					const QString& aName )
  : KMFolderMaildir( aParent, aName ),
    mSyncState( SYNC_STATE_INITIAL ), mContentState( imapNoInformation ),
    mSubfolderState( imapNoInformation ), mIsSelected( false ),
    mCheckFlags( true ), mAccount( NULL ), uidMapDirty( true ),
    mLastUid( 0 ), uidWriteTimer( -1 ),
    mIsConnected( false ), mFolderRemoved( false ), mResync( false ),
    mSuppressDialog( false ), mHoldSyncs( false ), mRemoveRightAway( false )
{
  KConfig* config = KMKernel::config();
  KConfigGroupSaver saver(config, "Folder-" + idString());
  if (mImapPath.isEmpty()) mImapPath = config->readEntry("ImapPath");
  if (aName == "INBOX" && mImapPath == "/INBOX/")
  {
    //mLabel = i18n("inbox");
  }
  mIsSystemFolder = false;
  mNoContent = config->readBoolEntry("NoContent", FALSE);
  mReadOnly = config->readBoolEntry("ReadOnly", FALSE);

  connect( this, SIGNAL( listMessagesComplete() ),
	   this, SLOT( serverSyncInternal() ) );

  setUidValidity("");
  mLastUid=0;
  readUidCache();

  mProgress = 0;
}

KMFolderCachedImap::~KMFolderCachedImap()
{
  if( !mFolderRemoved ) {
    // Only write configuration when the folder haven't been deleted
    KConfig* config = KMKernel::config();
    KConfigGroupSaver saver(config, "Folder-" + idString());
    config->writeEntry("ImapPath", mImapPath);
    config->writeEntry("NoContent", mNoContent);
    config->writeEntry("ReadOnly", mReadOnly);

    writeUidCache();
  }

  if (kmkernel->undoStack()) kmkernel->undoStack()->folderDestroyed(this);
}

int KMFolderCachedImap::remove()
{
  mFolderRemoved = true;
  int rc = KMFolderMaildir::remove();

  if( mRemoveRightAway ) {
    // This is the account folder of an account that was just removed
    // When this happens, be sure to delete all traces of the cache
    QString part1 = path() + "/." + dotEscape(name());
    QString uidCacheFile = part1 + ".uidcache";
    if( QFile::exists(uidCacheFile) )
      unlink( QFile::encodeName( uidCacheFile ) );
    KIO::del( part1 + ".directory" );
  } else {
    // Don't remove the uidcache file here, since presence of that is how
    // we figure out if a directory present on the server have been deleted
    // from the cache or if it's new on the server. The file is removed
    // during the sync
  }

  return rc;
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
	    mLastUid =
	      QString::fromLocal8Bit( buf).stripWhiteSpace().toULong();
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
  if( lastUid() == 0 || uidValidity().isEmpty() )
    // No info from the server yet
    return 0;

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

void KMFolderCachedImap::reloadUidMap()
{
  uidMap.clear();
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
      if( uid > mLastUid ) setLastUid( uid );
    }
  }
  close();
  uidMapDirty = false;
}

/* Reimplemented from KMFolderMaildir */
KMMessage* KMFolderCachedImap::take(int idx)
{
  uidMapDirty = true;
  return KMFolderMaildir::take(idx);
}

// Add a message without clearing it's X-UID field.
int KMFolderCachedImap::addMsgInternal( KMMessage* msg, bool newMail,
                                        int* index_return )
{
  // Possible optimization: Only dirty if not filtered below
  bool ok;
  ulong uid = msg->headerField("X-UID").toULong( &ok );
  if( ok ) {
    uidMapDirty = true;
    if( uid > mLastUid )
      setLastUid( uid );
  }

  // Add the message
  int rc = KMFolderMaildir::addMsg(msg, index_return);

  if( newMail && imapPath() == "/INBOX/" )
    // This is a new message. Filter it
    mAccount->processNewMsg( msg );

  return rc;
}

/* Reimplemented from KMFolderMaildir */
int KMFolderCachedImap::addMsg(KMMessage* msg, int* index_return)
{
  // Strip the IMAP UID
  msg->removeHeaderField( "X-UID" );

  // Add it to storage
  return addMsgInternal( msg, false, index_return );
}


/* Reimplemented from KMFolderMaildir */
void KMFolderCachedImap::removeMsg(int idx, bool imapQuiet)
{
  uidMapDirty = true;

  // Remove it from disk
  KMFolderMaildir::removeMsg(idx,imapQuiet);

  kmkernel->dimapFolderMgr()->contentsChanged();
}

bool KMFolderCachedImap::canRemoveFolder() const {
  // If this has subfolders it can't be removed
  if( child() != 0 && child()->count() > 0 )
    return false;

#if 0
  // No special condition here, so let base class decide
  return KMFolderMaildir::canRemoveFolder();
#endif
  return true;
}

/* Reimplemented from KMFolderDir */
int KMFolderCachedImap::rename( const QString& aName,
				KMFolderDir* /*aParent*/ )
{
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

void KMFolderCachedImap::setLastUid( ulong uid )
{
  mLastUid = uid;
  if( uidWriteTimer == -1 )
    // Write in one minute
    uidWriteTimer = startTimer( 60000 );
}

void KMFolderCachedImap::timerEvent( QTimerEvent* )
{
  killTimer( uidWriteTimer );
  uidWriteTimer = -1;
  writeUidCache();
}

ulong KMFolderCachedImap::lastUid()
{
  return mLastUid;
}

KMMessage* KMFolderCachedImap::findByUID( ulong uid )
{
  bool mapReloaded = false;
  if( uidMapDirty ) {
    reloadUidMap();
    mapReloaded = true;
  }

  QMap<ulong,int>::Iterator it = uidMap.find( uid );
  if( it != uidMap.end() ) {
    bool unget = !isMessage(count() - 1);
    KMMessage* msg = getMsg( *it );
    if( msg && msg->headerField("X-UID").toULong() == uid )
      return msg;
    else if( unget )
      unGetMsg( *it );
  }

  // Not found by now
  if( mapReloaded )
    // Not here then
    return 0;

  // There could be a problem in the maps. Rebuild them and try again
  reloadUidMap();
  it = uidMap.find( uid );
  if( it != uidMap.end() )
    // Since the uid map is just rebuilt, no need for the sanity check
    return getMsg( *it );

  // Then it's not here
  return 0;
}

// This finds and sets the proper account for this folder if it has
// not been done
KMAcctCachedImap *KMFolderCachedImap::account()
{
  if( (KMAcctCachedImap *)mAccount == 0 ) {
    // Find the account
    mAccount = static_cast<KMAcctCachedImap *>( kmkernel->acctMgr()->find( name() ) );
  }

  return mAccount;
}

void KMFolderCachedImap::slotTroubleshoot()
{
  const int rc = DImapTroubleShootDialog::run();

  if( rc == KDialogBase::User1 ) {
    // Refresh cache
    if( !account() ) {
      KMessageBox::sorry( 0, i18n("No account setup for this folder.\n"
				  "Please try running a sync before this.") );
      return;
    }
    QString str = i18n("Are you sure you want to refresh the IMAP cache of "
		       "the folder %1 and all it's subfolders?\nThis will "
		       "remove all changes you have done locally to your "
		       "folders").arg( name() );
    QString s1 = i18n("Refresh IMAP Cache");
    QString s2 = i18n("&Refresh");
    if( KMessageBox::warningContinueCancel( 0, str, s1, s2 ) ==
	KMessageBox::Continue )
      account()->invalidateIMAPFolders( this );
  } else if( rc == KDialogBase::User2 ) {
    // Rebuild index file
    createIndexFromContents();
    KMessageBox::information( 0, i18n( "The index of this folder has been "
				       "recreated." ) );
  }
}

void KMFolderCachedImap::processNewMail()
{
  if( account() )
    account()->processNewMail( this, true );
}

void KMFolderCachedImap::serverSync( bool suppressDialog )
{
  if( mSyncState != SYNC_STATE_INITIAL ) {
    if( KMessageBox::warningYesNo( 0, i18n("Folder %1 is not in initial sync state (state was %2). Do you want to reset\nit to initial sync state and sync anyway?" ).arg( imapPath() ).arg( mSyncState ) ) == KMessageBox::Yes ) {
      mSyncState = SYNC_STATE_INITIAL;
    } else return;
  }

  assert( account() );

  // Connect to the imap progress dialog
  mSuppressDialog = suppressDialog;
  if( mIsConnected != mAccount->isProgressDialogEnabled() &&
      suppressDialog )
  {
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

  if( mHoldSyncs ) {
    // All done for this folder.
    mProgress = 100; // all done
    emit newState( name(), mProgress, i18n("Synchronization skipped"));
    mAccount->displayProgress();
    mSyncState = SYNC_STATE_INITIAL;
    emit statusMsg( i18n("%1: Synchronization done").arg(name()) );
    emit folderComplete( this, true );
    return;
  }

  mResync = false;
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
    emit newState( name(), mProgress, i18n("Synchronizing"));

    open();

    kdDebug(5006) << k_funcinfo << " making connection" << endl;
    // Connect to the server (i.e. prepare the slave)
    ImapAccountBase::ConnectionState cs = mAccount->makeConnection();
    if ( cs == ImapAccountBase::Error ) {
      // Cancelled by user, or slave can't start
      kdDebug(5006) << "makeConnection said Error, aborting." << endl;
      // We stop here. We're already in SYNC_STATE_INITIAL for the next time.
      emit folderComplete(this, FALSE);
      break;
    } else if ( cs == ImapAccountBase::Connecting )
    {
      kdDebug(5006) << "makeConnection said Connecting, waiting for signal."
		    << endl;
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
    emit syncRunning( this, true );
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

    if( mResync ) {
      // Some conflict have been resolved, so restart the sync
      mResync = false;
      mSyncState = SYNC_STATE_INITIAL;
      serverSyncInternal();
      break;
    } else
      // Continue with the subfolders
      mSyncState = SYNC_STATE_FIND_SUBFOLDERS;

  case SYNC_STATE_FIND_SUBFOLDERS:
    {
      emit newState( name(), mProgress, i18n("Updating cache file"));

      mSyncState = SYNC_STATE_SYNC_SUBFOLDERS;
      mSubfoldersForSync.clear();
      mCurrentSubfolder = 0;
      if( child() ) {
	KMFolderNode *node = child()->first();
	while( node ) {
	  if( !node->isDir() ) {
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
    emit syncRunning( this, false );
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
	mCurrentSubfolder->serverSync( mSuppressDialog );
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
  kdDebug(5006) << k_funcinfo << errorCode << endl;
  disconnect( mAccount, SIGNAL( connectionResult(int) ),
	      this, SLOT( slotConnectionResult(int) ) );
  if ( !errorCode ) {
    // Success
    mSyncState = SYNC_STATE_CHECK_UIDVALIDITY;
    serverSyncInternal();
  } else {
    // Error (error message already shown by the account)
    emit folderComplete(this, FALSE);
  }
}

/* find new messages (messages without a UID) */
QValueList<unsigned long> KMFolderCachedImap::findNewMessages()
{
  QValueList<unsigned long> result;
  for( int i = 0; i < count(); ++i ) {
    bool unget = !isMessage(i);
    KMMessage *msg = getMsg(i);
    if( !msg ) continue; /* what goes on if getMsg() returns 0? */
    if( msg->headerField("X-UID").isEmpty() ) {
      result.append( msg->getMsgSerNum() );
    } else {
      if (unget) unGetMsg(i);
    }
  }
  return result;
}

/* Upload new messages to server */
void KMFolderCachedImap::uploadNewMessages()
{
  QValueList<unsigned long> newMsgs = findNewMessages();
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
	  kdDebug(5006) << "KMFolderCachedImap::findNewFolders(): ARGH!!! "
			<< node->name() << " is not an IMAP folder. It is a "
			<< node->className() << endl;
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
  QPtrList<KMMessage> msgsForDeletion;

  // It is not possible to just go over all indices and remove
  // them one by one because the index list can get resized under
  // us. So use msg pointers instead
  for( int i = 0; i < count(); ++i ) {
    bool unget = !isMessage(i);
    KMMessage *msg = getMsg(i);
    if( !msg ) continue;
    bool ok;
    ulong uid = msg->headerField( "X-UID" ).toULong( &ok );
    if( ok && !uidsOnServer.contains( uid ) )
      msgsForDeletion.append( msg );
    else
      if (unget) unGetMsg(i);
  }

  if( !msgsForDeletion.isEmpty() ) {
    emit statusMsg( i18n("%1: Deleting removed messages from cache").arg(name()) );
    removeMsg( msgsForDeletion );
  }

  mProgress += 10;
  //kdDebug() << name() << ": +10 (deleteMessages) -> " << mProgress << "%" << endl;
  emit newState( name(), mProgress, i18n("Deleting removed messages from server"));

  /* Delete messages from the server that we dont have anymore */
  if( !uidsForDeletionOnServer.isEmpty() ) {
    emit statusMsg( i18n("%1: Deleting removed messages from server").arg(name()) );
    QStringList sets = makeSets( uidsForDeletionOnServer, true );
    uidsForDeletionOnServer.clear();
    if( sets.count() > 1 ) {
      // Rerun the sync until the messages are all deleted
      mResync = true;
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
  // IMAP root folders don't seem to have a UID validity setting.
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

  if( mAccount->makeConnection() != ImapAccountBase::Connected ) {
    emit listMessagesComplete();
    emit folderComplete( this, false );
    return;
  }
  uidsOnServer.clear();
  uidsForDeletionOnServer.clear();
  mMsgsForDownload.clear();
  mUidsForDownload.clear();
  KURL url = mAccount->getUrl();
  url.setPath(imapPath() + ";UID=1:*;SECTION=ENVELOPE");
  KMAcctCachedImap::jobData jd( url.url(), this );

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
         flagsToStatus( existingMessage, flags );
      }
      delete msg;
    } else {
      ulong size = msg->headerField("X-Length").toULong();
      mMsgsForDownload << KMail::CachedImapJob::MsgForDownload(uid, flags, size);
      if( imapPath() == "/INBOX/" )
         mUidsForDownload << uid;
    }
    (*it).cdata.remove(0, pos);
    (*it).done++;
    pos = (*it).cdata.find("\r\n--IMAPDIGEST", 1);
    mAccount->displayProgress();
  }
}

void KMFolderCachedImap::getMessagesResult( KIO::Job * job, bool lastSet )
{

  KMAcctCachedImap::JobIterator it = mAccount->findJob(job);
  if ( it == mAccount->jobsEnd() ) { // Shouldn't happen
    kdDebug(5006) << "could not find job!?!?!" << endl;
    serverSyncInternal(); /* HACK^W Fix: we should at least try to keep going */
    return;
  }

  if( job->error() ) {
    mAccount->slotSlaveError( mAccount->slave(), job->error(),
			      job->errorText() );
    mContentState = imapNoInformation;
    emit folderComplete(this, FALSE);
  } else if (lastSet) mContentState = imapFinished;
  mAccount->removeJob(it);
  if( lastSet )
    emit listMessagesComplete();
}

void KMFolderCachedImap::slotProgress(unsigned long done, unsigned long total)
{
  //kdDebug() << "KMFolderCachedImap::slotProgress done=" << done << " total=" << total << "=> progress=" << mProgress + ( 20 * done ) / total << endl;
  // Progress info while retrieving new emails
  // (going from mProgress to mProgress+20)
  emit newState( name(), mProgress + (20 * done) / total, QString::null);
}

//-----------------------------------------------------------------------------
void KMFolderCachedImap::flagsToStatus(KMMsgBase *msg, int flags, bool newMsg)
{
  if (flags & 4) 
    msg->setStatus( KMMsgStatusFlag );
  if (flags & 2)
    msg->setStatus( KMMsgStatusReplied );
  if (flags & 1)
    msg->setStatus( KMMsgStatusOld );

  if (msg->isOfUnknownStatus()) {
    if (newMsg)
      msg->setStatus( KMMsgStatusNew );
    else
      msg->setStatus( KMMsgStatusUnread );
  }
}


void KMFolderCachedImap::setAccount(KMAcctCachedImap *aAccount)
{
  assert( aAccount->isA("KMAcctCachedImap") );
  mAccount = aAccount;
  if( imapPath()=="/" ) aAccount->setFolder(this);

  if( !mChild || mChild->count() == 0) return;
  for( KMFolderNode* node = mChild->first(); node; node = mChild->next() )
    if (!node->isDir())
      static_cast<KMFolderCachedImap*>(node)->setAccount(aAccount);
}


// This synchronizes the subfolders with the server
bool KMFolderCachedImap::listDirectory()
{
  mSubfolderState = imapInProgress;
  KURL url = mAccount->getUrl();
  url.setPath(imapPath() + ";TYPE="
	      + (mAccount->onlySubscribedFolders() ? "LSUB" : "LIST"));
  KMAcctCachedImap::jobData jd( url.url(), this );
  mSubfolderNames.clear();
  mSubfolderPaths.clear();
  mSubfolderMimeTypes.clear();

  if( mAccount->makeConnection() != ImapAccountBase::Connected ) {
    emit folderComplete( this, false );
    return false;
  }

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
  if( it == mAccount->jobsEnd() ) { // Shouldn't happen
    kdDebug(5006) << "could not find job!?!?!" << endl;
    serverSyncInternal(); /* HACK^W Fix: we should at least try to keep going */
    return;
  }

  if( job->error() ) {
    kdDebug(5006) << "listDirectory() - slotListResult: Job error\n";
    mAccount->slotSlaveError( mAccount->slave(), job->error(), job->errorText() );
  }

  mSubfolderState = imapFinished;
  mAccount->removeJob(it);

  if (!job->error()) {
    kmkernel->dimapFolderMgr()->quiet(TRUE);
    createChildFolder();

    // Find all subfolders present on disk but not on the server
    KMFolderCachedImap *folder;
    KMFolderNode *node = mChild->first();
    QPtrList<KMFolder> toRemove;
    while (node) {
      if (!node->isDir() ) {
	if( mSubfolderNames.findIndex(node->name()) == -1) {
	  // This subfolder isn't present on the server
	  kdDebug(5006) << node->name() << " isn't on the server." << endl;
	  folder = static_cast<KMFolderCachedImap*>(node);
	  if( !folder->uidValidity().isEmpty() ) {
	    // The folder have a uidValidity setting, so it has been on the
	    // server before. Delete it locally.
	    toRemove.append( folder );
	  }
	}
      }
      node = mChild->next();
    }
    // Remove all folders
    for ( KMFolder* doomed=toRemove.first(); doomed; doomed = toRemove.next() )
      kmkernel->dimapFolderMgr()->remove( doomed );

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
      QString part1 = path() + "/." + dotEscape(name()) + ".directory/."
	+ dotEscape(mSubfolderNames[i]);
      QString uidCacheFile = part1 + ".uidcache";
      if( QFile::exists(uidCacheFile) ) {
	// This is an old folder that is deleted locally - delete it on the server
	unlink( QFile::encodeName( uidCacheFile ) );
	foldersForDeletionOnServer << mSubfolderPaths[i];
	// Make sure all trace of the dir is gone
	KIO::del( part1 + ".directory" );
      } else {
	// This is a new folder, create the local cache
	folder = static_cast<KMFolderCachedImap*>
	  (mChild->createFolder(mSubfolderNames[i], false, KMFolderTypeCachedImap));
	if (folder) {
	  folder->close();
	  folder->setAccount(mAccount);
	  kmkernel->dimapFolderMgr()->contentsChanged();
	} else {
	  kdDebug(5006) << "can't create folder " << mSubfolderNames[i] <<endl;
	}
      }
    } else {
      // kdDebug(5006) << "node " << node->name() << " is a " << node->className() << endl;
      if( node->isA("KMFolderCachedImap") )
	folder = static_cast<KMFolderCachedImap*>(node);
    }

    if( folder && folder->imapPath().isEmpty() ) {
      // kdDebug(5006) << "folder("<<folder->name()<<")->imapPath()=" << folder->imapPath()
      // << "\nAssigning new imapPath " << mSubfolderPaths[i] << endl;
      // Write folder settings
      folder->setAccount(mAccount);
      folder->setNoContent(mSubfolderMimeTypes[i] == "inode/directory");
      folder->setImapPath(mSubfolderPaths[i]);
    }
  }

  kmkernel->dimapFolderMgr()->quiet(FALSE);
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
KMFolderCachedImap::doCreateJob( KMMessage *msg, FolderJob::JobType jt, KMFolder *folder,
                                 QString, const AttachmentStrategy* ) const
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
