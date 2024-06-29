/*
 *   kmail: KDE mail client
 *   SPDX-FileCopyrightText: 2016-2024 Laurent Montel <montel@kde.org>
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */
// configuredialog_p.cpp: classes internal to ConfigureDialog
// see configuredialog.cpp for details.

// my header:
#include "configuredialog_p.h"

// other KMail headers:
#include "settings/kmailsettings.h"

// other KDE headers:
#include <KConfigDialogManager>

// Qt headers:
#include <QShowEvent>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>

// Other headers:

ConfigModuleWithTabs::ConfigModuleWithTabs(QObject *parent, const KPluginMetaData &data)
    : ConfigModule(parent, data)
    , mTabWidget(new QTabWidget(widget()))
{
    auto vlay = new QVBoxLayout(widget());
    vlay->setContentsMargins({});
    vlay->addWidget(mTabWidget);
    mTabWidget->setDocumentMode(true);
    mTabWidget->tabBar()->setExpanding(true);

    addConfig(KMailSettings::self(), mTabWidget);
}

void ConfigModuleWithTabs::addTab(ConfigModuleTab *tab, const QString &title)
{
    mTabWidget->addTab(tab, title);
    connect(tab, &ConfigModuleTab::changed, this, [this](bool state) {
        setNeedsSave(state);
    });

    for (const auto configs = KCModule::configs(); auto config : configs) {
        config->addWidget(tab);
    }
}

void ConfigModuleWithTabs::load()
{
    const int numberOfTab = mTabWidget->count();
    for (int i = 0; i < numberOfTab; ++i) {
        auto tab = qobject_cast<ConfigModuleTab *>(mTabWidget->widget(i));
        if (tab) {
            tab->load();
        }
    }
    KCModule::load();
    mWasInitialized = true;
}

void ConfigModuleWithTabs::save()
{
    if (mWasInitialized) {
        KCModule::save();
        const int numberOfTab = mTabWidget->count();
        for (int i = 0; i < numberOfTab; ++i) {
            auto tab = qobject_cast<ConfigModuleTab *>(mTabWidget->widget(i));
            if (tab) {
                tab->save();
            }
        }
    }
}

void ConfigModuleWithTabs::defaults()
{
    auto tab = qobject_cast<ConfigModuleTab *>(mTabWidget->currentWidget());
    if (tab) {
        tab->defaults();
    }
    KCModule::defaults();
}

void ConfigModuleTab::load()
{
    mEmitChanges = false;
    doLoadFromGlobalSettings();
    doLoadOther();
    mEmitChanges = true;
}

void ConfigModuleTab::defaults()
{
    // reset other settings to default values
    doResetToDefaultsOther();
}

void ConfigModuleTab::slotEmitChanged()
{
    if (mEmitChanges) {
        Q_EMIT changed(true);
    }
}

#include "moc_configuredialog_p.cpp"
