/**
 *  kmfoldercachedimap.cpp
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

#include <errno.h>

#include "kmkernel.h"
#include "kmfoldercachedimap.h"
#include "undostack.h"
#include "kmfoldermgr.h"
#include "kmmessage.h"
#include "kmacctcachedimap.h"
#include "kmacctmgr.h"
#include "imapprogressdialog.h"
#include "kmailicalifaceimpl.h"
#include "kmfolder.h"

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


KMFolderCachedImap::KMFolderCachedImap( KMFolder* folder, const char* aName )
  : KMFolderMaildir( folder, aName ),
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
  if (QString(aName).upper() == "INBOX" && mImapPath == "/INBOX/")
  {
    mLabel = i18n("inbox");
    // for the icon
    folder->setSystemFolder(true);
  }
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

  if (kmkernel->undoStack()) kmkernel->undoStack()->folderDestroyed( folder() );
}

void KMFolderCachedImap::remove()
{
  mFolderRemoved = true;

  if( mRemoveRightAway ) {
    // This is the account folder of an account that was just removed
    // When this happens, be sure to delete all traces of the cache
    QString part1 = folder()->path() + "/." + dotEscape(name());
    QString uidCacheFile = part1 + ".uidcache";
    if( QFile::exists(uidCacheFile) )
      unlink( QFile::encodeName( uidCacheFile ) );
    KIO::del( KURL::fromPathOrURL( part1 + ".directory" ) );
  } else {
    // Don't remove the uidcache file here, since presence of that is how
    // we figure out if a directory present on the server have been deleted
    // from the cache or if it's new on the server. The file is removed
    // during the sync
  }
  FolderStorage::remove();
}

QString KMFolderCachedImap::uidCacheLocation() const
{
  QString sLocation(folder()->path());
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
  if( uidValidity().isEmpty() || uidValidity() == "INVALID" ) {
    // No info from the server yet, remove the file.
    if( QFile::exists( uidCacheLocation() ) )
      unlink( QFile::encodeName( uidCacheLocation() ) );
    return 0;
  }

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
    KMMsgBase *msg = getMsgBase( i );
    if( !msg ) continue;
    ulong uid = msg->UID();
    uidMap.insert( uid, i );
    if( uid > mLastUid ) setLastUid( uid );
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
  ulong uid = msg->UID();
  if( uid != 0 ) {
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
  msg->setUID( 0 );

  // Add it to storage
  return addMsgInternal( msg, false, index_return );
}


/* Reimplemented from KMFolderMaildir */
void KMFolderCachedImap::removeMsg(int idx, bool imapQuiet)
{
  uidMapDirty = true;
  // Remove it from disk
  KMFolderMaildir::removeMsg(idx,imapQuiet);
}

