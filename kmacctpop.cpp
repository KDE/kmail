// kmacctpop.cpp
// Authors: Stefan Taferner and Markus Wuebben

#include "kmacctpop.moc"

#include <assert.h>
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

#include "kmacctpop.h"
#include "kalarmtimer.h"
#include "kmglobal.h"
#include "kbusyptr.h"
#include "kmacctfolder.h"
#include "kmfiltermgr.h"
#include "kmbroadcaststatus.h"
#include "kprocess.h"
#include <klocale.h>
#include <kmessagebox.h>

#include <qlayout.h>
#include <kwm.h>
#include <kstddirs.h>
#include <kbuttonbox.h>

//-----------------------------------------------------------------------------
KMAcctPop::KMAcctPop(KMAcctMgr* aOwner, const char* aAccountName):
  KMAcctPopInherited(aOwner, aAccountName)
{
  initMetaObject();

  mStorePasswd = FALSE;
  mLeaveOnServer = FALSE;
  mRetrieveAll = TRUE;
  mProtocol = 3;
  mPort = 110;
}


//-----------------------------------------------------------------------------
KMAcctPop::~KMAcctPop()
{
}


//-----------------------------------------------------------------------------
const char* KMAcctPop::type(void) const
{
  return "pop";
}


//-----------------------------------------------------------------------------
void KMAcctPop::init(void)
{
  mHost   = "";
  mPort   = 110;
  mLogin  = "";
  mPasswd = "";
  mProtocol = 3;
  mStorePasswd = FALSE;
  mLeaveOnServer = FALSE;
  mRetrieveAll = TRUE;
}


//-----------------------------------------------------------------------------
void KMAcctPop::processNewMail(bool interactive)
{
  void (*oldHandler)(int);
  void (*pipeHandler)(int);
  bool hasNewMail;

  // Before we do anything else let's ignore the friggin' SIGALRM signal
  // This signal somehow interrupts the network functions and messed up
  // DwPopClient::Open().
  oldHandler = signal(SIGALRM, SIG_IGN);
  // Another one of those nice little SIGNALS which default action is to 
  // abort the app when received. SIGPIPE is send when e.g the client attempts
  // to write to a TCP socket when the connection was shutdown by the server.
  pipeHandler = signal(SIGPIPE, SIG_IGN);
  KMBroadcastStatus::instance()->reset();
  KMBroadcastStatus::instance()->setStatusMsg( 
                     i18n( "Preparing transmission..." ));
  KMBroadcastStatus::instance()->setStatusProgressEnable( true );
  hasNewMail = doProcessNewMail(interactive);
  KMBroadcastStatus::instance()->setStatusProgressEnable( false );
  KMBroadcastStatus::instance()->reset();

  signal(SIGALRM, oldHandler);
  signal(SIGPIPE, pipeHandler);
  emit finishedCheck(hasNewMail);
}


//-----------------------------------------------------------------------------
bool KMAcctPop::authenticate(DwPopClient& client)
{
  QString passwd;
  const char* msg=0;
  KMPasswdDialog* dlg;
  bool opened = FALSE;
  int replyCode;
  KProcess precommandProcess;

  if(mPasswd.isEmpty() || mLogin.isEmpty())
    msg = i18n("Please set Password and Username");

  passwd = decryptStr(mPasswd);
  

  while (1)
  {
    if (msg)
    {
      dlg = new KMPasswdDialog(NULL, NULL, this, msg,
			       mLogin, passwd);
      if (!dlg->exec()) return FALSE;
      delete dlg;
      msg = 0;
    }

    // Some POP servers close the connection upon failed
    // user/password. So we close the connection here to
    // be sure that everything below runs as we expect it.
    if (opened)
    {
      client.Quit();
      client.Close();
    }

    // Run the pre command if there is one
    // Not sure if this should be outside the while loop or not - mpilone 
    if (mPrecommand.length() != 0)
    {
       KMBroadcastStatus::instance()->setStatusMsg( 
	  i18n( QString("Executing precommand ") + mPrecommand ));
       kapp->processEvents();
       qDebug("Running precommand %s", mPrecommand.ascii());
       precommandProcess << mPrecommand;
       if (!precommandProcess.start(KProcess::Block))
       {
	  return popError(QString("Couldn't execute precommand:\n") + mPrecommand, client);
       }
    }
      
    // Open connection to server
    if (client.Open(mHost,mPort) != '+')
      return popError("OPEN", client);
    opened = TRUE;
    kapp->processEvents();

    // Send user name
    replyCode = client.User((const char*)mLogin);
    if (replyCode == '-')
    {
      msg = i18n("Incorrect Username");
      continue;
    }
    else if (replyCode != '+')
      return popError("USER", client);
    kapp->processEvents();

    // Send password
    passwd = decryptStr(mPasswd);
    
    replyCode = client.Pass((const char*)passwd);
    if (replyCode == '-')
    {
      msg = i18n("Incorrect Password");
      continue;
    }
    else if (replyCode != '+')
      return popError("PASS", client);

    // Ok, we are done
    break;
  }

  KMBroadcastStatus::instance()->setStatusMsg( 
		     i18n( "Transmission completed..." ));
  return TRUE;
}


