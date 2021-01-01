/*
   SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KMMIGRATEAPPLICATION_H
#define KMMIGRATEAPPLICATION_H

#include <PimCommon/MigrateApplicationFiles>
#include "kmail_export.h"
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

#endif // KMMIGRATEAPPLICATION_H
