/*
  SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include "configuredialog_p.h"
#include "kmail_export.h"
#include "ui_composercryptoconfiguration.h"
#include "ui_securitypagegeneraltab.h"
#include "ui_securitypagemdntab.h"
#include "ui_smimeconfiguration.h"
#include "ui_warningconfiguration.h"

#include <KCMultiDialog>

namespace QGpgME
{
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
    void slotOpenExternalReferenceExceptions();
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
    explicit SecurityPage(QWidget *parent = nullptr, const QVariantList &args = {});

    QString helpAnchor() const override;

    using ReadingTab = SecurityPageGeneralTab;
    using MDNTab = SecurityPageMDNTab;
    using ComposerCryptoTab = SecurityPageComposerCryptoTab;
    using WarningTab = SecurityPageWarningTab;
    using SMimeTab = SecurityPageSMimeTab;
};

