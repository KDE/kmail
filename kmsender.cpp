/*
 * kmail: KDE mail client
 * Copyright (c) 1996-1998 Stefan Taferner <taferner@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#define REALLY_WANT_KMSENDER
#include "kmsender.h"
#undef REALLY_WANT_KMSENDER

#include <kmime/kmime_header_parsing.h>
#include <QByteArray>
using namespace KMime::Types;

#include <kpimidentities/identity.h>
#include <kpimidentities/identitymanager.h>
#include <mailtransport/transport.h>
#include <mailtransport/transportjob.h>
#include <mailtransport/transportmanager.h>
using namespace MailTransport;

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
#include "progressmanager.h"
#include "kmaccount.h"
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
  :  mTransportJob( 0 ), mOutboxFolder( 0 ), mSentFolder( 0 )
{
  mSendProcStarted = false;
  mSendInProgress = false;
  mCurrentMsg = 0;
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
  if ( TransportManager::self()->transportNames().isEmpty() ) {
    KMessageBox::information( 0,
                              i18n("Please create an account for sending and try again.") );
    return false;
  }
  return true;
}

static void handleRedirections( KMMessage *m ) {
  const QString from  = m->headerField( "X-KMail-Redirect-From" );
  const QString msgId = m->msgId();
  if ( from.isEmpty() || msgId.isEmpty() ) {
    m->setMsgId( KMMessage::generateMessageId( m->sender() ) );
  }
}

//-----------------------------------------------------------------------------
bool KMSender::doSend(KMMessage *aMsg, short sendNow )
{
  if ( !aMsg ) {
    return false;
  }

  if ( !settingsOk() ) {
    return false;
  }

  if ( aMsg->to().isEmpty() ) {
    // RFC822 says:
    // Note that the "Bcc" field may be empty, while the "To" field is
    // required to have at least one address.
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
    aMsg->setTo( "Undisclosed.Recipients: ;" );
  }

  handleRedirections( aMsg );

  if ( sendNow == -1 ) {
    sendNow = mSendImmediate;
  }

  KMFolder * const outbox = kmkernel->outboxFolder();
  const KMFolderOpener openOutbox( outbox, "KMSender" );

  aMsg->setStatus( MessageStatus::statusQueued() );

  if ( const int err = openOutbox.folder()->addMsg( aMsg ) ) {
    Q_UNUSED( err );
    KMessageBox::information( 0, i18n("Cannot add message to outbox folder") );
    return false;
  }

  //Ensure the message is correctly and fully parsed
  openOutbox.folder()->unGetMsg( openOutbox.folder()->count() - 1 );

  if ( !sendNow || mSendInProgress ) {
    return true;
  }

  return sendQueued();
}

//-----------------------------------------------------------------------------
void KMSender::outboxMsgAdded( int idx )
{
  ++mTotalMessages;
  KMMsgBase* msg = kmkernel->outboxFolder()->getMsgBase( idx );
  Q_ASSERT( msg );
  if ( msg ) {
    mTotalBytes += msg->msgSize();
  }
}

//-----------------------------------------------------------------------------
bool KMSender::doSendQueued( const QString &customTransport )
{
  if ( !settingsOk() ) {
    return false;
  }

  if ( mSendInProgress ) {
    return false;
  }

  // open necessary folders
  mOutboxFolder = kmkernel->outboxFolder();
  mOutboxFolder->open( "dosendoutbox" );
  mTotalMessages = mOutboxFolder->count();
  if ( mTotalMessages == 0 ) {
    // Nothing in the outbox. We are done.
    mOutboxFolder->close( "dosendoutbox" );
    mOutboxFolder = 0;
    return true;
  }
  mTotalBytes = 0;
  for( int i = 0 ; i<mTotalMessages ; ++i ) {
    mTotalBytes += mOutboxFolder->getMsgBase(i)->msgSize();
  }

  connect( mOutboxFolder, SIGNAL( msgAdded( int ) ),
           this, SLOT( outboxMsgAdded( int ) ) );
  mCurrentMsg = 0;

  mSentFolder = kmkernel->sentFolder();
  mSentFolder->open( "dosendsent" );
  kmkernel->filterMgr()->ref();

  // start sending the messages
  mCustomTransport = customTransport;
  doSendMsg();
  return true;
}

//-----------------------------------------------------------------------------
void KMSender::slotProcessedSize( KJob *, qulonglong size )
{
  int percent = (mTotalBytes) ? ( 100 * (mSentBytes+size) / mTotalBytes ) : 0;
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
void KMSender::doSendMsg()
{
  if ( !kmkernel ) { //To handle message sending in progress when exiting
    return;	//TODO: handle this case better
  }

  const bool someSent = mCurrentMsg;
  if ( someSent ) {
      mSentMessages++;
      mSentBytes += mCurrentMsg->msgSize();
  }

  // Post-process sent message (filtering)
  KMFolder *sentFolder = 0, *imapSentFolder = 0;
  if ( mCurrentMsg  && kmkernel->filterMgr() ) {
    mCurrentMsg->setTransferInProgress( false );
    if ( mCurrentMsg->hasUnencryptedMsg() ) {
      kDebug(5006) <<"KMSender::doSendMsg() post-processing: replace mCurrentMsg body by unencryptedMsg data";
      // delete all current body parts
      mCurrentMsg->deleteBodyParts();
      // copy Content-[..] headers from unencrypted message to current one
      KMMessage & newMsg( *mCurrentMsg->unencryptedMsg() );
      mCurrentMsg->dwContentType() = newMsg.dwContentType();
      mCurrentMsg->setContentTransferEncodingStr( newMsg.contentTransferEncodingStr() );
      QByteArray newDispo =
        newMsg.headerField( "Content-Disposition" ).toLatin1();
      if (  newDispo.isEmpty() ) {
        mCurrentMsg->removeHeaderField( "Content-Disposition" );
      } else {
        mCurrentMsg->setHeaderField( "Content-Disposition", newDispo );
      }
      // copy the body
      mCurrentMsg->setBody( newMsg.body() );
      // copy all the body parts
      KMMessagePart msgPart;
      for ( int i = 0; i < newMsg.numBodyParts(); ++i ) {
        newMsg.bodyPart( i, &msgPart );
        mCurrentMsg->addBodyPart( &msgPart );
      }
    }
    MessageStatus status = MessageStatus::statusSent();
    status.setRead(); // otherwise it defaults to new on imap
    mCurrentMsg->setStatus( status );
    mCurrentMsg->updateAttachmentState();

    const KPIMIdentities::Identity & id =
      kmkernel->identityManager()->identityForUoidOrDefault(
        mCurrentMsg->headerField( "X-KMail-Identity" ).trimmed().toUInt() );
    if ( !mCurrentMsg->fcc().isEmpty() ) {
      sentFolder = kmkernel->folderMgr()->findIdString( mCurrentMsg->fcc() );
      if ( sentFolder == 0 ) {
      // This is *NOT* supposed to be imapSentFolder!
        sentFolder =
          kmkernel->dimapFolderMgr()->findIdString( mCurrentMsg->fcc() );
      }
      if ( sentFolder == 0 ) {
        imapSentFolder =
          kmkernel->imapFolderMgr()->findIdString( mCurrentMsg->fcc() );
      }
    }
    // No, or no usable sentFolder, and no, or no usable imapSentFolder,
    // let's try the on in the identity
    if ( ( sentFolder == 0 || sentFolder->isReadOnly() )
      && ( imapSentFolder == 0 || imapSentFolder->isReadOnly() )
      && !id.fcc().isEmpty() ) {
      sentFolder = kmkernel->folderMgr()->findIdString( id.fcc() );
      if ( sentFolder == 0 ) {
        // This is *NOT* supposed to be imapSentFolder!
        sentFolder = kmkernel->dimapFolderMgr()->findIdString( id.fcc() );
      }
      if ( sentFolder == 0 ) {
        imapSentFolder = kmkernel->imapFolderMgr()->findIdString( id.fcc() );
      }
    }
    if (imapSentFolder &&
        ( imapSentFolder->noContent() || imapSentFolder->isReadOnly() ) ) {
        imapSentFolder = 0;
    }

    if ( sentFolder == 0 || sentFolder->isReadOnly() ) {
      sentFolder = kmkernel->sentFolder();
    }

    if ( const int err = sentFolder->open( "sentFolder" ) ) {
      Q_UNUSED( err );
      cleanup();
      return;
    }

    // Disable the emitting of msgAdded signal, because the message is
    // taken out of the current folder (outbox) and re-added, to make
    // filter actions changing the message work. We don't want that to
    // screw up message counts.
    if ( mCurrentMsg->parent() ) {
      mCurrentMsg->parent()->quiet( true );
    }
    const int processResult =
      kmkernel->filterMgr()->process( mCurrentMsg, KMFilterMgr::Outbound );
    if ( mCurrentMsg->parent() ) {
      mCurrentMsg->parent()->quiet( false );
    }

    // 0==processed ok, 1==no filter matched, 2==critical error, abort!
    switch ( processResult ) {
    case 2:
      perror( "Critical error: Unable to process sent mail (out of space?)" );
      KMessageBox::information( 0,
                                i18n("Critical error: "
                                     "Unable to process sent mail (out of space?)"
                                     "Moving failing message to \"sent-mail\" folder.") );
      if ( sentFolder ) {
        sentFolder->moveMsg( mCurrentMsg );
        sentFolder->close( "sentFolder" );
      }
      cleanup();
      return;
    case 1:
      if ( sentFolder->moveMsg( mCurrentMsg ) != 0 ) {
        KMessageBox::error( 0,
                            i18n("Moving the sent message \"%1\" from the "
                                 "\"outbox\" to the \"sent-mail\" folder failed.\n"
                                 "Possible reasons are lack of disk space or write permission. "
                                 "Please try to fix the problem and move the message manually.",
                                 mCurrentMsg->subject() ) );
        cleanup();
        return;
      }
      if ( imapSentFolder ) {
        // Does proper folder refcounting and message locking
        KMCommand *command = new KMMoveCommand( imapSentFolder, mCurrentMsg );
        command->keepFolderOpen( sentFolder ); // will open it, and close when done
        command->start();
      }
    default:
      break;
    }
    setStatusByLink( mCurrentMsg );
    if ( mCurrentMsg->parent() && !imapSentFolder ) {
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
  mCurrentMsg = mOutboxFolder->getMsg( mFailedMessages );
  if ( mCurrentMsg && !mCurrentMsg->transferInProgress() &&
       mCurrentMsg->sender().isEmpty() ) {
    // if we do not have a sender address then use the email address of the
    // message's identity or of the default identity unless those two are
    // also empty
    const KPIMIdentities::Identity &id =
      kmkernel->identityManager()->identityForUoidOrDefault(
        mCurrentMsg->headerField( "X-KMail-Identity" ).trimmed().toUInt() );
    if ( !id.emailAddr().isEmpty() ) {
      mCurrentMsg->setFrom( id.fullEmailAddr() );
    } else if ( !kmkernel->identityManager()->defaultIdentity().emailAddr().isEmpty() ) {
      mCurrentMsg->setFrom( kmkernel->identityManager()->defaultIdentity().fullEmailAddr() );
    } else {
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
  if ( !mCurrentMsg || mCurrentMsg->transferInProgress() ) {
    // a message is locked finish the send
    if ( mCurrentMsg && mCurrentMsg->transferInProgress() ) {
      mCurrentMsg = 0;
    }
    // no more message: cleanup and done
    if ( sentFolder != 0 ) {
      sentFolder->close( "sentFolder" );
    }
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

  /// start the sender process or initialize communication
  if ( !mSendInProgress ) {
    Q_ASSERT( !mProgressItem );
    mProgressItem =
      KPIM::ProgressManager::createProgressItem(
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
  if ( msgTransport.isEmpty() )
    msgTransport = mCurrentMsg->headerField( "X-KMail-Transport" );

  if ( msgTransport.isEmpty() )
    msgTransport = TransportManager::self()->defaultTransportName();

  if ( !mTransportJob ) {
    mMethodStr = msgTransport;
    mTransportJob = TransportManager::self()->createTransportJob( msgTransport );
    if ( !mTransportJob ) {
      KMessageBox::error( 0, i18n( "Transport '%1' is invalid.", mMethodStr ),
                          i18n( "Sending failed" ) );
      mProgressItem->cancel();
      mProgressItem->setComplete();
      cleanup();
      return;
    }

    if ( mTransportJob->transport()->encryption() == Transport::EnumEncryption::TLS ||
         mTransportJob->transport()->encryption() == Transport::EnumEncryption::SSL ) {
      mProgressItem->setUsesCrypto( true );
    } else if ( !mCustomTransport.isEmpty() ) {
      int result = KMessageBox::warningContinueCancel(
        0,
        i18n( "You have chosen to send all queued email using an unencrypted transport, do you want to continue? "),
        i18n( "Security Warning" ),
        KGuiItem( i18n( "Send Unencrypted" ) ),
        KStandardGuiItem::cancel(),
        "useCustomTransportWithoutAsking", false );

      if ( result == KMessageBox::Cancel ) {
        mProgressItem->cancel();
        mProgressItem->setComplete();
        slotAbortSend();
        cleanup();
        return;
      }
    }

    setStatusMsg( i18nc("%3: subject of message","Sending message %1 of %2: %3",
                  mSentMessages+mFailedMessages+1, mTotalMessages,
                  mCurrentMsg->subject()) );
    QStringList to, cc, bcc;
    QString sender;
    extractSenderToCCAndBcc( mCurrentMsg, &sender, &to, &cc, &bcc );

    // MDNs are required to have an empty envelope from as per RFC2298.
    if ( messageIsDispositionNotificationReport( mCurrentMsg ) && GlobalSettings::self()->sendMDNsWithEmptySender() )
      sender = "<>";

    const QByteArray message = mCurrentMsg->asSendableString();
    if ( sender.isEmpty() ) {
      if ( mCurrentMsg )
        mCurrentMsg->setTransferInProgress( false );
      if ( mOutboxFolder )
        mOutboxFolder->unGetMsg( mFailedMessages );
      mCurrentMsg = 0;
      cleanup();
      setStatusMsg(i18n("Failed to send (some) queued messages."));
      return;
    }

    mTransportJob->setSender( sender );
    mTransportJob->setTo( to );
    mTransportJob->setCc( cc );
    mTransportJob->setBcc( bcc );
    mTransportJob->setData( message );

    connect( mTransportJob, SIGNAL(result(KJob*)), SLOT(slotResult(KJob*)) );
    connect( mTransportJob, SIGNAL(processedSize(KJob *, qulonglong)),
             SLOT( slotProcessedSize(KJob *, qulonglong)) );
    mSendProcStarted = true;
    mTransportJob->start();
  }
}


//-----------------------------------------------------------------------------
void KMSender::cleanup( void )
{
  kDebug(5006) ;
  if ( mTransportJob && mSendProcStarted ) {
    mTransportJob->kill();
  }
  mTransportJob = 0;
  mSendProcStarted = false;
  if ( mSendInProgress ) {
    KGlobal::deref();
  }
  mSendInProgress = false;
  if ( mCurrentMsg ) {
    mCurrentMsg->setTransferInProgress( false );
    mCurrentMsg = 0;
  }
  if ( mSentFolder ) {
    mSentFolder->close( "dosendsent" );
    mSentFolder = 0;
  }
  if ( mOutboxFolder ) {
    disconnect( mOutboxFolder, SIGNAL(msgAdded(int)),
                this, SLOT(outboxMsgAdded(int)) );
    mOutboxFolder->close( "dosendoutbox" );
    if ( mOutboxFolder->count( true ) == 0 ) {
      mOutboxFolder->expunge();
    } else if ( mOutboxFolder->needsCompacting() ) {
      mOutboxFolder->compact( KMFolder::CompactSilentlyNow );
    }
    mOutboxFolder = 0;
  }

  mSendAborted = false;
  mSentMessages = 0;
  mFailedMessages = 0;
  mSentBytes = 0;
  if ( mProgressItem ) {
    mProgressItem->setComplete();
  }
  mProgressItem = 0;
  kmkernel->filterMgr()->deref();
}

//-----------------------------------------------------------------------------
void KMSender::slotAbortSend()
{
  mSendAborted = true;
  if ( mTransportJob ) {
    mTransportJob->kill( KJob::EmitResult );
  }
}

//-----------------------------------------------------------------------------
void KMSender::slotResult( KJob *job )
{
  assert( mTransportJob == job );
  mTransportJob = 0;
  mSendProcStarted = false;

  QString msg;
  QString errString = job->errorString();

  if ( mSendAborted ) {
    // sending of message aborted
    if ( mCurrentMsg ) {
      mCurrentMsg->setTransferInProgress( false );
      if ( mOutboxFolder ) {
        mOutboxFolder->unGetMsg( mFailedMessages );
      }
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
    if ( job->error() ) {
      if ( mCurrentMsg ) {
        mCurrentMsg->setTransferInProgress( false );
      }
      if ( mOutboxFolder ) {
        mOutboxFolder->unGetMsg( mFailedMessages );
      }
      mCurrentMsg = 0;
      mFailedMessages++;

      // Sending of message failed.
      if ( !errString.isEmpty() ) {
        int res = KMessageBox::Yes;
        if ( mSentMessages+mFailedMessages != mTotalMessages ) {
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
void KMSender::setStatusByLink( const KMMessage *aMsg )
{
  int n = 0;
  while ( 1 ) {
    ulong msn;
    MessageStatus status;
    aMsg->getLink( n, &msn, status );
    if ( !msn || status.isOfUnknownStatus() ) {
      break;
    }
    n++;

    KMFolder *folder = 0;
    int index = -1;
    KMMsgDict::instance()->getLocation( msn, &folder, &index );
    if ( folder && index != -1 ) {
      KMFolderOpener openFolder( folder, "setstatus" );
      if ( status.isDeleted() ) {
        // Move the message to the trash folder
        KMDeleteMsgCommand *cmd =
          new KMDeleteMsgCommand( folder, folder->getMsg( index ) );
        cmd->start();
      } else {
        folder->setStatus( index, status );
      }
    } else {
      kWarning(5006) <<"Cannot update linked message, it could not be found!";
    }
  }
}

#include "kmsender.moc"
