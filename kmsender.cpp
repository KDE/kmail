// kmsender.cpp


#include "kmfoldermgr.h"
#include "kmglobal.h"
#include "kmfolder.h"

#include "kmsender.h"
#include "kmmessage.h"
#include "kmidentity.h"
#include "kmiostatusdlg.h"
#include "kbusyptr.h"
#include "kmaccount.h"

#include <kdebug.h>
#include <kconfig.h>
#include <kprocess.h>
#include <kapp.h>
#include <kmessagebox.h>
#include <qregexp.h>
#include <qdialog.h>

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
  mSendDlg = NULL;
  mSendProc = NULL;
  mMsgSendProc = NULL;
  mSendProcStarted = FALSE;
  mSendInProgress = FALSE;
  mCurrentMsg = NULL;
  readConfig();
  quitOnDone = false;
  labelDialog = new QDialog(0);
  label = new QLabel(labelDialog);
  labelDialog->setCaption("KMail");
  labelDialog->setIcon(kapp->miniIcon());
}


//-----------------------------------------------------------------------------
KMSender::~KMSender()
{
  writeConfig(FALSE);
  if (mSendProc) delete mSendProc;
  if (labelDialog) delete labelDialog;
}

//-----------------------------------------------------------------------------
void
KMSender::setStatusMsg(const QString &msg)
{
   label->setText( msg );
   emit statusMsg( msg);
}

//-----------------------------------------------------------------------------
void KMSender::readConfig(void)
{
  QString str;
  KConfig* config = kapp->config();

  config->setGroup(SENDER_GROUP);

  mSendImmediate = (bool)config->readNumEntry("Immediate", TRUE);
  mSendQuotedPrintable = (bool)config->readNumEntry("Quoted-Printable", FALSE);
  mMailer = config->readEntry("Mailer",  _PATH_SENDMAIL);
  mSmtpHost = config->readEntry("Smtp Host", "localhost");
  mSmtpPort = config->readNumEntry("Smtp Port", 25);
  mPrecommand = config->readEntry("Precommand", "");

  str = config->readEntry("Method");
  if (str=="mail") mMethod = smMail;
  else if (str=="smtp") mMethod = smSMTP;
  else mMethod = smUnknown;
}


//-----------------------------------------------------------------------------
void KMSender::writeConfig(bool aWithSync)
{
  KConfig* config = kapp->config();
  config->setGroup(SENDER_GROUP);

  config->writeEntry("Immediate", mSendImmediate);
  config->writeEntry("Quoted-Printable", mSendQuotedPrintable);
  config->writeEntry("Mailer", mMailer);
  config->writeEntry("Smtp Host", mSmtpHost);
  config->writeEntry("Smtp Port", (int)mSmtpPort);
  config->writeEntry("Method", (mMethod==smSMTP) ? "smtp" : "mail");
  config->writeEntry("Precommand", mPrecommand);

  if (aWithSync) config->sync();
}


//-----------------------------------------------------------------------------
bool KMSender::settingsOk(void) const
{
  KMIdentity ident( i18n( "Default" ));
  ident.readConfig();
  if (mMethod!=smSMTP && mMethod!=smMail)
  {
    KMessageBox::information(0,i18n("Please specify a send\n"
				    "method in the settings\n"
				    "and try again."));
    return FALSE;
  }
  if (!ident.mailingAllowed())
  {
    KMessageBox::information(0,i18n("Please set the required fields in the\n"
				    "identity settings:\n"
				    "user-name and email-address"));
    return FALSE;
  }
  return TRUE;
}


//-----------------------------------------------------------------------------
bool KMSender::send(KMMessage* aMsg, short sendNow)
{
  int rc;

  //assert(aMsg != NULL);
  if(!aMsg)
    {
      kdDebug() << "KMSender::send() : aMsg == NULL\n" << endl;
      return false;
    }
  if (!settingsOk()) return FALSE;

  if (aMsg->to().isEmpty())
  {
    kernel->kbp()->idle();
    KMessageBox::information(0,i18n("You must specify a receiver"));
    return FALSE;
  }

  if (aMsg->subject().isEmpty())
  {
    kernel->kbp()->idle();
    int rc = KMessageBox::questionYesNo(0, i18n("You did not specify a subject. Send message anyway?"),
    		i18n("No subject specified"), i18n("Yes"), i18n("No, let me specify the subject"));
    if( rc == KMessageBox::No ) {
      return FALSE;
    }
    kernel->kbp()->busy();
  }

  if (sendNow==-1) sendNow = mSendImmediate;

  kernel->outboxFolder()->open();
  aMsg->setStatus(KMMsgStatusQueued);

  // Handle redirections
  QString f = aMsg->headerField("X-KMail-Redirect-From");
  if(f.length() > 0) {
    KMIdentity ident( i18n( "Default" ));
    ident.readConfig();

    aMsg->setFrom(f + QString(" (by way of %1 <%2>)").
		  arg(ident.fullName()).arg(ident.emailAddr()));
  }

  rc = kernel->outboxFolder()->addMsg(aMsg);
  if (rc)
  {
    KMessageBox::information(0,i18n("Cannot add message to outbox folder"));
    return FALSE;
  }

  if (sendNow && !mSendInProgress) rc = sendQueued();
  else rc = TRUE;
  kernel->outboxFolder()->close();

  return rc;
}


