/*
  SPDX-FileCopyrightText: 2013-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "configuredialog_p.h"
#include "kmail_export.h"
#include "ui_accountspagereceivingtab.h"

class QCheckBox;
class QComboBox;
class UndoSendCombobox;
class OrgFreedesktopAkonadiNewMailNotifierInterface;
namespace KLDAPWidgets
{
class LdapConfigureWidgetNg;
}
// subclasses: one class per tab:
class AccountsPageSendingTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit AccountsPageSendingTab(QWidget *parent = nullptr);
    ~AccountsPageSendingTab() override;
    [[nodiscard]] QString helpAnchor() const;
    void save() override;

private:
    void doLoadFromGlobalSettings() override;
    void doLoadOther() override;

private:
    QCheckBox *mConfirmSendCheck = nullptr;
    QCheckBox *mCheckSpellingBeforeSending = nullptr;
    QComboBox *mSendOnCheckCombo = nullptr;
    QComboBox *mSendMethodCombo = nullptr;
    UndoSendCombobox *mUndoSendComboBox = nullptr;
    QCheckBox *mUndoSend = nullptr;
};

// subclasses: one class per tab:
class LdapCompetionTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit LdapCompetionTab(QWidget *parent = nullptr);
    ~LdapCompetionTab() override;
    QString helpAnchor() const;
    void save() override;

private:
    void doLoadOther() override;

private:
    KLDAPWidgets::LdapConfigureWidgetNg *const mLdapConfigureWidget;
};

class AccountsPageReceivingTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit AccountsPageReceivingTab(QWidget *parent = nullptr);
    ~AccountsPageReceivingTab() override;
    [[nodiscard]] QString helpAnchor() const;
    void save() override;

Q_SIGNALS:
    void accountListChanged(const QStringList &);

private:
    void slotEditNotifications();
    void slotShowMailCheckMenu(const QString &, const QPoint &);
    void slotCustomizeAccountOrder();
    void slotIncludeInCheckChanged(bool checked);
    void slotOfflineOnShutdownChanged(bool checked);
    void slotCheckOnStatupChanged(bool checked);
    void doLoadFromGlobalSettings() override;

    struct RetrievalOptions {
        RetrievalOptions(bool manualCheck, bool offline, bool checkOnStartup)
            : IncludeInManualChecks(manualCheck)
            , OfflineOnShutdown(offline)
            , CheckOnStartup(checkOnStartup)
        {
        }

        bool IncludeInManualChecks = false;
        bool OfflineOnShutdown = false;
        bool CheckOnStartup = false;
    };

    QHash<QString, QSharedPointer<RetrievalOptions>> mRetrievalHash;

private:
    void slotAddCustomAccount();
    void slotAddMailAccount();
    Ui_AccountsPageReceivingTab mAccountsReceiving;
    OrgFreedesktopAkonadiNewMailNotifierInterface *mNewMailNotifierInterface = nullptr;
};

class KMAIL_EXPORT AccountsPage : public ConfigModuleWithTabs
{
    Q_OBJECT
public:
    explicit AccountsPage(QObject *parent, const KPluginMetaData &data);
    [[nodiscard]] QString helpAnchor() const override;

Q_SIGNALS:
    void accountListChanged(const QStringList &);
};
