/**
 * Copyright (c) 2004 David Faure <faure@kde.org>
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

#include "expirejob.h"
#include "kmfolder.h"
#include "globalsettings.h"
#include "folderstorage.h"
#include "kmbroadcaststatus.h"
#include "kmcommands.h"

#include <kdebug.h>
#include <klocale.h>

using namespace KMail;

// Look at this number of messages in each slotDoWork call
#define EXPIREJOB_NRMESSAGES 100
// And wait this number of milliseconds before calling it again
#define EXPIREJOB_TIMERINTERVAL 100

/*
 Testcases for folder expiry:
  Automatic expiry:
  - normal case (ensure folder has old mails and expiry settings, wait for auto-expiry)
  - having the folder selected when the expiry job would run (gets delayed)
  - selecting a folder while an expiry job is running for it (should interrupt)
  - exiting kmail while an expiry job is running (should abort & delete things cleanly)
  Manual expiry:
  - RMB / expire (for one folder)                   [KMMainWidget::slotExpireFolder()]
  - RMB on Local Folders / Expire All Folders       [KMFolderMgr::expireAll()]
  - Expire All Folders                              [KMMainWidget::slotExpireAll()]
*/


ExpireJob::ExpireJob( KMFolder* folder, bool immediate )
 : FolderJob( 0, tOther, folder ), mTimer( this ), mCurrentIndex( 0 ),
   mImmediate( immediate ), mFolderOpen( false )
{
  mCancellable = true;
}

ExpireJob::~ExpireJob()
{
}

void ExpireJob::kill()
{
  Q_ASSERT( mCancellable );
  // We must close the folder if we opened it and got interrupted
  if ( mFolderOpen && mDestFolder && mDestFolder->storage() )
    mDestFolder->storage()->close();
  FolderJob::kill();
}

void ExpireJob::execute()
{
  mMaxUnreadTime = 0;
  mMaxReadTime = 0;
  mCurrentIndex = 0;

  int unreadDays, readDays;
  mDestFolder->daysToExpire( unreadDays, readDays );
  if (unreadDays > 0) {
    kdDebug(5006) << "ExpireJob: deleting unread older than "<< unreadDays << " days" << endl;
    mMaxUnreadTime = time(0) - unreadDays * 3600 * 24;
  }
  if (readDays > 0) {
    kdDebug(5006) << "ExpireJob: deleting read older than "<< readDays << " days" << endl;
    mMaxReadTime = time(0) - readDays * 3600 * 24;
  }

  if ((mMaxUnreadTime == 0) && (mMaxReadTime == 0)) {
    kdDebug(5006) << "ExpireJob: nothing to do" << endl;
    delete this;
    return;
  }

  FolderStorage* storage = mDestFolder->storage();
  storage->open();
  mFolderOpen = true;
  mCurrentIndex = storage->count()-1;
  kdDebug(5006) << "ExpireJob: starting to expire in folder " << mDestFolder->location() << endl;
  connect( &mTimer, SIGNAL( timeout() ), SLOT( slotDoWork() ) );
  mTimer.start( EXPIREJOB_TIMERINTERVAL );
  slotDoWork();
  // do nothing here, we might be deleted!
}

void ExpireJob::slotDoWork()
{
  // No need to worry about mDestFolder==0 here. The FolderStorage deletes the jobs on destruction.
  FolderStorage* storage = mDestFolder->storage();
  int stopIndex = mImmediate ? 0 : QMAX( 0, mCurrentIndex - EXPIREJOB_NRMESSAGES );
#ifdef DEBUG_SCHEDULER
  kdDebug(5006) << "ExpireJob: checking messages " << mCurrentIndex << " to " << stopIndex << endl;
#endif
  for( ; mCurrentIndex >= stopIndex; mCurrentIndex-- ) {
    const KMMsgBase *mb = storage->getMsgBase( mCurrentIndex );
    if (mb == 0)
      continue;
    if ( mb->isImportant()
      && GlobalSettings::excludeImportantMailFromExpiry() )
       continue;

    time_t maxTime = mb->isUnread() ? mMaxUnreadTime : mMaxReadTime;

    if (mb->date() < maxTime) {
      mRemovedMsgs.append( storage->getMsgBase( mCurrentIndex ) );
    }
  }
  if ( stopIndex == 0 )
    done();
}

void ExpireJob::done()
{
  QString str;
  FolderStorage* storage = mDestFolder->storage();

  if ( !mRemovedMsgs.isEmpty() ) {
    int count = mRemovedMsgs.count();
    // The command shouldn't kill us because it opens the folder
    mCancellable = false;
    if ( mDestFolder->expireAction() == KMFolder::ExpireDelete ) {
      // Expire by deletion, i.e. move to null target folder
      kdDebug(5006) << "ExpireJob: finished expiring in folder "
                    << mDestFolder->location()
                    << " " << count << " messages to remove." << endl;
      KMMoveCommand* cmd = new KMMoveCommand( 0, mRemovedMsgs );
      cmd->start();
      str = i18n( "Removing 1 old message from folder %1...",
                  "Removing %n old messages from folder %1...", count )
            .arg( mDestFolder->label() );
    } else {
      // Expire by moving
      KMFolder *moveToFolder =
        kmkernel->findFolderById( mDestFolder->expireToFolderId() );
      if ( !moveToFolder ) {
        str = i18n( "Cannot expire messages from folder %1: destination "
                    "folder %2 not found" )
              .arg( mDestFolder->label(), mDestFolder->expireToFolderId() );
        kdWarning(5006) << str << endl;
      } else {
        kdDebug(5006) << "ExpireJob: finished expiring in folder "
                      << mDestFolder->location() << " "
                      << mRemovedMsgs.count() << " messages to move to "
                      << moveToFolder->label() << endl;
        KMMoveCommand* cmd = new KMMoveCommand( moveToFolder, mRemovedMsgs );
        cmd->start();
        str = i18n( "Moving 1 old message from folder %1 to folder %2...",
                    "Moving %n old messages from folder %1 to folder %2...",
                    count )
              .arg( mDestFolder->label(), moveToFolder->label() );
      }
    }
  }
  if ( !str.isEmpty() )
    KMBroadcastStatus::instance()->setStatusMsg( str );

  storage->close();
  mFolderOpen = false;
  delete this;
}

#include "expirejob.moc"
