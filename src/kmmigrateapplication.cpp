/*
   Copyright (C) 2015-2020 Laurent Montel <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kmmigrateapplication.h"
#include <MessageComposer/MessageComposerSettings>
#include <KIdentityManagement/IdentityManager>
#include <KIdentityManagement/Identity>
#include <Kdelibs4ConfigMigrator>

KMMigrateApplication::KMMigrateApplication()
{
    initializeMigrator();
}

void KMMigrateApplication::migrate()
{
    // Migrate to xdg.
    Kdelibs4ConfigMigrator migrate(QStringLiteral("kmail"));

    migrate.setConfigFiles(QStringList() << QStringLiteral("kmail2rc") << QStringLiteral("kmail2.notifyrc") << QStringLiteral("kmailsnippetrc")
                                         << QStringLiteral("customtemplatesrc") << QStringLiteral("templatesconfigurationrc") << QStringLiteral("kpimcompletionorder")
                                         << QStringLiteral("messageviewer.notifyrc") << QStringLiteral("sievetemplaterc") << QStringLiteral("foldermailarchiverc")
                                         << QStringLiteral("kpimbalooblacklist"));
    migrate.setUiFiles(QStringList() << QStringLiteral("kmail_part.rc") << QStringLiteral("kmcomposerui.rc") <<  QStringLiteral("kmmainwin.rc")
                                     << QStringLiteral("kmreadermainwin.rc"));
    migrate.migrate();

    // Migrate folders and files.
    if (mMigrator.checkIfNecessary()) {
        mMigrator.start();
    }

    // Migrate global "Always encrypt" option to per-identity options
    migrateAlwaysEncrypt();
}

void KMMigrateApplication::initializeMigrator()
{
    const int currentVersion = 2;
    mMigrator.setApplicationName(QStringLiteral("kmail2"));
    mMigrator.setConfigFileName(QStringLiteral("kmail2rc"));
    mMigrator.setCurrentConfigVersion(currentVersion);

    // To migrate we need a version > currentVersion
    const int initialVersion = currentVersion + 1;
    // autosave
    PimCommon::MigrateFileInfo migrateInfoAutoSave;
    migrateInfoAutoSave.setFolder(true);
    migrateInfoAutoSave.setType(QStringLiteral("data"));
    migrateInfoAutoSave.setPath(QStringLiteral("kmail2/autosave/"));
    migrateInfoAutoSave.setVersion(initialVersion);
    mMigrator.insertMigrateInfo(migrateInfoAutoSave);

    //MessageViewer
    PimCommon::MigrateFileInfo migrateInfoMessageViewer;
    migrateInfoMessageViewer.setFolder(true);
    migrateInfoMessageViewer.setType(QStringLiteral("data"));
    migrateInfoMessageViewer.setPath(QStringLiteral("messageviewer/theme/"));
    migrateInfoMessageViewer.setVersion(initialVersion);
    mMigrator.insertMigrateInfo(migrateInfoMessageViewer);

    //autocorrect
    PimCommon::MigrateFileInfo migrateInfoAutocorrect;
    migrateInfoAutocorrect.setFolder(true);
    migrateInfoAutocorrect.setType(QStringLiteral("data"));
    migrateInfoAutocorrect.setPath(QStringLiteral("autocorrect/"));
    migrateInfoAutocorrect.setVersion(initialVersion);
    mMigrator.insertMigrateInfo(migrateInfoAutocorrect);

    //gravatar
    PimCommon::MigrateFileInfo migrateInfoGravatar;
    migrateInfoGravatar.setFolder(true);
    migrateInfoGravatar.setType(QStringLiteral("data"));
    migrateInfoGravatar.setPath(QStringLiteral("gravatar/"));
    migrateInfoGravatar.setVersion(initialVersion);
    mMigrator.insertMigrateInfo(migrateInfoGravatar);

    //adblock
    PimCommon::MigrateFileInfo migrateInfoAdblockrules;
    migrateInfoAdblockrules.setFolder(false);
    migrateInfoAdblockrules.setType(QStringLiteral("data"));
    migrateInfoAdblockrules.setPath(QStringLiteral("kmail2/"));
    migrateInfoAdblockrules.setFilePatterns(QStringList() << QStringLiteral("adblockrules_*"));
    migrateInfoAdblockrules.setVersion(initialVersion);
    mMigrator.insertMigrateInfo(migrateInfoAdblockrules);

    //vcard from identity
    PimCommon::MigrateFileInfo migrateInfoVCardFromIdentity;
    migrateInfoVCardFromIdentity.setFolder(false);
    migrateInfoVCardFromIdentity.setType(QStringLiteral("data"));
    migrateInfoVCardFromIdentity.setPath(QStringLiteral("kmail2/"));
    migrateInfoVCardFromIdentity.setFilePatterns(QStringList() << QStringLiteral("*.vcf"));
    migrateInfoVCardFromIdentity.setVersion(initialVersion);
    mMigrator.insertMigrateInfo(migrateInfoVCardFromIdentity);
}

void KMMigrateApplication::migrateAlwaysEncrypt()
{
    KConfig cfg(QStringLiteral("kmail2rc"));
    if (!cfg.hasGroup("Composer")) {
        return;
    }

    KConfigGroup grp = cfg.group("Composer");
    if (!grp.hasKey("pgp-auto-encrypt")) {
        return;
    }

    const bool pgpAutoEncrypt = grp.readEntry("pgp-auto-encrypt", false);
    grp.deleteEntry("pgp-auto-encrypt");

    // Only update the per-identity flag to true
    if (pgpAutoEncrypt) {
        KIdentityManagement::IdentityManager mgr;
        for (auto iter = mgr.modifyBegin(), end = mgr.modifyEnd(); iter != end; ++iter) {
            iter->setPgpAutoEncrypt(pgpAutoEncrypt);
        }
        mgr.commit();
    }
}