bool KMSender::sendSingleMail( KMMessage*)
{

  return true;

}


//-----------------------------------------------------------------------------
bool KMSender::sendQueued(void)
{
  if (!settingsOk()) return FALSE;

  if (mSendInProgress)
  {
    KMessageBox::information(0,i18n("Sending still in progress"));
    return FALSE;
  }


  // open necessary folders
  kernel->outboxFolder()->open();
  mCurrentMsg = NULL;

  kernel->sentFolder()->open();


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
}


//-----------------------------------------------------------------------------
void KMSender::doSendMsg()
{
  assert(mSendProc != NULL);
  bool someSent = mCurrentMsg;

  // Move previously sent message to folder "sent"
  if (mCurrentMsg)
    {
      mCurrentMsg->setStatus(KMMsgStatusSent);
      kernel->sentFolder()->moveMsg(mCurrentMsg);
      mCurrentMsg = NULL;
    }

  // If we have been using a message specific transport, lose it now.
  // Would be more efficient to only do this when the mail transport
  // (server or port) has changed
  if (mMsgSendProc) {
    mMsgSendProc->finish();
    delete mMsgSendProc;
    mMsgSendProc = 0;
    restoreTransportSettings();
    mSendProcStarted = FALSE;
  }

  // See if there is another queued message
  mCurrentMsg = kernel->outboxFolder()->getMsg(0);
  if (!mCurrentMsg)
  {
    // no more message: cleanup and done
    cleanup();
    if (someSent)
      setStatusMsg(i18n("Queued messages successfully sent."));
    return;
  }

  // start the sender process or initialize communication
  if (!mSendProcStarted)
  {
    kernel->serverReady (false); //sven - stop IPC

    label->setText(i18n("Initiating sender process..."));
    label->resize(400, label->sizeHint().height());
    labelDialog->show();
    kapp->processEvents(50); // Not sure this is safe -sanders

    // Run the precommand if there is one
    setStatusMsg(i18n(QString("Executing precommand ") + mPrecommand));
    if (!KMAccount::runPrecommand(mPrecommand))
      {	
	KMessageBox::error(0, QString("Couldn't execute precommand:\n") + mPrecommand);
      }

    setStatusMsg(i18n("Initiating sender process..."));
  }

  mMethodStr = transportString();

  QString msgTransport = mCurrentMsg->headerField("X-KMail-Transport");
  if (!msgTransport.isEmpty()  && (msgTransport != mMethodStr)) {
    if (mSendProcStarted && mSendProc) {
      mSendProc->finish();
      mSendProcStarted = FALSE;
    }

    saveTransportSettings();

    mMsgSendProc = parseTransportFromString(mCurrentMsg->headerField("X-KMail-Transport"));
    mMethodStr = transportString();

    if (!mMsgSendProc || !mMsgSendProc->start()) {
      if (mMsgSendProc) {
	mMsgSendProc->finish();
	delete mMsgSendProc;
      }
      else
	setStatusMsg(i18n("Unrecognised transport protocol, could not send message."));
      mMsgSendProc = 0;
      mSendProcStarted = false;
      restoreTransportSettings();
      cleanup();
      return;
    }
    connect(mMsgSendProc,SIGNAL(idle()),SLOT(slotIdle()));
  }
  else if (!mSendProcStarted)
    if (!mSendProc->start())
    {
      cleanup();
      return;
    }
  mSendProcStarted = TRUE;
  mSendInProgress = TRUE;

  // remove header fields that shall not be included in sending
  mCurrentMsg->removeHeaderField("Status");
  mCurrentMsg->removeHeaderField("X-Status");
  mCurrentMsg->removeHeaderField("X-KMail-Identity");
  mCurrentMsg->removeHeaderField("X-KMail-Transport");

  // start sending the current message

  if (mMsgSendProc) {
    mMsgSendProc->preSendInit();
    setStatusMsg(i18n("Sending message: %1").arg(mCurrentMsg->subject()));
    if (!mMsgSendProc->send(mCurrentMsg))
      {
	cleanup();
	setStatusMsg(i18n("Failed to send (some) queued messages."));
	return;
      }
  } else {
    mSendProc->preSendInit();
    setStatusMsg(i18n("Sending message: %1").arg(mCurrentMsg->subject()));
    if (!mSendProc->send(mCurrentMsg))
      {
	cleanup();
	setStatusMsg(i18n("Failed to send (some) queued messages."));
	return;
      }
  }
  // Do *not* add code here, after send(). It can happen that this method
  // is called recursively if send() emits the idle signal directly.
}


