// kmsender.cpp

#include <config.h>

#include <kmime_header_parsing.h>
using namespace KMime::Types;

#include <kio/passdlg.h>
#include <kio/scheduler.h>
#include <kapplication.h>
#include <kmessagebox.h>
#include <kdeversion.h>
#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "kmfiltermgr.h"

#include "kcursorsaver.h"
#include "kmsender.h"
#include "kmidentity.h"
#include "identitymanager.h"
#include "kmbroadcaststatus.h"
#include "kmaccount.h"
#include "kmtransport.h"
#include "kmfolderindex.h"
#include "kmfoldermgr.h"
#include "kmmsgdict.h"
#include "kmmsgpart.h"
#include <mimelib/mediatyp.h>

#define SENDER_GROUP "sending mail"

//-----------------------------------------------------------------------------
KMSender::KMSender()
{
  mPrecommand = 0;
  mSendDlg = 0;
  mSendProc = 0;
  mSendProcStarted = FALSE;
  mSendInProgress = FALSE;
  mCurrentMsg = 0;
  mTransportInfo = new KMTransportInfo();
  readConfig();
  mSendAborted = false;
  mSentMessages = 0;
  mTotalMessages = 0;
  mFailedMessages = 0;
  mSentBytes = 0;
  mTotalBytes = 0;
}


//-----------------------------------------------------------------------------
KMSender::~KMSender()
{
  writeConfig(FALSE);
  delete mSendProc;
  delete mPrecommand;
  delete mTransportInfo;
}

//-----------------------------------------------------------------------------
void KMSender::setStatusMsg(const QString &msg)
{
  KMBroadcastStatus::instance()->setStatusMsg(msg);
}

//-----------------------------------------------------------------------------
void KMSender::readConfig(void)
{
  QString str;
  KConfigGroup config(KMKernel::config(), SENDER_GROUP);

  mSendImmediate = config.readBoolEntry("Immediate", TRUE);
  mSendQuotedPrintable = config.readBoolEntry("Quoted-Printable", TRUE);
}


//-----------------------------------------------------------------------------
void KMSender::writeConfig(bool aWithSync)
{
  KConfigGroup config(KMKernel::config(), SENDER_GROUP);

  config.writeEntry("Immediate", mSendImmediate);
  config.writeEntry("Quoted-Printable", mSendQuotedPrintable);

  if (aWithSync) config.sync();
}


//-----------------------------------------------------------------------------
bool KMSender::settingsOk() const
{
  if (KMTransportInfo::availableTransports().isEmpty())
  {
    KMessageBox::information(0,i18n("Please create an account for sending and try again."));
    return false;
  }
  return true;
}


