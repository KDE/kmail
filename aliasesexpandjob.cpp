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

#include "aliasesexpandjob.h"
#include "aliasesexpandjob_p.h"

#include <akonadi/contact/contactgroupexpandjob.h>
#include <akonadi/contact/contactgroupsearchjob.h>
#include <akonadi/contact/contactsearchjob.h>

#include <KPIMUtils/Email>

#include <messagecore/stringutil.h>

DistributionListExpandJob::DistributionListExpandJob( const QString &name, QObject *parent )
  : KJob( parent ), mListName( name ), mIsEmpty( false )
{
}

DistributionListExpandJob::~DistributionListExpandJob()
{
}

void DistributionListExpandJob::start()
{
  if ( mListName.isEmpty() ) {
    emitResult();
    return;
  }

  Akonadi::ContactGroupSearchJob *job = new Akonadi::ContactGroupSearchJob( this );
  job->setQuery( Akonadi::ContactGroupSearchJob::Name, mListName );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( slotSearchDone( KJob* ) ) );
}

QString DistributionListExpandJob::addresses() const
{
  return mEmailAddresses.join( QLatin1String( ", " ) );
}

bool DistributionListExpandJob::isEmpty() const
{
  return mIsEmpty;
}

void DistributionListExpandJob::slotSearchDone( KJob *job )
{
  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
    emitResult();
    return;
  }

  const Akonadi::ContactGroupSearchJob *searchJob = qobject_cast<Akonadi::ContactGroupSearchJob*>( job );

  const KABC::ContactGroup::List groups = searchJob->contactGroups();
  if ( groups.isEmpty() ) {
    emitResult();
    return;
  }

  Akonadi::ContactGroupExpandJob *expandJob = new Akonadi::ContactGroupExpandJob( groups.first() );
  connect( expandJob, SIGNAL( result( KJob* ) ), SLOT( slotExpansionDone( KJob* ) ) );
  expandJob->start();
}

void DistributionListExpandJob::slotExpansionDone( KJob *job )
{
  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
    emitResult();
    return;
  }

  const Akonadi::ContactGroupExpandJob *expandJob = qobject_cast<Akonadi::ContactGroupExpandJob*>( job );

  const KABC::Addressee::List contacts = expandJob->contacts();

  foreach ( const KABC::Addressee &contact, contacts )
    mEmailAddresses << contact.fullEmail();

  mIsEmpty = mEmailAddresses.isEmpty();

  emitResult();
}


AliasesExpandJob::AliasesExpandJob( const QString &recipients, const QString &defaultDomain, QObject *parent )
  : KJob( parent ),
    mRecipients( KPIMUtils::splitAddressList( recipients ) ),
    mDefaultDomain( defaultDomain ),
    mDistributionListExpansionJobs( 0 ),
    mNicknameExpansionJobs( 0 )
{
}

AliasesExpandJob::~AliasesExpandJob()
{
}

void AliasesExpandJob::start()
{
  // At first we try to expand the recipient to a distribution list
  // or nick name and save the results in a map for later lookup
  foreach ( const QString &recipient, mRecipients ) {
    // check for distribution list
    DistributionListExpandJob *expandJob = new DistributionListExpandJob( recipient, this );
    expandJob->setProperty( "recipient", recipient );
    connect( expandJob, SIGNAL( result( KJob* ) ), SLOT( slotDistributionListExpansionDone( KJob* ) ) );
    mDistributionListExpansionJobs++;
    expandJob->start();

    // check for nick name
    Akonadi::ContactSearchJob *searchJob = new Akonadi::ContactSearchJob( this );
    searchJob->setProperty( "recipient", recipient );
    searchJob->setQuery( Akonadi::ContactSearchJob::NickName, recipient.toLower() );
    connect( searchJob, SIGNAL( result( KJob* ) ), SLOT( slotNicknameExpansionDone( KJob* ) ) );
    mNicknameExpansionJobs++;
    searchJob->start();
  }

  if ( mDistributionListExpansionJobs == 0 && mNicknameExpansionJobs == 0 )
    emitResult();
}

QString AliasesExpandJob::addresses() const
{
  return mEmailAddresses;
}

QStringList AliasesExpandJob::emptyDistributionLists() const
{
  return mEmptyDistributionLists;
}

void AliasesExpandJob::slotDistributionListExpansionDone( KJob *job )
{
  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
    emitResult();
    return;
  }

  const DistributionListExpandJob *expandJob = qobject_cast<DistributionListExpandJob*>( job );
  const QString recipient = expandJob->property( "recipient" ).toString();

  DistributionListExpansionResult result;
  result.addresses = expandJob->addresses();
  result.isEmpty = expandJob->isEmpty();

  mDistListExpansionResults.insert( recipient, result );

  mDistributionListExpansionJobs--;
  if ( mDistributionListExpansionJobs == 0 && mNicknameExpansionJobs == 0 )
    finishExpansion();
}

void AliasesExpandJob::slotNicknameExpansionDone( KJob *job )
{
  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
    emitResult();
    return;
  }

  const Akonadi::ContactSearchJob *searchJob = qobject_cast<Akonadi::ContactSearchJob*>( job );
  const KABC::Addressee::List contacts = searchJob->contacts();
  const QString recipient = searchJob->property( "recipient" ).toString();

  foreach ( const KABC::Addressee &contact, contacts ) {
    if ( contact.nickName().toLower() == recipient.toLower() ) {
      mNicknameExpansionResults.insert( recipient, contact.fullEmail() );
      break;
    }
  }

  mNicknameExpansionJobs--;
  if ( mDistributionListExpansionJobs == 0 && mNicknameExpansionJobs == 0 )
    finishExpansion();
}

void AliasesExpandJob::finishExpansion()
{
  foreach ( const QString &recipient, mRecipients ) {
    if ( !mEmailAddresses.isEmpty() )
      mEmailAddresses += QLatin1String( ", " );

    const QString receiver = recipient.trimmed();

    // take prefetched expand distribution list results
    const DistributionListExpansionResult result = mDistListExpansionResults.value( recipient );

    if ( result.isEmpty ) {
      mEmailAddresses += receiver;
      mEmptyDistributionLists << receiver;
      continue;
    }

    if ( !result.addresses.isEmpty() ) {
      mEmailAddresses += result.addresses;
      continue;
    }

    // take prefetched expand nick name results
    if ( !mNicknameExpansionResults.value( recipient ).isEmpty() ) {
      mEmailAddresses += mNicknameExpansionResults.value( recipient );
      continue;
    }

    // check whether the address is missing the domain part
    QByteArray displayName, addrSpec, comment;
    KPIMUtils::splitAddress( receiver.toLatin1(), displayName, addrSpec, comment );
    if ( !addrSpec.contains( '@' ) ) {
      if ( !mDefaultDomain.isEmpty() )
        mEmailAddresses += KPIMUtils::normalizedAddress( displayName, addrSpec + QLatin1Char( '@' ) +
                                                            mDefaultDomain, comment );
      else
        mEmailAddresses += MessageCore::StringUtil::guessEmailAddressFromLoginName( addrSpec );
    } else
      mEmailAddresses += receiver;
  }

  emitResult();
}

#include "aliasesexpandjob.moc"
#include "aliasesexpandjob_p.moc"
