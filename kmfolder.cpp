// kmfolder.cpp
// Author: Stefan Taferner <taferner@alpin.or.at>

#include <config.h>
#include "kmmessage.h"
#include "kmfolderdir.h"
#include "kmfolderindex.h"
#include "kmfolderimap.h"
#include "kmfolderdia.h"
#include "kmundostack.h"
#include "kmmsgdict.h"
#include "kmkernel.h"
#include "identitymanager.h"
#include "kmidentity.h"
#include "kmfoldermgr.h"

#include <mimelib/mimepp.h>
#include <qregexp.h>
#include <qtimer.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kcursor.h>
#include <errno.h>
#include <klocale.h>
#include <kapplication.h>

//-----------------------------------------------------------------------------

KMFolder :: KMFolder(KMFolderDir* aParent, const QString& aName) :
  KMFolderInherited(aParent, aName)
{
  mOpenCount      = 0;
  mQuiet          = 0;
  mChanged        = FALSE;
  mAutoCreateIndex= TRUE;
  mIsSystemFolder = FALSE;
  mType           = "plain";
  mAcctList       = 0;
  mDirty          = FALSE;
  mUnreadMsgs      = -1;
  mGuessedUnreadMsgs = -1;
  mTotalMsgs      = -1;
  needsCompact    = FALSE;
  mChild          = 0;
  mConvertToUtf8  = FALSE;
  mMailingListEnabled = FALSE;
  mCompactable     = TRUE;
  mNoContent      = FALSE;
  expireMessages = FALSE;
  unreadExpireAge = 28;
  unreadExpireUnits = expireNever;
  readExpireAge = 14;
  readExpireUnits = expireNever;
  mRDict = 0;
  mUseCustomIcons = false;
  mDirtyTimer = new QTimer();
  connect(mDirtyTimer, SIGNAL(timeout()),
	  this, SLOT(updateIndex()));

  if ( aParent ) {
      connect(this, SIGNAL(msgAdded(KMFolder*, Q_UINT32)),
              parent()->manager(), SIGNAL(msgAdded(KMFolder*, Q_UINT32)));
      connect(this, SIGNAL(msgRemoved(KMFolder*, Q_UINT32)),
              parent()->manager(), SIGNAL(msgRemoved(KMFolder*, Q_UINT32)));
      connect(this, SIGNAL(msgChanged(KMFolder*, Q_UINT32, int)),
              parent()->manager(), SIGNAL(msgChanged(KMFolder*, Q_UINT32, int)));
  }
  //FIXME: Centralize all the readConfig calls somehow - Zack
  readConfig();
}


//-----------------------------------------------------------------------------
KMFolder :: ~KMFolder()
{
  delete mAcctList;
  mJobList.setAutoDelete( true );
  QObject::disconnect( SIGNAL(destroyed(QObject*)), this, 0 );
  mJobList.clear();
  KMMsgDict::deleteRentry(mRDict);
}


//-----------------------------------------------------------------------------
QString KMFolder::dotEscape(const QString& aStr) const
{
  if (aStr[0] != '.') return aStr;
  return aStr.left(aStr.find(QRegExp("[^\\.]"))) + aStr;
}

void KMFolder::addJob( FolderJob* job ) const
{
  QObject::connect( job, SIGNAL(destroyed(QObject*)),
                    SLOT(removeJob(QObject*)) );
  mJobList.append( job );
}

void KMFolder::removeJob( QObject* job )
{
  mJobList.remove( static_cast<FolderJob*>( job ) );
}


//-----------------------------------------------------------------------------
QString KMFolder::location() const
{
  QString sLocation(path());

  if (!sLocation.isEmpty()) sLocation += '/';
  sLocation += dotEscape(fileName());

  return sLocation;
}



//-----------------------------------------------------------------------------
QString KMFolder::subdirLocation() const
{
  QString sLocation(path());

  if (!sLocation.isEmpty()) sLocation += '/';
  sLocation += '.';
  sLocation += dotEscape(fileName());
  sLocation += ".directory";

  return sLocation;
}

