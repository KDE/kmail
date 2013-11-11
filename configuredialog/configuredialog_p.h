// -*- c++ -*-
// configuredialog_p.h: classes internal to ConfigureDialog
// see configuredialog.h for details.

#ifndef _CONFIGURE_DIALOG_PRIVATE_H_
#define _CONFIGURE_DIALOG_PRIVATE_H_

#include <config-enterprise.h>

#include "kmail_export.h"
#include "configmodule.h"

#include <QSharedPointer>
#include <QListWidgetItem>

#include <kcmodule.h>

#include <akonadi/agentinstance.h>

#include "ui_warningconfiguration.h"
#include "ui_smimeconfiguration.h"
#include "ui_customtemplates_base.h"
#include "ui_accountspagereceivingtab.h"
#include "tag.h"
class QPushButton;
class QLabel;
class QCheckBox;
class QFont;
class QGroupBox;
class QSpinBox;
class QListWidget;

class KLineEdit;
class KButtonGroup;
class KUrlRequester;
class KFontChooser;
class KTabWidget;
class ListView;
class ConfigureDialog;
class KIntSpinBox;
class OrgFreedesktopAkonadiNewMailNotifierInterface;
class KComboBox;
class ColorListBox;
class KCModuleProxy;
class KIntNumInput;



// Individual tab of a ConfigModuleWithTabs
class ConfigModuleTab : public QWidget {
    Q_OBJECT
public:
    explicit ConfigModuleTab( QWidget *parent=0 )
        : QWidget( parent ),
          mEmitChanges( true )
    {}
    ~ConfigModuleTab() {}
    virtual void save() = 0;
    void defaults();
signals:
    // forwarded to the ConfigModule
    void changed(bool);
public slots:
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
class KMAIL_EXPORT ConfigModuleWithTabs : public ConfigModule {
    Q_OBJECT
public:
    explicit ConfigModuleWithTabs( const KComponentData &instance, QWidget *parent=0 );
    ~ConfigModuleWithTabs() {}

    // don't reimplement any of those methods
    virtual void load();
    virtual void save();
    virtual void defaults();

protected:
    virtual void showEvent ( QShowEvent * event );
    void addTab( ConfigModuleTab* tab, const QString & title );

private:
    KTabWidget *mTabWidget;
    bool mWasInitialized;
};


//
//
// AccountsPage
//
//

// subclasses: one class per tab:
class AccountsPageSendingTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit AccountsPageSendingTab( QWidget * parent=0 );
    virtual ~AccountsPageSendingTab();
    QString helpAnchor() const;
    void save();

private:
    void doLoadFromGlobalSettings();
    void doLoadOther();
    //FIXME virtual void doResetToDefaultsOther();

private:
    QCheckBox   *mConfirmSendCheck;
    QCheckBox   *mCheckSpellingBeforeSending;
    KComboBox   *mSendOnCheckCombo;
    KComboBox   *mSendMethodCombo;
    KLineEdit   *mDefaultDomainEdit;
};


class AccountsPageReceivingTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit AccountsPageReceivingTab( QWidget * parent=0 );
    ~AccountsPageReceivingTab();
    QString helpAnchor() const;
    void save();

signals:
    void accountListChanged( const QStringList & );

private slots:
    void slotAccountSelected(const Akonadi::AgentInstance&);
    void slotAddAccount();
    void slotModifySelectedAccount();
    void slotRemoveSelectedAccount();
    void slotRestartSelectedAccount();
    void slotEditNotifications();
    void slotShowMailCheckMenu( const QString &, const QPoint & );
    void slotCustomizeAccountOrder();
    void slotIncludeInCheckChanged( bool checked );
    void slotOfflineOnShutdownChanged( bool checked );
    void slotCheckOnStatupChanged( bool checked );

private:
    void doLoadFromGlobalSettings();
    void doLoadOther();

    struct RetrievalOptions {
        RetrievalOptions( bool manualCheck, bool offline, bool checkOnStartup )
            : IncludeInManualChecks( manualCheck )
            , OfflineOnShutdown( offline )
            ,CheckOnStartup( checkOnStartup ) {}
        bool IncludeInManualChecks;
        bool OfflineOnShutdown;
        bool CheckOnStartup;
    };

    QHash<QString, QSharedPointer<RetrievalOptions> > mRetrievalHash;
    //FIXME virtual void doResetToDefaultsOther();

private:
    QString mSpecialMailCollectionIdentifier;
    Ui_AccountsPageReceivingTab mAccountsReceiving;
    OrgFreedesktopAkonadiNewMailNotifierInterface *mNewMailNotifierInterface;
};

class KMAIL_EXPORT AccountsPage : public ConfigModuleWithTabs {
    Q_OBJECT
public:
    explicit AccountsPage( const KComponentData &instance, QWidget *parent=0 );
    QString helpAnchor() const;


    // hrmpf. moc doesn't like nested classes with slots/signals...:
    typedef AccountsPageSendingTab SendingTab;
    typedef AccountsPageReceivingTab ReceivingTab;

signals:
    void accountListChanged( const QStringList & );

private:
    SendingTab   *mSendingTab;
    ReceivingTab *mReceivingTab;
};


#endif // _CONFIGURE_DIALOG_PRIVATE_H_
