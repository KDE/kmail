/**
 * kmfolderimap.cpp
 *
 * Copyright (c) 2001 Kurt Granroth <granroth@kde.org>
 * Copyright (c) 2000-2002 Michael Haeckel <haeckel@kde.org>
 *
 * This file is based on kmacctimap.coo by Michael Haeckel which was
 * based on kmacctexppop.cpp by Don Sanders
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
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmfolderimap.h"
#include "kmfoldertree.h"
#include "undostack.h"
#include "kmfoldermgr.h"
#include "imapjob.h"
using KMail::ImapJob;

#include <kdebug.h>
#include <kio/scheduler.h>
#include <kconfig.h>

#include <qbuffer.h>
#include <qtextcodec.h>

#include <assert.h>

KMFolderImap::KMFolderImap(KMFolderDir* aParent, const QString& aName)
  : KMFolderImapInherited(aParent, aName)
{
  mContentState = imapNoInformation;
  mSubfolderState = imapNoInformation;
  mAccount = 0;
  mIsSelected = FALSE;
  mLastUid = 0;
  mCheckFlags = TRUE;
  mCheckMail = TRUE;
  mCheckingValidity = FALSE;

  KConfig* config = KMKernel::config();
  KConfigGroupSaver saver(config, "Folder-" + idString());
  mUidValidity = config->readEntry("UidValidity");
  if (mImapPath.isEmpty()) mImapPath = config->readEntry("ImapPath");
  if (aName == "INBOX" && mImapPath == "/INBOX/")
  {
    mIsSystemFolder = TRUE;
    mLabel = i18n("inbox");
  }
  mNoContent = config->readBoolEntry("NoContent", FALSE);
  mReadOnly = config->readBoolEntry("ReadOnly", FALSE);

  readConfig();
}

KMFolderImap::~KMFolderImap()
{
  writeConfig();
  if (kernel->undoStack()) kernel->undoStack()->folderDestroyed(this);
}

//-----------------------------------------------------------------------------
void KMFolderImap::close(bool aForced)
{
  if (mOpenCount <= 0 ) return;
  if (mOpenCount > 0) mOpenCount--;
  if (mOpenCount > 0 && !aForced) return;
  if (mAccount)
    mAccount->ignoreJobsForFolder( this );
  int idx = count();
  while (--idx >= 0) {
    if ( mMsgList[idx]->isMessage() ) {
      KMMessage *msg = static_cast<KMMessage*>(mMsgList[idx]);
      if (msg->transferInProgress())
          msg->setTransferInProgress( false );
    }
  }
  // The inherited close will decrement again, so we have to adjust.
  mOpenCount++;
  KMFolderImapInherited::close(aForced);
}

//-----------------------------------------------------------------------------
KMMessage* KMFolderImap::getMsg(int idx)
{
  KMMessage* msg = KMFolder::getMsg( idx );
  if ( msg )
    msg->setComplete( false );
  return msg;
}

//-----------------------------------------------------------------------------
void KMFolderImap::setAccount(KMAcctImap *aAccount)
{
  mAccount = aAccount;
  if (!mChild) return;
  KMFolderNode* node;
  for (node = mChild->first(); node; node = mChild->next())
  {
    if (!node->isDir())
      static_cast<KMFolderImap*>(node)->setAccount(aAccount);
  }
}

//-----------------------------------------------------------------------------
void KMFolderImap::readConfig()
{
  KConfig* config = KMKernel::config();
  KConfigGroupSaver saver(config, "Folder-" + idString());
  mCheckMail = config->readBoolEntry("checkmail", true);
}

//-----------------------------------------------------------------------------
void KMFolderImap::writeConfig()
{
  KConfig* config = KMKernel::config();
  KConfigGroupSaver saver(config, "Folder-" + idString());
  config->writeEntry("checkmail", mCheckMail);
  config->writeEntry("UidValidity", mUidValidity);
  config->writeEntry("ImapPath", mImapPath);
  config->writeEntry("NoContent", mNoContent);
  config->writeEntry("ReadOnly", mReadOnly);
  KMFolderImapInherited::writeConfig();
}

//-----------------------------------------------------------------------------
void KMFolderImap::removeOnServer()
{
  KURL url = mAccount->getUrl();
  url.setPath(imapPath());
  if (!mAccount->makeConnection()) return;
  KIO::SimpleJob *job = KIO::file_delete(url, FALSE);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), job);
  ImapAccountBase::jobData jd(url.url());
  mAccount->insertJob(job, jd);
  connect(job, SIGNAL(result(KIO::Job *)),
          this, SLOT(slotRemoveFolderResult(KIO::Job *)));
}

//-----------------------------------------------------------------------------
void KMFolderImap::slotRemoveFolderResult(KIO::Job *job)
{
  ImapAccountBase::JobIterator it = mAccount->findJob(job);
  if ( it == mAccount->jobsEnd() ) return;
  mAccount->removeJob(it);
  if (job->error())
  {
    mAccount->slotSlaveError( mAccount->slave(), job->error(),
        job->errorText() );
  } else {
    mAccount->displayProgress();
    kernel->imapFolderMgr()->remove(this);
  }
}

//-----------------------------------------------------------------------------
void KMFolderImap::removeMsg(int idx, bool quiet)
{
  if (idx < 0)
    return;

  if (!quiet)
  {
    KMMessage *msg = getMsg(idx);
    deleteMessage(msg);
  }

  mLastUid = 0;
  KMFolderImapInherited::removeMsg(idx);
}

void KMFolderImap::removeMsg(QPtrList<KMMessage> msgList, bool quiet)
{
  if (!quiet)
    deleteMessage(msgList);

  mLastUid = 0;
  KMFolderImapInherited::removeMsg(msgList);
}

//-----------------------------------------------------------------------------
int KMFolderImap::rename( const QString& newName, KMFolderDir */*aParent*/ )
{
  if ( newName == name() )
    return 0;

  QString path = imapPath();
  int i = path.findRev( '.' );
  path = path.left( i );
  path += "." + newName;
  KURL src( mAccount->getUrl() );
  src.setPath( imapPath() );
  KURL dst( mAccount->getUrl() );
  dst.setPath( path );
  KIO::SimpleJob *job = KIO::rename( src, dst, true );
  kdDebug(5006)<< "### Rename : " << src.prettyURL()
           << " |=> " << dst.prettyURL()
           << endl;
  KIO::Scheduler::assignJobToSlave( mAccount->slave(), job );
  connect( job, SIGNAL(result(KIO::Job*)),
           SLOT(slotRenameResult(KIO::Job*)) );
  setImapPath( path );
  return 0;
}

