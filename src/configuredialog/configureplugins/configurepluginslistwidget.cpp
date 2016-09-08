/*
  Copyright (c) 2016 Montel Laurent <montel@kde.org>

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

#include "configurepluginslistwidget.h"
#include <MessageViewer/ViewerPluginManager>
#include <MessageComposer/PluginEditorCheckBeforeSendManager>
#include <PimCommon/GenericPluginManager>

#include <PimCommon/PluginUtil>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KLocalizedString>
#include <MessageComposer/PluginEditorManager>

#include <QHBoxLayout>
#include <QLabel>
#include <QTreeWidget>
#include <QDebug>

ConfigurePluginsListWidget::ConfigurePluginsListWidget(QWidget *parent)
    : PimCommon::ConfigurePluginsListWidget(parent)
{
}

ConfigurePluginsListWidget::~ConfigurePluginsListWidget()
{
}

void ConfigurePluginsListWidget::savePlugins(const QString &groupName, const QString &prefixSettingKey, const QList<PluginItem *> &listItems)
{
    QStringList enabledPlugins;
    QStringList disabledPlugins;
    Q_FOREACH (PluginItem *item, listItems) {
        if (item->checkState(0) == Qt::Checked) {
            enabledPlugins << item->mIdentifier;
        } else {
            disabledPlugins << item->mIdentifier;
        }
    }
    PimCommon::PluginUtil::savePluginSettings(groupName,
            prefixSettingKey,
            enabledPlugins, disabledPlugins);
}

void ConfigurePluginsListWidget::save()
{
    savePlugins(MessageComposer::PluginEditorManager::self()->configGroupName(),
                MessageComposer::PluginEditorManager::self()->configPrefixSettingKey(),
                mPluginEditorItems);
    savePlugins(MessageViewer::ViewerPluginManager::self()->configGroupName(),
                MessageViewer::ViewerPluginManager::self()->configPrefixSettingKey(),
                mPluginMessageViewerItems);
    savePlugins(MessageComposer::PluginEditorCheckBeforeSendManager::self()->configGroupName(),
                MessageComposer::PluginEditorCheckBeforeSendManager::self()->configPrefixSettingKey(),
                mPluginSendBeforeSendItems);
}

void ConfigurePluginsListWidget::doLoadFromGlobalSettings()
{
    initialize();
}

void ConfigurePluginsListWidget::doResetToDefaultsOther()
{
    Q_FOREACH (PluginItem *item, mPluginEditorItems) {
        item->setCheckState(0, item->mEnableByDefault ? Qt::Checked : Qt::Unchecked);
    }

    Q_FOREACH (PluginItem *item, mPluginMessageViewerItems) {
        item->setCheckState(0, item->mEnableByDefault ? Qt::Checked : Qt::Unchecked);
    }

    Q_FOREACH (PluginItem *item, mPluginSendBeforeSendItems) {
        item->setCheckState(0, item->mEnableByDefault ? Qt::Checked : Qt::Unchecked);
    }
}

void ConfigurePluginsListWidget::initialize()
{
    mListWidget->clear();
    mPluginMessageViewerItems.clear();
    mPluginEditorItems.clear();
    mPluginSendBeforeSendItems.clear();

    //Load CheckBeforeSend
    const QVector<PimCommon::PluginUtilData> lstPluginCheckBeforeSend = MessageComposer::PluginEditorCheckBeforeSendManager::self()->pluginsDataList();
    if (!lstPluginCheckBeforeSend.isEmpty()) {
        QTreeWidgetItem *topLevel = new QTreeWidgetItem(mListWidget, QStringList() << i18n("Check Before Send Plugins"));
        topLevel->setFlags(topLevel->flags() & ~Qt::ItemIsSelectable);
        const QPair<QStringList, QStringList> pair = PimCommon::PluginUtil::loadPluginSetting(MessageComposer::PluginEditorCheckBeforeSendManager::self()->configGroupName(),
                MessageComposer::PluginEditorCheckBeforeSendManager::self()->configPrefixSettingKey());
        Q_FOREACH (const PimCommon::PluginUtilData &data, lstPluginCheckBeforeSend) {
            PluginItem *subItem = new PluginItem(topLevel);
            subItem->setText(0, data.mName);
            subItem->mIdentifier = data.mIdentifier;
            subItem->mDescription = data.mDescription;
            subItem->mEnableByDefault = data.mEnableByDefault;
            const bool isPluginActivated = PimCommon::PluginUtil::isPluginActivated(pair.first, pair.second, data.mEnableByDefault, data.mIdentifier);
            subItem->setCheckState(0, isPluginActivated ? Qt::Checked : Qt::Unchecked);
            mPluginSendBeforeSendItems.append(subItem);
        }
    }
    //Load generic plugins
    //TODO PimCommon::GenericPluginManager::self()->pluginsDataList()

    //Load plugin editor
    const QVector<PimCommon::PluginUtilData> lstPluginEditor = MessageComposer::PluginEditorManager::self()->pluginsDataList();
    if (!lstPluginEditor.isEmpty()) {
        QTreeWidgetItem *topLevel = new QTreeWidgetItem(mListWidget, QStringList() << i18n("Composer Plugins"));
        topLevel->setFlags(topLevel->flags() & ~Qt::ItemIsSelectable);
        const QPair<QStringList, QStringList> pair = PimCommon::PluginUtil::loadPluginSetting(MessageComposer::PluginEditorManager::self()->configGroupName(),
                MessageComposer::PluginEditorManager::self()->configPrefixSettingKey());
        Q_FOREACH (const PimCommon::PluginUtilData &data, lstPluginEditor) {
            PluginItem *subItem = new PluginItem(topLevel);
            subItem->setText(0, data.mName);
            subItem->mIdentifier = data.mIdentifier;
            subItem->mDescription = data.mDescription;
            subItem->mEnableByDefault = data.mEnableByDefault;
            const bool isPluginActivated = PimCommon::PluginUtil::isPluginActivated(pair.first, pair.second, data.mEnableByDefault, data.mIdentifier);
            subItem->setCheckState(0, isPluginActivated ? Qt::Checked : Qt::Unchecked);
            mPluginEditorItems.append(subItem);
        }
    }

    //Load messageviewer plugin
    const QVector<PimCommon::PluginUtilData> lstMessageViewerPluginData = MessageViewer::ViewerPluginManager::self()->pluginsDataList();
    if (!lstMessageViewerPluginData.isEmpty()) {
        QTreeWidgetItem *topLevel = new QTreeWidgetItem(mListWidget, QStringList() << i18n("Message Viewer"));
        topLevel->setFlags(topLevel->flags() & ~Qt::ItemIsSelectable);
        const QPair<QStringList, QStringList> pair = PimCommon::PluginUtil::loadPluginSetting(MessageViewer::ViewerPluginManager::self()->configGroupName(),
                MessageViewer::ViewerPluginManager::self()->configPrefixSettingKey());
        Q_FOREACH (const PimCommon::PluginUtilData &data, lstMessageViewerPluginData) {
            PluginItem *subItem = new PluginItem(topLevel);
            subItem->setText(0, data.mName);
            subItem->mIdentifier = data.mIdentifier;
            subItem->mDescription = data.mDescription;
            subItem->mEnableByDefault = data.mEnableByDefault;
            const bool isPluginActivated = PimCommon::PluginUtil::isPluginActivated(pair.first, pair.second, data.mEnableByDefault, data.mIdentifier);
            subItem->setCheckState(0, isPluginActivated ? Qt::Checked : Qt::Unchecked);
            mPluginMessageViewerItems.append(subItem);
        }
    }

    //Load webengineplugin
    //TODO
    mListWidget->expandAll();
}
