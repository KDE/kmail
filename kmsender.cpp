// kmsender.cpp


#ifndef KRN
#include "kmfoldermgr.h"
#include "kmglobal.h"
#include "kmfolder.h"
#endif

#include "kmsender.h"
#include "kmmessage.h"
#include "kmidentity.h"
#include "kmiostatusdlg.h"

#include <kconfig.h>
#include <kapp.h>
#include <kprocess.h>
#include <kapp.h>
#include <qregexp.h>

#ifdef KRN
#include <kapp.h>
#include <kapp.h>
extern KApplication *app;
extern KLocale *nls;
extern KMIdentity *identity;
#endif

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <klocale.h>

#ifdef HAVE_PATHS_H
#include <paths.h>
#endif

#ifndef _PATH_SENDMAIL
#define _PATH_SENDMAIL  "/usr/sbin/sendmail"
#endif

#define SENDER_GROUP "sending mail"

/** uncomment the following line for SMTP debug output */
//#define SMTP_DEBUG_OUTPUT

#define MSG_BLOCK_SIZE 1024

//-----------------------------------------------------------------------------
KMSender::KMSender()
{
  initMetaObject();
  mSendDlg = NULL;
  mSendProc = NULL;
  mSendProcStarted = FALSE;
  mSendInProgress = FALSE;
  mCurrentMsg = NULL;
  readConfig();
  quitOnDone = false;
  label = new QLabel(0);
  //label->setAutoResize(true);
  label->setCaption("KMail");
  label->setIcon(kapp->getMiniIcon());
  connect (this, SIGNAL(statusMsg(const QString&)),
           label, SLOT(setText(const QString&)));
}


//-----------------------------------------------------------------------------
KMSender::~KMSender()
{
  writeConfig(FALSE);
  if (mSendProc) delete mSendProc;
  if (label) delete label;
}


//-----------------------------------------------------------------------------
void KMSender::readConfig(void)
{
  QString str;
  KConfig* config = app->getConfig();

  config->setGroup(SENDER_GROUP);

  mSendImmediate = (bool)config->readNumEntry("Immediate", TRUE);
  mSendQuotedPrintable = (bool)config->readNumEntry("Quoted-Printable", FALSE);
  mMailer = config->readEntry("Mailer",  _PATH_SENDMAIL);
  mSmtpHost = config->readEntry("Smtp Host", "localhost");
  mSmtpPort = config->readNumEntry("Smtp Port", 25);

  str = config->readEntry("Method");
  if (str=="mail") mMethod = smMail;
  else if (str=="smtp") mMethod = smSMTP;
  else mMethod = smUnknown;
}


//-----------------------------------------------------------------------------
void KMSender::writeConfig(bool aWithSync)
{
  KConfig* config = app->getConfig();
  config->setGroup(SENDER_GROUP);

  config->writeEntry("Immediate", mSendImmediate);
  config->writeEntry("Quoted-Printable", mSendQuotedPrintable);
  config->writeEntry("Mailer", mMailer);
  config->writeEntry("Smtp Host", mSmtpHost);
  config->writeEntry("Smtp Port", mSmtpPort);
  config->writeEntry("Method", (mMethod==smSMTP) ? "smtp" : "mail");

  if (aWithSync) config->sync();
}


//-----------------------------------------------------------------------------
bool KMSender::settingsOk(void) const
{
  if (mMethod!=smSMTP && mMethod!=smMail)
  {
    warning(i18n("Please specify a send\nmethod in the settings\n"
			   "and try again."));
    return FALSE;
  }
  if (!identity->mailingAllowed())
  {
    warning(i18n("Please set the required fields in the\n"
			   "identity settings:\n"
			   "user-name and email-address"));
    return FALSE;
  }
  return TRUE;
}


