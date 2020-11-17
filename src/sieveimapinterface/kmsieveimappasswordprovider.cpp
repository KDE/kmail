/*
    SPDX-FileCopyrightText: 2017 Albert Astals Cid <aacid@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kmsieveimappasswordprovider.h"

#include <KWallet>

#include <memory>

KMSieveImapPasswordProvider::KMSieveImapPasswordProvider(WId wid)
    : mWid(wid)
{
}

QString KMSieveImapPasswordProvider::password(const QString &identifier)
{
    return walletPassword(identifier);
}

QString KMSieveImapPasswordProvider::sieveCustomPassword(const QString &identifier)
{
    return walletPassword(QStringLiteral("custom_sieve_") + identifier);
}

QString KMSieveImapPasswordProvider::walletPassword(const QString &identifier)
{
    std::unique_ptr<KWallet::Wallet> wallet(KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(), mWid));
    if (wallet) {
        if (wallet->hasFolder(QStringLiteral("imap"))) {
            QString pwd;
            wallet->setFolder(QStringLiteral("imap"));
            wallet->readPassword(identifier + QStringLiteral("rc"), pwd);
            return pwd;
        }
    }

    return {};
}
