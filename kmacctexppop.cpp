// KMAcctExpPop.cpp
// Authors: Don Sanders, (based on kmacctpop by)
//          Stefan Taferner and Markus Wuebben

#include "kmacctexppop.moc"

#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <mimelib/mimepp.h>
#include <kmmessage.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kio/scheduler.h>
#include <kio/passdlg.h>

#include "kmfiltermgr.h"
#include <klocale.h>
#include <kmessagebox.h>
#include "kmbroadcaststatus.h"
#include "kmfoldermgr.h"


//-----------------------------------------------------------------------------
KMAcctExpPop::KMAcctExpPop(KMAcctMgr* aOwner, const QString& aAccountName):
  KMAcctExpPopInherited(aOwner, aAccountName)
{
  init();
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
QString KMAcctExpPop::type(void) const
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
  mUsePipelining = FALSE;
  mStorePasswd = FALSE;
  mAskAgain = FALSE;
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
  slotAbortRequested();
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
  setUsePipelining(acct->usePipelining());
  setStorePasswd(acct->storePasswd());
  setPasswd(acct->passwd(), acct->storePasswd());
  setLeaveOnServer(acct->leaveOnServer());
  setPrecommand(acct->precommand());
}

//-----------------------------------------------------------------------------
void KMAcctExpPop::processNewMail(bool _interactive)
{
  if (stage == Idle) {

    if(mAskAgain || mPasswd.isEmpty() || mLogin.isEmpty()) {
      QString passwd = decryptStr(mPasswd);
      bool b = FALSE;
      if (KIO::PasswordDialog::getNameAndPassword(mLogin, passwd, &b,
        i18n("You need to supply a username and a password to access this "
        "mailbox."), FALSE, QString::null, mName, i18n("Account:"))
        != QDialog::Accepted)
      {
	emit finishedCheck(false);
	return;
      } else {
        mPasswd = encryptStr(passwd);
        mAskAgain = FALSE;
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
  mAuth = config.readEntry("auth", "USER");
  mUsePipelining = config.readNumEntry("pipelining", FALSE);
  mStorePasswd = config.readNumEntry("store-passwd", FALSE);
  if (mStorePasswd) mPasswd = config.readEntry("pass");
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
  config.writeEntry("pipelining", mUsePipelining);
  config.writeEntry("store-passwd", mStorePasswd);
  if (mStorePasswd) config.writeEntry("pass", mPasswd);
  else config.writeEntry("passwd", "");

  config.writeEntry("host", mHost);
  config.writeEntry("port", static_cast<int>(mPort));
  config.writeEntry("protocol", mProtocol);
  config.writeEntry("leave-on-server", mLeaveOnServer);
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
void KMAcctExpPop::setUsePipelining(bool b)
{
  mUsePipelining = b;
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


//-----------------------------------------------------------------------------
void KMAcctExpPop::connectJob() {
  KIO::Scheduler::assignJobToSlave(slave, job);
  if (stage != Dele)
  connect(job, SIGNAL( data( KIO::Job*, const QByteArray &)),
	  SLOT( slotData( KIO::Job*, const QByteArray &)));
  connect(job, SIGNAL( result( KIO::Job * ) ),
	  SLOT( slotResult( KIO::Job * ) ) );
  connect(job, SIGNAL(infoMessage( KIO::Job*, const QString & )),
          SLOT( slotMsgRetrieved(KIO::Job*, const QString &)));
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::slotCancel()
{
  idsOfMsgsPendingDownload.clear();
  lensOfMsgsPendingDownload.clear();
  processRemainingQueuedMessagesAndSaveUidList();
  slotJobFinished();
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::slotProcessPendingMsgs()
{
  if (mProcessing) // not reentrant
    return;
  mProcessing = true;

  bool addedOk;
  QValueList<KMMessage*>::Iterator cur = msgsAwaitingProcessing.begin();
  QStringList::Iterator curId = msgIdsAwaitingProcessing.begin();
  QStringList::Iterator curUid = msgUidsAwaitingProcessing.begin();

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
      idsOfMsgsToDelete.append( *curId );
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
  mSlaveConfig.insert("pipelining", (mUsePipelining) ? "on" : "off");
  mSlaveConfig.insert("tls", (mUseTLS) ? "on" : "off");
  if (mAuth == "PLAIN" || mAuth == "LOGIN" || mAuth == "CRAM-MD5" ||
    mAuth == "DIGEST-MD5")
  {
    mSlaveConfig.insert("auth", "SASL");
    mSlaveConfig.insert("sasl", mAuth);
  }
  else mSlaveConfig.insert("auth", mAuth);
  slave = KIO::Scheduler::getConnectedSlave( url.url(), mSlaveConfig );
  if (!slave)
  {
    slotSlaveError(0, KIO::ERR_CANNOT_LAUNCH_PROCESS, url.protocol());
    return;
  }
  url.setPath(QString("/index"));
  job = KIO::get( url.url(), false, false );
  connectJob();
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::slotMsgRetrieved(KIO::Job*, const QString & infoMsg) {
  if (infoMsg != "message complete") return;
  kdDebug(5006) << "stage == Retr" << endl;
  KMMessage *msg = new KMMessage;
  curMsgData.resize(curMsgData.size() + 1);
  curMsgData[curMsgData.size() - 1] = '\0';
  msg->fromString(QCString(curMsgData),TRUE);
  kdDebug(5006) << QString( "curMsgData.size() %1" ).arg( curMsgData.size() ) << endl;

  msgsAwaitingProcessing.append(msg);
  msgIdsAwaitingProcessing.append(idsOfMsgs[indexOfCurrentMsg]);
  msgUidsAwaitingProcessing.append(uidsOfMsgs[indexOfCurrentMsg]);
  slotGetNextMsg();
}


//-----------------------------------------------------------------------------
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

    if (mLeaveOnServer && uidsOfMsgs.isEmpty() && uidsOfNextSeenMsgs.isEmpty()
      && !idsOfMsgs.isEmpty())
    {
      KMessageBox::sorry(0, i18n("Your POP3 server doesn't support the UIDL "
      "command.\nThis command is required to determine in a reliable way,\n"
      "which of the mails on the server KMail has already seen before.\n"
      "The feature to leave the mails on the server will therefore not\n"
      "work properly."));
    }

    stage = Retr;
    numMsgs = idsOfMsgsPendingDownload.count();
    numBytesToRead = 0;
    QValueList<int>::Iterator len = lensOfMsgsPendingDownload.begin();
    for (len = lensOfMsgsPendingDownload.begin();
      len != lensOfMsgsPendingDownload.end(); len++)
        numBytesToRead += *len;
    KURL url = getUrl();
    url.setPath("/download/" + idsOfMsgsPendingDownload.join(","));
    job = KIO::get( url, false, false );
    connectJob();
    slotGetNextMsg();
    processMsgsTimer.start(processingDelay);
  }
  else if (stage == Retr) {
    processRemainingQueuedMessagesAndSaveUidList();
    kernel->folderMgr()->syncAllFolders();

    KURL url = getUrl();
    if (mLeaveOnServer || idsOfMsgsToDelete.isEmpty()) {
      url.setPath(QString("/commit"));
      job = KIO::get(url.url(), false, false );
    }
    else {
      stage = Dele;
      url.setPath("/remove/" + idsOfMsgsToDelete.join(","));
      idsOfMsgsToDelete.clear();
      job = KIO::get( url, false, false );
    }
    connectJob();
  }
  else if (stage == Dele) {
    kdDebug(5006) << "stage == Dele" << endl;
    KURL url = getUrl();
    url.setPath(QString("/commit"));
    job = KIO::get( url.url(), false, false );
    stage = Quit;
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


//-----------------------------------------------------------------------------
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


//-----------------------------------------------------------------------------
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
  kdDebug(5006) << "KMAcctExpPop::slotGetNextMsg was called too often" << endl;
  }
  else {
    curMsgStrm = new QDataStream( curMsgData, IO_WriteOnly );
    curMsgLen = *nextLen;
    ++indexOfCurrentMsg;
    idsOfMsgsPendingDownload.remove( next );
    kdDebug(5006) << QString("Length of message about to get %1").arg( *nextLen ) << endl;
    lensOfMsgsPendingDownload.remove( nextLen ); //xxx
  }
}


//-----------------------------------------------------------------------------
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
    if (stage == List) {
      QString length = qdata.mid(spc+1);
      if (length.find(' ') != -1) length = length.left(length.find(' '));
      int len = length.toInt();
      numBytes += len;
      QString id = qdata.left(spc);
      idsOfMsgs.append( id );
      lensOfMsgsPendingDownload.append( len );
      idsOfMsgsPendingDownload.append( id );
    }
    else { // stage == Uidl
      QString uid = qdata.mid(spc + 1);
      uidsOfMsgs.append( uid );
      if (uidsOfSeenMsgs.contains(uid)) {
        QString id = qdata.left(spc);
        int idx = idsOfMsgsPendingDownload.findIndex(id);
        if (idx != -1) {
          lensOfMsgsPendingDownload.remove( lensOfMsgsPendingDownload
                                            .at( idx ));
          idsOfMsgsPendingDownload.remove( id );
          idsOfMsgs.remove( id );
          uidsOfMsgs.remove( uid );
        }
        else
          kdDebug(5006) << "KMAcctExpPop::slotData synchronization failure." << endl;
        if (uidsOfSeenMsgs.contains( uid ))
          idsOfMsgsToDelete.append( id );
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


//-----------------------------------------------------------------------------
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


//-----------------------------------------------------------------------------
void KMAcctExpPop::slotSlaveError(KIO::Slave *aSlave, int error,
  const QString &errorMsg)
{
  if (aSlave != slave) return;
  if (error == KIO::ERR_SLAVE_DIED) slave = NULL;
  if (interactive)
    KMessageBox::error(0, KIO::buildErrorString(error, errorMsg));
  stage = Quit;
  if (error == KIO::ERR_COULD_NOT_LOGIN && !mStorePasswd)
    mAskAgain = TRUE;
  /* We need a timer, otherwise slotSlaveError of the next account is also
     executed, if it reuses the slave, because the slave member variable
     is changed too early */
  QTimer::singleShot(0, this, SLOT(slotCancel()));
}