//-----------------------------------------------------------------------------
KMFolderDir* KMFolder::createChildFolder()
{
  QString childName = "." + fileName() + ".directory";
  QString childDir = path() + "/" + childName;
  bool ok = true;

  if (mChild)
    return mChild;

  if (access(childDir.local8Bit(), W_OK) != 0) // Not there or not writable
  {
    if (mkdir(childDir.local8Bit(), S_IRWXU) != 0
      && chmod(childDir.local8Bit(), S_IRWXU) != 0)
        ok=false; //failed create new or chmod existing tmp/
  }

  if (!ok) {
    QString wmsg = QString(" '%1': %2").arg(childDir).arg(strerror(errno));
    KMessageBox::information(0,i18n("Failed to create directory") + wmsg);
    return 0;
  }

  KMFolderDir* folderDir = new KMFolderDir(parent(), childName,
    (protocol() == "imap") ? KMImapDir : KMStandardDir);
  if (!folderDir)
    return 0;
  folderDir->reload();
  parent()->append(folderDir);
  mChild = folderDir;
  return folderDir;
}

//-----------------------------------------------------------------------------
void KMFolder::setAutoCreateIndex(bool autoIndex)
{
  mAutoCreateIndex = autoIndex;
}

//-----------------------------------------------------------------------------
void KMFolder::setDirty(bool f)
{
  mDirty = f;
  if (mDirty  && mAutoCreateIndex)
    mDirtyTimer->changeInterval( mDirtyTimerInterval );
  else
    mDirtyTimer->stop();
}

//-----------------------------------------------------------------------------
void KMFolder::setIdentity( uint identity ) {
  mIdentity = identity;
  kernel->slotRequestConfigSync();
}


//-----------------------------------------------------------------------------
void KMFolder::markNewAsUnread()
{
  KMMsgBase* msgBase;
  int i;

  for (i=0; i< count(); ++i)
  {
    if (!(msgBase = getMsgBase(i))) continue;
    if (msgBase->status() == KMMsgStatusNew)
    {
      msgBase->setStatus(KMMsgStatusUnread);
      msgBase->setDirty(TRUE);
    }
  }
}

void KMFolder::markUnreadAsRead()
{
  const KMMsgBase* msgBase;
  QValueList<int> items;

  for (int i=count()-1; i>=0; --i)
  {
    msgBase = getMsgBase(i);
    assert(msgBase);
    if (msgBase->status() == KMMsgStatusNew || msgBase->status() == KMMsgStatusUnread)
    {
      items += i;
    }
  }

  if (items.count() > 0)
    setStatus(items, KMMsgStatusRead);
}

//-----------------------------------------------------------------------------
void KMFolder::quiet(bool beQuiet)
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

// Needed to use QSortedList in reduceSize()

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
int KMFolder::expungeOldMsg(int days)
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
/**
 * Return the number of days given some value, and the units for that
 * value. Currently, supported units are days, weeks and months.
 */
int
KMFolder::daysToExpire(int number, ExpireUnits units) {
  switch (units) {
  case expireDays: // Days
    return number;
  case expireWeeks: // Weeks
    return number * 7;
  case expireMonths: // Months - this could be better rather than assuming 31day months.
    return number * 31;
  default: // this avoids a compiler warning (not handled enumeration values)
    ;
  }

  return -1;

}

//-----------------------------------------------------------------------------
/**
 * Expire old messages from this folder. Read and unread messages have
 * different expiry times. An expiry time of 0 or less is considered to
 * mean no-expiry. Also check the general 'expire' flag as well.
 */
void KMFolder::expireOldMessages() {
  FolderJob *job = createJob( 0, FolderJob::tExpireMessages );
  job->start();
}


//-----------------------------------------------------------------------------
void KMFolder::emitMsgAddedSignals(int idx)
{
  if (!mQuiet) {
    Q_UINT32 serNum = kernel->msgDict()->getMsgSerNum(this, idx);
    emit msgAdded(idx);
    emit msgAdded(this, serNum);
  } else
    mChanged = TRUE;
}

