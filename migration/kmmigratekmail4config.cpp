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

#include "kmmigratekmail4config.h"
#include <kdelibs4migration.h>
#include <KSharedConfig>
#include <KConfigGroup>
#include <QDebug>

KMMigrateKMail4Config::KMMigrateKMail4Config(QObject *parent)
    : QObject(parent),
      mVersion(1)
{

}

KMMigrateKMail4Config::~KMMigrateKMail4Config()
{

}

bool KMMigrateKMail4Config::start()
{
    if (mMigrateInfoList.isEmpty()) {
        return false;
    }

    if (mConfigFileName.isEmpty()) {
        qDebug() << " config file name not defined.";
        return false;
    }
    // Testing for kdehome
    Kdelibs4Migration migration;
    if (!migration.kdeHomeFound()) {
        return false;
    }
    return migrateConfig();
}

bool KMMigrateKMail4Config::migrateConfig()
{
    Q_FOREACH(const MigrateInfo &info, mMigrateInfoList) {
        if (info.folder()) {
            migrateFolder(info);
        } else {
            migrateFile(info);
        }
    }
    return true;
}

QString KMMigrateKMail4Config::configFileName() const
{
    return mConfigFileName;
}

void KMMigrateKMail4Config::setConfigFileName(const QString &configFileName)
{
    mConfigFileName = configFileName;
}

void KMMigrateKMail4Config::writeConfig()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig(mConfigFileName, KConfig::SimpleConfig);
    KConfigGroup grp = config->group(QStringLiteral("Migrate"));
    grp.writeEntry(QStringLiteral("Version"), mVersion);
    grp.sync();
}

void KMMigrateKMail4Config::migrateFolder(const MigrateInfo &info)
{
    //TODO
}

void KMMigrateKMail4Config::migrateFile(const MigrateInfo &info)
{
    //TODO
}

int KMMigrateKMail4Config::version() const
{
    return mVersion;
}

void KMMigrateKMail4Config::setVersion(int version)
{
    mVersion = version;
}

bool KMMigrateKMail4Config::checkIfNecessary()
{
    if (mConfigFileName.isEmpty()) {
        qDebug() << " config file name not defined.";
        return false;
    }
    KSharedConfig::Ptr config = KSharedConfig::openConfig(mConfigFileName, KConfig::SimpleConfig);
    if (config->hasGroup(QStringLiteral("Migrate"))) {
        KConfigGroup grp = config->group(QStringLiteral("Migrate"));
        const int lastVersion = grp.readEntry(QStringLiteral("Version"), 0);
        if (lastVersion < mVersion) {
            return true;
        } else {
            return false;
        }
    }
    return true;
}

void KMMigrateKMail4Config::insertMigrateInfo(const MigrateInfo &info)
{
    if (info.isValid()) {
        mMigrateInfoList.append(info);
    }
}

