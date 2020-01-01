/*
  Copyright (c) 2013-2020 Laurent Montel <montel@kde.org>

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

#include <KCMultiDialog>

namespace QGpgME {
class CryptoConfig;
}

class SecurityPageGeneralTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit SecurityPageGeneralTab(QWidget *parent = nullptr);
    Q_REQUIRED_RESULT QString helpAnchor() const;

    void save() override;

private:
    void doLoadOther() override;

private:
    void slotLinkClicked(const QString &link);
    Ui_SecurityPageGeneralTab mSGTab;
};

class SecurityPageMDNTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit SecurityPageMDNTab(QWidget *parent = nullptr);
    QString helpAnchor() const;

    void save() override;

private:
    void doLoadOther() override;

private:
    void slotLinkClicked(const QString &link);
    QButtonGroup *mMDNGroup = nullptr;
    QButtonGroup *mOrigQuoteGroup = nullptr;
    Ui_SecurityPageMDNTab mUi;
};

class SecurityPageComposerCryptoTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit SecurityPageComposerCryptoTab(QWidget *parent = nullptr);
    ~SecurityPageComposerCryptoTab() override;

    QString helpAnchor() const;

    void save() override;

private:
    void doLoadFromGlobalSettings() override;
    void doLoadOther() override;

private:
    Ui::ComposerCryptoConfiguration *mWidget = nullptr;
};

class SecurityPageWarningTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit SecurityPageWarningTab(QWidget *parent = nullptr);
    ~SecurityPageWarningTab() override;

    QString helpAnchor() const;

    void save() override;

private:
    void slotReenableAllWarningsClicked();
    void slotConfigureGnupg();
    void doLoadFromGlobalSettings() override;
    void doLoadOther() override;

private:
    Ui::WarningConfiguration *mWidget = nullptr;
};

class SecurityPageSMimeTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit SecurityPageSMimeTab(QWidget *parent = nullptr);
    ~SecurityPageSMimeTab() override;

    QString helpAnchor() const;

    void save() override;

private Q_SLOTS:
    void slotUpdateHTTPActions();

private:
    void doLoadOther() override;

private:
    Ui::SMimeConfiguration *mWidget = nullptr;
    QGpgME::CryptoConfig *mConfig = nullptr;
};

class GpgSettingsDialog : public KCMultiDialog
{
    Q_OBJECT
public:
    explicit GpgSettingsDialog(QWidget *parent = nullptr);
    ~GpgSettingsDialog();
private:
    void readConfig();
    void saveConfig();
};

class KMAIL_EXPORT SecurityPage : public ConfigModuleWithTabs
{
    Q_OBJECT
public:
    explicit SecurityPage(QWidget *parent = nullptr);

    QString helpAnchor() const override;

    typedef SecurityPageGeneralTab ReadingTab;
    typedef SecurityPageMDNTab MDNTab;
    typedef SecurityPageComposerCryptoTab ComposerCryptoTab;
    typedef SecurityPageWarningTab WarningTab;
    typedef SecurityPageSMimeTab SMimeTab;
};

#endif // CONFIGURESECURITYPAGE_H
