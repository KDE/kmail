// kmsender.cpp

#include "kmsender.h"
#include "kmfolder.h"
#include "kmfoldermgr.h"
#include "kmmessage.h"
#include <Kconfig.h>
#include <kapp.h>
#include <kprocess.h>
#include <assert.h>
#include <stdio.h>

#define SENDER_GROUP "sending mail"

//-----------------------------------------------------------------------------
KMSender::KMSender(KMFolderMgr* aFolderMgr)
{
  mFolderMgr = aFolderMgr;
  mCfg = KApplication::getKApplication()->getConfig();

  mMailerProc = NULL;

  mCfg->setGroup(SENDER_GROUP);
  mMethod = (enum KMSender::Method)mCfg->readNumEntry("method", 2);
  mSendImmediate = (bool)mCfg->readNumEntry("immediate", TRUE);
  mMailer = mCfg->readEntry("mailer", &QString("/usr/sbin/sendmail"));
  mSmtpHost = mCfg->readEntry("smtphost", &QString("localhost"));
  mSmtpPort = mCfg->readNumEntry("smtpport", 110);

  mQueue = mFolderMgr->find("outbox");
  if (!mQueue) 
  {
    warning("The folder `outbox' does not exist in the\n"
	    "mail folder directory. The mail sender depends\n"
	    "on this folder and will not work without it.\n"
	    "Therefore the folder will now be created.");

    mQueue = mFolderMgr->createFolder("outbox");
    if (!mQueue) fatal("Cannot create the folder `outbox'.");
  }
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
  else warning("Please specify a send\nmethod in the settings\n"
	       "and try again.");

  return FALSE;
}


//-----------------------------------------------------------------------------
bool KMSender::sendSMTP(KMMessage*)
{
  warning("Sending via SMTP is not\nimplemented at the moment.\n"
	  "Please stay tuned.");

  return FALSE;
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
    warning("Please specify a mailer program\nin the settings.");
    return FALSE;
  }

  sendCmd = mMailer;
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
