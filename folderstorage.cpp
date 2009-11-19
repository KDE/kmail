/*
    Virtual base class for mail storage.

    This file is part of KMail.

    Copyright (c) 2004 Bo Thorsen <bo@sonofthor.dk>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/


#include "folderstorage.h"
#include "kmfolder.h"
#include "kmkernel.h"

#include "undostack.h"
#include "kmmsgdict.h"
#include "kmcommands.h"
#include "kmsearchpattern.h"
#include "globalsettings.h"

#include <kde_file.h>
#include <klocale.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kconfiggroup.h>

#include <kmime/kmime_message.h>

#include <QFile>
#include <QList>
#include <QRegExp>

#include <unistd.h>
#include <errno.h>

//-----------------------------------------------------------------------------

FolderStorage::FolderStorage( KMFolder *folder, const char *aName )
  : QObject( folder ), mFolder( folder ), mEmitChangedTimer( 0L )
{
  setObjectName( aName );
  mOpenCount = 0;
  mQuiet = 0;
  mChanged = false;
  mAutoCreateIndex = true;
  mExportsSernums = false;
  mDirty = false;
  mUnreadMsgs = -1;
  mGuessedUnreadMsgs = -1;
  mTotalMsgs = -1;
  mCachedSize = -1;
  needsCompact = false;
  mConvertToUtf8 = false;
  mCompactable = true;
  mNoContent = false;
  mNoChildren = false;
  mRDict = 0;
  mDirtyTimer = new QTimer( this );
  connect( mDirtyTimer, SIGNAL( timeout() ), this, SLOT( updateIndex() ) );

  mHasChildren = HasNoChildren;
  mContentsType = KMail::ContentsTypeMail;

  connect( this, SIGNAL( closed(KMFolder*) ), mFolder, SIGNAL( closed() ) );
}

//-----------------------------------------------------------------------------
FolderStorage::~FolderStorage()
{
  QObject::disconnect( SIGNAL(destroyed(QObject*)), this, 0 );
  qDeleteAll( mJobList );
  mJobList.clear();
  KMMsgDict::deleteRentry(mRDict);
}


void FolderStorage::close( const char*, bool aForced )
{
  if (mOpenCount <= 0) return;
  if (mOpenCount > 0) mOpenCount--;
  if (mOpenCount > 0 && !aForced) return;
  reallyDoClose();
}

//-----------------------------------------------------------------------------
QString FolderStorage::dotEscape(const QString& aStr)
{
  if (aStr.isEmpty() || aStr[0] != '.') return aStr;
  return aStr.left(aStr.indexOf(QRegExp("[^\\.]"))) + aStr;
}

void FolderStorage::addJob( FolderJob* job ) const
{
  connect( job, SIGNAL(destroyed(QObject*)),
           SLOT(removeJob(QObject*)) );
  mJobList.append( job );
}

void FolderStorage::removeJob( QObject* job )
{
  mJobList.removeAll( static_cast<FolderJob*>( job ) );
}


//-----------------------------------------------------------------------------
QString FolderStorage::location() const
{
#if 0
  QString sLocation(const_cast<FolderStorage*>(this)->folder()->path());

  if (!sLocation.isEmpty()) sLocation += '/';
  sLocation += dotEscape(fileName());

  return sLocation;
#endif
return QString();
}

QString FolderStorage::fileName() const
{
  //return mFolder->name();
return QString();
}



//-----------------------------------------------------------------------------
void FolderStorage::setAutoCreateIndex(bool autoIndex)
{
  mAutoCreateIndex = autoIndex;
}

//-----------------------------------------------------------------------------
void FolderStorage::setDirty(bool f)
{
  mDirty = f;
  if (mDirty  && mAutoCreateIndex)
    mDirtyTimer->start( mDirtyTimerInterval );
  else
    mDirtyTimer->stop();
}

//-----------------------------------------------------------------------------
void FolderStorage::markNewAsUnread()
{
#if 0 //TODO port to akonadi
  KMime::Content* msgBase;
  int i;

  for (i=0; i< count(); ++i)
  {
    if (!(msgBase = getMsgBase(i))) continue;
    if (msgBase->status().isNew())
    {
      msgBase->setStatus( MessageStatus::statusUnread() );
      msgBase->setDirty(true);
    }
  }
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

void FolderStorage::markUnreadAsRead()
{
#if 0 //TODO port to akonadi
  KMime::Content* msgBase;
  SerNumList serNums;

  for (int i=count()-1; i>=0; --i)
  {
    msgBase = getMsgBase(i);
    assert(msgBase);
    if (msgBase->status().isNew() || msgBase->status().isUnread())
    {
      serNums.append( msgBase->getMsgSerNum() );
    }
  }
  if (serNums.empty())
    return;
#ifdef OLD_COMMAND
  KMCommand *command = new KMSetStatusCommand( MessageStatus::statusRead(), serNums );
  command->start();
#endif
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

//-----------------------------------------------------------------------------
void FolderStorage::quiet(bool beQuiet)
{

  if (beQuiet)
  {
    /* Allocate the timer here to don't have one timer for each folder. BTW,
     * a timer is created when a folder is checked
     */
    if ( !mEmitChangedTimer) {
      mEmitChangedTimer= new QTimer( this );
      connect( mEmitChangedTimer, SIGNAL( timeout() ),
      this, SLOT( slotEmitChangedTimer() ) );
    }
    mQuiet++;
  } else {
    mQuiet--;
    if (mQuiet <= 0)
    {
      delete mEmitChangedTimer;
      mEmitChangedTimer=0L;

      mQuiet = 0;
      if (mChanged) {
       emit changed();
       // Don't hurt emit this if the mUnreadMsg really don't change
       // We emit it here, because this signal is delayed if mQuiet >0
       emit numUnreadMsgsChanged( folder() );
      }
      mChanged = false;
    }
  }
}

