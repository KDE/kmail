/*
   SPDX-FileCopyrightText: 2017-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KMAILSIEVEIMAPINSTANCEINTERFACE_H
#define KMAILSIEVEIMAPINSTANCEINTERFACE_H

#include <KSieveUi/SieveImapInstanceInterface>
#include "kmail_private_export.h"
class KMAILTESTS_TESTS_EXPORT KMailSieveImapInstanceInterface : public KSieveUi::SieveImapInstanceInterface
{
public:
    KMailSieveImapInstanceInterface();
    ~KMailSieveImapInstanceInterface() override = default;

    Q_REQUIRED_RESULT QVector<KSieveUi::SieveImapInstance> sieveImapInstances() override;
private:
    Q_DISABLE_COPY(KMailSieveImapInstanceInterface)
};

#endif // KMAILSIEVEIMAPINSTANCEINTERFACE_H