//-----------------------------------------------------------------------------
void KMFolderImap::slotRenameResult( KIO::Job *job )
{
   KIO::SimpleJob* sj = static_cast<KIO::SimpleJob*>(job);
  if ( job->error() ) {
    setImapPath( sj->url().path() );
    mAccount->slotSlaveError( mAccount->slave(), job->error(),
                              job->errorText() );
    return;
  }
  QString path = imapPath();
  int i = path.findRev( '.' );
  path = path.mid( ++i );
  path.remove( '/' );
  KMFolderImapInherited::rename( path );
  kernel->folderMgr()->contentsChanged();
}

//-----------------------------------------------------------------------------
void KMFolderImap::addMsgQuiet(KMMessage* aMsg)
{
  KMFolder *folder = aMsg->parent();
  if (folder) {
    kernel->undoStack()->pushSingleAction( aMsg->getMsgSerNum(), folder, this );
    int idx = folder->find( aMsg );
    if ( idx != -1 )
      folder->take( idx );
  }
  aMsg->setTransferInProgress( false );
  // Remember the status, so it can be transfered to the new message.
  mMetaDataMap.insert(aMsg->msgIdMD5(), new KMMsgMetaData(aMsg->status()));

  delete aMsg;
  aMsg = 0;
  getFolder();
}

//-----------------------------------------------------------------------------
void KMFolderImap::addMsgQuiet(QPtrList<KMMessage> msgList)
{
  KMFolder *folder = msgList.first()->parent();
  int undoId = -1;
  for ( KMMessage* msg = msgList.first(); msg; msg = msgList.next() )
  {
    if ( undoId == -1 )
      undoId = kernel->undoStack()->newUndoAction( folder, this );
    kernel->undoStack()->addMsgToAction( undoId, msg->getMsgSerNum() );
    // Remember the status, so it can be transfered to the new message.
    mMetaDataMap.insert(msg->msgIdMD5(), new KMMsgMetaData(msg->status()));
  }
  if (folder) folder->take(msgList);
  msgList.setAutoDelete(true);
  msgList.clear();
  getFolder();
}

//-----------------------------------------------------------------------------
int KMFolderImap::addMsg(KMMessage* aMsg, int* aIndex_ret)
{
  QPtrList<KMMessage> list; list.append(aMsg);
  return addMsg(list, aIndex_ret);
}

int KMFolderImap::addMsg(QPtrList<KMMessage>& msgList, int* aIndex_ret)
{
  mAccount->tempOpenFolder(this);
  KMMessage *aMsg = msgList.getFirst();
  KMFolder *msgParent = aMsg->parent();

  ImapJob *imapJob = 0;
  if (msgParent)
  {
    mAccount->tempOpenFolder(msgParent);
    if (msgParent->folderType() == KMFolderTypeImap)
    {
      if (static_cast<KMFolderImap*>(msgParent)->account() == account())
      {
        // make sure the messages won't be deleted while we work with them
        for ( KMMessage* msg = msgList.first(); msg; msg = msgList.next() )
          msg->setTransferInProgress(true);

        if (this == msgParent)
        {
          // transfer the whole message, e.g. a draft-message is canceled and re-added to the drafts-folder
          for ( KMMessage* msg = msgList.first(); msg; msg = msgList.next() )
          {
            if (!msg->isComplete())
            {
              int idx = msgParent->find(msg);
              assert(idx != -1);
              msg = msgParent->getMsg(idx);
            }
            imapJob = new ImapJob(msg, ImapJob::tPutMessage, this);
            connect(imapJob, SIGNAL(messageStored(KMMessage*)),
                SLOT(addMsgQuiet(KMMessage*)));
            imapJob->start();
          }

        } else {

          // get the messages and the uids
          QValueList<int> uids;
          getUids(msgList, uids);

          // get the sets (do not sort the uids)
          QStringList sets = makeSets(uids, false);

          for ( QStringList::Iterator it = sets.begin(); it != sets.end(); ++it )
          {
            // we need the messages that belong to the current set to pass them to the ImapJob
            QPtrList<KMMessage> temp_msgs = splitMessageList(*it, msgList);

            imapJob = new ImapJob(temp_msgs, *it, ImapJob::tCopyMessage, this);
            connect(imapJob, SIGNAL(messageCopied(QPtrList<KMMessage>)),
                SLOT(addMsgQuiet(QPtrList<KMMessage>)));
            imapJob->start();
          }
        }
        if (aIndex_ret) *aIndex_ret = -1;
        return 0;
      }
      else
      {
        // different account, check if messages can be added
        QPtrListIterator<KMMessage> it( msgList );
        KMMessage *msg;
        while ( (msg = it.current()) != 0 )
        {
          ++it;
          if (!canAddMsgNow(msg, aIndex_ret))
            msgList.remove(msg);
          else {
            if (!msg->transferInProgress())
              msg->setTransferInProgress(true);
          }
        }
      }
    } // if imap
  }

  for ( KMMessage* msg = msgList.first(); msg; msg = msgList.next() )
  {
    // transfer from local folders or other accounts
    if (msgParent && !msg->isMessage())
    {
      int idx = msgParent->find(msg);
      assert(idx != -1);
      msg = msgParent->getMsg(idx);
    }
    if (!msg->transferInProgress())
      msg->setTransferInProgress(true);
    imapJob = new ImapJob(msg, ImapJob::tPutMessage, this);
    connect(imapJob, SIGNAL(messageStored(KMMessage*)),
        SLOT(addMsgQuiet(KMMessage*)));
    imapJob->start();
  }

  if (aIndex_ret) *aIndex_ret = -1;
  return 0;
}

