/*
  SPDX-FileCopyrightText: 2016-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "configurepluginpage.h"
#include "configureplugins/configurepluginslistwidget.h"
#include <PimCommon/ConfigurePluginsWidget>

#include <QHBoxLayout>

ConfigurePluginPage::ConfigurePluginPage(QObject *parent, const KPluginMetaData &data)
    : ConfigModule(parent, data)
    , mConfigurePlugins(new PimCommon::ConfigurePluginsWidget(new ConfigurePluginsListWidget(widget()), widget()))
{
    auto l = new QHBoxLayout(widget());
    l->setContentsMargins({});
    l->addWidget(mConfigurePlugins);

    connect(mConfigurePlugins, &PimCommon::ConfigurePluginsWidget::wasChanged, this, [this](bool state) {
        setNeedsSave(state);
    });
}

ConfigurePluginPage::~ConfigurePluginPage() = default;

void ConfigurePluginPage::save()
{
    mConfigurePlugins->save();
}

void ConfigurePluginPage::defaults()
{
    mConfigurePlugins->defaults();
}

QString ConfigurePluginPage::helpAnchor() const
{
    return {};
}

void ConfigurePluginPage::load()
{
    mConfigurePlugins->doLoadFromGlobalSettings();
}

#include "moc_configurepluginpage.cpp"
