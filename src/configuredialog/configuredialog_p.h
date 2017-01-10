// -*- c++ -*-
// configuredialog_p.h: classes internal to ConfigureDialog
// see configuredialog.h for details.

#ifndef _CONFIGURE_DIALOG_PRIVATE_H_
#define _CONFIGURE_DIALOG_PRIVATE_H_

#include "kmail_export.h"
#include "configmodule.h"

#include <kcmodule.h>

class QTabWidget;
class ConfigureDialog;

// Individual tab of a ConfigModuleWithTabs
class ConfigModuleTab : public QWidget
{
    Q_OBJECT
public:
    explicit ConfigModuleTab(QWidget *parent = nullptr)
        : QWidget(parent),
          mEmitChanges(true)
    {}
    ~ConfigModuleTab() {}
    virtual void save() = 0;
    void defaults();
Q_SIGNALS:
    // forwarded to the ConfigModule
    void changed(bool);
public Q_SLOTS:
    void slotEmitChanged();
    void load();
protected:
    bool mEmitChanges;
private:
    // reimplement this for loading values of settings which are available
    // via GlobalSettings
    virtual void doLoadFromGlobalSettings() {}
    // reimplement this for loading values of settings which are not available
    // via GlobalSettings
    virtual void doLoadOther() {}
    // reimplement this for loading default values of settings which are
    // not available via GlobalSettings (KConfigXT).
    virtual void doResetToDefaultsOther() {}
};

/*
 * ConfigModuleWithTabs represents a kcm with several tabs.
 * It simply forwards load and save operations to all tabs.
 */
class KMAIL_EXPORT ConfigModuleWithTabs : public ConfigModule
{
    Q_OBJECT
public:
    explicit ConfigModuleWithTabs(QWidget *parent = nullptr);
    ~ConfigModuleWithTabs() {}

    // don't reimplement any of those methods
    void load() Q_DECL_OVERRIDE;
    void save() Q_DECL_OVERRIDE;
    void defaults() Q_DECL_OVERRIDE;

protected:
    void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;
    void addTab(ConfigModuleTab *tab, const QString &title);

private:
    QTabWidget *mTabWidget;
    bool mWasInitialized;
};

#endif // _CONFIGURE_DIALOG_PRIVATE_H_
