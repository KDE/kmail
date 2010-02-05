/*
 * This file is part of KMail.
 * Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>
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


#define REALLY_WANT_AKONADISENDER
#include "akonadisender.h"
#undef REALLY_WANT_AKONADISENDER

#include "kmkernel.h"
#include "progressmanager.h"
#include "messagehelper.h"

#include <KMessageBox>
#include <KLocale>
#include <KDebug>
#include <KConfig>
#include <KConfigGroup>

#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>

#include <mailtransport/dispatcherinterface.h>
#include <mailtransport/messagequeuejob.h>
#include <mailtransport/transport.h>
#include <mailtransport/transportmanager.h>

static const QString SENDER_GROUP( "sending mail" );

using namespace KMail;
using namespace KMime;
using namespace KMime::Types;
using namespace KPIM;
using namespace MailTransport;


static QStringList addrSpecListToStringList( const AddrSpecList &l, bool allowEmpty = false )
{
  QStringList result;
  for ( AddrSpecList::const_iterator it = l.begin(), end = l.end() ; it != end ; ++it ) {
    const QString s = (*it).asString();
    if ( allowEmpty || !s.isEmpty() )
      result.push_back( s );
  }
  return result;
}


static void extractSenderToCCAndBcc( const KMime::Message::Ptr &aMsg, QString &sender, QStringList &to, QStringList &cc, QStringList &bcc )
{
  sender = aMsg->sender()->asUnicodeString();
  if( aMsg->headerByType("X-KMail-Recipients") ) {
    // extended BCC handling to prevent TOs and CCs from seeing
    // BBC information by looking at source of an OpenPGP encrypted mail
    to = addrSpecListToStringList( MessageHelper::extractAddrSpecs( aMsg, "X-KMail-Recipients" ) );
    aMsg->removeHeader( "X-KMail-Recipients" );
  } else {
    to = addrSpecListToStringList( MessageHelper::extractAddrSpecs( aMsg, "To" ) );
    cc = addrSpecListToStringList( MessageHelper::extractAddrSpecs( aMsg, "Cc" ) );
    bcc = addrSpecListToStringList( MessageHelper::extractAddrSpecs( aMsg, "Bcc" ) );
  }
}



AkonadiSender::AkonadiSender()
{
  readConfig();
  mProgressItem = 0;
}

AkonadiSender::~AkonadiSender()
{
  writeConfig(false);
}

void AkonadiSender::readConfig()
{
  QString str;
  KConfigGroup config( KMKernel::config(), SENDER_GROUP );

  mSendImmediate = config.readEntry( "Immediate", true );
  mSendQuotedPrintable = config.readEntry( "Quoted-Printable", true );
}

void AkonadiSender::writeConfig( bool aWithSync )
{
  KConfigGroup config( KMKernel::config(), SENDER_GROUP );

  config.writeEntry( "Immediate", mSendImmediate );
  config.writeEntry( "Quoted-Printable", mSendQuotedPrintable );

  if ( aWithSync ) {
    config.sync();
  }
}

void AkonadiSender::setSendImmediate( bool aSendImmediate )
{
  mSendImmediate = aSendImmediate;
}

void AkonadiSender::setSendQuotedPrintable( bool aSendQuotedPrintable )
{
  mSendQuotedPrintable = aSendQuotedPrintable;
}

bool AkonadiSender::doSend( const KMime::Message::Ptr &aMsg, short sendNow  )
{
#if 0
  KMFolder * const outbox = kmkernel->outboxFolder();
  const KMFolderOpener openOutbox( outbox, "AkonadiSender" );

  Q_ASSERT( aMsg );
  //TODO Port to Akonadi
  aMsg->setStatus( MessageStatus::statusQueued() );
  if ( const int err = openOutbox.folder()->addMsg( aMsg ) ) {
    Q_UNUSED( err );
    KMessageBox::information( 0, i18n( "Cannot add message to outbox folder" ) );
    return false;
  }
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif

  if( sendNow == -1 ) {
    sendNow = mSendImmediate; // -1 == use default setting
  }
  if ( !sendNow ) {
    return true;
  } else {
    return sendQueued();
  }
}

bool AkonadiSender::doSendQueued( const QString &customTransport )
{
  //TODO: Fix sending using a different transport
  mCustomTransport = customTransport;

  // Watch progress of the MDA.
  mProgressItem = ProgressManager::createProgressItem( 0,
      DispatcherInterface().dispatcherInstance(),
      "Sender",
      i18n( "Sending messages" ),
      i18n( "Initiating sending process..." ),
      true );
  kDebug() << "Created ProgressItem" << mProgressItem;

  DispatcherInterface *dispatcher = new DispatcherInterface();
  if( mCustomTransport.isEmpty() ) {
    dispatcher->dispatchManually();
  } else {
    dispatcher->dispatchManualTransport( TransportManager::self()->transportByName( mCustomTransport )->id() );
  }
  delete dispatcher;
  return true;
}

void AkonadiSender::queueMessage( const KMime::Message::Ptr &message )
{
  Q_ASSERT( message );
  // Translate message to KMime.
  KMime::Message::Ptr messagePtr = KMime::Message::Ptr( new KMime::Message );
  messagePtr->setContent( MessageHelper::asSendableString( message ) );
  kDebug() << "KMime::Message: \n[\n" << messagePtr->encodedContent().left( 1000 ) << "\n]\n";

  MessageQueueJob *qjob = new MessageQueueJob( this );
  if ( message->headerByType( "X-KMail-Fcc" ) ) {
    qjob->sentBehaviourAttribute().setSentBehaviour(
                                 SentBehaviourAttribute::MoveToCollection );
    const int sentCollectionId = message->headerByType( "X-KMail-Fcc" )->asUnicodeString().toInt();
    qjob->sentBehaviourAttribute().setMoveToCollection(
                                 Akonadi::Collection( sentCollectionId ) );
  } else {
    qjob->sentBehaviourAttribute().setSentBehaviour(
           MailTransport::SentBehaviourAttribute::MoveToDefaultSentCollection );
  }
  qjob->setMessage( KMime::Message::Ptr(messagePtr) );

  // Get transport.
  QString transportName = mCustomTransport;
  kDebug() << "Custom transportName:" << mCustomTransport;
  if( transportName.isEmpty() ) {
    transportName = message->headerByType( "X-KMail-Transport" ) ? message->headerByType( "X-KMail-Transport" )->asUnicodeString() : "";
    kDebug() << "TransportName from headers:" << transportName;
  }
  if( transportName.isEmpty() ) {
    transportName = TransportManager::self()->defaultTransportName();
    kDebug() << "Default transport" << TransportManager::self()->defaultTransportName();
  }
  Transport *transport = TransportManager::self()->transportByName( transportName );
  Q_ASSERT( transport );
  kDebug() << "Using transport (" << transportName << "," << transport->id() << ")";
  qjob->transportAttribute().setTransportId( transport->id() );

  // Get addresses.
  QStringList to, cc, bcc;
  QString from;
  extractSenderToCCAndBcc( message, from, to, cc, bcc );
  qjob->addressAttribute().setFrom( from );
  qjob->addressAttribute().setTo( to );
  qjob->addressAttribute().setCc( cc );
  qjob->addressAttribute().setBcc( bcc );

  // Default sent-mail collection for now.
  // Send immediately (queuing is done by KMail's outbox for now...)

  // Queue the message.
  connect( qjob, SIGNAL(result(KJob*)), this, SLOT(queueJobResult(KJob*)) );
  mPendingJobs.insert( qjob );
  qjob->start();
  kDebug() << "QueueJob started.";

  // TODO potential problem:
  // The MDA finishes sending a message before I queue the next one, and
  // thinking it is finished, the progress item deletes itself.
  // Turn the MDA offline until everything is queued?
}

void AkonadiSender::queueJobResult( KJob *job )
{
  Q_ASSERT( mPendingJobs.contains( job ) );
  mPendingJobs.remove( job );

  if( job->error() ) {
    kDebug() << "QueueJob failed with error" << job->errorString();
  } else {
    kDebug() << "QueueJob success.";
  }
}

#include "akonadisender.moc"
