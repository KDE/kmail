// kmsender.cpp

#include "kmsender.h"
#include "kmfoldermgr.h"
#include "kmmessage.h"
#include "kmglobal.h"
#include "kmacctfolder.h"

#include <Kconfig.h>
#include <kapp.h>
#include <kprocess.h>
#include <klocale.h>

#include <assert.h>
#include <stdio.h>

#define SENDER_GROUP "sending mail"

//-----------------------------------------------------------------------------
KMSender::KMSender(KMFolderMgr* aFolderMgr)
{
  QString outboxName;

  mFolderMgr = aFolderMgr;
  mCfg = KApplication::getKApplication()->getConfig();

  mMailerProc = NULL;

  mCfg->setGroup(SENDER_GROUP);
  mMethod = (enum KMSender::Method)mCfg->readNumEntry("method", 2);
  mSendImmediate = (bool)mCfg->readNumEntry("immediate", TRUE);
  mMailer = mCfg->readEntry("mailer", QString("/usr/sbin/sendmail"));
  mSmtpHost = mCfg->readEntry("smtphost", QString("localhost"));
  mSmtpPort = mCfg->readNumEntry("smptport", 25);

  mCfg->setGroup("General");
  outboxName = mCfg->readEntry("outbox", QString("outbox"));

  mQueue = mFolderMgr->find(outboxName);
  if (!mQueue) 
  {
    warning(nls->translate("The folder `%s' does not exist in the\n"
			   "mail folder directory. The mail sender depends\n"
			   "on this folder and will not work without it.\n"
			   "Therefore the folder will now be created."),
	    (const char*)outboxName);

    mQueue = mFolderMgr->createFolder(outboxName, TRUE);
    if (!mQueue) fatal(nls->translate("Cannot create the folder `%s'."),
		       (const char*)outboxName);
  }

  mQueue->setType("out");
  mQueue->setLabel(nls->translate("outbox"));
}


//-----------------------------------------------------------------------------
KMSender::~KMSender()
{
  if (mMailerProc) delete mMailerProc;
}


//-----------------------------------------------------------------------------
bool KMSender::sendQueued(void)
{
  return FALSE;
}


//-----------------------------------------------------------------------------
bool KMSender::send(KMMessage* aMsg, short sendNow)
{
  assert(aMsg != NULL);

  if (sendNow==-1) sendNow = mSendImmediate;

  if (!sendNow) return (mQueue->addMsg(aMsg)==0);

  if (mMethod == smSMTP) return sendSMTP(aMsg);
  else if (mMethod == smMail) return sendMail(aMsg);
  else warning(nls->translate("Please specify a send\nmethod in the settings\n"
			      "and try again."));

  return FALSE;
}