//-----------------------------------------------------------------------------
void KMFolderImap::copyMsg(QPtrList<KMMessage>& msgList)
{
  mAccount->tempOpenFolder(this);
  KMMessage *aMsg = msgList.getFirst();
  if (aMsg)
  {
    KMFolder* parent = aMsg->parent();
    if (parent) mAccount->tempOpenFolder(parent);
  }
  for (KMMessage *msg = msgList.first(); msg; msg = msgList.next()) {
    // Remember the status, so it can be transfered to the new message.
    mMetaDataMap.insert(msg->msgIdMD5(), new KMMsgMetaData(msg->status()));
  }
  QValueList<int> uids;
  getUids(msgList, uids);
  QStringList sets = makeSets(uids, false);
  for ( QStringList::Iterator it = sets.begin(); it != sets.end(); ++it )
  {
    // we need the messages that belong to the current set to pass them to the ImapJob
    QPtrList<KMMessage> temp_msgs = splitMessageList(*it, msgList);

    ImapJob *job = new ImapJob(temp_msgs, *it, ImapJob::tCopyMessage, this);
    job->start();
  }
}

//-----------------------------------------------------------------------------
QPtrList<KMMessage> KMFolderImap::splitMessageList(QString set, QPtrList<KMMessage>& msgList)
{
  int lastcomma = set.findRev(",");
  int lastdub = set.findRev(":");
  int last = 0;
  if (lastdub > lastcomma) last = lastdub;
  else last = lastcomma;
  last++;
  if (last < 0) last = set.length();
  // the last uid of the current set
  QString last_uid = set.right(set.length() - last);
  QPtrList<KMMessage> temp_msgs;
  QString uid;
  if (!last_uid.isEmpty())
  {
    QPtrListIterator<KMMessage> it( msgList );
    KMMessage* msg = 0;
    while ( (msg = it.current()) != 0 )
    {
      // append the msg to the new list and delete it from the old
      temp_msgs.append(msg);
      uid = msg->headerField("X-UID");
      // remove modifies the current
      msgList.remove(msg);
      if (uid == last_uid) break;
    }
  }
  else
  {
    // probably only one element
    temp_msgs = msgList;
  }

  return temp_msgs;
}

//-----------------------------------------------------------------------------
KMMessage* KMFolderImap::take(int idx)
{
  KMMsgBase* mb(mMsgList[idx]);
  if (!mb) return 0;
  if (!mb->isMessage()) readMsg(idx);

  KMMessage *msg = static_cast<KMMessage*>(mb);
  deleteMessage(msg);

  mLastUid = 0;
  return KMFolderImapInherited::take(idx);
}

void KMFolderImap::take(QPtrList<KMMessage> msgList)
{
  deleteMessage(msgList);

  mLastUid = 0;
  KMFolderImapInherited::take(msgList);
}

//-----------------------------------------------------------------------------
bool KMFolderImap::listDirectory(bool secondStep)
{
  mSubfolderState = imapInProgress;
  if (!mAccount->makeConnection())
    return FALSE;

  // connect to folderlisting
  connect(mAccount, SIGNAL(receivedFolders(QStringList, QStringList,
          QStringList, const ImapAccountBase::jobData &)),
      this, SLOT(slotListResult(QStringList, QStringList,
          QStringList, const ImapAccountBase::jobData &)));

  // get the folders
  mAccount->listDirectory(mImapPath, mAccount->onlySubscribedFolders(),
      secondStep, this);

  return true;
}


//-----------------------------------------------------------------------------
void KMFolderImap::slotListResult( QStringList mSubfolderNames,
                                   QStringList mSubfolderPaths,
                                   QStringList mSubfolderMimeTypes,
                                   const ImapAccountBase::jobData & jobData )
{
  if (jobData.parent) {
    // the account is connected to several folders, so we
    // have to sort out if this result is for us
    if (jobData.parent != this) return;
  }
  // disconnect to avoid recursions
  disconnect(mAccount, SIGNAL(receivedFolders(QStringList, QStringList,
          QStringList, const ImapAccountBase::jobData &)),
      this, SLOT(slotListResult(QStringList, QStringList,
          QStringList, const ImapAccountBase::jobData &)));

  mSubfolderState = imapFinished;
  bool it_inboxOnly = jobData.inboxOnly;
  // don't react on changes
  kernel->imapFolderMgr()->quiet(TRUE);
  if (it_inboxOnly) {
    // list again only for the INBOX
    listDirectory(TRUE);
  } else {
    if (mIsSystemFolder && mImapPath == "/INBOX/"
        && mAccount->prefix() == "/INBOX/")
    {
      mAccount->setCreateInbox(FALSE);
      mSubfolderNames.clear();
    }
    createChildFolder();
    KMFolderImap *folder;
    KMFolderNode *node = mChild->first();
    while (node)
    {
      // check if the folders still exist on the server
      if (!node->isDir() && (node->name() != "INBOX" || !mAccount->createInbox())
          && mSubfolderNames.findIndex(node->name()) == -1)
      {
        kdDebug(5006) << node->name() << " disappeared." << endl;
        kernel->imapFolderMgr()->remove(static_cast<KMFolder*>(node));
        node = mChild->first();
      }
      else node = mChild->next();
    }
    if (mAccount->createInbox())
    {
      // INBOX-special
      for (node = mChild->first(); node; node = mChild->next())
        if (!node->isDir() && node->name() == "INBOX") break;
      if (node) folder = static_cast<KMFolderImap*>(node);
      else folder = static_cast<KMFolderImap*>
        (mChild->createFolder("INBOX", TRUE));
      folder->setAccount(mAccount);
      folder->setImapPath("/INBOX/");
      folder->setLabel(i18n("inbox"));
      if (!node) folder->close();
      folder->listDirectory();
      kernel->imapFolderMgr()->contentsChanged();
    }
    for (uint i = 0; i < mSubfolderNames.count(); i++)
    {
      // create folders if necessary
      for (node = mChild->first(); node; node = mChild->next())
        if (!node->isDir() && node->name() == mSubfolderNames[i]) break;
      if (node) folder = static_cast<KMFolderImap*>(node);
      else {
        folder = static_cast<KMFolderImap*>
          (mChild->createFolder(mSubfolderNames[i]));
        if (folder)
        {
          folder->close();
          kernel->imapFolderMgr()->contentsChanged();
        } else {
          kdDebug(5006) << "can't create folder " << mSubfolderNames[i] << endl;
        }
      }
      if (folder)
      {
        folder->setAccount(mAccount);
        folder->setNoContent(mSubfolderMimeTypes[i] == "inode/directory");
        folder->setImapPath(mSubfolderPaths[i]);
        if (mSubfolderMimeTypes[i] == "message/directory" ||
            mSubfolderMimeTypes[i] == "inode/directory")
          folder->listDirectory();
      }
    }
  }
  // now others should react on the changes
  kernel->imapFolderMgr()->quiet(FALSE);
}