//-----------------------------------------------------------------------------
bool KMSender::send(KMMessage* aMsg, short sendNow)
{
#ifndef KRN
  int rc;

  //assert(aMsg != NULL);
  if(!aMsg)
    {
      debug("KMSender::send() : aMsg == NULL\n");
      return false;
    }
  if (!settingsOk()) return FALSE;

  if (aMsg->to().isEmpty())
  {
    warning(i18n("You must specify a receiver"));
    return FALSE;
  }

  if (sendNow==-1) sendNow = mSendImmediate;

  outboxFolder->open();
  aMsg->setStatus(KMMsgStatusQueued);
  rc = outboxFolder->addMsg(aMsg);
  if (rc)
  {
    warning(i18n("Cannot add message to outbox folder"));
    return FALSE;
  }

  if (sendNow && !mSendInProgress) rc = sendQueued();
  else rc = TRUE;
  outboxFolder->close();

  return rc;
#else
  return true;
#endif
}


bool KMSender::sendSingleMail( KMMessage *aMsg) 
{

  return true;

}


//-----------------------------------------------------------------------------
bool KMSender::sendQueued(void)
{
#ifndef KRN
  if (!settingsOk()) return FALSE;

  if (mSendInProgress)
  {
    warning(i18n("Sending still in progress"));
    return FALSE;
  }


  // open necessary folders
  outboxFolder->open();
  mCurrentMsg = NULL;

  sentFolder->open();


  // create a sender
  if (mSendProc) delete mSendProc;
  if (mMethod == smMail) mSendProc = new KMSendSendmail(this);
  else if (mMethod == smSMTP) mSendProc = new KMSendSMTP(this);
  else mSendProc = NULL;
  assert(mSendProc != NULL);
  connect(mSendProc,SIGNAL(idle()),SLOT(slotIdle()));

  // start sending the messages
  doSendMsg();
  return TRUE;
#else
  return TRUE;
#endif
}


//-----------------------------------------------------------------------------
void KMSender::doSendMsg()
{
#ifndef KRN
  assert(mSendProc != NULL);

  // Move previously sent message to folder "sent"
  if (mCurrentMsg)
    {
      mCurrentMsg->setStatus(KMMsgStatusSent);
      sentFolder->moveMsg(mCurrentMsg);
      mCurrentMsg = NULL;
    }
  

  // See if there is another queued message
  mCurrentMsg = outboxFolder->getMsg(0);
  if (!mCurrentMsg)
  {
    // no more message: cleanup and done
    cleanup();
    return;
  }

  // start the sender process or initialize communication
  if (!mSendProcStarted)
  {
    serverReady(false); //sven - stop IPC
    
    label->setText(i18n("Initiating sender process..."));
    label->resize(400, label->sizeHint().height());
    label->show();
    //kapp->processEvents();
    emit statusMsg(i18n("Initiating sender process..."));
    if (!mSendProc->start())
    {
      cleanup();
      return;
    }
    mSendProcStarted = TRUE;
    mSendInProgress = TRUE;
  }

  // remove header fields that shall not be included in sending
  mCurrentMsg->removeHeaderField("Status");
  mCurrentMsg->removeHeaderField("X-Status");

  // start sending the current message
  mSendProc->preSendInit();
  emit statusMsg(i18n("Sending message: ")+mCurrentMsg->subject());
  if (!mSendProc->send(mCurrentMsg))
  {
    cleanup();
    return;
  }
  // Do *not* add code here, after send(). It can happen that this method
  // is called recursively if send() emits the idle signal directly.
#endif
}


//-----------------------------------------------------------------------------
void KMSender::cleanup(void)
{
#ifndef KRN
  assert(mSendProc!=NULL);

  if (mSendProcStarted) mSendProc->finish();
  mSendProcStarted = FALSE;
  mSendInProgress = FALSE;
  sentFolder->close();
  outboxFolder->close();
  if (outboxFolder->count()<0) 
    outboxFolder->expunge();

  else outboxFolder->compact();

  emit statusMsg(i18n("Done sending messages."));
  serverReady(true); // sven - enable ipc
  label->hide();
  if (quitOnDone)
  {
    debug ("Done sending messages.");
    kapp->quit();
  }
#endif
}


//-----------------------------------------------------------------------------
void KMSender::quitWhenFinished()
{
  quitOnDone=true;
}

//-----------------------------------------------------------------------------
void KMSender::slotIdle()
{
  assert(mSendProc != NULL);
  //assert(!mSendProc->sending());
  if (mSendProc->sendOk())
  {
    // sending succeeded
    doSendMsg();
  }
  else
  {
    // sending of message failed
    QString msg;
    msg = i18n("Sending failed:");
    msg += '\n';
    msg += mSendProc->message();
    warning(msg);
    cleanup();
  }
}


