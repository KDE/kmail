/*
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

#include "renamejob.h"
#include "kmfolderimap.h"
#include "kmfoldercachedimap.h"
#include "folderstorage.h"
#include "kmfolder.h"
#include "kmfolderdir.h"
#include "kmfoldermgr.h"
#include "imapaccountbase.h"
#include "kmacctimap.h"
#include "kmacctcachedimap.h"
#include "kmcommands.h"
#include "kmmsgbase.h"
#include "undostack.h"

#include <kdebug.h>
#include <kurl.h>
#include <kio/scheduler.h>
#include <kio/job.h>
#include <kio/global.h>
#include <klocale.h>
#include <config.h>
#include <kconfiggroup.h>

#include <QMap>

using namespace KMail;

RenameJob::RenameJob( FolderStorage* storage, const QString& newName,
    KMFolderDir* newParent )
 : FolderJob( 0, tOther, (storage ? storage->folder() : 0) ),
   mStorage( storage ), mNewParent( newParent ),
   mNewName( newName ), mNewFolder( 0 )
{
  mStorageTempOpened = 0;
  if ( storage ) {
    mOldName = storage->objectName();
    if ( storage->folderType() == KMFolderTypeImap ) {
      mOldImapPath = static_cast<KMFolderImap*>(storage)->imapPath();
    } else if ( storage->folderType() == KMFolderTypeCachedImap ) {
      mOldImapPath = static_cast<KMFolderCachedImap*>(storage)->imapPath();
    }
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
         mNewParent->type() == KMStandardDir &&
         mStorage->folderType() != KMFolderTypeCachedImap )
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
    KConfigGroup group(config, "General");
    int deftype = group.readEntry("default-mailbox-format", 1 );
    if ( deftype < 0 || deftype > 1 ) deftype = 1;

    // the type of the new folder
    KMFolderType typenew =
      ( deftype == 0 ) ? KMFolderTypeMbox : KMFolderTypeMaildir;
    if ( mNewParent->owner() )
      typenew = mNewParent->owner()->folderType();

    mNewFolder = folderMgr->createFolder( mNewName, false, typenew, mNewParent );
    if ( !mNewFolder )
    {
      kWarning(5006) << k_funcinfo << "could not create folder" << endl;
      emit renameDone( mNewName, false );
      deleteLater();
      return;
    }
    kDebug(5006)<< "RenameJob::rename - " << mStorage->folder()->idString()
      << " |=> " << mNewFolder->idString() << endl;

    if ( mNewParent->type() == KMImapDir )
    {
      // online imap
      // create it on the server and wait for the folderAdded signal
      // do not ask the user what kind of folder should be created
      connect( kmkernel->imapFolderMgr(), SIGNAL( changed() ),
          this, SLOT( slotMoveMessages() ) );
      KMFolderImap* imapFolder =
        static_cast<KMFolderImap*>(mNewParent->owner()->storage());
      imapFolder->createFolder( mNewName, QString(), false );
    } else if ( mNewParent->type() == KMDImapDir )
    {
      KMFolderCachedImap* newStorage = static_cast<KMFolderCachedImap*>(mNewFolder->storage());
      KMFolderCachedImap* owner = static_cast<KMFolderCachedImap*>(mNewParent->owner()->storage());
      newStorage->initializeFrom( owner );
      moveSubFoldersBeforeMessages();
    } else if ( mStorage->folderType() == KMFolderTypeCachedImap )
    {
      // from (d)IMAP to local account
      moveSubFoldersBeforeMessages();
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
    } else if ( mOldName == mNewName || mOldImapPath == "/INBOX/" ) {
      emit renameDone( mNewName, true ); // noop
      deleteLater();
      return;
    }
    ImapAccountBase* account = static_cast<KMFolderImap*>(mStorage)->account();
    // first rename it on the server
    mNewImapPath = mOldImapPath;
    mNewImapPath = mNewImapPath.replace( mOldName, mNewName );
    KUrl src( account->getUrl() );
    src.setPath( mOldImapPath );
    KUrl dst( account->getUrl() );
    dst.setPath( mNewImapPath );
    KIO::SimpleJob *job = KIO::rename( src, dst, true );
    kDebug(5006)<< "RenameJob::rename - " << src.prettyUrl()
      << " |=> " << dst.prettyUrl() << endl;
    ImapAccountBase::jobData jd( src.url() );
    account->insertJob( job, jd );
    KIO::Scheduler::assignJobToSlave( account->slave(), job );
    connect( job, SIGNAL(result(KJob*)),
        SLOT(slotRenameResult(KJob*)) );
  }
}

void RenameJob::slotRenameResult( KJob *job )
{
  ImapAccountBase* account = static_cast<KMFolderImap*>(mStorage)->account();
  ImapAccountBase::JobIterator it = account->findJob(static_cast<KIO::Job*>(job));
  if ( it == account->jobsEnd() )
  {
    emit renameDone( mNewName, false );
    deleteLater();
    return;
  }
  if ( job->error() )
  {
    account->handleJobError( static_cast<KIO::Job*>(job), i18n("Error while renaming a folder.") );
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
  kDebug(5006) << k_funcinfo << endl;
  disconnect( kmkernel->imapFolderMgr(), SIGNAL( changed() ),
      this, SLOT( slotMoveMessages() ) );
  mStorage->blockSignals( true );
  // move all messages to the new folder
  QList<KMMsgBase*> msgList;
  if ( !mStorage->isOpened() )
    mStorageTempOpened = mStorage->open() ? mStorage : 0;
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
  kDebug(5006) << k_funcinfo << (command?command->result():0) << endl;
  if ( mStorageTempOpened ) {
    mStorageTempOpened->close();
    mStorageTempOpened = 0;
  }
  if ( command ) {
    // just make sure nothing bounces
    disconnect( command, SIGNAL( completed( KMCommand * ) ),
        this, SLOT( slotMoveCompleted( KMCommand * ) ) );
  }
  if ( !command || command->result() == KMCommand::OK )
  {
    kDebug(5006) << "deleting old folder" << endl;
    // move complete or not necessary
    // save our settings
    QString oldconfig = "Folder-" + mStorage->folder()->idString();
    KConfig* config = KMKernel::config();
    QMap<QString, QString> entries = config->entryMap( oldconfig );
    KConfigGroup group(config, "Folder-" + mNewFolder->idString());
    for ( QMap<QString, QString>::Iterator it = entries.begin();
          it != entries.end(); ++it )
    {
      if ( it.key() == "Id" || it.key() == "ImapPath" ||
           it.key() == "UidValidity" )
        continue;
      group.writeEntry( it.key(), it.value() );
    }
    mNewFolder->readConfig( config );
    // make sure the children state is correct
    if ( mNewFolder->child() &&
         ( mNewFolder->storage()->hasChildren() == FolderStorage::HasNoChildren ) )
      mNewFolder->storage()->updateChildrenState();

    // delete the old folder
    mStorage->blockSignals( false );
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
      kWarning(5006) << k_funcinfo << "cannot remove a search folder" << endl;
    } else {
      kmkernel->folderMgr()->remove( mStorage->folder() );
    }

    emit renameDone( mNewName, true );
  } else
  {
    // move failed - rollback the last transaction
    kmkernel->undoStack()->undo();
    // FIXME Show a message box telling the user that he might want to delete the
    // partially copied folders himself.
    emit renameDone( mNewName, false );
  }
  delete this;
}

void RenameJob::slotMoveSubFolders( QString newName, bool success )
{
  if ( !success ) {
      emit renameDone( newName, false );
  } else {
    KMFolderDir* child = mStorage->folder()->child();
    if ( child && child->first() )
    {
      KMFolderNode* node = child->first();
      {
        FolderStorage* childStorage = static_cast<KMFolder*>(node)->storage();
        if ( !mNewFolder->child() )
          mNewFolder->createChildFolder();
        RenameJob* job = new RenameJob( childStorage, childStorage->objectName(),
                                        mNewFolder->child() );
        connect( job, SIGNAL( renameDone( QString, bool ) ),
            this, SLOT( slotMoveSubFolders( QString, bool ) ) );
        job->start();
      }
    }
    else slotMoveMessages();
  }
}

void RenameJob::moveSubFoldersBeforeMessages()
{
  // move child folders recursive if present - else simply move the messages
  KMFolderDir* child = mStorage->folder()->child();
  if ( child )
    slotMoveSubFolders( "", true );
  else slotMoveMessages();
}

#include "renamejob.moc"
