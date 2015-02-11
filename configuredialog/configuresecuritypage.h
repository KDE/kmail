/*
  Copyright (c) 2013-2015 Montel Laurent <montel@kde.org>

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
namespace MessageViewer
{
class AdBlockSettingWidget;
}

class SecurityPageGeneralTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit SecurityPageGeneralTab(QWidget *parent = Q_NULLPTR);
    QString helpAnchor() const;

    void save() Q_DECL_OVERRIDE;

private:
    //virtual void doLoadFromGlobalSettings();
    void doLoadOther() Q_DECL_OVERRIDE;
    //FIXME virtual void doResetToDefaultsOther();

private:
    Ui_SecurityPageGeneralTab mSGTab;

private Q_SLOTS:
    void slotLinkClicked(const QString &link);
};

class SecurityPageMDNTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit SecurityPageMDNTab(QWidget *parent = Q_NULLPTR);
    QString helpAnchor() const;

    void save() Q_DECL_OVERRIDE;

private:
    void doLoadOther() Q_DECL_OVERRIDE;

private:
    QButtonGroup *mMDNGroup;
    QButtonGroup *mOrigQuoteGroup;
    Ui_SecurityPageMDNTab mUi;

private Q_SLOTS:
    void slotLinkClicked(const QString &link);
};

class SecurityPageComposerCryptoTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit SecurityPageComposerCryptoTab(QWidget *parent = Q_NULLPTR);
    ~SecurityPageComposerCryptoTab();

    QString helpAnchor() const;

    void save() Q_DECL_OVERRIDE;

private:
    void doLoadFromGlobalSettings() Q_DECL_OVERRIDE;
    void doLoadOther() Q_DECL_OVERRIDE;
    //FIXME virtual void doResetToDefaultsOther();

private:
    Ui::ComposerCryptoConfiguration *mWidget;
};

class SecurityPageWarningTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit SecurityPageWarningTab(QWidget *parent = Q_NULLPTR);
    ~SecurityPageWarningTab();

    QString helpAnchor() const;

    void save() Q_DECL_OVERRIDE;

private Q_SLOTS:
    void slotReenableAllWarningsClicked();
    void slotConfigureGnupg();

private:
    void doLoadFromGlobalSettings() Q_DECL_OVERRIDE;
    void doLoadOther() Q_DECL_OVERRIDE;
    //FIXME virtual void doResetToDefaultsOther();

private:
    Ui::WarningConfiguration *mWidget;
};

class SecurityPageSMimeTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit SecurityPageSMimeTab(QWidget *parent = Q_NULLPTR);
    ~SecurityPageSMimeTab();

    QString helpAnchor() const;

    void save() Q_DECL_OVERRIDE;

private Q_SLOTS:
    void slotUpdateHTTPActions();

private:
    //virtual void doLoadFromGlobalSettings();
    void doLoadOther() Q_DECL_OVERRIDE;
    //FIXME virtual void doResetToDefaultsOther();

private:
    Ui::SMimeConfiguration *mWidget;
    Kleo::CryptoConfig *mConfig;
};

#ifndef KDEPIM_NO_WEBKIT
class SecurityPageAdBlockTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit SecurityPageAdBlockTab(QWidget *parent = Q_NULLPTR);
    ~SecurityPageAdBlockTab();

    QString helpAnchor() const;

    void save() Q_DECL_OVERRIDE;

private:
    void doLoadFromGlobalSettings() Q_DECL_OVERRIDE;
    void doLoadOther() Q_DECL_OVERRIDE;
    void doResetToDefaultsOther() Q_DECL_OVERRIDE;

private:
    MessageViewer::AdBlockSettingWidget *mWidget;
};
#endif

class KMAIL_EXPORT SecurityPage : public ConfigModuleWithTabs
{
    Q_OBJECT
public:
    explicit SecurityPage(QWidget *parent = Q_NULLPTR);

    QString helpAnchor() const Q_DECL_OVERRIDE;

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
#ifndef KDEPIM_NO_WEBKIT
    SecurityPageAdBlockTab *mSAdBlockTab;
#endif
};

#endif // CONFIGURESECURITYPAGE_H
