// kmsender.cpp

#include "kmsender.h"
#include "kmfoldermgr.h"
#include "kmmessage.h"
#include "kmglobal.h"
#include "kmacctfolder.h"

#include <kconfig.h>
#include <kapp.h>
#include <kprocess.h>
#include <klocale.h>

#include <assert.h>
#include <stdio.h>

#define SENDER_GROUP "sending mail"

//-----------------------------------------------------------------------------
KMSender::KMSender(KMFolderMgr* aFolderMgr)
{
  QString outboxName, sentName;

  mFolderMgr = aFolderMgr;
  mCfg = KApplication::getKApplication()->getConfig();

  mMailerProc = NULL;

  mCfg->setGroup(SENDER_GROUP);
  mMethod = (enum KMSender::Method)mCfg->readNumEntry("Method", 2);
  mSendImmediate = (bool)mCfg->readNumEntry("Immediate", TRUE);
  mMailer = mCfg->readEntry("Mailer", QString("/usr/sbin/sendmail"));
  mSmtpHost = mCfg->readEntry("Smtp Host", QString("localhost"));
  mSmtpPort = mCfg->readNumEntry("Smtp Port", 25);

  mCfg->setGroup("Identity");
  mEmailAddr= mCfg->readEntry("Email Address", "");
  mUserName = mCfg->readEntry("Name", "");
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
  bool sendOk = FALSE;

  assert(aMsg != NULL);

  if (sendNow==-1) sendNow = mSendImmediate;

  if (!sendNow) return (queuedFolder->addMsg(aMsg)==0);

  if (mMethod == smSMTP) sendOk = sendSMTP(aMsg);
  else if (mMethod == smMail) sendOk = sendMail(aMsg);
  else warning(nls->translate("Please specify a send\nmethod in the settings\n"
			      "and try again."));

  if (sendOk) sentFolder->addMsg(aMsg);
  return sendOk;
}


//-----------------------------------------------------------------------------
bool KMSender::sendSMTP(KMMessage* msg)
{
  // $markus: I just could not resist implementing smtp suppport
  // This code just must be stable. I checked every darn return code!
  // Date: 24. Sept. 97
  
  QString str, fromStr, toStr;
  int replyCode;
  DwSmtpClient client;
  DwString dwString;
  DwString dwSrc;

  assert(msg != NULL);

  debug("Msg has %i parts\n",msg->numBodyParts());
  // Now we check if message is multipart.
  if(msg->numBodyParts() != 0) // If message is not a simple text message
  {
  }
  else
  {
    dwSrc = msg->body();
    DwToCrLfEol(dwSrc,dwString); // Convert to CRLF 
  }

  cout << mSmtpHost << endl;
  cout << mSmtpPort << endl;
  client.Open(mSmtpHost,mSmtpPort); // Open connection
  cout << client.Response().c_str();
  if(!client.IsOpen) // Check if connection succeded
  {
    QString str;
    str.sprintf(nls->translate("Cannot open SMTP connection to\n"
			       "host %s for sending."), 
		(const char*)mSmtpHost);
    warning((const char*)str);
    return false;
  }
  
  replyCode = client.Helo(); // Send HELO command
  cout << client.Response().c_str();
  if(replyCode != 250)
    return smtpFailed(client, "HELO", replyCode);

  assert(!mEmailAddr.isEmpty()); // dump core if email address is unset.

  replyCode = client.Mail((const char*)mEmailAddr);
  cout << client.Response().c_str();
  if(replyCode != 250)
     return smtpFailed(client, "FROM", replyCode);

  toStr = msg->to(); // Check if to is set.
  assert(!toStr.isEmpty()); // dump core if "to" is unset.

  replyCode = client.Rcpt(msg->to()); // Send RCPT command
  cout << client.Response().c_str();
  if(replyCode != 250 && replyCode != 251)
     return smtpFailed(client, "RCPT", replyCode);

  str = msg->cc();
  if(!str.isEmpty())  // Check if cc is set.
  {
    replyCode = client.Rcpt(msg->cc()); // Send RCPT command
    cout << client.Response().c_str();
    if(replyCode != 250 && replyCode != 251)
      return smtpFailed(client, "RCPT", replyCode);
  }

  str = msg->bcc(); // Check if bcc ist set.
  if(!str.isEmpty())
  {
    replyCode = client.Rcpt(msg->bcc()); // Send RCPT command
    cout << client.Response().c_str();
    if(replyCode != 250 && replyCode != 251)
      return smtpFailed(client, "RCPT", replyCode);
  }

  replyCode = client.Data(); // Send DATA command
  cout << client.Response().c_str();
  if(replyCode != 354) 
    return smtpFailed(client, "DATA", replyCode);

  replyCode = client.SendData(dwString);
  cout << client.Response().c_str();
  if(replyCode != 250 && replyCode != 251)
    return smtpFailed(client, "<body>", replyCode);

  replyCode = client.Quit(); // Send QUIT command
  cout << client.Response().c_str();
  if(replyCode != 221)
    return smtpFailed(client, "QUIT", replyCode);

  return true;
}


