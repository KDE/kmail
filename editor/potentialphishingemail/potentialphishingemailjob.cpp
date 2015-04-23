/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

*/

#include "potentialphishingemailjob.h"
#include <KEmailAddress>
PotentialPhishingEmailJob::PotentialPhishingEmailJob(QObject *parent)
    : QObject(parent)
{

}

PotentialPhishingEmailJob::~PotentialPhishingEmailJob()
{

}

void PotentialPhishingEmailJob::setEmailWhiteList(const QStringList &emails)
{
    mEmailWhiteList = emails;
}

void PotentialPhishingEmailJob::setEmails(const QStringList &emails)
{
    mEmails = emails;
}

QStringList PotentialPhishingEmailJob::potentialPhisingEmails() const
{
    return mPotentialPhisingEmails;
}

bool PotentialPhishingEmailJob::start()
{
    mPotentialPhisingEmails.clear();
    if (mEmails.isEmpty()) {
        deleteLater();
        return false;
    }
    Q_FOREACH (const QString &addr, mEmails) {
        if (!mEmailWhiteList.contains(addr.trimmed())) {
            QString tname, temail;
            KEmailAddress::extractEmailAddressAndName(addr, temail, tname);    // ignore return value
            // which is always false
            if (tname.contains(QLatin1String("@"))) { //Potential address
                if (tname.startsWith(QLatin1Char('<')) && tname.endsWith(QLatin1Char('>'))) {
                    tname = tname.mid(1, tname.length() - 2);
                }
                if (temail.toLower() != tname.toLower()) {
                    mPotentialPhisingEmails.append(addr);
                }
            }
        }
    }
    Q_EMIT potentialPhishingEmailsFound(mPotentialPhisingEmails);
    deleteLater();
    return true;
}