//-----------------------------------------------------------------------------
bool KMFolder::canAddMsgNow(KMMessage* aMsg, int* aIndex_ret)
{
  if (aIndex_ret) *aIndex_ret = -1;
  KMFolder *msgParent = aMsg->parent();
  if (aMsg->transferInProgress())
      return false;
  #warning "FIXME : extract tempOpenFolder to some base class"
  if (msgParent  && msgParent->protocol() == "imap" &&!aMsg->isComplete())
  {
    FolderJob *job = msgParent->createJob(aMsg);
    connect(job, SIGNAL(messageRetrieved(KMMessage*)),
            SLOT(reallyAddMsg(KMMessage*)));
    job->start();
    aMsg->setTransferInProgress(TRUE);
    //FIXME: remove that. Maybe extract tempOpenFolder to some base class
    static_cast<KMFolderImap*>(msgParent)->account()->tempOpenFolder(this);
    return FALSE;
  }
  return TRUE;
}


//-----------------------------------------------------------------------------
void KMFolder::reallyAddMsg(KMMessage* aMsg)
{
  aMsg->setTransferInProgress(FALSE);
  KMFolder *folder = aMsg->parent();
  int index;
  ulong serNum = aMsg->getMsgSerNum();
  bool undo = aMsg->enableUndo();
  addMsg(aMsg, &index);
  if (index < 0) return;
  unGetMsg(index);
  if (undo)
  {
    kernel->undoStack()->pushAction( serNum, folder, this );
  }
}


//-----------------------------------------------------------------------------
void KMFolder::reallyAddCopyOfMsg(KMMessage* aMsg)
{
  aMsg->setParent( 0 );
  addMsg( aMsg );
  unGetMsg( count() - 1 );
}


//-----------------------------------------------------------------------------
void KMFolder::removeMsg(QPtrList<KMMessage> msgList, bool imapQuiet)
{
  for ( KMMessage* msg = msgList.first(); msg; msg = msgList.next() )
  {
    int idx = find(msg);
    assert( idx != -1);
    removeMsg(idx, imapQuiet);
  }
}

//-----------------------------------------------------------------------------
void KMFolder::removeMsg(int idx, bool)
{
  //assert(idx>=0);
  if(idx < 0)
  {
    kdDebug(5006) << "KMFolder::removeMsg() : idx < 0\n" << endl;
    return;
  }

  KMMsgBase* mb = getMsgBase(idx);
  QString msgIdMD5 = mb->msgIdMD5();
  Q_UINT32 serNum = kernel->msgDict()->getMsgSerNum(this, idx);
  if (!mQuiet)
    emit msgRemoved(this, serNum);
  mb = takeIndexEntry( idx );

  setDirty( true );
  needsCompact=true; // message is taken from here - needs to be compacted

  if (mb->status()==KMMsgStatusUnread ||
      mb->status()==KMMsgStatusNew ||
      (this == kernel->outboxFolder())) {
    --mUnreadMsgs;
    emit numUnreadMsgsChanged( this );
  }
  --mTotalMsgs;

  if (!mQuiet) {
    emit msgRemoved(idx, msgIdMD5);
    emit msgRemoved(this);
  } else {
    mChanged = TRUE;
  }
}


//-----------------------------------------------------------------------------
KMMessage* KMFolder::take(int idx)
{
  KMMsgBase* mb;
  KMMessage* msg;

  assert(idx>=0 && idx<=count());

  mb = getMsgBase(idx);
  if (!mb) return 0;
  if (!mb->isMessage()) readMsg(idx);
  Q_UINT32 serNum = kernel->msgDict()->getMsgSerNum(this, idx);
  if (!mQuiet)
    emit msgRemoved(this,serNum);

  msg = (KMMessage*)takeIndexEntry(idx);
  QString msgIdMD5 = msg->msgIdMD5();
  if (msg->status()==KMMsgStatusUnread ||
      msg->status()==KMMsgStatusNew ||
      (this == kernel->outboxFolder())) {
    --mUnreadMsgs;
    emit numUnreadMsgsChanged( this );
  }
  --mTotalMsgs;
  msg->setParent(0);
  setDirty( true );
  needsCompact=true; // message is taken from here - needs to be compacted
  if (!mQuiet) {
    emit msgRemoved(idx, msgIdMD5);
    emit msgRemoved(this);
  } else {
    mChanged = TRUE;
  }

  return msg;
}

void KMFolder::take(QPtrList<KMMessage> msgList)
{
  for ( KMMessage* msg = msgList.first(); msg; msg = msgList.next() )
  {
    if (msg->parent())
    {
      int idx = msg->parent()->find(msg);
      assert( idx != -1);
      KMFolder::take(idx);
    }
  }
}


