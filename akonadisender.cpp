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

#include "kmfolder.h"
#include "kmkernel.h"
#include "kmmessage.h"
#include "progressmanager.h"

#include <KMessageBox>
#include <KLocale>
#include <KDebug>
#include <KConfig>
#include <KConfigGroup>

//#include <mimelib/mediatyp.h>
//#include <mimelib/enum.h>
//#include <mimelib/param.h>

#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>

#include <mailtransport/transport.h>
#include <mailtransport/transportmanager.h>

#include <outboxinterface/dispatcherinterface.h>
#include <outboxinterface/messagequeuejob.h>

static const QString SENDER_GROUP( "sending mail" );

using namespace KMail;
using namespace KMime;
using namespace KMime::Types;
using namespace KPIM;
using namespace MailTransport;
using namespace OutboxInterface;


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

static void extractSenderToCCAndBcc( KMMessage *aMsg, QString *sender, QStringList *to, QStringList *cc, QStringList *bcc )
{
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

bool AkonadiSender::doSend( KMMessage *aMsg, short sendNow  )
{
  KMFolder * const outbox = kmkernel->outboxFolder();
  const KMFolderOpener openOutbox( outbox, "AkonadiSender" );

  Q_ASSERT( aMsg );
  aMsg->setStatus( MessageStatus::statusQueued() );
  if ( const int err = openOutbox.folder()->addMsg( aMsg ) ) {
    Q_UNUSED( err );
    KMessageBox::information( 0, i18n( "Cannot add message to outbox folder" ) );
    return false;
  }

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
  mCustomTransport = customTransport;

  // Watch progress of the MDA.
  Q_ASSERT( DispatcherInterface::self()->isReady() );
  mProgressItem = ProgressManager::createProgressItem( 0,
      DispatcherInterface::self()->dispatcherInstance(),
      "Sender",
      i18n( "Sending messages" ),
      i18n( "Initiating sending process..." ),
      true );
  kDebug() << "Created ProgressItem" << mProgressItem;

  // Traverse outbox.
  KMFolder *outbox = kmkernel->outboxFolder();
  outbox->open( "dosendoutbox" );
  while( outbox->count() > 0 ) {
    //outbox->take( 0 ); // does this do what I think it does? Why does it return zero?
    outbox->unGetMsg( 0 );
    KMMessage *msg = outbox->getMsg( 0 );
    queueMessage( msg );
    outbox->removeMsg( 0 );
  }

  return true;
}

void AkonadiSender::queueMessage( KMMessage *msg )
{
  Q_ASSERT( msg );
  msg->setTransferInProgress( true );

  // Translate message to KMime.
  MessageQueueJob *qjob = new MessageQueueJob( this );
  Message::Ptr message = Message::Ptr( new Message );
  message->setContent( msg->asSendableString() );
  kDebug() << "KMime::Message: \n[\n" << message->encodedContent().left( 1000 ) << "\n]\n";
  qjob->setMessage( message );

  // Get transport.
  QString transportName = mCustomTransport;
  kDebug() << "Custom transportName:" << mCustomTransport;
  if( transportName.isEmpty() ) {
    transportName = msg->headerField( "X-KMail-Transport" );
    kDebug() << "TransportName from headers:" << transportName;
  }
  if( transportName.isEmpty() ) {
    transportName = TransportManager::self()->defaultTransportName();
    kDebug() << "Default transport" << TransportManager::self()->defaultTransportName();
  }
  Transport *transport = TransportManager::self()->transportByName( transportName );
  Q_ASSERT( transport );
  kDebug() << "Using transport (" << transportName << "," << transport->id() << ")";
  qjob->setTransportId( transport->id() );

  // Get addresses.
  QStringList to, cc, bcc;
  QString from;
  extractSenderToCCAndBcc( msg, &from, &to, &cc, &bcc );
  qjob->setFrom( from );
  qjob->setTo( to );
  qjob->setCc( cc );
  qjob->setBcc( bcc );
  msg->setTransferInProgress( false ); // we are done with the KMMessage

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
