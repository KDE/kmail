/*
   SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "kmail_export.h"
#include <PimCommon/MigrateApplicationFiles>
#include <kcoreaddons_version.h>
#if KCOREADDONS_VERSION < QT_VERSION_CHECK(6, 0, 0)
class KMAIL_EXPORT KMMigrateApplication
{
public:
    KMMigrateApplication();

    void migrate();

private:
    void initializeMigrator();
    void migrateAlwaysEncrypt();

    PimCommon::MigrateApplicationFiles mMigrator;
};

#endif
