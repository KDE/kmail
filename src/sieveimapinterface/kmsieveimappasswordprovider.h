/*
    SPDX-FileCopyrightText: 2017 Albert Astals Cid <aacid@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KMSIEVEIMAPPASSWORDPROVIDER_H
#define KMSIEVEIMAPPASSWORDPROVIDER_H
#include "kmail_private_export.h"
#include <KSieveUi/SieveImapPasswordProvider>

#include <QWidget> // for WId

class KMAILTESTS_TESTS_EXPORT KMSieveImapPasswordProvider : public KSieveUi::SieveImapPasswordProvider
{
public:
    KMSieveImapPasswordProvider(WId wid);

    Q_REQUIRED_RESULT QString password(const QString &identifier) override;
    Q_REQUIRED_RESULT QString sieveCustomPassword(const QString &identifier) override;

private:
    Q_REQUIRED_RESULT QString walletPassword(const QString &identifier);

    const WId mWid;
};

#endif
