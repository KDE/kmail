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

#include "kmacctexppop.h"
#include "kalarmtimer.h"
#include "kmglobal.h"
#include "kbusyptr.h"
#include "kmacctfolder.h"
#include "kmfiltermgr.h"
#include <kprocess.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <qmessagebox.h> // just for kioslave testing
#include <qtooltip.h>
#include "kmbroadcaststatus.h"

#include <kwin.h>
#include <kbuttonbox.h>

//-----------------------------------------------------------------------------
KMAcctExpPop::KMAcctExpPop(KMAcctMgr* aOwner, const char* aAccountName):
  KMAcctExpPopInherited(aOwner, aAccountName)
{
  initMetaObject();

  mUseSSL = FALSE;
  mStorePasswd = FALSE;
  mLeaveOnServer = FALSE;
  mRetrieveAll = TRUE;
  mProtocol = 3;
  struct servent *serv = getservbyname("pop-3", "tcp");
  if (serv) {
    mPort = ntohs(serv->s_port);
  } else {
    mPort = 110;
  }
  job = 0L;
  stage = Idle;
  indexOfCurrentMsg = -1;
  curMsgStrm = 0;
  processingDelay = 2*100;
  mProcessing = false;
  connect(&processMsgsTimer,SIGNAL(timeout()),SLOT(slotProcessPendingMsgs()));
  ss = new QTimer();
  connect( ss, SIGNAL( timeout() ), this, SLOT( slotGetNextMsg() ));
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
  return "advanced pop";
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
  mStorePasswd = FALSE;
  mLeaveOnServer = FALSE;
  mRetrieveAll = TRUE;
}

