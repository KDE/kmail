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

#include "expirejob.h"
#include "kmfolder.h"
#include "globalsettings.h"
#include "folderstorage.h"
#include "broadcaststatus.h"
using KPIM::BroadcastStatus;
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
 : ScheduledJob( folder, immediate ), mTimer( this ), mCurrentIndex( 0 ),
   mFolderOpen( false ), mMoveToFolder( 0 )
{
}

ExpireJob::~ExpireJob()
{
}

void ExpireJob::kill()
{
  Q_ASSERT( mCancellable );
  // We must close the folder if we opened it and got interrupted
  if ( mFolderOpen && mSrcFolder && mSrcFolder->storage() )
    mSrcFolder->storage()->close();
  FolderJob::kill();
}

void ExpireJob::execute()
{
  mMaxUnreadTime = 0;
  mMaxReadTime = 0;
  mCurrentIndex = 0;

  int unreadDays, readDays;
  mSrcFolder->daysToExpire( unreadDays, readDays );
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

  FolderStorage* storage = mSrcFolder->storage();
  mOpeningFolder = true; // Ignore open-notifications while opening the folder
  storage->open();
  mOpeningFolder = false;
  mFolderOpen = true;
  mCurrentIndex = storage->count()-1;
  kdDebug(5006) << "ExpireJob: starting to expire in folder " << mSrcFolder->location() << endl;
  connect( &mTimer, SIGNAL( timeout() ), SLOT( slotDoWork() ) );
  mTimer.start( EXPIREJOB_TIMERINTERVAL );
  slotDoWork();
  // do nothing here, we might be deleted!
}

void ExpireJob::slotDoWork()
{
  // No need to worry about mSrcFolder==0 here. The FolderStorage deletes the jobs on destruction.
  FolderStorage* storage = mSrcFolder->storage();
  int stopIndex = mImmediate ? 0 : QMAX( 0, mCurrentIndex - EXPIREJOB_NRMESSAGES );
#ifdef DEBUG_SCHEDULER
  kdDebug(5006) << "ExpireJob: checking messages " << mCurrentIndex << " to " << stopIndex << endl;
#endif
  for( ; mCurrentIndex >= stopIndex; mCurrentIndex-- ) {
    const KMMsgBase *mb = storage->getMsgBase( mCurrentIndex );
    if (mb == 0)
      continue;
    if ( ( mb->isImportant() || mb->isTodo() || mb->isWatched() )
      && GlobalSettings::self()->excludeImportantMailFromExpiry() )
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
  mTimer.stop();

  QString str;
  bool moving = false;

  if ( !mRemovedMsgs.isEmpty() ) {
    int count = mRemovedMsgs.count();
    // The command shouldn't kill us because it opens the folder
    mCancellable = false;
    if ( mSrcFolder->expireAction() == KMFolder::ExpireDelete ) {
      // Expire by deletion, i.e. move to null target folder
      kdDebug(5006) << "ExpireJob: finished expiring in folder "
                    << mSrcFolder->location()
                    << " " << count << " messages to remove." << endl;
      KMMoveCommand* cmd = new KMMoveCommand( 0, mRemovedMsgs );
      connect( cmd, SIGNAL( completed( KMCommand * ) ),
               this, SLOT( slotMessagesMoved( KMCommand * ) ) );
      cmd->start();
      moving = true;
      str = i18n( "Removing 1 old message from folder %1...",
                  "Removing %n old messages from folder %1...", count )
            .arg( mSrcFolder->label() );
    } else {
      // Expire by moving
      mMoveToFolder =
        kmkernel->findFolderById( mSrcFolder->expireToFolderId() );
      if ( !mMoveToFolder ) {
        str = i18n( "Cannot expire messages from folder %1: destination "
                    "folder %2 not found" )
              .arg( mSrcFolder->label(), mSrcFolder->expireToFolderId() );
        kdWarning(5006) << str << endl;
      } else {
        kdDebug(5006) << "ExpireJob: finished expiring in folder "
                      << mSrcFolder->location() << " "
                      << mRemovedMsgs.count() << " messages to move to "
                      << mMoveToFolder->label() << endl;
        KMMoveCommand* cmd = new KMMoveCommand( mMoveToFolder, mRemovedMsgs );
        connect( cmd, SIGNAL( completed( KMCommand * ) ),
                 this, SLOT( slotMessagesMoved( KMCommand * ) ) );
        cmd->start();
        moving = true;
        str = i18n( "Moving 1 old message from folder %1 to folder %2...",
                    "Moving %n old messages from folder %1 to folder %2...",
                    count )
              .arg( mSrcFolder->label(), mMoveToFolder->label() );
      }
    }
  }
  if ( !str.isEmpty() )
    BroadcastStatus::instance()->setStatusMsg( str );

  KConfigGroup group( KMKernel::config(), "Folder-" + mSrcFolder->idString() );
  group.writeEntry( "Current", -1 ); // i.e. make it invalid, the serial number will be used

  if ( !moving ) {
    mSrcFolder->storage()->close();
    mFolderOpen = false;
    delete this;
  }
}

void ExpireJob::slotMessagesMoved( KMCommand *command )
{
  mSrcFolder->storage()->close();
  mFolderOpen = false;
  QString msg;
  switch ( command->result() ) {
  case KMCommand::OK:
    if ( mSrcFolder->expireAction() == KMFolder::ExpireDelete ) {
      msg = i18n( "Removed 1 old message from folder %1.",
                  "Removed %n old messages from folder %1.",
                  mRemovedMsgs.count() )
            .arg( mSrcFolder->label() );
    }
    else {
      msg = i18n( "Moved 1 old message from folder %1 to folder %2.",
                  "Moved %n old messages from folder %1 to folder %2.",
                  mRemovedMsgs.count() )
            .arg( mSrcFolder->label(), mMoveToFolder->label() );
    }
    break;
  case KMCommand::Failed:
    if ( mSrcFolder->expireAction() == KMFolder::ExpireDelete ) {
      msg = i18n( "Removing old messages from folder %1 failed." )
            .arg( mSrcFolder->label() );
    }
    else {
      msg = i18n( "Moving old messages from folder %1 to folder %2 failed." )
            .arg( mSrcFolder->label(), mMoveToFolder->label() );
    }
    break;
  case KMCommand::Canceled:
    if ( mSrcFolder->expireAction() == KMFolder::ExpireDelete ) {
      msg = i18n( "Removing old messages from folder %1 was canceled." )
            .arg( mSrcFolder->label() );
    }
    else {
      msg = i18n( "Moving old messages from folder %1 to folder %2 was "
                  "canceled." )
            .arg( mSrcFolder->label(), mMoveToFolder->label() );
    }
  default: ;
  }
  BroadcastStatus::instance()->setStatusMsg( msg );

  deleteLater();
}

#include "expirejob.moc"