//-----------------------------------------------------------------------------
void KMSender::setMethod(Method aMethod)
{
  mMethod = aMethod;
}


//-----------------------------------------------------------------------------
void KMSender::setSendImmediate(bool aSendImmediate)
{
  mSendImmediate = aSendImmediate;
}


//-----------------------------------------------------------------------------
void KMSender::setSendQuotedPrintable(bool aSendQuotedPrintable)
{
  mSendQuotedPrintable = aSendQuotedPrintable;
}


//-----------------------------------------------------------------------------
void KMSender::setMailer(const QString& aMailer)
{
  mMailer = aMailer;
}


//-----------------------------------------------------------------------------
void KMSender::setSmtpHost(const QString& aSmtpHost)
{
  mSmtpHost = aSmtpHost;
}


//-----------------------------------------------------------------------------
void KMSender::setSmtpPort(int aSmtpPort)
{
  mSmtpPort = aSmtpPort;
}


//=============================================================================
//=============================================================================
KMSendProc::KMSendProc(KMSender* aSender): QObject()
{
  initMetaObject();
  mSender = aSender;
  preSendInit();
}

//-----------------------------------------------------------------------------
void KMSendProc::preSendInit(void)
{
  mSending = FALSE;
  mSendOk = FALSE;
  mMsg = 0;
}

//-----------------------------------------------------------------------------
void KMSendProc::failed(const QString aMsg)
{
  mSending = FALSE;
  mSendOk = FALSE;
  mMsg = aMsg;
}

//-----------------------------------------------------------------------------
bool KMSendProc::start(void)
{
  return TRUE;
}

//-----------------------------------------------------------------------------
bool KMSendProc::finish(void)
{
  return TRUE;
}

//-----------------------------------------------------------------------------
const QString KMSendProc::prepareStr(const QString aStr, bool toCRLF)
{
  QString str;
  int num;
  int pos=0;

  if (aStr.isEmpty()) return str;

  for (num=0; pos<(int)aStr.length(); pos++)
  {
    if (aStr[pos]=='\n')
    {
      num++;
      if (aStr[pos+1]=='.') num++;
    }
  }

  // Convert LF to CR+LF and handle dots at beginning of line.
  for (pos=0; pos<(int)aStr.length(); pos++)
  {
    QChar c = aStr[pos];
    if (c=='\n')
    {
      if (toCRLF) str += '\r';
      str += c;
      if (aStr[pos+1]=='.' && aStr[pos+2]<=' ')
      {
	pos++;
	str += '.';
	str += '.';
      }
    }
    else str += c;
  }

  return str;
}

//-----------------------------------------------------------------------------
void KMSendProc::statusMsg(const QString& aMsg)
{
  if (mSender) emit mSender->statusMsg(aMsg);
  app->processEvents(500);
}

//-----------------------------------------------------------------------------
bool KMSendProc::addRecipients(const QStrList& aRecipientList)
{
  QStrList* recpList = (QStrList*)&aRecipientList;
  QString receiver;
  int i, j;
  bool rc;

  for (receiver=recpList->first(); !receiver.isNull(); receiver=recpList->next())
  {
    i = receiver.find('<');
    if (i >= 0)
    {
      j = receiver.find('>', i+1);
      if (j > i) receiver = receiver.mid(i+1, j-i-1);
    }
    else // if it's "radej@kde.org (Sven Radej)"
    {
      i=receiver.find('(');
      if (i > 0)
        receiver.truncate(i);  // "radej@kde.org "
    }
    //printf ("Receiver = %s\n", receiver.data());
    
    if (!receiver.isEmpty()) 
    {
      rc = addOneRecipient(receiver);
      if (!rc) return FALSE;
    }
  }
  return TRUE;
}


//=============================================================================
//=============================================================================
KMSendSendmail::KMSendSendmail(KMSender* aSender):
  KMSendSendmailInherited(aSender)
{
  initMetaObject();
  mMailerProc = NULL;
}

//-----------------------------------------------------------------------------
KMSendSendmail::~KMSendSendmail()
{
  if (mMailerProc) delete mMailerProc;
}