//-----------------------------------------------------------------------------
bool KMSender::smtpFailed(DwSmtpClient& client, const char* inCommand,
			  int replyCode)
{
  QString str;
  const char* errorStr = client.LastErrorStr();

  str.sprintf(nls->translate("Failed to send mail message\n"
			     "because a SMTP error occured\n"
			     "during the \"%s\" command.\n\n"
			     "Return code: %d\n"
			     "Message: `%s'"), 
	      inCommand, replyCode, errorStr ? errorStr : "(NULL)");
  warning((const char*)str);

  if (replyCode != 0) smtpClose(client);
  return FALSE;
}


//-----------------------------------------------------------------------------
void KMSender::smtpClose(DwSmtpClient& client)
{
  if (client.Close() != 0)
    warning(nls->translate("Cannot close SMTP connection."));
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

  //unlink(msgFileName);
  return true;
}


//-----------------------------------------------------------------------------
void KMSender::setMethod(Method aMethod)
{
  mMethod = aMethod;
  mCfg->setGroup(SENDER_GROUP);
  mCfg->writeEntry("Method", (int)mMethod);
  mCfg->sync();
}


//-----------------------------------------------------------------------------
void KMSender::setSendImmediate(bool aSendImmediate)
{
  mSendImmediate = aSendImmediate;
  mCfg->setGroup(SENDER_GROUP);
  mCfg->writeEntry("Immediate", (int)mSendImmediate);
}


//-----------------------------------------------------------------------------
void KMSender::setMailer(const QString& aMailer)
{
  mMailer = aMailer;
  mCfg->setGroup(SENDER_GROUP);
  mCfg->writeEntry("Mailer", mMailer);
}


//-----------------------------------------------------------------------------
void KMSender::setSmtpHost(const QString& aSmtpHost)
{
  mSmtpHost = aSmtpHost;
  mCfg->setGroup(SENDER_GROUP);
  mCfg->writeEntry("Smtp Host", mSmtpHost);
  mCfg->sync();
}


//-----------------------------------------------------------------------------
void KMSender::setSmtpPort(int aSmtpPort)
{
  mSmtpPort = aSmtpPort;
  mCfg->setGroup(SENDER_GROUP);
  mCfg->writeEntry("Smtp Port", mSmtpPort);
  mCfg->sync();
}


//-----------------------------------------------------------------------------
void KMSender::setUserName(const QString& aUserName)
{
  mUserName = aUserName;
  mCfg->setGroup(SENDER_GROUP);
  mCfg->writeEntry("User Name", mSmtpHost);
}


//-----------------------------------------------------------------------------
void KMSender::setEmailAddr(const QString& aEmailAddr)
{
  mEmailAddr = aEmailAddr;
  mCfg->setGroup(SENDER_GROUP);
  mCfg->writeEntry("Email Address", mEmailAddr);
}
