/**
 * kmacctimap.cpp
 *
 * Copyright (c) 2000 Michael Haeckel <Michael@Haeckel.Net>
 *
 * This file is based on kmacctexppop.cpp by Don Sanders
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

#include "kmacctimap.moc"

#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <mimelib/mimepp.h>
#include <kmfolder.h>
#include <kmmessage.h>
#include <qtextstream.h>
#include <kconfig.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <kdebug.h>
#include <kapp.h>
#include <kstddirs.h>
#include <qlayout.h>
#include <qdatastream.h>
#include <qbuffer.h>
#include <qqueue.h>
#include <kio/global.h>
#include <kio/scheduler.h>
#include <kio/slave.h>

#include "kmacctimap.h"
#include "kalarmtimer.h"
#include "kmglobal.h"
#include "kbusyptr.h"
#include "kmacctfolder.h"
#include "kmfiltermgr.h"
#include "kmfoldertree.h"
#include <kprocess.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <qtooltip.h>
#include "kmbroadcaststatus.h"

#include <kwin.h>
#include <kbuttonbox.h>

//-----------------------------------------------------------------------------
KMAcctImap::KMAcctImap(KMAcctMgr* aOwner, const char* aAccountName):
  KMAcctImapInherited(aOwner, aAccountName)
{
  initMetaObject();

  init();
  job = NULL;
  mSlave = NULL;
  stage = Idle;
  indexOfCurrentMsg = -1;
  curMsgStrm = 0;
  processingDelay = 2*100;
  mProcessing = false;
  connect(KMBroadcastStatus::instance(), SIGNAL(signalAbortRequested()),
          this, SLOT(slotAbortRequested()));
  KIO::Scheduler::connect(
    SIGNAL(slaveError(KIO::Slave *, int, const QString &)),
    this, SLOT(slotSlaveError(KIO::Slave *, int, const QString &)));
}


//-----------------------------------------------------------------------------
KMAcctImap::~KMAcctImap()
{
  killAllJobs();
}


//-----------------------------------------------------------------------------
const char* KMAcctImap::type(void) const
{
  return "imap";
}


//-----------------------------------------------------------------------------
void KMAcctImap::init(void)
{
  mHost   = "";
  struct servent *serv = getservbyname("imap-4", "tcp");
  if (serv) {
    mPort = ntohs(serv->s_port);
  } else {
    mPort = 143;
  }
  mLogin  = "";
  mPasswd = "";
  mAuth = "*";
  mStorePasswd = FALSE;
  mProgressEnabled = FALSE;
}

//-----------------------------------------------------------------------------
void KMAcctImap::pseudoAssign(KMAccount* account)
{
  assert(account->type() == "imap");
  KMAcctImap *acct = static_cast<KMAcctImap*>(account);
  setName(acct->name());
  setCheckInterval( 0 );
  setCheckExclude( TRUE );
  setFolder(acct->folder());
  setHost(acct->host());
  setPort(acct->port());
  setPrefix(acct->prefix());
  setLogin(acct->login());
  setAuth(acct->auth());
  setHiddenFolders(acct->hiddenFolders());
  setStorePasswd(acct->storePasswd());
  setPasswd(acct->passwd(), acct->storePasswd());
}


//-----------------------------------------------------------------------------
KURL KMAcctImap::getUrl()
{
  KURL url;
  url.setProtocol(QString("imap"));
  url.setUser(mLogin + ";AUTH=" + mAuth);
  url.setPass(decryptStr(mPasswd));
  url.setHost(mHost);
  url.setPort(mPort);
  return url;
}
  

//-----------------------------------------------------------------------------
bool KMAcctImap::makeConnection()
{
  if (mSlave) return TRUE;
  mSlave = KIO::Scheduler::getConnectedSlave(getUrl());
  if (!mSlave) return FALSE;
  return TRUE;
}


//-----------------------------------------------------------------------------
void KMAcctImap::slotSlaveError(KIO::Slave *aSlave, int errorCode,
  const QString &errorMsg)
{
  if (aSlave != mSlave) return;
  if (errorCode == KIO::ERR_SLAVE_DIED) mSlave = NULL;
  KMessageBox::error(0, errorMsg);
}


//-----------------------------------------------------------------------------
void KMAcctImap::displayProgress()
{
  if (mProgressEnabled == mapJobData.isEmpty())
  {
    mProgressEnabled = !mapJobData.isEmpty();
    KMBroadcastStatus::instance()->setStatusProgressEnable( mProgressEnabled );
    if (!mProgressEnabled)
      KMBroadcastStatus::instance()->setStatusMsg(
        i18n("Transmission completed.") );
  }
  int total = 0, done = 0;
  for (QMap<KIO::Job*, jobData>::Iterator it = mapJobData.begin();
    it != mapJobData.end(); it++)
  {
    total += (*it).total;
    done += (*it).done;
  } 
  total += mStatusQueue.count();
  if (total == 0) return;
  KMBroadcastStatus::instance()->setStatusProgressPercent( 100 * done / total );
}


//-----------------------------------------------------------------------------
void KMAcctImap::listDirectory(KMFolderTreeItem * fti, bool secondStep)
{
  jobData jd;
  jd.parent = fti;
  jd.total = 1; jd.done = 0;
  jd.inboxOnly = !secondStep && mPrefix != "/"
    && fti->folder->imapPath() == mPrefix;
  KURL url = getUrl();
  url.setPath((jd.inboxOnly) ? QString("/") : fti->folder->imapPath());
  makeConnection();
  KIO::SimpleJob *job = KIO::listDir(url, FALSE);
  KIO::Scheduler::assignJobToSlave(mSlave, job);
  mapJobData.insert(job, jd);
  connect(job, SIGNAL(result(KIO::Job *)),
          this, SLOT(slotListResult(KIO::Job *)));
  connect(job, SIGNAL(entries(KIO::Job *, const KIO::UDSEntryList &)),
          this, SLOT(slotListEntries(KIO::Job *, const KIO::UDSEntryList &)));
  displayProgress();
}


//-----------------------------------------------------------------------------
void KMAcctImap::slotListResult(KIO::Job * job)
{
  QMap<KIO::Job *, jobData>::Iterator it = mapJobData.find(job);
  if (it == mapJobData.end()) return;
  assert(it != mapJobData.end());
  if (job->error())
  {
    job->showErrorDialog();
    if (job->error() == KIO::ERR_SLAVE_DIED) mSlave = NULL;
  } else if ((*it).inboxOnly) listDirectory((*it).parent, TRUE);
  mapJobData.remove(it);
  displayProgress();
}


//-----------------------------------------------------------------------------
void KMAcctImap::slotListEntries(KIO::Job * job, const KIO::UDSEntryList & uds)
{
  QMap<KIO::Job *, jobData>::Iterator it = mapJobData.find(job);
  if (it == mapJobData.end()) return;
  assert(it != mapJobData.end());
  QString name, mimeType;
  for (KIO::UDSEntryList::ConstIterator udsIt = uds.begin();
    udsIt != uds.end(); udsIt++)
  {
    mimeType = QString::null;
    for (KIO::UDSEntry::ConstIterator eIt = (*udsIt).begin();
      eIt != (*udsIt).end(); eIt++)
    {
      if ((*eIt).m_uds == KIO::UDS_NAME)
        name = (*eIt).m_str;
      else if ((*eIt).m_uds == KIO::UDS_MIME_TYPE)
        mimeType = (*eIt).m_str;
    }
    if ((mimeType == "inode/directory" || mimeType == "message/digest")
        && name != ".." && (mHiddenFolders || name.at(0) != '.')
        && (!(*it).inboxOnly || name == "INBOX"))
    {
      static_cast<KMFolderTree*>((*it).parent->listView())
      ->addImapChildFolder((*it).parent, name, mimeType == "inode/directory",
      (*it).inboxOnly);
    }
  }
  static_cast<KMFolderTree*>((*it).parent->listView())->delayedUpdate();
}


//-----------------------------------------------------------------------------
void KMAcctImap::getFolder(KMFolderTreeItem * fti)
{
  if (fti->folder->count()) fti->folder->expunge();
  fti->mImapState = KMFolderTreeItem::imapInProgress;
  jobData jd;
  jd.parent = fti;
  jd.total = 1; jd.done = 0;
  KURL url = getUrl();
  url.setPath(fti->folder->imapPath());
  url.setQuery("UNDELETED");
  makeConnection();
  KIO::SimpleJob *job = KIO::listDir(url, FALSE);
  KIO::Scheduler::assignJobToSlave(mSlave, job);
  mapJobData.insert(job, jd);
  connect(job, SIGNAL(result(KIO::Job *)),
          this, SLOT(slotListFolderResult(KIO::Job *)));
  connect(job, SIGNAL(entries(KIO::Job *, const KIO::UDSEntryList &)),
          this, SLOT(slotListFolderEntries(KIO::Job *,
          const KIO::UDSEntryList &)));
  displayProgress();
  KMBroadcastStatus::instance()->setStatusMsg(
    i18n("Preparing transmission from %1...").arg(url.host()));
}


//-----------------------------------------------------------------------------
void KMAcctImap::getNextMessage(jobData & jd)
{
  if (jd.items.isEmpty())
  {
    jd.parent->mImapState = KMFolderTreeItem::imapFinished;
    return;
  }
  KURL url = getUrl();
  url.setPath(jd.parent->folder->imapPath() + ";UID=" + *jd.items.begin() +
    ";SECTION=ENVELOPE");
  jd.items.remove(jd.items.begin());
  makeConnection();
  KIO::SimpleJob *job = KIO::get(url, FALSE, FALSE);
  KIO::Scheduler::assignJobToSlave(mSlave, job);
  mapJobData.insert(job, jd);
  connect(job, SIGNAL(result(KIO::Job *)),
          this, SLOT(slotGetMessageResult(KIO::Job *)));
  connect(job, SIGNAL(data(KIO::Job *, const QByteArray &)),
          this, SLOT(slotGetMessageData(KIO::Job *, const QByteArray &)));
}


//-----------------------------------------------------------------------------
void KMAcctImap::slotListFolderResult(KIO::Job * job)
{
  QMap<KIO::Job *, jobData>::Iterator it = mapJobData.find(job);
  if (it == mapJobData.end()) return;
  assert(it != mapJobData.end());
  if (job->error())
  {
    job->showErrorDialog();
    if (job->error() == KIO::ERR_SLAVE_DIED) mSlave = NULL;
    mapJobData.remove(it);
    return;
  }
  jobData jd;
  jd.parent = (*it).parent;
  jd.items = (*it).items;
  jd.total = (*it).items.count(); jd.done = 0;
  QString uids;
  QStringList::ConstIterator uid = (*it).items.begin();
  if (jd.total == 0)
  {
    (*it).parent->mImapState = KMFolderTreeItem::imapFinished;
    mapJobData.remove(it);
    displayProgress();
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
  }
  KURL url = getUrl();
  url.setPath((*it).parent->folder->imapPath() + ";UID=" + uids
    + ";SECTION=ENVELOPE");
  (*it).parent->folder->quiet(TRUE);
  makeConnection();
  KIO::SimpleJob *newJob = KIO::get(url, FALSE, FALSE);
  KIO::Scheduler::assignJobToSlave(mSlave, newJob);
  mapJobData.insert(newJob, jd);
  connect(newJob, SIGNAL(result(KIO::Job *)),
          this, SLOT(slotGetMessagesResult(KIO::Job *)));
  connect(newJob, SIGNAL(data(KIO::Job *, const QByteArray &)),
          this, SLOT(slotGetMessagesData(KIO::Job *, const QByteArray &)));
  mapJobData.remove(it);
}


//-----------------------------------------------------------------------------
void KMAcctImap::slotListFolderEntries(KIO::Job * job,
  const KIO::UDSEntryList & uds)
{
  QMap<KIO::Job *, jobData>::Iterator it = mapJobData.find(job);
  if (it == mapJobData.end()) return;
  assert(it != mapJobData.end());
  for (KIO::UDSEntryList::ConstIterator udsIt = uds.begin();
    udsIt != uds.end(); udsIt++)
  {
    for (KIO::UDSEntry::ConstIterator eIt = (*udsIt).begin();
      eIt != (*udsIt).end(); eIt++)
    {
      if ((*eIt).m_uds == KIO::UDS_NAME)
        (*it).items.append((*eIt).m_str);
    }
  }
}


//-----------------------------------------------------------------------------
void KMAcctImap::slotGetMessagesData(KIO::Job * job, const QByteArray & data)
{
  QMap<KIO::Job *, jobData>::Iterator it = mapJobData.find(job);
  if (it == mapJobData.end()) return;
  assert(it != mapJobData.end());
  (*it).cdata += QCString(data + "\0");
  int pos = (*it).cdata.find("\r\n--IMAPDIGEST");
  if (pos > 0)
  {
    int p = (*it).cdata.find("\r\nX-uidValidity:");
    if (p != -1) (*it).parent->folder->setUidValidity((*it).cdata
      .mid(p + 17, (*it).cdata.find("\r\n", p+1) - p - 17));
    (*it).cdata.remove(0, pos);
  }
  pos = (*it).cdata.find("\r\n--IMAPDIGEST", 1);
  while (pos >= 0)
  {
    KMMessage *msg = new KMMessage;
    msg->fromString((*it).cdata.mid(16, pos - 16).
      replace(QRegExp("\r\n\r\n"),"\r\n"));
    int flags = msg->headerField("X-Flags").toInt();
    if (flags & 2) msg->setStatus(KMMsgStatusReplied); else
    if (flags & 1) msg->setStatus(KMMsgStatusRead);
    KMFolder *kf = (*it).parent->folder;
    kf->addMsg(msg);
    if (kf->count() > 1) kf->unGetMsg(kf->count() - 1);
    if (kf->count() % 100 == 0) { kf->quiet(FALSE); kf->quiet(TRUE); }
    (*it).cdata.remove(0, pos);
    (*it).done++;
    pos = (*it).cdata.find("\r\n--IMAPDIGEST", 1);
  }
  displayProgress();
}


//-----------------------------------------------------------------------------
void KMAcctImap::slotGetMessagesResult(KIO::Job * job)
{
  QMap<KIO::Job *, jobData>::Iterator it = mapJobData.find(job);
  if (it == mapJobData.end()) return;
  assert(it != mapJobData.end());
  if (job->error())
  {
    job->showErrorDialog();
    if (job->error() == KIO::ERR_SLAVE_DIED) mSlave = NULL;
  }
  (*it).parent->mImapState = KMFolderTreeItem::imapFinished;
  (*it).parent->folder->quiet(FALSE);
  mapJobData.remove(it);
  displayProgress();
}


//-----------------------------------------------------------------------------
void KMAcctImap::createFolder(KMFolderTreeItem * fti, const QString &name)
{
  KURL url = getUrl();
  url.setPath(fti->folder->imapPath() + name);
  makeConnection();
  KIO::SimpleJob *job = KIO::mkdir(url);
  KIO::Scheduler::assignJobToSlave(mSlave, job);
  jobData jd;
  jd.parent = fti;
  jd.items = name;
  jd.total = 1; jd.done = 0;
  mapJobData.insert(job, jd);
  displayProgress();
  connect(job, SIGNAL(result(KIO::Job *)),
          this, SLOT(slotCreateFolderResult(KIO::Job *)));
}


//-----------------------------------------------------------------------------
void KMAcctImap::slotCreateFolderResult(KIO::Job * job)
{
  QMap<KIO::Job *, jobData>::Iterator it = mapJobData.find(job);
  assert(it != mapJobData.end());
  if (job->error()) job->showErrorDialog(); else {
    KMFolderTreeItem *fti = new KMFolderTreeItem( (*it).parent,
      new KMFolder((*it).parent->folder->createChildFolder(),
      *(*it).items.begin()), (*it).parent->mPaintInfo );
    if (fti->folder->create())
    {
      fti->folder->remove();
      fti->folder->create();
    }
    fti->setText(0, *(*it).items.begin());
    fti->folder->setAccount( this );
    fti->folder->setImapPath( (*it).parent->folder->imapPath()
      + *(*it).items.begin() + "/" );
    connect(fti->folder,SIGNAL(numUnreadMsgsChanged(KMFolder*)),
            static_cast<KMFolderTree*>(fti->listView()),
            SLOT(refresh(KMFolder*)));
  }
  mapJobData.remove(it);
  displayProgress();
}


//-----------------------------------------------------------------------------
KMImapJob::KMImapJob(QList<KMMessage> msgList, KMFolder *destFolder)
{
  mSingleMessage = false;
  mDestFolder = destFolder;
  mMsgList = msgList;
  msgList.first()->parent()->account()->mJobList.append(this);
  slotGetNextMessage();
}


//-----------------------------------------------------------------------------
KMImapJob::KMImapJob(KMMessage *msg)
{
  mSingleMessage = true;
  mMsgList.append(msg);
  msg->parent()->account()->mJobList.append(this);
  slotGetNextMessage();
}


//-----------------------------------------------------------------------------
void KMImapJob::slotGetNextMessage()
{
  KMAcctImap *account = mMsgList.current()->parent()->account();
  KURL url = account->getUrl();
  url.setPath(mMsgList.current()->parent()->imapPath() + ";UID="
    + mMsgList.current()->headerField("X-UID"));
  KMAcctImap::jobData jd;
  jd.parent = NULL;
  jd.total = mMsgList.count(); jd.done = mMsgList.findRef(mMsgList.current());
  account->makeConnection();
  KIO::SimpleJob *simpleJob = KIO::get(url, FALSE, FALSE);
  KIO::Scheduler::assignJobToSlave(account->slave(), simpleJob);
  mJob = simpleJob;
  account->mapJobData.insert(mJob, jd);
  connect(mJob, SIGNAL(result(KIO::Job *)),
          this, SLOT(slotGetMessageResult(KIO::Job *)));
  connect(mJob, SIGNAL(data(KIO::Job *, const QByteArray &)),
          account, SLOT(slotGetMessageData(KIO::Job *, const QByteArray &)));
  account->displayProgress();
}


//-----------------------------------------------------------------------------
void KMImapJob::slotGetMessageResult(KIO::Job * job)
{
  KMAcctImap *account = mMsgList.current()->parent()->account();
  QMap<KIO::Job *, KMAcctImap::jobData>::Iterator it =
    account->mapJobData.find(job);
  if (it == account->mapJobData.end()) return;
  assert(it != account->mapJobData.end());
  if (job->error())
  {
    job->showErrorDialog();
    if (job->error() == KIO::ERR_SLAVE_DIED) account->slaveDied();
  } else {
    QString uid = mMsgList.current()->headerField("X-UID");
    mMsgList.current()->fromString((*it).data);
    mMsgList.current()->setHeaderField("X-UID",uid);
    mMsgList.current()->setComplete( TRUE );
    if (mMsgList.next())
      slotGetNextMessage();
    else if (mSingleMessage)
      emit messageRetrieved(mMsgList.first());
    else
      emit messagesRetrieved(mMsgList, mDestFolder);
  }
  account->mapJobData.remove(it);
  account->displayProgress();
  account->mJobList.remove(this);
  delete this;
}


//-----------------------------------------------------------------------------
void KMAcctImap::slotGetMessageData(KIO::Job * job, const QByteArray & data)
{
  QMap<KIO::Job *, jobData>::Iterator it = mapJobData.find(job);
  if (it == mapJobData.end()) return;
  assert(it != mapJobData.end());
  QBuffer buff((*it).data);
  buff.open(IO_WriteOnly | IO_Append);
  buff.writeBlock(data.data(), data.size());
  buff.close();
}


//-----------------------------------------------------------------------------
void KMImapJob::killJobsForMessage(KMMessage *msg)
{
  KMAcctImap *account;
  if (!msg->parent() || !(account = msg->parent()->account())) return;
  for (KMImapJob *it = account->mJobList.first(); it;
    it = account->mJobList.next())
  {
    if ((*it).mMsgList.containsRef(msg))
    {
      account->killAllJobs();
      break;
/*      (*it).mJob->kill( TRUE );
      account->mapJobData.remove( (*it).mJob );
      account->mJobList.remove( it );
      delete it;
      account->slaveDied(); */
    }
  }
}