//-----------------------------------------------------------------------------
bool KMAcctPop::doProcessNewMail(bool /* interactive */)
{
  DwPopClient client;
  QString passwd;
  QString response, status;
  int num, size;	// number of all msgs / size of all msgs
  int id, i;		// id of message to read
  int tmout;
  int dummy;
  char dummyStr[32];
  // int replyCode; // ReplyCode need from User & Passwd call.
  KMMessage* msg;
  gotMsgs = FALSE;
  bool doFetchMsg;
  bool addedOk;   //Flag if msg was delivered succesfully

  // is everything specified ?
  kapp->processEvents();

  if (mHost.isEmpty() || mPort==0)
  {
    KMessageBox::information(0,i18n("Please specify Host, Port  and\n"
				    "destination folder in the settings\n"
				    "and try again."));
    return FALSE;
  }

  client.SetReceiveTimeout(20);

  // Handle connect, user, and password.
  if (!authenticate(client)) return FALSE;

  if (client.Stat() != '+') return popError("STAT", client);
  response = client.SingleLineResponse().c_str();
  sscanf(response.data(), "%3s %d %d", dummyStr, &num, &size);

//#warning "*** If client.Last() cannot be found then install the latest kdesupport"
  if (client.Last() == '+' && !mRetrieveAll)
  {
    response = client.SingleLineResponse().c_str();
    sscanf(response.data(), "%3s %d", dummyStr, &id);
    id++;
  }
  else id = 1;

  // workaround but still is no good. If msgs are too big in size
  // we will get a timeout.
  client.SetReceiveTimeout(40);

  addedOk = true;
 
  // do while there are mesages to take and last msg wass added succesfully
  while (id <= num && addedOk)
  {
    client.SetReceiveTimeout(40);

    if (KMBroadcastStatus::instance()->abortRequested()) {
      client.Quit();
      return gotMsgs;
    }

    KMBroadcastStatus::instance()->setStatusMsg( i18n("Message ") +
			                QString("%1/%2").arg(id).arg(num) );
    KMBroadcastStatus::instance()->setStatusProgressPercent( (id*100) / num );

    kapp->processEvents();
    if (client.List(id) != '+')
      return popError("LIST", client);
    response = client.SingleLineResponse().c_str();
    sscanf(response.data(), "%3s %d %d", dummyStr, &dummy, &size);

    doFetchMsg = TRUE;
    if (size > 4500 && !mRetrieveAll)
    {
      // If the message is large it is cheaper to first load
      // the header to check the status of the message.
      // We will have to load the entire message with the RETR
      // command below, so we will not fetch the header for
      // small messages.
      if (client.Top(id,1) != '+')
	return popError("TOP", client);
      response = client.MultiLineResponse().c_str();
      i = response.find("\nStatus:");
      if (i<0) i = response.find("\rStatus:");
      if (i>=0)
      {
	status = response.mid(i+8,32).stripWhiteSpace();
	if (strnicmp(status,"RO",2)==0 ||
	    strnicmp(status,"OR",2)==0)
	{
	  doFetchMsg=FALSE;
	  debug("message %d is old -- no need to download it", id);
	}
      }
    }

    if (doFetchMsg)
    {
      // set timeout depending on size
      tmout = size >> 8;
      if (tmout < 30) tmout = 30;
      client.SetReceiveTimeout(tmout);

      if (client.Retr(id) != '+')
      return popError("RETR", client);
      response = client.MultiLineResponse().c_str();

      msg = new KMMessage;
      msg->fromString(response,TRUE);
      if (mRetrieveAll || msg->status()!=KMMsgStatusOld)
      addedOk = processNewMsg(msg); //added ok? Error displayed if not.
      else delete msg;
    }

    // If we should delete from server _and_ we added ok then delete it
    if(!mLeaveOnServer && addedOk)
    {
      if(client.Dele(id) != '+')
	return popError("DELE",client);
      else 
	cout << client.SingleLineResponse().c_str();
    }

    gotMsgs = TRUE;
    id++;
  }
  client.Quit();

  return gotMsgs;
}


