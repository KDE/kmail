/*
    SPDX-FileCopyrightText: 2017 Albert Astals Cid <aacid@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once
#include "kmail_private_export.h"
#include <KSieveUi/SieveImapPasswordProvider>
#include <qt5keychain/keychain.h>
class KMAILTESTS_TESTS_EXPORT KMSieveImapPasswordProvider : public KSieveUi::SieveImapPasswordProvider
{
    Q_OBJECT
public:
    explicit KMSieveImapPasswordProvider(QObject *parent = nullptr);
    ~KMSieveImapPasswordProvider() override;

    void passwords(const QString &identifier) override;

private:
    void readSieveServerPasswordFinished(QKeychain::Job *baseJob);
    void readSieveServerCustomPasswordFinished(QKeychain::Job *baseJob);
    QString mIdentifier;
    QString mSievePassword;
    QString mSieveCustomPassword;
};