//-----------------------------------------------------------------------------
KMMessage* KMFolder::getMsg(int idx)
{
  KMMsgBase* mb;

  if(!(idx >= 0 && idx <= count()))
    return 0;

  mb = getMsgBase(idx);
  if (!mb) return 0;

#if 0
  if (mb->isMessage()) return ((KMMessage*)mb);
  return readMsg(idx);
#else
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

  if (msg->getMsgSerNum() == 0) {
    msg->setMsgSerNum(kernel->msgDict()->insert(0, msg, idx));
    kdDebug(5006) << "Serial number generated for message in folder " << label() << endl;
  }
  msg->setComplete( true );
  return msg;
#endif


}


//-----------------------------------------------------------------------------
KMMsgInfo* KMFolder::unGetMsg(int idx)
{
  KMMsgBase* mb;

  if(!(idx >= 0 && idx <= count()))
    return 0;

  mb = getMsgBase(idx);
  if (!mb) return 0;

  if (mb->isMessage())
    return setIndexEntry( idx, (KMMessage*)mb );

  return 0;
}


//-----------------------------------------------------------------------------
bool KMFolder::isMessage(int idx)
{
  KMMsgBase* mb;
  if (!(idx >= 0 && idx <= count())) return FALSE;
  mb = getMsgBase(idx);
  return (mb && mb->isMessage());
}

//-----------------------------------------------------------------------------
FolderJob* KMFolder::createJob( KMMessage *msg, FolderJob::JobType jt,
                                KMFolder *folder ) const
{
  FolderJob * job = doCreateJob( msg, jt, folder );
  if ( job )
    addJob( job );
  return job;
}

//-----------------------------------------------------------------------------
FolderJob* KMFolder::createJob( QPtrList<KMMessage>& msgList, const QString& sets,
                                FolderJob::JobType jt, KMFolder *folder ) const
{
  FolderJob * job = doCreateJob( msgList, sets, jt, folder );
  if ( job )
    addJob( job );
  return job;
}


//-----------------------------------------------------------------------------
int KMFolder::moveMsg(KMMessage* aMsg, int* aIndex_ret)
{
  KMFolder* msgParent;
  int rc;

  assert(aMsg != 0);
  msgParent = aMsg->parent();

  if (msgParent)
    msgParent->open();

  open();
  rc = addMsg(aMsg, aIndex_ret);
  close();

  if (msgParent)
    msgParent->close();

  return rc;
}

//-----------------------------------------------------------------------------
int KMFolder::moveMsg(QPtrList<KMMessage> msglist, int* aIndex_ret)
{
  KMFolder* msgParent;
  int rc;

  KMMessage* aMsg = msglist.first();
  assert(aMsg != 0);
  msgParent = aMsg->parent();

  if (msgParent)
    msgParent->open();

  open();
  //FIXME : is it always imap ?
  rc = static_cast<KMFolderImap*>(this)->addMsg(msglist, aIndex_ret); //yuck: Don
  close();

  if (msgParent)
    msgParent->close();

  return rc;
}