//-----------------------------------------------------------------------------
bool KMAcctPop::popError(const QString aStage, DwPopClient& aClient) const
{
  QString msg, caption;
  kernel->kbp()->idle();

  caption = i18n("Pop Mail Error");

  // First we assume the worst: A network error
  if (aClient.LastFailure() != DwProtocolClient::kFailNoFailure)
  {
    caption = i18n("Pop Mail Network Error");
    msg = aClient.LastFailureStr();
  }

  // Maybe it is an app specific error
  else if (aClient.LastError() != DwProtocolClient::kErrNoError)
  {
    msg = aClient.LastErrorStr();
  }
  
  // Not all commands return multiLineResponses. If they do not
  // they return singleLineResponses and the multiLR command return NULL
  else
  {
    msg = aClient.MultiLineResponse().c_str();
    if (msg.isEmpty()) msg = aClient.SingleLineResponse().c_str();
    if (msg.isEmpty()) msg = i18n("Unknown error");
    // Negative response by the server e.g STAT responses '- ....'
  }

  QString tmp;
  tmp = i18n("Account: %1\nIn %2:\n%3")
		.arg(name())
		.arg(aStage)
		.arg(msg);
  KMessageBox::information(0, tmp, caption);
  //kernel->kbp()->busy();
  aClient.Quit();
  return gotMsgs;
}


//-----------------------------------------------------------------------------
void KMAcctPop::readConfig(KConfig& config)
{
  KMAcctPopInherited::readConfig(config);


  mLogin = config.readEntry("login", "");
  mStorePasswd = config.readNumEntry("store-passwd", TRUE);
  if (mStorePasswd) mPasswd = config.readEntry("passwd");
  else mPasswd = "";
  mHost = config.readEntry("host");
  mPort = config.readNumEntry("port");
  mProtocol = config.readNumEntry("protocol");
  mLeaveOnServer = config.readNumEntry("leave-on-server", FALSE);
  mRetrieveAll = config.readNumEntry("retrieve-all", FALSE);
  mPrecommand = config.readEntry("precommand");
}


//-----------------------------------------------------------------------------
void KMAcctPop::writeConfig(KConfig& config)
{
  KMAcctPopInherited::writeConfig(config);

  config.writeEntry("login", mLogin);
  config.writeEntry("store-passwd", mStorePasswd);
  if (mStorePasswd) config.writeEntry("passwd", mPasswd);
  else config.writeEntry("passwd", "");

  config.writeEntry("host", mHost);
  config.writeEntry("port", static_cast<int>(mPort));
  config.writeEntry("protocol", mProtocol);
  config.writeEntry("leave-on-server", mLeaveOnServer);
  config.writeEntry("retrieve-all", mRetrieveAll);
  config.writeEntry("precommand", mPrecommand);
}


//-----------------------------------------------------------------------------
const QString KMAcctPop::encryptStr(const QString aStr) const
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
const QString KMAcctPop::decryptStr(const QString aStr) const
{
  return encryptStr(aStr);
}


