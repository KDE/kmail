/*
   Copyright (C) 2019 Montel Laurent <montel@kde.org>

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

#include "refreshsettingscleanuppage.h"
#include <QHBoxLayout>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfigGroup>
#include <QPushButton>
#include <QRegularExpression>

RefreshSettingsCleanupPage::RefreshSettingsCleanupPage(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setObjectName(QStringLiteral("mainLayout"));
    mainLayout->setContentsMargins(0, 0, 0, 0);
    QPushButton *button = new QPushButton(i18n("Clean"), this);
    button->setObjectName(QStringLiteral("button"));
    mainLayout->addWidget(button);
    connect(button, &QPushButton::clicked, this, &RefreshSettingsCleanupPage::cleanSettings);
}

RefreshSettingsCleanupPage::~RefreshSettingsCleanupPage()
{
}

void RefreshSettingsCleanupPage::cleanSettings()
{
    const QStringList configNameFiles {QStringLiteral("kmail2rc"), QStringLiteral("kontactrc")};
    for (const QString &configName: configNameFiles) {
        initCleanupFolderSettings(configName);
        initCleanupFiltersSettings(configName);
        initCleanDialogSettings(configName);
        removeTipOfDay(configName);
    }
    Q_EMIT cleanUpDone();
}

void RefreshSettingsCleanupPage::removeTipOfDay(const QString &configName)
{
    KSharedConfigPtr settingsrc = KSharedConfig::openConfig(configName);

    const QString tipOfDayStr = QStringLiteral("TipOfDay");
    if (settingsrc->hasGroup(tipOfDayStr)) {
        settingsrc->deleteGroup(tipOfDayStr);
    }
    settingsrc->sync();
    Q_EMIT cleanDoneInfo(i18n("Remove obsolete \"TipOfDay\" settings: Done"));
}

void RefreshSettingsCleanupPage::initCleanDialogSettings(const QString &configName)
{
    KSharedConfigPtr settingsrc = KSharedConfig::openConfig(configName);

    const QStringList dialogList = settingsrc->groupList().filter(QRegularExpression(QStringLiteral(".*Dialog$")));
    for (const QString &str : dialogList) {
        settingsrc->deleteGroup(str);
    }
    settingsrc->sync();
    Q_EMIT cleanDoneInfo(i18n("Delete Dialog settings in file `%1`: Done", configName));
}

void RefreshSettingsCleanupPage::initCleanupFiltersSettings(const QString &configName)
{
    KSharedConfigPtr settingsrc = KSharedConfig::openConfig(configName);

    const QStringList filterList = settingsrc->groupList().filter(QRegularExpression(QStringLiteral("Filter #\\d+")));
    for (const QString &str : filterList) {
        settingsrc->deleteGroup(str);
    }
    settingsrc->sync();
    Q_EMIT cleanDoneInfo(i18n("Delete Filters settings in file `%1`: Done", configName));
}

void RefreshSettingsCleanupPage::initCleanupFolderSettings(const QString &configName)
{
    KSharedConfigPtr settingsrc = KSharedConfig::openConfig(configName);

    const QStringList folderList = settingsrc->groupList().filter(QRegularExpression(QStringLiteral("Folder-\\d+")));
    for (const QString &str : folderList) {
        KConfigGroup oldGroup = settingsrc->group(str);
        cleanupFolderSettings(oldGroup);
    }
    settingsrc->sync();
    Q_EMIT cleanDoneInfo(i18n("Clean Folder Settings in setting file `%1`: Done", configName));
}

void RefreshSettingsCleanupPage::cleanupFolderSettings(KConfigGroup &oldGroup)
{
    const bool mailingListEnabled = oldGroup.readEntry("MailingListEnabled", false);
    if (!mailingListEnabled) {
        oldGroup.deleteEntry("MailingListEnabled");
    }
    const int mailingListFeatures = oldGroup.readEntry("MailingListFeatures", 0);
    if (mailingListFeatures == 0) {
        oldGroup.deleteEntry("MailingListFeatures");
    }
    const int mailingListHandler = oldGroup.readEntry("MailingListHandler", 0);
    if (mailingListHandler == 0) {
        oldGroup.deleteEntry("MailingListHandler");
    }
    const QString mailingListId = oldGroup.readEntry("MailingListId");
    if (mailingListId.isEmpty()) {
        oldGroup.deleteEntry("MailingListId");
    }

    const bool putRepliesInSameFolder = oldGroup.readEntry("PutRepliesInSameFolder", false);
    if (!putRepliesInSameFolder) {
        oldGroup.deleteEntry("PutRepliesInSameFolder");
    }

    const bool folderHtmlLoadExtPreference = oldGroup.readEntry("htmlLoadExternalOverride", false);
    if (!folderHtmlLoadExtPreference) {
        oldGroup.deleteEntry("htmlLoadExternalOverride");
    }
    const bool useDefaultIdentity = oldGroup.readEntry("UseDefaultIdentity", false);
    if (useDefaultIdentity) {
        oldGroup.deleteEntry("UseDefaultIdentity");
    }
}

void RefreshSettingsCleanupPage::initCleanupDialogSettings(const QString &configName)
{
    KSharedConfigPtr settingsrc = KSharedConfig::openConfig(configName);

    const QStringList dialogListName{QStringLiteral("AddHostDialog"),
                QStringLiteral("AuditLogViewer"),
                QStringLiteral("CollectionPropertiesDialog"),
                QStringLiteral("MailSourceWebEngineViewer"),
                QStringLiteral("SelectAddressBookDialog"),
                QStringLiteral("VCardViewer")};
    for (const QString &str : dialogListName) {
        KConfigGroup oldGroup = settingsrc->group(str);
        cleanupFolderSettings(oldGroup);
    }
    settingsrc->sync();
    Q_EMIT cleanDoneInfo(i18n("Clean Dialog Size in setting file `%1`: Done", configName));
}
