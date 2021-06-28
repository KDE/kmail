/*
  SPDX-FileCopyrightText: 2016-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include <PimCommon/ConfigurePluginsListWidget>
#include <PimCommon/PluginUtil>
#include <QVector>
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
    PimCommon::PluginUtilData createAgentPluginData(const QString &agentIdentifier, const QString &path);
    Q_REQUIRED_RESULT bool agentActivateState(const QString &agentIdentifier, const QString &pathName);
    void changeAgentActiveState(const QString &agentIdentifier, const QString &path, bool enable);
    void saveAkonadiAgent();
    QVector<PluginItem *> mPluginEditorItems;
    QVector<PluginItem *> mPluginMessageViewerItems;
    QVector<PluginItem *> mPluginCheckBeforeSendItems;
    QVector<PluginItem *> mPluginEditorInitItems;
    QVector<PluginItem *> mPluginEditorGrammarItems;
    QVector<PluginItem *> mPluginGenericItems;
    QVector<PluginItem *> mPluginWebEngineItems;
    QVector<PluginItem *> mPluginHeaderStyleItems;
    QVector<PluginItem *> mAgentPluginsItems;
    QVector<PluginItem *> mPluginConvertTextItems;
    QVector<PluginItem *> mPluginConfigureItems;
    QVector<PluginItem *> mPluginCheckBeforeDeletingItems;
    QVector<PimCommon::PluginUtilData> mPluginUtilDataList;
};