//-----------------------------------------------------------------------------
void KMAcctImap::killAllJobs()
{
  QMap<KIO::Job*, jobData>::Iterator it = mapJobData.begin();
  for (it = mapJobData.begin(); it != mapJobData.end(); it++)
    if ((*it).parent) (*it).parent->folder->quiet(FALSE);
  if (mapJobData.begin() != mapJobData.end())
  {
    mSlave->kill();
    mSlave = NULL;
  }
  mapJobData.clear();
  mJobList.setAutoDelete(true);
  mJobList.clear();
  mJobList.setAutoDelete(false);
}


//-----------------------------------------------------------------------------
void KMAcctImap::killJobsForItem(KMFolderTreeItem * fti)
{
  QMap<KIO::Job *, jobData>::Iterator it = mapJobData.begin();
  while (it != mapJobData.end())
  {
    if (it.data().parent == fti)
    {
      killAllJobs();
      break;
/*      it.key()->kill( TRUE );
      it2 = it;
      it++;
      mapJobData.remove( it2 );
      slaveDied(); */
    }
    else it++;
  }
}


//-----------------------------------------------------------------------------
void KMAcctImap::deleteMessage(KMMessage * msg)
{
  statusData *data = new(statusData);
  data->url = getUrl();
  data->url.setPath(msg->parent()->imapPath() + ";UID=" + 
    msg->headerField("X-UID"));
  data->Delete = TRUE;
  if (mStatusQueue.isEmpty())
  {
    mStatusQueue.enqueue(data);
    nextStatusAction();
  } else mStatusQueue.enqueue(data);
}


