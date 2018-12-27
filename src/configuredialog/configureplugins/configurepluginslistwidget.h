/*
  Copyright (c) 2016-2019 Montel Laurent <montel@kde.org>

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

#ifndef KMAILCONFIGUREPLUGINSLISTWIDGET_H
#define KMAILCONFIGUREPLUGINSLISTWIDGET_H

#include <QList>
#include <PimCommon/ConfigurePluginsListWidget>
#include <PimCommon/PluginUtil>
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
    bool agentActivateState(const QString &agentIdentifier, const QString &pathName);
    void changeAgentActiveState(const QString &agentIdentifier, const QString &path, bool enable);
    void saveAkonadiAgent();
    QList<PluginItem *> mPluginEditorItems;
    QList<PluginItem *> mPluginMessageViewerItems;
    QList<PluginItem *> mPluginCheckBeforeSendItems;
    QList<PluginItem *> mPluginEditorInitItems;
    QList<PluginItem *> mPluginGenericItems;
    QList<PluginItem *> mPluginWebEngineItems;
    QList<PluginItem *> mPluginHeaderStyleItems;
    QList<PluginItem *> mAgentPluginsItems;
    QList<PluginItem *> mPluginConvertTextItems;
    QVector<PimCommon::PluginUtilData> mPluginUtilDataList;
};

#endif // KMAILCONFIGUREPLUGINSLISTWIDGET_H
