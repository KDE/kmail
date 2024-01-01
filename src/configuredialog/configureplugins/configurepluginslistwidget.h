/*
  SPDX-FileCopyrightText: 2016-2024 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include <PimCommon/ConfigurePluginsListWidget>
#include <PimCommon/PluginUtil>
#include <QList>
class ConfigurePluginsListWidget : public PimCommon::ConfigurePluginsListWidget
{
    Q_OBJECT
public:
    explicit ConfigurePluginsListWidget(QWidget *parent = nullptr);
    ~ConfigurePluginsListWidget() override;

    void save() override;
    void doLoadFromGlobalSettings() override;
    void doResetToDefaultsOther() override;
    void initialize() override;
    void defaults() override;

private:
    void slotConfigureClicked(const QString &configureGroupName, const QString &identifier);
    void initializeAgentPlugins();
    [[nodiscard]] PimCommon::PluginUtilData createAgentPluginData(const QString &agentIdentifier, const QString &path);
    [[nodiscard]] bool agentActivateState(const QString &agentIdentifier, const QString &pathName);
    void changeAgentActiveState(const QString &agentIdentifier, const QString &path, bool enable);
    void saveAkonadiAgent();
    QList<PluginItem *> mPluginEditorItems;
    QList<PluginItem *> mPluginMessageViewerItems;
    QList<PluginItem *> mPluginCheckBeforeSendItems;
    QList<PluginItem *> mPluginEditorInitItems;
    QList<PluginItem *> mPluginEditorGrammarItems;
    QList<PluginItem *> mPluginGenericItems;
    QList<PluginItem *> mPluginWebEngineItems;
    QList<PluginItem *> mPluginHeaderStyleItems;
    QList<PluginItem *> mAgentPluginsItems;
    QList<PluginItem *> mPluginConvertTextItems;
    QList<PluginItem *> mPluginConfigureItems;
    QList<PluginItem *> mPluginCheckBeforeDeletingItems;
    QList<PimCommon::PluginUtilData> mPluginUtilDataList;
};