//-----------------------------------------------------------------------------
void KMAcctImap::setStatus(KMMessage * msg, KMMsgStatus status)
{
  QCString flags;
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
    default:
      flags = "\\SEEN";
  }
  KURL url = getUrl();
  if (!msg || !msg->parent()) return;
  url.setPath(msg->parent()->imapPath() + ";UID=" + msg->headerField("X-UID"));
  QCString urlStr(url.url());
  QByteArray data;
  QBuffer buff(data);
  buff.open(IO_WriteOnly | IO_Append);
  buff.writeBlock(urlStr.data(), urlStr.size());
  buff.writeBlock(flags.data(), flags.size());
  buff.close();
  makeConnection();
  KIO::SimpleJob *job = KIO::special(url, data, FALSE);
  KIO::Scheduler::assignJobToSlave(mSlave, job);
  jobData jd;
  jd.total = 1; jd.done = 0; jd.parent = NULL;
  mapJobData.insert(job, jd);
  connect(job, SIGNAL(result(KIO::Job *)),
          this, SLOT(slotSimpleResult(KIO::Job *)));
  displayProgress();
}


//-----------------------------------------------------------------------------
void KMAcctImap::nextStatusAction()
{
  displayProgress();
  if (mStatusQueue.isEmpty()) return;
  statusData *data = mStatusQueue.dequeue();
  if (data->Delete)
  {
    makeConnection();
    KIO::SimpleJob *job = KIO::file_delete(data->url, FALSE);
    KIO::Scheduler::assignJobToSlave(mSlave, job);
    jobData jd;
    jd.total = 0; jd.done = 0; jd.parent = NULL;
    mapJobData.insert(job, jd);
    connect(job, SIGNAL(result(KIO::Job *)),
            this, SLOT(slotSimpleResult(KIO::Job *)));
  }
}


