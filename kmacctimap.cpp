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
  job = 0L;
  stage = Idle;
  indexOfCurrentMsg = -1;
  curMsgStrm = 0;
  processingDelay = 2*100;
  mProcessing = false;
  connect(KMBroadcastStatus::instance(), SIGNAL(signalAbortRequested()),
          this, SLOT(slotAbortRequested()));
}


//-----------------------------------------------------------------------------
KMAcctImap::~KMAcctImap()
{
  QMap<KIO::Job*, jobData>::Iterator it = mapJobData.begin();
  while ( it != mapJobData.end() )
  {
    it.key()->kill( TRUE );
    mapJobData.remove( it );
    it = mapJobData.begin();
  }                                                                             
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
void KMAcctImap::listDirectory(KMFolderTreeItem * fti)
{
  if (fti->folder->imapPath() == mPrefix)
    static_cast<KMFolderTree*>(fti->listView())
    ->addImapChildFolder(fti, "INBOX", FALSE, TRUE);
  jobData jd;
  jd.parent = fti;
  jd.total = 1; jd.done = 0;
  KURL url = getUrl();
  url.setPath(fti->folder->imapPath());
  KIO::Job *job = KIO::listDir(url, FALSE);
  mapJobData.insert(job, jd);
  connect(job, SIGNAL(result(KIO::Job *)),
          this, SLOT(slotListResult(KIO::Job *)));
  connect(job, SIGNAL(entries(KIO::Job *, const KIO::UDSEntryList &)),
          this, SLOT(slotListEntries(KIO::Job *, const KIO::UDSEntryList &)));
}


//-----------------------------------------------------------------------------
void KMAcctImap::slotListResult(KIO::Job * job)
{
  QMap<KIO::Job *, jobData>::Iterator it = mapJobData.find(job);
  assert(it != mapJobData.end());
  if (job->error()) job->showErrorDialog();
  mapJobData.remove(it);
}


//-----------------------------------------------------------------------------
void KMAcctImap::slotListEntries(KIO::Job * job, const KIO::UDSEntryList & uds)
{
  QMap<KIO::Job *, jobData>::Iterator it = mapJobData.find(job);
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
        && name != ".." && name != "INBOX")
    {
      static_cast<KMFolderTree*>((*it).parent->listView())
      ->addImapChildFolder((*it).parent, name, mimeType == "inode/directory",
      FALSE);
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
  KIO::Job *job = KIO::listDir(url, FALSE);
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
  KIO::Job *job = KIO::get(url, FALSE, FALSE);
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
  assert(it != mapJobData.end());
  if (job->error()) { job->showErrorDialog(); mapJobData.remove(it); return; }
  jobData jd;
  jd.parent = (*it).parent;
  jd.items = (*it).items;
  jd.total = (*it).items.count(); jd.done = 0;
  QString uids;
  QStringList::ConstIterator uid = (*it).items.begin();
  // Force digest mode, even if there is only one message in the folder
  if (jd.total == 1) uids = *uid + "," + *uid;
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
  KIO::Job *newJob = KIO::get(url, FALSE, FALSE);
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
  assert(it != mapJobData.end());
  if (job->error()) job->showErrorDialog();
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
  KIO::Job *job = KIO::mkdir(url);
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
KMImapJob::KMImapJob(QList<KMMessage> msgList)
{
  mSingleMessage = false;
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
  mJob = KIO::get(url, FALSE, FALSE);
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
  assert(it != account->mapJobData.end());
  QString uid = mMsgList.current()->headerField("X-UID");
  mMsgList.current()->fromString((*it).data);
  mMsgList.current()->setHeaderField("X-UID",uid);
  account->mapJobData.remove(it);
  if (mMsgList.next())
    slotGetNextMessage();
  else if (mSingleMessage)
    emit messageRetrieved(mMsgList.first());
  else
    emit messagesRetrieved(mMsgList);
  account->displayProgress();
  account->mJobList.remove(this);
  delete this;
}


//-----------------------------------------------------------------------------
void KMAcctImap::slotGetMessageData(KIO::Job * job, const QByteArray & data)
{
  QMap<KIO::Job *, jobData>::Iterator it = mapJobData.find(job);
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
      (*it).mJob->kill( TRUE );
      account->mapJobData.remove( (*it).mJob );
      account->mJobList.remove( it );
      delete it;
    }
  }
}


//-----------------------------------------------------------------------------
void KMAcctImap::killJobsForItem(KMFolderTreeItem * fti)
{
  QMap<KIO::Job *, jobData>::Iterator it = mapJobData.begin(), it2;
  while (it != mapJobData.end())
  {
    if (it.data().parent == fti)
    {
      it.key()->kill( TRUE );
      it2 = it;
      it++;
      mapJobData.remove( it2 );
    }
    else it++;
  }
  displayProgress();
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
void KMAcctImap::nextStatusAction()
{
  displayProgress();
  if (mStatusQueue.isEmpty()) return;
  statusData *data = mStatusQueue.dequeue();
  if (data->Delete)
  {
    KIO::Job *job = KIO::del(data->url, FALSE, FALSE);
    jobData jd;
    jd.total = 0; jd.done = 0; jd.parent = NULL;
    mapJobData.insert(job, jd);
    connect(job, SIGNAL(result(KIO::Job *)),
            this, SLOT(slotStatusResult(KIO::Job *)));
  }
}


//-----------------------------------------------------------------------------
void KMAcctImap::slotStatusResult(KIO::Job * job)
{
  QMap<KIO::Job *, jobData>::Iterator it = mapJobData.find(job);
  assert(it != mapJobData.end());
  mapJobData.remove(it);
  if (job->error())
  { 
    job->showErrorDialog();
    mStatusQueue.clear();
  }
  nextStatusAction();
}


//-----------------------------------------------------------------------------
void KMAcctImap::slotAbortRequested()
{
  QMap<KIO::Job*, jobData>::Iterator it = mapJobData.begin();
  while ( it != mapJobData.end() )
  {
    it.key()->kill( TRUE );
    mapJobData.remove( it );
    it = mapJobData.begin();
  }                                                                             
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
  mPrefix = config.readEntry("prefix");
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



