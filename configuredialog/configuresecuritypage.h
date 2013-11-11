/*
  Copyright (c) 2013 Montel Laurent <montel@kde.org>

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


#ifndef CONFIGURESECURITYPAGE_H
#define CONFIGURESECURITYPAGE_H

#include "kmail_export.h"
#include "configuredialog_p.h"
#include "ui_securitypagegeneraltab.h"
#include "ui_securitypagemdntab.h"
#include "ui_composercryptoconfiguration.h"
#include "ui_warningconfiguration.h"
#include "ui_smimeconfiguration.h"

class QButtonGroup;
namespace MessageViewer {
class AdBlockSettingWidget;
}

class SecurityPageGeneralTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit SecurityPageGeneralTab( QWidget * parent=0 );
    QString helpAnchor() const;

    void save();

private:
    //virtual void doLoadFromGlobalSettings();
    void doLoadOther();
    //FIXME virtual void doResetToDefaultsOther();

private:
    Ui_SecurityPageGeneralTab mSGTab;

private slots:
    void slotLinkClicked( const QString & link );
};

class SecurityPageMDNTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit SecurityPageMDNTab( QWidget * parent=0 );
    QString helpAnchor() const;

    void save();

private:
    void doLoadOther();

private:
    QButtonGroup *mMDNGroup;
    QButtonGroup *mOrigQuoteGroup;
    Ui_SecurityPageMDNTab mUi;

private slots:
    void slotLinkClicked( const QString & link );
};

class SecurityPageComposerCryptoTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit SecurityPageComposerCryptoTab( QWidget * parent=0 );
    ~SecurityPageComposerCryptoTab();

    QString helpAnchor() const;

    void save();

private:
    void doLoadFromGlobalSettings();
    void doLoadOther();
    //FIXME virtual void doResetToDefaultsOther();

private:
    Ui::ComposerCryptoConfiguration* mWidget;
};

class SecurityPageWarningTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit SecurityPageWarningTab( QWidget * parent=0 );
    ~SecurityPageWarningTab();

    QString helpAnchor() const;

    void save();

private Q_SLOTS:
    void slotReenableAllWarningsClicked();
    void slotConfigureGnupg();
    //void slotConfigureChiasmus();

private:
    void doLoadFromGlobalSettings();
    void doLoadOther();
    //FIXME virtual void doResetToDefaultsOther();

private:
    Ui::WarningConfiguration* mWidget;
};

class SecurityPageSMimeTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit SecurityPageSMimeTab( QWidget * parent=0 );
    ~SecurityPageSMimeTab();

    QString helpAnchor() const;

    void save();

private slots:
    void slotUpdateHTTPActions();

private:
    //virtual void doLoadFromGlobalSettings();
    void doLoadOther();
    //FIXME virtual void doResetToDefaultsOther();

private:
    Ui::SMimeConfiguration* mWidget;
    Kleo::CryptoConfig* mConfig;
};

class SecurityPageAdBlockTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit SecurityPageAdBlockTab( QWidget * parent=0 );
    ~SecurityPageAdBlockTab();

    QString helpAnchor() const;

    void save();

private:
    void doLoadFromGlobalSettings();
    void doLoadOther();
    void doResetToDefaultsOther();

private:
    MessageViewer::AdBlockSettingWidget *mWidget;
};


class KMAIL_EXPORT SecurityPage : public ConfigModuleWithTabs {
    Q_OBJECT
public:
    explicit SecurityPage( const KComponentData &instance, QWidget *parent=0 );

    QString helpAnchor() const;

    typedef SecurityPageGeneralTab GeneralTab;
    typedef SecurityPageMDNTab MDNTab;
    typedef SecurityPageComposerCryptoTab ComposerCryptoTab;
    typedef SecurityPageWarningTab WarningTab;
    typedef SecurityPageSMimeTab SMimeTab;

private:
    GeneralTab    *mGeneralTab;
    ComposerCryptoTab *mComposerCryptoTab;
    WarningTab    *mWarningTab;
    SMimeTab      *mSMimeTab;
    SecurityPageAdBlockTab *mSAdBlockTab;
};



#endif // CONFIGURESECURITYPAGE_H
