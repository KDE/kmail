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
#include "globalsettings.h"
#include "broadcaststatus.h"
using KPIM::BroadcastStatus;
#include "kmcommands.h"
#include "kmkernel.h"

#include <kdebug.h>
#include <klocale.h>
#include <kconfiggroup.h>
#include "foldercollection.h"
using namespace KMail;

#include <messagecore/messagestatus.h>

#include <Akonadi/ItemFetchJob>
#include <akonadi/kmime/messageparts.h>

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


ExpireJob::ExpireJob( const Akonadi::Collection& folder, bool immediate )
 : ScheduledJob( folder, immediate ), mCurrentIndex( 0 ),
   mFolderOpen( false ), mMoveToFolder( 0 )
{
}

ExpireJob::~ExpireJob()
{
  kDebug();
}

void ExpireJob::kill()
{
  ScheduledJob::kill();
}

void ExpireJob::execute()
{
  mMaxUnreadTime = 0;
  mMaxReadTime = 0;
  mCurrentIndex = 0;
  QSharedPointer<FolderCollection> fd( FolderCollection::forCollection( mSrcFolder ) );
  int unreadDays, readDays;
  fd->daysToExpire( unreadDays, readDays );
  if (unreadDays > 0) {
    kDebug() << "ExpireJob: deleting unread older than"<< unreadDays << "days";
    mMaxUnreadTime = time(0) - unreadDays * 3600 * 24;
  }
  if (readDays > 0) {
    kDebug() << "ExpireJob: deleting read older than"<< readDays << "days";
    mMaxReadTime = time(0) - readDays * 3600 * 24;
  }

  if ((mMaxUnreadTime == 0) && (mMaxReadTime == 0)) {
    kDebug() << "ExpireJob: nothing to do";
    deleteLater();
    return;
  }
  kDebug() << "ExpireJob: starting to expire in folder" << mSrcFolder.name();
  slotDoWork();
  // do nothing here, we might be deleted!
}

void ExpireJob::slotDoWork()
{
#ifdef DEBUG_SCHEDULER
  kDebug() << "ExpireJob: checking messages" << mCurrentIndex << "to" << stopIndex;
#endif
  Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( mSrcFolder, this );
  job->fetchScope().fetchPayloadPart( Akonadi::MessagePart::Header );
  connect( job, SIGNAL(result(KJob*)), SLOT(itemFetchResult(KJob*)) );
}

void ExpireJob::itemFetchResult( KJob *job )
{
  if ( job->error() ) {
    kWarning() << job->errorString();
    deleteLater();
    return;
  }
  foreach ( const Akonadi::Item &item, qobject_cast<Akonadi::ItemFetchJob*>( job )->items() ) {
    if ( !item.hasPayload<KMime::Message::Ptr>() )
      continue;
    const KMime::Message::Ptr mb = item.payload<KMime::Message::Ptr>();
    KPIM::MessageStatus status;
    status.setStatusFromFlags( item.flags() );
    if ( ( status.isImportant() || status.isToAct() || status.isWatched() )
      && GlobalSettings::self()->excludeImportantMailFromExpiry() )
       continue;

    time_t maxTime = status.isUnread() ? mMaxUnreadTime : mMaxReadTime;

    if ( !mb->date( false ) )
      continue;

    if ( mb->date()->dateTime().dateTime().toTime_t() < maxTime ) {
      mRemovedMsgs.append( item );
    }
  }

  done();
}

