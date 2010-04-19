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

#include "addressvalidationjob.h"
#include "aliasesexpandjob.h"

#include "globalsettings.h"

#include <klocale.h>
#include <kmessagebox.h>

#include <KPIMUtils/Email>

#include <messagecore/stringutil.h>

class AddressValidationJob::Private
{
  public:
    Private( AddressValidationJob *qq, const QString &emailAddresses, QWidget *parentWidget )
      : q( qq ), mEmailAddresses( emailAddresses ), mIsValid( false ),  mParentWidget( parentWidget )
    {
    }

    void slotAliasExpansionDone( KJob* );

    AddressValidationJob *q;
    QString mEmailAddresses;
    bool mIsValid;
    QWidget *mParentWidget;
};

void AddressValidationJob::Private::slotAliasExpansionDone( KJob *job )
{
  if ( job->error() ) {
    q->setError( job->error() );
    q->setErrorText( job->errorText() );
    mIsValid = false;
    q->emitResult();
    return;
  }

  const AliasesExpandJob *expandJob = qobject_cast<AliasesExpandJob*>( job );
  const QStringList emptyDistributionLists = expandJob->emptyDistributionLists();

  QString brokenAddress;

  const KPIMUtils::EmailParseResult errorCode = KPIMUtils::isValidAddressList( expandJob->addresses(), brokenAddress );
  if ( !emptyDistributionLists.isEmpty() ) {
    const QString errorMsg = i18n( "Distribution list \"%1\" is empty, it cannot be used.",
                                   emptyDistributionLists.join( ", " ) );
    KMessageBox::sorry( mParentWidget, errorMsg, i18n( "Invalid Email Address" ) );
    mIsValid = false;
  } else {
    if ( !( errorCode == KPIMUtils::AddressOk ||
            errorCode == KPIMUtils::AddressEmpty ) ) {
      const QString errorMsg( "<qt><p><b>" + brokenAddress +
                              "</b></p><p>" +
                              KPIMUtils::emailParseResultToString( errorCode ) +
                              "</p></qt>" );
      KMessageBox::sorry( mParentWidget, errorMsg, i18n( "Invalid Email Address" ) );
      mIsValid = false;
    }
  }

  mIsValid = true;

  q->emitResult();
}

AddressValidationJob::AddressValidationJob( const QString &emailAddresses, QWidget *parentWidget, QObject *parent )
  : KJob( parent ), d( new Private( this, emailAddresses, parentWidget ) )
{
}

AddressValidationJob::~AddressValidationJob()
{
  delete d;
}

void AddressValidationJob::start()
{
  AliasesExpandJob *job = new AliasesExpandJob( d->mEmailAddresses, GlobalSettings::defaultDomain(), this );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( slotAliasExpansionDone( KJob* ) ) );
  job->start();
}


bool AddressValidationJob::isValid() const
{
  return d->mIsValid;
}

#include "addressvalidationjob.moc"
