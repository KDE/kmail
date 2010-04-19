/*
 * This file is part of KMail.
 *
 * Copyright (c) 2010 KDAB
 *
 * Author: Tobias Koenig <tokoe@kde.org>
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

#include "emailaddressresolvejob.h"

#include "aliasesexpandjob.h"
#include "globalsettings.h"

#include <KPIMUtils/Email>

EmailAddressResolveJob::EmailAddressResolveJob( MailTransport::MessageQueueJob *job, KMime::Message::Ptr message, const Message::InfoPart *infoPart, QObject *parent )
  : KJob( parent ), mQueueJob( job ), mMessage( message ), mInfoPart( infoPart ), mEncrypted( false ), mJobCount( 0 )
{
}

EmailAddressResolveJob::~EmailAddressResolveJob()
{
}

void EmailAddressResolveJob::start()
{
  {
    AliasesExpandJob *job = new AliasesExpandJob( mInfoPart->from(), GlobalSettings::defaultDomain(), this );
    job->setProperty( "id", "infoPartFrom" );
    connect( job, SIGNAL( result( KJob* ) ), SLOT( slotAliasExpansionDone( KJob* ) ) );
    job->start();

    mJobCount++;
  }
  {
    if ( !mMessage->bcc()->addresses().isEmpty() ) {
      QStringList bcc;
      foreach( const QByteArray &address, mMessage->bcc()->addresses() )
        bcc << QString::fromUtf8( address );

      AliasesExpandJob *job = new AliasesExpandJob( bcc.join( QLatin1String( ", " ) ), GlobalSettings::defaultDomain(), this );
      job->setProperty( "id", "messageBcc" );
      connect( job, SIGNAL( result( KJob* ) ), SLOT( slotAliasExpansionDone( KJob* ) ) );
      job->start();

      mJobCount++;
    }
  }
  {
    AliasesExpandJob *job = new AliasesExpandJob( mInfoPart->to().join( QLatin1String( ", " ) ), GlobalSettings::defaultDomain(), this );
    job->setProperty( "id", "infoPartTo" );
    connect( job, SIGNAL( result( KJob* ) ), SLOT( slotAliasExpansionDone( KJob* ) ) );
    job->start();

    mJobCount++;
  }
  {
    AliasesExpandJob *job = new AliasesExpandJob( mInfoPart->cc().join( QLatin1String( ", " ) ), GlobalSettings::defaultDomain(), this );
    job->setProperty( "id", "infoPartCc" );
    connect( job, SIGNAL( result( KJob* ) ), SLOT( slotAliasExpansionDone( KJob* ) ) );
    job->start();

    mJobCount++;
  }
  {
    AliasesExpandJob *job = new AliasesExpandJob( mInfoPart->bcc().join( QLatin1String( ", " ) ), GlobalSettings::defaultDomain(), this );
    job->setProperty( "id", "infoPartBcc" );
    connect( job, SIGNAL( result( KJob* ) ), SLOT( slotAliasExpansionDone( KJob* ) ) );
    job->start();

    mJobCount++;
  }
}

void EmailAddressResolveJob::setEncrypted( bool encrypted )
{
  mEncrypted = encrypted;
}

void EmailAddressResolveJob::slotAliasExpansionDone( KJob *job )
{
  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
    emitResult();
    return;
  }

  const AliasesExpandJob *expandJob = qobject_cast<AliasesExpandJob*>( job );
  mResultMap.insert( expandJob->property( "id" ).toString(), expandJob->addresses() );

  mJobCount--;
  if ( mJobCount == 0 )
    finishExpansion();
}

void EmailAddressResolveJob::finishExpansion()
{
  mQueueJob->addressAttribute().setFrom( mResultMap.value( "infoPartFrom" ).toString() );

  if ( mEncrypted && !mInfoPart->bcc().isEmpty() ) { // have to deal with multiple message contents
    // if the bcc isn't empty, then we send it to the bcc because this is the bcc-only encrypted body
    if ( !mMessage->bcc()->addresses().isEmpty() ) {
      mQueueJob->addressAttribute().setTo( KPIMUtils::splitAddressList( mResultMap.value( "messageBcc" ).toString() ) );
    } else {
      // the main mail in the encrypted set, just don't set the bccs here
      mQueueJob->addressAttribute().setTo( KPIMUtils::splitAddressList( mResultMap.value( "infoPartTo" ).toString() ) );
      mQueueJob->addressAttribute().setCc( KPIMUtils::splitAddressList( mResultMap.value( "infoPartCc" ).toString() ) );
    }
  } else {
    // continue as normal
    mQueueJob->addressAttribute().setTo( KPIMUtils::splitAddressList( mResultMap.value( "infoPartTo" ).toString() ) );
    mQueueJob->addressAttribute().setCc( KPIMUtils::splitAddressList( mResultMap.value( "infoPartCc" ).toString() ) );
    mQueueJob->addressAttribute().setBcc( KPIMUtils::splitAddressList( mResultMap.value( "infoPartBcc" ).toString() ) );
  }

  emitResult();
}

MailTransport::MessageQueueJob* EmailAddressResolveJob::queueJob() const
{
  return mQueueJob;
}

#include "emailaddressresolvejob.moc"