bool KMFolderCachedImap::canRemoveFolder() const {
  // If this has subfolders it can't be removed
  if( folder() && folder()->child() && folder()->child()->count() > 0 )
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
    KMessageBox::error( 0, i18n("You cannot rename a folder whilst a sync is in progress") );
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

KMFolder* KMFolderCachedImap::trashFolder() const
{
  QString trashStr = account()->trash();
  return kmkernel->dimapFolderMgr()->findIdString( trashStr );
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

KMMsgBase* KMFolderCachedImap::findByUID( ulong uid )
{
  bool mapReloaded = false;
  if( uidMapDirty ) {
    reloadUidMap();
    mapReloaded = true;
  }

  QMap<ulong,int>::Iterator it = uidMap.find( uid );
  if( it != uidMap.end() ) {
    KMMsgBase *msg = getMsgBase( *it );
    if( msg && msg->UID() == uid )
      return msg;
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
KMAcctCachedImap *KMFolderCachedImap::account() const
{
  if( (KMAcctCachedImap *)mAccount == 0 ) {
    // Find the account
    mAccount = static_cast<KMAcctCachedImap *>( kmkernel->acctMgr()->findByName( name() ) );
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
  case SYNC_STATE_UPLOAD_FLAGS:      return "SYNC_STATE_UPLOAD_FLAGS";
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

    // kdDebug(5006) << k_funcinfo << " making connection" << endl;
    // Connect to the server (i.e. prepare the slave)
    ImapAccountBase::ConnectionState cs = mAccount->makeConnection();
    if ( cs == ImapAccountBase::Error ) {
      // Cancelled by user, or slave can't start
      kdDebug(5006) << "makeConnection said Error, aborting." << endl;
      // We stop here. We're already in SYNC_STATE_INITIAL for the next time.
      emit folderComplete(this, FALSE);
      break;
    } else if ( cs == ImapAccountBase::Connecting ) {
      // kdDebug(5006) << "makeConnection said Connecting, waiting for signal." << endl;
      // We'll wait for the connectionResult signal from the account.
      connect( mAccount, SIGNAL( connectionResult(int) ),
               this, SLOT( slotConnectionResult(int) ) );
      break;
    } else {
      // Connected
      // kdDebug(5006) << "makeConnection said Connected, proceeding." << endl;
      mSyncState = SYNC_STATE_CHECK_UIDVALIDITY;
      // Fall through to next state
    }
  }
  case SYNC_STATE_CHECK_UIDVALIDITY:
    emit syncRunning( folder(), true );
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
    mSyncState = SYNC_STATE_UPLOAD_FLAGS;
    if( !noContent() ) {
      uploadNewMessages();
      break;
    }
    // Else carry on
  case SYNC_STATE_UPLOAD_FLAGS:
    mSyncState = SYNC_STATE_LIST_SUBFOLDERS;
    if( !noContent() ) {
       // We haven't downloaded messages yet, so we need to build the map.
       if( uidMapDirty )
         reloadUidMap();
       uploadFlags();
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
      KMessageBox::error(0, i18n("Error while retrieving the folderlist"));
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
      emit statusMsg( i18n("%1: Retrieving message list").arg(name()) );
      emit newState( name(), mProgress, i18n("Retrieving message list"));
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
        if( mLastUid == 0 && uidWriteTimer == -1 )
          // This is probably a new and empty folder. Write the UID cache
          writeUidCache();
      }
    }
    // Else carry on

  case SYNC_STATE_HANDLE_INBOX:
    // Wrap up the 'download emails' stage (which has a 20% span)
    mProgress += 20;
    //kdDebug(5006) << name() << ": +20 -> " << mProgress << "%" << endl;

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
      if( folder() && folder()->child() ) {
        KMFolderNode *node = folder()->child()->first();
        while( node ) {
          if( !node->isDir() ) {
            if ( !static_cast<KMFolderCachedImap*>(static_cast<KMFolder*>(node)->storage())->imapPath().isEmpty() )
              // Only sync folders that have been accepted by the server
              mSubfoldersForSync << static_cast<KMFolderCachedImap*>(static_cast<KMFolder*>(node)->storage());
          }
          node = folder()->child()->next();
        }
      }
    }

    // All done for this folder.
    mProgress = 100; // all done
    emit newState( name(), mProgress, i18n("Synchronization done"));
    emit syncRunning( folder(), false );
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
    KMMsgBase *msg = getMsgBase( i );
    if( !msg ) continue; /* what goes on if getMsg() returns 0? */
    if ( msg->UID() == 0 )
      result.append( msg->getMsgSerNum() );
  }
  return result;
}

/* Upload new messages to server */
void KMFolderCachedImap::uploadNewMessages()
{
  QValueList<unsigned long> newMsgs = findNewMessages();
  emit syncState( SYNC_STATE_PUT_MESSAGES, newMsgs.count() );
  mProgress += 10;
  //kdDebug(5006) << name() << ": +10 (uploadNewMessages) -> " << mProgress << "%" << endl;
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

/* Upload message flags to server */
void KMFolderCachedImap::uploadFlags()
{
  mProgress += 10;
  //kdDebug(5006) << name() << ": +10 (uploadFlags) -> " << mProgress << "%" << endl;
  if ( !uidMap.isEmpty() ) {
    emit statusMsg( i18n("%1: Uploading status of messages to server").arg(name()) );
    emit newState( name(), mProgress, i18n("Uploading status of messages to server"));

    // FIXME DUPLICATED FROM KMFOLDERIMAP
    QMap< QString, QStringList > groups;
    open();
    for( int i = 0; i < count(); ++i ) {
      KMMsgBase* msg = getMsgBase( i );
      if( !msg || msg->UID() == 0 )
        // Either not a valid message or not one that is on the server yet
        continue;

      QString flags = KMFolderImap::statusToFlags(msg->status());
      // Collect uids for each typem of flags.
      QString uid;
      uid.setNum( msg->UID() );
      groups[flags].append(uid);
    }
    QMapIterator< QString, QStringList > dit;
    for( dit = groups.begin(); dit != groups.end(); ++dit ) {
      QCString flags = dit.key().latin1();
      QStringList sets = KMFolderImap::makeSets( (*dit), true );
      // Send off a status setting job for each set.
      for( QStringList::Iterator slit = sets.begin(); slit != sets.end(); ++slit ) {
        QString imappath = imapPath() + ";UID=" + ( *slit );
        mAccount->setImapStatus(imappath, flags);
      }
    }
    // FIXME END DUPLICATED FROM KMFOLDERIMAP
  } else {
    emit newState( name(), mProgress, i18n("No messages to upload to server"));
  }

  serverSyncInternal();
}

/* Upload new folders to server */
void KMFolderCachedImap::createNewFolders()
{
  QValueList<KMFolderCachedImap*> newFolders = findNewFolders();
  //emit syncState( SYNC_STATE_CREATE_SUBFOLDERS, newFolders.count() );
  mProgress += 10;
  //kdDebug(5006) << name() << ": +10 (createNewFolders) -> " << mProgress << "%" << endl;
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
  if( folder() && folder()->child() ) {
    KMFolderNode *node = folder()->child()->first();
    while( node ) {
      if( !node->isDir() ) {
        if( static_cast<KMFolder*>(node)->folderType() != KMFolderTypeCachedImap ) {
          kdError(5006) << "KMFolderCachedImap::findNewFolders(): ARGH!!! "
                        << node->name() << " is not an IMAP folder\n";
          node = folder()->child()->next();
          assert(0);
        }
        KMFolderCachedImap* folder = static_cast<KMFolderCachedImap*>(static_cast<KMFolder*>(node)->storage());
        if( folder->imapPath().isEmpty() ) newFolders << folder;
      }
      node = folder()->child()->next();
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
    KMMsgBase *msg = getMsgBase( i );
    if( !msg ) continue;
    ulong uid = msg->UID();
    if( uid!=0 && !uidsOnServer.contains( uid ) )
      msgsForDeletion.append( getMsg( i ) );
  }

  if( !msgsForDeletion.isEmpty() ) {
    emit statusMsg( i18n("%1: Deleting removed messages from cache").arg(name()) );
    removeMsg( msgsForDeletion );
  }

  mProgress += 10;
  //kdDebug(5006) << name() << ": +10 (deleteMessages) -> " << mProgress << "%" << endl;
  emit newState( name(), mProgress, i18n("Deleting removed messages from server"));

  /* Delete messages from the server that we dont have anymore */
  if( !uidsForDeletionOnServer.isEmpty() ) {
    emit statusMsg( i18n("%1: Deleting removed messages from server").arg(name()) );
    QStringList sets = KMFolderImap::makeSets( uidsForDeletionOnServer, true );
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
    //kdDebug(5006) << name() << ": +10 (checkUidValidity) -> " << mProgress << "%" << endl;
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
  KMAcctCachedImap::jobData jd( url.url(), folder() );

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
    ulong uid = msg->UID();
    if( uid != 0 ) 
       uidsOnServer.append( uid );
    if ( /*flags & 8 ||*/ uid <= lastUid()) {
      /* 
       * If this message UID is not present locally, then it must
       * have been deleted by the user, so we delete it on the
       * server also.
       *
       * This relies heavily on lastUid() being correct at all times.
       */
      // kdDebug(5006) << "KMFolderCachedImap::slotGetMessagesData() : folder "<<name()<<" already has msg="<<msg->headerField("Subject") << ", UID="<<uid << ", lastUid = " << mLastUid << endl;
      KMMsgBase *existingMessage = findByUID(uid);
      if( !existingMessage ) {
         // kdDebug(5006) << "message with uid " << uid << " is gone from local cache. Must be deleted on server!!!" << endl;
         uidsForDeletionOnServer << uid;
      } else {
         /* The message is OK, update flags */
         KMFolderImap::flagsToStatus( existingMessage, flags );
         // kdDebug(5006) << "message with uid " << uid << " found in the local cache. " << endl;
      }
      delete msg;
    } else {
      ulong size = msg->headerField("X-Length").toULong();
      mMsgsForDownload << KMail::CachedImapJob::MsgForDownload(uid, flags, size);
      if( imapPath() == "/INBOX/" )
         mUidsForDownload << uid;

      delete msg;
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
  //kdDebug(5006) << "KMFolderCachedImap::slotProgress done=" << done << " total=" << total << "=> progress=" << mProgress + ( 20 * done ) / total << endl;
  // Progress info while retrieving new emails
  // (going from mProgress to mProgress+20)
  emit newState( name(), mProgress + (20 * done) / total, QString::null);
}


void KMFolderCachedImap::setAccount(KMAcctCachedImap *aAccount)
{
  assert( aAccount->isA("KMAcctCachedImap") );
  mAccount = aAccount;
  if( imapPath()=="/" ) aAccount->setFolder( folder() );

  if( !folder() || !folder()->child() || !folder()->child()->count() ) return;
  for( KMFolderNode* node = folder()->child()->first(); node;
       node = folder()->child()->next() )
    if (!node->isDir())
      static_cast<KMFolderCachedImap*>(static_cast<KMFolder*>(node)->storage())->setAccount(aAccount);
}


// This synchronizes the subfolders with the server
bool KMFolderCachedImap::listDirectory(bool secondStep)
{
  mSubfolderState = imapInProgress;
  if( mAccount->makeConnection() != ImapAccountBase::Connected ) {
    emit folderComplete( this, false );
    return false;
  }

  // connect to folderlisting
  connect(mAccount, SIGNAL(receivedFolders(QStringList, QStringList,
          QStringList, const ImapAccountBase::jobData &)),
      this, SLOT(slotListResult(QStringList, QStringList,
          QStringList, const ImapAccountBase::jobData &)));

  // start a new listing for the root-folder
  bool reset = ( mImapPath == mAccount->prefix() && 
                !secondStep && !folder()->isSystemFolder() ) ? true : false;

  // get the folders
  mAccount->listDirectory(mImapPath, mAccount->onlySubscribedFolders(),
                          secondStep, folder(), reset);

  return true;
}

void KMFolderCachedImap::slotListResult( QStringList mFolderNames,
                                         QStringList mFolderPaths,
                                         QStringList mFolderMimeTypes,
                                         const ImapAccountBase::jobData & jobData )
{
  mSubfolderNames = mFolderNames;
  mSubfolderPaths = mFolderPaths;
  mSubfolderMimeTypes = mFolderMimeTypes;
  if (jobData.parent) {
    // the account is connected to several folders, so we
    // have to sort out if this result is for us
    if (jobData.parent != folder()) return;
  }
  // disconnect to avoid recursions
  disconnect(mAccount, SIGNAL(receivedFolders(QStringList, QStringList,
          QStringList, const ImapAccountBase::jobData &)),
      this, SLOT(slotListResult(QStringList, QStringList,
          QStringList, const ImapAccountBase::jobData &)));

  mSubfolderState = imapFinished;
  bool it_inboxOnly = jobData.inboxOnly;

  if (it_inboxOnly) {
    // list again only for the INBOX
    listDirectory(TRUE);
    return;
  }

  if ( folder()->isSystemFolder() && mImapPath == "/INBOX/"
       && mAccount->prefix() == "/INBOX/" )
  {
    // do not create folders under INBOX
    mAccount->setCreateInbox(FALSE);
    mSubfolderNames.clear();
  }
  folder()->createChildFolder();
  // Find all subfolders present on disk but not on the server
  KMFolderCachedImap *f;
  KMFolderNode *node = folder()->child()->first();
  QPtrList<KMFolder> toRemove;
  while (node) {
    if (!node->isDir() ) {
      if ( mSubfolderNames.findIndex(node->name()) == -1 && 
          (node->name().upper() != "INBOX" || !mAccount->createInbox()) ) 
      {
        // This subfolder isn't present on the server
        kdDebug(5006) << node->name() << " isn't on the server." << endl;
        f = static_cast<KMFolderCachedImap*>(static_cast<KMFolder*>(node)->storage());
        if( !f->uidValidity().isEmpty() ) {
          // The folder have a uidValidity setting, so it has been on the
          // server before. Delete it locally.
          toRemove.append( f->folder() );
        }
      }
    }
    node = folder()->child()->next();
  }
  // Remove all folders
  for ( KMFolder* doomed=toRemove.first(); doomed; doomed = toRemove.next() )
    kmkernel->dimapFolderMgr()->remove( doomed );

  mAccount->displayProgress();
  serverSyncInternal();
}


void KMFolderCachedImap::listDirectory2() {
  foldersForDeletionOnServer.clear();
  QString path = folder()->path();
  KMFolderCachedImap *f = 0;

  if (mAccount->createInbox())
  {
    KMFolderNode *node;
    // create the INBOX
    for (node = folder()->child()->first(); node; node = folder()->child()->next())
      if (!node->isDir() && node->name() == "INBOX") break;
    if (node) f = static_cast<KMFolderCachedImap*>(static_cast<KMFolder*>(node)->storage());
    else f = static_cast<KMFolderCachedImap*>
      (folder()->child()->createFolder("INBOX", true, KMFolderTypeCachedImap)->storage());
    f->setAccount(mAccount);
    f->setImapPath("/INBOX/");
    f->setLabel(i18n("inbox"));
    if (!node) f->close();
    // so we have an INBOX
    mAccount->setCreateInbox( false );
    mAccount->setHasInbox( true );
//    f->listDirectory();
    kmkernel->dimapFolderMgr()->contentsChanged();
  }

  // Find all subfolders present on server but not on disk
  for (uint i = 0; i < mSubfolderNames.count(); i++) {

    if (mSubfolderNames[i].upper() == "INBOX" &&
        mAccount->hasInbox()) // do not create an additional inbox
      continue;

    // Find the subdir, if already present
    KMFolderNode *node;
    for (node = folder()->child()->first(); node;
         node = folder()->child()->next())
      if (!node->isDir() && node->name() == mSubfolderNames[i]) break;

    if (!node) {
      // This folder is not present here
      QString part1 = path + "/." + dotEscape(name()) + ".directory/."
        + dotEscape(mSubfolderNames[i]);
      QString uidCacheFile = part1 + ".uidcache";
      if( QFile::exists(uidCacheFile) ) {
        // This is an old folder that is deleted locally - delete it on the server
        unlink( QFile::encodeName( uidCacheFile ) );
        foldersForDeletionOnServer << mSubfolderPaths[i];
        // Make sure all trace of the dir is gone
        KIO::del( KURL::fromPathOrURL( part1 + ".directory" ) );
      } else {
        // This is a new folder, create the local cache
        f = static_cast<KMFolderCachedImap*>
          (folder()->child()->createFolder(mSubfolderNames[i], false, KMFolderTypeCachedImap)->storage());
        if (f) {
          f->close();
          f->setAccount(mAccount);
          kmkernel->dimapFolderMgr()->contentsChanged();
        } else {
          kdDebug(5006) << "can't create folder " << mSubfolderNames[i] <<endl;
        }
      }
    } else {
      if( static_cast<KMFolder*>(node)->folderType() == KMFolderTypeCachedImap )
        f = static_cast<KMFolderCachedImap*>(static_cast<KMFolder*>(node)->storage());
    }

    if( f && f->imapPath().isEmpty() ) {
      // kdDebug(5006) << "folder("<<f->name()<<")->imapPath()=" << f->imapPath()
      // << "\nAssigning new imapPath " << mSubfolderPaths[i] << endl;
      // Write folder settings
      f->setAccount(mAccount);
      f->setNoContent(mSubfolderMimeTypes[i] == "inode/directory");
      f->setNoChildren(mSubfolderMimeTypes[i] == "message/digest");
      f->setImapPath(mSubfolderPaths[i]);
    }
  }

  emit listComplete(this);
  serverSyncInternal();
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


FolderJob*
KMFolderCachedImap::doCreateJob( KMMessage *msg, FolderJob::JobType jt, KMFolder *folder,
                                 QString, const AttachmentStrategy* ) const
{
  QPtrList<KMMessage> msgList;
  msgList.append( msg );
  CachedImapJob *job = new CachedImapJob( msgList, jt, folder? static_cast<KMFolderCachedImap*>( folder->storage() ):0 );
  job->setParentFolder( this );
  return job;
}

FolderJob*
KMFolderCachedImap::doCreateJob( QPtrList<KMMessage>& msgList, const QString& sets,
                                 FolderJob::JobType jt, KMFolder *folder ) const
{
  //FIXME: how to handle sets here?
  Q_UNUSED( sets );
  CachedImapJob *job = new CachedImapJob( msgList, jt, folder? static_cast<KMFolderCachedImap*>( folder->storage() ):0 );
  job->setParentFolder( this );
  return job;
}

#include "kmfoldercachedimap.moc"