//-----------------------------------------------------------------------------
void KMFolderImap::checkValidity()
{
  kdDebug(5006) << "KMFolderImap::checkValidity of: " << fileName() << endl;
  KURL url = mAccount->getUrl();
  url.setPath(imapPath() + ";UID=0:0");
  if (!mAccount->makeConnection())
  {
    emit folderComplete(this, FALSE);
    return;
  }
  // Only check once at a time.
  if (mCheckingValidity) return;
  mAccount->tempOpenFolder(this);
  ImapAccountBase::jobData jd( url.url(), this );
  KIO::SimpleJob *job = KIO::get(url, FALSE, FALSE);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), job);
  mAccount->insertJob(job, jd);
  connect(job, SIGNAL(result(KIO::Job *)),
          SLOT(slotCheckValidityResult(KIO::Job *)));
  connect(job, SIGNAL(data(KIO::Job *, const QByteArray &)),
          SLOT(slotSimpleData(KIO::Job *, const QByteArray &)));
  // Only check once at a time.
  mCheckingValidity = true;
}


//-----------------------------------------------------------------------------
ulong KMFolderImap::lastUid()
{
  if (mLastUid) return mLastUid;
  open();
  if (count() > 0)
  {
    bool unget = !isMessage(count() - 1);
    KMMessage *msg = getMsg(count() - 1);
    mLastUid = msg->headerField("X-UID").toULong();
    if (unget) unGetMsg(count() - 1);
  }
  close();
  return mLastUid;
}


//-----------------------------------------------------------------------------
void KMFolderImap::slotCheckValidityResult(KIO::Job * job)
{
  kdDebug(5006) << "KMFolderImap::slotCheckValidityResult of: " << fileName() << endl;
  mCheckingValidity = false;
  ImapAccountBase::JobIterator it = mAccount->findJob(job);
  if ( it == mAccount->jobsEnd() ) return;
  if (job->error()) {
    mAccount->slotSlaveError(mAccount->slave(), job->error(), job->errorText());
    emit folderComplete(this, FALSE);
  } else {
    QCString cstr((*it).data.data(), (*it).data.size() + 1);
    int a = cstr.find("X-uidValidity: ");
    int b = cstr.find("\r\n", a);
    QString uidv;
    if ( (b - a - 15) >= 0 ) uidv = cstr.mid(a + 15, b - a - 15);
    a = cstr.find("X-Access: ");
    b = cstr.find("\r\n", a);
    QString access;
    if ( (b - a - 10) >= 0 ) access = cstr.mid(a + 10, b - a - 10);
    mReadOnly = access == "Read only";
    QString startUid;
    if (uidValidity() != uidv)
    {
      // uidValidity changed
      expunge();
      mLastUid = 0;
      uidmap.clear();
    } else {
      if (!mCheckFlags)
        startUid = QString::number(lastUid() + 1);
    }
    mAccount->removeJob(it);
    reallyGetFolder(startUid);
  }
}

//-----------------------------------------------------------------------------
void KMFolderImap::getAndCheckFolder(bool force)
{
  if (mNoContent)
    return getFolder(force);

  mAccount->processNewMailSingleFolder(this);
  if (force) {
    // force an update
    mCheckFlags = TRUE;
  }
}

//-----------------------------------------------------------------------------
void KMFolderImap::getFolder(bool force)
{
  mGuessedUnreadMsgs = -1;
  if (mNoContent)
  {
    mContentState = imapFinished;
    emit folderComplete(this, true);
    return;
  }
  mContentState = imapInProgress;
  if (force) {
    // force an update
    mCheckFlags = TRUE;
  }
  checkValidity();
}


//-----------------------------------------------------------------------------
void KMFolderImap::reallyGetFolder(const QString &startUid)
{
  KURL url = mAccount->getUrl();
  if (!mAccount->makeConnection())
  {
    emit folderComplete(this, FALSE);
    return;
  }
  if (startUid.isEmpty())
  {
    url.setPath(imapPath() + ";SECTION=UID FLAGS");
    KIO::SimpleJob *job = KIO::listDir(url, FALSE);
    KIO::Scheduler::assignJobToSlave(mAccount->slave(), job);
    ImapAccountBase::jobData jd( url.url(), this );
    mAccount->insertJob(job, jd);
    connect(job, SIGNAL(result(KIO::Job *)),
            this, SLOT(slotListFolderResult(KIO::Job *)));
    connect(job, SIGNAL(entries(KIO::Job *, const KIO::UDSEntryList &)),
            this, SLOT(slotListFolderEntries(KIO::Job *,
            const KIO::UDSEntryList &)));
  } else {
    quiet(TRUE);
    url.setPath(imapPath() + ";UID=" + startUid
      + ":*;SECTION=ENVELOPE");
    KIO::SimpleJob *newJob = KIO::get(url, FALSE, FALSE);
    KIO::Scheduler::assignJobToSlave(mAccount->slave(), newJob);
    ImapAccountBase::jobData jd( url.url(), this );
    mAccount->insertJob(newJob, jd);
    connect(newJob, SIGNAL(result(KIO::Job *)),
            this, SLOT(slotGetLastMessagesResult(KIO::Job *)));
    connect(newJob, SIGNAL(data(KIO::Job *, const QByteArray &)),
            this, SLOT(slotGetMessagesData(KIO::Job *, const QByteArray &)));
  }
}