//-----------------------------------------------------------------------------
void KMAcctImap::slotStatusResult(KIO::Job * job)
{
  QMap<KIO::Job *, jobData>::Iterator it = mapJobData.find(job);
  if (it == mapJobData.end()) return;
  assert(it != mapJobData.end());
  mapJobData.remove(it);
  if (job->error())
  { 
    job->showErrorDialog();
    if (job->error() == KIO::ERR_SLAVE_DIED) mSlave = NULL;
    mStatusQueue.clear();
  }
  nextStatusAction();
}


//-----------------------------------------------------------------------------
void KMAcctImap::expungeFolder(KMFolder * aFolder)
{
  KURL url = getUrl();
  url.setPath(aFolder->imapPath() + ";UID=*");
  makeConnection();
  KIO::SimpleJob *job = KIO::file_delete(url, FALSE);
  KIO::Scheduler::assignJobToSlave(mSlave, job);
  jobData jd;
  jd.parent = NULL;
  jd.total = 1; jd.done = 0;
  mapJobData.insert(job, jd);
  connect(job, SIGNAL(result(KIO::Job *)),
          this, SLOT(slotSimpleResult(KIO::Job *)));
  displayProgress();
}


//-----------------------------------------------------------------------------
void KMAcctImap::slotSimpleResult(KIO::Job * job)
{
  QMap<KIO::Job *, jobData>::Iterator it = mapJobData.find(job);
  if (it == mapJobData.end()) return;
  mapJobData.remove(it);
  if (job->error())
  {
    job->showErrorDialog();
    if (job->error() == KIO::ERR_SLAVE_DIED) mSlave = NULL;
  }
  displayProgress();
}


