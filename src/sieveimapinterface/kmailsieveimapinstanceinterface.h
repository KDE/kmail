/*
   SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "kmail_private_export.h"
#include <KSieveUi/SieveImapInstanceInterface>
class KMAILTESTS_TESTS_EXPORT KMailSieveImapInstanceInterface : public KSieveUi::SieveImapInstanceInterface
{
public:
    KMailSieveImapInstanceInterface();
    ~KMailSieveImapInstanceInterface() override = default;

    Q_REQUIRED_RESULT QVector<KSieveUi::SieveImapInstance> sieveImapInstances() override;

private:
    Q_DISABLE_COPY(KMailSieveImapInstanceInterface)
};

