/*
  SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#pragma once

#include "kmail_private_export.h"
#include <QObject>
#include <QStringList>

class KMAILTESTS_TESTS_EXPORT PotentialPhishingEmailJob : public QObject
{
    Q_OBJECT
public:
    explicit PotentialPhishingEmailJob(QObject *parent = nullptr);
    ~PotentialPhishingEmailJob() override;

    void setEmailWhiteList(const QStringList &emails);
    void setPotentialPhishingEmails(const QStringList &emails);

    Q_REQUIRED_RESULT QStringList potentialPhisingEmails() const;
    Q_REQUIRED_RESULT bool start();

    Q_REQUIRED_RESULT QStringList checkEmails() const;

Q_SIGNALS:
    void potentialPhishingEmailsFound(const QStringList &emails);

private:
    Q_DISABLE_COPY(PotentialPhishingEmailJob)
    QStringList mEmails;
    QStringList mPotentialPhisingEmails;
    QStringList mEmailWhiteList;
};

