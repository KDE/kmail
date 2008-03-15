/**
 *  Copyright (c) 2005 Till Adam <adam@kde.org>
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
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

#include "copyfolderjob.h"
#include "folderstorage.h"
#include "kmacctcachedimap.h"
#include "kmfoldercachedimap.h"
#include "kmfolder.h"
#include "kmfolderdir.h"
#include "kmfoldertype.h"
#include "kmfoldermgr.h"
#include "kmcommands.h"
#include "kmmsgbase.h"
#include "undostack.h"

#include <kconfiggroup.h>
#include <kdebug.h>
#include <klocale.h>

using namespace KMail;

CopyFolderJob::CopyFolderJob( FolderStorage* const storage, KMFolderDir* const newParent )
 : FolderJob( 0, tOther, (storage ? storage->folder() : 0) ),
   mStorage( storage ), mNewParent( newParent ),
   mNewFolder( 0 ), mNextChildFolder( 0 )
{
  if ( mStorage->folder()->child() && 
       mStorage->folder()->child()->size() > 0 ) {
    mHasChildFolders = true;
    mChildFolderNodeIterator = mStorage->folder()->child()->begin();
  }
  else
    mHasChildFolders = false;

  mStorage->open( "copyfolder" );
}

CopyFolderJob::~CopyFolderJob()
{
  kDebug(5006) ;
  if ( mNewFolder )
    mNewFolder->setMoveInProgress( false );
  if ( mStorage )
  {
    mStorage->folder()->setMoveInProgress( false );
    mStorage->close( "copyfolder" );
  }
}

/*
 * The basic strategy is to first create the target folder, then copy all the mail
 * from the source to the target folder, then recurse for each of the folder's children
 */
void CopyFolderJob::execute()
{
  if ( createTargetDir() ) {
    copyMessagesToTargetDir();
  }
}

void CopyFolderJob::copyMessagesToTargetDir()
{
  // Hmmmm. Tasty hack. Can I have fries with that?
  mStorage->blockSignals( true );
  // move all messages to the new folder
  QList<KMMsgBase*> msgList;
  for ( int i = 0; i < mStorage->count(); i++ )
  {
    KMMsgBase* msgBase = mStorage->getMsgBase( i );
    assert( msgBase );
    msgList.append( msgBase );
  }
  if ( msgList.count() == 0 ) {
    mStorage->blockSignals( false );
    // ### be careful, after slotCopyNextChild() the source folder
    // (including mStorage) might already be deleted!
    slotCopyNextChild(); // no contents, check subfolders
  } else {
    KMCommand *command = new KMCopyCommand( mNewFolder, msgList );
    connect( command, SIGNAL( completed( KMCommand * ) ),
        this, SLOT( slotCopyCompleted( KMCommand * ) ) );
    command->start();
  }
}

void CopyFolderJob::slotCopyCompleted( KMCommand* command )
{
  kDebug(5006) << (command?command->result():0);
  disconnect( command, SIGNAL( completed( KMCommand * ) ),
      this, SLOT( slotCopyCompleted( KMCommand * ) ) );

  mStorage->blockSignals( false );

  if ( command && command->result() != KMCommand::OK ) {
    rollback();
    return;
  }
  // if we have children, recurse
  if ( mHasChildFolders )
    slotCopyNextChild();
  else {
    emit folderCopyComplete( true );
    deleteLater();
  }
}

void CopyFolderJob::slotCopyNextChild( bool success )
{
  if ( mNextChildFolder )
    mNextChildFolder->close( "copyfolder" ); // refcount
  // previous sibling failed
  if ( !success ) {
    kDebug(5006) <<"Failed to copy one subfolder, let's not continue:" << mNewFolder->prettyUrl();
    rollback();
    emit folderCopyComplete( false );
    deleteLater();
  }

  //Attempt to find the next child folder which is not a directory
  KMFolderNode* node = 0;
  bool folderFound = false;
  if ( mHasChildFolders )
    for ( ; mChildFolderNodeIterator != mStorage->folder()->child()->end();
          ++mChildFolderNodeIterator ) {
      node = *mChildFolderNodeIterator;
      if ( !node->isDir() ) {
        folderFound = true;
        break;
      }
    }

  if ( folderFound ) {
    mNextChildFolder = static_cast<KMFolder*>(node);
    ++mChildFolderNodeIterator;
  } else {
    // no more children, we are done
    emit folderCopyComplete( true );
    deleteLater();
    return;
  }

  KMFolderDir * const dir = mNewFolder->createChildFolder();
  if ( !dir ) {
    kDebug(5006) <<"Failed to create subfolders of:" << mNewFolder->prettyUrl();
    emit folderCopyComplete( false );
    deleteLater();
    return;
  }
  // let it do its thing and report back when we are ready to do the next sibling
  mNextChildFolder->open( "copyfolder" ); // refcount
  FolderJob* job = new CopyFolderJob( mNextChildFolder->storage(), dir);
  connect( job, SIGNAL( folderCopyComplete( bool ) ),
           this, SLOT( slotCopyNextChild( bool ) ) );
  job->start();
}