//-----------------------------------------------------------------------------
bool KMSender::sendSMTP(KMMessage* msg)
{
  // $markus: I just could not resist implementing smtp suppport
  // This code just must be stable. I checked every darn return code!
  // Date: 24. Sept. 97
  
  QString str;
  int replyCode;
  DwSmtpClient client;
  DwString dwString;
  DwString dwSrc;


  debug("Msg has %i parts\n",msg->numBodyParts());
  // Now we check if message is multipart.
  if(msg->numBodyParts() != 0) // If message is not a simple text message
    {
    }
  else
    {dwSrc = msg->body();
     DwToCrLfEol(dwSrc,dwString); // Convert to CRLF 
    }

  cout << mSmtpHost << endl;
  cout << mSmtpPort << endl;
  client.Open(mSmtpHost,mSmtpPort); // Open connection
  cout << client.Response().c_str();
  if(!client.IsOpen) // Check if connection succeded
    {KMsgBox::message(0,"Network Error!","Could not open connection to " +
		      mSmtpHost +"!");
    return false;
    }
  
  replyCode = client.Helo(); // Send HELO command
  if(replyCode != 250 && replyCode != 0)
    {KMsgBox::message(0,"Error!",client.LastErrorStr());
    if(client.Close() != 0)
      {KMsgBox::message(0,"Network Error!","Could not close connection to " +
		       mSmtpHost + "!");
      return false;
      }  
    return false;
    }
  else if(replyCode == 0 )
    {KMsgBox::message(0,"Network Error!",client.LastErrorStr());
    return false;
    }
  else  
    cout << client.Response().c_str();

  str = msg->from(); // Check if from is set.
  if(str.isEmpty())
    {KMsgBox::message(0,"?","How could you get this far without a from Field");
    if(client.Close() != 0)
      {KMsgBox::message(0,"Network Error!","Could not close connection to " +
		       mSmtpHost + "!");
      return false;
      }
    return false;
    }

  replyCode = client.Mail(msg->from());
  if(replyCode != 250 && replyCode != 0) // Send MAIL command
     {KMsgBox::message(0,"Error",client.LastErrorStr());
     if(client.Close() != 0)
      {KMsgBox::message(0,"Network Error!","Could not close connection to " +
		       mSmtpHost + "!");
      return false;
      }
     return false;
     }
  else if(replyCode == 0 )
    {KMsgBox::message(0,"Network Error!",client.LastErrorStr());
    return false;
    }    
  else
    cout << client.Response().c_str();

  str = msg->to(); // Check if to is set.
  if(str.isEmpty())
    {KMsgBox::message(0,"?","How could you get this far without a to Field");
    if(client.Close() != 0)
      {KMsgBox::message(0,"Network Error!","Could not close connection to " +
		       mSmtpHost + "!");
      return false;
      }
    return false;
    }
  replyCode = client.Rcpt(msg->to()); // Send RCPT command
  if(replyCode != 250 && replyCode != 251 && replyCode != 0)
    {KMsgBox::message(0,"Error",client.LastErrorStr());
    if(client.Close() != 0)
      {KMsgBox::message(0,"Network Error!","Could not close connection to " +
		       mSmtpHost + "!");
      return false;
      }
    return false;
    }  
  else if(replyCode == 0 )
    {KMsgBox::message(0,"Network Error!",client.LastErrorStr());
    return false;
    }    
  else
    cout << client.Response().c_str();

  str = msg->cc();
  if(!str.isEmpty())  // Check if cc is set.
    {replyCode = client.Rcpt(msg->cc()); // Send RCPT command
    if(replyCode != 250 && replyCode != 251 && replyCode != 0)
      {KMsgBox::message(0,"Error",client.LastErrorStr());
      if(client.Close() !=0)
	{KMsgBox::message(0,"Network Error!","Could not close connection to " +
		       mSmtpHost + "!");
	return false;
	}
      return false;
      }
    else if(replyCode == 0 )
      {KMsgBox::message(0,"Network Error!",client.LastErrorStr());
      return false;
      }    
    else
      cout << client.Response().c_str();
    }

  str = msg->bcc(); // Check if bcc ist set.
  if(!str.isEmpty())
    {replyCode = client.Rcpt(msg->bcc()); // Send RCPT command
    if(replyCode != 250 && replyCode != 251 && replyCode != 0)
      {KMsgBox::message(0,"Error",client.LastErrorStr());
      if(client.Close() != 0)
	{KMsgBox::message(0,"Network Error!","Could not close connection to " +
		       mSmtpHost + "!");
	return false;
	}
      return false;
      }
    else if(replyCode == 0 )
      {KMsgBox::message(0,"Network Error!",client.LastErrorStr());
      return false;
      }    
    else
      cout << client.Response().c_str();
    }

  replyCode = client.Data();
  if(replyCode != 354 && replyCode != 0) // Send DATA command
    {KMsgBox::message(0,"Error!",client.LastErrorStr());
    if(client.Close() != 0)
      {KMsgBox::message(0,"Network Error!","Could not close connection to " +
		       mSmtpHost + "!");
      return false;
      }
    return false;
    }
  else if(replyCode == 0 )
    {KMsgBox::message(0,"Network Error!",client.LastErrorStr());
    return false;
    }    
  else
    cout << client.Response().c_str();

  replyCode = client.SendData(dwString);
  if(replyCode != 250 && replyCode != 0) // Send data.
    {KMsgBox::message(0,"Error!",client.LastErrorStr());
    if(client.Close() != 0 )
      {KMsgBox::message(0,"Network Error!","Could not close connection to " +
		       mSmtpHost + "!");
      return false;
      }
    return false;
    }
  else if(replyCode == 0 )
    {KMsgBox::message(0,"Network Error!",client.LastErrorStr());
    return false;
    }    
  else
    cout << client.Response().c_str();

  replyCode = client.Quit(); // Send QUIT command
  if(replyCode != 221 && replyCode != 0)
    {KMsgBox::message(0,"Error!",client.LastErrorStr());
    if(client.Close() != 0 )
      {KMsgBox::message(0,"Network Error!","Could not close connection to " +
		       mSmtpHost + "!");
      return false;
      }
    return false;
    }
  else if(replyCode == 0 )
    {KMsgBox::message(0,"Network Error!",client.LastErrorStr());
    return false;
    }    
  else
    cout << client.Response().c_str();

  return true;

}


