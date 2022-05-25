/*
   SPDX-FileCopyrightText: 2015-2022 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once
#include <QtGlobal>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
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

#endif
