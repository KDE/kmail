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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.

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

#include "kmfolderimap.h" //for the nasty imap hacks, FIXME
#include "undostack.h"
#include "kmmsgdict.h"
#include "kmfoldermgr.h"
#include "kmkernel.h"
#include "kmcommands.h"
#include "listjob.h"
using KMail::ListJob;
#include "kmailicalifaceimpl.h"
#include "kmsearchpattern.h"

#include <klocale.h>
#include <kconfig.h>
#include <kdebug.h>

#include <qfile.h>
#include <qregexp.h>

#include <mimelib/mimepp.h>
#include <errno.h>

//-----------------------------------------------------------------------------

FolderStorage::FolderStorage( KMFolder* folder, const char* aName )
  : QObject( folder, aName ), mFolder( folder )
{
  mOpenCount      = 0;
  mQuiet	  = 0;
  mChanged        = FALSE;
  mAutoCreateIndex= TRUE;
  folder->setType( "plain" );
  mAcctList       = 0;
  mDirty          = FALSE;
  mUnreadMsgs      = -1;
  mGuessedUnreadMsgs = -1;
  mTotalMsgs      = -1;
  needsCompact    = FALSE;
  mConvertToUtf8  = FALSE;
  mCompactable     = TRUE;
  mNoContent      = FALSE;
  mNoChildren     = FALSE;
  mRDict = 0;
  mDirtyTimer = new QTimer(this);
  connect(mDirtyTimer, SIGNAL(timeout()),
	  this, SLOT(updateIndex()));
  mHasChildren = HasNoChildren;
  mContentsType = KMail::ContentsTypeMail;
}

//-----------------------------------------------------------------------------
FolderStorage::~FolderStorage()
{
  delete mAcctList;
  mJobList.setAutoDelete( true );
  QObject::disconnect( SIGNAL(destroyed(QObject*)), this, 0 );
  mJobList.clear();
  KMMsgDict::deleteRentry(mRDict);
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
      msgBase->setDirty(TRUE);
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
    mQuiet++;
  else {
    mQuiet--;
    if (mQuiet <= 0)
    {
      mQuiet = 0;
      if (mChanged)
       emit changed();
      mChanged = FALSE;
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

//-----------------------------------------------------------------------------
void FolderStorage::emitMsgAddedSignals(int idx)
{
  Q_UINT32 serNum = kmkernel->msgDict()->getMsgSerNum( folder() , idx );
  if (!mQuiet) {
    emit msgAdded(idx);
  } else {
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
    return FALSE;
  }
  return TRUE;
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

  Q_UINT32 serNum = kmkernel->msgDict()->getMsgSerNum( folder(), idx );
  if (serNum != 0)
    emit msgRemoved( folder(), serNum );
  mb = takeIndexEntry( idx );

  setDirty( true );
  needsCompact=true; // message is taken from here - needs to be compacted

  if (mb->isUnread() || mb->isNew() ||
      (folder() == kmkernel->outboxFolder())) {
    --mUnreadMsgs;
    emit numUnreadMsgsChanged( folder() );
  }
  --mTotalMsgs;

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
  Q_UINT32 serNum = kmkernel->msgDict()->getMsgSerNum( folder(), idx );
  emit msgRemoved( folder(), serNum );

  msg = (KMMessage*)takeIndexEntry(idx);

  if (msg->isUnread() || msg->isNew() ||
      ( folder() == kmkernel->outboxFolder() )) {
    --mUnreadMsgs;
    emit numUnreadMsgsChanged( folder() );
  }
  --mTotalMsgs;
  msg->setParent(0);
  setDirty( true );
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
      assert( idx != -1);
      FolderStorage::take(idx);
    }
  }
}


//-----------------------------------------------------------------------------
KMMessage* FolderStorage::getMsg(int idx)
{
  if(!(idx >= 0 && idx <= count()))
    return 0;

  KMMsgBase* mb = getMsgBase(idx);
  if (!mb) return 0;

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
	  mCompactable = FALSE; // Don't compact
	  writeConfig();
      }

  }
  msg->setEnableUndo(undo);

  // Can't happen. Either isMessage and we had a sernum, or readMsg gives us one
  // (via insertion into mMsgList).
  if (msg->getMsgSerNum() == 0) {
    msg->setMsgSerNum(kmkernel->msgDict()->insert(0, msg, idx));
    kdDebug(5006) << "Serial number generated for message in folder "
                  << label() << endl;
  }
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
  } else {
    // ## Those two lines need to be moved to a virtual method for KMFolderSearch, like readMsg
    msg = new KMMessage(*(KMMsgInfo*)mb);
    msg->setMsgSerNum(sernum); // before fromDwString so that readyToShow uses the right sernum
    msg->fromDwString(getDwString(idx));
  }
  msg->setEnableUndo(undo);
  msg->setComplete( true );
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
  if (!(idx >= 0 && idx <= count())) return FALSE;
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
    msgParent->open();

  open();
  int rc = addMsg(aMsg, aIndex_ret);
  close();

  if (msgParent)
    msgParent->close();

  return rc;
}

