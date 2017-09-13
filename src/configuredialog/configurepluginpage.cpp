/*
  Copyright (C) 2016-2017 Montel Laurent <montel@kde.org>

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

#include "configurepluginpage.h"
#include "configureplugins/configurepluginslistwidget.h"
#include <PimCommon/ConfigurePluginsWidget>

#include <QHBoxLayout>

ConfigurePluginPage::ConfigurePluginPage(QWidget *parent)
    : ConfigModule(parent)
{
    QHBoxLayout *l = new QHBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
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
