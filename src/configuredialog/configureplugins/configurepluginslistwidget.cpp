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

#include <PimCommon/PluginUtil>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KLocalizedString>
#include <MessageComposer/PluginEditorManager>

#include <QHBoxLayout>
#include <QLabel>
#include <QTreeWidget>


ConfigurePluginsListWidget::ConfigurePluginsListWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setObjectName(QStringLiteral("mainlayout"));
    mainLayout->setMargin(0);

    mListWidget = new QTreeWidget(this);
    mListWidget->setObjectName(QStringLiteral("listwidget"));
    mListWidget->setHeaderHidden(true);
    mListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(mListWidget, &QTreeWidget::itemSelectionChanged, this, &ConfigurePluginsListWidget::slotItemSelectionChanged);
    connect(mListWidget, &QTreeWidget::itemChanged, this, &ConfigurePluginsListWidget::slotItemChanged);

    mainLayout->addWidget(mListWidget);

}

ConfigurePluginsListWidget::~ConfigurePluginsListWidget()
{
}

void ConfigurePluginsListWidget::slotItemChanged(QTreeWidgetItem * item, int column)
{
    Q_UNUSED(item);
    if (column == 0) {
        Q_EMIT changed();
    }
}

void ConfigurePluginsListWidget::slotItemSelectionChanged()
{
    QTreeWidgetItem *item = mListWidget->currentItem();
    if (PluginItem *pluginItem = dynamic_cast<PluginItem *>(item)) {
        Q_EMIT descriptionChanged(pluginItem->mDescription);
    }
}

void ConfigurePluginsListWidget::save()
{
    QStringList enabledPlugins;
    QStringList disabledPlugins;
    Q_FOREACH(PluginItem *item, mPluginEditorItems) {
        if (item->checkState(0) == Qt::Checked) {
            enabledPlugins << item->mIdentifier;
        } else {
            disabledPlugins << item->mIdentifier;
        }
    }
    PimCommon::PluginUtil::savePluginSettings(MessageComposer::PluginEditorManager::self()->configGroupName(),
                                              MessageComposer::PluginEditorManager::self()->configPrefixSettingKey(),
                                              enabledPlugins, disabledPlugins);

    enabledPlugins.clear();
    disabledPlugins.clear();
    Q_FOREACH(PluginItem *item, mPluginMessageViewerItems) {
        if (item->checkState(0) == Qt::Checked) {
            enabledPlugins << item->mIdentifier;
        } else {
            disabledPlugins << item->mIdentifier;
        }
    }
    PimCommon::PluginUtil::savePluginSettings(MessageViewer::ViewerPluginManager::self()->configGroupName(),
                                              MessageViewer::ViewerPluginManager::self()->configPrefixSettingKey(),
                                              enabledPlugins, disabledPlugins);
}

void ConfigurePluginsListWidget::doLoadFromGlobalSettings()
{
    initialize();
}

void ConfigurePluginsListWidget::doResetToDefaultsOther()
{
    Q_FOREACH(PluginItem *item, mPluginEditorItems) {
        item->setCheckState(0, item->mEnableByDefault ? Qt::Checked : Qt::Unchecked);
    }

    Q_FOREACH(PluginItem *item, mPluginMessageViewerItems) {
        item->setCheckState(0, item->mEnableByDefault ? Qt::Checked : Qt::Unchecked);
    }
}


void ConfigurePluginsListWidget::initialize()
{
    mListWidget->clear();
    mPluginMessageViewerItems.clear();
    mPluginEditorItems.clear();

    //Load generic plugins

    //Load plugin editor
    const QVector<PimCommon::PluginUtilData> lstPluginEditor = MessageComposer::PluginEditorManager::self()->pluginsDataList();
    if (!lstPluginEditor.isEmpty()) {
        QTreeWidgetItem *topLevel = new QTreeWidgetItem(mListWidget, QStringList() << i18n("Composer Plugins"));
        topLevel->setFlags(topLevel->flags() & ~Qt::ItemIsSelectable);
        const QPair<QStringList, QStringList> pair = PimCommon::PluginUtil::loadPluginSetting(MessageComposer::PluginEditorManager::self()->configGroupName(),
                                                                                              MessageComposer::PluginEditorManager::self()->configPrefixSettingKey());
        Q_FOREACH(const PimCommon::PluginUtilData &data, lstPluginEditor) {
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
        Q_FOREACH(const PimCommon::PluginUtilData &data, lstMessageViewerPluginData) {
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
