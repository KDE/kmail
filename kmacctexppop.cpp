// KMAcctExpPop.cpp
// Authors: Don Sanders, (based on kmacctpop by)
//          Stefan Taferner and Markus Wuebben

#include "kmacctexppop.moc"

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
#include <kio/scheduler.h>

#include "kmacctexppop.h"
#include "kalarmtimer.h"
#include "kmglobal.h"
#include "kbusyptr.h"
#include "kmacctfolder.h"
#include "kmfiltermgr.h"
#include <kprocess.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <qtooltip.h>
#include "kmbroadcaststatus.h"

#include <kwin.h>
#include <kbuttonbox.h>

//-----------------------------------------------------------------------------
KMAcctExpPop::KMAcctExpPop(KMAcctMgr* aOwner, const QString& aAccountName):
  KMAcctExpPopInherited(aOwner, aAccountName)
{
  mUseSSL = FALSE;
  mUseTLS = FALSE;
  mStorePasswd = FALSE;
  mLeaveOnServer = FALSE;
  mProtocol = 3;
  struct servent *serv = getservbyname("pop-3", "tcp");
  if (serv) {
    mPort = ntohs(serv->s_port);
  } else {
    mPort = 110;
  }
  job = 0L;
  slave = 0L;
  stage = Idle;
  indexOfCurrentMsg = -1;
  curMsgStrm = 0;
  processingDelay = 2*100;
  mProcessing = false;
  dataCounter = 0;
  connect(&processMsgsTimer,SIGNAL(timeout()),SLOT(slotProcessPendingMsgs()));
  ss = new QTimer();
  connect( ss, SIGNAL( timeout() ), this, SLOT( slotGetNextMsg() ));
  KIO::Scheduler::connect(
    SIGNAL(slaveError(KIO::Slave *, int, const QString &)),
    this, SLOT(slotSlaveError(KIO::Slave *, int, const QString &)));
}


//-----------------------------------------------------------------------------
KMAcctExpPop::~KMAcctExpPop()
{
  if (job) {
    job->kill();
    idsOfMsgsPendingDownload.clear();
    lensOfMsgsPendingDownload.clear();
    processRemainingQueuedMessagesAndSaveUidList();
  }
}