//-----------------------------------------------------------------------------
int KMFolder::rename(const QString& newName, KMFolderDir *newParent)
{
  QString oldLoc, oldIndexLoc, oldIdsLoc, newLoc, newIndexLoc, newIdsLoc;
  QString oldSubDirLoc, newSubDirLoc;
  QString oldName;
  int rc=0, openCount=mOpenCount;
  KMFolderDir *oldParent;

  assert(!newName.isEmpty());

  oldLoc = location();
  oldIndexLoc = indexLocation();
  oldSubDirLoc = subdirLocation();
  if (kernel->msgDict())
    oldIdsLoc = kernel->msgDict()->getFolderIdsLocation(this);

  close(TRUE);

  oldName = fileName();
  oldParent = parent();
  if (newParent)
    setParent( newParent );

  setName(newName);
  newLoc = location();
  newIndexLoc = indexLocation();
  newSubDirLoc = subdirLocation();
  if (kernel->msgDict())
    newIdsLoc = kernel->msgDict()->getFolderIdsLocation(this);

  if (::rename(oldLoc.local8Bit(), newLoc.local8Bit())) {
    setName(oldName);
    setParent(oldParent);
    rc = errno;
  }
  else {
    // rename/move index file and index.sorted file
    if (!oldIndexLoc.isEmpty()) {
      ::rename(oldIndexLoc.local8Bit(), newIndexLoc.local8Bit());
      ::rename(oldIndexLoc.local8Bit() + ".sorted",
               newIndexLoc.local8Bit() + ".sorted");
    }

    // rename/move serial number file
    if (!oldIdsLoc.isEmpty())
      ::rename(oldIdsLoc.local8Bit(), newIdsLoc.local8Bit());

    // rename/move the subfolder directory
    if (!::rename(oldSubDirLoc.local8Bit(), newSubDirLoc.local8Bit() )) {
      // now that the subfolder directory has been renamed and/or moved also
      // change the name that is stored in the corresponding KMFolderNode
      // (provide that the name actually changed)
      if( mChild && ( oldName != newName ) ) {
        mChild->setName( "." + newName.local8Bit() + ".directory" );
      }
    }

    // if the folder is being moved then move its node and, if necessary, also
    // the associated subfolder directory node to the new parent
    if (newParent) {
      if (oldParent->findRef( this ) != -1)
        oldParent->take();
      newParent->inSort( this );
      if (mChild) {
        if (mChild->parent()->findRef( mChild ) != -1)
          mChild->parent()->take();
        newParent->inSort( mChild );
        mChild->setParent( newParent );
      }
    }
  }

  if (openCount > 0)
  {
    open();
    mOpenCount = openCount;
  }

  return rc;
}


//-----------------------------------------------------------------------------
int KMFolder::remove()
{
  assert(name() != "");

  clearIndex(true, true); // delete and remove from dict
  close(TRUE);

  if (kernel->msgDict()) kernel->msgDict()->removeFolderIds(this);
  unlink(indexLocation().local8Bit() + ".sorted");
  unlink(indexLocation().local8Bit());

  int rc = removeContents();
  if (rc) return rc;

  needsCompact = false; //we are dead - no need to compact us
  return 0;
}


//-----------------------------------------------------------------------------
int KMFolder::expunge()
{
  int openCount = mOpenCount;

  assert(name() != "");

  clearIndex(true, true);   // delete and remove from dict
  close(TRUE);

  kernel->msgDict()->removeFolderIds(this);
  if (mAutoCreateIndex)
    truncateIndex();
  else unlink(indexLocation().local8Bit());

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
  emit numUnreadMsgsChanged( this );
  if (mAutoCreateIndex)
    writeConfig();
  if (!mQuiet)
    emit changed();
  else
    mChanged = TRUE;
  return 0;
}


//-----------------------------------------------------------------------------
const char* KMFolder::type() const
{
  if (mAcctList) return "In";
  return KMFolderInherited::type();
}


//-----------------------------------------------------------------------------
QString KMFolder::label() const
{
  if (mIsSystemFolder && !mLabel.isEmpty()) return mLabel;
  if (mIsSystemFolder) return i18n(name().latin1());
  return name();
}

int KMFolder::count(bool cache) const
{
  if (cache && mTotalMsgs != -1)
    return mTotalMsgs;
  else
    return -1;
}

//-----------------------------------------------------------------------------
int KMFolder::countUnread()
{
  if (mGuessedUnreadMsgs > -1)
    return mGuessedUnreadMsgs;
  if (mUnreadMsgs > -1)
    return mUnreadMsgs;

  readConfig();

  if (mUnreadMsgs > -1)
    return mUnreadMsgs;

  open(); // will update unreadMsgs
  close();
  return (mUnreadMsgs > 0) ? mUnreadMsgs : 0;
}

//-----------------------------------------------------------------------------
int KMFolder::countUnreadRecursive()
{
  KMFolder *folder;
  int count = countUnread();
  KMFolderDir *dir = child();
  if (!dir)
    return count;

  QPtrListIterator<KMFolderNode> it(*dir);
  for ( ; it.current(); ++it )
    if (!it.current()->isDir()) {
      folder = static_cast<KMFolder*>(it.current());
      count += folder->countUnreadRecursive();
    }

  return count;
}