//-----------------------------------------------------------------------------
int FolderStorage::moveMsg(QPtrList<KMMessage> msglist, int* aIndex_ret)
{
  KMMessage* aMsg = msglist.first();
  assert(aMsg != 0);
  KMFolder* msgParent = aMsg->parent();

  if (msgParent)
    msgParent->open();

  QValueList<int> index;
  open();
  int rc = addMsg(msglist, index);
  close();
  // FIXME: we want to have a QValueList to pass it back, so change this method
  if ( !index.isEmpty() )
    aIndex_ret = &index.first();

  if (msgParent)
    msgParent->close();

  return rc;
}


//-----------------------------------------------------------------------------
int FolderStorage::rename(const QString& newName, KMFolderDir *newParent)
{
  QString oldLoc, oldIndexLoc, oldIdsLoc, newLoc, newIndexLoc, newIdsLoc;
  QString oldSubDirLoc, newSubDirLoc;
  QString oldName;
  int rc=0, openCount=mOpenCount;
  KMFolderDir *oldParent;

  assert(!newName.isEmpty());

  oldLoc = location();
  oldIndexLoc = indexLocation();
  oldSubDirLoc = folder()->subdirLocation();
  if (kmkernel->msgDict())
    oldIdsLoc = kmkernel->msgDict()->getFolderIdsLocation( folder() );
  QString oldConfigString = "Folder-" + folder()->idString();

  close(TRUE);

  oldName = folder()->fileName();
  oldParent = folder()->parent();
  if (newParent)
    folder()->setParent( newParent );

  folder()->setName(newName);
  newLoc = location();
  newIndexLoc = indexLocation();
  newSubDirLoc = folder()->subdirLocation();
  if (kmkernel->msgDict())
    newIdsLoc = kmkernel->msgDict()->getFolderIdsLocation( folder() );

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

  if (openCount > 0)
  {
    open();
    mOpenCount = openCount;
  }
  writeConfig();

  // delete the old entry as we get two entries with the same ID otherwise
  KMKernel::config()->deleteGroup( oldConfigString );

  emit locationChanged( oldLoc, newLoc );
  emit nameChanged();
  kmkernel->folderMgr()->contentsChanged();
  return rc;
}


//-----------------------------------------------------------------------------
void FolderStorage::remove()
{
  assert(!folder()->name().isEmpty());

  clearIndex(true, true); // delete and remove from dict
  close(TRUE);

  if (kmkernel->msgDict()) kmkernel->msgDict()->removeFolderIds( folder() );
  unlink(QFile::encodeName(indexLocation()) + ".sorted");
  unlink(QFile::encodeName(indexLocation()));

  int rc = removeContents();

  needsCompact = false; //we are dead - no need to compact us

  // Erase settings, otherwise they might interfer when recreating the folder
  KConfig* config = KMKernel::config();
  config->deleteGroup( "Folder-" + folder()->idString() );

  emit removed(folder(), (rc ? false : true));
}


//-----------------------------------------------------------------------------
int FolderStorage::expunge()
{
  int openCount = mOpenCount;

  assert(!folder()->name().isEmpty());

  clearIndex(true, true);   // delete and remove from dict
  close(TRUE);

  kmkernel->msgDict()->removeFolderIds( folder() );
  if (mAutoCreateIndex)
    truncateIndex();
  else unlink(QFile::encodeName(indexLocation()));

  int rc = expungeContents();
  if (rc) return rc;

  mDirty = FALSE;
  needsCompact = false; //we're cleared and truncated no need to compact

  if (openCount > 0)
  {
    open();
    mOpenCount = openCount;
  }

  mUnreadMsgs = 0;
  mTotalMsgs = 0;
  emit numUnreadMsgsChanged( folder() );
  if (mAutoCreateIndex)
    writeConfig();
  emit changed();
  emit expunged( folder() );

  return 0;
}


//-----------------------------------------------------------------------------
const char* FolderStorage::type() const
{
  if (mAcctList) return "In";
  return folder()->KMFolderNode::type();
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

  open(); // will update unreadMsgs
  int unread = mUnreadMsgs;
  close();
  return (unread > 0) ? unread : 0;
}

//-----------------------------------------------------------------------------
void FolderStorage::msgStatusChanged(const KMMsgStatus oldStatus,
  const KMMsgStatus newStatus, int idx)
{
  int oldUnread = 0;
  int newUnread = 0;

  if (oldStatus & KMMsgStatusUnread || oldStatus & KMMsgStatusNew ||
      (folder() == kmkernel->outboxFolder()))
    oldUnread = 1;
  if (newStatus & KMMsgStatusUnread || newStatus & KMMsgStatusNew ||
      (folder() == kmkernel->outboxFolder()))
    newUnread = 1;
  int deltaUnread = newUnread - oldUnread;

  mDirtyTimer->changeInterval(mDirtyTimerInterval);
  if (deltaUnread != 0) {
    if (mUnreadMsgs < 0) mUnreadMsgs = 0;
    mUnreadMsgs += deltaUnread;
    if ( !mQuiet )
      emit numUnreadMsgsChanged( folder() );
    else
      mChanged = true;
    Q_UINT32 serNum = kmkernel->msgDict()->getMsgSerNum(folder(), idx);
    emit msgChanged( folder(), serNum, deltaUnread );
  }
}

