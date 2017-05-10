/*
  Copyright (c) 2015-2017 Montel Laurent <montel@kde.org>

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
#include "kmail_debug.h"

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

void PotentialPhishingEmailJob::setPotentialPhishingEmails(const QStringList &list)
{
    QString str;
    for (int i = 0; i < list.count(); i++) {
        if (!list.at(i).trimmed().isEmpty()) {
            if (!str.isEmpty()) {
                str.append(QStringLiteral(", "));
            }
            str.append(list.at(i));
        }
    }
    const QStringList emails = KEmailAddress::splitAddressList(str);
    mEmails = emails;
}

QStringList PotentialPhishingEmailJob::checkEmails() const
{
    return mEmails;
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
    for (const QString &addr : qAsConst(mEmails)) {
        if (!mEmailWhiteList.contains(addr.trimmed())) {
            QString tname, temail;
            KEmailAddress::extractEmailAddressAndName(addr, temail, tname);    // ignore return value
            // which is always false
            if (tname.contains(QLatin1Char('@'))) { //Potential address
                if (tname.startsWith(QLatin1Char('<')) && tname.endsWith(QLatin1Char('>'))) {
                    tname = tname.mid(1, tname.length() - 2);
                }
                if (tname.startsWith(QLatin1Char('\'')) && tname.endsWith(QLatin1Char('\''))) {
                    tname = tname.mid(1, tname.length() - 2);
                }
                if (temail.toLower() != tname.toLower()) {
                    const QString str = QStringLiteral("(%1)").arg(temail);
                    if (!tname.contains(str, Qt::CaseInsensitive)) {
                        const QStringList lst = tname.trimmed().split(QLatin1Char(' '));
                        if (lst.count() > 1) {
                            const QString firstName = lst.at(0);

                            for (const QString &n : lst) {
                                if (n != firstName) {
                                    mPotentialPhisingEmails.append(addr);
                                    break;
                                }
                            }
                        } else {
                            mPotentialPhisingEmails.append(addr);
                        }
                    }
                }
            }
        }
    }
    Q_EMIT potentialPhishingEmailsFound(mPotentialPhisingEmails);
    deleteLater();
    return true;
}
