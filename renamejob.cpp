/**
 * Copyright (c) 2004 Carsten Burghardt <burghardt@kde.org>
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

#include "renamejob.h"
#include "kmfolderimap.h"
#include "kmfoldercachedimap.h"
#include "folderstorage.h"
#include "kmfolder.h"
#include "kmfolderdir.h"
#include "kmfoldertype.h"
#include "kmfoldermgr.h"
#include "imapaccountbase.h"
#include "kmacctimap.h"
#include "kmacctcachedimap.h"
#include "kmcommands.h"
#include "kmmsgbase.h"

#include <kdebug.h>
#include <kurl.h>
#include <kio/scheduler.h>
#include <kio/job.h>
#include <kio/global.h>
#include <klocale.h>
#include <config.h>

#include <qmap.h>

using namespace KMail;

RenameJob::RenameJob( FolderStorage* storage, const QString& newName, 
    KMFolderDir* newParent )
 : FolderJob( 0, tOther, (storage ? storage->folder() : 0) ),
   mStorage( storage ), mNewParent( newParent ),
   mNewName( newName ), mNewFolder( 0 )
{
  mOldName = storage->name();
  if ( storage->folderType() == KMFolderTypeImap ) {
    mOldImapPath = static_cast<KMFolderImap*>(storage)->imapPath();
  } else if ( storage->folderType() == KMFolderTypeCachedImap ) {
    mOldImapPath = static_cast<KMFolderCachedImap*>(storage)->imapPath();
  }
}

RenameJob::~RenameJob()
{
}

void RenameJob::execute()
{
  if ( mNewParent )
  {
    // move the folder to a different parent
    KMFolderType type = mStorage->folderType();
    if ( ( type == KMFolderTypeMbox || type == KMFolderTypeMaildir ) &&
         mNewParent->type() == KMStandardDir )
    {
      // local folders can handle this on their own
      mStorage->rename( mNewName, mNewParent );
      emit renameDone( mNewName, true );
      deleteLater();
      return;
    }
    // first create the new folder
    KMFolderMgr* folderMgr = kmkernel->folderMgr();
    if ( mNewParent->type() == KMImapDir ) {
      folderMgr = kmkernel->imapFolderMgr();
    } else if ( mNewParent->type() == KMDImapDir ) {
      folderMgr = kmkernel->dimapFolderMgr();
    }

    // get the default mailbox type
    KConfig *config = KMKernel::config();
    KConfigGroupSaver saver(config, "General");
    int deftype = config->readNumEntry("default-mailbox-format", 1);
    if ( deftype < 0 || deftype > 1 ) deftype = 1;

    // the type of the new folder
    KMFolderType typenew = 
      ( deftype == 0 ) ? KMFolderTypeMbox : KMFolderTypeMaildir;
    if ( mNewParent->owner() )
      typenew = mNewParent->owner()->folderType();

    mNewFolder = folderMgr->createFolder( mNewName, false, typenew, mNewParent );
    if ( !mNewFolder )
    {
      kdWarning(5006) << k_funcinfo << "could not create folder" << endl;
      emit renameDone( mNewName, false );
      deleteLater();
      return;
    }
    kdDebug(5006)<< "RenameJob::rename - " << mStorage->folder()->idString()
      << " |=> " << mNewFolder->idString() << endl;

    if ( mNewParent->type() == KMImapDir )
    {
      // online imap
      // create it on the server and wait for the folderAdded signal
      connect( kmkernel->imapFolderMgr(), SIGNAL( changed() ),
          this, SLOT( slotMoveMessages() ) );
      KMFolderImap* imapFolder = 
        static_cast<KMFolderImap*>(mNewParent->owner()->storage());
      imapFolder->createFolder( mNewName );
    } else if ( mNewParent->type() == KMDImapDir )
    {
      KMFolderCachedImap* newStorage = static_cast<KMFolderCachedImap*>(mNewFolder->storage());
      KMFolderCachedImap* owner = static_cast<KMFolderCachedImap*>(mNewParent->owner()->storage());
      newStorage->initializeFrom( owner );
      slotMoveMessages();
    } else
    {
      // local
      slotMoveMessages();
    }
  } else
  {
    // only rename the folder
    if ( mStorage->folderType() != KMFolderTypeImap )
    {
      // local and dimap folder handle this directly
      mStorage->rename( mNewName );
      emit renameDone( mNewName, true );
      deleteLater();
      return;
    }
    if ( mOldImapPath.isEmpty() )
    { 
      // sanity
      emit renameDone( mNewName, false );
      deleteLater();
      return;
    }
    ImapAccountBase* account = static_cast<KMFolderImap*>(mStorage)->account();
    // first rename it on the server
    mNewImapPath = mOldImapPath;
    mNewImapPath = mNewImapPath.replace( mOldName, mNewName );
    KURL src( account->getUrl() );
    src.setPath( mOldImapPath );
    KURL dst( account->getUrl() );
    dst.setPath( mNewImapPath );
    KIO::SimpleJob *job = KIO::rename( src, dst, true );
    kdDebug(5006)<< "RenameJob::rename - " << src.prettyURL()
      << " |=> " << dst.prettyURL() << endl;
    ImapAccountBase::jobData jd( src.url() );
    account->insertJob( job, jd );
    KIO::Scheduler::assignJobToSlave( account->slave(), job );
    connect( job, SIGNAL(result(KIO::Job*)),
        SLOT(slotRenameResult(KIO::Job*)) );
  }
}

void RenameJob::slotRenameResult( KIO::Job *job )
{
  ImapAccountBase* account = static_cast<KMFolderImap*>(mStorage)->account();
  ImapAccountBase::JobIterator it = account->findJob(job);
  if ( it == account->jobsEnd() )
  {
    emit renameDone( mNewName, false );
    deleteLater();
    return;
  }
  if ( job->error() )
  {
    account->handleJobError( job, i18n("Error while renaming a folder.") );
    emit renameDone( mNewName, false );
    deleteLater();
    return;
  }
  account->removeJob(it);
  // set the new path
  if ( mStorage->folderType() == KMFolderTypeImap )
    static_cast<KMFolderImap*>(mStorage)->setImapPath( mNewImapPath );
  // unsubscribe old (we don't want ghosts)
  account->changeSubscription( false, mOldImapPath );
  // subscribe new
  account->changeSubscription( true, mNewImapPath );

  // local part (will set the new name)
  mStorage->rename( mNewName );

  emit renameDone( mNewName, true );
  deleteLater();
}

void RenameJob::slotMoveMessages()
{
  kdDebug(5006) << k_funcinfo << endl;
  disconnect( kmkernel->imapFolderMgr(), SIGNAL( changed() ),
      this, SLOT( slotMoveMessages() ) );
  // move all messages to the new folder
  QPtrList<KMMsgBase> msgList;
  for ( int i = 0; i < mStorage->count(); i++ )
  {
    KMMsgBase* msgBase = mStorage->getMsgBase( i );
    assert( msgBase );
    msgList.append( msgBase );
  }
  if ( msgList.count() == 0 ) 
  {
    slotMoveCompleted( 0 );
  } else
  {
    KMCommand *command = new KMMoveCommand( mNewFolder, msgList );
    connect( command, SIGNAL( completed( KMCommand * ) ),
        this, SLOT( slotMoveCompleted( KMCommand * ) ) );
    command->start();
  }
}

void RenameJob::slotMoveCompleted( KMCommand* command )
{
  kdDebug(5006) << k_funcinfo << (command?command->result():0) << endl;
  if ( !command || command->result() == KMCommand::OK ) 
  {
    kdDebug(5006) << "deleting old folder" << endl;
    // move complete or not necessary
    // save our settings
    QString oldconfig = "Folder-" + mStorage->folder()->idString();
    KConfig* config = KMKernel::config();
    QMap<QString, QString> entries = config->entryMap( oldconfig );
    KConfigGroupSaver saver(config, "Folder-" + mNewFolder->idString());
    for ( QMap<QString, QString>::Iterator it = entries.begin(); 
          it != entries.end(); ++it ) 
    {
      if ( it.key() == "Id" || it.key() == "ImapPath" || 
           it.key() == "UidValidity" )
        continue;
      config->writeEntry( it.key(), it.data() );
    }
    mNewFolder->readConfig( config );
      
    // delete the old folder
    if ( mStorage->folderType() == KMFolderTypeImap )
    {
      kmkernel->imapFolderMgr()->remove( mStorage->folder() );
    } else if ( mStorage->folderType() == KMFolderTypeCachedImap ) 
    {
      // tell the account (see KMFolderCachedImap::listDirectory2)
      KMAcctCachedImap* acct = static_cast<KMFolderCachedImap*>(mStorage)->account();
      if ( acct )
        acct->addDeletedFolder( mOldImapPath );
      kmkernel->dimapFolderMgr()->remove( mStorage->folder() );
    } else if ( mStorage->folderType() == KMFolderTypeSearch ) 
    {
      // invalid
      kdWarning(5006) << k_funcinfo << "cannot remove a search folder" << endl;
    } else {
      kmkernel->folderMgr()->remove( mStorage->folder() );
    }
    
    emit renameDone( mNewName, true );
  } else 
  {
    kdDebug(5006) << "rollback - deleting folder" << endl;
    // move failed - rollback and delete the new folder
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
      kdWarning(5006) << k_funcinfo << "cannot remove a search folder" << endl;
    } else {
      kmkernel->folderMgr()->remove( mNewFolder );
    }
    
    emit renameDone( mNewName, false );
  }
  deleteLater();
}

#include "renamejob.moc"
