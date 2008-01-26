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

#include <config.h>

#include "folderstorage.h"
#include "kmfolder.h"
#include "kmkernel.h"

#include "kmfolderimap.h" //for the nasty imap hacks, FIXME
#include "undostack.h"
#include "kmmsgdict.h"
#include "kmfoldermgr.h"
#include "kmcommands.h"
#include "listjob.h"
using KMail::ListJob;
#include "kmsearchpattern.h"
#include "globalsettings.h"

#include <klocale.h>
#include <kconfig.h>
#include <kdebug.h>

#include <qfile.h>
#include <qregexp.h>

#include <mimelib/mimepp.h>
#include <errno.h>

//-----------------------------------------------------------------------------

FolderStorage::FolderStorage( KMFolder* folder, const char* aName )
  : QObject( folder, aName ), mFolder( folder ), mEmitChangedTimer( 0L )
{
  mOpenCount = 0;
  mQuiet = 0;
  mChanged = false;
  mAutoCreateIndex = true;
  mExportsSernums = false;
  mDirty = false;
  mUnreadMsgs = -1;
  mGuessedUnreadMsgs = -1;
  mTotalMsgs = -1;
  mSize = -1;
  needsCompact    = false;
  mConvertToUtf8  = false;
  mCompactable     = true;
  mNoContent      = false;
  mNoChildren     = false;
  mRDict = 0;
  mDirtyTimer = new QTimer(this, "mDirtyTimer");
  connect(mDirtyTimer, SIGNAL(timeout()),
	  this, SLOT(updateIndex()));

  mHasChildren = HasNoChildren;
  mContentsType = KMail::ContentsTypeMail;
 
  connect(this, SIGNAL(closed(KMFolder*)), mFolder, SIGNAL(closed()));  
}

//-----------------------------------------------------------------------------
FolderStorage::~FolderStorage()
{
  mJobList.setAutoDelete( true );
  QObject::disconnect( SIGNAL(destroyed(QObject*)), this, 0 );
  mJobList.clear();
  KMMsgDict::deleteRentry(mRDict);
}


void FolderStorage::close( const char* owner, bool aForced )
{
  if (mOpenCount <= 0) return;
  if (mOpenCount > 0) mOpenCount--;
  if (mOpenCount > 0 && !aForced) return;

  // kdWarning() << "Really closing: " << folder()->prettyURL()  << kdBacktrace() << endl;
  reallyDoClose(owner);
}

//-----------------------------------------------------------------------------
QString FolderStorage::dotEscape(const QString& aStr)
{
  if (aStr[0] != '.') return aStr;
  return aStr.left(aStr.find(QRegExp("[^\\.]"))) + aStr;
}

void FolderStorage::addJob( FolderJob* job ) const
{
  QObject::connect( job, SIGNAL(destroyed(QObject*)),
                    SLOT(removeJob(QObject*)) );
  mJobList.append( job );
}

void FolderStorage::removeJob( QObject* job )
{
  mJobList.remove( static_cast<FolderJob*>( job ) );
}


//-----------------------------------------------------------------------------
QString FolderStorage::location() const
{
  QString sLocation(const_cast<FolderStorage*>(this)->folder()->path());

  if (!sLocation.isEmpty()) sLocation += '/';
  sLocation += dotEscape(fileName());

  return sLocation;
}

QString FolderStorage::fileName() const
{
  return mFolder->name();
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
    mDirtyTimer->changeInterval( mDirtyTimerInterval );
  else
    mDirtyTimer->stop();
}

//-----------------------------------------------------------------------------
void FolderStorage::markNewAsUnread()
{
  KMMsgBase* msgBase;
  int i;

  for (i=0; i< count(); ++i)
  {
    if (!(msgBase = getMsgBase(i))) continue;
    if (msgBase->isNew())
    {
      msgBase->setStatus(KMMsgStatusUnread);
      msgBase->setDirty(true);
    }
  }
}

