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

#ifndef POTENTIALPHISHINGEMAILJOB_H
#define POTENTIALPHISHINGEMAILJOB_H

#include <QObject>
#include <QStringList>

class PotentialPhishingEmailJob : public QObject
{
    Q_OBJECT
public:
    explicit PotentialPhishingEmailJob(QObject *parent = Q_NULLPTR);
    ~PotentialPhishingEmailJob();

    void setEmailWhiteList(const QStringList &emails);
    void setEmails(const QStringList &emails);

    QStringList potentialPhisingEmails() const;
    bool start();

Q_SIGNALS:
    void potentialPhishingEmailsFound(const QStringList &emails);

private:
    QStringList mEmails;
    QStringList mPotentialPhisingEmails;
    QStringList mEmailWhiteList;
};

#endif // POTENTIALPHISHINGEMAILJOB_H
