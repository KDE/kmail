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

#include "configurepluginpage.h"
#include "configureplugins/configurepluginswidget.h"

#include <KLocalizedString>
#include <QHBoxLayout>

ConfigurePluginPage::ConfigurePluginPage(QWidget *parent)
    : ConfigModuleWithTabs(parent)
{
    ConfigurePluginTab *pluginTab = new ConfigurePluginTab();
    addTab(pluginTab, i18n("Plugins"));
}

ConfigurePluginPage::~ConfigurePluginPage()
{

}

QString ConfigurePluginPage::helpAnchor() const
{
    return {};
}

ConfigurePluginTab::ConfigurePluginTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    QHBoxLayout *l = new QHBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    mConfigurePlugins = new ConfigurePluginsWidget(this);
    l->addWidget(mConfigurePlugins);

    connect(mConfigurePlugins, &ConfigurePluginsWidget::changed, this, &ConfigurePluginTab::slotEmitChanged);
}

void ConfigurePluginTab::save()
{
    mConfigurePlugins->save();
}

QString ConfigurePluginTab::helpAnchor() const
{
    return {};
}

void ConfigurePluginTab::doLoadFromGlobalSettings()
{
    mConfigurePlugins->doLoadFromGlobalSettings();
}

void ConfigurePluginTab::doLoadOther()
{
}
