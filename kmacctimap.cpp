/**
 * kmacctimap.cpp
 *
 * Copyright (c) 2000-2002 Michael Haeckel <haeckel@kde.org>
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

#include <config.h>
#include "kmacctimap.moc"

#include "kmmainwin.h"
#include "kmbroadcaststatus.h"
#include "kmfoldertree.h"
#include "kmfoldermgr.h"
#include "kmfiltermgr.h"

#include <kmfolderimap.h>
#include <kio/passdlg.h>
#include <kio/scheduler.h>
#include <kio/slave.h>
#include <kmessagebox.h>
#include <kdebug.h>

#include <qregexp.h>

#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>
#include <assert.h>

//-----------------------------------------------------------------------------
KMAcctImap::KMAcctImap(KMAcctMgr* aOwner, const QString& aAccountName):
  KMAcctImapInherited(aOwner, aAccountName)
{
  init();
  mSlave = 0;
  mTotal = 0;
  mFolder = 0;
  mCountUnread = 0;
  mCountLastUnread = 0;
  mCountRemainChecks = 0;
  errorDialogIsActive = false;
  mOpenFolders.setAutoDelete(true);
  connect(KMBroadcastStatus::instance(), SIGNAL(signalAbortRequested()),
          this, SLOT(slotAbortRequested()));
  connect(&mIdleTimer, SIGNAL(timeout()), SLOT(slotIdleTimeout()));
  KIO::Scheduler::connect(
    SIGNAL(slaveError(KIO::Slave *, int, const QString &)),
    this, SLOT(slotSlaveError(KIO::Slave *, int, const QString &)));
}


//-----------------------------------------------------------------------------
KMAcctImap::~KMAcctImap()
{
  killAllJobs();
  if (mSlave) KIO::Scheduler::disconnectSlave(mSlave);
  emit deleted(this);
}


//-----------------------------------------------------------------------------
QString KMAcctImap::type(void) const
{
  return "imap";
}

//-----------------------------------------------------------------------------
void KMAcctImap::init(void)
{
  mHost   = "";
  mPort   = 143;
  mLogin  = "";
  mPasswd = "";
  mAuth = "*";
  mStorePasswd = FALSE;
  mAskAgain = FALSE;
  mProgressEnabled = FALSE;
  mPrefix = "/";
  mAutoExpunge = TRUE;
  mHiddenFolders = FALSE;
  mOnlySubscribedFolders = FALSE;
  mUseSSL = FALSE;
  mUseTLS = FALSE;
  mIdle = TRUE;
  mSieveConfig = KMail::SieveConfig();
}

//-----------------------------------------------------------------------------
void KMAcctImap::pseudoAssign(KMAccount* account)
{
  mIdleTimer.stop();
  killAllJobs();
  if (mSlave) KIO::Scheduler::disconnectSlave(mSlave);
  mSlave = 0;
  if (mFolder)
  {
    mFolder->setContentState(KMFolderImap::imapNoInformation);
    mFolder->setSubfolderState(KMFolderImap::imapNoInformation);
  }
  assert(account->type() == "imap");
  KMAcctImap *acct = static_cast<KMAcctImap*>(account);
  setName(acct->name());
  setCheckInterval(acct->checkInterval());
  setCheckExclude(acct->checkExclude());
  setFolder(acct->folder());
  setHost(acct->host());
  setPort(acct->port());
  setPrefix(acct->prefix());
  setLogin(acct->login());
  setTrash(acct->trash());
  setAuth(acct->auth());
  setAutoExpunge(acct->autoExpunge());
  setHiddenFolders(acct->hiddenFolders());
  setOnlySubscribedFolders(acct->onlySubscribedFolders());
  setStorePasswd(acct->storePasswd());
  setPasswd(acct->passwd(), acct->storePasswd());
  setUseSSL(acct->useSSL());
  setUseTLS(acct->useTLS());
  setSieveConfig(acct->sieveConfig());
}

//-----------------------------------------------------------------------------
void KMAcctImap::readConfig(KConfig& config)
{
  KMAcctImapInherited::readConfig(config);

  mLogin = config.readEntry("login", "");
  mStorePasswd = config.readNumEntry("store-passwd", FALSE);
  if (mStorePasswd)
  {
    mPasswd = config.readEntry("pass");
    if (mPasswd.isEmpty())
    {
      mPasswd = config.readEntry("passwd");
      if (!mPasswd.isEmpty()) mPasswd = importPassword(mPasswd);
    }
  }
  else mPasswd = "";
  mHost = config.readEntry("host");
  mPort = config.readNumEntry("port");
  mAuth = config.readEntry("auth", "*");
  mPrefix = config.readEntry("prefix", "/");
  mTrash = config.readEntry("trash");
  if (mFolder) mFolder->setImapPath(mPrefix);
  mAutoExpunge = config.readBoolEntry("auto-expunge", TRUE);
  mHiddenFolders = config.readBoolEntry("hidden-folders", FALSE);
  mOnlySubscribedFolders = config.readBoolEntry("subscribed-folders", FALSE);
  mUseSSL = config.readBoolEntry("use-ssl", FALSE);
  mUseTLS = config.readBoolEntry("use-tls", FALSE);
  mSieveConfig.readConfig( config );
}


//-----------------------------------------------------------------------------
void KMAcctImap::writeConfig(KConfig& config)
{
  KMAcctImapInherited::writeConfig(config);

  config.writeEntry("login", mLogin);
  config.writeEntry("store-passwd", mStorePasswd);
  if (mStorePasswd) config.writeEntry("pass", mPasswd);
  else config.writeEntry("passwd", "");

  config.writeEntry("host", mHost);
  config.writeEntry("port", static_cast<int>(mPort));
  config.writeEntry("auth", mAuth);
  config.writeEntry("prefix", mPrefix);
  config.writeEntry("trash", mTrash);
  config.writeEntry("auto-expunge", mAutoExpunge);
  config.writeEntry("hidden-folders", mHiddenFolders);
  config.writeEntry("subscribed-folders", mOnlySubscribedFolders);
  config.writeEntry("use-ssl", mUseSSL);
  config.writeEntry("use-tls", mUseTLS);

  mSieveConfig.writeConfig( config );
}


//-----------------------------------------------------------------------------
void KMAcctImap::setStorePasswd(bool b)
{
  mStorePasswd = b;
}


//-----------------------------------------------------------------------------
void KMAcctImap::setUseSSL(bool b)
{
  mUseSSL = b;
}


//-----------------------------------------------------------------------------
void KMAcctImap::setUseTLS(bool b)
{
  mUseTLS = b;
}


//-----------------------------------------------------------------------------
void KMAcctImap::setLogin(const QString& aLogin)
{
  mLogin = aLogin;
}


//-----------------------------------------------------------------------------
QString KMAcctImap::passwd(void) const
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
void KMAcctImap::ignoreJobsForMessage( KMMessage* msg )
{
  KMImapJob *job;
  for (KMFolderJob *it = mJobList.first(); it;
       it = mJobList.next()) {
    if ((*it).msgList().first() == msg) {
      job = dynamic_cast<KMImapJob*>(it);
      mapJobData.remove( job->mJob );
      mJobList.remove( job );
      delete job;
      break;
    }
  }
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
  mPrefix.replace(QRegExp("[%*\"]"), "");
  if (mPrefix.isEmpty() || mPrefix.at(0) != '/') mPrefix = '/' + mPrefix;
  if (mPrefix.at(mPrefix.length() - 1) != '/') mPrefix += '/';
  if (mFolder) mFolder->setImapPath(mPrefix);
}

//-----------------------------------------------------------------------------
void KMAcctImap::setImapFolder(KMFolderImap *aFolder)
{
  mFolder = aFolder;
  mFolder->setImapPath(mPrefix);
}


//-----------------------------------------------------------------------------
void KMAcctImap::initJobData(jobData &jd)
{
  jd.total = 1;
  jd.done = 0;
  jd.parent = 0;
  jd.quiet = FALSE;
  jd.inboxOnly = FALSE;
}


//-----------------------------------------------------------------------------
int KMAcctImap::tempOpenFolder(KMFolder *folder)
{
  int rc = folder->open();
  if (rc) return rc;
  mOpenFolders.append(new QGuardedPtr<KMFolder>(folder));
  return 0;
}


//-----------------------------------------------------------------------------
void KMAcctImap::setAutoExpunge(bool aAutoExpunge)
{
  mAutoExpunge = aAutoExpunge;
}


//-----------------------------------------------------------------------------
void KMAcctImap::setHiddenFolders(bool aHiddenFolders)
{
  mHiddenFolders = aHiddenFolders;
}


//-----------------------------------------------------------------------------
void KMAcctImap::setOnlySubscribedFolders(bool aOnlySubscribedFolders)
{
  mOnlySubscribedFolders = aOnlySubscribedFolders;
}


//-----------------------------------------------------------------------------
void KMAcctImap::setAuth(const QString& aAuth)
{
  mAuth = aAuth;
}


//-----------------------------------------------------------------------------
void KMAcctImap::initSlaveConfig()
{
  mSlaveConfig.clear();
  mSlaveConfig.insert("auth", mAuth);
  mSlaveConfig.insert("tls", (mUseTLS) ? "on" : "off");
  if (mAutoExpunge) mSlaveConfig.insert("expunge", "auto");
}


//-----------------------------------------------------------------------------
KURL KMAcctImap::getUrl()
{
  KURL url;
  url.setProtocol(mUseSSL ? QString("imaps") : QString("imap"));
  url.setUser(mLogin);
  url.setPass(decryptStr(mPasswd));
  url.setHost(mHost);
  url.setPort(mPort);
  return url;
}


//-----------------------------------------------------------------------------
bool KMAcctImap::makeConnection()
{
  if (mSlave) return TRUE;

  if(mAskAgain || mPasswd.isEmpty() || mLogin.isEmpty())
  {
    QString passwd = decryptStr(mPasswd);
    bool b = FALSE;
    if (KIO::PasswordDialog::getNameAndPassword(mLogin, passwd, &b,
      i18n("You need to supply a username and a password to access this "
      "mailbox."), FALSE, QString::null, mName, i18n("Account:"))
      != QDialog::Accepted)
    {
      emit finishedCheck(false);
      return FALSE;
    } else mPasswd = encryptStr(passwd);
  }

  initSlaveConfig();
  mSlave = KIO::Scheduler::getConnectedSlave(getUrl(), mSlaveConfig);
  if (!mSlave)
  {
    KMessageBox::error(0, i18n("Could not start process for %1.")
      .arg(getUrl().protocol()));
    return FALSE;
  }
  return TRUE;
}


//-----------------------------------------------------------------------------
void KMAcctImap::slotSlaveError(KIO::Slave *aSlave, int errorCode,
  const QString &errorMsg)
{
  if (aSlave != mSlave) return;
  if (errorCode == KIO::ERR_SLAVE_DIED) slaveDied();
  if (errorCode == KIO::ERR_COULD_NOT_LOGIN && !mStorePasswd) mAskAgain = TRUE;
  if (errorCode == KIO::ERR_DOES_NOT_EXIST)
  {
    // folder is gone, so reload the folderlist
    if (mFolder) mFolder->listDirectory();
    return;
  }
  // check if we still display an error
  killAllJobs();
  if ( !errorDialogIsActive )
  {
    errorDialogIsActive = true;
    if ( KMessageBox::messageBox(kernel->mainWin(), KMessageBox::Error,
          KIO::buildErrorString(errorCode, errorMsg),
          i18n("Error")) == KMessageBox::Ok )
    {
      errorDialogIsActive = false;
    }
  } else
    kdDebug(5006) << "suppressing error:" << errorMsg << endl;
}


//-----------------------------------------------------------------------------
void KMAcctImap::displayProgress()
{
  if (mProgressEnabled == mapJobData.isEmpty())
  {
    mProgressEnabled = !mapJobData.isEmpty();
    KMBroadcastStatus::instance()->setStatusProgressEnable( "I" + mName,
      mProgressEnabled );
    if (!mProgressEnabled)
    {
      QPtrListIterator<QGuardedPtr<KMFolder> > it(mOpenFolders);
      for ( it.toFirst() ; it.current() ; ++it )
        if (*it) (*(*it))->close();
      mOpenFolders.clear();
    }
  }
  mIdle = FALSE;
  if (mapJobData.isEmpty())
    mIdleTimer.start(15000);
  else
    mIdleTimer.stop();
  int total = 0, done = 0;
  for (QMap<KIO::Job*, jobData>::Iterator it = mapJobData.begin();
    it != mapJobData.end(); it++)
  {
    total += (*it).total;
    done += (*it).done;
  }
  if (total == 0)
  {
    mTotal = 0;
    return;
  }
  if (total > mTotal) mTotal = total;
  done += mTotal - total;
  KMBroadcastStatus::instance()->setStatusProgressPercent( "I" + mName,
    100*done / mTotal );
}


//-----------------------------------------------------------------------------
void KMAcctImap::slotIdleTimeout()
{
  if (mIdle)
  {
    if (mSlave) KIO::Scheduler::disconnectSlave(mSlave);
    mSlave = 0;
    mIdleTimer.stop();
  } else {
    if (mSlave)
    {
      QByteArray packedArgs;
      QDataStream stream( packedArgs, IO_WriteOnly);

      stream << (int) 'N';

      KIO::SimpleJob *job = KIO::special(getUrl(), packedArgs, FALSE);
      KIO::Scheduler::assignJobToSlave(mSlave, job);
      connect(job, SIGNAL(result(KIO::Job *)),
        this, SLOT(slotSimpleResult(KIO::Job *)));
    }
    else mIdleTimer.stop();
  }
}


//-----------------------------------------------------------------------------
void KMAcctImap::slotAbortRequested()
{
  killAllJobs();
}


//-----------------------------------------------------------------------------
void KMAcctImap::killAllJobs()
{
  QMap<KIO::Job*, jobData>::Iterator it = mapJobData.begin();
  for (it = mapJobData.begin(); it != mapJobData.end(); it++)
    if ((*it).parent)
    {
      // clear folder state
      KMFolderImap *fld = (*it).parent;
      fld->setContentState(KMFolderImap::imapNoInformation);
      fld->setSubfolderState(KMFolderImap::imapNoInformation);
      fld->sendFolderComplete(FALSE);
      fld->quiet(FALSE);
    }
  if (mSlave && mapJobData.begin() != mapJobData.end())
  {
    mSlave->kill();
    mSlave = 0;
  }
  // remove the jobs
  mapJobData.clear();
  KMAccount::deleteFolderJobs();
  // make sure that no new-mail-check is blocked
  if (mCountRemainChecks > 0)
  {
    mCountRemainChecks = 0;
    emit finishedCheck(false);
  }
  displayProgress();
}


//-----------------------------------------------------------------------------
void KMAcctImap::killJobsForItem(KMFolderTreeItem * fti)
{
  QMap<KIO::Job *, jobData>::Iterator it = mapJobData.begin();
  while (it != mapJobData.end())
  {
    if (it.data().parent == fti->folder())
    {
      killAllJobs();
      break;
    }
    else it++;
  }
}


//-----------------------------------------------------------------------------
void KMAcctImap::slotSimpleResult(KIO::Job * job)
{
  QMap<KIO::Job *, jobData>::Iterator it = mapJobData.find(job);
  bool quiet = FALSE;
  if (it != mapJobData.end())
  {
    quiet = (*it).quiet;
    mapJobData.remove(it);
  }
  if (job->error())
  {
    if (!quiet) slotSlaveError(mSlave, job->error(),
        job->errorText() );
    if (job->error() == KIO::ERR_SLAVE_DIED) slaveDied();
  }
  displayProgress();
}


//-----------------------------------------------------------------------------
void KMAcctImap::processNewMail(bool interactive)
{
  emit newMailsProcessed(-1);
  if (!mFolder || !mFolder->child() ||
      !makeConnection())
  {
    mCountRemainChecks = 0;
    emit finishedCheck(false);
    return;
  }
  QStringList strList;
  QValueList<QGuardedPtr<KMFolder> > folderList;
  kernel->imapFolderMgr()->createFolderList(&strList, &folderList,
    mFolder->child(), QString::null, false);
  QValueList<QGuardedPtr<KMFolder> >::Iterator it;
  // first get the current count of unread-messages
  mCountRemainChecks = 0;
  mCountLastUnread = 0;
  for (it = folderList.begin(); it != folderList.end(); it++)
  {
    KMFolder *folder = *it;
    if (folder && !folder->noContent())
    {
      mCountLastUnread += folder->countUnread();
    }
  }
  // then check for new mails
  for (it = folderList.begin(); it != folderList.end(); it++)
  {
    KMFolder *folder = *it;
    if (folder && !folder->noContent())
    {
      KMFolderImap *imapFolder = static_cast<KMFolderImap*>(folder);
      if (imapFolder->getContentState() != KMFolderImap::imapInProgress)
      {
        // connect the result-signals for new-mail-notification
        mCountRemainChecks++;
        if (imapFolder->isSelected()) {
          connect(imapFolder, SIGNAL(folderComplete(KMFolderImap*, bool)),
              this, SLOT(postProcessNewMail(KMFolderImap*, bool)));
          imapFolder->getFolder();
        }
        else {
          connect(imapFolder, SIGNAL(numUnreadMsgsChanged(KMFolder*)),
              this, SLOT(postProcessNewMail(KMFolder*)));
          imapFolder->processNewMail(interactive);
        }
      }
    }
  }
}

//-----------------------------------------------------------------------------
void KMAcctImap::postProcessNewMail(KMFolderImap* folder, bool)
{
  disconnect(folder, SIGNAL(folderComplete(KMFolderImap*, bool)),
      this, SLOT(postProcessNewMail(KMFolderImap*, bool)));
  postProcessNewMail(static_cast<KMFolder*>(folder));
}

//-----------------------------------------------------------------------------
void KMAcctImap::postProcessNewMail(KMFolder* folder)
{
  disconnect(folder, SIGNAL(numUnreadMsgsChanged(KMFolder*)),
      this, SLOT(postProcessNewMail(KMFolder*)));

  mCountRemainChecks--;

  // count the unread messages
  mCountUnread += folder->countUnread();
  if (mCountRemainChecks == 0)
  {
    // all checks are done
    if (mCountUnread > 0 && mCountUnread > mCountLastUnread)
    {
      emit finishedCheck(true);
      mCountLastUnread = mCountUnread;
    } else {
      emit finishedCheck(false);
    }
    mCountUnread = 0;
  }
}

