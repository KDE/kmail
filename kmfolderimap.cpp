/**
 * kmfolderimap.cpp
 *
 * Copyright (c) 2001 Kurt Granroth <granroth@kde.org>
 * Copyright (c) 2000 Michael Haeckel <Michael@Haeckel.Net>
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
#include <qregexp.h>

KMFolderImap::KMFolderImap(KMFolderDir* aParent, const QString& aName)
  : KMFolderImapInherited(aParent, aName)
{
  mImapState = imapNoInformation;
  mAccount = NULL;
  mIsSelected = FALSE;

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
}

KMFolderImap::~KMFolderImap()
{
  KConfig* config = kapp->config();
  KConfigGroupSaver saver(config, "Folder-" + idString());
  config->writeEntry("UidValidity", mUidValidity);
  config->writeEntry("ImapPath", mImapPath);
  config->writeEntry("NoContent", mNoContent);

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
  mAccount->makeConnection();
  KIO::SimpleJob *job = KIO::file_delete(url);
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
    job->showErrorDialog();
    if (job->error() == KIO::ERR_SLAVE_DIED) mAccount->slaveDied();
  }
  mAccount->mapJobData.remove(it);
  mAccount->displayProgress();
  if (!job->error()) kernel->imapFolderMgr()->remove(this);
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

  KMFolderImapInherited::removeMsg(idx);
}


//-----------------------------------------------------------------------------
void KMFolderImap::addMsgQuiet(KMMessage* aMsg)
{
  KMFolder *folder = aMsg->parent();
  kernel->undoStack()->pushAction( aMsg->getMsgSerNum(), folder, this );
  if (folder) folder->take(folder->find(aMsg));
  delete aMsg;
}

//-----------------------------------------------------------------------------
int KMFolderImap::addMsg(KMMessage* aMsg, int* aIndex_ret)
{
  KMFolder *msgParent = aMsg->parent();
  if (msgParent)
  {
    int idx = msgParent->find(aMsg);
    msgParent->getMsg( idx );
    if (msgParent->protocol() == "imap")
    {
      if (static_cast<KMFolderImap*>(msgParent)->account() == account())
      {
				KMImapJob *imapJob = NULL;
				if (this == msgParent) {
        	imapJob = new KMImapJob(aMsg, KMImapJob::tPutMessage, this);
				} else {	
        	imapJob = new KMImapJob(aMsg, KMImapJob::tCopyMessage, this);
				}	
				connect(imapJob, SIGNAL(messageCopied(KMMessage*)),
          SLOT(addMsgQuiet(KMMessage*)));
        aMsg->setTransferInProgress(TRUE);
				if (this == msgParent) this->getFolder();
        if (aIndex_ret) *aIndex_ret = -1;
        return 0;
      }
      else if (!canAddMsgNow(aMsg, aIndex_ret)) return 0;
    }
  }
  aMsg->setTransferInProgress(TRUE);
  KMImapJob *imapJob = new KMImapJob(aMsg, KMImapJob::tPutMessage, this);
  connect(imapJob, SIGNAL(messageStored(KMMessage*)),
    SLOT(addMsgQuiet(KMMessage*)));
  if (aIndex_ret) *aIndex_ret = -1;
  return 0;

  KMFolderImapInherited::addMsg(aMsg, aIndex_ret);
}

//-----------------------------------------------------------------------------
KMMessage* KMFolderImap::take(int idx)
{
  KMMsgBase* mb(mMsgList[idx]);
  if (!mb) return NULL;
  if (!mb->isMessage()) readMsg(idx);

  KMMessage *msg = static_cast<KMMessage*>(mb);
  deleteMessage(msg);

  return KMFolderImapInherited::take(idx);
}

//-----------------------------------------------------------------------------
// Originally from KMAcctImap
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void KMFolderImap::listDirectory(KMFolderTreeItem * fti, bool secondStep)
{
  kdDebug(5006) << "KMFolderImap::listDirectory " << label() << ", "
    << secondStep << endl;
  mImapState = imapInProgress;
  KMAcctImap::jobData jd;
  jd.parent = this;
  jd.total = 1; jd.done = 0;
  jd.inboxOnly = !secondStep && mAccount->prefix() != "/"
    && imapPath() == mAccount->prefix();
  KURL url = mAccount->getUrl();
  url.setPath(((jd.inboxOnly) ? QString("/") : imapPath())
    + ";TYPE=" + ((mAccount->onlySubscribedFolders()) ? "LSUB" : "LIST"));
  if (!mAccount->makeConnection())
  { 
    if (fti) fti->setOpen( FALSE );
    return;
  }
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
    job->showErrorDialog();
    if (job->error() == KIO::ERR_SLAVE_DIED) mAccount->slaveDied();
  }
  mImapState = imapFinished;
  bool it_inboxOnly = (*it).inboxOnly;
  mAccount->mapJobData.remove(it);
  if (!job->error())
  {
    kernel->imapFolderMgr()->quiet(TRUE);
    if (it_inboxOnly) listDirectory(NULL, TRUE);
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
        folder->close();
        folder->listDirectory(NULL);
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
          folder->close();
          kernel->imapFolderMgr()->contentsChanged();
        }
        folder->setAccount(mAccount);
        folder->setNoContent(mSubfolderMimeTypes[i] == "inode/directory");
        folder->setImapPath(mSubfolderPaths[i]);
        if (mSubfolderMimeTypes[i] == "message/directory" ||
            mSubfolderMimeTypes[i] == "inode/directory")
          folder->listDirectory(NULL);
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
  QString name, url, mimeType;
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
        url = (*eIt).m_str;
      else if ((*eIt).m_uds == KIO::UDS_MIME_TYPE)
        mimeType = (*eIt).m_str;
    }
    if ((mimeType == "inode/directory" || mimeType == "message/digest"
        || mimeType == "message/directory")
        && name != ".." && (mAccount->hiddenFolders() || name.at(0) != '.')
        && (!(*it).inboxOnly || name == "INBOX"))
    {
kdDebug() << "path = " << KURL(url).path() << endl;
      if (((*it).inboxOnly || KURL(url).path() == "/INBOX/") && name == "INBOX")
        mHasInbox = TRUE;
      else {
        mSubfolderNames.append(name);
        mSubfolderPaths.append(KURL(url).path());
        mSubfolderMimeTypes.append(mimeType);
      }
/*      static_cast<KMFolderTree*>((*it).parent->listView())
        ->addImapChildFolder((*it).parent, name, KURL(url).path(),
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
  mAccount->makeConnection();
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
void KMFolderImap::slotCheckValidityResult(KIO::Job * job)
{
kdDebug(5006) << "KMFolderImap::slotCheckValidityResult" << endl;
  QMap<KIO::Job *, KMAcctImap::jobData>::Iterator it =
    mAccount->mapJobData.find(job);
  if (it == mAccount->mapJobData.end()) return;
  if (job->error())
  {
    job->showErrorDialog();
    if (job->error() == KIO::ERR_SLAVE_DIED) mAccount->slaveDied();
    emit folderComplete(this, FALSE);
    mAccount->mapJobData.remove(it);
    mAccount->displayProgress();
  } else {
    QString startUid = uidNext();
    setUidNext("");
    QCString cstr((*it).data.data(), (*it).data.size() + 1);
    int a = cstr.find("X-uidValidity: ");
    int  b = cstr.find("\r\n", a);
		QString uidv;
		if ( (b - a - 15) >= 0 ) uidv = cstr.mid(a + 15, b - a - 15);
    if (uidValidity() != uidv)
    {
      expunge();
      startUid = "";
    } else {
      int p = cstr.find("\r\nX-UidNext:");
      if (p != -1) setUidNext(cstr
        .mid(p + 13, cstr.find("\r\n", p+1) - p - 13));
			kdDebug(5006) << "uidnext = " << uidNext() << endl;
   	}
    mAccount->mapJobData.remove(it);
    if (startUid.isEmpty() || startUid != uidNext())
      reallyGetFolder(startUid);
    else {
      mImapState = imapFinished;
      mAccount->displayProgress();
    }
  }
}


//-----------------------------------------------------------------------------
void KMFolderImap::getFolder()
{
  if (mNoContent) return;
  mImapState = imapInProgress;
  checkValidity();
}


//-----------------------------------------------------------------------------
void KMFolderImap::reallyGetFolder(const QString &startUid)
{
  KMAcctImap::jobData jd;
  jd.parent = this;
  jd.total = 1; jd.done = 0;
  KURL url = mAccount->getUrl();
  if (startUid.isEmpty())
  {
    url.setPath(imapPath() + ";SECTION=UID FLAGS");
    mAccount->makeConnection();
    KIO::SimpleJob *job = KIO::listDir(url, FALSE);
    KIO::Scheduler::assignJobToSlave(mAccount->slave(), job);
    mAccount->mapJobData.insert(job, jd);
    connect(job, SIGNAL(result(KIO::Job *)),
            this, SLOT(slotListFolderResult(KIO::Job *)));
    connect(job, SIGNAL(entries(KIO::Job *, const KIO::UDSEntryList &)),
            this, SLOT(slotListFolderEntries(KIO::Job *,
            const KIO::UDSEntryList &)));
  } else {
    url.setPath(imapPath() + ";UID=" + startUid
      + ":*;SECTION=ENVELOPE");
    mAccount->makeConnection();
    KIO::SimpleJob *newJob = KIO::get(url, FALSE, FALSE);
    KIO::Scheduler::assignJobToSlave(mAccount->slave(), newJob);
    mAccount->mapJobData.insert(newJob, jd);
    connect(newJob, SIGNAL(result(KIO::Job *)),
            this, SLOT(slotGetMessagesResult(KIO::Job *)));
    connect(newJob, SIGNAL(data(KIO::Job *, const QByteArray &)),
            this, SLOT(slotGetMessagesData(KIO::Job *, const QByteArray &)));
  }
  mAccount->displayProgress();
}


//-----------------------------------------------------------------------------
void KMFolderImap::getNextMessage(KMAcctImap::jobData & jd)
{
  if (jd.items.isEmpty())
  {
    mImapState = imapFinished;
    return;
  }
  KURL url = mAccount->getUrl();
  url.setPath(imapPath() + ";UID=" + *jd.items.begin() +
    ";SECTION=ENVELOPE");
  jd.items.remove(jd.items.begin());
  mAccount->makeConnection();
  KIO::SimpleJob *job = KIO::get(url, FALSE, FALSE);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), job);
  mAccount->mapJobData.insert(job, jd);
  connect(job, SIGNAL(result(KIO::Job *)),
          this, SLOT(slotGetMessageResult(KIO::Job *)));
  connect(job, SIGNAL(data(KIO::Job *, const QByteArray &)),
          this, SLOT(slotSimpleData(KIO::Job *, const QByteArray &)));
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
    job->showErrorDialog();
    if (job->error() == KIO::ERR_SLAVE_DIED) mAccount->slaveDied();
    emit folderComplete(this, FALSE);
    mAccount->mapJobData.remove(it);
    return;
  }
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
    mImapState = imapFinished;
    emit folderComplete(this, TRUE);
    mAccount->mapJobData.remove(it);
    mAccount->displayProgress();
    return;
  }
  // Force digest mode, even if there is only one message in the folder
  if (jd.total == 1) uids = *uid + ":" + *uid;
  else while (uid != (*it).items.end())
  {
    int first = (*uid).toInt();
    int last = first - 1;
    while (uid != (*it).items.end() && (*uid).toInt() == last + 1)
    {
      last = (*uid).toInt();
      uid++;
    }
    if (!uids.isEmpty()) uids += ",";
    if (first == last)
      uids += QString::number(first);
    else
      uids += QString::number(first) + ":" + QString::number(last);

    /* Workaround for a bug in the Courier IMAP server */
    if (uids.length() > 100 && uid != (*it).items.end())
    {
      KURL url = mAccount->getUrl();
      url.setPath(imapPath() + ";UID=" + uids + ";SECTION=ENVELOPE");
      mAccount->makeConnection();
      KIO::SimpleJob *newJob = KIO::get(url, FALSE, FALSE);
      KIO::Scheduler::assignJobToSlave(mAccount->slave(), newJob);
      KMAcctImap::jobData jd2 = jd;
      jd2.total = 0;
      jd2.quiet = FALSE;
      mAccount->mapJobData.insert(newJob, jd2);
      connect(newJob, SIGNAL(result(KIO::Job *)),
          mAccount, SLOT(slotSimpleResult(KIO::Job *)));
      connect(newJob, SIGNAL(data(KIO::Job *, const QByteArray &)),
          this, SLOT(slotGetMessagesData(KIO::Job *, const QByteArray &)));
      uids = "";
    }
    /* end workaround */
  }
  KURL url = mAccount->getUrl();
  url.setPath(imapPath() + ";UID=" + uids + ";SECTION=ENVELOPE");
  mAccount->makeConnection();
  KIO::SimpleJob *newJob = KIO::get(url, FALSE, FALSE);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), newJob);
  mAccount->mapJobData.insert(newJob, jd);
  connect(newJob, SIGNAL(result(KIO::Job *)),
          this, SLOT(slotGetMessagesResult(KIO::Job *)));
  connect(newJob, SIGNAL(data(KIO::Job *, const QByteArray &)),
          this, SLOT(slotGetMessagesData(KIO::Job *, const QByteArray &)));
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
    p = (*it).cdata.find("\r\nX-UidNext:");
    if (p != -1) setUidNext((*it).cdata
      .mid(p + 13, (*it).cdata.find("\r\n", p+1) - p - 13));
    (*it).cdata.remove(0, pos);
  }
  pos = (*it).cdata.find("\r\n--IMAPDIGEST", 1);
  int flags;
  while (pos >= 0)
  {
    KMMessage *msg = new KMMessage;
    msg->fromString((*it).cdata.mid(16, pos - 16));
    flags = msg->headerField("X-Flags").toInt();
    if (flags & 8) delete msg;
    else {
      msg->setStatus(flagsToStatus(flags));
      KMFolderImapInherited::addMsg(msg, NULL);
      if (count() > 1) unGetMsg(count() - 1);
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
void KMFolderImap::slotGetMessagesResult(KIO::Job * job)
{
  QMap<KIO::Job *, KMAcctImap::jobData>::Iterator it =
    mAccount->mapJobData.find(job);
  if (it == mAccount->mapJobData.end()) return;
  assert(it != mAccount->mapJobData.end());
  if (job->error())
  {
    job->showErrorDialog();
    if (job->error() == KIO::ERR_SLAVE_DIED) mAccount->slaveDied();
    mImapState = imapNoInformation;
    emit folderComplete(this, FALSE);
  } else mImapState = imapFinished;
  quiet(FALSE);
  mAccount->mapJobData.remove(it);
  mAccount->displayProgress();
  if (!job->error()) emit folderComplete(this, TRUE);
}


//-----------------------------------------------------------------------------
void KMFolderImap::createFolder(const QString &name)
{
  KURL url = mAccount->getUrl();
  url.setPath(imapPath() + name);
  mAccount->makeConnection();
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
    job->showErrorDialog();
    if (job->error() == KIO::ERR_SLAVE_DIED) mAccount->slaveDied();
  } else {
    listDirectory(NULL);
  }
  mAccount->mapJobData.remove(it);
  mAccount->displayProgress();
}


//-----------------------------------------------------------------------------
KMImapJob::KMImapJob(KMMessage *msg, JobType jt, KMFolderImap* folder)
{
  assert(jt == tGetMessage || folder);
  mType = jt;
  mDestFolder = folder;
  mMsg = msg;
  KMFolderImap *msg_parent = static_cast<KMFolderImap*>(msg->parent());
  KMAcctImap *account = (folder) ? folder->account() : msg_parent->account();
  account->mJobList.append(this);
  if (jt == tPutMessage)
  {
    KURL url = account->getUrl();
    url.setPath(folder->imapPath());
    KMAcctImap::jobData jd;
    jd.parent = NULL; mOffset = 0;
    jd.total = 1; jd.done = 0;
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
    account->makeConnection();
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
    url.setPath(msg_parent->imapPath() + ";UID="
      + msg->headerField("X-UID"));
    KURL destUrl = account->getUrl();
    destUrl.setPath(folder->imapPath());
    KMAcctImap::jobData jd;
    jd.parent = NULL; mOffset = 0;
    jd.total = 1; jd.done = 0;
    QCString urlStr("C" + url.url().utf8());
    QByteArray data;
    QBuffer buff(data);
    buff.open(IO_WriteOnly | IO_Append);
    buff.writeBlock(urlStr.data(), urlStr.size());
    urlStr = destUrl.url().utf8();
    buff.writeBlock(urlStr.data(), urlStr.size());
    buff.close();
    account->makeConnection();
    KIO::SimpleJob *simpleJob = KIO::special(url, data, FALSE);
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
  if (mMsg) mMsg->setTransferInProgress( FALSE );
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
  account->makeConnection();
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
  KMAcctImap *account = static_cast<KMFolderImap*>(mMsg->parent())->account();
  QMap<KIO::Job *, KMAcctImap::jobData>::Iterator it =
    account->mapJobData.find(job);
  if (it == account->mapJobData.end()) return;
  if (job->error())
  {
    job->showErrorDialog();
    if (job->error() == KIO::ERR_SLAVE_DIED) account->slaveDied();
  } else {
    QString uid = mMsg->headerField("X-UID");
    (*it).data.resize((*it).data.size() + 1);
    (*it).data[(*it).data.size() - 1] = '\0';
    mMsg->fromString(QCString((*it).data));
    mMsg->setHeaderField("X-UID",uid);
    mMsg->setComplete( TRUE );
    emit messageRetrieved(mMsg);
    mMsg = NULL;
  }
  account->mapJobData.remove(it);
  account->displayProgress();
  account->mJobList.remove(this);
  delete this;
}


//-----------------------------------------------------------------------------
void KMImapJob::slotPutMessageDataReq(KIO::Job *job, QByteArray &data)
{
  assert(mJob == job);
  if (mData.size() - mOffset > 0x8000)
  {
    data.duplicate(mData.data() + mOffset, 0x8000);
    mOffset += 0x8000;
  }
  else if (mData.size() - mOffset > 0)
  {
    data.duplicate(mData.data() + mOffset, mData.size() - mOffset);
    mOffset = mData.size();
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
    job->showErrorDialog();
    if (job->error() == KIO::ERR_SLAVE_DIED) account->slaveDied();
  } else {
    emit messageStored(mMsg);
    mMsg = NULL;
  }
  account->mapJobData.remove(it);
  account->displayProgress();
  account->mJobList.remove(this);
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
    job->showErrorDialog();
    if (job->error() == KIO::ERR_SLAVE_DIED) account->slaveDied();
  } else {
    emit messageCopied(mMsg);
    mMsg = NULL;
  }
  account->mapJobData.remove(it);
  account->displayProgress();
  account->mJobList.remove(this);
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
  mAccount->makeConnection();
  KIO::SimpleJob *job = KIO::file_delete(url, FALSE);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), job);
  KMAcctImap::jobData jd;
  jd.total = 1; jd.done = 0; jd.parent = NULL; jd.quiet = FALSE;
  mAccount->mapJobData.insert(job, jd);
  connect(job, SIGNAL(result(KIO::Job *)),
          mAccount, SLOT(slotSimpleResult(KIO::Job *)));
  mAccount->displayProgress();
}


