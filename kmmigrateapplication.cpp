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
                           << QStringLiteral("messageviewer.notifyrc") << QStringLiteral("sievetemplaterc") << QStringLiteral("foldermailarchiverc"));
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
    const int currentVersion = 2;
    mMigrator.setApplicationName(QStringLiteral("kmail2"));
    mMigrator.setConfigFileName(QStringLiteral("kmail2rc"));
    mMigrator.setCurrentConfigVersion(currentVersion);

    // To migrate we need a version < currentVersion
    const int initialVersion = currentVersion - 1;
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
}
