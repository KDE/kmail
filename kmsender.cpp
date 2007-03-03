// kmsender.cpp

#include <config.h>

#define REALLY_WANT_KMSENDER
#include "kmsender.h"
#include "kmsender_p.h"
#undef REALLY_WANT_KMSENDER

#include <kmime/kmime_header_parsing.h>
#include <QByteArray>
using namespace KMime::Types;

#include <kio/passworddialog.h>
#include <kio/scheduler.h>
#include <kmessagebox.h>
#include <kdeversion.h>
#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kconfiggroup.h>

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "globalsettings.h"
#include "kmfiltermgr.h"

#include "kcursorsaver.h"
#include <libkpimidentities/identity.h>
#include <libkpimidentities/identitymanager.h>
#include "progressmanager.h"
#include "kmaccount.h"
#include "kmtransport.h"
#include "kmfolderindex.h"
#include "kmfoldermgr.h"
#include "kmmsgdict.h"
#include "kmmsgpart.h"
#include "protocols.h"
#include "kmcommands.h"
#include <mimelib/mediatyp.h>
#include <mimelib/enum.h>
#include <mimelib/param.h>

#define SENDER_GROUP "sending mail"

//-----------------------------------------------------------------------------
KMSender::KMSender()
  : mOutboxFolder( 0 ), mSentFolder( 0 )
{
  mPrecommand = 0;
  mSendProc = 0;
  mSendProcStarted = false;
  mSendInProgress = false;
  mCurrentMsg = 0;
  mTransportInfo = new KMTransportInfo();
  readConfig();
  mSendAborted = false;
  mSentMessages = 0;
  mTotalMessages = 0;
  mFailedMessages = 0;
  mSentBytes = 0;
  mTotalBytes = 0;
  mProgressItem = 0;
}


//-----------------------------------------------------------------------------
KMSender::~KMSender()
{
  writeConfig(false);
  delete mSendProc;
  delete mPrecommand;
  delete mTransportInfo;
}

//-----------------------------------------------------------------------------
void KMSender::setStatusMsg(const QString &msg)
{
  if ( mProgressItem )
    mProgressItem->setStatus(msg);
}