void ExpireJob::done()
{
  QString str;
  bool moving = false;
  QSharedPointer<FolderCollection> fd( FolderCollection::forCollection( mSrcFolder ) );

  if ( !mRemovedMsgs.isEmpty() ) {
    int count = mRemovedMsgs.count();
    // The command shouldn't kill us because it opens the folder
    mCancellable = false;
    if ( fd->expireAction() ==  FolderCollection::ExpireDelete ) {
      // Expire by deletion, i.e. move to null target folder
      kDebug() << "ExpireJob: finished expiring in folder"
                    << mSrcFolder.name()
                    << count << "messages to remove.";
      KMMoveCommand* cmd = new KMMoveCommand( kmkernel->trashCollectionFolder(), mRemovedMsgs, MessageList::Core::MessageItemSetReference() );
      connect( cmd, SIGNAL( completed( KMCommand * ) ),
               this, SLOT( slotMessagesMoved( KMCommand * ) ) );
      cmd->start();
      moving = true;
      str = i18np( "Removing 1 old message from folder %2...",
                  "Removing %1 old messages from folder %2...", count,
              mSrcFolder.name() );
    } else {
      // Expire by moving
      mMoveToFolder =
        KMKernel::self()->collectionFromId( fd->expireToFolderId() );
      if ( !mMoveToFolder.isValid() ) {
        str = i18n( "Cannot expire messages from folder %1: destination "
                    "folder %2 not found",
                    mSrcFolder.name(), fd->expireToFolderId() );
        kWarning() << str;
      } else {
        kDebug() << "ExpireJob: finished expiring in folder"
                      << mSrcFolder.name()
                      << mRemovedMsgs.count() << "messages to move to"
                      << mMoveToFolder.name();
        KMMoveCommand* cmd = new KMMoveCommand( mMoveToFolder, mRemovedMsgs, MessageList::Core::MessageItemSetReference() );
        connect( cmd, SIGNAL( completed( KMCommand * ) ),
                 this, SLOT( slotMessagesMoved( KMCommand * ) ) );
        cmd->start();
        moving = true;
        str = i18np( "Moving 1 old message from folder %2 to folder %3...",
                     "Moving %1 old messages from folder %2 to folder %3...",
                     count, mSrcFolder.name(), mMoveToFolder.name() );
      }
    }
  }
  if ( !str.isEmpty() )
    BroadcastStatus::instance()->setStatusMsg( str );

  KConfigGroup group( KMKernel::config(), fd->configGroupName() );
  group.writeEntry( "Current", -1 ); // i.e. make it invalid, the serial number will be used

  if ( !moving )
    deleteLater();
}

void ExpireJob::slotMessagesMoved( KMCommand *command )
{
  kDebug() << command << command->result();
  QString msg;
  QSharedPointer<FolderCollection> fd( FolderCollection::forCollection( mSrcFolder ) );
  switch ( command->result() ) {
  case KMCommand::OK:
    if ( fd->expireAction() == FolderCollection::ExpireDelete ) {
      msg = i18np( "Removed 1 old message from folder %2.",
                  "Removed %1 old messages from folder %2.",
                  mRemovedMsgs.count(),
              mSrcFolder.name() );
    }
    else {
      msg = i18np( "Moved 1 old message from folder %2 to folder %3.",
                   "Moved %1 old messages from folder %2 to folder %3.",
                   mRemovedMsgs.count(), mSrcFolder.name(), mMoveToFolder.name() );
    }
    break;
  case KMCommand::Failed:
    if ( fd->expireAction() == FolderCollection::ExpireDelete ) {
      msg = i18n( "Removing old messages from folder %1 failed.",
              mSrcFolder.name() );
    }
    else {
      msg = i18n( "Moving old messages from folder %1 to folder %2 failed.",
                  mSrcFolder.name(), mMoveToFolder.name() );
    }
    break;
  case KMCommand::Canceled:
    if ( fd->expireAction() == FolderCollection::ExpireDelete ) {
      msg = i18n( "Removing old messages from folder %1 was canceled.",
              mSrcFolder.name() );
    }
    else {
      msg = i18n( "Moving old messages from folder %1 to folder %2 was "
                  "canceled.",
                  mSrcFolder.name(), mMoveToFolder.name() );
    }
  default: ;
  }
  BroadcastStatus::instance()->setStatusMsg( msg );

  deleteLater();
}

#include "expirejob.moc"