void FolderStorage::markUnreadAsRead()
{
  KMMsgBase* msgBase;
  SerNumList serNums;

  for (int i=count()-1; i>=0; --i)
  {
    msgBase = getMsgBase(i);
    assert(msgBase);
    if (msgBase->isNew() || msgBase->isUnread())
    {
      serNums.append( msgBase->getMsgSerNum() );
    }
  }
  if (serNums.empty())
    return;

  KMCommand *command = new KMSetStatusCommand( KMMsgStatusRead, serNums );
  command->start();
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
      mEmitChangedTimer= new QTimer( this, "mEmitChangedTimer" );
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
int operator<( KMMsgBase & m1, KMMsgBase & m2 )
{
  return (m1.date() < m2.date());
}

/** Compare message's date. This is useful for message sorting */
int operator==( KMMsgBase & m1, KMMsgBase & m2 )
{
  return (m1.date() == m2.date());
}


//-----------------------------------------------------------------------------
int FolderStorage::expungeOldMsg(int days)
{
  int i, msgnb=0;
  time_t msgTime, maxTime;
  const KMMsgBase* mb;
  QValueList<int> rmvMsgList;

  maxTime = time(0) - days * 3600 * 24;

  for (i=count()-1; i>=0; i--) {
    mb = getMsgBase(i);
    assert(mb);
    msgTime = mb->date();

    if (msgTime < maxTime) {
      //kdDebug(5006) << "deleting msg " << i << " : " << mb->subject() << " - " << mb->dateStr(); // << endl;
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
  Q_UINT32 serNum = KMMsgDict::instance()->getMsgSerNum( folder() , idx );
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
bool FolderStorage::canAddMsgNow(KMMessage* aMsg, int* aIndex_ret)
{
  if (aIndex_ret) *aIndex_ret = -1;
  KMFolder *msgParent = aMsg->parent();
  // If the message has a parent and is in transfer, bail out. If it does not
  // have a parent we want to be able to add it even if it is in transfer.
  if (aMsg->transferInProgress() && msgParent)
      return false;
  if (!aMsg->isComplete() && msgParent && msgParent->folderType() == KMFolderTypeImap)
  {
    FolderJob *job = msgParent->createJob(aMsg);
    connect(job, SIGNAL(messageRetrieved(KMMessage*)),
            SLOT(reallyAddMsg(KMMessage*)));
    job->start();
    aMsg->setTransferInProgress( true );
    return false;
  }
  return true;
}


//-----------------------------------------------------------------------------
void FolderStorage::reallyAddMsg(KMMessage* aMsg)
{
  if (!aMsg) // the signal that is connected can call with aMsg=0
    return;
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
}


//-----------------------------------------------------------------------------
void FolderStorage::reallyAddCopyOfMsg(KMMessage* aMsg)
{
  if ( !aMsg ) return; // messageRetrieved(0) is always possible
  aMsg->setParent( 0 );
  aMsg->setTransferInProgress( false );
  addMsg( aMsg );
  unGetMsg( count() - 1 );
}

int FolderStorage::find( const KMMessage * msg ) const {
  return find( &msg->toMsgBase() );
}

//-----------------------------------------------------------------------------
void FolderStorage::removeMsg(const QPtrList<KMMsgBase>& msgList, bool imapQuiet)
{
  for( QPtrListIterator<KMMsgBase> it( msgList ); *it; ++it )
  {
    int idx = find(it.current());
    assert( idx != -1);
    removeMsg(idx, imapQuiet);
  }
}

//-----------------------------------------------------------------------------
void FolderStorage::removeMsg(const QPtrList<KMMessage>& msgList, bool imapQuiet)
{
  for( QPtrListIterator<KMMessage> it( msgList ); *it; ++it )
  {
    int idx = find(it.current());
    assert( idx != -1);
    removeMsg(idx, imapQuiet);
  }
}

//-----------------------------------------------------------------------------
void FolderStorage::removeMsg(int idx, bool)
{
  //assert(idx>=0);
  if(idx < 0)
  {
    kdDebug(5006) << "FolderStorage::removeMsg() : idx < 0\n" << endl;
    return;
  }

  KMMsgBase* mb = getMsgBase(idx);

  Q_UINT32 serNum = KMMsgDict::instance()->getMsgSerNum( folder(), idx );
  if (serNum != 0)
    emit msgRemoved( folder(), serNum );
  mb = takeIndexEntry( idx );

  setDirty( true );
  needsCompact=true; // message is taken from here - needs to be compacted

  if (mb->isUnread() || mb->isNew() ||
      (folder() == kmkernel->outboxFolder())) {
    --mUnreadMsgs;
    if ( !mQuiet ) {
//      kdDebug( 5006 ) << "FolderStorage::msgStatusChanged" << endl;
      emit numUnreadMsgsChanged( folder() );
    }else{
      if ( !mEmitChangedTimer->isActive() ) {
//        kdDebug( 5006 )<< "EmitChangedTimer started" << endl;
        mEmitChangedTimer->start( 3000 );
      }
      mChanged = true;
    }
  }
  --mTotalMsgs;

  mSize = -1;
  QString msgIdMD5 = mb->msgIdMD5();
  emit msgRemoved( idx, msgIdMD5 );
  emit msgRemoved( folder() );
}


//-----------------------------------------------------------------------------
KMMessage* FolderStorage::take(int idx)
{
  KMMsgBase* mb;
  KMMessage* msg;

  assert(idx>=0 && idx<=count());

  mb = getMsgBase(idx);
  if (!mb) return 0;
  if (!mb->isMessage()) readMsg(idx);
  Q_UINT32 serNum = KMMsgDict::instance()->getMsgSerNum( folder(), idx );
  emit msgRemoved( folder(), serNum );

  msg = (KMMessage*)takeIndexEntry(idx);

  if (msg->isUnread() || msg->isNew() ||
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
  mSize = -1;
  needsCompact=true; // message is taken from here - needs to be compacted
  QString msgIdMD5 = msg->msgIdMD5();
  emit msgRemoved( idx, msgIdMD5 );
  emit msgRemoved( folder() );

  return msg;
}

void FolderStorage::take(QPtrList<KMMessage> msgList)
{
  for ( KMMessage* msg = msgList.first(); msg; msg = msgList.next() )
  {
    if (msg->parent())
    {
      int idx = msg->parent()->find(msg);
      if ( idx >= 0 )
        take(idx);
    }
  }
}


//-----------------------------------------------------------------------------
KMMessage* FolderStorage::getMsg(int idx)
{
  if ( mOpenCount <= 0 ) {
    kdWarning(5006) << "FolderStorage::getMsg was called on a closed folder: " << folder()->prettyURL() << endl; 
    return 0;
  }
  if ( idx < 0 || idx >= count() ) { 
    kdWarning(5006) << "FolderStorage::getMsg was asked for an invalid index. idx =" << idx << " count()=" << count() << endl; 
    return 0;
  }

  KMMsgBase* mb = getMsgBase(idx);
  if (!mb) {
    kdWarning(5006) << "FolderStorage::getMsg, getMsgBase failed for index: " << idx << endl;
    return 0;
  }

  KMMessage *msg = 0;
  bool undo = mb->enableUndo();
  if (mb->isMessage()) {
      msg = ((KMMessage*)mb);
  } else {
      QString mbSubject = mb->subject();
      msg = readMsg(idx);
      // sanity check
      if (mCompactable && (!msg || (msg->subject().isEmpty() != mbSubject.isEmpty()))) {
        kdDebug(5006) << "Error: " << location() <<
          " Index file is inconsistent with folder file. This should never happen." << endl;
        mCompactable = false; // Don't compact
        writeConfig();
      }

  }
  // Either isMessage and we had a sernum, or readMsg gives us one
  // (via insertion into mMsgList). sernum == 0 may still occur due to
  // an outdated or corrupt IMAP cache.
  if ( msg->getMsgSerNum() == 0 ) {
    kdWarning(5006) << "FolderStorage::getMsg, message has no sernum, index: " << idx << endl;
    return 0;
  }
  msg->setEnableUndo(undo);
  msg->setComplete( true );
  return msg;
}

//-----------------------------------------------------------------------------
KMMessage* FolderStorage::readTemporaryMsg(int idx)
{
  if(!(idx >= 0 && idx <= count()))
    return 0;

  KMMsgBase* mb = getMsgBase(idx);
  if (!mb) return 0;

  unsigned long sernum = mb->getMsgSerNum();

  KMMessage *msg = 0;
  bool undo = mb->enableUndo();
  if (mb->isMessage()) {
    // the caller will delete it, so we must make a copy it
    msg = new KMMessage(*(KMMessage*)mb);
    msg->setMsgSerNum(sernum);
    msg->setComplete( true );
  } else {
    // ## Those two lines need to be moved to a virtual method for KMFolderSearch, like readMsg
    msg = new KMMessage(*(KMMsgInfo*)mb);
    msg->setMsgSerNum(sernum); // before fromDwString so that readyToShow uses the right sernum
    msg->setComplete( true );
    msg->fromDwString(getDwString(idx));
  }
  msg->setEnableUndo(undo);
  return msg;
}


//-----------------------------------------------------------------------------
KMMsgInfo* FolderStorage::unGetMsg(int idx)
{
  KMMsgBase* mb;

  if(!(idx >= 0 && idx <= count()))
    return 0;

  mb = getMsgBase(idx);
  if (!mb) return 0;


  if (mb->isMessage()) {
    // Remove this message from all jobs' list it might still be on.
    // setIndexEntry deletes the message.
    KMMessage *msg = static_cast<KMMessage*>(mb);
    if ( msg->transferInProgress() ) return 0;
    ignoreJobsForMessage( msg );
    return setIndexEntry( idx, msg );
  }

  return 0;
}


//-----------------------------------------------------------------------------
bool FolderStorage::isMessage(int idx)
{
  KMMsgBase* mb;
  if (!(idx >= 0 && idx <= count())) return false;
  mb = getMsgBase(idx);
  return (mb && mb->isMessage());
}

//-----------------------------------------------------------------------------
FolderJob* FolderStorage::createJob( KMMessage *msg, FolderJob::JobType jt,
                                KMFolder *folder, QString partSpecifier,
                                const AttachmentStrategy *as ) const
{
  FolderJob * job = doCreateJob( msg, jt, folder, partSpecifier, as );
  if ( job )
    addJob( job );
  return job;
}

//-----------------------------------------------------------------------------
FolderJob* FolderStorage::createJob( QPtrList<KMMessage>& msgList, const QString& sets,
                                FolderJob::JobType jt, KMFolder *folder ) const
{
  FolderJob * job = doCreateJob( msgList, sets, jt, folder );
  if ( job )
    addJob( job );
  return job;
}

//-----------------------------------------------------------------------------
int FolderStorage::moveMsg(KMMessage* aMsg, int* aIndex_ret)
{
  assert(aMsg != 0);
  KMFolder* msgParent = aMsg->parent();

  if (msgParent)
    msgParent->open("moveMsgSrc");

  open("moveMsgDest");
  int rc = addMsg(aMsg, aIndex_ret);
  close("moveMsgDest");

  if (msgParent)
    msgParent->close("moveMsgSrc");

  return rc;
}

//-----------------------------------------------------------------------------
int FolderStorage::moveMsg(QPtrList<KMMessage> msglist, int* aIndex_ret)
{
  KMMessage* aMsg = msglist.first();
  assert(aMsg != 0);
  KMFolder* msgParent = aMsg->parent();

  if (msgParent)
    msgParent->open("foldermovemsg");

  QValueList<int> index;
  open("moveMsg");
  int rc = addMsg(msglist, index);
  close("moveMsg");
  // FIXME: we want to have a QValueList to pass it back, so change this method
  if ( !index.isEmpty() )
    aIndex_ret = &index.first();

  if (msgParent)
    msgParent->close("foldermovemsg");

  return rc;
}


//-----------------------------------------------------------------------------
int FolderStorage::rename(const QString& newName, KMFolderDir *newParent)
{
  QString oldLoc, oldIndexLoc, oldIdsLoc, newLoc, newIndexLoc, newIdsLoc;
  QString oldSubDirLoc, newSubDirLoc;
  QString oldName;
  int rc=0;
  KMFolderDir *oldParent;

  assert(!newName.isEmpty());

  oldLoc = location();
  oldIndexLoc = indexLocation();
  oldSubDirLoc = folder()->subdirLocation();
  oldIdsLoc =  KMMsgDict::instance()->getFolderIdsLocation( *this );
  QString oldConfigString = "Folder-" + folder()->idString();

  close("rename", true);

  oldName = folder()->fileName();
  oldParent = folder()->parent();
  if (newParent)
    folder()->setParent( newParent );

  folder()->setName(newName);
  newLoc = location();
  newIndexLoc = indexLocation();
  newSubDirLoc = folder()->subdirLocation();
  newIdsLoc = KMMsgDict::instance()->getFolderIdsLocation( *this );

  if (::rename(QFile::encodeName(oldLoc), QFile::encodeName(newLoc))) {
    folder()->setName(oldName);
    folder()->setParent(oldParent);
    rc = errno;
  }
  else {
    // rename/move index file and index.sorted file
    if (!oldIndexLoc.isEmpty()) {
      ::rename(QFile::encodeName(oldIndexLoc), QFile::encodeName(newIndexLoc));
      ::rename(QFile::encodeName(oldIndexLoc) + ".sorted",
               QFile::encodeName(newIndexLoc) + ".sorted");
    }

    // rename/move serial number file
    if (!oldIdsLoc.isEmpty())
      ::rename(QFile::encodeName(oldIdsLoc), QFile::encodeName(newIdsLoc));

    // rename/move the subfolder directory
    KMFolderDir* child = 0;
    if( folder() )
      child = folder()->child();

    if (!::rename(QFile::encodeName(oldSubDirLoc), QFile::encodeName(newSubDirLoc) )) {
      // now that the subfolder directory has been renamed and/or moved also
      // change the name that is stored in the corresponding KMFolderNode
      // (provide that the name actually changed)
      if( child && ( oldName != newName ) ) {
        child->setName( "." + QFile::encodeName(newName) + ".directory" );
      }
    }

    // if the folder is being moved then move its node and, if necessary, also
    // the associated subfolder directory node to the new parent
    if (newParent) {
      if (oldParent->findRef( folder() ) != -1)
        oldParent->take();
      newParent->inSort( folder() );
      if ( child ) {
        if ( child->parent()->findRef( child ) != -1 )
          child->parent()->take();
        newParent->inSort( child );
        child->setParent( newParent );
      }
    }
  }

  writeConfig();

  // delete the old entry as we get two entries with the same ID otherwise
  if ( oldConfigString != "Folder-" + folder()->idString() )
    KMKernel::config()->deleteGroup( oldConfigString );

  emit locationChanged( oldLoc, newLoc );
  emit nameChanged();
  kmkernel->folderMgr()->contentsChanged();
  emit closed(folder()); // let the ticket owners regain
  return rc;
}


//-----------------------------------------------------------------------------
void FolderStorage::remove()
{
  assert(!folder()->name().isEmpty());

  clearIndex( true, mExportsSernums ); // delete and remove from dict if necessary
  close("remove", true);

  if ( mExportsSernums ) {
    KMMsgDict::mutableInstance()->removeFolderIds( *this );
    mExportsSernums = false;	// do not writeFolderIds after removal
  }
  unlink(QFile::encodeName(indexLocation()) + ".sorted");
  unlink(QFile::encodeName(indexLocation()));

  int rc = removeContents();

  needsCompact = false; //we are dead - no need to compact us

  // Erase settings, otherwise they might interfer when recreating the folder
  KConfig* config = KMKernel::config();
  config->deleteGroup( "Folder-" + folder()->idString() );

  emit closed(folder());
  emit removed(folder(), (rc ? false : true));
}


//-----------------------------------------------------------------------------
int FolderStorage::expunge()
{
  assert(!folder()->name().isEmpty());

  clearIndex( true, mExportsSernums );   // delete and remove from dict, if needed
  close( "expunge", true );

  if ( mExportsSernums )
    KMMsgDict::mutableInstance()->removeFolderIds( *this );
  if ( mAutoCreateIndex )
    truncateIndex();
  else unlink(QFile::encodeName(indexLocation()));

  int rc = expungeContents();
  if (rc) return rc;

  mDirty = false;
  needsCompact = false; //we're cleared and truncated no need to compact

  mUnreadMsgs = 0;
  mTotalMsgs = 0;
  mSize = 0;
  emit numUnreadMsgsChanged( folder() );
  if ( mAutoCreateIndex ) // FIXME Heh? - Till
    writeConfig();
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
  if (mGuessedUnreadMsgs > -1)
    return mGuessedUnreadMsgs;
  if (mUnreadMsgs > -1)
    return mUnreadMsgs;

  readConfig();

  if (mUnreadMsgs > -1)
    return mUnreadMsgs;

  open("countunread"); // will update unreadMsgs
  int unread = mUnreadMsgs;
  close("countunread");
  return (unread > 0) ? unread : 0;
}

Q_INT64 FolderStorage::folderSize() const
{
    if ( mSize != -1 ) {
        return mSize;
    } else {
        return doFolderSize();
    }
}


/*virtual*/
bool FolderStorage::isCloseToQuota() const
{
  return false;
}

//-----------------------------------------------------------------------------
void FolderStorage::msgStatusChanged(const KMMsgStatus oldStatus,
  const KMMsgStatus newStatus, int idx)
{
  int oldUnread = 0;
  int newUnread = 0;

  if (((oldStatus & KMMsgStatusUnread || oldStatus & KMMsgStatusNew) &&
      !(oldStatus & KMMsgStatusIgnored)) ||
      (folder() == kmkernel->outboxFolder()))
    oldUnread = 1;
  if (((newStatus & KMMsgStatusUnread || newStatus & KMMsgStatusNew) &&
      !(newStatus & KMMsgStatusIgnored)) ||
      (folder() == kmkernel->outboxFolder()))
    newUnread = 1;
  int deltaUnread = newUnread - oldUnread;

  mDirtyTimer->changeInterval(mDirtyTimerInterval);
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
    Q_UINT32 serNum = KMMsgDict::instance()->getMsgSerNum(folder(), idx);
    emit msgChanged( folder(), serNum, deltaUnread );
  }
}

//-----------------------------------------------------------------------------
void FolderStorage::headerOfMsgChanged(const KMMsgBase* aMsg, int idx)
{
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
}

//-----------------------------------------------------------------------------
void FolderStorage::readConfig()
{
  //kdDebug(5006)<<"#### READING CONFIG  = "<< name() <<endl;
  KConfig* config = KMKernel::config();
  KConfigGroupSaver saver(config, "Folder-" + folder()->idString());
  if (mUnreadMsgs == -1)
    mUnreadMsgs = config->readNumEntry("UnreadMsgs", -1);
  if (mTotalMsgs == -1)
    mTotalMsgs = config->readNumEntry("TotalMsgs", -1);
  mCompactable = config->readBoolEntry("Compactable", true);
  if ( mSize == -1 )
      mSize = config->readNum64Entry("FolderSize", -1);
  
  int type = config->readNumEntry( "ContentsType", 0 );
  if ( type < 0 || type > KMail::ContentsTypeLast ) type = 0;
  setContentsType( static_cast<KMail::FolderContentsType>( type ) );

  if( folder() ) folder()->readConfig( config );
}

//-----------------------------------------------------------------------------
void FolderStorage::writeConfig()
{
  KConfig* config = KMKernel::config();
  KConfigGroupSaver saver(config, "Folder-" + folder()->idString());
  config->writeEntry("UnreadMsgs",
      mGuessedUnreadMsgs == -1 ? mUnreadMsgs : mGuessedUnreadMsgs);
  config->writeEntry("TotalMsgs", mTotalMsgs);
  config->writeEntry("Compactable", mCompactable);
  config->writeEntry("ContentsType", mContentsType);
  config->writeEntry("FolderSize", mSize);

  // Write the KMFolder parts
  if( folder() ) folder()->writeConfig( config );

  GlobalSettings::self()->requestSync();
}

//-----------------------------------------------------------------------------
void FolderStorage::correctUnreadMsgsCount()
{
  open("countunreadmsg");
  close("countunreadmsg");
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
  unlink(QFile::encodeName( indexLocation()) + ".sorted");
  unlink(QFile::encodeName( indexLocation()) + ".ids");
  fillMessageDict();
  KMMsgDict::mutableInstance()->writeFolderIds( *this );
  emit invalidated( folder() );
}


//-----------------------------------------------------------------------------
int FolderStorage::writeFolderIdsFile() const
{
  if ( !mExportsSernums ) return -1;
  return KMMsgDict::mutableInstance()->writeFolderIds( *this );
}

//-----------------------------------------------------------------------------
int FolderStorage::touchFolderIdsFile()
{
  if ( !mExportsSernums ) return -1;
  return KMMsgDict::mutableInstance()->touchFolderIds( *this );
}

//-----------------------------------------------------------------------------
int FolderStorage::appendToFolderIdsFile( int idx )
{
  if ( !mExportsSernums ) return -1;
  int ret = 0;
  if ( count() == 1 ) {
    ret = KMMsgDict::mutableInstance()->writeFolderIds( *this );
  } else {
    ret = KMMsgDict::mutableInstance()->appendToFolderIds( *this, idx );
  }
  return ret;
}

void FolderStorage::replaceMsgSerNum( unsigned long sernum, KMMsgBase* msg, int idx )
{
  if ( !mExportsSernums ) return;
  KMMsgDict::mutableInstance()->replace( sernum, msg, idx );
}

void FolderStorage::setRDict( KMMsgDictREntry *rentry ) const
{
  if ( ! mExportsSernums )
    kdDebug(5006) << "WTF, this FolderStorage should be invisible to the msgdict, who is calling us?" << kdBacktrace() << endl;
  assert( mExportsSernums ); // otherwise things are very wrong
  if ( rentry == mRDict )
    return;
  KMMsgDict::deleteRentry( mRDict );
  mRDict = rentry;
}

//-----------------------------------------------------------------------------
void FolderStorage::setStatus(int idx, KMMsgStatus status, bool toggle)
{
  KMMsgBase *msg = getMsgBase(idx);
  if ( msg ) {
    if (toggle)
      msg->toggleStatus(status, idx);
    else
      msg->setStatus(status, idx);
  }
}


//-----------------------------------------------------------------------------
void FolderStorage::setStatus(QValueList<int>& ids, KMMsgStatus status, bool toggle)
{
  for ( QValueList<int>::Iterator it = ids.begin(); it != ids.end(); ++it )
  {
    FolderStorage::setStatus(*it, status, toggle);
  }
}

void FolderStorage::ignoreJobsForMessage( KMMessage *msg )
{
  if ( !msg || msg->transferInProgress() )
    return;

  QPtrListIterator<FolderJob> it( mJobList );
  while ( it.current() )
  {
    //FIXME: the questions is : should we iterate through all
    //messages in jobs? I don't think so, because it would
    //mean canceling the jobs that work with other messages
    if ( it.current()->msgList().first() == msg )
    {
      FolderJob* job = it.current();
      mJobList.remove( job );
      delete job;
    } else
      ++it;
  }
}

//-----------------------------------------------------------------------------
void FolderStorage::removeJobs()
{
  mJobList.setAutoDelete( true );
  mJobList.clear();
  mJobList.setAutoDelete( false );
}



//-----------------------------------------------------------------------------
void FolderStorage::updateChildrenState()
{
  if ( folder() && folder()->child() )
  {
    if ( kmkernel->folderMgr()->folderCount( folder()->child() ) > 0 )
      setHasChildren( HasChildren );
    else
      setHasChildren( HasNoChildren );
  }
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
  QValueList<Q_UINT32> matchingSerNums;
  const int end = QMIN( mCurrentSearchedMsg + 15, count() );
  for ( int i = mCurrentSearchedMsg; i < end; ++i )
  {
    Q_UINT32 serNum = KMMsgDict::instance()->getMsgSerNum( folder(), i );
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
void FolderStorage::search( const KMSearchPattern* pattern, Q_UINT32 serNum )
{
  bool matches = pattern && pattern->matches( serNum );

  emit searchDone( folder(), serNum, pattern, matches );
}

//-----------------------------------------------------------------------------
int FolderStorage::addMsg( QPtrList<KMMessage>& msgList, QValueList<int>& index_ret )
{
  int ret = 0;
  int index;
  for ( QPtrListIterator<KMMessage> it( msgList ); *it; ++it )
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
  return ( folder()->isSystemFolder() ) ? false : true;
}


/*virtual*/
KMAccount* FolderStorage::account() const
{
    return 0;
}

#include "folderstorage.moc"