//-----------------------------------------------------------------------------
bool KMSender::send(KMMessage* aMsg, short sendNow)
{
  int rc;

  //assert(aMsg != 0);
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
    return FALSE;
  }

  QString msgId = aMsg->msgId();
  if( msgId.isEmpty() )
  {
    msgId = KMMessage::generateMessageId( aMsg->fromEmail() );
    //kdDebug(5006) << "Setting Message-Id to '" << msgId << "'\n";
    aMsg->setMsgId( msgId );
  }
  //else
  //  kdDebug(5006) << "Message has already a Message-Id (" << msgId << ")\n";

  if (sendNow==-1) sendNow = mSendImmediate;

  kernel->outboxFolder()->open();
  aMsg->setStatus(KMMsgStatusQueued);

  // Handle redirections
  QString f = aMsg->headerField("X-KMail-Redirect-From");
  if(!f.isEmpty()) {
    uint id = aMsg->headerField("X-KMail-Identity").stripWhiteSpace().toUInt();
    const KMIdentity & ident =
      kernel->identityManager()->identityForUoidOrDefault( id );
    aMsg->setFrom(f + QString(" (by way of %1 <%2>)")
      .arg(ident.fullName()).arg(ident.emailAddr()));
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


//-----------------------------------------------------------------------------
void KMSender::outboxMsgAdded(int idx)
{
    ++mTotalMessages;
    KMMsgBase* msg = kernel->outboxFolder()->getMsgBase(idx);
    Q_ASSERT(msg);
    if ( msg )
        mTotalBytes += msg->msgSize();
}


//-----------------------------------------------------------------------------
bool KMSender::sendQueued(void)
{
  if (!settingsOk()) return FALSE;

  if (mSendInProgress)
  {
    return FALSE;
  }

  // open necessary folders
  KMFolder* outbox = kernel->outboxFolder();
  outbox->open();
  mTotalMessages = outbox->count();
  mTotalBytes = 0;
  for( int i = 0 ; i<mTotalMessages ; ++i )
      mTotalBytes += outbox->getMsgBase(i)->msgSize();

  connect(outbox, SIGNAL(msgAdded(int)),
          this, SLOT(outboxMsgAdded(int)));
  mCurrentMsg = 0;

  kernel->sentFolder()->open();

  // start sending the messages
  doSendMsg();
  return TRUE;
}

//-----------------------------------------------------------------------------
void KMSender::emitProgressInfo( int currentFileProgress )
{
  int percent = (mTotalBytes) ? ( 100 * (mSentBytes+currentFileProgress) / mTotalBytes ) : 0;
  if (percent > 100) percent = 100;
  KMBroadcastStatus::instance()->setStatusProgressPercent("Sender", percent);
}

//-----------------------------------------------------------------------------
void KMSender::doSendMsg()
{
  if (!kernel)  //To handle message sending in progress when kaplan is exited
    return;	//TODO: handle this case better

  KMFolder *sentFolder = 0, *imapSentFolder = 0;
  bool someSent = mCurrentMsg;
  int rc;
  if (someSent) {
      mSentMessages++;
      mSentBytes += mCurrentMsg->msgSize();
  }
  emitProgressInfo( 0 );

  // Post-process sent message (filtering)
  if (mCurrentMsg  && kernel->filterMgr())
  {
    mCurrentMsg->setTransferInProgress( FALSE );
    if( mCurrentMsg->hasUnencryptedMsg() ) {
kdDebug(5006) << "KMSender::doSendMsg() post-processing: replace mCurrentMsg body by unencryptedMsg data" << endl;
      // delete all current body parts
      mCurrentMsg->deleteBodyParts();
      // copy Content-[..] headers from unencrypted message to current one
      KMMessage & newMsg( *mCurrentMsg->unencryptedMsg() );
      mCurrentMsg->dwContentType() = newMsg.dwContentType();
      mCurrentMsg->setContentTransferEncodingStr( newMsg.contentTransferEncodingStr() );
      QCString newDispo = newMsg.headerField("Content-Disposition").latin1();
      if( newDispo.isEmpty() )
        mCurrentMsg->removeHeaderField( "Content-Disposition" );
      else
        mCurrentMsg->setHeaderField( "Content-Disposition", newDispo );
      // copy the body
      mCurrentMsg->setBody( newMsg.body() );
      // copy all the body parts
      KMMessagePart msgPart;
      for( int i = 0; i < newMsg.numBodyParts(); ++i ) {
        newMsg.bodyPart( i, &msgPart );
        mCurrentMsg->addBodyPart( &msgPart );
      }
    }
    mCurrentMsg->setStatus(KMMsgStatusSent);

    const KMIdentity & id = kernel->identityManager()
      ->identityForUoidOrDefault( mCurrentMsg->headerField( "X-KMail-Identity" ).stripWhiteSpace().toUInt() );
    if ( !mCurrentMsg->fcc().isEmpty() )
    {
      sentFolder = kernel->folderMgr()->findIdString( mCurrentMsg->fcc() );
      if ( sentFolder == 0 )
        imapSentFolder = kernel->imapFolderMgr()->findIdString(
          mCurrentMsg->fcc() );
    }
    else if ( !id.fcc().isEmpty() )
    {
      sentFolder = kernel->folderMgr()->findIdString( id.fcc() );
      if ( sentFolder == 0 )
        imapSentFolder = kernel->imapFolderMgr()->findIdString( id.fcc() );
    }
    if (imapSentFolder && imapSentFolder->noContent()) imapSentFolder = 0;

    if ( sentFolder == 0 )
      sentFolder = kernel->sentFolder();
    else {
      rc = sentFolder->open();
      if (rc != 0) {
	cleanup();
        return;
      }
    }

    // 0==processed ok, 1==no filter matched, 2==critical error, abort!
    int processResult = kernel->filterMgr()->process(mCurrentMsg,KMFilterMgr::Outbound);
    switch (processResult) {
    case 2:
      perror("Critical error: Unable to process sent mail (out of space?)");
      KMessageBox::information(0, i18n("Critical error: "
				       "Unable to process sent mail (out of space?)"
				       "Moving failing message to \"sent-mail\" folder."));
      sentFolder->quiet(TRUE);
      sentFolder->moveMsg(mCurrentMsg);
      if ( sentFolder != kernel->sentFolder() )
          sentFolder->close();
      cleanup();
      sentFolder->quiet(FALSE);
      return;
    case 1:
      sentFolder->quiet(TRUE);
      if (sentFolder->moveMsg(mCurrentMsg) != 0)
      {
        KMessageBox::error(0, i18n("Moving the sent message \"%1\" from the "
          "\"outbox\" to the \"sent-mail\" folder failed.\n"
          "Possible reasons are lack of disk space or write permission. "
          "Please try to fix the problem and move the message manually.")
          .arg(mCurrentMsg->subject()));
        cleanup();
        sentFolder->quiet(FALSE);
        return;
      }
      if (imapSentFolder) imapSentFolder->moveMsg(mCurrentMsg);
      sentFolder->quiet(FALSE);
    default:
      break;
    }
    setStatusByLink( mCurrentMsg );
    if (mCurrentMsg->parent() && !imapSentFolder) {
      // for speed optimization, this code assumes that mCurrentMsg is the
      // last one in it's parent folder; make sure that's really the case:
      assert( mCurrentMsg->parent()->find( mCurrentMsg )
	      == mCurrentMsg->parent()->count() - 1 );
       // unGet this message:
      mCurrentMsg->parent()->unGetMsg( mCurrentMsg->parent()->count() -1 );
    }

    mCurrentMsg = 0;
  }

  // See if there is another queued message
  mCurrentMsg = kernel->outboxFolder()->getMsg(mFailedMessages);
  if (!mCurrentMsg || mCurrentMsg->transferInProgress())
  {
    // a message is locked finish the send
    if (mCurrentMsg && mCurrentMsg->transferInProgress())
    	mCurrentMsg = 0;
    // no more message: cleanup and done
    if ( ( sentFolder != kernel->sentFolder() ) && ( sentFolder != 0 ) )
        sentFolder->close();
    if (someSent) {
      if ( mSentMessages == mTotalMessages ) {
        setStatusMsg(i18n("%n queued message successfully sent.",
		       	  "%n queued messages successfully sent.",
			  mSentMessages));
      } else {
        setStatusMsg(i18n("%1 of %2 queued messages successfully sent.")
            .arg(mSentMessages).arg( mTotalMessages ));
      }
    }
    cleanup();
    return;
  }
  mCurrentMsg->setTransferInProgress( TRUE );

  // start the sender process or initialize communication
  if (!mSendInProgress)
  {
    KMBroadcastStatus::instance()->reset();
    KMBroadcastStatus::instance()->setStatusProgressEnable( "Sender", true );
    connect(KMBroadcastStatus::instance(), SIGNAL(signalAbortRequested()),
      SLOT(slotAbortSend()));
    kapp->ref();

    mSendInProgress = TRUE;
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

      // Run the precommand if there is one
      if (!mTransportInfo->precommand.isEmpty())
      {
        setStatusMsg(i18n("Executing precommand %1")
          .arg(mTransportInfo->precommand));
        mPrecommand = new KMPrecommand(mTransportInfo->precommand);
        connect(mPrecommand, SIGNAL(finished(bool)),
          SLOT(slotPrecommandFinished(bool)));
        if (!mPrecommand->start())
        {
          delete mPrecommand;
          mPrecommand = 0;
        }
        return;
      }

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
      setStatusMsg(i18n("Unrecognized transport protocol. Unable to send message."));
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

  // start sending the current message

  mSendProc->preSendInit();
  setStatusMsg(i18n("%3: subject of message","Sending message %1 of %2: %3")
	       .arg(mSentMessages+mFailedMessages+1).arg(mTotalMessages)
	       .arg(mCurrentMsg->subject()));
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
  mSendProc = 0;
  mSendProcStarted = FALSE;
  if (mSendInProgress) kapp->deref();
  mSendInProgress = FALSE;
  if (mCurrentMsg)
  {
    mCurrentMsg->setTransferInProgress( FALSE );
    mCurrentMsg = 0;
  }
  disconnect(kernel->outboxFolder(), SIGNAL(msgAdded(int)),
             this, SLOT(outboxMsgAdded(int)));
  kernel->sentFolder()->close();
  kernel->outboxFolder()->close();
  if (kernel->outboxFolder()->count()<0)
    kernel->outboxFolder()->expunge();
  else kernel->outboxFolder()->compact();

  mSendAborted = false;
  mSentMessages = 0;
  mFailedMessages = 0;
  mSentBytes = 0;
  disconnect(KMBroadcastStatus::instance(), SIGNAL(signalAbortRequested()),
    this, SLOT(slotAbortSend()));
  KMBroadcastStatus::instance()->setStatusProgressEnable( "Sender", false );
  KMBroadcastStatus::instance()->reset();
  kernel->filterMgr()->cleanup();
}


//-----------------------------------------------------------------------------
void KMSender::slotAbortSend()
{
  mSendAborted = true;
  delete mPrecommand;
  mPrecommand = 0;
  if (mSendProc) mSendProc->abort();
}

//-----------------------------------------------------------------------------
void KMSender::slotIdle()
{
  assert(mSendProc != 0);

  QString msg;
  QString errString;
  if (mSendProc)
      errString = mSendProc->message();

  if (mSendAborted) {
    // sending of message aborted
    msg = i18n("Sending aborted:\n%1\n"
        "The message will stay in the 'outbox' folder until you either "
        "fix the problem (e.g. a broken address) or remove the message "
        "from the 'outbox' folder.\n"
        "The following transport protocol was used:\n  %2")
      .arg(errString)
      .arg(mMethodStr);
    if (!errString.isEmpty()) KMessageBox::error(0,msg);
  } else {
    if (!mSendProc->sendOk()) {
      mCurrentMsg->setTransferInProgress( false );
      mCurrentMsg = 0;
      mFailedMessages++;
      // Sending of message failed.
      if (!errString.isEmpty()) {
        int res = KMessageBox::Yes;
        if (mSentMessages+mFailedMessages != mTotalMessages) {
          msg = i18n("<p>Sending failed:</p>"
            "<p>%1</p>"
            "<p>The message will stay in the 'outbox' folder until you either "
            "fix the problem (e.g. a broken address) or remove the message "
            "from the 'outbox' folder.</p>"
            "<p>The following transport protocol was used:  %2</p>"
            "<p>Do you want me to continue sending the remaining messages?</p>")
            .arg(errString)
            .arg(mMethodStr);
          res = KMessageBox::warningYesNo( 0 , msg ,
                  i18n( "Continue sending" ), i18n( "&Continue sending" ),
                  i18n("&Abort sending") );
        } else {
          msg = i18n("Sending failed:\n%1\n"
            "The message will stay in the 'outbox' folder until you either "
            "fix the problem (e.g. a broken address) or remove the message "
            "from the 'outbox' folder.\n"
            "The following transport protocol was used:\n %2")
            .arg(errString)
            .arg(mMethodStr);
          KMessageBox::error(0,msg);
        }
        if (res == KMessageBox::Yes) {
          // Try the next one.
          doSendMsg();
          return;
        } else {
          setStatusMsg( i18n( "Sending aborted." ) );
        }
      }
    } else {
      // Sending suceeded.
      doSendMsg();
      return;
    }
  }
  mSendProc->finish(true);
  mSendProc = 0;
  mSendProcStarted = false;

  cleanup();
}


//-----------------------------------------------------------------------------
void KMSender::slotPrecommandFinished(bool normalExit)
{
  delete mPrecommand;
  mPrecommand = 0;
  if (normalExit) mSendProc->start();
  else slotIdle();
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
    if (transport.startsWith("smtp://"))
    {
      mTransportInfo->type = "smtp";
      mTransportInfo->auth = FALSE;
      mTransportInfo->encryption = "NONE";
      QString serverport = transport.mid(7);
      int colon = serverport.find(':');
      if (colon != -1) {
        mTransportInfo->host = serverport.left(colon);
        mTransportInfo->port = serverport.mid(colon + 1);
      } else {
        mTransportInfo->host = serverport;
        mTransportInfo->port = "25";
      }
    } else
    if (transport.startsWith("smtps://"))
    {
      mTransportInfo->type = "smtps";
      mTransportInfo->auth = FALSE;
      mTransportInfo->encryption = "ssl";
      QString serverport = transport.mid(7);
      int colon = serverport.find(':');
      if (colon != -1) {
        mTransportInfo->host = serverport.left(colon);
        mTransportInfo->port = serverport.mid(colon + 1);
      } else {
        mTransportInfo->host = serverport;
        mTransportInfo->port = "465";
      }
    }
    else if (transport.startsWith("file://"))
    {
      mTransportInfo->type = "sendmail";
      mTransportInfo->host = transport.mid(7);
    }
  }

  if (mTransportInfo->type == "sendmail")
    return new KMSendSendmail(this);
  if (mTransportInfo->type == "smtp" || mTransportInfo->type == "smtps")
    return new KMSendSMTP(this);

  return 0L;
}

//-----------------------------------------------------------------------------
void KMSender::setStatusByLink(const KMMessage *aMsg)
{
  int n = 0;
  while (1) {
    ulong msn;
    KMMsgStatus status;
    aMsg->getLink(n, &msn, &status);
    if (!msn || !status)
      break;
    n++;

    KMFolder *folder;
    int index;
    kernel->msgDict()->getLocation(msn, &folder, &index);

    if (folder) {
      folder->open();
      folder->setStatus(index, status);
      folder->close();
    }
  }
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
  if (destructive) deleteLater();
  return TRUE;
}

#if !KDE_IS_VERSION( 3, 1, 90 ) // dotstuffing and LF->CRLF is not
				// done by the SMTP kioslave
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
#endif

//-----------------------------------------------------------------------------
void KMSendProc::statusMsg(const QString& aMsg)
{
  if (mSender) mSender->setStatusMsg(aMsg);
}

//-----------------------------------------------------------------------------
bool KMSendProc::addRecipients( const AddrSpecList & al )
{
  for ( AddrSpecList::const_iterator it = al.begin() ; it != al.end() ; ++it )
    if ( !addOneRecipient( (*it).asString() ) )
      return false;
  return true;
}


//=============================================================================
//=============================================================================
KMSendSendmail::KMSendSendmail(KMSender* aSender):
  KMSendSendmailInherited(aSender)
{
  mMailerProc = 0;
}

//-----------------------------------------------------------------------------
KMSendSendmail::~KMSendSendmail()
{
  delete mMailerProc;
}

//-----------------------------------------------------------------------------
void KMSendSendmail::start(void)
{
  if (mSender->transportInfo()->host.isEmpty())
  {
    QString str = i18n("Please specify a mailer program in the settings.");
    QString msg;
    msg = i18n("Sending failed:\n%1\n"
	"The message will stay in the 'outbox' folder and will be resent.\n"
        "Please remove it from there if you do not want the message to "
		"be resent.\n"
	"The following transport protocol was used:\n  %2")
    .arg(str + "\n")
    .arg("sendmail://");
    KMessageBox::information(0,msg);
    emit started(false);
    return;
  }

  if (!mMailerProc)
  {
    mMailerProc = new KProcess;
    assert(mMailerProc != 0);
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
  delete mMailerProc;
  mMailerProc = 0;
  if (destructive)
    	deleteLater();
  return TRUE;
}

//-----------------------------------------------------------------------------
void KMSendSendmail::abort()
{
  delete mMailerProc;
  mMailerProc = 0;
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

  if( !aMsg->headerField("X-KMail-Recipients").isEmpty() ) {
    // extended BCC handling to prevent TOs and CCs from seeing
    // BBC information by looking at source of an OpenPGP encrypted mail
    addRecipients(aMsg->extractAddrSpecs("X-KMail-Recipients"));
    aMsg->removeHeaderField( "X-KMail-Recipients" );
  } else {
    addRecipients(aMsg->extractAddrSpecs("To"));
    addRecipients(aMsg->extractAddrSpecs("Cc"));
    addRecipients(aMsg->extractAddrSpecs("Bcc"));
  }

  mMsgStr = aMsg->asSendableString();

  if (!mMailerProc->start(KProcess::NotifyOnExit,KProcess::All))
  {
    KMessageBox::information(0,i18n("Failed to execute mailer program %1")
			     .arg(mSender->transportInfo()->host));
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

  assert(proc!=0);
  Q_UNUSED( proc );

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
  assert(proc!=0);
  Q_UNUSED( proc );
  mMsg.replace(mMsg.length(), buflen, buffer);
}


//-----------------------------------------------------------------------------
void KMSendSendmail::sendmailExited(KProcess *proc)
{
  assert(proc!=0);
  mSendOk = (proc->normalExit() && proc->exitStatus()==0);
  if (!mSendOk) failed(i18n("Sendmail exited abnormally."));
  mMsgStr = 0;
  emit idle();
}


//-----------------------------------------------------------------------------
bool KMSendSendmail::addOneRecipient(const QString& aRcpt)
{
  assert(mMailerProc!=0);
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
  assert(aMsg != 0);

  // email this is from
  mQuery = QString("headers=0&from=%1")
                  .arg(KURL::encode_string(KMMessage::getEmailAddr(aMsg->from())));

  // recipients
  if( !aMsg->headerField("X-KMail-Recipients").isEmpty() ) {
    // extended BCC handling to prevent TOs and CCs from seeing
    // BBC information by looking at source of an OpenPGP encrypted mail
    mQueryField = "&to=";
    if( !addRecipients( aMsg->extractAddrSpecs("X-KMail-Recipients")) ) {
      return FALSE;
    }
    aMsg->removeHeaderField( "X-KMail-Recipients" );
  } else {
    mQueryField = "&to=";
    if(!addRecipients(aMsg->extractAddrSpecs("To")))
    {
      return FALSE;
    }

    if(!aMsg->cc().isEmpty())
    {
      mQueryField = "&cc=";
      if(!addRecipients(aMsg->extractAddrSpecs("Cc"))) return FALSE;
    }

    QString bccStr = aMsg->bcc();
    if(!bccStr.isEmpty())
    {
      mQueryField = "&bcc=";
      if (!addRecipients(aMsg->extractAddrSpecs("Bcc"))) return FALSE;
    }
  }

  if (ti->specifyHostname)
    mQuery += "&hostname=" + KURL::encode_string(ti->localHostname);

  if ( !kernel->msgSender()->sendQuotedPrintable() )
    mQuery += "&body=8bit";

  KURL destination;

  destination.setProtocol((ti->encryption == "SSL") ? "smtps" : "smtp");
  destination.setHost(ti->host);
  destination.setPort(ti->port.toUShort());

  if (ti->auth)
  {
    if(ti->user.isEmpty() || ti->pass.isEmpty())
    {
      bool b = FALSE;
      int result;

      KCursorSaver idle(KBusyPtr::idle());
      result = KIO::PasswordDialog::getNameAndPassword(ti->user, ti->pass,
	&b, i18n("You need to supply a username and a password to use this "
	     "SMTP server."), FALSE, QString::null, ti->name, QString::null);

      if ( result != QDialog::Accepted )
      {
        abort();
        return FALSE;
      }
      if (int id = KMTransportInfo::findTransport(ti->name))
        ti->writeConfig(id);
    }
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

#if KDE_IS_VERSION( 3, 1, 90 )
  // dotstuffing is now done by the slave (see setting of metadata)
  mMessage = aMsg->asSendableString();
#else
  mMessage = prepareStr(aMsg->asSendableString(), TRUE);
#endif
  mMessageLength = mMessage.length();
  mMessageOffset = 0;

  if ( mMessageLength )
    // allow +5% for subsequent LF->CRLF and dotstuffing (an average
    // over 2G-lines gives an average line length of 42-43):
    mQuery += "&size=" + QString::number( qRound( mMessageLength * 1.05 ) );

  destination.setPath("/send");
  destination.setQuery(mQuery);
  mQuery = QString::null;

  if ((mJob = KIO::put(destination, -1, false, false, false)))
  {
#if KDE_IS_VERSION( 3, 1, 90 )
    mJob->addMetaData( "lf2crlf+dotstuff", "slave" );
#endif
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
    mQuery += mQueryField + KURL::encode_string(_addr);

  return true;
}

void KMSendSMTP::dataReq(KIO::Job *, QByteArray &array)
{
  // Send it by 32K chuncks
  int chunkSize = QMIN( mMessageLength - mMessageOffset, 0x8000 );
  if ( chunkSize > 0 ) {
    array.duplicate(mMessage.data() + mMessageOffset, chunkSize);
    mMessageOffset += chunkSize;
  } else
  {
    array.resize(0);
    mMessage.resize(0);
  }
  mSender->emitProgressInfo( mMessageOffset );
}

void KMSendSMTP::result(KIO::Job *_job)
{
  if (!mJob) return;
  mJob = 0;

  if(_job->error())
  {
    mSendOk = false;
    if (_job->error() == KIO::ERR_SLAVE_DIED) mSlave = 0;
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
    if (error == KIO::ERR_SLAVE_DIED) mSlave = 0;
    mSendOk = false;
    mJob = 0;
    failed(KIO::buildErrorString(error, errorMsg));
    abort();
  }
}

#include "kmsender.moc"
