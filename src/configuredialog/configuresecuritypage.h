/*
  SPDX-FileCopyrightText: 2013-2023 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include "configuredialog_p.h"
#include "kmail_export.h"
#include "ui_securitypageencryptiontab.h"
#include "ui_securitypagegeneraltab.h"
#include "ui_securitypagemdntab.h"
#include "ui_smimeconfiguration.h"

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
    Q_REQUIRED_RESULT QString helpAnchor() const;

    void save() override;

private:
    void doLoadOther() override;

private:
    void slotLinkClicked(const QString &link);
    QButtonGroup *mMDNGroup = nullptr;
    QButtonGroup *mOrigQuoteGroup = nullptr;
    Ui_SecurityPageMDNTab mUi;
};

class SecurityPageEncryptionTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit SecurityPageEncryptionTab(QWidget *parent = nullptr);
    ~SecurityPageEncryptionTab() override;

    Q_REQUIRED_RESULT QString helpAnchor() const;

    void save() override;

private:
    void slotReenableAllWarningsClicked();
    void slotConfigureGnupg();
    void doLoadFromGlobalSettings() override;
    void doLoadOther() override;

private:
    Ui::SecurityPageEncryptionTab *mWidget = nullptr;
};

class SecurityPageSMimeTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit SecurityPageSMimeTab(QWidget *parent = nullptr);
    ~SecurityPageSMimeTab() override;

    Q_REQUIRED_RESULT QString helpAnchor() const;

    void save() override;

private:
    void slotUpdateHTTPActions();
    void doLoadOther() override;

private:
    Ui::SMimeConfiguration *const mWidget;
    QGpgME::CryptoConfig *mConfig = nullptr;
};

class GpgSettingsDialog : public KCMultiDialog
{
    Q_OBJECT
public:
    explicit GpgSettingsDialog(QWidget *parent = nullptr);
    ~GpgSettingsDialog() override;

private:
    void readConfig();
    void saveConfig();
};

class KMAIL_EXPORT SecurityPage : public ConfigModuleWithTabs
{
    Q_OBJECT
public:
    explicit SecurityPage(QWidget *parent = nullptr, const QVariantList &args = {});

    Q_REQUIRED_RESULT QString helpAnchor() const override;
};
