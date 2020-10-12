/*
  SPDX-FileCopyrightText: 2016-2020 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "configurepluginpage.h"
#include "configureplugins/configurepluginslistwidget.h"
#include <PimCommon/ConfigurePluginsWidget>

#include <QHBoxLayout>

ConfigurePluginPage::ConfigurePluginPage(QWidget *parent)
    : ConfigModule(parent)
{
    QHBoxLayout *l = new QHBoxLayout(this);
    l->setContentsMargins({});
    mConfigurePlugins = new PimCommon::ConfigurePluginsWidget(new ConfigurePluginsListWidget(this), this);
    l->addWidget(mConfigurePlugins);

    connect(mConfigurePlugins, &PimCommon::ConfigurePluginsWidget::changed, this, &ConfigurePluginPage::slotConfigureChanged);
}

ConfigurePluginPage::~ConfigurePluginPage()
{
}

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

void ConfigurePluginPage::slotConfigureChanged()
{
    Q_EMIT changed(true);
}
