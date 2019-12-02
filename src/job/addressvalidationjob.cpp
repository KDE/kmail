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
#include <MessageComposer/AliasesExpandJob>
using MessageComposer::AliasesExpandJob;

#include "messagecomposer/messagecomposersettings.h"

#include <KLocalizedString>
#include <KMessageBox>

#include <KEmailAddress>

AddressValidationJob::AddressValidationJob(const QString &emailAddresses, QWidget *parentWidget, QObject *parent)
    : KJob(parent)
    , mEmailAddresses(emailAddresses)
    , mParentWidget(parentWidget)
{
}

AddressValidationJob::~AddressValidationJob()
{
}

void AddressValidationJob::setDefaultDomain(const QString &domainName)
{
    mDomainDefaultName = domainName;
}

void AddressValidationJob::start()
{
    AliasesExpandJob *job = new AliasesExpandJob(mEmailAddresses, mDomainDefaultName, this);
    connect(job, &AliasesExpandJob::result, this, &AddressValidationJob::slotAliasExpansionDone);
    job->start();
}

bool AddressValidationJob::isValid() const
{
    return mIsValid;
}

void AddressValidationJob::slotAliasExpansionDone(KJob *job)
{
    mIsValid = true;

    if (job->error()) {
        setError(job->error());
        setErrorText(job->errorText());
        mIsValid = false;
        emitResult();
        return;
    }

    const AliasesExpandJob *expandJob = qobject_cast<AliasesExpandJob *>(job);
    const QStringList emptyDistributionLists = expandJob->emptyDistributionLists();

    QString brokenAddress;

    const KEmailAddress::EmailParseResult errorCode = KEmailAddress::isValidAddressList(expandJob->addresses(), brokenAddress);
    if (!emptyDistributionLists.isEmpty()) {
        QString errorMsg;
        const int numberOfDistributionList(emptyDistributionLists.count());
        QString listOfDistributionList;
        for (int i = 0; i < numberOfDistributionList; ++i) {
            if (i != 0) {
                listOfDistributionList.append(QLatin1String(", "));
            }
            listOfDistributionList.append(QStringLiteral("\"%1\"").arg(emptyDistributionLists.at(i)));
        }
        errorMsg = i18np("Distribution list %2 is empty, it cannot be used.",
                         "Distribution lists %2 are empty, they cannot be used.",
                         numberOfDistributionList, listOfDistributionList);
        KMessageBox::sorry(mParentWidget, errorMsg, i18n("Invalid Email Address"));
        mIsValid = false;
    } else {
        if (!(errorCode == KEmailAddress::AddressOk
              || errorCode == KEmailAddress::AddressEmpty)) {
            const QString errorMsg(QLatin1String("<qt><p><b>") + brokenAddress
                                   +QLatin1String("</b></p><p>")
                                   +KEmailAddress::emailParseResultToString(errorCode)
                                   +QLatin1String("</p></qt>"));
            KMessageBox::sorry(mParentWidget, errorMsg, i18n("Invalid Email Address"));
            mIsValid = false;
        }
    }

    emitResult();
}