//-----------------------------------------------------------------------------
void KMFolder::msgStatusChanged(const KMMsgStatus oldStatus,
  const KMMsgStatus newStatus, int idx)
{
  int oldUnread = 0;
  int newUnread = 0;

  if (oldStatus==KMMsgStatusUnread || oldStatus==KMMsgStatusNew ||
      (this == kernel->outboxFolder()))
    oldUnread = 1;
  if (newStatus==KMMsgStatusUnread || newStatus==KMMsgStatusNew ||
      (this == kernel->outboxFolder()))
    newUnread = 1;
  int deltaUnread = newUnread - oldUnread;

  mDirtyTimer->changeInterval(mDirtyTimerInterval);
  if (deltaUnread != 0) {
    if (mUnreadMsgs < 0) mUnreadMsgs = 0;
    mUnreadMsgs += deltaUnread;
    emit numUnreadMsgsChanged( this );

    Q_UINT32 serNum = kernel->msgDict()->getMsgSerNum(this, idx);
    emit msgChanged( this, serNum, deltaUnread );
  }
}

//-----------------------------------------------------------------------------
void KMFolder::headerOfMsgChanged(const KMMsgBase* aMsg, int idx)
{
  if (mQuiet)
  {
    mChanged = TRUE;
    return;
  }

  if (idx < 0)
    idx = aMsg->parent()->find( aMsg );
  if (idx >= 0)
    emit msgHeaderChanged(idx);
   else
     mChanged = TRUE;
}

//-----------------------------------------------------------------------------
QString KMFolder::idString() const
{
  KMFolderNode* folderNode = parent();
  if (!folderNode)
    return "";
  while (folderNode->parent())
    folderNode = folderNode->parent();
  int pathLen = path().length() - folderNode->path().length();
  QString relativePath = path().right( pathLen );
  if (!relativePath.isEmpty())
    relativePath = relativePath.right( relativePath.length() - 1 ) + "/";
  return relativePath + QString(name());
}

//-----------------------------------------------------------------------------
void KMFolder::readConfig()
{
  //kdDebug(5006)<<"#### READING CONFIG  = "<< name() <<endl;
  KConfig* config = KMKernel::config();
  KConfigGroupSaver saver(config, "Folder-" + idString());
  if (mUnreadMsgs == -1)
    mUnreadMsgs = config->readNumEntry("UnreadMsgs", -1);
  if (mTotalMsgs == -1)
    mTotalMsgs = config->readNumEntry("TotalMsgs", -1);
  mMailingListEnabled = config->readBoolEntry("MailingListEnabled");
  mMailingListPostingAddress = config->readEntry("MailingListPostingAddress");
  mMailingListAdminAddress = config->readEntry("MailingListAdminAddress");
  mIdentity = config->readUnsignedNumEntry("Identity",0);
  mCompactable = config->readBoolEntry("Compactable", TRUE);

  expireMessages = config->readBoolEntry("ExpireMessages", FALSE);
  readExpireAge = config->readNumEntry("ReadExpireAge", 3);
  readExpireUnits = (ExpireUnits)config->readNumEntry("ReadExpireUnits", expireMonths);
  unreadExpireAge = config->readNumEntry("UnreadExpireAge", 12);
  unreadExpireUnits = (ExpireUnits)config->readNumEntry("UnreadExpireUnits", expireNever);
  setUserWhoField( config->readEntry("WhoField"), false );
  mUseCustomIcons = config->readBoolEntry("UseCustomIcons", false );
  mNormalIconPath = config->readEntry("NormalIconPath" );
  mUnreadIconPath = config->readEntry("UnreadIconPath" );
  if ( mUseCustomIcons )
      emit iconsChanged();
}

//-----------------------------------------------------------------------------
void KMFolder::writeConfig()
{
  KConfig* config = KMKernel::config();
  KConfigGroupSaver saver(config, "Folder-" + idString());
  config->writeEntry("UnreadMsgs", mUnreadMsgs);
  config->writeEntry("TotalMsgs", mTotalMsgs);
  config->writeEntry("MailingListEnabled", mMailingListEnabled);
  config->writeEntry("MailingListPostingAddress", mMailingListPostingAddress);
  config->writeEntry("MailingListAdminAddress", mMailingListAdminAddress);
  config->writeEntry("Identity", mIdentity);
  config->writeEntry("Compactable", mCompactable);
  config->writeEntry("ExpireMessages", expireMessages);
  config->writeEntry("ReadExpireAge", readExpireAge);
  config->writeEntry("ReadExpireUnits", readExpireUnits);
  config->writeEntry("UnreadExpireAge", unreadExpireAge);
  config->writeEntry("UnreadExpireUnits", unreadExpireUnits);
  config->writeEntry("WhoField", mUserWhoField);

  config->writeEntry("UseCustomIcons", mUseCustomIcons);
  config->writeEntry("NormalIconPath", mNormalIconPath);
  config->writeEntry("UnreadIconPath", mUnreadIconPath);
}

