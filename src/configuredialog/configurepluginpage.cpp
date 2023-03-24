/*
  SPDX-FileCopyrightText: 2016-2023 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "configurepluginpage.h"
#include "configureplugins/configurepluginslistwidget.h"
#include <PimCommon/ConfigurePluginsWidget>

#include <QHBoxLayout>

#if KCMUTILS_VERSION < QT_VERSION_CHECK(5, 240, 0)
ConfigurePluginPage::ConfigurePluginPage(QWidget *parent, const QVariantList &args)
    : ConfigModule(parent, args)
    , mConfigurePlugins(new PimCommon::ConfigurePluginsWidget(new ConfigurePluginsListWidget(this), this))
#else
ConfigurePluginPage::ConfigurePluginPage(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : ConfigModule(parent, data, args)
    , mConfigurePlugins(new PimCommon::ConfigurePluginsWidget(new ConfigurePluginsListWidget(widget()), widget()))
#endif
{
#if KCMUTILS_VERSION < QT_VERSION_CHECK(5, 240, 0)
    auto l = new QHBoxLayout(this);
#else
    auto l = new QHBoxLayout(widget());
#endif
    l->setContentsMargins({});
    l->addWidget(mConfigurePlugins);

    connect(mConfigurePlugins, &PimCommon::ConfigurePluginsWidget::changed, this, &ConfigurePluginPage::slotConfigureChanged);
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

void ConfigurePluginPage::slotConfigureChanged()
{
#if KCMUTILS_VERSION < QT_VERSION_CHECK(5, 240, 0)
    Q_EMIT changed(true);
#else
    markAsChanged();
#endif
}