// FIXME factor into CreateFolderJob and make async, so it works with online imap
// (create folder code taken from newfolderdialog.cpp)
bool CopyFolderJob::createTargetDir()
{
  // get the default mailbox type
  KConfig * const config = KMKernel::config();
  KConfigGroup saver(config, "General");
  int deftype = saver.readEntry("default-mailbox-format", 1);
  if ( deftype < 0 || deftype > 1 ) deftype = 1;

  // the type of the new folder
  KMFolderType typenew =
    ( deftype == 0 ) ? KMFolderTypeMbox : KMFolderTypeMaildir;
  if ( mNewParent->owner() )
    typenew = mNewParent->owner()->folderType();

  bool success = false, waitForFolderCreation = false;

  if ( mNewParent->owner() && mNewParent->owner()->folderType() == KMFolderTypeImap ) {
    KMFolderImap* selectedStorage = static_cast<KMFolderImap*>( mNewParent->owner()->storage() );
    KMAcctImap *anAccount = selectedStorage->account();
    // check if a connection is available BEFORE creating the folder
    if (anAccount->makeConnection() == ImapAccountBase::Connected) {
      mNewFolder = kmkernel->imapFolderMgr()->createFolder( mStorage->folder()->name(), false, typenew, mNewParent );
      if ( mNewFolder ) {
        QString imapPath;
        imapPath = anAccount->createImapPath( selectedStorage->imapPath(), mStorage->folder()->name() );
        KMFolderImap* newStorage = static_cast<KMFolderImap*>( mNewFolder->storage() );
        connect( selectedStorage, SIGNAL(folderCreationResult(const QString&, bool)),
                 this, SLOT(folderCreationDone(const QString&, bool)) );
        selectedStorage->createFolder( mStorage->folder()->name(), QString() ); // create it on the server
        newStorage->initializeFrom( selectedStorage, imapPath, QString() );
        static_cast<KMFolderImap*>(mNewParent->owner()->storage())->setAccount( selectedStorage->account() );
        waitForFolderCreation = true;
        success = true;
      }
    }
  } else if ( mNewParent->owner() && mNewParent->owner()->folderType() == KMFolderTypeCachedImap ) {
    mNewFolder = kmkernel->dimapFolderMgr()->createFolder( mStorage->folder()->name(), false, typenew, mNewParent );
    if ( mNewFolder ) {
      KMFolderCachedImap* selectedStorage = static_cast<KMFolderCachedImap*>( mNewParent->owner()->storage() );
      KMFolderCachedImap* newStorage = static_cast<KMFolderCachedImap*>( mNewFolder->storage() );
      newStorage->initializeFrom( selectedStorage );
      success = true;
    }
  } else {
    // local folder
    mNewFolder = kmkernel->folderMgr()->createFolder(mStorage->folder()->name(), false, typenew, mNewParent );
    if ( mNewFolder )
      success = true;
  }

  if ( !success ) {
      kWarning(5006) <<"could not create folder";
      emit folderCopyComplete( false );
      deleteLater();
      return false;
  }

  mNewFolder->setMoveInProgress( true );
  mStorage->folder()->setMoveInProgress( true );

  // inherit the folder type
  // FIXME we should probably copy over most if not all settings
  mNewFolder->storage()->setContentsType( mStorage->contentsType(), true /*quiet*/ );
  mNewFolder->storage()->writeConfig();
  kDebug(5006) << mStorage->folder()->idString() << "|=>" << mNewFolder->idString();
  return !waitForFolderCreation;
}


void CopyFolderJob::rollback()
{
  // copy failed - rollback the last transaction
//   kmkernel->undoStack()->undo();
  // .. and delete the new folder
  if ( mNewFolder ) {
    if ( mNewFolder->folderType() == KMFolderTypeImap )
    {
      kmkernel->imapFolderMgr()->remove( mNewFolder );
    } else if ( mNewFolder->folderType() == KMFolderTypeCachedImap )
    {
      // tell the account (see KMFolderCachedImap::listDirectory2)
      KMFolderCachedImap* folder = static_cast<KMFolderCachedImap*>(mNewFolder->storage());
      KMAcctCachedImap* acct = folder->account();
      if ( acct )
        acct->addDeletedFolder( folder->imapPath() );
      kmkernel->dimapFolderMgr()->remove( mNewFolder );
    } else if ( mNewFolder->folderType() == KMFolderTypeSearch )
    {
      // invalid
      kWarning(5006) <<"cannot remove a search folder";
    } else {
      kmkernel->folderMgr()->remove( mNewFolder );
    }
  }

  emit folderCopyComplete( false );
  deleteLater();
}

void CopyFolderJob::folderCreationDone(const QString & name, bool success)
{
  if ( mStorage->folder()->name() != name )
    return; // not our business
  kDebug(5006) << success;

  if ( !success ) {
    rollback();
  } else {
    copyMessagesToTargetDir();
  }
}
#include "copyfolderjob.moc"