//-----------------------------------------------------------------------------
void KMFolder::correctUnreadMsgsCount()
{
  open();
  close();
  emit numUnreadMsgsChanged( this );
}

//-----------------------------------------------------------------------------
void KMFolder::fillMsgDict(KMMsgDict *dict)
{
  fillDictFromIndex(dict);
}

//-----------------------------------------------------------------------------
int KMFolder::writeMsgDict(KMMsgDict *dict)
{
  int ret = 0;
  if (!dict)
    dict = kernel->msgDict();
  if (dict)
    ret = dict->writeFolderIds(this);
  return ret;
}

//-----------------------------------------------------------------------------
int KMFolder::touchMsgDict()
{
  int ret = 0;
  KMMsgDict *dict = kernel->msgDict();
  if (dict)
    ret = dict->touchFolderIds(this);
  return ret;
}

//-----------------------------------------------------------------------------
int KMFolder::appendtoMsgDict(int idx)
{
  int ret = 0;
  KMMsgDict *dict = kernel->msgDict();
  if (dict)
    ret = dict->appendtoFolderIds(this, idx);
  return ret;
}

//-----------------------------------------------------------------------------
void KMFolder::setStatus(int idx, KMMsgStatus status)
{
  KMMsgBase *msg = getMsgBase(idx);
  msg->setStatus(status, idx);
}

void KMFolder::setRDict(KMMsgDictREntry *rentry) {
  if (rentry == mRDict)
	return;
  KMMsgDict::deleteRentry(mRDict);
  mRDict = rentry;
}

//-----------------------------------------------------------------------------
void KMFolder::setStatus(QValueList<int>& ids, KMMsgStatus status)
{
  for ( QValueList<int>::Iterator it = ids.begin(); it != ids.end(); ++it )
  {
    KMFolder::setStatus(*it, status);
  }
}

//-----------------------------------------------------------------------------
void KMFolder::setUserWhoField(const QString &whoField, bool aWriteConfig)
{
  mUserWhoField = whoField;
  if ( whoField.isEmpty() )
  {
    // default setting
    const KMIdentity & identity =
      kernel->identityManager()->identityForUoidOrDefault( mIdentity );

    if ( mIsSystemFolder && protocol() != "imap" )
    {
      // local system folders
      if ( this == kernel->inboxFolder() || this == kernel->trashFolder() ) mWhoField = "From";
      if ( this == kernel->outboxFolder() || this == kernel->sentFolder() || this == kernel->draftsFolder() ) mWhoField = "To";

    } else if ( identity.drafts() == idString() || identity.fcc() == idString() ) {
      // drafts or sent of the identity
      mWhoField = "To";
    } else {
      mWhoField = "From";
    }

  } else if ( whoField == "From" || whoField == "To" ) {

    // set the whoField according to the user-setting
    mWhoField = whoField;

  } else {
    // this should not happen...
    kdDebug(5006) << "Illegal setting " << whoField << " for userWhoField!" << endl;
  }

  if (aWriteConfig)
    writeConfig();
}

void KMFolder::ignoreJobsForMessage( KMMessage *msg )
{
  if ( !msg || msg->transferInProgress() )
    return;

  for( QPtrListIterator<FolderJob> it( mJobList ); it.current(); ++it ) {
    //FIXME: the questions is : should we iterate through all
    //messages in jobs? I don't think so, because it would
    //mean canceling the jobs that work with other messages
    if ( it.current()->msgList().first() == msg) {
      mJobList.remove( it.current() );
      delete it.current();
      break;
    }
  }
}

void KMFolder::setIconPaths(const QString &normalPath, const QString &unreadPath)
{
  mNormalIconPath = normalPath;
  mUnreadIconPath = unreadPath;
  writeConfig();
  emit iconsChanged();
}

#include "kmfolder.moc"