//-----------------------------------------------------------------------------
void KMAcctImap::slotAbortRequested()
{
  killAllJobs();
  displayProgress();
}

//-----------------------------------------------------------------------------
void KMAcctImap::readConfig(KConfig& config)
{
  KMAcctImapInherited::readConfig(config);

  mLogin = config.readEntry("login", "");
  mStorePasswd = config.readNumEntry("store-passwd", TRUE);
  if (mStorePasswd) mPasswd = config.readEntry("passwd");
  else mPasswd = "";
  mHost = config.readEntry("host");
  mPort = config.readNumEntry("port");
  mAuth = config.readEntry("auth", "*");
  mPrefix = config.readEntry("prefix", "/");
  mHiddenFolders = config.readBoolEntry("hidden-folders", FALSE);
}


//-----------------------------------------------------------------------------
void KMAcctImap::writeConfig(KConfig& config)
{
  KMAcctImapInherited::writeConfig(config);

  config.writeEntry("login", mLogin);
  config.writeEntry("store-passwd", mStorePasswd);
  if (mStorePasswd) config.writeEntry("passwd", mPasswd);
  else config.writeEntry("passwd", "");

  config.writeEntry("host", mHost);
  config.writeEntry("port", static_cast<int>(mPort));
  config.writeEntry("auth", mAuth);
  config.writeEntry("prefix", mPrefix);
  config.writeEntry("hidden-folders", mHiddenFolders);
}


