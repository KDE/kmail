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
#include <qtstream.h>
#include <kconfig.h>
#include <qlined.h>
#include <qpushbt.h>
#include <klocale.h>

#include "kmacctpop.h"
#include "kalarmtimer.h"
#include "kmglobal.h"
#include "kbusyptr.h"
#include "kmacctfolder.h"
#include "kmfiltermgr.h"


//-----------------------------------------------------------------------------
KMAcctPop::KMAcctPop(KMAcctMgr* aOwner, const char* aAccountName):
  KMAcctPopInherited(aOwner, aAccountName)
{
  initMetaObject();

  mStorePasswd = FALSE;
  mLeaveOnServer = TRUE;
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
}


//-----------------------------------------------------------------------------
bool KMAcctPop::processNewMail(void)
{
  void (*oldHandler)(int);
  bool result;

  warning("POP support is still experimental\nand may not work.");

  // Before we do anything else let's ignore the friggin' SIGALRM signal
  // This signal somehow interrupts the network functions and messed up
  // DwPopClient::Open().
  oldHandler = signal(SIGALRM, SIG_IGN);
  result = doProcessNewMail();
  signal(SIGALRM, oldHandler);

  return result;
}


//-----------------------------------------------------------------------------
bool KMAcctPop::doProcessNewMail(void)
{
  DwPopClient client;
  QString passwd;
  QString response, status;
  int num, size;	// number of all msgs / size of all msgs
  int id;		// id of message to read
  int dummy;
  char dummyStr[32];
  KMMessage* msg;
  bool gotMsgs = FALSE;

  // is everything specified ?
  if (mHost.isEmpty() || mPort<=0 || mLogin.isEmpty())
  {
    warning(nls->translate("Please specify Host, Port, Login, and\n"
			   "destination folder in the settings\n"
			   "and try again."));
    return FALSE;
  }

  client.SetReceiveTimeout(20);
  passwd = decryptStr(mPasswd);

  if (client.Open(mHost,mPort) != '+')
    return popError("OPEN", client);
  if (client.User(mLogin) != '+')
    return popError("USER", client);
  if (client.Pass(decryptStr(passwd)) != '+')
    return popError("PASS", client);

  if (client.Stat() != '+') return popError("STAT", client);
  response = client.SingleLineResponse().c_str();
  sscanf(response.data(), "%3s %d %d", dummyStr, &num, &size);

#ifdef DWPOPCLIENT_HAS_NO_LAST
  if (client.Last() != '+') return popError("LAST", client);
  response = client.SingleLineResponse().c_str();
  response >> status >> id;
  id++;
#else
  id = 1;
#endif

  while (id <= num)
  {
    debug("processing message %d", id);

    if (client.List(id) != '+')
      return popError("LIST", client);
    response = client.SingleLineResponse().c_str();
    sscanf(response.data(), "%3s %d %o", dummyStr, &dummy, &size);

    if (client.Retr(id) != '+')
      return popError("RETR", client);
    response = client.MultiLineResponse().c_str();

    msg = new KMMessage;
    msg->fromString(response);
    mFolder->addMsg(msg);

    if (!mLeaveOnServer && client.Dele(id) != '+')
      return popError("DELE", client);

    gotMsgs = TRUE;
    id++;
  }

  return gotMsgs;
}


//-----------------------------------------------------------------------------
bool KMAcctPop::popError(const QString aStage, DwPopClient& aClient) const
{
  KMsgBox::message(0, "Pop-Mail Error", nls->translate("In ")+aStage+":\n"+
		   aClient.MultiLineResponse().c_str());
  return FALSE;
}


//-----------------------------------------------------------------------------
void KMAcctPop::readConfig(KConfig& config)
{
  KMAcctPopInherited::readConfig(config);

  mLogin = config.readEntry("login", "");
  mStorePasswd = config.readNumEntry("store-passwd", TRUE);
  if (mStorePasswd) 
    mPasswd = decryptStr(config.readEntry("passwd"));
  else 
    mPasswd = "";
  mHost = config.readEntry("host");
  mPort = config.readNumEntry("port");
  mProtocol = config.readNumEntry("protocol");
}


