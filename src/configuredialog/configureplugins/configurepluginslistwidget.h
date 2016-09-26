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

#ifndef KMAILCONFIGUREPLUGINSLISTWIDGET_H
#define KMAILCONFIGUREPLUGINSLISTWIDGET_H

#include <QList>
#include <PimCommon/ConfigurePluginsListWidget>
#include <PimCommon/PluginUtil>
class ConfigurePluginsListWidget : public PimCommon::ConfigurePluginsListWidget
{
    Q_OBJECT
public:
    explicit ConfigurePluginsListWidget(QWidget *parent = Q_NULLPTR);
    ~ConfigurePluginsListWidget();

    void save() Q_DECL_OVERRIDE;
    void doLoadFromGlobalSettings() Q_DECL_OVERRIDE;
    void doResetToDefaultsOther() Q_DECL_OVERRIDE;
    void initialize() Q_DECL_OVERRIDE;

private Q_SLOTS:
    void slotConfigureClicked(const QString &configureGroupName, const QString &identifier);

private:
    void initializeAgentPlugins();
    PimCommon::PluginUtilData createAgentPluginData(const QString &interfaceName, const QString &path);
    bool agentActivateState(const QString &interfaceName, const QString &pathName, bool &failed);
    void changeAgentActiveState(const QString &interfaceName, const QString &path, bool enable);
    QList<PluginItem *> mPluginEditorItems;
    QList<PluginItem *> mPluginMessageViewerItems;
    QList<PluginItem *> mPluginSendBeforeSendItems;
    QList<PluginItem *> mPluginGenericItems;
    QList<PluginItem *> mPluginWebEngineItems;
    QList<PluginItem *> mPluginHeaderStyleItems;
    QList<PluginItem *> mAgentPluginsItems;
};

#endif // KMAILCONFIGUREPLUGINSLISTWIDGET_H