//-----------------------------------------------------------------------------
void KMFolderImap::setStatus(KMMessage * msg, KMMsgStatus status)
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
  KURL url = mAccount->getUrl();
  if (!msg || !msg->parent()) return;
  KMFolderImap *msg_parent = static_cast<KMFolderImap*>(msg->parent());
  url.setPath(msg_parent->imapPath() + ";UID=" + msg->headerField("X-UID"));
  QCString urlStr("S" + url.url().utf8());
  QByteArray data;
  QBuffer buff(data);
  buff.open(IO_WriteOnly | IO_Append);
  buff.writeBlock(urlStr.data(), urlStr.size());
  buff.writeBlock(flags.data(), flags.size());
  buff.close();
  mAccount->makeConnection();
  KIO::SimpleJob *job = KIO::special(url, data, FALSE);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), job);
  KMAcctImap::jobData jd;
  jd.total = 1; jd.done = 0; jd.parent = NULL;
  mAccount->mapJobData.insert(job, jd);
  connect(job, SIGNAL(result(KIO::Job *)),
          SLOT(slotSetStatusResult(KIO::Job *)));
  mAccount->displayProgress();
}


//-----------------------------------------------------------------------------
void KMFolderImap::expungeFolder(KMFolderImap * aFolder, bool quiet)
{
  aFolder->setNeedsCompacting(FALSE);
  KURL url = mAccount->getUrl();
  url.setPath(aFolder->imapPath() + ";UID=*");
  mAccount->makeConnection();
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
    job->showErrorDialog();
    if (job->error() == KIO::ERR_SLAVE_DIED) mAccount->slaveDied();
  }
  mAccount->displayProgress();
}


//-----------------------------------------------------------------------------
void KMFolderImap::processNewMail(bool)
{
  KURL url = mAccount->getUrl();
  url.setPath(imapPath() + ";SECTION=UNSEEN");
  mAccount->makeConnection();
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
    job->showErrorDialog();
    if (job->error() == KIO::ERR_SLAVE_DIED) mAccount->slaveDied();
  } else {
    KIO::UDSEntry uds = static_cast<KIO::StatJob*>(job)->statResult();
    for (KIO::UDSEntry::ConstIterator it = uds.begin(); it != uds.end(); it++)
    {
      if ((*it).m_uds == KIO::UDS_SIZE)
      {
        mUnreadMsgs = (*it).m_long;
        emit numUnreadMsgsChanged( this );
      }
    }
  }
  mAccount->displayProgress();
}


#include "kmfolderimap.moc"