//-----------------------------------------------------------------------------
void KMFolderImap::slotListFolderResult(KIO::Job * job)
{
  ImapAccountBase::JobIterator it = mAccount->findJob(job);
  if ( it == mAccount->jobsEnd() ) return;
  QString uids;
  if (job->error())
  {
    mAccount->slotSlaveError( mAccount->slave(), job->error(),
        job->errorText() );
    emit folderComplete(this, FALSE);
    mAccount->removeJob(it);
    return;
  }
  mCheckFlags = FALSE;
  QStringList::Iterator uid;
  quiet(TRUE);
  // Check for already retrieved headers
  if (count())
  {
    QCString cstr;
    int idx = 0, a, b, c, serverFlags;
    long int mailUid, serverUid;
    uid = (*it).items.begin();
    while (idx < count() && uid != (*it).items.end())
    {
      getMsgString(idx, cstr);
      a = cstr.find("X-UID: ");
      b = cstr.find("\n", a);
      if (a == -1 || b == -1) mailUid = -1;
      else mailUid = cstr.mid(a + 7, b - a - 7).toLong();
      c = (*uid).find(",");
      serverUid = (*uid).left(c).toLong();
      serverFlags = (*uid).mid(c+1).toInt();
      if (mailUid < serverUid) removeMsg(idx, TRUE);
      else if (mailUid == serverUid)
      {
        if (!mReadOnly)
          flagsToStatus(getMsgBase(idx), serverFlags, false);
        idx++;
        uid = (*it).items.remove(uid);
      }
      else break;  // happens only, if deleted mails reappear on the server
    }
    while (idx < count()) removeMsg(idx, TRUE);
  }
  for (uid = (*it).items.begin(); uid != (*it).items.end(); uid++)
    (*uid).truncate((*uid).find(","));
  ImapAccountBase::jobData jd( QString::null, (*it).parent );
//jd.items = (*it).items;
  jd.total = (*it).items.count();
  if (jd.total == 0)
  {
    quiet(FALSE);
    mContentState = imapFinished;
    emit folderComplete(this, TRUE);
    mAccount->removeJob(it);
    return;
  }

  QStringList sets;
  uid = (*it).items.begin();
  if (jd.total == 1) sets.append(*uid + ":" + *uid);
  else sets = makeSets( (*it).items );
  mAccount->removeJob(it); // don't use *it below

  for (QStringList::Iterator i = sets.begin(); i != sets.end(); ++i)
  {
    KURL url = mAccount->getUrl();
    url.setPath(imapPath() + ";UID=" + *i + ";SECTION=ENVELOPE");
    if (!mAccount->makeConnection())
    {
      quiet(FALSE);
      emit folderComplete(this, FALSE);
      return;
    }
    KIO::SimpleJob *newJob = KIO::get(url, FALSE, FALSE);
    jd.url = url.url();
    KIO::Scheduler::assignJobToSlave(mAccount->slave(), newJob);
    mAccount->insertJob(newJob, jd);
    connect(newJob, SIGNAL(result(KIO::Job *)),
        this, (i == sets.at(sets.count() - 1))
        ? SLOT(slotGetLastMessagesResult(KIO::Job *))
        : SLOT(slotGetMessagesResult(KIO::Job *)));
    connect(newJob, SIGNAL(data(KIO::Job *, const QByteArray &)),
        this, SLOT(slotGetMessagesData(KIO::Job *, const QByteArray &)));
  }
}


//-----------------------------------------------------------------------------
void KMFolderImap::slotListFolderEntries(KIO::Job * job,
  const KIO::UDSEntryList & uds)
{
  ImapAccountBase::JobIterator it = mAccount->findJob(job);
  if ( it == mAccount->jobsEnd() ) return;
  QString mimeType, name;
  long int flags = 0;
  for (KIO::UDSEntryList::ConstIterator udsIt = uds.begin();
    udsIt != uds.end(); udsIt++)
  {
    for (KIO::UDSEntry::ConstIterator eIt = (*udsIt).begin();
      eIt != (*udsIt).end(); eIt++)
    {
      if ((*eIt).m_uds == KIO::UDS_NAME)
        name = (*eIt).m_str;
      else if ((*eIt).m_uds == KIO::UDS_MIME_TYPE)
        mimeType = (*eIt).m_str;
      else if ((*eIt).m_uds == KIO::UDS_ACCESS)
        flags = (*eIt).m_long;
    }
    if (mimeType == "message/rfc822-imap" && !(flags & 8))
      (*it).items.append(name + "," + QString::number(flags));
  }
}


//-----------------------------------------------------------------------------
void KMFolderImap::flagsToStatus(KMMsgBase *msg, int flags, bool newMsg)
{
  if (flags & 4) 
    msg->setStatus( KMMsgStatusFlag );
  if (flags & 2)
    msg->setStatus( KMMsgStatusReplied );
  if (flags & 1)
    msg->setStatus( KMMsgStatusOld );

  if (msg->isOfUnknownStatus()) {
    if (newMsg)
      msg->setStatus( KMMsgStatusNew );
    else
      msg->setStatus( KMMsgStatusUnread );
  }
}


//-----------------------------------------------------------------------------
QCString KMFolderImap::statusToFlags(KMMsgStatus status)
{
  QCString flags = "";
  if (status & KMMsgStatusNew || status & KMMsgStatusUnread) 
    return flags;
  if (status & KMMsgStatusDeleted) 
    flags += "\\DELETED";
  if (status & KMMsgStatusRead) 
    flags += "\\SEEN";
  if (status & KMMsgStatusReplied) 
    flags += "\\ANSWERED";
  if (status & KMMsgStatusFlag) 
    flags += "\\FLAGGED";
  return flags;
}

