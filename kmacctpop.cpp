// kmacctpop.cpp
// Author: Stefan Taferner Modification by Markus Wuebben

#include "kmacctpop.moc"
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <mimelib/mimepp.h>
#include <kmfolder.h>
#include "kalarmtimer.h"
#include "kmglobal.h"
#include "kbusyptr.h"

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
  // Before we do anything else let's ignore the friggin' SIGALRM signal
  // This signal somehow interrupts the network functions and messed up
  // DwPopClient::Open().
  signal(SIGALRM,SIG_IGN);

  debug("processNewMail");
  DwPopClient client;
  client.SetReceiveTimeout(20);
  int replyCode;
  int num, size;
  QString status, response;

  cout << "Host: " << mHost << endl;
  cout << "Port: " << mPort << endl;
  cout << "Login: " << mLogin << endl;
  cout << "Passwd: " << mPasswd << endl;

  if(mLogin.isEmpty())
    {KMPasswdDialog d(0,0,"Login name is missing!");
    if(!d.exec())
      return false;
    }
  if(mPasswd.isEmpty())
    {KMPasswdDialog d(0,0,"Password is missing!");
    if(!d.exec())
      return false;
    }

  replyCode = client.Open(mHost,mPort);
  printf("replyCode for Open: %i\n",replyCode);
  if(!replyCode)
	{status.sprintf("%s", client.LastFailureStr());
	KMsgBox::message(0,"Network Error" , status);
        // Reinstall old sighandler
        signal (SIGALRM, KAlarmTimeoutHandler);
	return false;
	}


  replyCode = client.User(mLogin); // Send USER command
  printf("replyCode for User: %i\n",replyCode);
  if(replyCode != 43 && replyCode != 0)
    {KMsgBox::message(0,"Error",client.SingleLineResponse().c_str());
    // Reinstall old sighandler
    signal (SIGALRM, KAlarmTimeoutHandler);
    return false;
    }
  else if(replyCode == 0)
    {KMsgBox::message(0,"Network Error",client.LastFailureStr());
    signal (SIGALRM, KAlarmTimeoutHandler);
    return false;
    }
  else
    cout << client.SingleLineResponse().c_str();

  replyCode = client.Pass(mPasswd); // Send PASS command
  printf("replyCode for Pass: %i\n",replyCode);
    if(replyCode != 43 && replyCode != 0)
    {KMsgBox::message(0,"Error",client.SingleLineResponse().c_str());
    signal (SIGALRM, KAlarmTimeoutHandler);
    return false;
    }
  else if(replyCode == 0)
    {KMsgBox::message(0,"Network Error",client.LastFailureStr());
    signal (SIGALRM, KAlarmTimeoutHandler);
    return false;
    }
  else
    cout << client.SingleLineResponse().c_str();

  replyCode = client.Stat();// Send STAT command
  printf("reply Code1 for stat: %i\n",replyCode);
    if(replyCode != 43 && replyCode != 0)
    {KMsgBox::message(0,"Error",client.SingleLineResponse().c_str());
    signal (SIGALRM, KAlarmTimeoutHandler);
    return false;
    }
  else if(replyCode == 0)
    {KMsgBox::message(0,"Network Error",client.LastFailureStr());
    signal (SIGALRM, KAlarmTimeoutHandler);
    return false;
    }
  else
    response = client.SingleLineResponse().c_str();

  cout << response;  
  QTextStream str(response, IO_ReadOnly);
  str >> status >> num >> size;
  debug("GOT POP %s %d %d",status.data(), num, size);
  if(num == 0)
    {KMsgBox::message(0,"checking Mail...","No new messages on " + mHost);
    signal (SIGALRM, KAlarmTimeoutHandler);
    return false;
    }
  for (int i=1; i<=num; i++)
  {
    debug("processing message %d", i);
    
    // I need the size of the msg for buffer

    replyCode = client.List(i);
    response = client.SingleLineResponse().c_str();
    QTextStream size_str(response,IO_ReadOnly);
    int no;
    long size_msg;
    QString status_msg;
    size_str >> status_msg >> no >> size_msg;
    printf("msg size : %ld\n",size_msg);
    // Shit! How do you convert octets to decs.

    replyCode = client.Retr(i); // Retrieve message
    if(replyCode != 43 && replyCode != 0); // If no such msg num get next.

    else if(replyCode == 0) // Some network error.
      {KMsgBox::message(0,"Network Error",client.LastFailureStr());
      signal (SIGALRM, KAlarmTimeoutHandler);
      return false;
      }
    else // Ok let's get the msg
    {
      cout << client.SingleLineResponse().c_str() << endl;

      char buffer[300];
      strncpy(buffer, client.MultiLineResponse().c_str(), 299);

      buffer[299]='\0';

      debug("GOT %s", buffer);
      DwMessage *dmsg = new DwMessage(client.MultiLineResponse());
      dmsg->Parse();
      /*
        KMMessage *msg = new KMMessage((KMFolder*)mFolder, dmsg);
        mFolder->addMsg(msg);
        Stephan: Und nun?
      */

    }
  }
  // Now let's reinstall the old signal handler for SIGALRM

  signal (SIGALRM, KAlarmTimeoutHandler);
  return (num > 0);

}

KMPasswdDialog::KMPasswdDialog(QWidget *parent , const char *name ,
			       const char *caption )
  :QDialog(parent,name,true)
{
  // This function pops up a little dialog which asks you 
  // for a new username and password if one of them was wrong.

  kbp->idle();
  setMaximumSize(300,180);
  setMinimumSize(300,180);
  setCaption(caption);

  QLabel *label = new QLabel(this);
  label->setText("Login Name:");
  label->resize(label->sizeHint());

  label->move(20,30);
  usernameLEdit = new QLineEdit(this,"NULL");
  //  usernameLEdit->setText(mLogin);
  usernameLEdit->setGeometry(100,27,150,25);
  
  QLabel *label1 = new QLabel(this);
  label1->setText("Password:");
  label1->resize(label1->sizeHint());
  label1->move(20,80);

  passwdLEdit = new QLineEdit(this,"NULL");
  passwdLEdit->setEchoMode(QLineEdit::Password);
  passwdLEdit->setGeometry(100,76,150,25);

  ok = new QPushButton("Ok" ,this,"NULL");
  ok->setGeometry(55,130,70,25);
  connect(ok,SIGNAL(pressed()),this,SLOT(slotOkPressed()));

  cancel = new QPushButton("Cancel", this);
  cancel->setGeometry(180,130,70,25);
  connect(cancel,SIGNAL(pressed()),this,SLOT(slotCancelPressed()));

}

void KMPasswdDialog::slotOkPressed()
{
  kbp->busy();
  done(1);
}

void KMPasswdDialog::slotCancelPressed()
{
  kbp->busy();
  done(0);
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