//-----------------------------------------------------------------------------
const QString KMAcctImap::encryptStr(const QString aStr) const
{
  unsigned int i, val;
  unsigned int len = aStr.length();
  QCString result;
  result.resize(len+1);

  for (i=0; i<len; i++)
  {
    val = aStr[i] - ' ';
    val = (255-' ') - val;
    result[i] = (char)(val + ' ');
  }
  result[i] = '\0';

  return result;
}


//-----------------------------------------------------------------------------
const QString KMAcctImap::decryptStr(const QString aStr) const
{
  return encryptStr(aStr);
}


//-----------------------------------------------------------------------------
void KMAcctImap::setStorePasswd(bool b)
{
  mStorePasswd = b;
}


//-----------------------------------------------------------------------------
void KMAcctImap::setLogin(const QString& aLogin)
{
  mLogin = aLogin;
}


//-----------------------------------------------------------------------------
const QString KMAcctImap::passwd(void) const
{
  return decryptStr(mPasswd);
}


//-----------------------------------------------------------------------------
void KMAcctImap::setPasswd(const QString& aPasswd, bool aStoreInConfig)
{
  mPasswd = encryptStr(aPasswd);
  mStorePasswd = aStoreInConfig;
}


//-----------------------------------------------------------------------------
void KMAcctImap::setHost(const QString& aHost)
{
  mHost = aHost;
}