//-----------------------------------------------------------------------------
bool KMSendSendmail::start(void)
{
  if (mSender->mailer().isEmpty())
  {
    warning(i18n("Please specify a mailer program\n"
			   "in the settings."));
    return FALSE;
  }

  if (!mMailerProc) 
  {
    mMailerProc = new KProcess;
    assert(mMailerProc != NULL);
    connect(mMailerProc,SIGNAL(processExited(KProcess*)),
	    this, SLOT(sendmailExited(KProcess*)));
    connect(mMailerProc,SIGNAL(wroteStdin(KProcess*)),
	    this, SLOT(wroteStdin(KProcess*)));
    connect(mMailerProc,SIGNAL(receivedStderr(KProcess*,char*,int)),
	    this, SLOT(receivedStderr(KProcess*, char*, int)));
  }
  return TRUE;
}

//-----------------------------------------------------------------------------
bool KMSendSendmail::finish(void)
{
  if (mMailerProc) delete mMailerProc;
  mMailerProc = NULL;
  return TRUE;
}

//-----------------------------------------------------------------------------
bool KMSendSendmail::send(KMMessage* aMsg)
{
  mMsgStr = prepareStr(aMsg->asString());

  mMailerProc->clearArguments();
  *mMailerProc << mSender->mailer();
  addRecipients(aMsg->headerAddrField("To"));
  if (!aMsg->cc().isEmpty()) addRecipients(aMsg->headerAddrField("Cc"));
  if (!aMsg->bcc().isEmpty()) addRecipients(aMsg->headerAddrField("Bcc"));

  if (!mMailerProc->start(KProcess::NotifyOnExit,KProcess::All))
  {
    warning(i18n("Failed to execute mailer program %s"),
	    (const char*)mSender->mailer());
    return FALSE;
  }
  mMsgPos  = mMsgStr.data();
  mMsgRest = mMsgStr.length();
  wroteStdin(mMailerProc);

  return TRUE;
}


//-----------------------------------------------------------------------------
void KMSendSendmail::wroteStdin(KProcess *proc)
{
  char* str;
  int len;

  assert(proc!=NULL);

  str = mMsgPos;
  len = (mMsgRest>1024 ? 1024 : mMsgRest);

  if (len <= 0)
  {
    mMailerProc->closeStdin();
  }
  else
  {
    mMsgRest -= len;
    mMsgPos  += len;
    mMailerProc->writeStdin(str,len);
    // if code is added after writeStdin() KProcess probably initiates
    // a race condition.
  }
}


//-----------------------------------------------------------------------------
void KMSendSendmail::receivedStderr(KProcess *proc, char *buffer, int buflen)
{
  assert(proc!=NULL);
  mMsg.replace(mMsg.length(), buflen, buffer);
}


//-----------------------------------------------------------------------------
void KMSendSendmail::sendmailExited(KProcess *proc)
{
  assert(proc!=NULL);
  mSendOk = (proc->normalExit() && proc->exitStatus()==0);
  mMsgStr = 0;
  emit idle();
}


//-----------------------------------------------------------------------------
bool KMSendSendmail::addOneRecipient(const QString aRcpt)
{
  assert(mMailerProc!=NULL);
  if (!aRcpt.isEmpty()) *mMailerProc << aRcpt;
  return TRUE;
}


//=============================================================================
//=============================================================================
KMSendSMTP::KMSendSMTP(KMSender* aSender):
  KMSendSMTPInherited(aSender)
{
  mClient = NULL;
  mOldHandler = 0;
}

//-----------------------------------------------------------------------------
KMSendSMTP::~KMSendSMTP()
{
}

//-----------------------------------------------------------------------------
bool KMSendSMTP::start(void)
{
  int replyCode;

  mOldHandler = signal(SIGALRM, SIG_IGN);
  mClient = new DwSmtpClient;
  assert(mClient != NULL);

  smtpInCmd(i18n("connecting to server"));
  mClient->Open(mSender->smtpHost(), mSender->smtpPort()); // Open connection
  if(!mClient->IsOpen()) // Check if connection succeded
  {
    QString str;
    str.sprintf(i18n("Cannot open SMTP connection to\n"
			       "host %s for sending:\n%s"), 
		(const char*)mSender->smtpHost(),
		(const char*)mClient->Response().c_str());
    warning((const char*)str);
    return FALSE;
  }
  app->processEvents(1000);

  smtpInCmd("HELO");
  replyCode = mClient->Helo(); // Send HELO command
  smtpDebug("HELO");
  if (replyCode != 250) return smtpFailed("HELO", replyCode);

  return TRUE;
}

