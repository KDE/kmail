// kmsender.cpp


#include "kmfoldermgr.h"
#include "kmfiltermgr.h"
#include "kmglobal.h"
#include "kmfolder.h"

#include "kmsender.h"
#include "kmmessage.h"
#include "kmidentity.h"
#include "kmiostatusdlg.h"
#include "kbusyptr.h"
#include "kmaccount.h"
#include "kmtransport.h"

#include <kdebug.h>
#include <kconfig.h>
#include <kio/global.h>
#include <kio/job.h>
#include <kio/scheduler.h>
#include <kio/slave.h>
#include <kprocess.h>
#include <kapp.h>
#include <kmessagebox.h>
#include <kmainwindow.h>
#include <kwin.h>
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

//-----------------------------------------------------------------------------
KMSender::KMSender()
{
  mSendDlg = NULL;
  mSendProc = NULL;
  mSendProcStarted = FALSE;
  mSendInProgress = FALSE;
  mCurrentMsg = NULL;
  labelDialog = 0;
  mTransportInfo = new KMTransportInfo();
  readConfig();
  quitOnDone = false;
  mSendAborted = false;
}


//-----------------------------------------------------------------------------
KMSender::~KMSender()
{
  writeConfig(FALSE);
  if (mSendProc) delete mSendProc;
  delete labelDialog;
  delete mTransportInfo;
}

//-----------------------------------------------------------------------------
void
KMSender::setStatusMsg(const QString &msg)
{
   if (labelDialog)
     label->setText( msg );
   emit statusMsg( msg);
}

//-----------------------------------------------------------------------------
void KMSender::readConfig(void)
{
  QString str;
  KConfig* config = kapp->config();

  KConfigGroupSaver saver(config, SENDER_GROUP);

  mSendImmediate = (bool)config->readNumEntry("Immediate", TRUE);
  mSendQuotedPrintable = (bool)config->readNumEntry("Quoted-Printable", TRUE);
}


//-----------------------------------------------------------------------------
void KMSender::writeConfig(bool aWithSync)
{
  KConfig* config = kapp->config();
  KConfigGroupSaver saver(config, SENDER_GROUP);

  config->writeEntry("Immediate", mSendImmediate);
  config->writeEntry("Quoted-Printable", mSendQuotedPrintable);

  if (aWithSync) config->sync();
}


//-----------------------------------------------------------------------------
bool KMSender::settingsOk(void) const
{
  KMIdentity ident( i18n( "Default" ));
  ident.readConfig();
  if (KMTransportInfo::availableTransports().isEmpty())
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
      return false;
    }
  if (!settingsOk()) return FALSE;

  if (aMsg->to().isEmpty())
  {
    // RFC822 says:
    // Note that the "Bcc" field may be empty, while the "To" field is required to
    // have at least one address.
    kernel->kbp()->idle();
    KMessageBox::information(0,i18n("You must specify at least one receiver in the To: field."));
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
    QString idStr = aMsg->headerField("X-KMail-Identity");
    KMIdentity ident( idStr.isEmpty() ? i18n( "Default" ) : idStr );
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

  // start sending the messages
  doSendMsg();
  return TRUE;
}


