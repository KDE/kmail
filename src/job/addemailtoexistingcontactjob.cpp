/*
   Copyright (C) 2013-2020 Laurent Montel <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "addemailtoexistingcontactjob.h"
#include "kmail_debug.h"
#include <Libkdepim/BroadcastStatus>

#include <AkonadiCore/ItemModifyJob>

#include <KContacts/Addressee>

#include <KLocalizedString>

AddEmailToExistingContactJob::AddEmailToExistingContactJob(const Akonadi::Item &item, const QString &email, QObject *parent)
    : KJob(parent)
    , mItem(item)
{
    QString name;
    KContacts::Addressee::parseEmailAddress(email, name, mEmail);
}

AddEmailToExistingContactJob::~AddEmailToExistingContactJob()
{
}

void AddEmailToExistingContactJob::start()
{
    if (mItem.hasPayload<KContacts::Addressee>()) {
        KContacts::Addressee address = mItem.payload<KContacts::Addressee>();
        QStringList emails = address.emails();
        if (emails.contains(mEmail)) {
            emitResult();
        } else {
            emails.append(mEmail);
            address.setEmails(emails);
            mItem.setPayload<KContacts::Addressee>(address);
            Akonadi::ItemModifyJob *job = new Akonadi::ItemModifyJob(mItem);
            connect(job, &Akonadi::ItemModifyJob::result, this, &AddEmailToExistingContactJob::slotAddEmailDone);
        }
    } else {
        qCDebug(KMAIL_LOG) << " not a KContacts::Addressee item ";
        //TODO add error
        emitResult();
    }
}

void AddEmailToExistingContactJob::slotAddEmailDone(KJob *job)
{
    if (job->error()) {
        setError(job->error());
        setErrorText(job->errorText());
    } else {
        KPIM::BroadcastStatus::instance()->setStatusMsg(i18n("Email added successfully."));
    }
    emitResult();
}