//-----------------------------------------------------------------------------
bool KMSendSMTP::finish(void)
{
  if (mClient->ReplyCode() != 0)
  {
    int replyCode = mClient->Quit(); // Send QUIT command
    smtpDebug("QUIT");
    if(replyCode != 221)
      return smtpFailed("QUIT", replyCode);
  }

  if (mClient->Close() != 0)
    warning(i18n("Cannot close SMTP connection."));

  signal(SIGALRM, mOldHandler);
  delete mClient;
  mClient = NULL;

  return TRUE;
}


//-----------------------------------------------------------------------------
bool KMSendSMTP::send(KMMessage *msg)
{
  mSendOk = smtpSend(msg);
  emit idle();
  return mSendOk;
}


//-----------------------------------------------------------------------------
bool KMSendSMTP::smtpSend(KMMessage* aMsg)
{
  QString str, msgStr;
  int replyCode;

  assert(aMsg != NULL);

  msgStr = prepareStr(aMsg->asString(), TRUE);

  smtpInCmd("MAIL");
  replyCode = mClient->Mail(identity->emailAddr());
  smtpDebug("MAIL");
  if(replyCode != 250) return smtpFailed("MAIL", replyCode);

  if (!addRecipients(aMsg->headerAddrField("To"))) return FALSE;

  if (!aMsg->cc().isEmpty())
    if (!addRecipients(aMsg->headerAddrField("Cc"))) return FALSE;

  if (!aMsg->bcc().isEmpty())
    if (!addRecipients(aMsg->headerAddrField("Bcc"))) return FALSE;

  app->processEvents(500);

  smtpInCmd("DATA");
  replyCode = mClient->Data(); // Send DATA command
  smtpDebug("DATA");
  if(replyCode != 354) 
    return smtpFailed("DATA", replyCode);

  smtpInCmd(i18n("transmitting message"));
  replyCode = mClient->SendData((const char*)msgStr);
  smtpDebug("<body>");
  if(replyCode != 250 && replyCode != 251)
    return smtpFailed("<body>", replyCode);

  return TRUE;
}


//-----------------------------------------------------------------------------
bool KMSendSMTP::addOneRecipient(const QString aRcpt)
{
  int replyCode;
  if (aRcpt.isEmpty()) return TRUE;

  smtpInCmd("RCPT");
  replyCode = mClient->Rcpt(aRcpt);
  smtpDebug("RCPT");

  if(replyCode != 250 && replyCode != 251)
    return smtpFailed("RCPT", replyCode);
  return TRUE;
}


//-----------------------------------------------------------------------------
bool KMSendSMTP::smtpFailed(const char* inCommand,
			  int replyCode)
{
  QString str;
  const char* errorStr = mClient->Response().c_str();

  if (replyCode==0 && (!errorStr || !*errorStr))
    errorStr = i18n("network error");

  str.sprintf(i18n("a SMTP error occured.\n"
			     "Command: %s\n"
			     "Response: %s\n" 
			     "Return code: %d"),
	      inCommand, errorStr ? errorStr : "(nothing)", replyCode);
  mMsg = str;

  return FALSE;
}


//-----------------------------------------------------------------------------
void KMSendSMTP::smtpInCmd(const char* inCommand)
{
  char str[80];
  sprintf(str,i18n("Sending SMTP command: %s"), inCommand);
  statusMsg(str);
}


//-----------------------------------------------------------------------------
void KMSendSMTP::smtpDebug(const char* inCommand)
{
#ifdef SMTP_DEBUG_OUTPUT
  const char* errorStr = mClient->Response().c_str();
  int replyCode = mClient->ReplyCode();
  debug("SMTP '%s': rc=%d, msg='%s'", inCommand, replyCode, errorStr);
#endif
}


//-----------------------------------------------------------------------------
#include "kmsender.moc"
