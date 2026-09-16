/*
  SPDX-FileCopyrightText: 2015-2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#include "potentialphishingemailjob.h"
#include <KEmailAddress>
#include <PimCommon/PimUtil>
#include <algorithm>
#include <utility>
using namespace Qt::Literals::StringLiterals;

namespace
{
[[nodiscard]] bool containsDifferentName(const QList<QStringView> &lst, QStringView firstName)
{
    return std::any_of(lst.cbegin(), lst.cend(), [firstName](QStringView n) {
        return n != firstName;
    });
}
}

PotentialPhishingEmailJob::PotentialPhishingEmailJob(QObject *parent)
    : QObject(parent)
{
}

PotentialPhishingEmailJob::~PotentialPhishingEmailJob() = default;

void PotentialPhishingEmailJob::setEmailWhiteList(const QStringList &emails)
{
    mEmailWhiteList.clear();
    mEmailWhiteList.reserve(emails.count());
    for (const QString &email : emails) {
        if (QString normalizedEmail = email.trimmed().toCaseFolded(); !normalizedEmail.isEmpty()) {
            mEmailWhiteList.append(std::move(normalizedEmail));
        }
    }
}

void PotentialPhishingEmailJob::setPotentialPhishingEmails(const QStringList &emails)
{
    mEmails = PimCommon::Util::generateEmailList(emails);
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
        if (mEmailWhiteList.isEmpty() || !mEmailWhiteList.contains(addr.trimmed().toCaseFolded())) {
            QString tname;
            QString temail;
            KEmailAddress::extractEmailAddressAndName(addr, temail, tname); // ignore return value
            // which is always false
            if (tname.startsWith(u'@')) { // Special case when name is just @foo <…> it mustn't recognize as a valid email
                continue;
            }
            if (tname.contains(u'@')) { // Potential address
                if (tname.startsWith(u'<') && tname.endsWith(u'>')) {
                    tname = tname.mid(1, tname.length() - 2);
                }
                if (tname.startsWith(u'\'') && tname.endsWith(u'\'')) {
                    tname = tname.mid(1, tname.length() - 2);
                }
                if (temail.compare(tname, Qt::CaseInsensitive) != 0) {
                    if (const QString str = u"(%1)"_s.arg(temail); !tname.contains(str, Qt::CaseInsensitive)) {
                        // Keep the trimmed string alive: the QStringViews below point into it.
                        const QString trimmedName = tname.trimmed();
                        if (const QList<QStringView> lst = QStringView(trimmedName).split(u' '); lst.count() > 1) {
                            if (containsDifferentName(lst, lst.constFirst())) {
                                mPotentialPhisingEmails.append(addr);
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

#include "moc_potentialphishingemailjob.cpp"
