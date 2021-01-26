/*
   SPDX-FileCopyrightText: 2019-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "refreshsettingscleanuppage.h"
#include <KLocalizedString>
#include <KSharedConfig>
#include <QHBoxLayout>
#include <QPushButton>
#include <QRegularExpression>

RefreshSettingsCleanupPage::RefreshSettingsCleanupPage(QWidget *parent)
    : QWidget(parent)
{
    auto mainLayout = new QHBoxLayout(this);
    mainLayout->setObjectName(QStringLiteral("mainLayout"));
    mainLayout->setContentsMargins({});
    auto button = new QPushButton(i18n("Clean"), this);
    button->setObjectName(QStringLiteral("button"));
    mainLayout->addWidget(button);
    connect(button, &QPushButton::clicked, this, &RefreshSettingsCleanupPage::cleanSettings);
}

RefreshSettingsCleanupPage::~RefreshSettingsCleanupPage() = default;

void RefreshSettingsCleanupPage::cleanSettings()
{
    const QStringList configNameFiles{QStringLiteral("kmail2rc"), QStringLiteral("kontactrc")};
    for (const QString &configName : configNameFiles) {
        initCleanupFolderSettings(configName);
        initCleanupFiltersSettings(configName);
        initCleanDialogSettings(configName);
        initCleanupDialogSettings(configName);
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
        if (oldGroup.keyList().isEmpty()) {
            oldGroup.deleteGroup();
        }
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
                                     QStringLiteral("VCardViewer"),
                                     QStringLiteral("MailSourceWebEngineViewer"),
                                     QStringLiteral("ConfigurePluginsWidget"),
                                     QStringLiteral("ConfigureAgentsWidget"),
                                     QStringLiteral("AuditLogViewer")};
    for (const QString &str : dialogListName) {
        KConfigGroup oldGroup = settingsrc->group(str);
        cleanupFolderSettings(oldGroup);
    }
    settingsrc->sync();
    Q_EMIT cleanDoneInfo(i18n("Clean Dialog Size in setting file `%1`: Done", configName));
}