//-----------------------------------------------------------------------------

/** Compare message's date. This is useful for message sorting */
int operator<( KMime::Message & m1, KMime::Message & m2 )
{
  return (m1.date()->dateTime() < m2.date()->dateTime());
}

/** Compare message's date. This is useful for message sorting */
int operator==( KMime::Message & m1, KMime::Message & m2 )
{
  return (m1.date()->dateTime() == m2.date()->dateTime());
}


//-----------------------------------------------------------------------------
int FolderStorage::expungeOldMsg(int days)
{
  int msgnb=0;
  time_t msgTime, maxTime;
  KMime::Message* mb;
  QList<int> rmvMsgList;

  maxTime = time(0) - days * 3600 * 24;

  for (int i=count()-1; i>=0; i--) {
    mb = getMsgBase(i);
    assert(mb);
    msgTime = mb->date()->dateTime().dateTime().toTime_t();

    if (msgTime < maxTime) {
      //kDebug() << "deleting msg" << i << ":" << mb->subject() << "-" << mb->dateStr(); //;
      removeMsg( i );
      msgnb++;
    }
  }
  return msgnb;
}

//------------------------------------------
void FolderStorage::slotEmitChangedTimer()
{
  emit changed();
  mChanged=false;
}
//-----------------------------------------------------------------------------
void FolderStorage::emitMsgAddedSignals(int idx)
{
  quint32 serNum = KMMsgDict::instance()->getMsgSerNum( folder(), idx );
  if (!mQuiet) {
    emit msgAdded(idx);
  } else {
    /** Restart always the timer or not. BTW we get a kmheaders refresh
     * each 3 seg.?*/
    if ( !mEmitChangedTimer->isActive() ) {
      mEmitChangedTimer->start( 3000 );
    }
    mChanged=true;
  }
  emit msgAdded( folder(), serNum );
}

//-----------------------------------------------------------------------------
bool FolderStorage::canAddMsgNow(KMime::Message* aMsg, int* aIndex_ret)
{
#if 0 //TODO port to akonadi
  if (aIndex_ret) *aIndex_ret = -1;
  KMFolder *msgParent = aMsg->parent();
  // If the message has a parent and is in transfer, bail out. If it does not
  // have a parent we want to be able to add it even if it is in transfer.
  if (aMsg->transferInProgress() && msgParent)
      return false;
  if (!aMsg->isComplete() && msgParent && msgParent->folderType() == KMFolderTypeImap)
  {
    FolderJob *job = msgParent->createJob(aMsg);
    connect(job, SIGNAL(messageRetrieved(KMime::Message*)),
            SLOT(reallyAddMsg(KMime::Message*)));
    job->start();
    aMsg->setTransferInProgress( true );
    return false;
  }
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  return true;
}