//-----------------------------------------------------------------------------
void KMAcctImap::setPort(unsigned short int aPort)
{
  mPort = aPort;
}


//-----------------------------------------------------------------------------
void KMAcctImap::setPrefix(const QString& aPrefix)
{
  mPrefix = aPrefix;
  if (mPrefix.isEmpty() || mPrefix.at(0) != '/') mPrefix = '/' + mPrefix;
  if (mPrefix.at(mPrefix.length() - 1) != '/') mPrefix += '/';
}


//-----------------------------------------------------------------------------
void KMAcctImap::setHiddenFolders(bool aHiddenFolders)
{
  mHiddenFolders = aHiddenFolders;
}


//-----------------------------------------------------------------------------
void KMAcctImap::setAuth(const QString& aAuth)
{
  mAuth = aAuth;
}


//=============================================================================
//
//  Class  KMImapPasswdDialog
//
//=============================================================================

KMImapPasswdDialog::KMImapPasswdDialog(QWidget *parent, const char *name,
			             KMAcctImap *account ,
				     const QString caption,
			             const char *login, QString passwd)
  :QDialog(parent,name,true)
{
  // This function pops up a little dialog which asks you
  // for a new username and password if one of them was wrong or not set.
  QLabel *l;

  kernel->kbp()->idle();
  act = account;
  KWin::setIcons(winId(), kapp->icon(), kapp->miniIcon());
  if (!caption.isNull())
    setCaption(caption);

  QGridLayout *gl = new QGridLayout(this, 5, 2, 10);

  QPixmap pix(locate("data", QString::fromLatin1("kdeui/pics/keys.png")));
  if(!pix.isNull()) {
    l = new QLabel(this);
    l->setPixmap(pix);
    l->setFixedSize(l->sizeHint());
    gl->addWidget(l, 0, 0);
  }

  l = new QLabel(i18n("You need to supply a username and a\n"
		      "password to access this mailbox."),
		 this);
  l->setFixedSize(l->sizeHint());
  gl->addWidget(l, 0, 1);

  l = new QLabel(i18n("Server:"), this);
  l->setMinimumSize(l->sizeHint());
  gl->addWidget(l, 1, 0);

  l = new QLabel(act->host(), this);
  l->setMinimumSize(l->sizeHint());
  gl->addWidget(l, 1, 1);

  l = new QLabel(i18n("Login Name:"), this);
  l->setMinimumSize(l->sizeHint());
  gl->addWidget(l, 2, 0);

  usernameLEdit = new QLineEdit(login, this);
  usernameLEdit->setFixedHeight(usernameLEdit->sizeHint().height());
  usernameLEdit->setMinimumWidth(usernameLEdit->sizeHint().width());
  gl->addWidget(usernameLEdit, 2, 1);

  l = new QLabel(i18n("Password:"), this);
  l->setMinimumSize(l->sizeHint());
  gl->addWidget(l, 3, 0);

  passwdLEdit = new QLineEdit(this,"NULL");
  passwdLEdit->setEchoMode(QLineEdit::Password);
  passwdLEdit->setText(passwd);
  passwdLEdit->setFixedHeight(passwdLEdit->sizeHint().height());
  passwdLEdit->setMinimumWidth(passwdLEdit->sizeHint().width());
  gl->addWidget(passwdLEdit, 3, 1);
  connect(passwdLEdit, SIGNAL(returnPressed()),
	  SLOT(slotOkPressed()));

  KButtonBox *bbox = new KButtonBox(this);
  bbox->addStretch(1);
  ok = bbox->addButton(i18n("OK"));
  ok->setDefault(true);
  cancel = bbox->addButton(i18n("Cancel"));
  bbox->layout();
  gl->addMultiCellWidget(bbox, 4, 4, 0, 1);

  connect(ok, SIGNAL(pressed()),
	  this, SLOT(slotOkPressed()));
  connect(cancel, SIGNAL(pressed()),
	  this, SLOT(slotCancelPressed()));

  if(strlen(login) > 0)
    passwdLEdit->setFocus();
  else
    usernameLEdit->setFocus();
  gl->activate();
}

//-----------------------------------------------------------------------------
void KMImapPasswdDialog::slotOkPressed()
{
  act->setLogin(usernameLEdit->text());
  act->setPasswd(passwdLEdit->text(), act->storePasswd());
  done(1);
}

//-----------------------------------------------------------------------------
void KMImapPasswdDialog::slotCancelPressed()
{
  done(0);
}

void KMAcctImap::connectJob() {
  connect(job, SIGNAL( data( KIO::Job*, const QByteArray &)),
	  SLOT( slotData( KIO::Job*, const QByteArray &)));
  connect( job, SIGNAL( result( KIO::Job * ) ),
	   SLOT( slotResult( KIO::Job * ) ) );
}