//-----------------------------------------------------------------------------
bool KMSender::sendMail(KMMessage* aMsg)
{
  char  msgFileName[1024];
  FILE* msgFile;
  const char* msgstr = aMsg->asString();
  QString sendCmd;

  // write message to temporary file such that mail can
  // process it afterwards easily.
  tmpnam(msgFileName);
  msgFile = fopen(msgFileName, "w");
  fwrite(msgstr, strlen(msgstr), 1, msgFile);
  fclose(msgFile);

  if (!mMailerProc) mMailerProc = new KProcess;

  if (mMailer.isEmpty())
  {
    warning(nls->translate("Please specify a mailer program\nin the settings."));
    return FALSE;
  }

  sendCmd = mMailer.copy();
  sendCmd += " \"";
  sendCmd += aMsg->to();
  sendCmd += "\" < ";
  sendCmd += msgFileName;

  mMailerProc->setExecutable("/bin/sh");
  *mMailerProc << "-c" << (const char*)sendCmd;

  debug("sending message with command: "+sendCmd);
  mMailerProc->start(KProcess::Block);
  debug("sending done");

  unlink(msgFileName);
  return true;
}


//-----------------------------------------------------------------------------
void KMSender::setMethod(Method aMethod)
{
  mMethod = aMethod;
  mCfg->setGroup(SENDER_GROUP);
  mCfg->writeEntry("method", (int)mMethod);
}


//-----------------------------------------------------------------------------
void KMSender::setSendImmediate(bool aSendImmediate)
{
  mSendImmediate = aSendImmediate;
  mCfg->setGroup(SENDER_GROUP);
  mCfg->writeEntry("immediate", (int)mSendImmediate);
}


//-----------------------------------------------------------------------------
void KMSender::setMailer(const QString& aMailer)
{
  mMailer = aMailer;
  mCfg->setGroup(SENDER_GROUP);
  mCfg->writeEntry("mailer", mMailer);
}


//-----------------------------------------------------------------------------
void KMSender::setSmtpHost(const QString& aSmtpHost)
{
  mSmtpHost = aSmtpHost;
  mCfg->setGroup(SENDER_GROUP);
  mCfg->writeEntry("smtphost", mSmtpHost);
}


//-----------------------------------------------------------------------------
void KMSender::setSmtpPort(int aSmtpPort)
{
  mSmtpPort = aSmtpPort;
  mCfg->setGroup(SENDER_GROUP);
  mCfg->writeEntry("smtpport", mSmtpPort);
}
