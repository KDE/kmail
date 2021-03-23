/*
 * This file is part of KMail.
 *
 * SPDX-FileCopyrightText: 2010 KDAB
 *
 * SPDX-FileContributor: Tobias Koenig <tokoe@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <KJob>

class AddressValidationJob : public KJob
{
    Q_OBJECT

public:
    explicit AddressValidationJob(const QString &emailAddresses, QWidget *parentWidget, QObject *parent = nullptr);
    ~AddressValidationJob() override;

    void start() override;

    Q_REQUIRED_RESULT bool isValid() const;

    void setDefaultDomain(const QString &domainName);

private:
    void slotAliasExpansionDone(KJob *);
    const QString mEmailAddresses;
    QString mDomainDefaultName;
    bool mIsValid = false;
    QWidget *const mParentWidget;
};