//-----------------------------------------------------------------------------
void KMSender::doSendMsg()
{
  bool someSent = mCurrentMsg;

  // Post-process sent message (filtering)
  if (mCurrentMsg  && kernel->filterMgr())
  {
    mCurrentMsg->setTransferInProgress( FALSE );
    mCurrentMsg->setStatus(KMMsgStatusSent);

    // Filters assume that the message has no parent
    KMFolder *parent = mCurrentMsg->parent();
    if ( parent )
      parent->removeMsg( mCurrentMsg );
    mCurrentMsg->setParent(0);

    // 0==processed ok, 1==no filter matched, 2==critical error, abort!
    int processResult = kernel->filterMgr()->process(mCurrentMsg,KMFilterMgr::Outbound);
    switch (processResult) {
    case 2:
      perror("Critical error: Unable to process sent mail (out of space?)");
      KMessageBox::information(0, i18n("Critical error: "
				       "Unable to process sent mail (out of space?)"
				       "Moving failing message to \"sent-mail\" folder."));
      kernel->sentFolder()->quiet(TRUE);
      kernel->sentFolder()->moveMsg(mCurrentMsg);
      cleanup();
      kernel->sentFolder()->quiet(FALSE);
      return;
    case 1:
      kernel->sentFolder()->quiet(TRUE);
      kernel->sentFolder()->moveMsg(mCurrentMsg);
      kernel->sentFolder()->quiet(FALSE);
    default:
      break;
    }
    if (!mCurrentMsg->parent())
      parent->addMsg( mCurrentMsg );
    if (mCurrentMsg->parent()) // unGet this message
      mCurrentMsg->parent()->unGetMsg( mCurrentMsg->parent()->count() -1 );

    mCurrentMsg = 0;
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
  mCurrentMsg->setTransferInProgress( TRUE );

  // start the sender process or initialize communication
  if (!mSendProcStarted)
  {
    kernel->serverReady (false); //sven - stop IPC

    if (!labelDialog) {
      labelDialog = new KMainWindow(0, "sendinglabel", WStyle_Dialog | WDestructiveClose );
      label = new QLabel(labelDialog);
      connect(labelDialog,SIGNAL(destroyed()),this,SLOT(slotAbortSend()));
      label->resize(400, label->sizeHint().height());
      labelDialog->resize(400, labelDialog->sizeHint().height());
      label->setText(i18n("Initiating sender process..."));
      labelDialog->show();
    }

    // Run the precommand if there is one
    setStatusMsg(i18n("Executing precommand %1")
    .arg(mTransportInfo->precommand));
    if (!KMAccount::runPrecommand(mTransportInfo->precommand))
      {
	KMessageBox::error(0, i18n("Couldn't execute precommand:\n%1")
          .arg(mTransportInfo->precommand));
      }

    setStatusMsg(i18n("Initiating sender process..."));
  }

  QString msgTransport = mCurrentMsg->headerField("X-KMail-Transport");
  if (msgTransport.isEmpty())
  {
    QStringList sl = KMTransportInfo::availableTransports();
    if (!sl.isEmpty()) msgTransport = sl[0];
  }
  if (!mSendProc || msgTransport != mMethodStr) {
    if (mSendProcStarted && mSendProc) {
      mSendProc->finish(true);
      mSendProcStarted = FALSE;
    }

    mSendProc = createSendProcFromString(msgTransport);
    mMethodStr = msgTransport;

    if (!mSendProc)
      sendProcStarted(false);
    else {
      connect(mSendProc, SIGNAL(idle()), SLOT(slotIdle()));
      connect(mSendProc, SIGNAL(started(bool)), SLOT(sendProcStarted(bool)));
      mSendProc->start();
    }
  }
  else if (!mSendProcStarted)
    mSendProc->start();
  else
    doSendMsgAux();
}


//-----------------------------------------------------------------------------
void KMSender::sendProcStarted(bool success)
{
  if (!success) {
    if (mSendProc)
       mSendProc->finish(true);
    else
      setStatusMsg(i18n("Unrecognised transport protocol, could not send message."));
    mSendProc = 0;
    mSendProcStarted = false;
    cleanup();
    return;
  }
  doSendMsgAux();
}


//-----------------------------------------------------------------------------
void KMSender::doSendMsgAux()
{
  mSendProcStarted = TRUE;
  mSendInProgress = TRUE;

  // remove header fields that shall not be included in sending
  mCurrentMsg->removeHeaderField("Status");
  mCurrentMsg->removeHeaderField("X-Status");
  mCurrentMsg->removeHeaderField("X-KMail-Transport");

  // start sending the current message

  mSendProc->preSendInit();
  setStatusMsg(i18n("Sending message: %1").arg(mCurrentMsg->subject()));
  if (!mSendProc->send(mCurrentMsg))
  {
    cleanup();
    setStatusMsg(i18n("Failed to send (some) queued messages."));
    return;
  }
  // Do *not* add code here, after send(). It can happen that this method
  // is called recursively if send() emits the idle signal directly.
}


//-----------------------------------------------------------------------------
void KMSender::cleanup(void)
{
  if (mSendProc && mSendProcStarted) mSendProc->finish(true);
  mSendProc = NULL;
  mSendProcStarted = FALSE;
  mSendInProgress = FALSE;
  if (mCurrentMsg)
  {
    mCurrentMsg->setTransferInProgress( FALSE );
    mCurrentMsg = NULL;
  }
  kernel->sentFolder()->close();
  kernel->outboxFolder()->close();
  if (kernel->outboxFolder()->count()<0)
    kernel->outboxFolder()->expunge();
  else kernel->outboxFolder()->compact();

  kernel->serverReady (true); // sven - enable ipc
  mSendAborted = false;
  if (labelDialog) {
      disconnect(labelDialog,SIGNAL(destroyed()),this,SLOT(slotAbortSend()));
      labelDialog->close();
      labelDialog = 0;
  }
  if (quitOnDone)
  {
    kapp->quit();
  }
}


//-----------------------------------------------------------------------------
void KMSender::quitWhenFinished()
{
  quitOnDone=true;
}

//-----------------------------------------------------------------------------
void KMSender::slotAbortSend()
{
    labelDialog = 0;
    mSendAborted = true;
    if (mSendProc)
	mSendProc->abort();
}

//-----------------------------------------------------------------------------
void KMSender::slotIdle()
{
  assert(mSendProc != NULL);

  if (!mSendAborted) {
      if (mSendProc->sendOk()) {
	  // sending succeeded
	  doSendMsg();
	  return;
      }
  }

  // sending of message failed
  QString msg;
  QString errString;
  if (mSendProc)
      errString = mSendProc->message();

  msg = i18n("Sending failed:\n%1\n"
        "The message will stay in the 'outbox' folder until you either\n"
        "fix the problem (e.g. a broken address) or remove the message\n"
	"from the 'outbox' folder.\n\n"
	"Note: other messages will also be blocked by this message, as\n"
	"long as it is in the 'outbox' folder\n\n"
	"The following transport protocol was used:\n  %2")
    .arg(errString)
    .arg(mMethodStr);
  if (!errString.isEmpty()) KMessageBox::error(0,msg);

  mSendProc->finish(true);
  mSendProc = 0;
  mSendProcStarted = false;

  cleanup();
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
KMSendProc* KMSender::createSendProcFromString(QString transport)
{
  mTransportInfo->type = QString::null;
  int nr = KMTransportInfo::findTransport(transport);
  if (nr)
  {
    mTransportInfo->readConfig(nr);
  } else {
    if (transport.left(7) == "smtp://")
    {
      mTransportInfo->type = "smtp";
      mTransportInfo->auth = FALSE;
      QString serverport = transport.mid(7);
      mTransportInfo->host = serverport;
      mTransportInfo->port = "25";
      int colon = serverport.find(":");
      if (colon != -1) {
        mTransportInfo->host = serverport.left(colon);
        mTransportInfo->port = serverport.mid(colon + 1);
      }
    }
    else if (transport.left(7) == "file://")
    {
      mTransportInfo->type = "sendmail";
      mTransportInfo->host = transport.mid(7);
    }
  }

  if (mTransportInfo->type == "sendmail")
    return new KMSendSendmail(this);
  if (mTransportInfo->type == "smtp")
    return new KMSendSMTP(this);

  return NULL;
}


//=============================================================================
//=============================================================================
KMSendProc::KMSendProc(KMSender* aSender): QObject()
{
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
void KMSendProc::failed(const QString &aMsg)
{
  mSending = FALSE;
  mSendOk = FALSE;
  mMsg = aMsg;
}

//-----------------------------------------------------------------------------
void KMSendProc::start(void)
{
  emit started(true);
}

//-----------------------------------------------------------------------------
bool KMSendProc::finish(bool destructive)
{
  if (destructive) delete this;
  return TRUE;
}

//-----------------------------------------------------------------------------
QCString KMSendProc::prepareStr(const QCString &aStr, bool toCRLF,
 bool noSingleDot)
{
  int tlen;
  const int len = aStr.length();

  if (aStr.isEmpty()) return QCString();

  QCString target( "" );

  if ( toCRLF ) {
    // (mmutz) headroom so we actually don't need to resize the target
    // array. Five percent should suffice. I measured a mean line
    // length of 42 (no joke) over the my last month's worth of mails.
    tlen = int(len * 1.05);
    target.resize( tlen );

    QCString::Iterator t = target.begin();
    QCString::Iterator te = target.end();
    te -= 5; // 4 is the max. #(chars) appended in one round, plus one for the \0.
    QCString::ConstIterator s = aStr.begin();
    while( (*s) ) {

      char c = *s++;

      if ( c == '\n' ) {
	*t++ = '\r';
	*t++ = c;
	
	if ( noSingleDot && (*s) == '.' ) {
	  s++;
	  *t++ = '.';
	  *t++ = '.';
	}
      } else
	*t++ = c;

      if ( t >= te ) { // nearing the end of the target buffer.
	int tskip = t - target.begin();
	tlen += QMAX( len/128, 128 );
	if ( !target.resize( tlen ) )
	  // OOM, what else can we do?
	  return aStr;
	t = target.begin() + tskip;
      }
    }
    *t = '\0';
  } else {
    if ( !noSingleDot ) return aStr;

    tlen = 0;

    QCString::Iterator t = target.begin();
    QCString::ConstIterator olds = aStr.begin();
    QCString::ConstIterator s = aStr.begin();

    while ( (*s) ) {
      if ( *s++ == '\n' && *s == '.' ) {

	int skip = s - olds + 1;

	if ( tlen ) {
	  if ( tlen + skip >= (int)target.size() ) {
	    // resize to 128 + <currently used> + <yet to be copied>
	    target.resize( 128 + tlen + len - ( olds - aStr.begin() ) );
	    t = target.begin() + tlen;
	  }
	} else {
	  target.resize( int( len * 1.02 ) );
	  t = target.begin();
	}

	memcpy( t, olds, skip );
	tlen += skip; // incl. '.'
	t += skip;
	olds = s; // *olds == '.', thus we double the dot in the next round
      }
    }
    // *s == \0 here.

    if ( !tlen ) return aStr; // didn't change anything

    // copy last chunk.
    if ( tlen + s - olds + 1 /* incl. \0 */ >= (int)target.size() ) {
      target.resize( tlen + s - olds + 1 );
      t = target.begin() + tlen;
    }
    memcpy( t, olds, s - olds + 1 );
  }

  return target;
}

//-----------------------------------------------------------------------------
void KMSendProc::statusMsg(const QString& aMsg)
{
  if (mSender) mSender->setStatusMsg(aMsg);
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
  mMailerProc = NULL;
}

//-----------------------------------------------------------------------------
KMSendSendmail::~KMSendSendmail()
{
  if (mMailerProc) delete mMailerProc;
}

//-----------------------------------------------------------------------------
void KMSendSendmail::start(void)
{
  if (mSender->transportInfo()->host.isEmpty())
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
    emit started(false);
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
  emit started(true);
}

//-----------------------------------------------------------------------------
bool KMSendSendmail::finish(bool destructive)
{
  if (mMailerProc) delete mMailerProc;
  mMailerProc = NULL;
  if (destructive)
      delete this;
  return TRUE;
}

//-----------------------------------------------------------------------------
void KMSendSendmail::abort()
{
  if (mMailerProc) delete mMailerProc;
  mMailerProc = NULL;
  mSendOk = false;
  mMsgStr = 0;
  idle();
}


//-----------------------------------------------------------------------------
bool KMSendSendmail::send(KMMessage* aMsg)
{
  QString bccStr;

  mMailerProc->clearArguments();
  *mMailerProc << mSender->transportInfo()->host;
  *mMailerProc << "-i";
  aMsg->removeHeaderField("X-KMail-Identity");
  addRecipients(aMsg->headerAddrField("To"));
  if (!aMsg->cc().isEmpty()) addRecipients(aMsg->headerAddrField("Cc"));

  bccStr = aMsg->bcc();
  if (!bccStr.isEmpty())
  {
    addRecipients(aMsg->headerAddrField("Bcc"));
    aMsg->removeHeaderField("Bcc");
  }

  mMsgStr = aMsg->asString();
  if (!bccStr.isEmpty()) aMsg->setBcc(bccStr);

  if (!mMailerProc->start(KProcess::NotifyOnExit,KProcess::All))
  {
    KMessageBox::information(0,i18n("Failed to execute mailer program") +
				    mSender->transportInfo()->host);
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
  if (!mSendOk) failed(i18n("Sendmail exited abnormally."));
  mMsgStr = 0;
  emit idle();
}


//-----------------------------------------------------------------------------
bool KMSendSendmail::addOneRecipient(const QString& aRcpt)
{
  assert(mMailerProc!=NULL);
  if (!aRcpt.isEmpty()) *mMailerProc << aRcpt;
  return TRUE;
}



//-----------------------------------------------------------------------------
//=============================================================================
//=============================================================================
KMSendSMTP::KMSendSMTP(KMSender *sender)
  : KMSendProc(sender), 
    mInProcess(false),
    mJob(0),
    mSlave(0)
{
  KIO::Scheduler::connect(SIGNAL(slaveError(KIO::Slave *, int,
    const QString &)), this, SLOT(slaveError(KIO::Slave *, int,
    const QString &)));
}

KMSendSMTP::~KMSendSMTP()
{
  if (mJob) mJob->kill();
}

bool KMSendSMTP::send(KMMessage *aMsg)
{
  KMTransportInfo *ti = mSender->transportInfo();
  assert(aMsg != NULL);

  // email this is from
  mQuery = QString("headers=0&from=%1").arg(KMMessage::getEmailAddr(aMsg->from()));

  // recipients
  mQueryField = "&to=";
  if(!addRecipients(aMsg->headerAddrField("To"))) 
  {
    return FALSE;
  }

  if(!aMsg->cc().isEmpty())
  {
    mQueryField = "&cc=";
    if(!addRecipients(aMsg->headerAddrField("Cc"))) return FALSE;
  }

  if(!aMsg->bcc().isEmpty())
  {
    mQueryField = "&bcc=";
    if (!addRecipients(aMsg->headerAddrField("Bcc"))) return FALSE;
    aMsg->removeHeaderField("Bcc");
  }

  if(!aMsg->subject().isEmpty())
    mQuery += QString("&subject=") + aMsg->subject();
  
  KURL destination;
 
  destination.setProtocol((ti->encryption == "SSL") ? "smtps" : "smtp");
  destination.setHost(ti->host);
  destination.setPort(ti->port.toUShort());
  
  if (ti->auth)
  {
kdDebug() << "user = " << ti->user << endl;
    destination.setUser(ti->user);
    destination.setPass(ti->pass);
  }

  if (!mSlave || !mInProcess)
  {
    KIO::MetaData slaveConfig;
    slaveConfig.insert("tls", (ti->encryption == "TLS") ? "on" : "off");
    if (ti->auth) slaveConfig.insert("sasl", ti->authType);
    mSlave = KIO::Scheduler::getConnectedSlave(destination.url(), slaveConfig);
  }

  if (!mSlave)
  {
    abort();
    return false;
  }
  
  destination.setPath("/send");
  destination.setQuery(mQuery);

  mQuery = "";

  mMessage = prepareStr(aMsg->asString(), TRUE);
  
  if ((mJob = KIO::put(destination, -1, false, false, false)))
  {
      KIO::Scheduler::assignJobToSlave(mSlave, mJob);
      connect(mJob, SIGNAL(result(KIO::Job *)), this, SLOT(result(KIO::Job *)));
      connect(mJob, SIGNAL(dataReq(KIO::Job *, QByteArray &)), 
              this, SLOT(dataReq(KIO::Job *, QByteArray &)));
      mSendOk = true;
      mInProcess = true;
      return mSendOk;
  }
  else 
  {
    abort();
    return false;
  }
}

void KMSendSMTP::abort()
{
  finish(false);
  emit idle();
}

bool KMSendSMTP::finish(bool b)
{
  if(mJob)
  {
    mJob->kill(TRUE);
    mJob = 0;
    mSlave = 0;
  }
  
  if (mSlave)
  {
    KIO::Scheduler::disconnectSlave(mSlave);
    mSlave = 0;
  }
  
  mInProcess = false;
  return KMSendProc::finish(b);
}

bool KMSendSMTP::addOneRecipient(const QString& _addr)
{
  if(!_addr.isEmpty())
    mQuery += mQueryField + _addr;

  return true;
}

void KMSendSMTP::dataReq(KIO::Job *, QByteArray &array)
{
  if(mMessage.length())
  {
    array.duplicate(mMessage);
    mMessage = "";
  }
}

void KMSendSMTP::result(KIO::Job *_job)
{
  if (!mJob) return;
  mJob = 0;

  if(_job->error())
  {
    mSendOk = false;
    if (_job->error() == KIO::ERR_SLAVE_DIED) mSlave = NULL;
    failed(_job->errorString());
    abort();
  } else {
    emit idle();
  }
}

void KMSendSMTP::slaveError(KIO::Slave *aSlave, int error, const QString &errorMsg)
{
  if (aSlave == mSlave)
  {
    if (error == KIO::ERR_SLAVE_DIED) mSlave = NULL;
    mSendOk = false;
    mJob = NULL;
    failed(KIO::buildErrorString(error, errorMsg));
    abort();
  }
}

#include "kmsender.moc"
