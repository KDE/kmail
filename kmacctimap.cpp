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
#include <netinet/in.h>

#include <kmfolderimap.h>
#include <kmmessage.h>
#include <kconfig.h>
#include <qpushbutton.h>
#include <qlineedit.h>
#include <kapp.h>
#include <klocale.h>
#include <kio/scheduler.h>
#include <kio/slave.h>
#include <kmessagebox.h>

#include "kmacctimap.h"
#include "kmglobal.h"
#include "kbusyptr.h"
#include "kmacctfolder.h"
#include "kmbroadcaststatus.h"
#include "kmfoldertree.h"
#include <kprocess.h>
#include <qtooltip.h>
#include <qlayout.h>

#include <kwin.h>
#include <kbuttonbox.h>
#include <kstddirs.h>

//-----------------------------------------------------------------------------
KMAcctImap::KMAcctImap(KMAcctMgr* aOwner, const QString& aAccountName):
  KMAcctImapInherited(aOwner, aAccountName)
{
  init();
  mSlave = NULL;
  mTotal = 0;
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
  mPrefix = "/";
  mAutoExpunge = TRUE;
  mHiddenFolders = FALSE;
  mUseSSL = FALSE;
  mUseTLS = FALSE;
  mIdle = TRUE;
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
  setAutoExpunge(acct->autoExpunge());
  setHiddenFolders(acct->hiddenFolders());
  setStorePasswd(acct->storePasswd());
  setPasswd(acct->passwd(), acct->storePasswd());
  setUseSSL(acct->useSSL());
  setUseTLS(acct->useTLS());
}

//-----------------------------------------------------------------------------
void KMAcctImap::readConfig(KConfig& config)
{
  KMAcctImapInherited::readConfig(config);

  mLogin = config.readEntry("login", "");
  mStorePasswd = config.readNumEntry("store-passwd", FALSE);
  if (mStorePasswd) mPasswd = config.readEntry("passwd");
  else mPasswd = "";
  mHost = config.readEntry("host");
  mPort = config.readNumEntry("port");
  mAuth = config.readEntry("auth", "*");
  mPrefix = config.readEntry("prefix", "/");
  mAutoExpunge = config.readBoolEntry("auto-expunge", TRUE);
  mHiddenFolders = config.readBoolEntry("hidden-folders", FALSE);
  mUseSSL = config.readBoolEntry("use-ssl", FALSE);
  mUseTLS = config.readBoolEntry("use-tls", FALSE);
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
  config.writeEntry("auto-expunge", mAutoExpunge);
  config.writeEntry("hidden-folders", mHiddenFolders);
  config.writeEntry("use-ssl", mUseSSL);
  config.writeEntry("use-tls", mUseTLS);
}


//-----------------------------------------------------------------------------
QString KMAcctImap::encryptStr(const QString &aStr) const
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
QString KMAcctImap::decryptStr(const QString &aStr) const
{
  return encryptStr(aStr);
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
  if(mPasswd.isEmpty() || mLogin.isEmpty())
  {
    QString passwd = decryptStr(mPasswd);
    QString msg = i18n("Please set Password and Username");
    KMImapPasswdDialog dlg(NULL, NULL, this, msg, mLogin, passwd);
    if (!dlg.exec()) return FALSE;
  }
  initSlaveConfig();
  mSlave = KIO::Scheduler::getConnectedSlave(getUrl(), mSlaveConfig);
  if (!mSlave) return FALSE;
  return TRUE;
}
 
 
//-----------------------------------------------------------------------------
void KMAcctImap::slotSlaveError(KIO::Slave *aSlave, int errorCode,
  const QString &errorMsg)
{
  if (aSlave != mSlave) return;
  if (errorCode == KIO::ERR_SLAVE_DIED)
  {
    mSlave = NULL;
    KMessageBox::error(0,
    i18n("The process for \n%1\ndied unexpectedly").arg(errorMsg));
  }
  else KMessageBox::error(0, i18n("Error connecting to %1:\n%2")
    .arg(mHost).arg(errorMsg));
  killAllJobs();
}


//-----------------------------------------------------------------------------
void KMAcctImap::displayProgress()
{
  if (mProgressEnabled == mapJobData.isEmpty())
  {
    mProgressEnabled = !mapJobData.isEmpty();
    KMBroadcastStatus::instance()->setStatusProgressEnable( mProgressEnabled );
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
  KMBroadcastStatus::instance()->setStatusProgressPercent( 100*done / mTotal );
}


//-----------------------------------------------------------------------------
void KMAcctImap::slotIdleTimeout()
{
  if (mIdle)
  {
    if (mSlave) KIO::Scheduler::disconnectSlave(mSlave);
    mSlave = NULL;
    mIdleTimer.stop();
  } else {
    if (mSlave)
    {
      KIO::SimpleJob *job = KIO::special(getUrl(), QCString("NOOP"), FALSE);
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
      KMFolderImap *fld = static_cast<KMFolderImap*>((*it).parent->folder);
      (*it).parent->mImapState = KMFolderTreeItem::imapFinished;
      fld->setUidNext("");
      fld->sendFolderComplete((*it).parent, FALSE);
      (*it).parent->folder->quiet(FALSE);
    }
  if (mapJobData.begin() != mapJobData.end())
  {
    mSlave->kill();
    mSlave = NULL;
  }
  mapJobData.clear();
  mJobList.setAutoDelete(true);
  mJobList.clear();
  mJobList.setAutoDelete(false);
  displayProgress();
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
    }
    else it++;
  }
}


//-----------------------------------------------------------------------------
void KMAcctImap::slotSimpleResult(KIO::Job * job)
{
  QMap<KIO::Job *, jobData>::Iterator it = mapJobData.find(job);
  if (it != mapJobData.end()) mapJobData.remove(it);
  if (job->error())
  {
    job->showErrorDialog();
    if (job->error() == KIO::ERR_SLAVE_DIED) mSlave = NULL;
  }
  displayProgress();
}


//=============================================================================
//
//  Class  KMImapPasswdDialog
//
//=============================================================================
 
KMImapPasswdDialog::KMImapPasswdDialog(QWidget *parent, const char *name,
                                     KMAcctImap *account,
                                     const QString &caption,
                                     const QString &login,
                                     const QString &passwd)
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
 
  if(!login.isEmpty())
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
