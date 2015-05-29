/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "kmmigrateapplication.h"

#include <Kdelibs4ConfigMigrator>

KMMigrateApplication::KMMigrateApplication()
{
    initializeMigrator();
}

void KMMigrateApplication::migrate()
{
    // Migrate to xdg.
    Kdelibs4ConfigMigrator migrate(QLatin1String("kmail"));
    migrate.setConfigFiles(QStringList() << QStringLiteral("kmail2rc") << QStringLiteral("kmail2.notifyrc") << QStringLiteral("kmailsnippetrc")
                           << QStringLiteral("customtemplatesrc") << QStringLiteral("templatesconfigurationrc") << QStringLiteral("kpimcompletionorder")
                           << QStringLiteral("messageviewer.notifyrc") << QStringLiteral("sievetemplaterc"));
    migrate.setUiFiles(QStringList() << QStringLiteral("kmail_part.rc") << QStringLiteral("kmcomposerui.rc") <<  QStringLiteral("kmmainwin.rc")
                       <<  QStringLiteral("kmreadermainwin.rc"));
    migrate.migrate();

    // Migrate folders and files.
    if (mMigrator.checkIfNecessary()) {
        mMigrator.start();
    }
}

void KMMigrateApplication::initializeMigrator()
{
    const int currentVersion = 1;
    mMigrator.setApplicationName(QStringLiteral("kmail2"));
    mMigrator.setConfigFileName(QStringLiteral("kmail2rc"));
    mMigrator.setCurrentConfigVersion(currentVersion);
    // autosave
    PimCommon::MigrateFileInfo migrateInfoAutoSave;
    migrateInfoAutoSave.setFolder(true);
    migrateInfoAutoSave.setType(QStringLiteral("apps"));
    migrateInfoAutoSave.setPath(QStringLiteral("kmail2/autosave/"));
    mMigrator.insertMigrateInfo(migrateInfoAutoSave);

    //MessageViewer
    PimCommon::MigrateFileInfo migrateInfoMessageViewer;
    migrateInfoMessageViewer.setFolder(true);
    migrateInfoMessageViewer.setType(QStringLiteral("apps"));
    migrateInfoMessageViewer.setPath(QStringLiteral("messageviewer/theme/"));
    mMigrator.insertMigrateInfo(migrateInfoMessageViewer);

    //autocorrect
    PimCommon::MigrateFileInfo migrateInfoAutocorrect;
    migrateInfoAutocorrect.setFolder(true);
    migrateInfoAutocorrect.setType(QStringLiteral("apps"));
    migrateInfoAutocorrect.setPath(QStringLiteral("autocorrect/"));
    mMigrator.insertMigrateInfo(migrateInfoAutocorrect);

    //autocorrect
    PimCommon::MigrateFileInfo migrateInfoGravatar;
    migrateInfoGravatar.setFolder(true);
    migrateInfoGravatar.setType(QStringLiteral("apps"));
    migrateInfoGravatar.setPath(QStringLiteral("gravatar/"));
    mMigrator.insertMigrateInfo(migrateInfoGravatar);
}