//-----------------------------------------------------------------------------
void FolderStorage::headerOfMsgChanged(const KMMsgBase* aMsg, int idx)
{
  if (idx < 0)
    idx = aMsg->parent()->find( aMsg );
  if (idx >= 0 && !mQuiet)
    emit msgHeaderChanged(folder(), idx);
  else
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
  mCompactable = config->readBoolEntry("Compactable", TRUE);

  int type = config->readNumEntry( "ContentsType", 0 );
  if ( type < 0 || type > KMail::ContentsTypeLast ) type = 0;
  setContentsType( static_cast<KMail::FolderContentsType>( type ) );
  mContentsTypeChanged = false;

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

  // Write the KMFolder parts
  if( folder() ) folder()->writeConfig( config );
}

//-----------------------------------------------------------------------------
void FolderStorage::correctUnreadMsgsCount()
{
  open();
  close();
  emit numUnreadMsgsChanged( folder() );
}

//-----------------------------------------------------------------------------
void FolderStorage::fillMsgDict(KMMsgDict *dict)
{
  fillDictFromIndex(dict);
}

//-----------------------------------------------------------------------------
int FolderStorage::writeMsgDict(KMMsgDict *dict)
{
  int ret = 0;
  if (!dict)
    dict = kmkernel->msgDict();
  if (dict)
    ret = dict->writeFolderIds(folder());
  return ret;
}

//-----------------------------------------------------------------------------
int FolderStorage::touchMsgDict()
{
  int ret = 0;
  KMMsgDict *dict = kmkernel->msgDict();
  if (dict)
    ret = dict->touchFolderIds(folder());
  return ret;
}

//-----------------------------------------------------------------------------
int FolderStorage::appendtoMsgDict(int idx)
{
  int ret = 0;
  KMMsgDict *dict = kmkernel->msgDict();
  if (dict) {
    if (count() == 1) {
      ret = dict->writeFolderIds(folder());
    } else {
      ret = dict->appendtoFolderIds(folder(), idx);
    }
  }
  return ret;
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

void FolderStorage::setRDict(KMMsgDictREntry *rentry) {
  if (rentry == mRDict)
	return;
  KMMsgDict::deleteRentry(mRDict);
  mRDict = rentry;
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
size_t FolderStorage::crlf2lf( char* str, const size_t strLen )
{
  if ( !str || strLen == 0 ) return 0;

  const char* source = str;
  const char* sourceEnd = source + strLen;

  // search the first occurrence of "\r\n"
  for ( ; source < sourceEnd - 1; ++source ) {
    if ( *source == '\r' && *( source + 1 ) == '\n' )
      break;
  }

  if ( source == sourceEnd - 1 ) {
    // no "\r\n" found
    return strLen;
  }

  // replace all occurrences of "\r\n" with "\n" (in place)
  char* target = const_cast<char*>( source ); // target points to '\r'
  ++source; // source points to '\n'
  for ( ; source < sourceEnd; ++source ) {
    if ( *source != '\r' || *( source + 1 ) != '\n' )
      *target++ = *source;
  }
  *target = '\0'; // terminate result
  return target - str;
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
void FolderStorage::setContentsType( KMail::FolderContentsType type )
{
  if ( type != mContentsType ) {
    mContentsType = type;
    kmkernel->iCalIface().folderContentsTypeChanged( folder(), type );
  }
}

//-----------------------------------------------------------------------------
void FolderStorage::search( KMSearchPattern* pattern )
{
  mSearchPattern = pattern;
  mCurrentSearchedMsg = 0;
  slotProcessNextSearchBatch();
}

void FolderStorage::slotProcessNextSearchBatch()
{
  QValueList<Q_UINT32> matchingSerNums;
  int end = ( count() - mCurrentSearchedMsg > 100 ) ? 100+mCurrentSearchedMsg : count();
  for ( int i = mCurrentSearchedMsg; i < end; ++i )
  {
    Q_UINT32 serNum = kmkernel->msgDict()->getMsgSerNum( folder(), i );
    if ( mSearchPattern->matches( serNum ) )
      matchingSerNums.append( serNum );
  }
  mCurrentSearchedMsg = end;
  bool complete = ( end == count() ) ? true : false;
  emit searchResult( folder(), matchingSerNums, mSearchPattern, complete );
  if ( !complete )
    QTimer::singleShot( 0, this, SLOT(slotProcessNextSearchBatch()) );
}

//-----------------------------------------------------------------------------
void FolderStorage::search( KMSearchPattern* pattern, Q_UINT32 serNum )
{
  Q_UINT32 mySerNum = 0;
  if ( pattern->matches( serNum ) )
    mySerNum = serNum;

  emit searchDone( folder(), mySerNum, pattern );
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

#include "folderstorage.moc"
