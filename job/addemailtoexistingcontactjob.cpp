/*
  Copyright (c) 2013 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "addemailtoexistingcontactjob.h"

#include <Akonadi/ItemModifyJob>

#include <KABC/Addressee>

AddEmailToExistingContactJob::AddEmailToExistingContactJob(const Akonadi::Item &item, const QString &email, QObject *parent)
    : KJob(parent),
      mEmail(email),
      mItem(item)
{
}

AddEmailToExistingContactJob::~AddEmailToExistingContactJob()
{

}

void AddEmailToExistingContactJob::start()
{
    if (mItem.hasPayload<KABC::Addressee>()) {
        KABC::Addressee address = mItem.payload<KABC::Addressee>();
        QStringList emails = address.emails();
        if (emails.contains(mEmail)) {
            Q_EMIT emitResult();
        } else {
            emails.append(mEmail);
            address.setEmails(emails);
            mItem.setPayload<KABC::Addressee>(address);
            Akonadi::ItemModifyJob *job = new Akonadi::ItemModifyJob( mItem );
            connect( job, SIGNAL(result(KJob*)), SLOT(slotAddEmailDone(KJob*)) );
        }
    } else {
        qDebug()<<" not a KABC::Addressee item ";
        //TODO add error
        Q_EMIT emitResult();
    }
}

void AddEmailToExistingContactJob::slotAddEmailDone(KJob *job)
{
    if ( job->error() ) {
        setError( job->error() );
        setErrorText( job->errorText() );
    }
    Q_EMIT emitResult();
}

#include "moc_addemailtoexistingcontactjob.cpp"