//-----------------------------------------------------------------------------
void KMSender::cleanup(void)
{
  assert(mSendProc!=NULL);

  if (mSendProcStarted) mSendProc->finish();
  mSendProcStarted = FALSE;
  mSendInProgress = FALSE;
  kernel->sentFolder()->close();
  kernel->outboxFolder()->close();
  if (kernel->outboxFolder()->count()<0)
    kernel->outboxFolder()->expunge();

  else kernel->outboxFolder()->compact();

  kernel->serverReady (true); // sven - enable ipc
  labelDialog->hide();
  if (quitOnDone)
  {
    kdDebug() << "Done sending messages." << endl;
    kapp->quit();
  }
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
  if (mMsgSendProc) {
    if (mMsgSendProc->sendOk()) {
      doSendMsg();
      return;
    }
  } else if (mSendProc->sendOk()) {
    // sending succeeded
    doSendMsg();
    return;
  }


  // sending of message failed
  QString msg;
  msg = i18n("Sending failed:\n%1\n"
        "The message will stay in the 'outbox' folder until you either\n"
        "fix the problem (e.g. a broken address) or remove the message\n"
	"from the 'outbox' folder.\n\n"
	"Note: other messages will also be blocked by this message, as\n"
	"long as it is in the 'outbox' folder\n\n"
	"The following transport protocol was used:\n  %2")
    .arg(mSendProc->message())
    .arg(mMethodStr);
  KMessageBox::error(0,msg);

  if (mMsgSendProc) {
    mMsgSendProc->finish();
    delete mMsgSendProc;
    mMsgSendProc = 0;
    mSendProcStarted = false;
    restoreTransportSettings();
  }
  cleanup();
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
void KMSender::setSmtpPort(unsigned short int aSmtpPort)
{
  mSmtpPort = aSmtpPort;
}


//-----------------------------------------------------------------------------
void KMSender::saveTransportSettings(void)
{
  mOldMethod = mMethod;
  mOldMailer = mMailer;
  mOldSmtpHost = mSmtpHost;
  mOldSmtpPort = mSmtpPort;
}


//-----------------------------------------------------------------------------
void KMSender::restoreTransportSettings(void)
{
  mMethod = mOldMethod;
  mMailer = mOldMailer;
  mSmtpHost = mOldSmtpHost;
  mSmtpPort = mOldSmtpPort;
}


//-----------------------------------------------------------------------------
KMSendProc* KMSender::parseTransportFromString(QString transport)
{
  if (transport.left(7) == "smtp://") { // to i18n or not to i18n?
    mMethod = smSMTP;
    QString serverport = transport.mid(7);
    QString server = serverport;
    QString port = "25";
    int colon = serverport.find(":");
    if (colon != -1) {
      server = serverport.left(colon);
      port = serverport.mid(colon + 1);
    }
    mSmtpHost = server;
    mSmtpPort = port.toInt();
    return new KMSendSMTP(this);
  }
  else if (transport.left(7) == "file://") {
    mMethod = smMail;
    mMailer = transport.mid(7);
    return new KMSendSendmail(this);
  }
  else return 0;
}

//-----------------------------------------------------------------------------
QString KMSender::transportString(void) const
{
  if (mMethod == smSMTP)
    return QString("smtp://%1:%2").arg(mSmtpHost).arg(mSmtpPort);
  else if (mMethod == smMail)
    return QString("file://%1").arg(mMailer);
  else
    return "";
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
  mMsg = QString::null;
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
const QString KMSendProc::prepareStr(const QString aStr, bool toCRLF,
  bool noSingleDot)
{
  QString str;
  int pos=0;

  if (aStr.isEmpty()) return str;

  // Convert LF to CR+LF and handle dots at beginning of line.
  for (pos=0; pos<(int)aStr.length(); pos++)
  {
    QChar c = aStr[pos];
    if (c=='\n')
    {
      if (toCRLF) str += '\r';
      str += c;
      if (noSingleDot && aStr[pos+1]=='.' && aStr[pos+2]=='\n')
      {
	pos++;
	str += '.';
        str += ' ';
      }
    }
    else str += c;
  }

  return str;
}

//-----------------------------------------------------------------------------
void KMSendProc::statusMsg(const QString& aMsg)
{
  if (mSender) mSender->setStatusMsg(aMsg);
  kapp->processEvents(50); // Not sure this is safe -sanders
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

    receiver = receiver.stripWhiteSpace();
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
    QString str = i18n("Please specify a mailer program\n"
				    "in the settings.");
    QString msg;
    msg = i18n("Sending failed:\n%1\n"
	"The message will stay in the 'outbox' folder and will be resent.\n"
        "Please remove it from there if you do not want the message to\n"
		"be resent.\n\n"
	"The following transport protocol was used:\n  %2")
    .arg(str + "\n")
    .arg("sendmail://");
    KMessageBox::information(0,msg);
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
  QString bccStr;

  mMailerProc->clearArguments();
  *mMailerProc << mSender->mailer();
  addRecipients(aMsg->headerAddrField("To"));
  if (!aMsg->cc().isEmpty()) addRecipients(aMsg->headerAddrField("Cc"));

  bccStr = aMsg->bcc();
  if (!bccStr.isEmpty())
  {
    addRecipients(aMsg->headerAddrField("Bcc"));
    aMsg->removeHeaderField("Bcc");
  }

  mMsgStr = prepareStr(aMsg->asString());
  if (!bccStr.isEmpty()) aMsg->setBcc(bccStr);

  if (!mMailerProc->start(KProcess::NotifyOnExit,KProcess::All))
  {
    KMessageBox::information(0,i18n("Failed to execute mailer program") +
				    mSender->mailer());
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

  statusMsg(i18n("connecting to server"));
  mClient->Open(mSender->smtpHost(), mSender->smtpPort()); // Open connection
  if(!mClient->IsOpen()) // Check if connection succeded
  {
    QString str;
    QString msg;
    str = i18n("Cannot open SMTP connection to\n"
			       "host %1 for sending:\n%2")
		.arg(mSender->smtpHost())
		.arg((const char*)mClient->Response().c_str());
    msg = i18n("Sending failed:\n%1\n"
	"The message will stay in the 'outbox' folder and will be resent.\n"
        "Please remove it from there if you do not want the message to\n"
		"be resent.\n\n"
	"The following transport protocol was used:\n  %2")
    .arg(str)
    .arg(QString("smtp://%1:%2").arg(mSender->smtpHost()).arg(mSender->smtpPort()));

  KMessageBox::information(0,msg);
    return FALSE;
  }
  kapp->processEvents(50); // not sure this is safe -sanders

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
    KMessageBox::information(0,i18n("Cannot close SMTP connection."));

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
  QString str, msgStr, bccStr;
  int replyCode;
  KMIdentity ident( i18n( "Default" ));
  ident.readConfig();

  assert(aMsg != NULL);

  smtpInCmd("MAIL");
  replyCode = mClient->Mail(ident.emailAddr());
  smtpDebug("MAIL");
  if(replyCode != 250) return smtpFailed("MAIL", replyCode);

  if (!addRecipients(aMsg->headerAddrField("To"))) return FALSE;

  if (!aMsg->cc().isEmpty())
    if (!addRecipients(aMsg->headerAddrField("Cc"))) return FALSE;

  bccStr = aMsg->bcc();
  if (!bccStr.isEmpty())
  {
    if (!addRecipients(aMsg->headerAddrField("Bcc"))) return FALSE;
    aMsg->removeHeaderField("Bcc");
  }

  kapp->processEvents(50); // not sure this is safe -sanders

  smtpInCmd("DATA");
  replyCode = mClient->Data(); // Send DATA command
  smtpDebug("DATA");
  if(replyCode != 354)
    return smtpFailed("DATA", replyCode);

  statusMsg(i18n("transmitting message"));
  msgStr = prepareStr(aMsg->asString(), TRUE, FALSE);
  replyCode = mClient->SendData((const char*)msgStr);
  if (!bccStr.isEmpty()) aMsg->setBcc(bccStr);

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
  QString errorStr = mClient->Response().c_str();

  if (replyCode==0 && (!errorStr || !*errorStr))
    errorStr = i18n("network error");

  str = i18n("a SMTP error occured.\n"
			     "Command: %1\n"
			     "Response: %2\n"
			     "Return code: %3")
		.arg(inCommand)
		.arg(!errorStr.isEmpty() ? errorStr : i18n("(nothing)"))
		.arg(replyCode);
  mMsg = str;

  return FALSE;
}


//-----------------------------------------------------------------------------
void KMSendSMTP::smtpInCmd(const char* inCommand)
{
  QString str;
  str = i18n("Sending SMTP command: %1").arg(inCommand);
  statusMsg(str);
}


//-----------------------------------------------------------------------------
void KMSendSMTP::smtpDebug(const char* /*inCommand*/)
{
#ifdef SMTP_DEBUG_OUTPUT
  const char* errorStr = mClient->Response().c_str();
  int replyCode = mClient->ReplyCode();
  kdDebug() << "SMTP '" << inCommand << "': rc=" << replyCode << ", msg='" << errorStr << "'" << endl;
#endif
}


//-----------------------------------------------------------------------------
#include "kmsender.moc"
