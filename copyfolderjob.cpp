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

#include "copyfolderjob.h"
#include "folderstorage.h"
#include "kmfolder.h"
#include "kmfolderdir.h"
#include "kmfoldertype.h"
#include "kmfoldermgr.h"
#include "kmcommands.h"
#include "kmmsgbase.h"
#include "undostack.h"

#include <kdebug.h>
#include <klocale.h>
#include <config.h>

using namespace KMail;

CopyFolderJob::CopyFolderJob( const FolderStorage* const storage, KMFolderDir* const newParent )
 : FolderJob( 0, tOther, (storage ? storage->folder() : 0) ),
   mStorage( storage ), mNewParent( newParent ),
   mNewFolder( 0 ), mChildFolderNodeIterator( *mStorage->folder()->createChildFolder() ), 
   mNextChildFolder( 0 )
{
}

CopyFolderJob::~CopyFolderJob()
{
  kdDebug(5006) << k_funcinfo << endl;
}

/*
 * The basic strategy is to first create the target folder, then copy all the mail
 * from the source to the target folder, then recurse for each of the folder's children
 */
void CopyFolderJob::execute()
{
  createTargetDir();
  copyMessagesToTargetDir();
}

void CopyFolderJob::copyMessagesToTargetDir()
{
  // Hmmmm. Tasty hack. Can I have fries with that?
  const_cast<FolderStorage*>( mStorage )->blockSignals( true );
  // move all messages to the new folder
  QPtrList<KMMsgBase> msgList;
  for ( int i = 0; i < mStorage->count(); i++ )
  {
    const KMMsgBase* msgBase = mStorage->getMsgBase( i );
    assert( msgBase );
    msgList.append( msgBase );
  }
  if ( msgList.count() == 0 ) {
    slotCopyNextChild(); // no contents, check subfolders
    const_cast<FolderStorage*>( mStorage )->blockSignals( false );
  } else {
    KMCommand *command = new KMCopyCommand( mNewFolder, msgList );
    connect( command, SIGNAL( completed( KMCommand * ) ),
        this, SLOT( slotCopyCompleted( KMCommand * ) ) );
    command->start();
  }
}

void CopyFolderJob::slotCopyCompleted( KMCommand* command )
{
  kdDebug(5006) << k_funcinfo << (command?command->result():0) << endl;
  disconnect( command, SIGNAL( completed( KMCommand * ) ),
      this, SLOT( slotCopyCompleted( KMCommand * ) ) );
  
  const_cast<FolderStorage*>( mStorage )->blockSignals( false );
  
  if ( command && command->result() != KMCommand::OK ) {
    rollback();
  }
  // if we have children, recurse
  if ( mStorage->folder()->child() ) {
    slotCopyNextChild();
  } else {
    emit folderCopyComplete( true );
    deleteLater();
  }
}

void CopyFolderJob::slotCopyNextChild( bool success )
{
  //kdDebug(5006) << k_funcinfo << endl;
  if ( mNextChildFolder ) 
    mNextChildFolder->close(); // refcount
  // previous sibling failed
  if ( !success ) {
    kdDebug(5006) << "Failed to copy one subfolder, let's not continue:  " << mNewFolder->prettyURL() << endl;
    rollback();
    emit folderCopyComplete( false );
    deleteLater();
  }

  KMFolderNode* node = mChildFolderNodeIterator.current();
  while ( node && node->isDir() ) {
    ++mChildFolderNodeIterator;
    node = mChildFolderNodeIterator.current();
  }
  if ( node ) {
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
    kdDebug(5006) << "Failed to create subfolders of:  " << mNewFolder->prettyURL() << endl;
    emit folderCopyComplete( false );
    deleteLater();
    return;
  }
  // let it do its thing and report back when we are ready to do the next sibling
  mNextChildFolder->open(); // refcount
  FolderJob* job = new CopyFolderJob( mNextChildFolder->storage(), dir);
  connect( job, SIGNAL( folderCopyComplete( bool ) ),
           this, SLOT( slotCopyNextChild( bool ) ) );
  job->start();
}


// FIXME factor into CreateFolderJob and make async, so it works with online imap
// FIXME this is the same in renamejob. Refactor RenameJob to use a copy job and then delete
void CopyFolderJob::createTargetDir()
{
    KMFolderMgr* folderMgr = kmkernel->folderMgr();
    if ( mNewParent->type() == KMImapDir ) {
      folderMgr = kmkernel->imapFolderMgr();
    } else if ( mNewParent->type() == KMDImapDir ) {
      folderMgr = kmkernel->dimapFolderMgr();
    }

    // get the default mailbox type
    KConfig * const config = KMKernel::config();
    KConfigGroupSaver saver(config, "General");
    int deftype = config->readNumEntry("default-mailbox-format", 1);
    if ( deftype < 0 || deftype > 1 ) deftype = 1;

    // the type of the new folder
    KMFolderType typenew = 
      ( deftype == 0 ) ? KMFolderTypeMbox : KMFolderTypeMaildir;
    if ( mNewParent->owner() )
      typenew = mNewParent->owner()->folderType();

    mNewFolder = folderMgr->createFolder( mStorage->name(), false, typenew, mNewParent );
    if ( !mNewFolder )
    {
      kdWarning(5006) << k_funcinfo << "could not create folder" << endl;
      emit folderCopyComplete( false );
      deleteLater();
      return;
    }
    // inherit the folder type
    // FIXME we should probably copy over most if not all settings
    mNewFolder->storage()->setContentsType( mStorage->contentsType() );
    mNewFolder->storage()->writeConfig();
    kdDebug(5006)<< "CopyJob::createTargetDir - " << mStorage->folder()->idString()
      << " |=> " << mNewFolder->idString() << endl;
}


void CopyFolderJob::rollback()
{
  // FIXME do something
  KMFolderMgr * const folderMgr = mNewFolder->createChildFolder()->manager();
  folderMgr->remove( mNewFolder );
  emit folderCopyComplete( false );
  deleteLater();
}
#include "copyfolderjob.moc"
