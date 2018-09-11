/*
  Copyright (C) 2016-2018 Montel Laurent <montel@kde.org>

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

#include "kmail_debug.h"
#include "util.h"
#include "../../plugininterface/kmailplugininterface.h"
#include <MessageViewer/ViewerPluginManager>
#include <MessageViewer/HeaderStylePluginManager>
#include <MessageComposer/PluginEditorCheckBeforeSendManager>
#include <MessageComposer/PluginEditorInitManager>
#include <MessageComposer/PluginEditorConvertTextManager>
#include <MessageComposer/PluginEditorConvertText>
#include <WebEngineViewer/NetworkUrlInterceptorPluginManager>
#include <PimCommon/GenericPluginManager>
#include <AkonadiCore/ServerManager>
#include <PimCommon/PluginUtil>
#include <KSharedConfig>
#include <KLocalizedString>
#include <MessageComposer/PluginEditorManager>

#include <QDBusInterface>
#include <QDBusReply>
#include <MessageComposer/PluginEditorInit>
#include <WebEngineViewer/NetworkPluginUrlInterceptor>
#include <MessageComposer/PluginEditorCheckBeforeSend>
#include <MessageViewer/ViewerPlugin>
#include <PimCommon/GenericPlugin>
#include <MessageComposer/PluginEditor>
#include <MessageViewer/HeaderStylePlugin>
#include <AkonadiCore/AgentManager>
#include <AkonadiCore/AgentInstance>

namespace {
QString pluginEditorGroupName()
{
    return QStringLiteral("plugineditorgroupname");
}

QString viewerPluginGroupName()
{
    return QStringLiteral("viewerplugingroupname");
}

QString pluginEditorCheckBeforeGroupName()
{
    return QStringLiteral("plugineditorcheckbeforegroupname");
}

QString pluginEditorInitGroupName()
{
    return QStringLiteral("plugineditorinitgroupname");
}

QString pluginEditorConvertTextGroupName()
{
    return QStringLiteral("plugineditorconvertTextgroupname");
}

QString kmailPluginToolsGroupName()
{
    return QStringLiteral("kmailplugintoolsgroupname");
}

QString networkUrlInterceptorGroupName()
{
    return QStringLiteral("networkurlinterceptorgroupname");
}

QString headerStyleGroupName()
{
    return QStringLiteral("headerstylegroupname");
}

QString agentAkonadiGroupName()
{
    return QStringLiteral("agentakonadigroupname");
}
}

ConfigurePluginsListWidget::ConfigurePluginsListWidget(QWidget *parent)
    : PimCommon::ConfigurePluginsListWidget(parent)
{
    connect(this, &ConfigurePluginsListWidget::configureClicked, this, &ConfigurePluginsListWidget::slotConfigureClicked);
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
    PimCommon::ConfigurePluginsListWidget::savePlugins(MessageComposer::PluginEditorInitManager::self()->configGroupName(),
                                                       MessageComposer::PluginEditorInitManager::self()->configPrefixSettingKey(),
                                                       mPluginEditorInitItems);
    PimCommon::ConfigurePluginsListWidget::savePlugins(MessageComposer::PluginEditorCheckBeforeSendManager::self()->configGroupName(),
                                                       MessageComposer::PluginEditorCheckBeforeSendManager::self()->configPrefixSettingKey(),
                                                       mPluginCheckBeforeSendItems);
    PimCommon::ConfigurePluginsListWidget::savePlugins(KMailPluginInterface::self()->configGroupName(),
                                                       KMailPluginInterface::self()->configPrefixSettingKey(),
                                                       mPluginGenericItems);
    PimCommon::ConfigurePluginsListWidget::savePlugins(WebEngineViewer::NetworkUrlInterceptorPluginManager::self()->configGroupName(),
                                                       WebEngineViewer::NetworkUrlInterceptorPluginManager::self()->configPrefixSettingKey(),
                                                       mPluginWebEngineItems);
    PimCommon::ConfigurePluginsListWidget::savePlugins(MessageViewer::HeaderStylePluginManager::self()->configGroupName(),
                                                       MessageViewer::HeaderStylePluginManager::self()->configPrefixSettingKey(),
                                                       mPluginHeaderStyleItems);
    PimCommon::ConfigurePluginsListWidget::savePlugins(MessageComposer::PluginEditorConvertTextManager::self()->configGroupName(),
                                                       MessageComposer::PluginEditorConvertTextManager::self()->configPrefixSettingKey(),
                                                       mPluginConvertTextItems);
    saveAkonadiAgent();
}

void ConfigurePluginsListWidget::saveAkonadiAgent()
{
    for (PluginItem *item : qAsConst(mAgentPluginsItems)) {
        for (const PimCommon::PluginUtilData &data : qAsConst(mPluginUtilDataList)) {
            if (item->mIdentifier == data.mIdentifier) {
                changeAgentActiveState(data.mExtraInfo.at(0), data.mExtraInfo.at(1), item->checkState(0) == Qt::Checked);
                break;
            }
        }
    }
}

void ConfigurePluginsListWidget::doLoadFromGlobalSettings()
{
    initialize();
}

void ConfigurePluginsListWidget::doResetToDefaultsOther()
{
    changeState(mPluginEditorItems);
    changeState(mPluginMessageViewerItems);
    changeState(mPluginCheckBeforeSendItems);
    changeState(mPluginGenericItems);
    changeState(mPluginWebEngineItems);
    changeState(mPluginHeaderStyleItems);
    changeState(mAgentPluginsItems);
    changeState(mPluginEditorInitItems);
    changeState(mPluginConvertTextItems);
}

void ConfigurePluginsListWidget::initialize()
{
    mListWidget->clear();

    //Load CheckBeforeSend
    PimCommon::ConfigurePluginsListWidget::fillTopItems(MessageComposer::PluginEditorCheckBeforeSendManager::self()->pluginsDataList(),
                                                        i18n("Check Before Send Plugins"),
                                                        MessageComposer::PluginEditorCheckBeforeSendManager::self()->configGroupName(),
                                                        MessageComposer::PluginEditorCheckBeforeSendManager::self()->configPrefixSettingKey(),
                                                        mPluginCheckBeforeSendItems,
                                                        pluginEditorCheckBeforeGroupName());

    PimCommon::ConfigurePluginsListWidget::fillTopItems(MessageComposer::PluginEditorInitManager::self()->pluginsDataList(),
                                                        i18n("Composer Plugins"),
                                                        MessageComposer::PluginEditorInitManager::self()->configGroupName(),
                                                        MessageComposer::PluginEditorInitManager::self()->configPrefixSettingKey(),
                                                        mPluginEditorInitItems,
                                                        pluginEditorInitGroupName());

    //Load generic plugins
    //Necessary to initialize pluging when we load it outside kmail
    KMailPluginInterface::self()->initializePlugins();
    PimCommon::ConfigurePluginsListWidget::fillTopItems(KMailPluginInterface::self()->pluginsDataList(),
                                                        i18n("Tools Plugins"),
                                                        KMailPluginInterface::self()->configGroupName(),
                                                        KMailPluginInterface::self()->configPrefixSettingKey(),
                                                        mPluginGenericItems,
                                                        kmailPluginToolsGroupName());

    //Load plugin editor
    PimCommon::ConfigurePluginsListWidget::fillTopItems(MessageComposer::PluginEditorManager::self()->pluginsDataList(),
                                                        i18n("Editor Plugins"),
                                                        MessageComposer::PluginEditorManager::self()->configGroupName(),
                                                        MessageComposer::PluginEditorManager::self()->configPrefixSettingKey(),
                                                        mPluginEditorItems,
                                                        pluginEditorGroupName());

    //Load messageviewer plugin
    PimCommon::ConfigurePluginsListWidget::fillTopItems(MessageViewer::ViewerPluginManager::self()->pluginsDataList(),
                                                        i18n("Message Viewer"),
                                                        MessageViewer::ViewerPluginManager::self()->configGroupName(),
                                                        MessageViewer::ViewerPluginManager::self()->configPrefixSettingKey(),
                                                        mPluginMessageViewerItems,
                                                        viewerPluginGroupName());

    //Load webengineplugin
    PimCommon::ConfigurePluginsListWidget::fillTopItems(WebEngineViewer::NetworkUrlInterceptorPluginManager::self()->pluginsDataList(),
                                                        i18n("Webengine Plugins"),
                                                        WebEngineViewer::NetworkUrlInterceptorPluginManager::self()->configGroupName(),
                                                        WebEngineViewer::NetworkUrlInterceptorPluginManager::self()->configPrefixSettingKey(),
                                                        mPluginWebEngineItems,
                                                        networkUrlInterceptorGroupName());

    //Load headerstyle
    PimCommon::ConfigurePluginsListWidget::fillTopItems(MessageViewer::HeaderStylePluginManager::self()->pluginsDataList(),
                                                        i18n("Header Style Plugins"),
                                                        MessageViewer::HeaderStylePluginManager::self()->configGroupName(),
                                                        MessageViewer::HeaderStylePluginManager::self()->configPrefixSettingKey(),
                                                        mPluginHeaderStyleItems,
                                                        headerStyleGroupName());
//    //Load headerstyle
    PimCommon::ConfigurePluginsListWidget::fillTopItems(MessageComposer::PluginEditorConvertTextManager::self()->pluginsDataList(),
                                                        i18n("Convertor Text Plugins"),
                                                        MessageComposer::PluginEditorConvertTextManager::self()->configGroupName(),
                                                        MessageComposer::PluginEditorConvertTextManager::self()->configPrefixSettingKey(),
                                                        mPluginConvertTextItems,
                                                        pluginEditorConvertTextGroupName());

    //Load Agent Plugin
    initializeAgentPlugins();
    mListWidget->expandAll();
}

void ConfigurePluginsListWidget::initializeAgentPlugins()
{
    mPluginUtilDataList.clear();
    mPluginUtilDataList.reserve(5);
    mPluginUtilDataList << createAgentPluginData(QStringLiteral("akonadi_sendlater_agent"), QStringLiteral("/SendLaterAgent"));
    mPluginUtilDataList << createAgentPluginData(QStringLiteral("akonadi_archivemail_agent"), QStringLiteral("/ArchiveMailAgent"));
    mPluginUtilDataList << createAgentPluginData(QStringLiteral("akonadi_newmailnotifier_agent"), QStringLiteral("/NewMailNotifierAgent"));
    mPluginUtilDataList << createAgentPluginData(QStringLiteral("akonadi_followupreminder_agent"), QStringLiteral("/FollowUpReminder"));
    mPluginUtilDataList << createAgentPluginData(QStringLiteral("akonadi_unifiedmailbox_agent"), QStringLiteral("/UnifiedMailboxAgent"));

    PimCommon::ConfigurePluginsListWidget::fillTopItems(mPluginUtilDataList,
                                                        i18n("Akonadi Agents"),
                                                        QString(),
                                                        QString(),
                                                        mAgentPluginsItems,
                                                        agentAkonadiGroupName());
}

PimCommon::PluginUtilData ConfigurePluginsListWidget::createAgentPluginData(const QString &agentIdentifier, const QString &path)
{
    PimCommon::PluginUtilData data;
    data.mEnableByDefault = true;
    data.mHasConfigureDialog = true;
    const Akonadi::AgentType::List lstAgent = Akonadi::AgentManager::self()->types();
    for (const Akonadi::AgentType &type : lstAgent) {
        if (type.identifier() == agentIdentifier) {
            data.mExtraInfo << agentIdentifier;
            data.mExtraInfo << path;
            const bool enabled = agentActivateState(agentIdentifier, path);
            data.mEnableByDefault = enabled;
            data.mName = type.name();
            data.mDescription = type.description();
            data.mIdentifier = type.identifier();
            break;
        }
    }
    return data;
}

bool ConfigurePluginsListWidget::agentActivateState(const QString &agentIdentifier, const QString &pathName)
{
    const QString service
        = Akonadi::ServerManager::agentServiceName(Akonadi::ServerManager::Agent, agentIdentifier);
    QDBusInterface interface(service, pathName);
    if (interface.isValid()) {
        QDBusReply<bool> enabled = interface.call(QStringLiteral("enabledAgent"));
        if (enabled.isValid()) {
            return enabled;
        } else {
            qCDebug(KMAIL_LOG) << agentIdentifier << "doesn't have enabledAgent function";
            return false;
        }
    } else {
        qCDebug(KMAIL_LOG) << agentIdentifier << "does not exist when trying to activate the agent state";
    }
    return false;
}

void ConfigurePluginsListWidget::changeAgentActiveState(const QString &agentIdentifier, const QString &path, bool enable)
{
    if (!agentIdentifier.isEmpty() && !path.isEmpty()) {
        const QString service
            = Akonadi::ServerManager::agentServiceName(Akonadi::ServerManager::Agent, agentIdentifier);
        QDBusInterface interface(service, path);
        if (interface.isValid()) {
            interface.call(QStringLiteral("setEnableAgent"), enable);
        } else {
            qCDebug(KMAIL_LOG) << agentIdentifier << "does not exist when trying to change the agent active state";
        }
    }
}

void ConfigurePluginsListWidget::slotConfigureClicked(const QString &configureGroupName, const QString &identifier)
{
    if (!configureGroupName.isEmpty() && !identifier.isEmpty()) {
        if (configureGroupName == headerStyleGroupName()) {
            MessageViewer::HeaderStylePlugin *plugin = MessageViewer::HeaderStylePluginManager::self()->pluginFromIdentifier(identifier);
            plugin->showConfigureDialog(this);
        } else if (configureGroupName == networkUrlInterceptorGroupName()) {
            WebEngineViewer::NetworkPluginUrlInterceptor *plugin = WebEngineViewer::NetworkUrlInterceptorPluginManager::self()->pluginFromIdentifier(identifier);
            plugin->showConfigureDialog(this);
        } else if (configureGroupName == viewerPluginGroupName()) {
            MessageViewer::ViewerPlugin *plugin = MessageViewer::ViewerPluginManager::self()->pluginFromIdentifier(identifier);
            plugin->showConfigureDialog(this);
        } else if (configureGroupName == pluginEditorGroupName()) {
            MessageComposer::PluginEditor *plugin = MessageComposer::PluginEditorManager::self()->pluginFromIdentifier(identifier);
            plugin->showConfigureDialog(this);
        } else if (configureGroupName == kmailPluginToolsGroupName()) {
            PimCommon::GenericPlugin *plugin = KMailPluginInterface::self()->pluginFromIdentifier(identifier);
            plugin->showConfigureDialog(this);
        } else if (configureGroupName == pluginEditorInitGroupName()) {
            MessageComposer::PluginEditorInit *plugin = MessageComposer::PluginEditorInitManager::self()->pluginFromIdentifier(identifier);
            plugin->showConfigureDialog(this);
        } else if (configureGroupName == pluginEditorCheckBeforeGroupName()) {
            MessageComposer::PluginEditorCheckBeforeSend *plugin = MessageComposer::PluginEditorCheckBeforeSendManager::self()->pluginFromIdentifier(identifier);
            plugin->showConfigureDialog(this);
        } else if (configureGroupName == pluginEditorConvertTextGroupName()) {
            MessageComposer::PluginEditorConvertText *plugin = MessageComposer::PluginEditorConvertTextManager::self()->pluginFromIdentifier(identifier);
            plugin->showConfigureDialog(this);
        } else if (configureGroupName == agentAkonadiGroupName()) {
            for (const PimCommon::PluginUtilData &data : qAsConst(mPluginUtilDataList)) {
                if (data.mIdentifier == identifier) {
                    auto instance = Akonadi::AgentManager::self()->instance(identifier);
                    if (instance.isValid()) {
                        instance.configure(this);
                    }
                    break;
                }
            }
        } else {
            qCWarning(KMAIL_LOG) << "Unknown configureGroupName" << configureGroupName;
        }
    }
}

void ConfigurePluginsListWidget::defaults()
{
    doResetToDefaultsOther();
}
