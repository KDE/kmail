// kmsender.cpp

#include "kmsender.h"
#include "kmfoldermgr.h"
#include "kmmessage.h"
#include "kmglobal.h"
#include "kmfolder.h"
#include "kmidentity.h"

#include <kconfig.h>
#include <kapp.h>
#include <kprocess.h>
#include <klocale.h>
#include <qregexp.h>

#include <assert.h>
#include <stdio.h>

#define SENDER_GROUP "sending mail"


//-----------------------------------------------------------------------------
KMSender::KMSender()
{
  mMailerProc = NULL;
  readConfig();
}


//-----------------------------------------------------------------------------
KMSender::~KMSender()
{
  if (mMailerProc) delete mMailerProc;
  writeConfig(TRUE);
}


//-----------------------------------------------------------------------------
void KMSender::readConfig(void)
{
  QString str;
  KConfig* config = app->getConfig();

  config->setGroup(SENDER_GROUP);

  mSendImmediate = (bool)config->readNumEntry("Immediate", TRUE);
  mMailer = config->readEntry("Mailer", "/usr/sbin/sendmail");
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
  config->writeEntry("Mailer", mMailer);
  config->writeEntry("Smtp Host", mSmtpHost);
  config->writeEntry("Smtp Port", mSmtpPort);
  config->writeEntry("Method", (mMethod==smSMTP) ? "smtp" : "mail");

  if (aWithSync) config->sync();
}


//-----------------------------------------------------------------------------
bool KMSender::sendQueued(void)
{
  KMMessage* msg;
  bool rc = TRUE;

  outboxFolder->open();
  while(outboxFolder->count() > 0)
  {
    msg = outboxFolder->getMsg(0);
    rc = send(msg, TRUE);
    if (!rc) break;
  }
  outboxFolder->close();
  return rc;
}


//-----------------------------------------------------------------------------
bool KMSender::send(KMMessage* aMsg, short sendNow)
{
  bool sendOk = FALSE;
  int rc;

  assert(aMsg != NULL);
  if (!identity->mailingAllowed())
  {
    warning(nls->translate("Please set the required fields in the\n"
			   "identity settings:\n"
			   "user-name and email-address"));
    return FALSE;
  }
  if (!aMsg->to() || aMsg->to()[0]=='\0') return FALSE;

  //aMsg->viewSource("KMSender::send()");

  if (sendNow==-1) sendNow = mSendImmediate;
  if (!sendNow)
  {
    rc = outboxFolder->addMsg(aMsg);
    if (!rc) aMsg->setStatus(KMMsgStatusQueued);
    return (rc==0);
  }

  if (mMethod == smSMTP) sendOk = sendSMTP(aMsg);
  else if (mMethod == smMail) sendOk = sendMail(aMsg);
  else warning(nls->translate("Please specify a send\nmethod in the settings\n"
			      "and try again."));
  if (sendOk)
  {
    aMsg->setStatus(KMMsgStatusSent);
    sentFolder->addMsg(aMsg);
  }

  return sendOk;
}


//-----------------------------------------------------------------------------
bool KMSender::sendSMTP(KMMessage *msg)
{
  void (*oldHandler)(int);
  bool result;

  oldHandler = signal(SIGALRM, SIG_IGN);
  result = doSendSMTP(msg);
  signal(SIGALRM, oldHandler);

  return result;
}

//-----------------------------------------------------------------------------
bool KMSender::doSendSMTP(KMMessage* msg)
{
  // $markus: I just could not resist implementing smtp suppport
  // This code just must be stable. I checked every darn return code!
  // Date: 24. Sept. 97
  
  QString str, msgStr;
  int replyCode;
  DwSmtpClient client;

  assert(msg != NULL);

  msgStr = prepareStr(msg->asString(), TRUE);

  client.Open(mSmtpHost,mSmtpPort); // Open connection
  if(!client.IsOpen) // Check if connection succeded
  {
    QString str;
    str.sprintf(nls->translate("Cannot open SMTP connection to\n"
			       "host %s for sending:\n%s"), 
		(const char*)mSmtpHost,(const char*)client.Response().c_str());
    warning((const char*)str);
    return FALSE;
  }
  
  replyCode = client.Helo(); // Send HELO command
  if(replyCode != 250) return smtpFailed(client, "HELO", replyCode);

  replyCode = client.Mail(identity->emailAddr());
  if(replyCode != 250) return smtpFailed(client, "FROM", replyCode);

  replyCode = client.Rcpt(msg->to()); // Send RCPT command
  if(replyCode != 250 && replyCode != 251) 
    return smtpFailed(client, "RCPT", replyCode);

  if(!msg->cc().isEmpty())  // Check if cc is set.
  {
    replyCode = client.Rcpt(msg->cc()); // Send RCPT command
    if(replyCode != 250 && replyCode != 251)
      return smtpFailed(client, "RCPT", replyCode);
  }

  if(!msg->bcc().isEmpty())  // Check if bcc ist set.
  {
    replyCode = client.Rcpt(msg->bcc()); // Send RCPT command
    if(replyCode != 250 && replyCode != 251)
      return smtpFailed(client, "RCPT", replyCode);
  }

  replyCode = client.Data(); // Send DATA command
  if(replyCode != 354) 
    return smtpFailed(client, "DATA", replyCode);

  replyCode = client.SendData((const char*)msgStr);
  if(replyCode != 250 && replyCode != 251)
    return smtpFailed(client, "<body>", replyCode);

  replyCode = client.Quit(); // Send QUIT command
  if(replyCode != 221)
    return smtpFailed(client, "QUIT", replyCode);

  return TRUE;
}


//-----------------------------------------------------------------------------
bool KMSender::smtpFailed(DwSmtpClient& client, const char* inCommand,
			  int replyCode)
{
  QString str;
  const char* errorStr = client.Response().c_str();

  str.sprintf(nls->translate("Failed to send mail message\n"
			     "because a SMTP error occured\n"
			     "during the \"%s\" command.\n\n"
			     "Return code: %d\n"
			     "Response: `%s'"), 
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
  QString msgstr;

  if (mMailer.isEmpty())
  {
    warning(nls->translate("Please specify a mailer program\n"
			   "in the settings."));
    return FALSE;
  }

  msgstr = prepareStr(aMsg->asString());

  if (!mMailerProc) mMailerProc = new KProcess;
  assert(mMailerProc != NULL);

  mMailerProc->clearArguments();
  *mMailerProc << aMsg->to();

  mMailerProc->setExecutable(mMailer);
  mMailerProc->start(KProcess::DontCare, KProcess::Stdin);
  if (!mMailerProc->writeStdin(msgstr.data(), msgstr.length()))
    return FALSE;
  if (!mMailerProc->closeStdin()) return FALSE;
  return TRUE;
}


//-----------------------------------------------------------------------------
const QString KMSender::prepareStr(const QString aStr, bool toCRLF)
{
  QString str = aStr.copy();

  str.replace(QRegExp("\\n\\."), "\n ."); 
  str.replace(QRegExp("\\nFrom "), "\n>From "); 
  if (toCRLF) str.replace(QRegExp("\\n"), "\r\n");

  return str;
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
