/*
   SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "kmail_export.h"
#include <PimCommon/MigrateApplicationFiles>
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