//-------------------------------------------------------------
void
KMFolderImap::ignoreJobsForMessage( KMMessage* msg )
{
  if (!msg || msg->transferInProgress())
    return;
  KMAcctImap *account;
  if ( !(account = static_cast<KMFolderImap*>(msg->parent())->account()) )
    return;

  account->ignoreJobsForMessage( msg );
}

//-----------------------------------------------------------------------------
void KMFolderImap::slotGetMessagesData(KIO::Job * job, const QByteArray & data)
{
  ImapAccountBase::JobIterator it = mAccount->findJob(job);
  if ( it == mAccount->jobsEnd() ) return;
  (*it).cdata += QCString(data, data.size() + 1);
  int pos = (*it).cdata.find("\r\n--IMAPDIGEST");
  if (pos > 0)
  {
    int p = (*it).cdata.find("\r\nX-uidValidity:");
    if (p != -1) setUidValidity((*it).cdata
      .mid(p + 17, (*it).cdata.find("\r\n", p+1) - p - 17));
    (*it).cdata.remove(0, pos);
  }
  pos = (*it).cdata.find("\r\n--IMAPDIGEST", 1);
  int flags;
  while (pos >= 0)
  {
    KMMessage *msg = new KMMessage;
    msg->fromString((*it).cdata.mid(16, pos - 16));
    flags = msg->headerField("X-Flags").toInt();
    ulong uid = msg->headerField("X-UID").toULong();
    if (flags & 8 || uid <= lastUid()) {
      delete msg;
      msg = 0;
    }
    else {
      if (uidmap.find(uid))
      {
        // assign the sernum from the cache
        const ulong sernum = (ulong) uidmap[uid];
        kdDebug() << "set sernum:" << sernum << " for " << uid << endl;
        msg->setMsgSerNum(sernum);
        // delete the entry
        uidmap.remove(uid);
      }
      open();
      KMFolderImapInherited::addMsg(msg, 0);
      /* The above calls emitMsgAddedSignals, but since we are in quiet mode,
         that has no effect. To get search folders to update on arrival of new
         messages explicitly emit the signal below on its own, so the folder
         manager realizes there is a new message. */
      emit msgAdded(this, msg->getMsgSerNum());
      close();
      // Transfer the status, if it is cached.
      QString id = msg->msgIdMD5();
      if ( mMetaDataMap.find( id ) ) {
        KMMsgMetaData *md =  mMetaDataMap[id];
        msg->setStatus( md->status() );
        mMetaDataMap.remove( id );
        delete md;
      }
      // Merge with the flags from the server.
      flagsToStatus((KMMsgBase*)msg, flags);

      if (count() > 1) unGetMsg(count() - 1);
      mLastUid = uid;
/*      if ((*it).total > 20 &&
        ((*it).done + 1) * 5 / (*it).total > (*it).done * 5 / (*it).total)
      {
        quiet(FALSE);
        quiet(TRUE);
      } */
    }
    (*it).cdata.remove(0, pos);
    (*it).done++;
    pos = (*it).cdata.find("\r\n--IMAPDIGEST", 1);
    mAccount->displayProgress();
  }
}

//-------------------------------------------------------------
FolderJob*
KMFolderImap::doCreateJob( KMMessage *msg, FolderJob::JobType jt,
                           KMFolder *folder ) const
{
  KMFolderImap* kmfi = dynamic_cast<KMFolderImap*>(folder);
  ImapJob *job = new ImapJob( msg, jt, kmfi );
  return job;
}

//-------------------------------------------------------------
FolderJob*
KMFolderImap::doCreateJob( QPtrList<KMMessage>& msgList, const QString& sets,
                           FolderJob::JobType jt, KMFolder *folder ) const
{
  KMFolderImap* kmfi = dynamic_cast<KMFolderImap*>(folder);
  ImapJob *job = new ImapJob( msgList, sets, jt, kmfi );
  return job;
}



//-----------------------------------------------------------------------------
void KMFolderImap::getMessagesResult(KIO::Job * job, bool lastSet)
{
  ImapAccountBase::JobIterator it = mAccount->findJob(job);
  if ( it == mAccount->jobsEnd() ) return;
  if (job->error())
  {
    mAccount->slotSlaveError( mAccount->slave(), job->error(),
        job->errorText() );
    mContentState = imapNoInformation;
    emit folderComplete(this, FALSE);
  } else if (lastSet) mContentState = imapFinished;
  if (lastSet) quiet(FALSE);
  mAccount->removeJob(it);
  if (!job->error() && lastSet)
      emit folderComplete(this, TRUE);
}


//-----------------------------------------------------------------------------
void KMFolderImap::slotGetLastMessagesResult(KIO::Job * job)
{
  getMessagesResult(job, true);
}


//-----------------------------------------------------------------------------
void KMFolderImap::slotGetMessagesResult(KIO::Job * job)
{
  getMessagesResult(job, false);
}


//-----------------------------------------------------------------------------
void KMFolderImap::createFolder(const QString &name)
{
  KURL url = mAccount->getUrl();
  url.setPath(imapPath() + name);
  if (!mAccount->makeConnection()) return;
  KIO::SimpleJob *job = KIO::mkdir(url);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), job);
  ImapAccountBase::jobData jd( url.url(), this );
  jd.items = name;
  mAccount->insertJob(job, jd);
  connect(job, SIGNAL(result(KIO::Job *)),
          this, SLOT(slotCreateFolderResult(KIO::Job *)));
}


//-----------------------------------------------------------------------------
void KMFolderImap::slotCreateFolderResult(KIO::Job * job)
{
  ImapAccountBase::JobIterator it = mAccount->findJob(job);
  if ( it == mAccount->jobsEnd() ) return;
  if (job->error())
  {
    mAccount->slotSlaveError( mAccount->slave(), job->error(),
        job->errorText() );
  } else {
    listDirectory();
  }
  mAccount->removeJob(job);
}