//-----------------------------------------------------------------------------
const char* KMAcctExpPop::type(void) const
{
  return "pop";
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::init(void)
{
  mHost   = "";
  struct servent *serv = getservbyname("pop-3", "tcp");
  if (serv) {
    mPort = ntohs(serv->s_port);
  } else {
    mPort = 110;
  }
  mLogin  = "";
  mPasswd = "";
  mProtocol = 3;
  mUseSSL = FALSE;
  mUseTLS = FALSE;
  mStorePasswd = FALSE;
  mLeaveOnServer = FALSE;
}

//-----------------------------------------------------------------------------
KURL KMAcctExpPop::getUrl()
{
  KURL url;
  if (mUseSSL)
        url.setProtocol(QString("pop3s"));
  else
        url.setProtocol(QString("pop3"));
  url.setUser(mLogin);
  url.setPass(decryptStr(mPasswd));
  url.setHost(mHost);
  url.setPort(mPort);
  return url;
}

//-----------------------------------------------------------------------------
void KMAcctExpPop::pseudoAssign(KMAccount* account)
{
  assert(account->type() == "pop");
  KMAcctExpPop *acct = static_cast<KMAcctExpPop*>(account);
  setName(acct->name());
  setCheckInterval(acct->checkInterval());
  setCheckExclude(acct->checkExclude());
  setFolder(acct->folder());
  setHost(acct->host());
  setPort(acct->port());
  setLogin(acct->login());
  setUseSSL(acct->useSSL());
  setUseTLS(acct->useTLS());
  setAuth(acct->auth());
  setStorePasswd(acct->storePasswd());
  setPasswd(acct->passwd(), acct->storePasswd());
  setLeaveOnServer(acct->leaveOnServer());
  setPrecommand(acct->precommand());
}

//-----------------------------------------------------------------------------
void KMAcctExpPop::processNewMail(bool _interactive)
{
  if (stage == Idle) {

    if(mPasswd.isEmpty() || mLogin.isEmpty()) {
      QString passwd = decryptStr(mPasswd);
      QString msg = i18n("Please set Password and Username");
      KMExpPasswdDialog dlg(NULL, NULL, this, msg, mLogin, passwd);
      if (!dlg.exec()) {
	emit finishedCheck(false);
	return;
      }
    }

    QString seenUidList = locateLocal( "appdata", mLogin + ":" + "@" + mHost +
				       ":" + QString("%1").arg(mPort) );
    KConfig config( seenUidList );
    uidsOfSeenMsgs = config.readListEntry( "seenUidList" );
    uidsOfNextSeenMsgs.clear();

    interactive = _interactive;
    mUidlFinished = FALSE;
    startJob();
  }
  else {
    emit finishedCheck(false);
    return;
  }
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::readConfig(KConfig& config)
{
  KMAcctExpPopInherited::readConfig(config);


  mLogin = config.readEntry("login", "");
  mUseSSL = config.readNumEntry("use-ssl", FALSE);
  mUseTLS = config.readNumEntry("use-tls", FALSE);
  mAuth = config.readEntry("auth", "AUTO");
  mStorePasswd = config.readNumEntry("store-passwd", FALSE);
  if (mStorePasswd) mPasswd = config.readEntry("passwd");
  else mPasswd = "";
  mHost = config.readEntry("host");
  mPort = config.readNumEntry("port");
  mProtocol = config.readNumEntry("protocol");
  mLeaveOnServer = config.readNumEntry("leave-on-server", FALSE);
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::writeConfig(KConfig& config)
{
  KMAcctExpPopInherited::writeConfig(config);

  config.writeEntry("login", mLogin);
  config.writeEntry("use-ssl", mUseSSL);
  config.writeEntry("use-tls", mUseTLS);
  config.writeEntry("auth", mAuth);
  config.writeEntry("store-passwd", mStorePasswd);
  if (mStorePasswd) config.writeEntry("passwd", mPasswd);
  else config.writeEntry("passwd", "");

  config.writeEntry("host", mHost);
  config.writeEntry("port", static_cast<int>(mPort));
  config.writeEntry("protocol", mProtocol);
  config.writeEntry("leave-on-server", mLeaveOnServer);
}


//-----------------------------------------------------------------------------
QString KMAcctExpPop::encryptStr(const QString &aStr) const
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
QString KMAcctExpPop::decryptStr(const QString &aStr) const
{
  return encryptStr(aStr);
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::setUseSSL(bool b)
{
  mUseSSL = b;
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::setUseTLS(bool b)
{
  mUseTLS = b;
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::setAuth(const QString &aAuth)
{
  mAuth = aAuth;
}

//-----------------------------------------------------------------------------
void KMAcctExpPop::setStorePasswd(bool b)
{
  mStorePasswd = b;
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::setLeaveOnServer(bool b)
{
  mLeaveOnServer = b;
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::setLogin(const QString& aLogin)
{
  mLogin = aLogin;
}


//----------------------------------------------------------------------------
QString KMAcctExpPop::passwd(void) const
{
  return decryptStr(mPasswd);
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::setPasswd(const QString& aPasswd, bool aStoreInConfig)
{
  mPasswd = encryptStr(aPasswd);
  mStorePasswd = aStoreInConfig;
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::clearPasswd()
{
  mPasswd = "";
  mStorePasswd = FALSE;
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::setHost(const QString& aHost)
{
  mHost = aHost;
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::setPort(unsigned short int aPort)
{
  mPort = aPort;
}


//-----------------------------------------------------------------------------
bool KMAcctExpPop::setProtocol(short aProtocol)
{
  //assert(aProtocol==2 || aProtocol==3);
  if(aProtocol != 2 || aProtocol != 3)
    return false;
  mProtocol = aProtocol;
  return true;
}


//=============================================================================
//
//  Class  KMExpPasswdDialog
//
//=============================================================================

KMExpPasswdDialog::KMExpPasswdDialog(QWidget *parent, const char *name,
			             KMAcctExpPop *account,
				     const QString& caption,
			             const QString& login,
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
void KMExpPasswdDialog::slotOkPressed()
{
  act->setLogin(usernameLEdit->text());
  act->setPasswd(passwdLEdit->text(), act->storePasswd());
  done(1);
}

//-----------------------------------------------------------------------------
void KMExpPasswdDialog::slotCancelPressed()
{
  done(0);
}

void KMAcctExpPop::connectJob() {
  KIO::Scheduler::assignJobToSlave(slave, job);
  if (stage != Dele)
  connect(job, SIGNAL( data( KIO::Job*, const QByteArray &)),
	  SLOT( slotData( KIO::Job*, const QByteArray &)));
  connect( job, SIGNAL( result( KIO::Job * ) ),
	   SLOT( slotResult( KIO::Job * ) ) );
}

void KMAcctExpPop::slotCancel()
{
  idsOfMsgsPendingDownload.clear();
  lensOfMsgsPendingDownload.clear();
  processRemainingQueuedMessagesAndSaveUidList();
  slotJobFinished();
}

void KMAcctExpPop::slotProcessPendingMsgs()
{
  if (mProcessing) // not reentrant
    return;
  mProcessing = true;

  bool addedOk;
  QValueList<KMMessage*>::Iterator cur = msgsAwaitingProcessing.begin();
  QStringList::Iterator curId = msgIdsAwaitingProcessing.begin();
  QStringList::Iterator curUid = msgUidsAwaitingProcessing.begin();

  KURL url = getUrl();

  while (cur != msgsAwaitingProcessing.end()) {
    // note we can actually end up processing events in processNewMsg
    // this happens when send receipts is turned on
    // hence the check for re-entry at the start of this method.
    // -sanders Update processNewMsg should no longer process events

    addedOk = processNewMsg(*cur); //added ok? Error displayed if not.
    if ((*cur)->parent()) {
      int count = (*cur)->parent()->count();
      if (count != 1 && (*cur)->parent()->operator[](count - 1) == *cur)
	(*cur)->parent()->unGetMsg(count - 1);
    }

    if (!addedOk) {
      idsOfMsgsPendingDownload.clear();
      lensOfMsgsPendingDownload.clear();
      msgIdsAwaitingProcessing.clear();
      msgUidsAwaitingProcessing.clear();
      break;
    }
    else {
      url.setPath(QString("/%1").arg(*curId));
      idsOfMsgsToDelete.append(url.url());
      uidsOfNextSeenMsgs.append( *curUid );
    }
    ++cur;
    ++curId;
    ++curUid;
  }

  msgsAwaitingProcessing.clear();
  msgIdsAwaitingProcessing.clear();
  msgUidsAwaitingProcessing.clear();
  mProcessing = false;
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::slotAbortRequested()
{
  if (stage == Idle) return;
  disconnect(KMBroadcastStatus::instance(), SIGNAL(signalAbortRequested()),
          this, SLOT(slotAbortRequested()));
  stage = Quit;
  if (job) job->kill();
  job = 0L;
  slave = 0L;
  slotCancel();
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::startJob() {

  // Run the precommand
  if (!runPrecommand(precommand()))
    {
      KMessageBox::sorry(0,
                         i18n("Couldn't execute precommand: %1").arg(precommand()),
                         i18n("Kmail Error Message"));
      emit finishedCheck(idsOfMsgs.count() > 0);
      return;
    }
  // end precommand code

  KURL url = getUrl();

  if ( url.isMalformed() ) {
    KMessageBox::error(0, i18n("Source URL is malformed"),
                          i18n("Kioslave Error Message") );
    return;
  }

  idsOfMsgsPendingDownload.clear();
  lensOfMsgsPendingDownload.clear();
  idsOfMsgs.clear();
  uidsOfMsgs.clear();
  idsOfMsgsToDelete.clear();
  indexOfCurrentMsg = -1;
  KMBroadcastStatus::instance()->reset();
  KMBroadcastStatus::instance()->setStatusProgressEnable( true );
  KMBroadcastStatus::instance()->setStatusMsg(
	i18n("Preparing transmission from %1...").arg(mHost));
  connect(KMBroadcastStatus::instance(), SIGNAL(signalAbortRequested()),
          this, SLOT(slotAbortRequested()));

  numBytes = 0;
  numBytesRead = 0;
  stage = List;
  mSlaveConfig.clear();
  mSlaveConfig.insert("progress", "off");
  mSlaveConfig.insert("tls", (mUseTLS) ? "on" : "off");
  if (mAuth == "PLAIN" || mAuth == "CRAM-MD5")
  {
    mSlaveConfig.insert("auth", "SASL");
    mSlaveConfig.insert("sasl", mAuth);
  }
  else if (mAuth != "AUTO") mSlaveConfig.insert("auth", mAuth);
  slave = KIO::Scheduler::getConnectedSlave( url.url(), mSlaveConfig );
  if (!slave)
  {
    slotSlaveError(0, KIO::ERR_COULD_NOT_CONNECT, "");
    return;
  }
  url.setPath(QString("/index"));
  job = KIO::get( url.url(), false, false );
  connectJob();
}

void KMAcctExpPop::slotJobFinished() {
  QStringList emptyList;
  if (stage == List) {
    kdDebug(5006) << "stage == List" << endl;
    KURL url = getUrl();
    url.setPath(QString("/uidl"));
    job = KIO::get( url.url(), false, false );
    connectJob();
    stage = Uidl;
  }
  else if (stage == Uidl) {
    kdDebug(5006) << "stage == Uidl" << endl;
    mUidlFinished = TRUE;
    stage = Retr;
    numMsgs = idsOfMsgsPendingDownload.count();
    numBytesToRead = 0;
    QValueList<int>::Iterator len = lensOfMsgsPendingDownload.begin();
    for (len = lensOfMsgsPendingDownload.begin();
      len != lensOfMsgsPendingDownload.end(); len++)
        numBytesToRead += *len;
    slotGetNextMsg();
    processMsgsTimer.start(processingDelay);

  }
  else if (stage == Retr) {
    kdDebug(5006) << "stage == Retr" << endl;
    KMMessage *msg = new KMMessage;
    curMsgData.resize(curMsgData.size() + 1);
    curMsgData[curMsgData.size() - 1] = '\0';
    msg->fromString(QCString(curMsgData),TRUE);
    kdDebug(5006) << QString( "curMsgData.size() %1" ).arg( curMsgData.size() ) << endl;

    msgsAwaitingProcessing.append(msg);
    msgIdsAwaitingProcessing.append(idsOfMsgs[indexOfCurrentMsg]);
    msgUidsAwaitingProcessing.append(uidsOfMsgs[indexOfCurrentMsg]);
    // Have to user timer otherwise littleProgress only works for
    // one job->get call.
    ss->start( 0, true );
  }
  else if (stage == Dele) {
    kdDebug(5006) << "stage == Dele" << endl;
    if (idsOfMsgsToDelete.isEmpty())
    {
      KURL url = getUrl();
      url.setPath(QString("/commit"));
      job = KIO::get(  url.url(), false, false );
      stage = Quit;
    } else {
      KURL::List::Iterator it = idsOfMsgsToDelete.begin();
      job = KIO::file_delete( *it, false );
      idsOfMsgsToDelete.remove(it);
    }
    connectJob();
  }
  else if (stage == Quit) {
    kdDebug(5006) << "stage == Quit" << endl;
    job = 0L;
    if (slave) KIO::Scheduler::disconnectSlave(slave);
    slave = NULL;
    stage = Idle;
    KMBroadcastStatus::instance()->setStatusProgressPercent( 100 );
    int numMessages = (KMBroadcastStatus::instance()->abortRequested()) ?
      indexOfCurrentMsg : idsOfMsgs.count();
    if (numMessages > 0) {
      QString msg = i18n("Transmission completed. (%n message, %1 KB)",
      "Transmission completed. (%n messages, %1 KB)", numMessages)
        .arg(numBytesRead/1024);
      if (numBytesToRead != numBytes && mLeaveOnServer)
        msg += " " + i18n("(%1 KB remain on the server)").arg(numBytes/1024);
      KMBroadcastStatus::instance()->setStatusMsg( msg );
    } else {
      KMBroadcastStatus::instance()->setStatusMsg(i18n("Transmission completed." ));
    }
    KMBroadcastStatus::instance()->setStatusProgressEnable( false );
    KMBroadcastStatus::instance()->reset();

    emit finishedCheck(numMessages > 0);
  }
}

void KMAcctExpPop::processRemainingQueuedMessagesAndSaveUidList()
{
  slotProcessPendingMsgs(); // Force processing of any messages still in the queue
  processMsgsTimer.stop();

  stage = Quit;
  // Don't update the seen uid list unless we successfully got
  // a new list from the server
  if (!mUidlFinished) return;
  QString seenUidList = locateLocal( "appdata", mLogin + ":" + "@" + mHost +
				       ":" + QString("%1").arg(mPort) );
  KConfig config( seenUidList );
  config.writeEntry( "seenUidList", uidsOfNextSeenMsgs );
  config.sync(); 
}

void KMAcctExpPop::slotGetNextMsg()
{
  QStringList::Iterator next = idsOfMsgsPendingDownload.begin();
  QValueList<int>::Iterator nextLen = lensOfMsgsPendingDownload.begin();

  curMsgData.resize(0);
  numMsgBytesRead = 0;
  curMsgLen = 0;
  if (curMsgStrm)
    delete curMsgStrm;
  curMsgStrm = 0;

  if (next == idsOfMsgsPendingDownload.end()) {
    processRemainingQueuedMessagesAndSaveUidList();


    if (mLeaveOnServer || idsOfMsgsToDelete.isEmpty()) {
      KURL url = getUrl();
      url.setPath(QString("/commit"));
      job = KIO::get(url.url(), false, false );
    }
    else {
      stage = Dele;
      KURL::List::Iterator it = idsOfMsgsToDelete.begin();
      job = KIO::file_delete( *it, false );
      idsOfMsgsToDelete.remove(it);
    }
  }
  else {
    curMsgStrm = new QDataStream( curMsgData, IO_WriteOnly );
    curMsgLen = *nextLen;
    ++indexOfCurrentMsg;
    job = KIO::get( *next, false, false );
    idsOfMsgsPendingDownload.remove( next );
    kdDebug(5006) << QString("Length of message about to get %1").arg( *nextLen ) << endl;
    lensOfMsgsPendingDownload.remove( nextLen ); //xxx
  }
  connectJob();
}

void KMAcctExpPop::slotData( KIO::Job* job, const QByteArray &data)
{
  if (data.size() == 0) {
    kdDebug(5006) << "Data: <End>" << endl;
    if ((stage == Retr) && (numMsgBytesRead < curMsgLen))
      numBytesRead += curMsgLen - numMsgBytesRead;
    return;
  }

  int oldNumMsgBytesRead = numMsgBytesRead;
  if (stage == Retr) {
    curMsgStrm->writeRawBytes( data.data(), data.size() );
    numMsgBytesRead += data.size();
    if (numMsgBytesRead > curMsgLen)
      numMsgBytesRead = curMsgLen;
    numBytesRead += numMsgBytesRead - oldNumMsgBytesRead;
    dataCounter++;
    if (dataCounter % 5 == 0)
    {
      QString msg = i18n("Message ") + QString("%1/%2 (%3/%4 KB)").
        arg(indexOfCurrentMsg+1).arg(numMsgs).arg(numBytesRead/1024).
        arg(numBytesToRead/1024);
      if (numBytes != numBytesToRead && mLeaveOnServer)
        msg += " " + i18n("(%1 KB remain on the server)").arg(numBytes/1024);
      KMBroadcastStatus::instance()->setStatusMsg( msg );
      KMBroadcastStatus::instance()->setStatusProgressPercent(
        numBytesRead * 100 / numBytesToRead );
    }
    return;
  }

  // otherwise stage is List Or Uidl
  QString qdata = data;
  int spc = qdata.find( ' ' );
  if (spc > 0) {
    KURL url = getUrl();

    if (stage == List) {
      QString length = qdata.mid(spc+1);
      if (length.find(' ') != -1) length = length.left(length.find(' '));
      int len = length.toInt();
      numBytes += len;
      QString id = qdata.left(spc);
      idsOfMsgs.append( id );
      lensOfMsgsPendingDownload.append( len );
      url.setPath(QString("/download/%1").arg(id));
      idsOfMsgsPendingDownload.append(url.url());
    }
    else { // stage == Uidl
      QString uid = qdata.mid(spc + 1);
      uidsOfMsgs.append( uid );
      if (uidsOfSeenMsgs.contains(uid)) {
        QString id = qdata.left(spc);
        url.setPath(QString("/download/%1").arg(id));
        int idx = idsOfMsgsPendingDownload.findIndex(url.url());
        if (idx != -1) {
          lensOfMsgsPendingDownload.remove( lensOfMsgsPendingDownload
                                            .at( idx ));
          idsOfMsgsPendingDownload.remove(url.url());
          idsOfMsgs.remove( id );
          uidsOfMsgs.remove( uid );
        }
        else
          kdDebug(5006) << "KMAcctExpPop::slotData synchronization failure." << endl;
        url.setPath(QString("/%1").arg(id));
        if (uidsOfSeenMsgs.contains( uid ))
          idsOfMsgsToDelete.append(url.url());
        uidsOfNextSeenMsgs.append( uid );
      }
    }
  }
  else {
    stage = Idle;
    if (job) job->kill();
    job = 0L;
    slave = 0L;
    KMessageBox::error(0, i18n( "Unable to complete LIST operation" ),
                          i18n("Invalid response from server"));
    return;
  }
}

void KMAcctExpPop::slotResult( KIO::Job* )
{
  if (!job) return;
  if ( job->error() )
  {
    if (interactive) {
      // force the dialog to be shown next time the account is checked
      if (!mStorePasswd) mPasswd = "";
      job->showErrorDialog();
    }
    slotCancel();
  }
  else
    slotJobFinished();
}

void KMAcctExpPop::slotSlaveError(KIO::Slave *aSlave, int error,
  const QString &errorMsg)
{
  if (aSlave != slave) return;
  if (interactive)
  {
    if (error == KIO::ERR_SLAVE_DIED)
    {
      slave = NULL;
      KMessageBox::error(0,
      i18n("The process for \n%1\ndied unexpectedly").arg(errorMsg));
    } else
      KMessageBox::error(0, i18n("Error connecting to %1:\n\n%2")
        .arg(mHost).arg(errorMsg));
  }
  stage = Quit;
  slotCancel();
}
