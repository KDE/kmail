/*
  SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#include "potentialphishingemailjob.h"
#include "kmail_debug.h"
#include <KEmailAddress>
#include <PimCommon/PimUtil>

PotentialPhishingEmailJob::PotentialPhishingEmailJob(QObject *parent)
    : QObject(parent)
{
}

PotentialPhishingEmailJob::~PotentialPhishingEmailJob() = default;

void PotentialPhishingEmailJob::setEmailWhiteList(const QStringList &emails)
{
    mEmailWhiteList = emails;
}

void PotentialPhishingEmailJob::setPotentialPhishingEmails(const QStringList &list)
{
    mEmails = PimCommon::Util::generateEmailList(list);
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
        Q_EMIT potentialPhishingEmailsFound(mPotentialPhisingEmails);
        deleteLater();
        return false;
    }
    for (const QString &addr : std::as_const(mEmails)) {
        if (!mEmailWhiteList.contains(addr.trimmed())) {
            QString tname, temail;
            KEmailAddress::extractEmailAddressAndName(addr, temail, tname); // ignore return value
            // which is always false
            if (tname.startsWith(QLatin1Char('@'))) { // Special case when name is just @foo <...> it mustn't recognize as a valid email
                continue;
            }
            if (tname.contains(QLatin1Char('@'))) { // Potential address
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