//-----------------------------------------------------------------------------
void KMAcctExpPop::pseudoAssign(KMAccount* account)
{
  assert(account->type() == "advanced pop");
  KMAcctExpPop *acct = static_cast<KMAcctExpPop*>(account);
  setName(acct->name());
  setCheckInterval(acct->checkInterval());
  setCheckExclude(acct->checkExclude());
  setFolder(acct->folder());
  setHost(acct->host());
  setPort(acct->port());
  setLogin(acct->login());
  setUseSSL(acct->useSSL());
  setStorePasswd(acct->storePasswd());
  setPasswd(acct->passwd(), acct->storePasswd());
  setLeaveOnServer(acct->leaveOnServer());
  setRetrieveAll(acct->retrieveAll());
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
  mStorePasswd = config.readNumEntry("store-passwd", TRUE);
  if (mStorePasswd) mPasswd = config.readEntry("passwd");
  else mPasswd = "";
  mHost = config.readEntry("host");
  mPort = config.readNumEntry("port");
  mProtocol = config.readNumEntry("protocol");
  mLeaveOnServer = config.readNumEntry("leave-on-server", FALSE);
  mRetrieveAll = config.readNumEntry("retrieve-all", FALSE);
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::writeConfig(KConfig& config)
{
  KMAcctExpPopInherited::writeConfig(config);

  config.writeEntry("login", mLogin);
  config.writeEntry("use-ssl", mUseSSL);
  config.writeEntry("store-passwd", mStorePasswd);
  if (mStorePasswd) config.writeEntry("passwd", mPasswd);
  else config.writeEntry("passwd", "");

  config.writeEntry("host", mHost);
  config.writeEntry("port", static_cast<int>(mPort));
  config.writeEntry("protocol", mProtocol);
  config.writeEntry("leave-on-server", mLeaveOnServer);
  config.writeEntry("retrieve-all", mRetrieveAll);
}


//-----------------------------------------------------------------------------
const QString KMAcctExpPop::encryptStr(const QString aStr) const
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
const QString KMAcctExpPop::decryptStr(const QString aStr) const
{
  return encryptStr(aStr);
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::setUseSSL(bool b)
{
  mUseSSL = b;
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
void KMAcctExpPop::setRetrieveAll(bool b)
{
  mRetrieveAll = b;
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::setLogin(const QString& aLogin)
{
  mLogin = aLogin;
}
//-----------------------------------------------------------------------------
const QString KMAcctExpPop::passwd(void) const
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
			             KMAcctExpPop *account ,
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

  if ((stage != Idle) && (stage != Quit))
    KMBroadcastStatus::instance()->setStatusMsg( i18n("Message ") + QString("%1/%2 (%3/%4 KB)").arg(indexOfCurrentMsg+1).arg(numMsgs).arg(numBytesRead/1024).arg(numBytes/1024) );

  QString prefix;
  bool addedOk;
  QValueList<KMMessage*>::Iterator cur = msgsAwaitingProcessing.begin();
  QStringList::Iterator curId = msgIdsAwaitingProcessing.begin();
  QStringList::Iterator curUid = msgUidsAwaitingProcessing.begin();

  prefix = KURL::encode_string( mLogin ) + ":" +
	   KURL::encode_string(decryptStr(mPasswd)) + "@" + mHost  + ":" +
	   QString("%1").arg(mPort);
  if (mUseSSL)
      prefix = "pop3s://" + prefix;
  else
      prefix = "pop3://" + prefix;

  while (cur != msgsAwaitingProcessing.end()) {
    // note we can actually end up processing events in processNewMsg
    // this happens when send receipts is turned on
    // hence the check for re-entry at the start of this method.
    // -sanders Update processNewMsg should no longer process events

    addedOk = processNewMsg(*cur); //added ok? Error displayed if not.
    /*
    if ((*cur)->parent()) {
      int count = (*cur)->parent()->count();
      if ((*cur)->parent()->operator[](count - 1) == *cur)
	(*cur)->parent()->unGetMsg(count - 1);
    }
    */

    if (!addedOk) {
      idsOfMsgsPendingDownload.clear();
      lensOfMsgsPendingDownload.clear();
      msgIdsAwaitingProcessing.clear();
      msgUidsAwaitingProcessing.clear();
      break;
    }
    else {
      idsOfMsgsToDelete.append(prefix + QString("/%1").arg(*curId));
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
  if (stage != List) return;
  stage = Idle;
  job->kill();
  job = 0L;
  slotCancel();
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::startJob() {
  QString text;

  // Run the precommand
  if (!runPrecommand(precommand()))
    {
      QMessageBox::warning(0, i18n("Kmail Error Message"),
			    i18n("Couldn't execute precommand: %1").arg(precommand()) );
      emit finishedCheck(idsOfMsgs.count() > 0);
      return;
    }
  // end precommand code

  if (mUseSSL) {
    text = "pop3s://" + mLogin + ":" + decryptStr(mPasswd) + "@" +
            mHost + ":" + QString("%1").arg(mPort) + "/index";
  } else {
    text = "pop3://" + mLogin + ":" + decryptStr(mPasswd) + "@" +
            mHost + ":" + QString("%1").arg(mPort) + "/index";
  }
  KURL url = text;
  if ( url.isMalformed() ) {
    QMessageBox::critical(0, i18n("Kioslave Error Message"),
			  i18n("Source URL is malformed") );
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
  job = KIO::get( text, false, false );
  connectJob();
}

void KMAcctExpPop::slotJobFinished() {
  QStringList emptyList;
  if (stage == List) {
    kdDebug() << "stage == List" << endl;
    QString command;
    if (mUseSSL) {
      command = "pop3s://" + mLogin + ":" + decryptStr(mPasswd) + "@" + mHost
        + ":" + QString("%1/uidl").arg(mPort);
    } else {
      command = "pop3://" + mLogin + ":" + decryptStr(mPasswd) + "@" + mHost
        + ":" + QString("%1/uidl").arg(mPort);
    }
    job = KIO::get( command, false, false );
    connectJob();
    stage = Uidl;
  }
  else if (stage == Uidl) {
    kdDebug() << "stage == Uidl" << endl;
    stage = Retr;
    numMsgs = idsOfMsgsPendingDownload.count();
    slotGetNextMsg();
    processMsgsTimer.start(processingDelay);

  }
  else if (stage == Retr) {
    kdDebug() << "stage == Retr" << endl;
    KMMessage *msg = new KMMessage;
    msg->fromString(curMsgData,TRUE);
    kdDebug() << QString( "curMsgData.size() %1" ).arg( curMsgData.size() ) << endl;

    msgsAwaitingProcessing.append(msg);
    msgIdsAwaitingProcessing.append(idsOfMsgs[indexOfCurrentMsg]);
    msgUidsAwaitingProcessing.append(uidsOfMsgs[indexOfCurrentMsg]);
    // Have to user timer otherwise littleProgress only works for
    // one job->get call.
    ss->start( 0, true );
  }
  else if (stage == Dele) {
    kdDebug() << "stage == Dele" << endl;
    QString prefix;
    if (mUseSSL) {
      prefix = "pop3s://" + mLogin + ":" + decryptStr(mPasswd) + "@" +
                mHost + ":" + QString("%1").arg(mPort);
    } else {
      prefix = "pop3://" + mLogin + ":" + decryptStr(mPasswd) + "@" +
                mHost + ":" + QString("%1").arg(mPort);
    }
    job = KIO::get(  prefix + "/commit", false, false );
    connectJob();
    stage = Quit;
  }
  else if (stage == Quit) {
    kdDebug() << "stage == Quit" << endl;
    job = 0L;
    stage = Idle;
    KMBroadcastStatus::instance()->setStatusProgressPercent( 100 );
    if( idsOfMsgs.count() > 0 ) {
      KMBroadcastStatus::instance()->setStatusMsg(i18n("Transmission completed (%1 messages) (%2 KB)...").arg(idsOfMsgs.count()).arg(numBytesRead/1024));
    } else {
      KMBroadcastStatus::instance()->setStatusMsg(i18n("Transmission completed..." ));	
    }
    kapp->processEvents(200);
    KMBroadcastStatus::instance()->setStatusProgressEnable( false );
    KMBroadcastStatus::instance()->reset();

    emit finishedCheck(idsOfMsgs.count() > 0);
  }
}

void KMAcctExpPop::processRemainingQueuedMessagesAndSaveUidList()
{
  int oldStage = stage;
  slotProcessPendingMsgs(); // Force processing of any messages still in the queue
  processMsgsTimer.stop();

  stage = Quit;
  // Don't update the seen uid list unless we successfully got
  // a new list from the server
  if ((oldStage == List) || (oldStage == Uidl))
    return;
  QString seenUidList = locateLocal( "appdata", mLogin + ":" + "@" + mHost +
				       ":" + QString("%1").arg(mPort) );
  KConfig config( seenUidList );
  config.writeEntry( "seenUidList", uidsOfNextSeenMsgs );
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

  if (KMBroadcastStatus::instance()->abortRequested()) {
    slotCancel();
    return;
  }

  if (next == idsOfMsgsPendingDownload.end()) {
    processRemainingQueuedMessagesAndSaveUidList();
    QString prefix;
    if (mUseSSL) {
      prefix = "pop3s://" + mLogin + ":" + decryptStr(mPasswd) + "@" +
                mHost + ":" + QString("%1").arg(mPort);
    } else {
      prefix = "pop3://" + mLogin + ":" + decryptStr(mPasswd) + "@" +
                mHost + ":" + QString("%1").arg(mPort);
    }

    if (mLeaveOnServer || idsOfMsgsToDelete.isEmpty())
      job = KIO::get(  prefix + "/commit", false, false );
    else {
      stage = Dele;
      job = KIO::del( idsOfMsgsToDelete, false, false );
    }
  }
  else {
    curMsgStrm = new QDataStream( curMsgData, IO_WriteOnly );
    curMsgLen = *nextLen;
    ++indexOfCurrentMsg;
    KMBroadcastStatus::instance()->setStatusMsg( i18n("Message ") + QString("%1/%2 (%3/%4 KB)").arg(indexOfCurrentMsg+1).arg(numMsgs).arg(numBytesRead/1024).arg(numBytes/1024));
    KMBroadcastStatus::instance()->setStatusProgressPercent(
      ((indexOfCurrentMsg)*100) / numMsgs );
//      ((indexOfCurrentMsg + 1)*100) / numMsgs );

    job = KIO::get( *next, false, false );
    idsOfMsgsPendingDownload.remove( next );
    kdDebug() << QString("Length of message about to get %1").arg( *nextLen ) << endl;
    lensOfMsgsPendingDownload.remove( nextLen ); //xxx
  }
  connectJob();
}

void KMAcctExpPop::slotData( KIO::Job* job, const QByteArray &data)
{
  if (data.size() == 0) {
    kdDebug() << "Data: <End>" << endl;
    if (numMsgBytesRead < curMsgLen)
      numBytesRead += curMsgLen - numMsgBytesRead;
    return;
  }

  int oldNumMsgBytesRead = numMsgBytesRead;
  if (stage == Retr) {
    curMsgStrm->writeRawBytes( data.data(), data.size() );
    curMsgStrm->writeRawBytes( "\n", 1 );
    numMsgBytesRead += data.size() + 1;
    if (numMsgBytesRead > curMsgLen)
      numMsgBytesRead = curMsgLen;
    numBytesRead += numMsgBytesRead - oldNumMsgBytesRead;
    return;
  }

  // otherwise stage is List Or Uidl
  QString qdata = data;
  int spc = qdata.find( ' ' );
  if (spc > 0) {
    QString text;
    if (mUseSSL) {
      text = "pop3s://" + mLogin + ":" + decryptStr(mPasswd) + "@" +
              mHost + ":" + QString("%1/download/").arg(mPort);
    } else {
      text = "pop3://" + mLogin + ":" + decryptStr(mPasswd) + "@" +
              mHost + ":" + QString("%1/download/").arg(mPort);
    }
    if (stage == List) {
      QString length = qdata.mid(spc+1);
      int len = length.toInt();
      numBytes += len;
      QString id = qdata.left(spc);
      idsOfMsgs.append( id );
      lensOfMsgsPendingDownload.append( len );
      idsOfMsgsPendingDownload.append( text + id );
    }
    else { // stage == Uidl
      QString uid = qdata.mid(spc + 1);
      uidsOfMsgs.append( uid );
      if (uidsOfSeenMsgs.contains(uid)) {
	if (!mRetrieveAll) {
	  QString id = qdata.left(spc);
	  int idx = idsOfMsgsPendingDownload.findIndex( text + id );
	  if (idx != -1) {
	    lensOfMsgsPendingDownload.remove( lensOfMsgsPendingDownload
					      .at( idx ));
	    idsOfMsgsPendingDownload.remove( text + id );
	    idsOfMsgs.remove( id );
	    uidsOfMsgs.remove( uid );
	  }
	  else
	    kdDebug() << "KMAcctExpPop::slotData synchronization failure." << endl;
	}
	uidsOfNextSeenMsgs.append( uid );
      }
    }
  }
  else {
    stage = Idle;
    job->kill();
    job = 0L;
    QMessageBox::critical(0, i18n("Invalid response from server"),
			  i18n( "Unable to complete LIST operation" ));
    return;
  }
}

void KMAcctExpPop::slotResult( KIO::Job* )
{
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

