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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmfolderimap.h"
#include "kmfoldertree.h"
#include "kmundostack.h"
#include "kmfoldermgr.h"

#include <kapplication.h>
#include <kdebug.h>
#include <kio/scheduler.h>

#include <qbuffer.h>
#include <qtextcodec.h>

#include <assert.h>

KMFolderImap::KMFolderImap(KMFolderDir* aParent, const QString& aName)
  : KMFolderImapInherited(aParent, aName)
{
  mContentState = imapNoInformation;
  mSubfolderState = imapNoInformation;
  mAccount = NULL;
  mIsSelected = FALSE;
  mLastUid = 0;
  mCheckFlags = TRUE;

  KConfig* config = kapp->config();
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
}

KMFolderImap::~KMFolderImap()
{
  KConfig* config = kapp->config();
  KConfigGroupSaver saver(config, "Folder-" + idString());
  config->writeEntry("UidValidity", mUidValidity);
  config->writeEntry("ImapPath", mImapPath);
  config->writeEntry("NoContent", mNoContent);
  config->writeEntry("ReadOnly", mReadOnly);

  if (kernel->undoStack()) kernel->undoStack()->folderDestroyed(this);
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
  KMFolderImapInherited::readConfig();
}

//-----------------------------------------------------------------------------
void KMFolderImap::writeConfig()
{
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
  KMAcctImap::jobData jd;
  KMAcctImap::initJobData(jd);
  mAccount->mapJobData.insert(job, jd);
  mAccount->displayProgress();
  connect(job, SIGNAL(result(KIO::Job *)),
          this, SLOT(slotRemoveFolderResult(KIO::Job *)));
}

//-----------------------------------------------------------------------------
void KMFolderImap::slotRemoveFolderResult(KIO::Job *job)
{
  QMap<KIO::Job *, KMAcctImap::jobData>::Iterator it =
    mAccount->mapJobData.find(job);
  if (it == mAccount->mapJobData.end()) return;
  if (job->error())
  {
    mAccount->slotSlaveError( mAccount->slave(), job->error(),
        job->errorText() );
  } else {
    if (mAccount->slave()) mAccount->mapJobData.remove(it);
    mAccount->displayProgress();
    kernel->imapFolderMgr()->remove(this);
  }
}

