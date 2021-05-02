/*
   SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "addemailtoexistingcontactjob.h"
#include "kmail_debug.h"
#include <PimCommon/BroadcastStatus>

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

AddEmailToExistingContactJob::~AddEmailToExistingContactJob() = default;

void AddEmailToExistingContactJob::start()
{
    if (mItem.hasPayload<KContacts::Addressee>()) {
        auto address = mItem.payload<KContacts::Addressee>();
        QStringList emails = address.emails();
        if (emails.contains(mEmail)) {
            emitResult();
        } else {
            emails.append(mEmail);
            address.setEmails(emails);
            mItem.setPayload<KContacts::Addressee>(address);
            auto job = new Akonadi::ItemModifyJob(mItem);
            connect(job, &Akonadi::ItemModifyJob::result, this, &AddEmailToExistingContactJob::slotAddEmailDone);
        }
    } else {
        qCDebug(KMAIL_LOG) << " not a KContacts::Addressee item ";
        // TODO add error
        emitResult();
    }
}

void AddEmailToExistingContactJob::slotAddEmailDone(KJob *job)
{
    if (job->error()) {
        setError(job->error());
        setErrorText(job->errorText());
    } else {
        PimCommon::BroadcastStatus::instance()->setStatusMsg(i18n("Email added successfully."));
    }
    emitResult();
}
