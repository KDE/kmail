/*
  SPDX-FileCopyrightText: 2016-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "configurepluginslistwidget.h"

#include "../../plugininterface/kmailplugininterface.h"
#include "kmail_debug.h"
#include <Akonadi/ServerManager>
#include <KLocalizedString>
#include <KSharedConfig>
#include <MessageComposer/PluginEditorCheckBeforeSendManager>
#include <MessageComposer/PluginEditorConvertText>
#include <MessageComposer/PluginEditorConvertTextManager>
#include <MessageComposer/PluginEditorGrammarManager>
#include <MessageComposer/PluginEditorInitManager>
#include <MessageComposer/PluginEditorManager>
#include <MessageViewer/HeaderStylePluginManager>
#include <MessageViewer/MessageViewerCheckBeforeDeletingPlugin>
#include <MessageViewer/MessageViewerCheckBeforeDeletingPluginManager>
#include <MessageViewer/ViewerPluginManager>
#include <PimCommon/CustomToolsPlugin>
#include <PimCommon/GenericPluginManager>
#include <WebEngineViewer/NetworkUrlInterceptorPluginManager>

#include <Akonadi/AgentConfigurationDialog>
#include <Akonadi/AgentInstance>
#include <Akonadi/AgentManager>
#include <MessageComposer/PluginEditor>
#include <MessageComposer/PluginEditorCheckBeforeSend>
#include <MessageComposer/PluginEditorInit>
#include <MessageViewer/HeaderStylePlugin>
#include <MessageViewer/MessageViewerConfigureSettingsPlugin>
#include <MessageViewer/MessageViewerConfigureSettingsPluginManager>
#include <MessageViewer/ViewerPlugin>
#include <PimCommon/GenericPlugin>
#include <QDBusInterface>
#include <QDBusReply>
#include <QPointer>
#include <WebEngineViewer/NetworkPluginUrlInterceptor>

namespace
{
QString pluginEditorCheckBeforeDeletingGroupName()
{
    return QStringLiteral("plugincheckbeforedeletinggroupname");
}

QString pluginEditorGroupName()
{
    return QStringLiteral("plugineditorgroupname");
}

QString pluginEditorGrammarGroupName()
{
    return QStringLiteral("plugineditorgrammargroupname");
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

QString configurePluginGroupName()
{
    return QStringLiteral("configuregroupname");
}
}

ConfigurePluginsListWidget::ConfigurePluginsListWidget(QWidget *parent)
    : TextAddonsWidgets::ConfigurePluginsWidget(parent)
{
    connect(this, &TextAddonsWidgets::ConfigurePluginsWidget::configureClicked, this, &ConfigurePluginsListWidget::slotConfigureClicked);
}

ConfigurePluginsListWidget::~ConfigurePluginsListWidget() = default;

void ConfigurePluginsListWidget::save()
{
    TextAddonsWidgets::ConfigurePluginsWidget::savePlugins(MessageComposer::PluginEditorManager::self()->configGroupName(),
                                                           MessageComposer::PluginEditorManager::self()->configPrefixSettingKey(),
                                                           mPluginEditorItems,
                                                           PimCommon::PluginUtil::pluginConfigFile());
    TextAddonsWidgets::ConfigurePluginsWidget::savePlugins(MessageViewer::ViewerPluginManager::self()->configGroupName(),
                                                           MessageViewer::ViewerPluginManager::self()->configPrefixSettingKey(),
                                                           mPluginMessageViewerItems,
                                                           PimCommon::PluginUtil::pluginConfigFile());
    TextAddonsWidgets::ConfigurePluginsWidget::savePlugins(MessageComposer::PluginEditorInitManager::self()->configGroupName(),
                                                           MessageComposer::PluginEditorInitManager::self()->configPrefixSettingKey(),
                                                           mPluginEditorInitItems,
                                                           PimCommon::PluginUtil::pluginConfigFile());
    TextAddonsWidgets::ConfigurePluginsWidget::savePlugins(MessageComposer::PluginEditorCheckBeforeSendManager::self()->configGroupName(),
                                                           MessageComposer::PluginEditorCheckBeforeSendManager::self()->configPrefixSettingKey(),
                                                           mPluginCheckBeforeSendItems,
                                                           PimCommon::PluginUtil::pluginConfigFile());
    TextAddonsWidgets::ConfigurePluginsWidget::savePlugins(MessageComposer::PluginEditorGrammarManager::self()->configGroupName(),
                                                           MessageComposer::PluginEditorGrammarManager::self()->configPrefixSettingKey(),
                                                           mPluginEditorGrammarItems,
                                                           PimCommon::PluginUtil::pluginConfigFile());
    TextAddonsWidgets::ConfigurePluginsWidget::savePlugins(KMailPluginInterface::self()->configGroupName(),
                                                           KMailPluginInterface::self()->configPrefixSettingKey(),
                                                           mPluginGenericItems,
                                                           PimCommon::PluginUtil::pluginConfigFile());
    TextAddonsWidgets::ConfigurePluginsWidget::savePlugins(WebEngineViewer::NetworkUrlInterceptorPluginManager::self()->configGroupName(),
                                                           WebEngineViewer::NetworkUrlInterceptorPluginManager::self()->configPrefixSettingKey(),
                                                           mPluginWebEngineItems,
                                                           PimCommon::PluginUtil::pluginConfigFile());
    TextAddonsWidgets::ConfigurePluginsWidget::savePlugins(MessageViewer::HeaderStylePluginManager::self()->configGroupName(),
                                                           MessageViewer::HeaderStylePluginManager::self()->configPrefixSettingKey(),
                                                           mPluginHeaderStyleItems,
                                                           PimCommon::PluginUtil::pluginConfigFile());
    TextAddonsWidgets::ConfigurePluginsWidget::savePlugins(MessageComposer::PluginEditorConvertTextManager::self()->configGroupName(),
                                                           MessageComposer::PluginEditorConvertTextManager::self()->configPrefixSettingKey(),
                                                           mPluginConvertTextItems,
                                                           PimCommon::PluginUtil::pluginConfigFile());
    TextAddonsWidgets::ConfigurePluginsWidget::savePlugins(MessageViewer::MessageViewerCheckBeforeDeletingPluginManager::self()->configGroupName(),
                                                           MessageViewer::MessageViewerCheckBeforeDeletingPluginManager::self()->configPrefixSettingKey(),
                                                           mPluginCheckBeforeDeletingItems,
                                                           PimCommon::PluginUtil::pluginConfigFile());
    saveAkonadiAgent();
}

void ConfigurePluginsListWidget::saveAkonadiAgent()
{
    for (PluginItem *item : std::as_const(mAgentPluginsItems)) {
        for (const TextAddonsWidgets::PluginUtilData &data : std::as_const(mPluginUtilDataList)) {
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
    initializeDone();
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
    changeState(mPluginEditorGrammarItems);
    changeState(mPluginCheckBeforeDeletingItems);
}

void ConfigurePluginsListWidget::initialize()
{
    mTreePluginWidget->clear();

    // Load CheckBeforeSend
    TextAddonsWidgets::ConfigurePluginsWidget::fillTopItems(MessageComposer::PluginEditorCheckBeforeSendManager::self()->pluginsDataList(),
                                                            i18n("Check Before Send Plugins"),
                                                            MessageComposer::PluginEditorCheckBeforeSendManager::self()->configGroupName(),
                                                            MessageComposer::PluginEditorCheckBeforeSendManager::self()->configPrefixSettingKey(),
                                                            mPluginCheckBeforeSendItems,
                                                            pluginEditorCheckBeforeGroupName(),
                                                            true,
                                                            PimCommon::PluginUtil::pluginConfigFile());

    TextAddonsWidgets::ConfigurePluginsWidget::fillTopItems(MessageComposer::PluginEditorInitManager::self()->pluginsDataList(),
                                                            i18n("Composer Plugins"),
                                                            MessageComposer::PluginEditorInitManager::self()->configGroupName(),
                                                            MessageComposer::PluginEditorInitManager::self()->configPrefixSettingKey(),
                                                            mPluginEditorInitItems,
                                                            pluginEditorInitGroupName(),
                                                            true,
                                                            PimCommon::PluginUtil::pluginConfigFile());
    TextAddonsWidgets::ConfigurePluginsWidget::fillTopItems(MessageComposer::PluginEditorGrammarManager::self()->pluginsDataList(),
                                                            i18n("Grammar Checker Plugins"),
                                                            MessageComposer::PluginEditorGrammarManager::self()->configGroupName(),
                                                            MessageComposer::PluginEditorGrammarManager::self()->configPrefixSettingKey(),
                                                            mPluginEditorGrammarItems,
                                                            pluginEditorGrammarGroupName(),
                                                            true,
                                                            PimCommon::PluginUtil::pluginConfigFile());

    // Load generic plugins
    // Necessary to initialize plugin when we load it outside kmail
    KMailPluginInterface::self()->initializePlugins();
    TextAddonsWidgets::ConfigurePluginsWidget::fillTopItems(KMailPluginInterface::self()->pluginsDataList(),
                                                            i18n("Tools Plugins"),
                                                            KMailPluginInterface::self()->configGroupName(),
                                                            KMailPluginInterface::self()->configPrefixSettingKey(),
                                                            mPluginGenericItems,
                                                            kmailPluginToolsGroupName(),
                                                            true,
                                                            PimCommon::PluginUtil::pluginConfigFile());
    // Load plugin editor
    TextAddonsWidgets::ConfigurePluginsWidget::fillTopItems(MessageComposer::PluginEditorManager::self()->pluginsDataList(),
                                                            i18n("Editor Plugins"),
                                                            MessageComposer::PluginEditorManager::self()->configGroupName(),
                                                            MessageComposer::PluginEditorManager::self()->configPrefixSettingKey(),
                                                            mPluginEditorItems,
                                                            pluginEditorGroupName());

    // Load messageviewer plugin
    TextAddonsWidgets::ConfigurePluginsWidget::fillTopItems(MessageViewer::ViewerPluginManager::self()->pluginsDataList(),
                                                            i18n("Message Viewer"),
                                                            MessageViewer::ViewerPluginManager::self()->configGroupName(),
                                                            MessageViewer::ViewerPluginManager::self()->configPrefixSettingKey(),
                                                            mPluginMessageViewerItems,
                                                            viewerPluginGroupName(),
                                                            true,
                                                            PimCommon::PluginUtil::pluginConfigFile());

    // Load webengineplugin
    TextAddonsWidgets::ConfigurePluginsWidget::fillTopItems(WebEngineViewer::NetworkUrlInterceptorPluginManager::self()->pluginsDataList(),
                                                            i18n("Webengine Plugins"),
                                                            WebEngineViewer::NetworkUrlInterceptorPluginManager::self()->configGroupName(),
                                                            WebEngineViewer::NetworkUrlInterceptorPluginManager::self()->configPrefixSettingKey(),
                                                            mPluginWebEngineItems,
                                                            networkUrlInterceptorGroupName(),
                                                            true,
                                                            PimCommon::PluginUtil::pluginConfigFile());

    // Load headerstyle
    TextAddonsWidgets::ConfigurePluginsWidget::fillTopItems(MessageViewer::HeaderStylePluginManager::self()->pluginsDataList(),
                                                            i18n("Header Style Plugins"),
                                                            MessageViewer::HeaderStylePluginManager::self()->configGroupName(),
                                                            MessageViewer::HeaderStylePluginManager::self()->configPrefixSettingKey(),
                                                            mPluginHeaderStyleItems,
                                                            headerStyleGroupName(),
                                                            true,
                                                            PimCommon::PluginUtil::pluginConfigFile());
    // Load Converter plugin
    TextAddonsWidgets::ConfigurePluginsWidget::fillTopItems(MessageComposer::PluginEditorConvertTextManager::self()->pluginsDataList(),
                                                            i18n("Text Conversion Plugins"),
                                                            MessageComposer::PluginEditorConvertTextManager::self()->configGroupName(),
                                                            MessageComposer::PluginEditorConvertTextManager::self()->configPrefixSettingKey(),
                                                            mPluginConvertTextItems,
                                                            pluginEditorConvertTextGroupName(),
                                                            true,
                                                            PimCommon::PluginUtil::pluginConfigFile());

    TextAddonsWidgets::ConfigurePluginsWidget::fillTopItems(MessageViewer::MessageViewerConfigureSettingsPluginManager::self()->pluginsDataList(),
                                                            i18n("Misc"),
                                                            MessageViewer::MessageViewerConfigureSettingsPluginManager::self()->configGroupName(),
                                                            MessageViewer::MessageViewerConfigureSettingsPluginManager::self()->configPrefixSettingKey(),
                                                            mPluginConfigureItems,
                                                            configurePluginGroupName(),
                                                            false,
                                                            PimCommon::PluginUtil::pluginConfigFile());

    TextAddonsWidgets::ConfigurePluginsWidget::fillTopItems(MessageViewer::MessageViewerCheckBeforeDeletingPluginManager::self()->pluginsDataList(),
                                                            i18n("Confirm Deleting Emails Plugins"),
                                                            MessageViewer::MessageViewerCheckBeforeDeletingPluginManager::self()->configGroupName(),
                                                            MessageViewer::MessageViewerCheckBeforeDeletingPluginManager::self()->configPrefixSettingKey(),
                                                            mPluginCheckBeforeDeletingItems,
                                                            pluginEditorCheckBeforeDeletingGroupName(),
                                                            true,
                                                            PimCommon::PluginUtil::pluginConfigFile());

    // Load Agent Plugin
    initializeAgentPlugins();
    mTreePluginWidget->expandAll();
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

    TextAddonsWidgets::ConfigurePluginsWidget::fillTopItems(mPluginUtilDataList,
                                                            i18n("Akonadi Agents"),
                                                            QString(),
                                                            QString(),
                                                            mAgentPluginsItems,
                                                            agentAkonadiGroupName());
}

TextAddonsWidgets::PluginUtilData ConfigurePluginsListWidget::createAgentPluginData(const QString &agentIdentifier, const QString &path)
{
    TextAddonsWidgets::PluginUtilData data;
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
    const QString service = Akonadi::ServerManager::agentServiceName(Akonadi::ServerManager::Agent, agentIdentifier);
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
        const QString service = Akonadi::ServerManager::agentServiceName(Akonadi::ServerManager::Agent, agentIdentifier);
        QDBusInterface interface(service, path);
        if (interface.isValid()) {
            interface.call(QStringLiteral("setEnableAgent"), enable);
        } else {
            qCDebug(KMAIL_LOG) << agentIdentifier << "does not exist when trying to change the agent active state";
        }
    }
}

void ConfigurePluginsListWidget::slotConfigureClicked(const QString &groupName, const QString &identifier)
{
    if (!groupName.isEmpty() && !identifier.isEmpty()) {
        if (groupName == headerStyleGroupName()) {
            MessageViewer::HeaderStylePlugin *plugin = MessageViewer::HeaderStylePluginManager::self()->pluginFromIdentifier(identifier);
            plugin->showConfigureDialog(this);
        } else if (groupName == networkUrlInterceptorGroupName()) {
            WebEngineViewer::NetworkPluginUrlInterceptor *plugin =
                WebEngineViewer::NetworkUrlInterceptorPluginManager::self()->pluginFromIdentifier(identifier);
            plugin->showConfigureDialog(this);
        } else if (groupName == viewerPluginGroupName()) {
            MessageViewer::ViewerPlugin *plugin = MessageViewer::ViewerPluginManager::self()->pluginFromIdentifier(identifier);
            plugin->showConfigureDialog(this);
        } else if (groupName == pluginEditorGroupName()) {
            MessageComposer::PluginEditor *plugin = MessageComposer::PluginEditorManager::self()->pluginFromIdentifier(identifier);
            plugin->showConfigureDialog(this);
        } else if (groupName == kmailPluginToolsGroupName()) {
            PimCommon::GenericPlugin *plugin = KMailPluginInterface::self()->pluginFromIdentifier(identifier);
            plugin->showConfigureDialog(this);
        } else if (groupName == pluginEditorInitGroupName()) {
            MessageComposer::PluginEditorInit *plugin = MessageComposer::PluginEditorInitManager::self()->pluginFromIdentifier(identifier);
            plugin->showConfigureDialog(this);
        } else if (groupName == pluginEditorCheckBeforeGroupName()) {
            MessageComposer::PluginEditorCheckBeforeSend *plugin =
                MessageComposer::PluginEditorCheckBeforeSendManager::self()->pluginFromIdentifier(identifier);
            plugin->showConfigureDialog(this);
        } else if (groupName == pluginEditorCheckBeforeDeletingGroupName()) {
            MessageViewer::MessageViewerCheckBeforeDeletingPlugin *plugin =
                MessageViewer::MessageViewerCheckBeforeDeletingPluginManager::self()->pluginFromIdentifier(identifier);
            plugin->showConfigureDialog(this);
        } else if (groupName == pluginEditorConvertTextGroupName()) {
            MessageComposer::PluginEditorConvertText *plugin = MessageComposer::PluginEditorConvertTextManager::self()->pluginFromIdentifier(identifier);
            plugin->showConfigureDialog(this);
        } else if (groupName == pluginEditorGrammarGroupName()) {
            PimCommon::CustomToolsPlugin *plugin = MessageComposer::PluginEditorGrammarManager::self()->pluginFromIdentifier(identifier);
            plugin->showConfigureDialog(this);
        } else if (groupName == configurePluginGroupName()) {
            MessageViewer::MessageViewerConfigureSettingsPlugin *plugin =
                MessageViewer::MessageViewerConfigureSettingsPluginManager::self()->pluginFromIdentifier(identifier);
            plugin->showConfigureDialog(this);
        } else if (groupName == agentAkonadiGroupName()) {
            for (const TextAddonsWidgets::PluginUtilData &data : std::as_const(mPluginUtilDataList)) {
                if (data.mIdentifier == identifier) {
                    auto instance = Akonadi::AgentManager::self()->instance(identifier);
                    if (instance.isValid()) {
                        QPointer<Akonadi::AgentConfigurationDialog> dlg = new Akonadi::AgentConfigurationDialog(instance, this);
                        dlg->exec();
                        delete dlg;
                    }
                    break;
                }
            }
        } else {
            qCWarning(KMAIL_LOG) << "Unknown configureGroupName" << groupName;
        }
    }
}

void ConfigurePluginsListWidget::defaults()
{
    doResetToDefaultsOther();
}

#include "moc_configurepluginslistwidget.cpp"