//-----------------------------------------------------------------------------
void KMFolderImap::removeMsg(int idx, bool quiet)
{
  if (idx < 0)
    return;

  KMMsgBase* mb = mMsgList[idx];
  if (!quiet && !mb->isMessage())
    readMsg(idx);

  if (!quiet)
  {
    KMMessage *msg = static_cast<KMMessage*>(mb);
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
void KMFolderImap::addMsgQuiet(KMMessage* aMsg)
{
  KMFolder *folder = aMsg->parent();
  if (folder) kernel->undoStack()->pushAction( aMsg->getMsgSerNum(), folder, this );
  if (folder) folder->take(folder->find(aMsg));
  delete aMsg;
  getFolder();
}

//-----------------------------------------------------------------------------
void KMFolderImap::addMsgQuiet(QPtrList<KMMessage> msgList)
{
  KMFolder *folder = msgList.first()->parent();
  for ( KMMessage* msg = msgList.first(); msg; msg = msgList.next() )
  {
    kernel->undoStack()->pushAction( msg->getMsgSerNum(), folder, this );
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
  KMMessage *aMsg = msgList.getFirst();
  KMFolder *msgParent = aMsg->parent();

  // make sure the messages won't be deleted while we work with them
  for ( KMMessage* msg = msgList.first(); msg; msg = msgList.next() )
    msg->setTransferInProgress(true);

  KMImapJob *imapJob = NULL;
  if (msgParent)
  {
    if (msgParent->protocol() == "imap")
    {
      if (static_cast<KMFolderImap*>(msgParent)->account() == account())
      {
        if (this == msgParent)
        {
          // transfer the whole message, e.g. a draft-message is canceled and re-added to the drafts-folder
          for ( KMMessage* msg = msgList.first(); msg; msg = msgList.next() )
          {
            int idx = msgParent->find(msg);
            assert(idx != -1);
            msg = msgParent->getMsg(idx);
            imapJob = new KMImapJob(msg, KMImapJob::tPutMessage, this);
            connect(imapJob, SIGNAL(messageCopied(KMMessage*)),
                SLOT(addMsgQuiet(KMMessage*)));
          }

        } else {

          /* get the messages and the uids
             don't unGet the messages because we need to access the serial number in addMsgQuiet */
          QValueList<int> uids;
          getUids(msgList, uids);

          // get the sets (do not sort the uids)
          QStringList sets = makeSets(uids, false);

          for ( QStringList::Iterator it = sets.begin(); it != sets.end(); ++it )
          {
            // we need the messages that belong to the current set to pass them to the ImapJob
            QPtrList<KMMessage> temp_msgs = splitMessageList(*it, msgList);

            imapJob = new KMImapJob(temp_msgs, *it, KMImapJob::tCopyMessage, this);
            connect(imapJob, SIGNAL(messageCopied(QPtrList<KMMessage>)),
                SLOT(addMsgQuiet(QPtrList<KMMessage>)));
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
    imapJob = new KMImapJob(msg, KMImapJob::tPutMessage, this);
    connect(imapJob, SIGNAL(messageStored(KMMessage*)),
        SLOT(addMsgQuiet(KMMessage*)));
  }

  if (aIndex_ret) *aIndex_ret = -1;
  return 0;
}

//-----------------------------------------------------------------------------
void KMFolderImap::copyMsg(QPtrList<KMMessage>& msgList)
{
  QValueList<int> uids;
  getUids(msgList, uids);
  QStringList sets = makeSets(uids, false);
  for ( QStringList::Iterator it = sets.begin(); it != sets.end(); ++it )
  {
    // we need the messages that belong to the current set to pass them to the ImapJob
    QPtrList<KMMessage> temp_msgs = splitMessageList(*it, msgList);

    new KMImapJob(temp_msgs, *it, KMImapJob::tCopyMessage, this);
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
    KMMessage* msg = NULL;
    while ( (msg = it.current()) != NULL )
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
  if (!mb) return NULL;
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
// Originally from KMAcctImap
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool KMFolderImap::listDirectory(bool secondStep)
{
  mSubfolderState = imapInProgress;
  KMAcctImap::jobData jd;
  jd.parent = this;
  jd.total = 1; jd.done = 0;
  jd.inboxOnly = !secondStep && mAccount->prefix() != "/"
    && imapPath() == mAccount->prefix();
  KURL url = mAccount->getUrl();
  url.setPath(((jd.inboxOnly) ? QString("/") : imapPath())
    + ";TYPE=" + ((mAccount->onlySubscribedFolders()) ? "LSUB" : "LIST"));
  if (!mAccount->makeConnection())
    return FALSE;

  if (!secondStep) mHasInbox = FALSE;
  mSubfolderNames.clear();
  mSubfolderPaths.clear();
  mSubfolderMimeTypes.clear();
  KIO::SimpleJob *job = KIO::listDir(url, FALSE);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), job);
  mAccount->mapJobData.insert(job, jd);
  connect(job, SIGNAL(result(KIO::Job *)),
          this, SLOT(slotListResult(KIO::Job *)));
  connect(job, SIGNAL(entries(KIO::Job *, const KIO::UDSEntryList &)),
          this, SLOT(slotListEntries(KIO::Job *, const KIO::UDSEntryList &)));
  mAccount->displayProgress();
  
  return TRUE;
}


//-----------------------------------------------------------------------------
void KMFolderImap::slotListResult(KIO::Job * job)
{
  QMap<KIO::Job *, KMAcctImap::jobData>::Iterator it =
    mAccount->mapJobData.find(job);
  if (it == mAccount->mapJobData.end()) return;
  assert(it != mAccount->mapJobData.end());
  if (job->error())
  {
    mAccount->slotSlaveError( mAccount->slave(), job->error(),
        job->errorText() );
  }
  mSubfolderState = imapFinished;
  bool it_inboxOnly = (*it).inboxOnly;
  if (mAccount->slave()) mAccount->mapJobData.remove(it);
  if (!job->error())
  {
    kernel->imapFolderMgr()->quiet(TRUE);
    if (it_inboxOnly) listDirectory(TRUE);
    else {
      if (mIsSystemFolder && mImapPath == "/INBOX/"
        && mAccount->prefix() == "/INBOX/")
      {
        mHasInbox = FALSE;
        mSubfolderNames.clear();
      }
      createChildFolder();
      KMFolderImap *folder;
      KMFolderNode *node = mChild->first();
      while (node)
      {
        if (!node->isDir() && (node->name() != "INBOX" || !mHasInbox)
            && mSubfolderNames.findIndex(node->name()) == -1)
        {
          kdDebug(5006) << node->name() << " disappeared." << endl;
          kernel->imapFolderMgr()->remove(static_cast<KMFolder*>(node));
          node = mChild->first();
        }
        else node = mChild->next();
      }
      if (mHasInbox)
      {
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
    kernel->imapFolderMgr()->quiet(FALSE);
  }
  mAccount->displayProgress();
}


//-----------------------------------------------------------------------------
void KMFolderImap::slotListEntries(KIO::Job * job, const KIO::UDSEntryList & uds)
{
  QMap<KIO::Job *, KMAcctImap::jobData>::Iterator it =
    mAccount->mapJobData.find(job);
  if (it == mAccount->mapJobData.end()) return;
  assert(it != mAccount->mapJobData.end());
  QString name;
  KURL url;
  QString mimeType;
  for (KIO::UDSEntryList::ConstIterator udsIt = uds.begin();
    udsIt != uds.end(); udsIt++)
  {
    mimeType = QString::null;
    for (KIO::UDSEntry::ConstIterator eIt = (*udsIt).begin();
      eIt != (*udsIt).end(); eIt++)
    {
      if ((*eIt).m_uds == KIO::UDS_NAME)
        name = (*eIt).m_str;
      else if ((*eIt).m_uds == KIO::UDS_URL)
        url = KURL((*eIt).m_str, 106); // utf-8
      else if ((*eIt).m_uds == KIO::UDS_MIME_TYPE)
        mimeType = (*eIt).m_str;
    }
    if ((mimeType == "inode/directory" || mimeType == "message/digest"
        || mimeType == "message/directory")
        && name != ".." && (mAccount->hiddenFolders() || name.at(0) != '.')
        && (!(*it).inboxOnly || name == "INBOX"))
    {
      if (((*it).inboxOnly || url.path() == "/INBOX/") && name == "INBOX")
        mHasInbox = TRUE;
      else {
        // Some servers send _lots_ of duplicates
        if (mSubfolderNames.findIndex(name) == -1)
        {
          mSubfolderNames.append(name);
          mSubfolderPaths.append(url.path());
          mSubfolderMimeTypes.append(mimeType);
        }
      }
/*      static_cast<KMFolderTree*>((*it).parent->listView())
        ->addImapChildFolder((*it).parent, name, url.path(),
        mimeType, (*it).inboxOnly); */
    }
  }
//  static_cast<KMFolderTree*>((*it).parent->listView())->delayedUpdate();
}


//-----------------------------------------------------------------------------
void KMFolderImap::checkValidity()
{
kdDebug(5006) << "KMFolderImap::checkValidity" << endl;
  KMAcctImap::jobData jd;
  jd.parent = this;
  jd.total = 1; jd.done = 0;
  KURL url = mAccount->getUrl();
  url.setPath(imapPath() + ";UID=0:0");
  if (!mAccount->makeConnection())
  {
    emit folderComplete(this, FALSE);
    return;
  }
  KIO::SimpleJob *job = KIO::get(url, FALSE, FALSE);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), job);
  mAccount->mapJobData.insert(job, jd);
  connect(job, SIGNAL(result(KIO::Job *)),
          SLOT(slotCheckValidityResult(KIO::Job *)));
  connect(job, SIGNAL(data(KIO::Job *, const QByteArray &)),
          SLOT(slotSimpleData(KIO::Job *, const QByteArray &)));
  mAccount->displayProgress();
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
kdDebug(5006) << "KMFolderImap::slotCheckValidityResult" << endl;
  QMap<KIO::Job *, KMAcctImap::jobData>::Iterator it =
    mAccount->mapJobData.find(job);
  if (it == mAccount->mapJobData.end()) return;
  if (job->error())
  {
    mAccount->slotSlaveError( mAccount->slave(), job->error(),
        job->errorText() );
    emit folderComplete(this, FALSE);
    mAccount->displayProgress();
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
      expunge();
      mLastUid = 0;
    } else {
      if (!mCheckFlags)
        startUid = QString::number(lastUid() + 1);
    }
    mAccount->mapJobData.remove(it);
    reallyGetFolder(startUid);
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
  KMAcctImap::jobData jd;
  jd.parent = this;
  jd.total = 1; jd.done = 0;
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
    mAccount->mapJobData.insert(job, jd);
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
    mAccount->mapJobData.insert(newJob, jd);
    connect(newJob, SIGNAL(result(KIO::Job *)),
            this, SLOT(slotGetLastMessagesResult(KIO::Job *)));
    connect(newJob, SIGNAL(data(KIO::Job *, const QByteArray &)),
            this, SLOT(slotGetMessagesData(KIO::Job *, const QByteArray &)));
  }
  mAccount->displayProgress();
}


//-----------------------------------------------------------------------------
void KMFolderImap::slotListFolderResult(KIO::Job * job)
{
  QMap<KIO::Job *, KMAcctImap::jobData>::Iterator it =
    mAccount->mapJobData.find(job);
  if (it == mAccount->mapJobData.end()) return;
  assert(it != mAccount->mapJobData.end());
  QString uids;
  KMAcctImap::jobData jd;
  jd.parent = (*it).parent;
  jd.done = 0;
  if (job->error())
  {
    mAccount->slotSlaveError( mAccount->slave(), job->error(),
        job->errorText() );
    emit folderComplete(this, FALSE);
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
          getMsgBase(idx)->setStatus(flagsToStatus(serverFlags, false));
        idx++;
        uid = (*it).items.remove(uid);
      }
      else break;  // happens only, if deleted mails reappear on the server
    }
    while (idx < count()) removeMsg(idx, TRUE);
  }
  for (uid = (*it).items.begin(); uid != (*it).items.end(); uid++)
    (*uid).truncate((*uid).find(","));
//jd.items = (*it).items;
  jd.total = (*it).items.count();
  uid = (*it).items.begin();
  if (jd.total == 0)
  {
    quiet(FALSE);
    mContentState = imapFinished;
    emit folderComplete(this, TRUE);
    mAccount->mapJobData.remove(it);
    mAccount->displayProgress();
    return;
  }

  QStringList sets;
  if (jd.total == 1) sets.append(*uid + ":" + *uid);
  else sets = makeSets( (*it).items );

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
    KIO::Scheduler::assignJobToSlave(mAccount->slave(), newJob);
    mAccount->mapJobData.insert(newJob, jd);
    connect(newJob, SIGNAL(result(KIO::Job *)),
        this, (i == sets.at(sets.count() - 1))
        ? SLOT(slotGetLastMessagesResult(KIO::Job *))
        : SLOT(slotGetMessagesResult(KIO::Job *)));
    connect(newJob, SIGNAL(data(KIO::Job *, const QByteArray &)),
        this, SLOT(slotGetMessagesData(KIO::Job *, const QByteArray &)));
  }
  mAccount->mapJobData.remove(it);
}


//-----------------------------------------------------------------------------
void KMFolderImap::slotListFolderEntries(KIO::Job * job,
  const KIO::UDSEntryList & uds)
{
  QMap<KIO::Job *, KMAcctImap::jobData>::Iterator it =
    mAccount->mapJobData.find(job);
  if (it == mAccount->mapJobData.end()) return;
  assert(it != mAccount->mapJobData.end());
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
KMMsgStatus KMFolderImap::flagsToStatus(int flags, bool newMsg)
{
  if (flags & 4) return KMMsgStatusFlag;
  if (flags & 2) return KMMsgStatusReplied;
  if (flags & 1) return KMMsgStatusOld;
  return (newMsg) ? KMMsgStatusNew : KMMsgStatusUnread;
}


//-----------------------------------------------------------------------------
QCString KMFolderImap::statusToFlags(KMMsgStatus status)
{
  QCString flags = "";
  switch (status)
  {
    case KMMsgStatusNew:
    case KMMsgStatusUnread:
      break;
    case KMMsgStatusDeleted:
      flags = "\\DELETED";
      break;
    case KMMsgStatusReplied:
      flags = "\\SEEN \\ANSWERED";
      break;
    case KMMsgStatusFlag:
      flags = "\\SEEN \\FLAGGED";
      break;
    default:
      flags = "\\SEEN";
  }
  return flags;
}


//-----------------------------------------------------------------------------
void KMFolderImap::slotGetMessagesData(KIO::Job * job, const QByteArray & data)
{
  QMap<KIO::Job *, KMAcctImap::jobData>::Iterator it =
    mAccount->mapJobData.find(job);
  if (it == mAccount->mapJobData.end()) return;
  assert(it != mAccount->mapJobData.end());
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
    if (flags & 8 || uid <= lastUid()) delete msg;
    else {
      msg->setStatus(flagsToStatus(flags));
      KMFolderImapInherited::addMsg(msg, NULL);
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


//-----------------------------------------------------------------------------
void KMFolderImap::getMessagesResult(KIO::Job * job, bool lastSet)
{
  QMap<KIO::Job *, KMAcctImap::jobData>::Iterator it =
    mAccount->mapJobData.find(job);
  if (it == mAccount->mapJobData.end()) return;
  assert(it != mAccount->mapJobData.end());
  if (job->error())
  {
    mAccount->slotSlaveError( mAccount->slave(), job->error(),
        job->errorText() );
    mContentState = imapNoInformation;
    emit folderComplete(this, FALSE);
  } else if (lastSet) mContentState = imapFinished;
  if (lastSet) quiet(FALSE);
  if (mAccount->slave()) mAccount->mapJobData.remove(it);
  mAccount->displayProgress();
  if (!job->error() && lastSet) emit folderComplete(this, TRUE);
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
  KMAcctImap::jobData jd;
  jd.parent = this;
  jd.items = name;
  jd.total = 1; jd.done = 0;
  mAccount->mapJobData.insert(job, jd);
  mAccount->displayProgress();
  connect(job, SIGNAL(result(KIO::Job *)),
          this, SLOT(slotCreateFolderResult(KIO::Job *)));
}


//-----------------------------------------------------------------------------
void KMFolderImap::slotCreateFolderResult(KIO::Job * job)
{
  QMap<KIO::Job *, KMAcctImap::jobData>::Iterator it =
    mAccount->mapJobData.find(job);
  if (it == mAccount->mapJobData.end()) return;
  if (job->error())
  {
    mAccount->slotSlaveError( mAccount->slave(), job->error(),
        job->errorText() );
  } else {
    listDirectory();
  }
  if (mAccount->slave()) mAccount->mapJobData.remove(it);
  mAccount->displayProgress();
}


//-----------------------------------------------------------------------------
static QTextCodec *sUtf7Codec = NULL;

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
KMImapJob::KMImapJob(KMMessage *msg, JobType jt, KMFolderImap* folder)
{
  mMsg = msg;
  QPtrList<KMMessage> list; list.append(msg);
  init(jt, msg->headerField("X-UID"), folder, list);
}

//-----------------------------------------------------------------------------
KMImapJob::KMImapJob(QPtrList<KMMessage>& msgList, QString sets, JobType jt, KMFolderImap* folder)
{
  mMsg = msgList.first();
  init(jt, sets, folder, msgList);
}

void KMImapJob::init(JobType jt, QString sets, KMFolderImap* folder, QPtrList<KMMessage>& msgList)
{
  assert(jt == tGetMessage || folder);
  KMMessage* msg = msgList.first();
  mType = jt;
  mDestFolder = folder;
  KMFolderImap *msg_parent = static_cast<KMFolderImap*>(msg->parent());
  KMAcctImap *account = (folder) ? folder->account() : msg_parent->account();
  account->mJobList.append(this);
  if (jt == tPutMessage)
  {
    // transfers the complete message to the server
    KURL url = account->getUrl();
    url.setPath(folder->imapPath() + ";SECTION="
      + QString::fromLatin1(KMFolderImap::statusToFlags(msg->status())));
    KMAcctImap::jobData jd;
    jd.parent = NULL; jd.offset = 0;
    jd.total = 1; jd.done = 0;
    jd.msgList.append(msg);
    QCString cstr(msg->asString());
    int a = cstr.find("\nX-UID: ");
    int b = cstr.find("\n", a);
    if (a != -1 && b != -1 && cstr.find("\n\n") > a) cstr.remove(a, b-a);
    mData.resize(cstr.length() + cstr.contains("\n"));
    unsigned int i = 0;
    for (char *ch = cstr.data(); *ch; ch++)
    {
      if (*ch == '\n') { mData.at(i) = '\r'; i++; }
      mData.at(i) = *ch; i++;
    }
    jd.data = mData;
    if (!account->makeConnection())
    {
      account->mJobList.remove(this);
      delete this;
      return;
    }
    KIO::SimpleJob *simpleJob = KIO::put(url, 0, FALSE, FALSE, FALSE);
    KIO::Scheduler::assignJobToSlave(account->slave(), simpleJob);
    mJob = simpleJob;
    account->mapJobData.insert(mJob, jd);
    connect(mJob, SIGNAL(result(KIO::Job *)),
        SLOT(slotPutMessageResult(KIO::Job *)));
    connect(mJob, SIGNAL(dataReq(KIO::Job *, QByteArray &)),
        SLOT(slotPutMessageDataReq(KIO::Job *, QByteArray &)));
    account->displayProgress();
  }
  else if (jt == tCopyMessage)
  {
    KURL url = account->getUrl();
    KURL destUrl = account->getUrl();
    destUrl.setPath(folder->imapPath());
    url.setPath(msg_parent->imapPath() + ";UID=" + sets);
    KMAcctImap::jobData jd;
    jd.parent = NULL; mOffset = 0;
    jd.total = 1; jd.done = 0;
    jd.msgList = msgList;

    QByteArray packedArgs;
    QDataStream stream( packedArgs, IO_WriteOnly);

    stream << (int) 'C' << url << destUrl;

    if (!account->makeConnection())
    {
      account->mJobList.remove(this);
      delete this;
      return;
    }
    KIO::SimpleJob *simpleJob = KIO::special(url, packedArgs, FALSE);
    KIO::Scheduler::assignJobToSlave(account->slave(), simpleJob);
    mJob = simpleJob;
    account->mapJobData.insert(mJob, jd);
    connect(mJob, SIGNAL(result(KIO::Job *)),
        SLOT(slotCopyMessageResult(KIO::Job *)));
    account->displayProgress();
  } else {
    slotGetNextMessage();
  }
}


//-----------------------------------------------------------------------------
KMImapJob::~KMImapJob()
{
  if (mDestFolder)
  {
    KMAcctImap *account = mDestFolder->account();
    if (!account || !mJob) return;
    QMap<KIO::Job *, KMAcctImap::jobData>::Iterator it =
      account->mapJobData.find(mJob);
    if (it == account->mapJobData.end()) return;
    if ( !(*it).msgList.isEmpty() )
    {
      for ( KMMessage* msg = (*it).msgList.first(); msg; msg = (*it).msgList.next() )
        msg->setTransferInProgress(false);
    }
  }
  emit finished();
}


//-----------------------------------------------------------------------------
void KMImapJob::slotGetNextMessage()
{
  KMFolderImap *msgParent = static_cast<KMFolderImap*>(mMsg->parent());
  KMAcctImap *account = msgParent->account();
  if (mMsg->headerField("X-UID").isEmpty())
  {
    emit messageRetrieved(mMsg);
    account->mJobList.remove(this);
    delete this;
    return;
  }
  KURL url = account->getUrl();
  url.setPath(msgParent->imapPath() + ";UID="
    + mMsg->headerField("X-UID"));
  KMAcctImap::jobData jd;
  jd.parent = NULL;
  jd.total = 1; jd.done = 0;
  if (!account->makeConnection())
  {
    account->mJobList.remove(this);
    delete this;
    return;
  }
  KIO::SimpleJob *simpleJob = KIO::get(url, FALSE, FALSE);
  KIO::Scheduler::assignJobToSlave(account->slave(), simpleJob);
  mJob = simpleJob;
  account->mapJobData.insert(mJob, jd);
  connect(mJob, SIGNAL(result(KIO::Job *)),
          this, SLOT(slotGetMessageResult(KIO::Job *)));
  connect(mJob, SIGNAL(data(KIO::Job *, const QByteArray &)),
          msgParent, SLOT(slotSimpleData(KIO::Job *, const QByteArray &)));
  account->displayProgress();
}


//-----------------------------------------------------------------------------
void KMImapJob::slotGetMessageResult(KIO::Job * job)
{
  if( !mMsg || !mMsg->parent() )
    return;
  KMAcctImap *account = static_cast<KMFolderImap*>(mMsg->parent())->account();
  QMap<KIO::Job *, KMAcctImap::jobData>::Iterator it =
    account->mapJobData.find(job);
  if (it == account->mapJobData.end()) return;
  if (job->error())
  {
    account->slotSlaveError( account->slave(), job->error(),
        job->errorText() );
    return;
  } else {
    if ((*it).data.size() > 0)
    {
      QString uid = mMsg->headerField("X-UID");
      (*it).data.resize((*it).data.size() + 1);
      (*it).data[(*it).data.size() - 1] = '\0';
      mMsg->fromString(QCString((*it).data));
      mMsg->setHeaderField("X-UID",uid);
      mMsg->setComplete( TRUE );
      emit messageRetrieved(mMsg);
    } else {
      emit messageRetrieved(NULL);
    }
    mMsg = NULL;
  }
  if (account->slave()) account->mapJobData.remove(it);
  account->displayProgress();
  if (account->slave()) account->mJobList.remove(this);
  delete this;
}


//-----------------------------------------------------------------------------
void KMImapJob::slotPutMessageDataReq(KIO::Job *job, QByteArray &data)
{
  KMAcctImap *account = mDestFolder->account();
  QMap<KIO::Job *, KMAcctImap::jobData>::Iterator it =
    account->mapJobData.find(job);
  if (it == account->mapJobData.end()) return;

  if ((*it).data.size() - (*it).offset > 0x8000)
  {
    data.duplicate((*it).data.data() + (*it).offset, 0x8000);
    (*it).offset += 0x8000;
  }
  else if ((*it).data.size() - (*it).offset > 0)
  {
    data.duplicate((*it).data.data() + (*it).offset, (*it).data.size() - (*it).offset);
    (*it).offset = (*it).data.size();
  } else data.resize(0);
}


//-----------------------------------------------------------------------------
void KMImapJob::slotPutMessageResult(KIO::Job *job)
{
  KMAcctImap *account = mDestFolder->account();
  QMap<KIO::Job *, KMAcctImap::jobData>::Iterator it =
    account->mapJobData.find(job);
  if (it == account->mapJobData.end()) return;
  if (job->error())
  {
    account->slotSlaveError( account->slave(), job->error(),
        job->errorText() );
    return;
  } else {
    if ( !(*it).msgList.isEmpty() )
    {
      emit messageStored((*it).msgList.last());
      (*it).msgList.removeLast();
    } else if (mMsg)
    {
      emit messageStored(mMsg);
    }
    mMsg = NULL;
  }
  if (account->slave()) account->mapJobData.remove(it);
  account->displayProgress();
  if (account->slave()) account->mJobList.remove(this);
  delete this;
}


//-----------------------------------------------------------------------------
void KMImapJob::slotCopyMessageResult(KIO::Job *job)
{
  KMAcctImap *account = mDestFolder->account();
  QMap<KIO::Job *, KMAcctImap::jobData>::Iterator it =
    account->mapJobData.find(job);
  if (it == account->mapJobData.end()) return;
  if (job->error())
  {
    account->slotSlaveError( account->slave(), job->error(),
        job->errorText() );
    return;
  } else {
    if ( !(*it).msgList.isEmpty() )
    {
      emit messageCopied((*it).msgList);
    } else if (mMsg) {
      emit messageCopied(mMsg);
      mMsg = NULL;
    }
  }
  if (account->slave()) account->mapJobData.remove(it);
  account->displayProgress();
  if (account->slave()) account->mJobList.remove(this);
  delete this;
}


//-----------------------------------------------------------------------------
void KMFolderImap::slotSimpleData(KIO::Job * job, const QByteArray & data)
{
  QMap<KIO::Job *, KMAcctImap::jobData>::Iterator it =
    mAccount->mapJobData.find(job);
  if (it == mAccount->mapJobData.end()) return;
  assert(it != mAccount->mapJobData.end());
  QBuffer buff((*it).data);
  buff.open(IO_WriteOnly | IO_Append);
  buff.writeBlock(data.data(), data.size());
  buff.close();
}


//-----------------------------------------------------------------------------
void KMImapJob::ignoreJobsForMessage(KMMessage *msg)
{
  if (!msg || msg->transferInProgress()) return;
  KMAcctImap *account;
  if (!msg->parent() || !(account = static_cast<KMFolderImap*>(msg->parent())
    ->account())) return;
  for (KMImapJob *it = account->mJobList.first(); it;
    it = account->mJobList.next())
  {
    if ((*it).mMsg == msg)
    {
      account->mapJobData.remove( (*it).mJob );
      account->mJobList.remove( it );
      delete it;
      break;
    }
  }
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
  KMAcctImap::jobData jd;
  jd.total = 1; jd.done = 0; jd.parent = NULL; jd.quiet = FALSE;
  mAccount->mapJobData.insert(job, jd);
  connect(job, SIGNAL(result(KIO::Job *)),
          mAccount, SLOT(slotSimpleResult(KIO::Job *)));
  mAccount->displayProgress();
}

void KMFolderImap::deleteMessage(QPtrList<KMMessage> msgList)
{
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
    KMAcctImap::jobData jd;
    jd.total = 1; jd.done = 0; jd.parent = NULL; jd.quiet = FALSE;
    mAccount->mapJobData.insert(job, jd);
    connect(job, SIGNAL(result(KIO::Job *)),
        mAccount, SLOT(slotSimpleResult(KIO::Job *)));
  }
  mAccount->displayProgress();
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
          set += "," + QString::number(*it);
        else
          set += ":" + QString::number(last) + "," + QString::number(*it);
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
    set += ":" + QString::number(uids.last());

  if (!set.isEmpty()) sets.append(set);

  return sets;
}

//-----------------------------------------------------------------------------
void KMFolderImap::getUids(QValueList<int>& ids, QValueList<int>& uids)
{
  KMMessage *msg = NULL;
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
  KMMessage *msg = NULL;

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
  KMAcctImap::jobData jd;
  jd.total = 1; jd.done = 0; jd.parent = NULL;
  mAccount->mapJobData.insert(job, jd);
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
  KMAcctImap::jobData jd;
  jd.parent = NULL;
  jd.quiet = quiet;
  jd.total = 1; jd.done = 0;
  mAccount->mapJobData.insert(job, jd);
  connect(job, SIGNAL(result(KIO::Job *)),
          mAccount, SLOT(slotSimpleResult(KIO::Job *)));
  mAccount->displayProgress();
}


//-----------------------------------------------------------------------------
void KMFolderImap::slotSetStatusResult(KIO::Job * job)
{
  QMap<KIO::Job *, KMAcctImap::jobData>::Iterator it =
    mAccount->mapJobData.find(job);
  if (it == mAccount->mapJobData.end()) return;
  mAccount->mapJobData.remove(it);
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
  KMAcctImap::jobData jd;
  KMAcctImap::initJobData(jd);
  mAccount->mapJobData.insert(job, jd);
  connect(job, SIGNAL(result(KIO::Job *)),
          SLOT(slotStatResult(KIO::Job *)));
  mAccount->displayProgress();
}


//-----------------------------------------------------------------------------
void KMFolderImap::slotStatResult(KIO::Job * job)
{
  QMap<KIO::Job *, KMAcctImap::jobData>::Iterator it =
    mAccount->mapJobData.find(job);
  if (it == mAccount->mapJobData.end()) return;
  mAccount->mapJobData.remove(it);
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
          mGuessedUnreadMsgs = (*it).m_long;
        }
        emit numUnreadMsgsChanged( this );
      }
    }
  }
  mAccount->displayProgress();
}


#include "kmfolderimap.moc"