//-----------------------------------------------------------------------------
void KMAcctPop::writeConfig(KConfig& config)
{
  KMAcctPopInherited::writeConfig(config);

  config.writeEntry("login", mLogin);
  config.writeEntry("store-passwd", mStorePasswd);
  if (mStorePasswd)
  {
    // very primitive password encryption
    config.writeEntry("passwd", encryptStr(mPasswd));
  }
  else config.writeEntry("passwd", "");

  config.writeEntry("host", mHost);
  config.writeEntry("port", mPort);
  config.writeEntry("protocol", mProtocol);
}


//-----------------------------------------------------------------------------
const QString KMAcctPop::encryptStr(const QString aStr)
{
  unsigned int i, val;
  unsigned int len = aStr.length();
  QString result(len+1);

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
const QString KMAcctPop::decryptStr(const QString aStr)
{
  return encryptStr(aStr);
}


//-----------------------------------------------------------------------------
void KMAcctPop::setLogin(const QString& aLogin)
{
  mLogin = aLogin;
}


//-----------------------------------------------------------------------------
void KMAcctPop::setPasswd(const QString& aPasswd, bool aStoreInConfig)
{
  mPasswd = aPasswd;
  mStorePasswd = aStoreInConfig;
}


//-----------------------------------------------------------------------------
void KMAcctPop::setHost(const QString& aHost)
{
  mHost = aHost;
}


//-----------------------------------------------------------------------------
void KMAcctPop::setPort(int aPort)
{
  mPort = aPort;
}


//-----------------------------------------------------------------------------
void KMAcctPop::setProtocol(short aProtocol)
{
  assert(aProtocol==2 || aProtocol==3);
  mProtocol = aProtocol;
}


//=============================================================================
//
//  Class  KMPasswdDialog
//
//=============================================================================

KMPasswdDialog::KMPasswdDialog(QWidget *parent , const char *name ,
			       const char *caption, const char *login,
			       const char *passwd)
  :QDialog(parent,name,true)
{
  // This function pops up a little dialog which asks you 
  // for a new username and password if one of them was wrong.

  kbp->idle();
  setMaximumSize(300,180);
  setMinimumSize(300,180);
  setCaption(caption);

  QLabel *label = new QLabel(this);
  label->setText(nls->translate("Login Name:"));
  label->resize(label->sizeHint());

  label->move(20,30);
  usernameLEdit = new QLineEdit(this,"NULL");
  usernameLEdit->setText(login);
  usernameLEdit->setGeometry(100,27,150,25);
  
  QLabel *label1 = new QLabel(this);
  label1->setText(nls->translate("Password:"));
  label1->resize(label1->sizeHint());
  label1->move(20,80);

  passwdLEdit = new QLineEdit(this,"NULL");
  passwdLEdit->setEchoMode(QLineEdit::Password);
  passwdLEdit->setText(passwd);
  passwdLEdit->setGeometry(100,76,150,25);
  connect(passwdLEdit,SIGNAL(returnPressed()),SLOT(slotOkPressed()));

  ok = new QPushButton("Ok" ,this,"NULL");
  ok->setGeometry(55,130,70,25);
  connect(ok,SIGNAL(pressed()),this,SLOT(slotOkPressed()));

  cancel = new QPushButton("Cancel", this);
  cancel->setGeometry(180,130,70,25);
  connect(cancel,SIGNAL(pressed()),this,SLOT(slotCancelPressed()));

}

//-----------------------------------------------------------------------------
void KMPasswdDialog::slotOkPressed()
{
  kbp->busy();
  done(1);
}

//-----------------------------------------------------------------------------
void KMPasswdDialog::slotCancelPressed()
{
  kbp->busy();
  done(0);
}