//-----------------------------------------------------------------------------
static QTextCodec *sUtf7Codec = 0;

QTextCodec * KMFolderImap::utf7Codec()
{
  if (!sUtf7Codec) sUtf7Codec = QTextCodec::codecForName("utf-7");
  return sUtf7Codec;
}


//-----------------------------------------------------------------------------
QString KMFolderImap::encodeFileName(const QString &name)
{
  QString result = utf7Codec()->fromUnicode(name);
  return KURL::encode_string_no_slash(result);
}


//-----------------------------------------------------------------------------
QString KMFolderImap::decodeFileName(const QString &name)
{
  QString result = KURL::decode_string(name);
  return utf7Codec()->toUnicode(result.latin1());
}

//-----------------------------------------------------------------------------
bool KMFolderImap::autoExpunge()
{
  if (mAccount)
    return mAccount->autoExpunge();

  return false;
}


//-----------------------------------------------------------------------------
void KMFolderImap::slotSimpleData(KIO::Job * job, const QByteArray & data)
{
  ImapAccountBase::JobIterator it = mAccount->findJob(job);
  if ( it == mAccount->jobsEnd() ) return;
  QBuffer buff((*it).data);
  buff.open(IO_WriteOnly | IO_Append);
  buff.writeBlock(data.data(), data.size());
  buff.close();
}

//-----------------------------------------------------------------------------
void KMFolderImap::deleteMessage(KMMessage * msg)
{
  KURL url = mAccount->getUrl();
  KMFolderImap *msg_parent = static_cast<KMFolderImap*>(msg->parent());
  url.setPath(msg_parent->imapPath() + ";UID=" + msg->headerField("X-UID"));
  if (!mAccount->makeConnection()) return;
  KIO::SimpleJob *job = KIO::file_delete(url, FALSE);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), job);
  ImapAccountBase::jobData jd( url.url(), 0 );
  mAccount->insertJob(job, jd);
  connect(job, SIGNAL(result(KIO::Job *)),
          mAccount, SLOT(slotSimpleResult(KIO::Job *)));
}

void KMFolderImap::deleteMessage(QPtrList<KMMessage> msgList)
{
  mAccount->tempOpenFolder(this);
  KMMessage *aMsg = msgList.getFirst();
  if (aMsg)
  {
    KMFolder* parent = aMsg->parent();
    if (parent) mAccount->tempOpenFolder(parent);
  }
  QValueList<int> uids;
  getUids(msgList, uids);
  QStringList sets = makeSets(uids);

  KURL url = mAccount->getUrl();
  KMFolderImap *msg_parent = static_cast<KMFolderImap*>(msgList.first()->parent());
  for ( QStringList::Iterator it = sets.begin(); it != sets.end(); ++it )
  {
    url.setPath(msg_parent->imapPath() + ";UID=" + *it);
    if (!mAccount->makeConnection()) return;
    KIO::SimpleJob *job = KIO::file_delete(url, FALSE);
    KIO::Scheduler::assignJobToSlave(mAccount->slave(), job);
    ImapAccountBase::jobData jd( url.url(), 0 );
    mAccount->insertJob(job, jd);
    connect(job, SIGNAL(result(KIO::Job *)),
        mAccount, SLOT(slotSimpleResult(KIO::Job *)));
  }
}

//-----------------------------------------------------------------------------
void KMFolderImap::setStatus(int idx, KMMsgStatus status)
{
  QValueList<int> ids; ids.append(idx);
  setStatus(ids, status);
}

void KMFolderImap::setStatus(QValueList<int>& ids, KMMsgStatus status)
{
  KMFolder::setStatus(ids, status);
  if (mReadOnly) return;

  // get the uids
  QValueList<int> uids;
  getUids(ids, uids);

  // get the flags
  QCString flags = statusToFlags(status);

  // get the sets of ranges..
  QStringList sets = makeSets(uids);
  // ..and pass them to the server
  for ( QStringList::Iterator it = sets.begin(); it != sets.end(); ++it )
  {
    setImapStatus(imapPath() + ";UID=" + *it, flags);
  }
  mAccount->displayProgress();

}

//-----------------------------------------------------------------------------
QStringList KMFolderImap::makeSets(QStringList& uids, bool sort)
{
  QValueList<int> tmp;
  for ( QStringList::Iterator it = uids.begin(); it != uids.end(); ++it )
    tmp.append( (*it).toInt() );
  return makeSets(tmp, sort);
}

QStringList KMFolderImap::makeSets(QValueList<int>& uids, bool sort)
{
  QStringList sets;
  QString set;

  if (uids.size() == 1)
  {
    sets.append(QString::number(uids.first()));
    return sets;
  }

  if (sort) qHeapSort(uids);

  int last = 0;
  // needed to make a uid like 124 instead of 124:124
  bool inserted = false;
  /* iterate over uids and build sets like 120:122,124,126:150 */
  for ( QValueList<int>::Iterator it = uids.begin(); it != uids.end(); ++it )
  {
    if (it == uids.begin() || set.isEmpty()) {
      set = QString::number(*it);
      inserted = true;
    } else
    {
      if (last+1 != *it)
      {
        // end this range
        if (inserted)
          set += ',' + QString::number(*it);
        else
          set += ':' + QString::number(last) + ',' + QString::number(*it);
        inserted = true;
        if (set.length() > 100)
        {
          // just in case the server has a problem with longer lines..
          sets.append(set);
          set = "";
        }
      } else {
        inserted = false;
      }
    }
    last = *it;
  }
  // last element
  if (!inserted)
    set += ':' + QString::number(uids.last());

  if (!set.isEmpty()) sets.append(set);

  return sets;
}

//-----------------------------------------------------------------------------
void KMFolderImap::getUids(QValueList<int>& ids, QValueList<int>& uids)
{
  KMMessage *msg = 0;
  // get the uids
  for ( QValueList<int>::Iterator it = ids.begin(); it != ids.end(); ++it )
  {
    bool unget = !isMessage(*it);
    msg = getMsg(*it);
    if (!msg) continue;
    uids.append(msg->headerField("X-UID").toInt());
    if (unget) unGetMsg(*it);
  }
}

