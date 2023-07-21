/*
 *   kmail: KDE mail client
 *   SPDX-FileCopyrightText: 2016-2023 Laurent Montel <montel@kde.org>
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

// -*- c++ -*-
// configuredialog_p.h: classes internal to ConfigureDialog
// see configuredialog.h for details.

#pragma once

#include "configmodule.h"
#include "kcmutils_version.h"
#include "kmail_export.h"
class QTabWidget;
class ConfigureDialog;

// Individual tab of a ConfigModuleWithTabs
class ConfigModuleTab : public QWidget
{
    Q_OBJECT
public:
    explicit ConfigModuleTab(QWidget *parent = nullptr)
        : QWidget(parent)
    {
    }

    ~ConfigModuleTab() override = default;

    virtual void save() = 0;
    void defaults();
Q_SIGNALS:
    // forwarded to the ConfigModule
    void changed(bool);
public Q_SLOTS:
    void slotEmitChanged();
    void load();

protected:
    bool mEmitChanges{true};

private:
    // reimplement this for loading values of settings which are available
    // via GlobalSettings
    virtual void doLoadFromGlobalSettings()
    {
    }

    // reimplement this for loading values of settings which are not available
    // via GlobalSettings
    virtual void doLoadOther()
    {
    }

    // reimplement this for loading default values of settings which are
    // not available via GlobalSettings (KConfigXT).
    virtual void doResetToDefaultsOther()
    {
    }
};

/*
 * ConfigModuleWithTabs represents a kcm with several tabs.
 * It simply forwards load and save operations to all tabs.
 */
class KMAIL_EXPORT ConfigModuleWithTabs : public ConfigModule
{
    Q_OBJECT
public:
#if KCMUTILS_VERSION < QT_VERSION_CHECK(5, 240, 0)
    explicit ConfigModuleWithTabs(QWidget *parent = nullptr, const QVariantList &args = {});
#else
    explicit ConfigModuleWithTabs(QObject *parent, const KPluginMetaData &data, const QVariantList &args);
#endif
    ~ConfigModuleWithTabs() override = default;

    // don't reimplement any of those methods
    void load() override;
    void save() override;
    void defaults() override;

protected:
#if KCMUTILS_VERSION < QT_VERSION_CHECK(5, 240, 0)
    void showEvent(QShowEvent *event) override;
#else
#pragma "NEED TO PORT TO QT6";
#endif
    void addTab(ConfigModuleTab *tab, const QString &title);

private:
    QTabWidget *const mTabWidget;
    bool mWasInitialized = false;
};
