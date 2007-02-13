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
#include "copyfolderjob.h"
#include "kmfolderimap.h"
#include "kmfoldercachedimap.h"
#include "folderstorage.h"
#include "kmfolder.h"
#include "kmfolderdir.h"
#include "kmfoldermgr.h"
#include "imapaccountbase.h"
#include "kmacctimap.h"
#include "kmacctcachedimap.h"
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
   mNewName( newName ), mNewFolder( 0 ), mCopyFolderJob( 0 )
{
  mStorageTempOpened = 0;
  if ( storage ) {
    mOldName = storage->name();
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

// FIXME: move on the server for online imap given source and target are on the same server
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
    // copy to the new folder
    mCopyFolderJob = new CopyFolderJob( mStorage, mNewParent );
    connect( mCopyFolderJob, SIGNAL(folderCopyComplete(bool)), SLOT(folderCopyComplete(bool)) );
    mCopyFolderJob->execute();

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
    } else if ( mOldName == mNewName ) {
      emit renameDone( mNewName, true ); // noop
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

void RenameJob::folderCopyComplete(bool success)
{
  kdDebug(5006) << k_funcinfo << success << endl;
  if ( !success ) {
    kdWarning(5006) << k_funcinfo << "could not copy folder" << endl;
    emit renameDone( mNewName, false );
    deleteLater();
    return;
  }
  mNewFolder = mCopyFolderJob->targetFolder();
  mCopyFolderJob = 0;

  if ( mStorageTempOpened ) {
    mStorageTempOpened->close();
    mStorageTempOpened = 0;
  }

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
    kdWarning(5006) << k_funcinfo << "cannot remove a search folder" << endl;
  } else {
    kmkernel->folderMgr()->remove( mStorage->folder() );
  }

  emit renameDone( mNewName, true );
}

#include "renamejob.moc"
