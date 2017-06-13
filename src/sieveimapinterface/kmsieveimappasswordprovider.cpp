/*
    Copyright (c) 2017 Albert Astals Cid <aacid@kde.org>

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

#include "kmsieveimappasswordprovider.h"

#include <kwallet.h>

#include <memory>

KMSieveImapPasswordProvider::KMSieveImapPasswordProvider(WId wid)
    : m_wid(wid)
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
    std::unique_ptr<KWallet::Wallet> wallet(KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(), m_wid));
    if (wallet) {
        if (wallet->hasFolder(QStringLiteral("imap"))) {
            QString pwd;
            wallet->setFolder(QStringLiteral("imap"));
            wallet->readPassword(identifier + QStringLiteral("rc"), pwd);
            return pwd;
        }
    }

    return QString();

}