//-----------------------------------------------------------------------------
void FolderStorage::reallyAddMsg(KMime::Message* aMsg)
{
  if (!aMsg) // the signal that is connected can call with aMsg=0
    return;
#if 0 //TODO port to akonadi
  aMsg->setTransferInProgress( false );
  aMsg->setComplete( true );
  KMFolder *aFolder = aMsg->parent();
  int index;
  ulong serNum = aMsg->getMsgSerNum();
  bool undo = aMsg->enableUndo();
  addMsg(aMsg, &index);
  if (index < 0) return;
  unGetMsg(index);
  if (undo)
  {
    kmkernel->undoStack()->pushSingleAction( serNum, aFolder, folder() );
  }
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}


//-----------------------------------------------------------------------------
void FolderStorage::reallyAddCopyOfMsg(KMime::Message* aMsg)
{
  if ( !aMsg ) return; // messageRetrieved(0) is always possible
#if 0 //TODO port to akonadi
  aMsg->setParent( 0 );
  aMsg->setTransferInProgress( false );
  addMsg( aMsg );
  unGetMsg( count() - 1 );
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

int FolderStorage::find( const KMime::Message * msg ) const {
#if 0 //TODO port to akonadi
  return find( &msg->toMsgBase() );
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
    return -1;
#endif
}

//-----------------------------------------------------------------------------
void FolderStorage::removeMessages(const QList<KMime::Content*>& msgList, bool imapQuiet)
{
  for( QList<KMime::Content*>::const_iterator it = msgList.begin();
      it != msgList.end(); ++it )
  {
    int idx = find( *it );
    assert( idx != -1 );
    removeMsg( idx, imapQuiet );
  }
}

//-----------------------------------------------------------------------------
void FolderStorage::removeMessages(const QList<KMime::Message*>& msgList, bool imapQuiet)
{
  for( QList<KMime::Message*>::const_iterator it = msgList.begin();
      it != msgList.end(); ++it )
  {
    int idx = find( *it );
    assert( idx != -1 );
    removeMsg( idx, imapQuiet );
  }
}

//-----------------------------------------------------------------------------
void FolderStorage::removeMsg(int idx, bool)
{
#if 0 //TODO port to akonadi
  //assert(idx>=0);
  if(idx < 0)
  {
    kDebug() << "idx < 0";
    return;
  }

  KMime::Content* mb = getMsgBase(idx);

  quint32 serNum = KMMsgDict::instance()->getMsgSerNum( folder(), idx );
  if (serNum != 0)
    emit msgRemoved( folder(), serNum );
  mb = takeIndexEntry( idx );

  setDirty( true );
  needsCompact=true; // message is taken from here - needs to be compacted

  if (mb->status().isUnread() || mb->status().isNew() ||
      (folder() == kmkernel->outboxFolder())) {
    --mUnreadMsgs;
    if ( !mQuiet ) {
//      kDebug() << "FolderStorage::msgStatusChanged";
      emit numUnreadMsgsChanged( folder() );
    }else{
      if ( !mEmitChangedTimer->isActive() ) {
//        kDebug()<<"EmitChangedTimer started";
        mEmitChangedTimer->start( 3000 );
      }
      mChanged = true;
    }
  }
  --mTotalMsgs;

  mCachedSize = -1;
  QString msgIdMD5 = mb->msgIdMD5();
  emit msgRemoved( idx, msgIdMD5 );
  emit msgRemoved( folder() );
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}


//-----------------------------------------------------------------------------
KMime::Message* FolderStorage::take(int idx)
{
#if 0 //TODO port to akonadi
  KMime::Content* mb;
  KMime::Message* msg;

  assert(idx>=0 && idx<=count());

  mb = getMsgBase(idx);
  if (!mb) return 0;
  if (!mb->isMessage()) readMsg(idx);
  quint32 serNum = KMMsgDict::instance()->getMsgSerNum( folder(), idx );
  emit msgRemoved( folder(), serNum );

  msg = (KMime::Message*)takeIndexEntry(idx);

  if (msg->status().isUnread() || msg->status().isNew() ||
      ( folder() == kmkernel->outboxFolder() )) {
    --mUnreadMsgs;
    if ( !mQuiet ) {
      emit numUnreadMsgsChanged( folder() );
    }else{
      if ( !mEmitChangedTimer->isActive() ) {
        mEmitChangedTimer->start( 3000 );
      }
      mChanged = true;
    }
  }
  --mTotalMsgs;
  msg->setParent(0);
  setDirty( true );
  mCachedSize = -1;
  needsCompact=true; // message is taken from here - needs to be compacted
  QString msgIdMD5 = msg->msgIdMD5();
  emit msgRemoved( idx, msgIdMD5 );
  emit msgRemoved( folder() );

  return msg;
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
    return 0;
#endif
}

void FolderStorage::takeMessages(const QList<KMime::Message*>& msgList)
{
#if 0 //TODO port to akonadi
  for( QList<KMime::Message*>::const_iterator it = msgList.begin();
      it != msgList.end(); ++it )
  {
    KMime::Message *msg = (*it);
    if (msg->parent())
    {
      int idx = msg->parent()->find(msg);
      take(idx);
    }
  }
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}


//-----------------------------------------------------------------------------
KMime::Message* FolderStorage::getMsg(int idx)
{
#if 0 //TODO port to akonadi
  if ( idx < 0 || idx >= count() )
    return 0;

  KMime::Content* mb = getMsgBase(idx);
  if (!mb) return 0;

  KMime::Message *msg = 0;
  bool undo = mb->enableUndo();
  if (mb->isMessage()) {
      msg = ((KMime::Message*)mb);
  } else {
      QString mbSubject = mb->subject();
      msg = readMsg(idx);
      // sanity check
      if (mCompactable && (msg->subject().isEmpty() != mbSubject.isEmpty())) {
        kDebug() << "Error:" << location() <<
          "Index file is inconsistent with folder file. This should never happen.";
        mCompactable = false; // Don't compact
        writeConfig();
      }

  }
  // Either isMessage and we had a sernum, or readMsg gives us one
  // (via insertion into mMsgList). sernum == 0 may still occur due to
  // an outdated or corrupt IMAP cache.
  if ( msg->getMsgSerNum() == 0 ) {
    kWarning() << "msg serial number == 0";
    return 0;
  }
  msg->setEnableUndo(undo);
  msg->setComplete( true );
  return msg;
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

//-----------------------------------------------------------------------------
KMime::Message* FolderStorage::readTemporaryMsg(int idx)
{
  if(!(idx >= 0 && idx <= count()))
    return 0;
#if 0 //TODO port to akonadi
  KMime::Content* mb = getMsgBase(idx);
  if (!mb) return 0;

  unsigned long sernum = mb->getMsgSerNum();
  // sanity check: serial num can be broken for any reason (storage?), give up in this case
  if (sernum == 0) {
    kWarning() << "msg serial number == 0";
    return 0;
  }

  KMime::Message *msg = 0;
  bool undo = mb->enableUndo();
  if (mb->isMessage()) {
    // the caller will delete it, so we must make a copy it
    msg = new KMime::Message(*(KMime::Message*)mb);
    msg->setMsgSerNum(sernum);
    msg->setComplete( true );
  } else {
    // ## Those two lines need to be moved to a virtual method for KMFolderSearch, like readMsg
    msg = new KMime::Message(*(KMMsgInfo*)mb);
    msg->setMsgSerNum(sernum); // before fromDwString so that readyToShow uses the right sernum
    msg->setComplete( true );
    msg->fromDwString(getDwString(idx));
  }
  msg->setEnableUndo(undo);
  return msg;
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
    return 0;
#endif
}

#if 0
//-----------------------------------------------------------------------------
KMMsgInfo* FolderStorage::unGetMsg(int idx)
{
#if 0 //TODO port to akonadi
  KMime::Content* mb;

  if(!(idx >= 0 && idx <= count()))
    return 0;

  mb = getMsgBase(idx);
  if (!mb) return 0;


  if (mb->isMessage()) {
    // Remove this message from all jobs' list it might still be on.
    // setIndexEntry deletes the message.
    KMime::Message *msg = static_cast<KMime::Message*>(mb);
    if ( msg->transferInProgress() ) return 0;
    ignoreJobsForMessage( msg );
    return setIndexEntry( idx, msg );
  }
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  return 0;
}
#endif

//-----------------------------------------------------------------------------
bool FolderStorage::isMessage(int idx)
{
#if 0 //TODO port to akonadi
  KMime::Message* mb;
  if (!(idx >= 0 && idx <= count())) return false;
  mb = getMsgBase(idx);
  return (mb && mb->isMessage());
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

//-----------------------------------------------------------------------------
FolderJob* FolderStorage::createJob( KMime::Message *msg, FolderJob::JobType jt,
                                KMFolder *folder, const QString &partSpecifier,
                                const MessageViewer::AttachmentStrategy *as ) const
{
  FolderJob * job = doCreateJob( msg, jt, folder, partSpecifier, as );
  if ( job )
    addJob( job );
  return job;
}

//-----------------------------------------------------------------------------
FolderJob* FolderStorage::createJob( QList<KMime::Message*>& msgList, const QString& sets,
                                FolderJob::JobType jt, KMFolder *folder ) const
{
  FolderJob * job = doCreateJob( msgList, sets, jt, folder );
  if ( job )
    addJob( job );
  return job;
}

//-----------------------------------------------------------------------------
int FolderStorage::moveMsg( KMime::Message *aMsg, int *aIndex_ret )
{
  assert( aMsg != 0 );
#if 0 //TODO port to akonadi
   KMFolder* msgParent = aMsg->parent();

  if ( msgParent ) {
    msgParent->open( "moveMsgSrc" );
  }

  open( "moveMsgDest" );
  int rc = addMsg( aMsg, aIndex_ret );
  close( "moveMsgDest" );

  if ( msgParent ) {
    msgParent->close( "moveMsgSrc" );
  }

  return rc;
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
    return -1;
#endif
}

//-----------------------------------------------------------------------------
int FolderStorage::moveMsg( QList<KMime::Message*> msglist, int *aIndex_ret )
{
#if 0 //TODO port to akonadi
  KMime::Message* aMsg = msglist.first();
  assert( aMsg != 0 );
  KMFolder* msgParent = aMsg->parent();

  if ( msgParent ) {
    msgParent->open( "foldermovemsg" );
  }

  QList<int> index;
  open( "moveMsg" );
  int rc = addMessages( msglist, index );
  close( "moveMsg" );
  // FIXME: we want to have a QValueList to pass it back, so change this method
  if ( !index.isEmpty() ) {
    aIndex_ret = &index.first();
  }

  if ( msgParent ) {
    msgParent->close( "foldermovemsg" );
  }

  return rc;
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
    return -1;
#endif
}


//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void FolderStorage::remove()
{
  assert( !folder()->name().isEmpty() );

  // delete and remove from dict if necessary
  clearIndex( true, mExportsSernums );
  close( "remove", true );

  if ( mExportsSernums ) {
    KMMsgDict::mutableInstance()->removeFolderIds( *this );
    mExportsSernums = false;  // do not writeFolderIds after removal
  }
  unlink( QFile::encodeName( sortedLocation() ) );
  unlink( QFile::encodeName( indexLocation() ) );

  int rc = removeContents();

  needsCompact = false; //we are dead - no need to compact us

  // Erase settings, otherwise they might interfere when recreating the folder
  KSharedConfig::Ptr config = KMKernel::config();
  config->deleteGroup( folder()->configGroupName() );

  emit closed( folder() );
  emit removed( folder(), (rc ? false : true) );
}

//-----------------------------------------------------------------------------
int FolderStorage::expunge()
{
  assert( !folder()->name().isEmpty() );

  // delete and remove from dict, if needed
  clearIndex( true, mExportsSernums );
  close( "expunge", true );

  if ( mExportsSernums ) {
    KMMsgDict::mutableInstance()->removeFolderIds( *this );
  }
  if ( mAutoCreateIndex ) {
    truncateIndex();
  } else {
    unlink( QFile::encodeName( indexLocation() ) );
  }

  int rc = expungeContents();
  if ( rc ) {
    return rc;
  }

  mDirty = false;
  needsCompact = false; //we're cleared and truncated no need to compact

  mUnreadMsgs = 0;
  mTotalMsgs = 0;
  mCachedSize = 0;
  emit numUnreadMsgsChanged( folder() );
  if ( mAutoCreateIndex ) {  // FIXME Heh? - Till
    writeConfig();
  }
  emit changed();
  emit expunged( folder() );

  return 0;
}

//-----------------------------------------------------------------------------
QString FolderStorage::label() const
{
  return folder()->label();
}

int FolderStorage::count(bool cache) const
{
  if (cache && mTotalMsgs != -1)
    return mTotalMsgs;
  else
    return -1;
}

//-----------------------------------------------------------------------------
int FolderStorage::countUnread()
{
  if ( mGuessedUnreadMsgs > -1 ) {
    return mGuessedUnreadMsgs;
  }
  if ( mUnreadMsgs > -1 ) {
    return mUnreadMsgs;
  }

  readConfig();

  if ( mUnreadMsgs > -1 ) {
    return mUnreadMsgs;
  }

  open( "countunread" ); // will update unreadMsgs
  int unread = mUnreadMsgs;
  close( "countunread" );
  return (unread > 0) ? unread : 0;
}

qint64 FolderStorage::folderSize() const
{
  if ( mCachedSize == -1 ) {
    mCachedSize = doFolderSize();
  }
  return mCachedSize;
}


/*virtual*/
bool FolderStorage::isCloseToQuota() const
{
  return false;
}

//-----------------------------------------------------------------------------
void FolderStorage::msgStatusChanged( const MessageStatus& oldStatus,
                                      const MessageStatus& newStatus, int idx)
{
  int oldUnread = 0;
  int newUnread = 0;

  if ( ( oldStatus.isUnread() || oldStatus.isNew() ) ) {
    oldUnread = 1;
  }
  if ( ( newStatus.isUnread() || newStatus.isNew() ) ) {
    newUnread = 1;
  }
  int deltaUnread = newUnread - oldUnread;

  mDirtyTimer->start(mDirtyTimerInterval);
  if (deltaUnread != 0) {
    if (mUnreadMsgs < 0) mUnreadMsgs = 0;
    mUnreadMsgs += deltaUnread;
    if ( !mQuiet ) {
      emit numUnreadMsgsChanged( folder() );
    }else{
      if ( !mEmitChangedTimer->isActive() ) {
        mEmitChangedTimer->start( 3000 );
      }
      mChanged = true;
    }
    quint32 serNum = KMMsgDict::instance()->getMsgSerNum(folder(), idx);
    emit msgChanged( folder(), serNum, deltaUnread );
  }
}

//-----------------------------------------------------------------------------
void FolderStorage::headerOfMsgChanged(const KMime::Content* aMsg, int idx)
{
#if 0 //TODO port to akonadi
  if (idx < 0)
    idx = aMsg->parent()->find( aMsg );

  if (idx >= 0 )
  {
    if ( !mQuiet )
      emit msgHeaderChanged(folder(), idx);
    else{
      if ( !mEmitChangedTimer->isActive() ) {
        mEmitChangedTimer->start( 3000 );
      }
      mChanged = true;
    }
  } else
    mChanged = true;
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

//-----------------------------------------------------------------------------
void FolderStorage::readConfig()
{
  //kDebug()<<"#### READING CONFIG  ="<< name();
  KSharedConfig::Ptr config = KMKernel::config();
  KConfigGroup group(config, folder()->configGroupName());
  if (mUnreadMsgs == -1)
    mUnreadMsgs = group.readEntry("UnreadMsgs", -1 );
  if (mTotalMsgs == -1)
    mTotalMsgs = group.readEntry("TotalMsgs", -1 );
  mCompactable = group.readEntry("Compactable", true );
  if ( mCachedSize == -1 )
    mCachedSize = group.readEntry( "FolderSize", Q_INT64_C(-1) );

  int type = group.readEntry( "ContentsType", 0 );
  if ( type < 0 || type > KMail::ContentsTypeLast ) type = 0;
  setContentsType( static_cast<KMail::FolderContentsType>( type ) );

  if( folder() ) folder()->readConfig( group );
}

//-----------------------------------------------------------------------------
void FolderStorage::writeConfig()
{
  KSharedConfig::Ptr config = KMKernel::config();
  KConfigGroup group( config, folder()->configGroupName() );
  group.writeEntry( "UnreadMsgs",
                    mGuessedUnreadMsgs == -1 ? mUnreadMsgs : mGuessedUnreadMsgs);
  group.writeEntry( "TotalMsgs", mTotalMsgs );
  group.writeEntry( "Compactable", mCompactable );
  group.writeEntry( "ContentsType", (int)mContentsType );
  group.writeEntry( "FolderSize", mCachedSize );

  // Write the KMFolder parts
  if( folder() ) folder()->writeConfig( group );

  GlobalSettings::self()->requestSync();
}

//-----------------------------------------------------------------------------
void FolderStorage::enableCompaction()
{
  if ( mCompactable ) return;

  kDebug() << "#### Enabling compaction for folder" << folder()->idString();
  mCompactable = true;
  writeConfig();
}

//-----------------------------------------------------------------------------
void FolderStorage::correctUnreadMsgsCount()
{
  open( "countunreadmsg" );
  close( "countunreadmsg" );
  emit numUnreadMsgsChanged( folder() );
}

void FolderStorage::registerWithMessageDict()
{
  mExportsSernums = true;
  readFolderIdsFile();
}

void FolderStorage::deregisterFromMessageDict()
{
  writeFolderIdsFile();
  mExportsSernums = false;
}

void FolderStorage::readFolderIdsFile()
{
  if ( !mExportsSernums ) return;
  if ( KMMsgDict::mutableInstance()->readFolderIds( *this ) == -1 ) {
    invalidateFolder();
  }
  if ( !KMMsgDict::mutableInstance()->hasFolderIds( *this ) ) {
    invalidateFolder();
  }
}

void FolderStorage::invalidateFolder()
{
  if ( !mExportsSernums ) return;
  unlink(QFile::encodeName( sortedLocation()) );
  unlink(QFile::encodeName( idsLocation()) );
  fillMessageDict();
  KMMsgDict::mutableInstance()->writeFolderIds( *this );
  emit invalidated( folder() );
}


//-----------------------------------------------------------------------------
int FolderStorage::writeFolderIdsFile() const
{
  if ( !mExportsSernums ) return 0;
  return KMMsgDict::mutableInstance()->writeFolderIds( *this );
}

//-----------------------------------------------------------------------------
int FolderStorage::touchFolderIdsFile()
{
  if ( !mExportsSernums ) return 0;
  return KMMsgDict::mutableInstance()->touchFolderIds( *this );
}

//-----------------------------------------------------------------------------
int FolderStorage::appendToFolderIdsFile( int idx )
{
  if ( !mExportsSernums ) return 0;
  int ret = 0;
  if ( count() == 1 ) {
    ret = KMMsgDict::mutableInstance()->writeFolderIds( *this );
  } else {
    ret = KMMsgDict::mutableInstance()->appendToFolderIds( *this, idx );
  }
  return ret;
}

void FolderStorage::replaceMsgSerNum( unsigned long sernum, KMime::Content* msg, int idx )
{
  if ( !mExportsSernums ) return;
  KMMsgDict::mutableInstance()->replace( sernum, msg, idx );
}

void FolderStorage::setRDict( KMMsgDictREntry *rentry ) const
{
  if ( !mExportsSernums ) {
    kWarning() << "WTF, this FolderStorage should be invisible to the msgdict, who is calling us?"
               << kBacktrace();
  }
  assert( mExportsSernums ); // otherwise things are very wrong
  if ( rentry == mRDict )
    return;
  KMMsgDict::deleteRentry( mRDict );
  mRDict = rentry;
}

//-----------------------------------------------------------------------------
void FolderStorage::setStatus(int idx, const MessageStatus& status, bool toggle)
{
  KMime::Message *msg = getMsgBase(idx);
#if 0 //TODO port to akonadi
  if ( msg ) {
    if (toggle)
      msg->toggleStatus(status, idx);
    else
      msg->setStatus(status, idx);
  }
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}


//-----------------------------------------------------------------------------
void FolderStorage::setStatus(QList<int>& ids, const MessageStatus& status, bool toggle)
{
  for ( QList<int>::Iterator it = ids.begin(); it != ids.end(); ++it )
  {
    FolderStorage::setStatus(*it, status, toggle);
  }
}

void FolderStorage::ignoreJobsForMessage( KMime::Message *msg )
{
#if 0 //TODO port to akonadi
  if ( !msg || msg->transferInProgress() )
    return;

  QList<FolderJob*>::iterator it;
  for ( it = mJobList.begin(); it != mJobList.end(); )
  {
    //FIXME: the questions is : should we iterate through all
    //messages in jobs? I don't think so, because it would
    //mean canceling the jobs that work with other messages
    if ( !(*it)->msgList().isEmpty() && (*it)->msgList().first() == msg )
    {
      FolderJob* job = (*it);
      it = mJobList.erase( it );
      delete job;
    }
    else
      ++it;
  }
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

//-----------------------------------------------------------------------------
void FolderStorage::removeJobs()
{
  qDeleteAll( mJobList );
  mJobList.clear();
}



//-----------------------------------------------------------------------------
void FolderStorage::updateChildrenState()
{
}

//-----------------------------------------------------------------------------
void FolderStorage::setNoChildren( bool aNoChildren )
{
  mNoChildren = aNoChildren;
  if ( aNoChildren )
    setHasChildren( HasNoChildren );
}

//-----------------------------------------------------------------------------
void FolderStorage::setContentsType( KMail::FolderContentsType type, bool quiet )
{
  if ( type != mContentsType ) {
    mContentsType = type;
    if ( !quiet )
       emit contentsTypeChanged( type );
  }
}

//-----------------------------------------------------------------------------
void FolderStorage::search( const KMSearchPattern* pattern )
{
  mSearchPattern = pattern;
  mCurrentSearchedMsg = 0;
  if ( pattern )
    slotProcessNextSearchBatch();
}

void FolderStorage::slotProcessNextSearchBatch()
{
  if ( !mSearchPattern )
    return;
  QList<quint32> matchingSerNums;
  const int end = qMin( mCurrentSearchedMsg + 10, count() );
  for ( int i = mCurrentSearchedMsg; i < end; ++i )
  {
    quint32 serNum = KMMsgDict::instance()->getMsgSerNum( folder(), i );
    if ( mSearchPattern->matches( serNum ) )
      matchingSerNums.append( serNum );
  }
  mCurrentSearchedMsg = end;
  bool complete = ( end >= count() );
  emit searchResult( folder(), matchingSerNums, mSearchPattern, complete );
  if ( !complete )
    QTimer::singleShot( 0, this, SLOT(slotProcessNextSearchBatch()) );
}

//-----------------------------------------------------------------------------
void FolderStorage::search( const KMSearchPattern* pattern, quint32 serNum )
{
  bool matches = pattern && pattern->matches( serNum );

  emit searchDone( folder(), serNum, pattern, matches );
}

//-----------------------------------------------------------------------------
int FolderStorage::addMessages( QList<KMime::Message*>& msgList, QList<int>& index_ret )
{
  int ret = 0;
  int index;
  for ( QList<KMime::Message*>::const_iterator it = msgList.constBegin();
        it != msgList.constEnd(); ++it )
  {
    int aret = addMsg( *it, &index );
    index_ret << index;
    if ( aret != 0 ) // error condition
      ret = aret;
  }
  return ret;
}

//-----------------------------------------------------------------------------
bool FolderStorage::isMoveable() const
{
  return !folder()->isSystemFolder();
}

/*virtual*/
KMAccount* FolderStorage::account() const
{
  return 0;
}

QString FolderStorage::location(const QString& suffix) const
{
#if 0
  QString sLocation( folder()->path() );

  if ( !sLocation.isEmpty() )
    sLocation += "/.";
  sLocation += dotEscape( fileName() ) + ".index";
  if ( !suffix.isEmpty() ) {
    sLocation += '.';
    sLocation += suffix;
  }
  return sLocation;
#endif
return QString();
}

QString FolderStorage::indexLocation() const
{
#ifdef KMAIL_SQLITE_INDEX
  return location( "db" );
#else
  return location( QString() );
#endif
}

QString FolderStorage::idsLocation() const
{
  return location( "ids" );
}

QString FolderStorage::sortedLocation() const
{
  return location( "sorted" );
}

bool FolderStorage::canDeleteMessages() const
{
  return !isReadOnly();
}

void FolderStorage::setNoContent(bool aNoContent)
{
  const bool changed = aNoContent != mNoContent;
  mNoContent = aNoContent;
  if ( changed )
    emit noContentChanged();
}

#include "folderstorage.moc"