//-----------------------------------------------------------------------------
void KMAcctPop::setStorePasswd(bool b)
{
  mStorePasswd = b;
}

//-----------------------------------------------------------------------------
void KMAcctPop::setLeaveOnServer(bool b)
{
  mLeaveOnServer = b;
}


//-----------------------------------------------------------------------------
void KMAcctPop::setRetrieveAll(bool b)
{
  mRetrieveAll = b;
}


//-----------------------------------------------------------------------------
void KMAcctPop::setLogin(const QString& aLogin)
{
  mLogin = aLogin;
}
//-----------------------------------------------------------------------------
const QString KMAcctPop::passwd(void) const
{
  return decryptStr(mPasswd);
}

//-----------------------------------------------------------------------------
void KMAcctPop::setPasswd(const QString& aPasswd, bool aStoreInConfig)
{
  mPasswd = encryptStr(aPasswd);
  mStorePasswd = aStoreInConfig;
}


//-----------------------------------------------------------------------------
void KMAcctPop::setHost(const QString& aHost)
{
  mHost = aHost;
}


//-----------------------------------------------------------------------------
void KMAcctPop::setPort(unsigned short int aPort)
{
  mPort = aPort;
}


//-----------------------------------------------------------------------------
bool KMAcctPop::setProtocol(short aProtocol)
{
  //assert(aProtocol==2 || aProtocol==3);
  if(aProtocol != 2 || aProtocol != 3)
    return false;
  mProtocol = aProtocol;
  return true;
}

//-----------------------------------------------------------------------------
void KMAcctPop::setPrecommand(const QString& cmd)
{
   mPrecommand = cmd;
}

//=============================================================================
//
//  Class  KMPasswdDialog
//
//=============================================================================

KMPasswdDialog::KMPasswdDialog(QWidget *parent, const char *name, 
			       KMAcctPop *account , const char *caption,
			       const char *login, QString passwd)
  :QDialog(parent,name,true)
{
  // This function pops up a little dialog which asks you 
  // for a new username and password if one of them was wrong or not set.
  QLabel *l;

  kernel->kbp()->idle();
  act = account;
  KWM::setMiniIcon(winId(), kapp->miniIcon());
  setCaption(caption);

  QGridLayout *gl = new QGridLayout(this, 4, 2, 10);

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
  
  l = new QLabel(i18n("Login Name:"), this);
  l->setMinimumSize(l->sizeHint());
  gl->addWidget(l, 1, 0);

  usernameLEdit = new QLineEdit(login, this);
  usernameLEdit->setFixedHeight(usernameLEdit->sizeHint().height());
  usernameLEdit->setMinimumWidth(usernameLEdit->sizeHint().width());
  gl->addWidget(usernameLEdit, 1, 1);

  l = new QLabel(i18n("Password:"), this);
  l->setMinimumSize(l->sizeHint());
  gl->addWidget(l, 2, 0);  

  passwdLEdit = new QLineEdit(this,"NULL");
  passwdLEdit->setEchoMode(QLineEdit::Password);
  passwdLEdit->setText(passwd);
  passwdLEdit->setFixedHeight(passwdLEdit->sizeHint().height());
  passwdLEdit->setMinimumWidth(passwdLEdit->sizeHint().width());
  gl->addWidget(passwdLEdit, 2, 1);
  connect(passwdLEdit, SIGNAL(returnPressed()),
	  SLOT(slotOkPressed()));

  KButtonBox *bbox = new KButtonBox(this);
  bbox->addStretch(1);
  ok = bbox->addButton(i18n("OK"));
  ok->setDefault(true);
  cancel = bbox->addButton(i18n("Cancel"));
  bbox->layout();
  gl->addMultiCellWidget(bbox, 3, 3, 0, 1);

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
void KMPasswdDialog::slotOkPressed()
{
  //kernel->kbp()->busy();
  act->setLogin(usernameLEdit->text());
  act->setPasswd(passwdLEdit->text(), act->storePasswd());
  done(1);
}

//-----------------------------------------------------------------------------
void KMPasswdDialog::slotCancelPressed()
{
  //kernel->kbp()->busy();
  done(0);
}
