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
#include <kapp.h>
#include <kstddirs.h>
#include <qlayout.h>

#include "kmacctexppop.h"
#include "kalarmtimer.h"
#include "kmglobal.h"
#include "kbusyptr.h"
#include "kmacctfolder.h"
#include "kmfiltermgr.h"
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
  processingDelay = 2*100;
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
void KMAcctExpPop::setPrecommand(const QString& cmd)
{
   mPrecommand = cmd;
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
				     const char *caption,
			             const char *login, QString passwd)
  :QDialog(parent,name,true)
{
  // This function pops up a little dialog which asks you 
  // for a new username and password if one of them was wrong or not set.
  QLabel *l;

  kernel->kbp()->idle();
  act = account;
  KWin::setIcons(winId(), kapp->icon(), kapp->miniIcon());
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
  processRemainingQueuedMessagesAndSaveUidList();
  slotJobFinished();
}

void KMAcctExpPop::slotProcessPendingMsgs()
{
  if ((stage != Idle) && (stage != Quit))
    KMBroadcastStatus::instance()->setStatusMsg( i18n("Message ") + QString("%1/%2 (%3 KB)").arg(indexOfCurrentMsg+1).arg(numMsgs).arg(numBytesRead/1024) );

  QString prefix;
  bool addedOk;
  QValueList<KMMessage*>::Iterator cur = msgsAwaitingProcessing.begin();
  QStringList::Iterator curId = msgIdsAwaitingProcessing.begin();
  QStringList::Iterator curUid = msgUidsAwaitingProcessing.begin();

  if (mUseSSL) {
    prefix = "spop3://" + mLogin + ":" + decryptStr(mPasswd) + "@" + mHost
    + ":" + QString("%1").arg(mPort);
  } else {
    prefix = "pop3://" + mLogin + ":" + decryptStr(mPasswd) + "@" + mHost
    + ":" + QString("%1").arg(mPort);
  }

  while (cur != msgsAwaitingProcessing.end()) {
    addedOk = processNewMsg(*cur); //added ok? Error displayed if not.
    if (!addedOk) {
      idsOfMsgsPendingDownload.clear();
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
}

void KMAcctExpPop::startJob() {
  QString text;

  if (mUseSSL) {
    text = "spop3://" + mLogin + ":" + decryptStr(mPasswd) + "@" + 
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
  idsOfMsgs.clear();
  uidsOfMsgs.clear();
  idsOfMsgsToDelete.clear();
  indexOfCurrentMsg = -1;
  KMBroadcastStatus::instance()->reset();
  KMBroadcastStatus::instance()->setStatusProgressEnable( true );
  KMBroadcastStatus::instance()->setStatusMsg( 
	i18n("Preparing transmission from %1...").arg(mHost));

  numBytes = 0;
  numBytesRead = 0;
  stage = List;  
  job = KIO::get( text, false, false );
  connectJob();
}

void KMAcctExpPop::slotJobFinished() {
  QStringList emptyList;
  if (stage == List) {
    debug( "stage == List" );
    QString command;
    if (mUseSSL) {
      command = "spop3://" + mLogin + ":" + decryptStr(mPasswd) + "@" + mHost
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
    debug( "stage == Uidl" );
    stage = Retr;
    numMsgs = idsOfMsgsPendingDownload.count();
    slotGetNextMsg();
    processMsgsTimer.start(processingDelay);

  }
  else if (stage == Retr) {
    debug( "stage == Retr" );
    KMMessage *msg = new KMMessage;
    msg->fromString(curMsgData,TRUE);
    msgsAwaitingProcessing.append(msg);
    msgIdsAwaitingProcessing.append(idsOfMsgs[indexOfCurrentMsg]);
    msgUidsAwaitingProcessing.append(uidsOfMsgs[indexOfCurrentMsg]);
    // Have to user timer otherwise littleProgress only works for
    // one job->get call.
    ss->start( 0, true );
  }
  else if (stage == Dele) {
    debug( "stage == Dele" );
    QString prefix;
    if (mUseSSL) {
      prefix = "spop3://" + mLogin + ":" + decryptStr(mPasswd) + "@" + 
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
    debug( "stage == Quit" );
    job = 0L;
    stage = Idle;
    KMBroadcastStatus::instance()->setStatusProgressPercent( 100 );
    if( idsOfMsgs.count() > 0 ) {
      KMBroadcastStatus::instance()->setStatusMsg(i18n("Transmission completed (%1 messages) (%2 KB)...").arg(idsOfMsgs.count()).arg(numBytesRead/1024));
    } else {
      KMBroadcastStatus::instance()->setStatusMsg(i18n("No new messages."));
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
  if (KMBroadcastStatus::instance()->abortRequested()) {
    slotCancel();
    return;
  }

  if (next == idsOfMsgsPendingDownload.end()) {
    processRemainingQueuedMessagesAndSaveUidList();
    QString prefix;
    if (mUseSSL) {
      prefix = "spop3://" + mLogin + ":" + decryptStr(mPasswd) + "@" + 
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
    curMsgData = "";
    ++indexOfCurrentMsg;
    KMBroadcastStatus::instance()->setStatusMsg( i18n("Message ") + QString("%1/%2 (%3 KB)").arg(indexOfCurrentMsg+1).arg(numMsgs).arg(numBytesRead/1024) );
    KMBroadcastStatus::instance()->setStatusProgressPercent( 
      ((indexOfCurrentMsg)*100) / numMsgs );
//      ((indexOfCurrentMsg + 1)*100) / numMsgs );

    job = KIO::get( *next, false, false );
    idsOfMsgsPendingDownload.remove( next );
  }
  connectJob();
}

void KMAcctExpPop::slotData( KIO::Job* job, const QByteArray &data)
{
  if (data.size() == 0) {
    debug( "Data: <End>");
    return;
  }

  if (stage == Retr) {
    curMsgData += data;
    curMsgData += '\n';
    numBytesRead += data.size() + 2;
    return;
  }

  // otherwise stage is List Or Uidl
  QString qdata = data;
  int spc = qdata.find( ' ' );
  if (spc > 0) {
    QString text;
    if (mUseSSL) {
      text = "spop3://" + mLogin + ":" + decryptStr(mPasswd) + "@" + 
              mHost + ":" + QString("%1/download/").arg(mPort);
    } else {
      text = "pop3://" + mLogin + ":" + decryptStr(mPasswd) + "@" + 
              mHost + ":" + QString("%1/download/").arg(mPort);
    }
    if (stage == List) {
      QString length = qdata.mid(spc+1);
      numBytes += length.toInt();
      QString id = qdata.left(spc);
      idsOfMsgs.append( id );
      idsOfMsgsPendingDownload.append( text + id );
    }
    else { // stage == Uidl
      QString uid = qdata.mid(spc + 1);
      uidsOfMsgs.append( uid );
      if (uidsOfSeenMsgs.contains(uid)) {
	if (!mRetrieveAll) {
	  QString id = qdata.left(spc);
	  idsOfMsgsPendingDownload.remove( text + id );
	  idsOfMsgs.remove( id );
	  uidsOfMsgs.remove( uid );
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
    if (interactive)
      job->showErrorDialog();
    slotCancel();
  }
  else
    slotJobFinished();
}

