// kmacctpop.cpp
// Authors: Stefan Taferner and Markus Wuebben

#include "kmacctpop.moc"

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <mimelib/mimepp.h>
#include <kmfolder.h>
#include <qtstream.h>
#include <kconfig.h>
#include <qlined.h>
#include <qpushbt.h>

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
  // This functions has been rewritten by Markus.

  // Before we do anything else let's ignore the friggin' SIGALRM signal
  // This signal somehow interrupts the network functions and messed up
  // DwPopClient::Open().
  signal(SIGALRM,SIG_IGN);

  debug("processNewMail");
  DwPopClient client;
  client.SetReceiveTimeout(20);

  int replyCode; // the reply Code the server responds
  int num, size; // number of all msgs / size of all msgs
  QString status, response;
  int no;  // Msg number List(i)
  long size_msg; // Size of single msg
  QString status_msg; // Status of List(i)

  cout << "Host: " << mHost << endl;
  cout << "Port: " << mPort << endl;
  cout << "Login: " << mLogin << endl;
  cout << "Passwd: " << mPasswd << endl;


  if(mLogin.isEmpty()) // If the login name is not set call modal Dialog
    {KMPasswdDialog d(0,0,"Login name is missing!",login(),passwd());
    if(!d.exec())
      return false;
    }
  if(mPasswd.isEmpty()) // If the Password is not set call modal Dialog
    {KMPasswdDialog d(0,0,"Password is missing!",login(),passwd());
    if(!d.exec())
      return false;
    }

  replyCode = client.Open(mHost,mPort); // Open connection
  printf("replyCode for Open: %i\n",replyCode);
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
  str >> status >> num >> size; // ReplyCode , number of msgs, size in octets
  debug("GOT POP %s %d %d",status.data(), num, size);

  if(num == 0) // If there are no new msgs.
    {KMsgBox::message(0,"checking Mail...","No new messages on " + mHost);
    signal (SIGALRM, KAlarmTimeoutHandler);
    return false;
    }


  int i = 1; // Start with msg 1
  while(i<=num)
  {
    debug("processing message %d", i);
    
    // I need the size of the msg for buffer

    replyCode = client.List(i); //List (i) to get size for buffer
                                //and check if exists
    if(replyCode != 43 && replyCode != 0) // If no such msg (i) get next.
      {i++;
      break;
      }
    else if(replyCode == 0) // Some network error.
      {KMsgBox::message(0,"Network Error",client.LastFailureStr());
      signal (SIGALRM, KAlarmTimeoutHandler); // Reinstall handler
      return false;
      }
    else
      {response = client.SingleLineResponse().c_str();
      QTextStream size_str(response,IO_ReadOnly);
      size_str >> status_msg >> no >> size_msg; // Same as above
      printf("msg size : %ld\n",size_msg); // Size for buffer in octets
    // Shit! How do I convert octets to decs.
      }


    replyCode = client.Retr(i); // Retrieve message
    if(replyCode != 43 && replyCode != 0) // If no such msg (i) get next.
      {i++;
      break;
      }
    else if(replyCode == 0) // Some network error.
      {KMsgBox::message(0,"Network Error",client.LastFailureStr());
      signal (SIGALRM, KAlarmTimeoutHandler); // Reinstall handler
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
      int error;
      QString err_str;
      KMMessage *msg = new KMMessage((KMFolder*)mFolder, dmsg);
      if((error = mFolder->addMsg(msg)) != 0)
	{err_str = "Adding message to folder failed\n";
	err_str.append(strerror(error));
	KMsgBox::message(0,"Folder error!", err_str);
	}*/   

      i++;   // Next msg;
    }
  }
  // Now let's reinstall the old signal handler for SIGALRM

  signal (SIGALRM, KAlarmTimeoutHandler);
  return (num > 0);

}

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
  label->setText("Login Name:");
  label->resize(label->sizeHint());

  label->move(20,30);
  usernameLEdit = new QLineEdit(this,"NULL");
  usernameLEdit->setText(login);
  usernameLEdit->setGeometry(100,27,150,25);
  
  QLabel *label1 = new QLabel(this);
  label1->setText("Password:");
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
void KMAcctPop::readConfig(KConfig& config)
{
  KMAcctPopInherited::readConfig(config);

  mLogin = config.readEntry("login", "");
  mStorePasswd = config.readNumEntry("store-passwd", 1);
  if (mStorePasswd) 
    mPasswd = config.readEntry("passwd");
  else 
    mPasswd = "?";
  mHost = config.readEntry("host");
  mPort = config.readNumEntry("port");
  mProtocol = config.readNumEntry("protocol");
}


//-----------------------------------------------------------------------------
void KMAcctPop::writeConfig(KConfig& config)
{
  QString cryptPasswd;
  unsigned int i;

  KMAcctPopInherited::readConfig(config);

  config.writeEntry("login", mLogin);
  config.writeEntry("store-passwd", mStorePasswd);
  if (mStorePasswd)
  {
    // very primitive password encryption
    for (i=0; i<mPasswd.length(); i++)
      cryptPasswd[i] = (char)((int)mPasswd[i] ^ 'F');
    config.writeEntry("passwd", mPasswd);
  }
  else config.writeEntry("passwd", "");

  config.writeEntry("host", mHost);
  config.writeEntry("port", mPort);
  config.writeEntry("protocol", mProtocol);
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


