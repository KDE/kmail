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
    //TODO update next button
}

RefreshSettingsCleanupPage::~RefreshSettingsCleanupPage()
{
}

void RefreshSettingsCleanupPage::cleanSettings()
{
    initCleanupFolderSettings(QStringLiteral("kmail2rc"));
    initCleanupFolderSettings(QStringLiteral("kontactrc"));
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
