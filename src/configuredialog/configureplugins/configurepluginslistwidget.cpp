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
#include "../../plugininterface/kmailplugininterface.h"
#include <MessageViewer/ViewerPluginManager>
#include <MessageViewer/HeaderStylePluginManager>
#include <MessageComposer/PluginEditorCheckBeforeSendManager>
#include <WebEngineViewer/NetworkUrlInterceptorPluginManager>
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

void ConfigurePluginsListWidget::save()
{
    PimCommon::ConfigurePluginsListWidget::savePlugins(MessageComposer::PluginEditorManager::self()->configGroupName(),
            MessageComposer::PluginEditorManager::self()->configPrefixSettingKey(),
            mPluginEditorItems);
    PimCommon::ConfigurePluginsListWidget::savePlugins(MessageViewer::ViewerPluginManager::self()->configGroupName(),
            MessageViewer::ViewerPluginManager::self()->configPrefixSettingKey(),
            mPluginMessageViewerItems);
    PimCommon::ConfigurePluginsListWidget::savePlugins(MessageComposer::PluginEditorCheckBeforeSendManager::self()->configGroupName(),
            MessageComposer::PluginEditorCheckBeforeSendManager::self()->configPrefixSettingKey(),
            mPluginSendBeforeSendItems);
    PimCommon::ConfigurePluginsListWidget::savePlugins(KMailPluginInterface::self()->configGroupName(),
            KMailPluginInterface::self()->configPrefixSettingKey(),
            mPluginGenericItems);
    PimCommon::ConfigurePluginsListWidget::savePlugins(WebEngineViewer::NetworkUrlInterceptorPluginManager::self()->configGroupName(),
            WebEngineViewer::NetworkUrlInterceptorPluginManager::self()->configPrefixSettingKey(),
            mPluginWebEngineItems);
    PimCommon::ConfigurePluginsListWidget::savePlugins(MessageViewer::HeaderStylePluginManager::self()->configGroupName(),
            MessageViewer::HeaderStylePluginManager::self()->configPrefixSettingKey(),
            mPluginHeaderStyleItems);
}

void ConfigurePluginsListWidget::doLoadFromGlobalSettings()
{
    initialize();
}

void ConfigurePluginsListWidget::doResetToDefaultsOther()
{
    changeState(mPluginEditorItems);
    changeState(mPluginMessageViewerItems);
    changeState(mPluginSendBeforeSendItems);
    changeState(mPluginGenericItems);
    changeState(mPluginWebEngineItems);
    changeState(mPluginHeaderStyleItems);
}

void ConfigurePluginsListWidget::initialize()
{
    mListWidget->clear();

    //Load CheckBeforeSend
    PimCommon::ConfigurePluginsListWidget::fillTopItems(MessageComposer::PluginEditorCheckBeforeSendManager::self()->pluginsDataList(), i18n("Check Before Send Plugins"),
            MessageComposer::PluginEditorCheckBeforeSendManager::self()->configGroupName(),
            MessageComposer::PluginEditorCheckBeforeSendManager::self()->configPrefixSettingKey(), mPluginSendBeforeSendItems);

    //Load generic plugins
    //Necessary to initialize pluging when we load it outside kmail
    KMailPluginInterface::self()->initializePlugins();
    PimCommon::ConfigurePluginsListWidget::fillTopItems(KMailPluginInterface::self()->pluginsDataList(), i18n("Tools Plugins"),
            KMailPluginInterface::self()->configGroupName(),
            KMailPluginInterface::self()->configPrefixSettingKey(), mPluginGenericItems);

    //Load plugin editor
    PimCommon::ConfigurePluginsListWidget::fillTopItems(MessageComposer::PluginEditorManager::self()->pluginsDataList(), i18n("Composer Plugins"),
            MessageComposer::PluginEditorManager::self()->configGroupName(),
            MessageComposer::PluginEditorManager::self()->configPrefixSettingKey(), mPluginEditorItems);

    //Load messageviewer plugin
    PimCommon::ConfigurePluginsListWidget::fillTopItems(MessageViewer::ViewerPluginManager::self()->pluginsDataList(), i18n("Message Viewer"),
            MessageViewer::ViewerPluginManager::self()->configGroupName(),
            MessageViewer::ViewerPluginManager::self()->configPrefixSettingKey(), mPluginMessageViewerItems);

    //Load webengineplugin
    PimCommon::ConfigurePluginsListWidget::fillTopItems(WebEngineViewer::NetworkUrlInterceptorPluginManager::self()->pluginsDataList(), i18n("Webengine Plugins"),
            WebEngineViewer::NetworkUrlInterceptorPluginManager::self()->configGroupName(),
            WebEngineViewer::NetworkUrlInterceptorPluginManager::self()->configPrefixSettingKey(), mPluginWebEngineItems);

    //Load headerstyle
    PimCommon::ConfigurePluginsListWidget::fillTopItems(MessageViewer::HeaderStylePluginManager::self()->pluginsDataList(), i18n("Header Style Plugins"),
            MessageViewer::HeaderStylePluginManager::self()->configGroupName(),
            MessageViewer::HeaderStylePluginManager::self()->configPrefixSettingKey(), mPluginHeaderStyleItems);
    mListWidget->expandAll();
}