void KMFolderImap::getUids(QPtrList<KMMessage>& msgList, QValueList<int>& uids, KMFolder* msgParent)
{
  KMMessage *msg = 0;

  if (!msgParent) msgParent = msgList.first()->parent();
  if (!msgParent) return;

  for ( msg = msgList.first(); msg; msg = msgList.next() )
  {
    if ( !msg->headerField("X-UID").isEmpty() )
      uids.append(msg->headerField("X-UID").toInt());
  }
}

//-----------------------------------------------------------------------------
void KMFolderImap::setImapStatus(QString path, QCString flags)
{
  // set the status on the server, the uids are integrated in the path
  kdDebug(5006) << "setImapStatus path=" << path << endl;
  KURL url = mAccount->getUrl();
  url.setPath(path);

  QByteArray packedArgs;
  QDataStream stream( packedArgs, IO_WriteOnly);

  stream << (int) 'S' << url << flags;

  if (!mAccount->makeConnection()) return;
  KIO::SimpleJob *job = KIO::special(url, packedArgs, FALSE);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), job);
  ImapAccountBase::jobData jd( url.url(), 0 );
  mAccount->insertJob(job, jd);
  connect(job, SIGNAL(result(KIO::Job *)),
          SLOT(slotSetStatusResult(KIO::Job *)));
}


//-----------------------------------------------------------------------------
void KMFolderImap::expungeFolder(KMFolderImap * aFolder, bool quiet)
{
  aFolder->setNeedsCompacting(FALSE);
  KURL url = mAccount->getUrl();
  url.setPath(aFolder->imapPath() + ";UID=*");
  if (!mAccount->makeConnection()) return;
  KIO::SimpleJob *job = KIO::file_delete(url, FALSE);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), job);
  ImapAccountBase::jobData jd( url.url(), 0 );
  jd.quiet = quiet;
  mAccount->insertJob(job, jd);
  connect(job, SIGNAL(result(KIO::Job *)),
          mAccount, SLOT(slotSimpleResult(KIO::Job *)));
}


//-----------------------------------------------------------------------------
void KMFolderImap::slotSetStatusResult(KIO::Job * job)
{
  ImapAccountBase::JobIterator it = mAccount->findJob(job);
  if ( it == mAccount->jobsEnd() ) return;
  mAccount->removeJob(it);
  if (job->error() && job->error() != KIO::ERR_CANNOT_OPEN_FOR_WRITING)
  {
    mAccount->slotSlaveError( mAccount->slave(), job->error(),
        job->errorText() );
  }
  mAccount->displayProgress();
}


//-----------------------------------------------------------------------------
void KMFolderImap::processNewMail(bool)
{
  KURL url = mAccount->getUrl();
  if (mReadOnly)
    url.setPath(imapPath() + ";SECTION=UIDNEXT");
  else
    url.setPath(imapPath() + ";SECTION=UNSEEN");
  if (!mAccount->makeConnection()) return;
  KIO::SimpleJob *job = KIO::stat(url, FALSE);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), job);
  ImapAccountBase::jobData jd(url.url());
  mAccount->insertJob(job, jd);
  connect(job, SIGNAL(result(KIO::Job *)),
          SLOT(slotStatResult(KIO::Job *)));
}


//-----------------------------------------------------------------------------
void KMFolderImap::slotStatResult(KIO::Job * job)
{
  ImapAccountBase::JobIterator it = mAccount->findJob(job);
  if ( it == mAccount->jobsEnd() ) return;
  mAccount->removeJob(it);
  if (job->error())
  {
    mAccount->slotSlaveError( mAccount->slave(), job->error(),
        job->errorText() );
  } else {
    KIO::UDSEntry uds = static_cast<KIO::StatJob*>(job)->statResult();
    for (KIO::UDSEntry::ConstIterator it = uds.begin(); it != uds.end(); it++)
    {
      if ((*it).m_uds == KIO::UDS_SIZE)
      {
        if (mReadOnly)
        {
          mGuessedUnreadMsgs = -1;
          mGuessedUnreadMsgs = countUnread() + (*it).m_long - lastUid() - 1;
          if (mGuessedUnreadMsgs < 0) mGuessedUnreadMsgs = 0;
        } else {
          mUnreadMsgs = (*it).m_long;
        }
        emit numUnreadMsgsChanged( this );
      }
    }
  }
  mAccount->displayProgress();
}

//-----------------------------------------------------------------------------
int KMFolderImap::create(bool imap)
{
  readConfig();
  mUnreadMsgs = -1;
  return KMFolderImapInherited::create(imap);
}

QValueList<int> KMFolderImap::splitSets(QString uids)
{
  QValueList<int> uidlist;

  // ex: 1205,1204,1203,1202,1236:1238
  QString buffer = QString::null;
  int setstart = -1;
  // iterate over the uids
  for (uint i = 0; i < uids.length(); i++)
  {
    QChar chr = uids[i];
    if (chr == ',')
    {
      if (setstart > -1)
      {
        // a range (uid:uid) was before
        for (int j = setstart; j <= buffer.toInt(); j++)
        {
          uidlist.append(j);
        }
        setstart = -1;
      } else {
        // single uid
        uidlist.append(buffer.toInt());
      }
      buffer = "";
    } else if (chr == ':') {
      // remember the start of the range
      setstart = buffer.toInt();
      buffer = "";
    } else if (chr.category() == QChar::Number_DecimalDigit) {
      // digit
      buffer += chr;
    } else {
      // ignore
    }
  }
  // process the last data
  if (setstart > -1)
  {
    for (int j = setstart; j <= buffer.toInt(); j++)
    {
      uidlist.append(j);
    }
  } else {
    uidlist.append(buffer.toInt());
  }

  return uidlist;
}

#include "kmfolderimap.moc"