//-----------------------------------------------------------------------------
void KMSender::readConfig(void)
{
  QString str;
  KConfigGroup config(KMKernel::config(), SENDER_GROUP);

  mSendImmediate = config.readEntry( "Immediate", true );
  mSendQuotedPrintable = config.readEntry( "Quoted-Printable", true );
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

static void handleRedirections( KMMessage * m ) {
  const QString from  = m->headerField("X-KMail-Redirect-From");
  const QString msgId = m->msgId();
  if( from.isEmpty() || msgId.isEmpty() )
    m->setMsgId( KMMessage::generateMessageId( m->sender() ) );
}

//-----------------------------------------------------------------------------
bool KMSender::doSend(KMMessage* aMsg, short sendNow)
{
  if(!aMsg)
      return false;

  if (!settingsOk()) return false;

  if (aMsg->to().isEmpty())
  {
    // RFC822 says:
    // Note that the "Bcc" field may be empty, while the "To" field is required to
    // have at least one address.
    //
    // however:
    //
    // The following string is accepted according to RFC 2822,
    // section 3.4 "Address Specification" where they say:
    //
    //     "An address may either be an individual mailbox,
    //      or a group of mailboxes."
    // and:
    //     "group   +   display-name ":" [mailbox-list / CFWS] ";"
    //      [CFWS]"
    //
    // In this syntax our "undisclosed-recipients: ;"
    // just specifies an empty group.
    //
    // In further explanations RFC 2822 states that it *is*
    // allowed to have a ZERO number of mailboxes in the "mailbox-list".
    aMsg->setTo("Undisclosed.Recipients: ;");
  }

  handleRedirections( aMsg );

  if (sendNow==-1) sendNow = mSendImmediate;

  kmkernel->outboxFolder()->open();
  const KMFolderCloser openOutbox( kmkernel->outboxFolder() );

  aMsg->setStatus( MessageStatus::statusQueued() );

  if ( const int err = openOutbox.folder()->addMsg(aMsg) ) {
    Q_UNUSED( err );
    KMessageBox::information(0,i18n("Cannot add message to outbox folder"));
    return false;
  }

  //Ensure the message is correctly and fully parsed
  openOutbox.folder()->unGetMsg( openOutbox.folder()->count() - 1 );

  if ( !sendNow || mSendInProgress )
    return true;

  return sendQueued();
}


//-----------------------------------------------------------------------------
void KMSender::outboxMsgAdded(int idx)
{
    ++mTotalMessages;
    KMMsgBase* msg = kmkernel->outboxFolder()->getMsgBase(idx);
    Q_ASSERT(msg);
    if ( msg )
        mTotalBytes += msg->msgSize();
}


//-----------------------------------------------------------------------------
bool KMSender::doSendQueued( const QString &customTransport )
{
  if (!settingsOk()) return false;

  if (mSendInProgress)
  {
    return false;
  }

  // open necessary folders
  mOutboxFolder = kmkernel->outboxFolder();
  mOutboxFolder->open();
  mTotalMessages = mOutboxFolder->count();
  if (mTotalMessages == 0) {
    // Nothing in the outbox. We are done.
    mOutboxFolder->close();
    mOutboxFolder = 0;
    return true;
  }
  mTotalBytes = 0;
  for( int i = 0 ; i<mTotalMessages ; ++i )
      mTotalBytes += mOutboxFolder->getMsgBase(i)->msgSize();

  connect( mOutboxFolder, SIGNAL(msgAdded(int)),
           this, SLOT(outboxMsgAdded(int)) );
  mCurrentMsg = 0;

  mSentFolder = kmkernel->sentFolder();
  mSentFolder->open();
  kmkernel->filterMgr()->ref();

  // start sending the messages
  mCustomTransport = customTransport;
  doSendMsg();
  return true;
}

//-----------------------------------------------------------------------------
void KMSender::emitProgressInfo( int currentFileProgress )
{
  int percent = (mTotalBytes) ? ( 100 * (mSentBytes+currentFileProgress) / mTotalBytes ) : 0;
  if (percent > 100) percent = 100;
  mProgressItem->setProgress(percent);
}

static bool messageIsDispositionNotificationReport( KMMessage *msg )
{
    if ( msg->type() == DwMime::kTypeMessage &&
         msg->subtype() == DwMime::kSubtypeDispositionNotification )
      return true;

    if ( msg->type() != DwMime::kTypeMultipart ||
         msg->subtype() != DwMime::kSubtypeReport )
      return false;

    DwMediaType& ct = msg->dwContentType();
    DwParameter *param = ct.FirstParameter();
    while( param ) {
      if ( !qstricmp( param->Attribute().c_str(), "report-type")
        && !qstricmp( param->Value().c_str(), "disposition-notification" ) )
        return true;
      else
        param = param->Next();
    }
    return false;
}

//-----------------------------------------------------------------------------
void KMSender::doSendMsg()
{
  if (!kmkernel)  //To handle message sending in progress when kaplan is exited
    return;	//TODO: handle this case better

  const bool someSent = mCurrentMsg;
  if (someSent) {
      mSentMessages++;
      mSentBytes += mCurrentMsg->msgSize();
  }

  // Post-process sent message (filtering)
  KMFolder *sentFolder = 0, *imapSentFolder = 0;
  if (mCurrentMsg  && kmkernel->filterMgr())
  {
    mCurrentMsg->setTransferInProgress( false );
    if( mCurrentMsg->hasUnencryptedMsg() ) {
      kDebug(5006) << "KMSender::doSendMsg() post-processing: replace mCurrentMsg body by unencryptedMsg data" << endl;
      // delete all current body parts
      mCurrentMsg->deleteBodyParts();
      // copy Content-[..] headers from unencrypted message to current one
      KMMessage & newMsg( *mCurrentMsg->unencryptedMsg() );
      mCurrentMsg->dwContentType() = newMsg.dwContentType();
      mCurrentMsg->setContentTransferEncodingStr( newMsg.contentTransferEncodingStr() );
      QByteArray newDispo = newMsg.headerField("Content-Disposition").toLatin1();
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
    MessageStatus status = MessageStatus::statusSent();
    status.setRead(); // otherwise it defaults to new on imap
    mCurrentMsg->setStatus( status );
    mCurrentMsg->updateAttachmentState();

    const KPIM::Identity & id = kmkernel->identityManager()
      ->identityForUoidOrDefault( mCurrentMsg->headerField( "X-KMail-Identity" ).trimmed().toUInt() );
    if ( !mCurrentMsg->fcc().isEmpty() )
    {
      sentFolder = kmkernel->folderMgr()->findIdString( mCurrentMsg->fcc() );
      if ( sentFolder == 0 )
      // This is *NOT* supposed to be imapSentFolder!
        sentFolder =
          kmkernel->dimapFolderMgr()->findIdString( mCurrentMsg->fcc() );
      if ( sentFolder == 0 )
        imapSentFolder =
          kmkernel->imapFolderMgr()->findIdString( mCurrentMsg->fcc() );
    }
    // No, or no usable sentFolder, and no, or no usable imapSentFolder,
    // let's try the on in the identity
    if ( ( sentFolder == 0 || sentFolder->isReadOnly() )
      && ( imapSentFolder == 0 || imapSentFolder->isReadOnly() )
      && !id.fcc().isEmpty() )
    {
      sentFolder = kmkernel->folderMgr()->findIdString( id.fcc() );
      if ( sentFolder == 0 )
        // This is *NOT* supposed to be imapSentFolder!
        sentFolder = kmkernel->dimapFolderMgr()->findIdString( id.fcc() );
      if ( sentFolder == 0 )
        imapSentFolder = kmkernel->imapFolderMgr()->findIdString( id.fcc() );
    }
    if (imapSentFolder
        && ( imapSentFolder->noContent() || imapSentFolder->isReadOnly() ) )
        imapSentFolder = 0;

    if ( sentFolder == 0 || sentFolder->isReadOnly() )
      sentFolder = kmkernel->sentFolder();

    if ( const int err = sentFolder->open() ) {
      Q_UNUSED( err );
      cleanup();
      return;
    }

    // Disable the emitting of msgAdded signal, because the message is taken out of the
    // current folder (outbox) and re-added, to make filter actions changing the message
    // work. We don't want that to screw up message counts.
    if ( mCurrentMsg->parent() ) mCurrentMsg->parent()->quiet( true );
    const int processResult = kmkernel->filterMgr()->process(mCurrentMsg,KMFilterMgr::Outbound);
    if ( mCurrentMsg->parent() ) mCurrentMsg->parent()->quiet( false );

    // 0==processed ok, 1==no filter matched, 2==critical error, abort!
    switch (processResult) {
    case 2:
      perror("Critical error: Unable to process sent mail (out of space?)");
      KMessageBox::information(0, i18n("Critical error: "
                   "Unable to process sent mail (out of space?)"
                   "Moving failing message to \"sent-mail\" folder."));
      if ( sentFolder ) {
        sentFolder->moveMsg(mCurrentMsg);
        sentFolder->close();
      }
      cleanup();
      return;
    case 1:
      if (sentFolder->moveMsg(mCurrentMsg) != 0)
      {
        KMessageBox::error(0, i18n("Moving the sent message \"%1\" from the "
          "\"outbox\" to the \"sent-mail\" folder failed.\n"
          "Possible reasons are lack of disk space or write permission. "
          "Please try to fix the problem and move the message manually.",
           mCurrentMsg->subject()));
        cleanup();
        return;
      }
      if (imapSentFolder) {
        // Does proper folder refcounting and message locking
        KMCommand *command = new KMMoveCommand( imapSentFolder, mCurrentMsg );
        command->keepFolderOpen( sentFolder ); // will open it, and close it once done
        command->start();
      }
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
  mCurrentMsg = mOutboxFolder->getMsg(mFailedMessages);
  if ( mCurrentMsg && !mCurrentMsg->transferInProgress() &&
       mCurrentMsg->sender().isEmpty() ) {
    // if we do not have a sender address then use the email address of the
    // message's identity or of the default identity unless those two are also
    // empty
    const KPIM::Identity & id = kmkernel->identityManager()
      ->identityForUoidOrDefault( mCurrentMsg->headerField( "X-KMail-Identity" ).trimmed().toUInt() );
    if ( !id.emailAddr().isEmpty() ) {
      mCurrentMsg->setFrom( id.fullEmailAddr() );
    }
    else if ( !kmkernel->identityManager()->defaultIdentity().emailAddr().isEmpty() ) {
      mCurrentMsg->setFrom( kmkernel->identityManager()->defaultIdentity().fullEmailAddr() );
    }
    else {
      KMessageBox::sorry( 0, i18n( "It is not possible to send messages "
                                   "without specifying a sender address.\n"
                                   "Please set the email address of "
                                   "identity '%1' in the Identities "
                                   "section of the configuration dialog "
                                   "and then try again." ,
                               id.identityName() ) );
      mOutboxFolder->unGetMsg( mFailedMessages );
      mCurrentMsg = 0;
    }
  }
  if (!mCurrentMsg || mCurrentMsg->transferInProgress())
  {
    // a message is locked finish the send
    if (mCurrentMsg && mCurrentMsg->transferInProgress())
    	mCurrentMsg = 0;
    // no more message: cleanup and done
    if ( sentFolder != 0 )
        sentFolder->close();
    if ( someSent ) {
      if ( mSentMessages == mTotalMessages ) {
        setStatusMsg(i18np("%1 queued message successfully sent.",
                          "%1 queued messages successfully sent.",
                          mSentMessages));
      } else {
        setStatusMsg(i18n("%1 of %2 queued messages successfully sent.",
             mSentMessages, mTotalMessages ));
      }
    }
    cleanup();
    return;
  }
  mCurrentMsg->setTransferInProgress( true );

  // start the sender process or initialize communication
  if (!mSendInProgress)
  {
    Q_ASSERT( !mProgressItem );
    mProgressItem = KPIM::ProgressManager::createProgressItem(
      "Sender",
      i18n( "Sending messages" ),
      i18n("Initiating sender process..."),
      true );
    connect( mProgressItem, SIGNAL(progressItemCanceled(KPIM::ProgressItem*)),
             this, SLOT( slotAbortSend() ) );
    KGlobal::ref();
    mSendInProgress = true;
  }

  QString msgTransport = mCustomTransport;
  if ( msgTransport.isEmpty() ) {
    msgTransport = mCurrentMsg->headerField("X-KMail-Transport");
  }
  if ( msgTransport.isEmpty() ) {
    const QStringList sl = KMTransportInfo::availableTransports();
    if (!sl.empty()) msgTransport = sl.front();
  }

  if (!mSendProc || msgTransport != mMethodStr) {
    if (mSendProcStarted && mSendProc) {
      mSendProc->finish();
      mSendProcStarted = false;
    }

    mSendProc = createSendProcFromString(msgTransport);
    mMethodStr = msgTransport;

    if( mTransportInfo->encryption == "TLS" || mTransportInfo->encryption == "SSL" ) {
      mProgressItem->setUsesCrypto( true );
    } else if ( !mCustomTransport.isEmpty() ) {
        int result = KMessageBox::warningContinueCancel( 0,
        i18n( "You have chosen to send all queued email using an unencrypted transport, do you want to continue? "),
        i18n( "Security Warning" ),
        KGuiItem(i18n( "Send Unencrypted" )),
        "useCustomTransportWithoutAsking", false);

      if( result == KMessageBox::Cancel ) {
        mProgressItem->cancel();
        mProgressItem->setComplete();
        slotAbortSend();
        cleanup();
        return;
      }
    }

    if (!mSendProc)
      sendProcStarted(false);
    else {
      connect(mSendProc, SIGNAL(idle()), SLOT(slotIdle()));
      connect(mSendProc, SIGNAL(started(bool)), SLOT(sendProcStarted(bool)));

      // Run the precommand if there is one
      if ( !mTransportInfo->precommand.isEmpty() ) {
        runPrecommand( mTransportInfo->precommand );
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

bool KMSender::runPrecommand( const QString & cmd ) {
  setStatusMsg( i18n("Executing precommand %1", cmd ) );
  mPrecommand = new KMPrecommand( cmd );
  connect( mPrecommand, SIGNAL(finished(bool)),
           SLOT(slotPrecommandFinished(bool)) );
  if ( !mPrecommand->start() ) {
    delete mPrecommand; mPrecommand = 0;
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
void KMSender::sendProcStarted(bool success)
{
  if (!success) {
    if (mSendProc)
       mSendProc->finish();
    else
      setStatusMsg(i18n("Unrecognized transport protocol. Unable to send message."));
    mSendProc = 0;
    mSendProcStarted = false;
    cleanup();
    return;
  }
  doSendMsgAux();
}


static QStringList addrSpecListToStringList( const AddrSpecList & l, bool allowEmpty=false ) {
  QStringList result;
  for ( AddrSpecList::const_iterator it = l.begin(), end = l.end() ; it != end ; ++it ) {
    const QString s = (*it).asString();
    if ( allowEmpty || !s.isEmpty() )
      result.push_back( s );
  }
  return result;
}

static void extractSenderToCCAndBcc( KMMessage * aMsg, QString * sender, QStringList * to, QStringList * cc, QStringList * bcc ) {
  if ( sender ) *sender = aMsg->sender();
  if( !aMsg->headerField("X-KMail-Recipients").isEmpty() ) {
    // extended BCC handling to prevent TOs and CCs from seeing
    // BBC information by looking at source of an OpenPGP encrypted mail
    if ( to ) *to = addrSpecListToStringList( aMsg->extractAddrSpecs( "X-KMail-Recipients" ) );
    aMsg->removeHeaderField( "X-KMail-Recipients" );
  } else {
    if ( to ) *to = addrSpecListToStringList( aMsg->extractAddrSpecs( "To" ) );
    if ( cc ) *cc = addrSpecListToStringList( aMsg->extractAddrSpecs( "Cc" ) );
    if ( bcc ) *bcc = addrSpecListToStringList( aMsg->extractAddrSpecs( "Bcc" ) );
  }
}

//-----------------------------------------------------------------------------
void KMSender::doSendMsgAux()
{
  mSendProcStarted = true;

  // start sending the current message

  setStatusMsg(i18nc("%3: subject of message","Sending message %1 of %2: %3",
	        mSentMessages+mFailedMessages+1, mTotalMessages,
	        mCurrentMsg->subject()));
  QStringList to, cc, bcc;
  QString sender;
  extractSenderToCCAndBcc( mCurrentMsg, &sender, &to, &cc, &bcc );

  // MDNs are required to have an empty envelope from as per RFC2298.
  if ( messageIsDispositionNotificationReport( mCurrentMsg ) && GlobalSettings::self()->sendMDNsWithEmptySender() )
    sender = "<>";

  const QByteArray message = mCurrentMsg->asSendableString();
  if ( sender.isEmpty() || !mSendProc->send( sender, to, cc, bcc, message ) ) {
    if ( mCurrentMsg )
      mCurrentMsg->setTransferInProgress( false );
    if ( mOutboxFolder )
      mOutboxFolder->unGetMsg( mFailedMessages );
    mCurrentMsg = 0;
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
  kDebug(5006) << k_funcinfo << endl;
  if (mSendProc && mSendProcStarted) mSendProc->finish();
  mSendProc = 0;
  mSendProcStarted = false;
  if (mSendInProgress) KGlobal::deref();
  mSendInProgress = false;
  if (mCurrentMsg)
  {
    mCurrentMsg->setTransferInProgress( false );
    mCurrentMsg = 0;
  }
  if ( mSentFolder ) {
    mSentFolder->close();
    mSentFolder = 0;
  }
  if ( mOutboxFolder ) {
    disconnect( mOutboxFolder, SIGNAL(msgAdded(int)),
                this, SLOT(outboxMsgAdded(int)) );
    mOutboxFolder->close();
    if ( mOutboxFolder->count( true ) == 0 ) {
      mOutboxFolder->expunge();
    }
    else if ( mOutboxFolder->needsCompacting() ) {
      mOutboxFolder->compact( KMFolder::CompactSilentlyNow );
    }
    mOutboxFolder = 0;
  }

  mSendAborted = false;
  mSentMessages = 0;
  mFailedMessages = 0;
  mSentBytes = 0;
  if ( mProgressItem )
    mProgressItem->setComplete();
  mProgressItem = 0;
  kmkernel->filterMgr()->deref();
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
      errString = mSendProc->lastErrorMessage();

  if (mSendAborted) {
    // sending of message aborted
    if ( mCurrentMsg ) {
      mCurrentMsg->setTransferInProgress( false );
      if ( mOutboxFolder )
        mOutboxFolder->unGetMsg( mFailedMessages );
      mCurrentMsg = 0;
    }
    msg = i18n("Sending aborted:\n%1\n"
        "The message will stay in the 'outbox' folder until you either "
        "fix the problem (e.g. a broken address) or remove the message "
        "from the 'outbox' folder.\n"
        "The following transport protocol was used:\n  %2",
       errString,
       mMethodStr);
    if (!errString.isEmpty()) KMessageBox::error(0,msg);
    setStatusMsg( i18n( "Sending aborted." ) );
  } else {
    if (!mSendProc->sendOk()) {
      if ( mCurrentMsg )
        mCurrentMsg->setTransferInProgress( false );
      if ( mOutboxFolder )
        mOutboxFolder->unGetMsg( mFailedMessages );
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
            "<p>Do you want me to continue sending the remaining messages?</p>",
             errString,
             mMethodStr);
          res = KMessageBox::warningYesNo( 0 , msg ,
                  i18n( "Continue Sending" ), KGuiItem(i18n( "&Continue Sending" )),
                  KGuiItem(i18n("&Abort Sending")) );
        } else {
          msg = i18n("Sending failed:\n%1\n"
            "The message will stay in the 'outbox' folder until you either "
            "fix the problem (e.g. a broken address) or remove the message "
            "from the 'outbox' folder.\n"
            "The following transport protocol was used:\n %2",
             errString,
             mMethodStr);
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
  mSendProc->finish();
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
KMSendProc* KMSender::createSendProcFromString( const QString & transport )
{
  mTransportInfo->type.clear();
  int nr = KMTransportInfo::findTransport(transport);
  if (nr)
  {
    mTransportInfo->readConfig(nr);
  } else {
    if (transport.startsWith("smtp://")) // should probably use KUrl and SMTP_PROTOCOL
    {
      mTransportInfo->type = "smtp";
      mTransportInfo->auth = false;
      mTransportInfo->encryption = "NONE";
      QString serverport = transport.mid(7);
      int colon = serverport.indexOf(':');
      if (colon != -1) {
        mTransportInfo->host = serverport.left(colon);
        mTransportInfo->port = serverport.mid(colon + 1);
      } else {
        mTransportInfo->host = serverport;
        mTransportInfo->port = "25";
      }
    } else
    if (transport.startsWith("smtps://"))  // should probably use KUrl and SMTPS_PROTOCOL
    {
      mTransportInfo->type = "smtps";
      mTransportInfo->auth = false;
      mTransportInfo->encryption = "ssl";
      QString serverport = transport.mid(7);
      int colon = serverport.indexOf(':');
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
  // strip off a trailing "/"
  while (mTransportInfo->host.endsWith("/")) {
    mTransportInfo->host.truncate(mTransportInfo->host.length()-1);
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
    MessageStatus status;
    aMsg->getLink(n, &msn, status);
    if ( !msn || status.isOfUnknownStatus() )
      break;
    n++;

    KMFolder *folder = 0;
    int index = -1;
    KMMsgDict::instance()->getLocation(msn, &folder, &index);
    if (folder && index != -1) {
      folder->open();
      if ( status.isDeleted() ) {
        // Move the message to the trash folder
        KMDeleteMsgCommand *cmd =
          new KMDeleteMsgCommand( folder, folder->getMsg( index ) );
        cmd->start();
      } else {
        folder->setStatus(index, status);
      }
      folder->close();
    } else {
      kWarning(5006) << k_funcinfo << "Cannot update linked message, it could not be found!" << endl;
    }
  }
}

//=============================================================================
//=============================================================================
KMSendProc::KMSendProc( KMSender * sender )
  : QObject( 0 ),
    mSender( sender ),
    mLastErrorMessage(),
    mSendOk( false ),
    mSending( false )
{
}

//-----------------------------------------------------------------------------
void KMSendProc::reset()
{
  mSending = false;
  mSendOk = false;
  mLastErrorMessage.clear();
}

//-----------------------------------------------------------------------------
void KMSendProc::failed(const QString &aMsg)
{
  mSending = false;
  mSendOk = false;
  mLastErrorMessage = aMsg;
}

//-----------------------------------------------------------------------------
void KMSendProc::statusMsg(const QString& aMsg)
{
  if (mSender) mSender->setStatusMsg(aMsg);
}

//=============================================================================
//=============================================================================
KMSendSendmail::KMSendSendmail( KMSender * sender )
  : KMSendProc( sender ),
    mMsgStr(),
    mMsgPos( 0 ),
    mMsgRest( 0 ),
    mMailerProc( 0 )
{

}

KMSendSendmail::~KMSendSendmail() {
  delete mMailerProc; mMailerProc = 0;
}

bool KMSendSendmail::doStart() {

  if (mSender->transportInfo()->host.isEmpty())
  {
    const QString str = i18n("Please specify a mailer program in the settings.");
    const QString msg = i18n("Sending failed:\n%1\n"
                             "The message will stay in the 'outbox' folder and will be resent.\n"
                             "Please remove it from there if you do not want the message to "
                             "be resent.\n"
                             "The following transport protocol was used:\n  %2",
                         str + '\n',
                         "sendmail://");
    KMessageBox::information(0,msg);
    return false;
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
  return true;
}

void KMSendSendmail::doFinish() {
  delete mMailerProc;
  mMailerProc = 0;
}

void KMSendSendmail::abort()
{
  delete mMailerProc;
  mMailerProc = 0;
  mSendOk = false;
  mMsgStr = 0;
  idle();
}

bool KMSendSendmail::doSend( const QString & sender, const QStringList & to, const QStringList & cc, const QStringList & bcc, const QByteArray & message ) {
  mMailerProc->clearArguments();
  *mMailerProc << mSender->transportInfo()->host
               << "-i" << "-f" << sender
               << to << cc << bcc ;

  mMsgStr = message;

  if ( !mMailerProc->start( KProcess::NotifyOnExit, KProcess::All ) ) {
    KMessageBox::information( 0, i18n("Failed to execute mailer program %1",
                                mSender->transportInfo()->host ) );
    return false;
  }
  mMsgPos  = mMsgStr.data();
  mMsgRest = mMsgStr.size();
  wroteStdin( mMailerProc );

  return true;
}


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


void KMSendSendmail::receivedStderr(KProcess *proc, char *buffer, int buflen)
{
  assert(proc!=0);
  Q_UNUSED( proc );
  mLastErrorMessage.replace(mLastErrorMessage.length(), buflen, buffer);
}


void KMSendSendmail::sendmailExited(KProcess *proc)
{
  assert(proc!=0);
  mSendOk = (proc->normalExit() && proc->exitStatus()==0);
  if (!mSendOk) failed(i18n("Sendmail exited abnormally."));
  mMsgStr = 0;
  emit idle();
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

bool KMSendSMTP::doSend( const QString & sender, const QStringList & to, const QStringList & cc, const QStringList & bcc, const QByteArray & message ) {
  QString query = "headers=0&from=";
  query += KUrl::toPercentEncoding( sender );

  foreach ( QString str, to )
    query += "&to=" + KUrl::toPercentEncoding( str );
  foreach ( QString str, cc )
    query += "&cc=" + KUrl::toPercentEncoding( str );
  foreach ( QString str, bcc )
    query += "&bcc=" + KUrl::toPercentEncoding( str );

  KMTransportInfo * ti = mSender->transportInfo();

  if ( ti->specifyHostname )
    query += "&hostname=" + KUrl::toPercentEncoding( ti->localHostname );

  if ( !kmkernel->msgSender()->sendQuotedPrintable() )
    query += "&body=8bit";

  KUrl destination;

  destination.setProtocol((ti->encryption == "SSL") ? SMTPS_PROTOCOL : SMTP_PROTOCOL);
  destination.setHost(ti->host);
  destination.setPort(ti->port.toUShort());

  if (ti->auth)
  {
    if( (ti->user.isEmpty() || ti->passwd().isEmpty()) &&
      ti->authType != "GSSAPI" )
    {
      bool b = false;
      int result;

      KCursorSaver idle(KBusyPtr::idle());
      QString passwd = ti->passwd();
      result = KIO::PasswordDialog::getNameAndPassword(ti->user, passwd,
	&b, i18n("You need to supply a username and a password to use this "
	     "SMTP server."), false, QString(), ti->name, QString());

      if ( result != QDialog::Accepted )
      {
        abort();
        return false;
      }
      if (int id = KMTransportInfo::findTransport(ti->name)) {
        ti->setPasswd( passwd );
        ti->writeConfig(id);
      }
    }
    destination.setUser(ti->user);
    destination.setPass(ti->passwd());
  }

  if (!mSlave || !mInProcess)
  {
    KIO::MetaData slaveConfig;
    slaveConfig.insert("tls", (ti->encryption == "TLS") ? "on" : "off");
    if (ti->auth) slaveConfig.insert("sasl", ti->authType);
    mSlave = KIO::Scheduler::getConnectedSlave(destination, slaveConfig);
  }

  if (!mSlave)
  {
    abort();
    return false;
  }

  // dotstuffing is now done by the slave (see setting of metadata)
  mMessage = message;
  mMessageLength = mMessage.size();
  mMessageOffset = 0;

  if ( mMessageLength )
    // allow +5% for subsequent LF->CRLF and dotstuffing (an average
    // over 2G-lines gives an average line length of 42-43):
    query += "&size=" + QString::number( qRound( mMessageLength * 1.05 ) );

  destination.setPath("/send");
  destination.setQuery( query );

  mJob = KIO::put( destination, -1, false, false, false );
  if ( !mJob ) {
    abort();
    return false;
  }
  mJob->addMetaData( "lf2crlf+dotstuff", "slave" );
  KIO::Scheduler::assignJobToSlave(mSlave, mJob);
  connect(mJob, SIGNAL(result(KJob *)), this, SLOT(result(KJob *)));
  connect(mJob, SIGNAL(dataReq(KIO::Job *, QByteArray &)),
          this, SLOT(dataReq(KIO::Job *, QByteArray &)));
  mSendOk = true;
  mInProcess = true;
  return true;
}

void KMSendSMTP::cleanup() {
  if(mJob)
  {
    mJob->kill( KJob::EmitResult );
    mJob = 0;
    mSlave = 0;
  }

  if (mSlave)
  {
    KIO::Scheduler::disconnectSlave(mSlave);
    mSlave = 0;
  }

  mInProcess = false;
}

void KMSendSMTP::abort() {
  cleanup();
  emit idle();
}

void KMSendSMTP::doFinish() {
  cleanup();
}

void KMSendSMTP::dataReq(KIO::Job *, QByteArray &array)
{
  // Send it by 32K chuncks
  const int chunkSize = qMin( mMessageLength - mMessageOffset, (unsigned int)32*1024 );
  if ( chunkSize > 0 ) {
    array = QByteArray(mMessage.data() + mMessageOffset, chunkSize);
    mMessageOffset += chunkSize;
  } else
  {
    array.resize(0);
    mMessage.resize(0);
  }
  mSender->emitProgressInfo( mMessageOffset );
}

void KMSendSMTP::result(KJob *_job)
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
#include "kmsender_p.moc"
