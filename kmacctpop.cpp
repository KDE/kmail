// kmacctpop.cpp

#include "kmacctpop.moc"
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <mimelib/mimepp.h>
#include <kmfolder.h>

//-----------------------------------------------------------------------------
KMAcctPop::KMAcctPop(KMAcctMgr* aOwner, const char* aAccountName):
  KMAcctPopInherited(aOwner, aAccountName)
{
  mStorePasswd = FALSE;
  mProtocol = 3;
  mPort = 110;
}


//-----------------------------------------------------------------------------
KMAcctPop::~KMAcctPop()
{
  writeConfig();
}


//-----------------------------------------------------------------------------
const char* KMAcctPop::type(void) const
{
  return "pop";
}


//-----------------------------------------------------------------------------
void KMAcctPop::init(void)
{
  mStorePasswd = FALSE;
  mPort = 110;
  mHost = "";
  mHost.detach();

  mLogin.detach();
  mProtocol = 3;
  mPasswd = "";
  mPasswd.detach();
}


//-----------------------------------------------------------------------------
bool KMAcctPop::processNewMail(void)
{
  debug("processNewMail");
  DwPopClient client;
  int replyCode;
  int num, size;
  QString status, response;

  cout << mHost << endl;
  cout << mPort << endl;
  cout << mLogin << endl;
  cout << mPasswd << endl;

  client.Open(mHost, mPort); 
  if(!client.IsOpen())
    {KMsgBox::message(0,"Network error!","Could not open connection to " +
		      mHost +"!");
    return false;
    }
  cout << client.SingleLineResponse().c_str();
  replyCode = client.User(mLogin); // Send USER command
  cout << client.SingleLineResponse().c_str();
  replyCode = client.Pass(mPasswd); // Send PASS command
  cout << client.SingleLineResponse().c_str();
  replyCode = client.Stat(); // Send STAT command
  response = client.SingleLineResponse().c_str();
    
  QTextStream str(response, IO_ReadOnly);
  str >> status >> num >> size;
  debug("GOT POP %s %d %d",status.data(), num, size);
  if(num == 0)
    {KMsgBox::message(0,"checking Mail...","No new messages on " + mHost);
    return false;
    }
  for (int i=1; i<=num; i++)
  {
    debug("processing message %d", i);
    
    client.Retr(i);
    char buffer[300];
    strncpy(buffer, client.MultiLineResponse().c_str(), 299);
    
    buffer[299]=0;

    debug("GOT %s", buffer);
    DwMessage *dmsg = new DwMessage(client.MultiLineResponse());
    dmsg->Parse();
    /*
      KMMessage *msg = new KMMessage((KMFolder*)mFolder, dmsg);
      mFolder->addMsg(msg);
      Stephan: Und nun?
    */
  }
  return (num > 0);
}

void KMAcctPop::passwdError()
{
  // This function pops up a little dialog which asks you 
  // for a new username and password if one of them was wrong.

  newWidget = new QWidget(0);
  newWidget->setMaximumSize(300,180);
  newWidget->setMinimumSize(300,180);
  newWidget->setCaption("Login Error!");

  QLabel *label = new QLabel(newWidget);
  label->setText("Login Name:");
  label->resize(label->sizeHint());

  label->move(20,30);
  usernameLEdit = new QLineEdit(newWidget,"NULL");
  usernameLEdit->setText(mLogin);
  usernameLEdit->setGeometry(100,27,150,25);
  
  QLabel *label1 = new QLabel(newWidget);
  label1->setText("Password:");
  label1->resize(label1->sizeHint());
  label1->move(20,80);

  passwdLEdit = new QLineEdit(newWidget,"NULL");
  passwdLEdit->setEchoMode(QLineEdit::Password);
  passwdLEdit->setGeometry(100,76,150,25);

  ok = new QPushButton("Ok" ,newWidget,"NULL");
  ok->setGeometry(55,130,70,25);
  connect(ok,SIGNAL(pressed()),this,SLOT(slotOkPressed()));

  cancel = new QPushButton("Cancel", newWidget);
  cancel->setGeometry(180,130,70,25);
  connect(cancel,SIGNAL(pressed()),this,SLOT(slotCancelPressed()));

  newWidget->show();
}

void KMAcctPop::slotOkPressed()
{
  mLogin = usernameLEdit->text();
  mPasswd = passwdLEdit->text();
  delete newWidget;
  processNewMail();
}

void KMAcctPop::slotCancelPressed()
{
  delete newWidget;
}


//-----------------------------------------------------------------------------
void KMAcctPop::readConfig(void)
{
  mConfig->setGroup("Account");
  mLogin = mConfig->readEntry("login");
  mStorePasswd = mConfig->readNumEntry("store-passwd");
  if (mStorePasswd) 
    mPasswd = mConfig->readEntry("passwd");
  else 
    mPasswd="?";
  mHost = mConfig->readEntry("host");
  mPort = mConfig->readNumEntry("port");
  mProtocol = mConfig->readNumEntry("protocol");
}


//-----------------------------------------------------------------------------
void KMAcctPop::writeConfig(void)
{
  //char *buf;
  mConfig->setGroup("Account");
  mConfig->writeEntry("type", "pop");
  mConfig->writeEntry("login", mLogin);
  if (mStorePasswd) 
    {/*buf = crypt(mPasswd.data(),"AA");
    mPasswd.sprintf("%s",buf);*/
    mConfig->writeEntry("password", mPasswd);}
    
  else 
    mConfig->writeEntry("passwd", "");
  mConfig->writeEntry("store-passwd", mStorePasswd);
  mConfig->writeEntry("host", mHost);
  mConfig->writeEntry("port", mPort);
  mConfig->writeEntry("protocol", mProtocol);
  mConfig->sync();
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


