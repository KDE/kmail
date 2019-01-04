/*
  Copyright (c) 2013-2019 Montel Laurent <montel@kde.org>

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

#ifndef CONFIGUREACCOUNTPAGE_H
#define CONFIGUREACCOUNTPAGE_H

#include "kmail_export.h"
#include "configuredialog_p.h"
#include "ui_accountspagereceivingtab.h"

class QCheckBox;
class KComboBox;
class OrgFreedesktopAkonadiNewMailNotifierInterface;
namespace KLDAP
{
class LdapConfigureWidget;
}
// subclasses: one class per tab:
class AccountsPageSendingTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit AccountsPageSendingTab(QWidget *parent = nullptr);
    ~AccountsPageSendingTab() override;
    QString helpAnchor() const;
    void save() override;

private:
    void doLoadFromGlobalSettings() override;
    void doLoadOther() override;

private:
    QCheckBox *mConfirmSendCheck = nullptr;
    QCheckBox *mCheckSpellingBeforeSending = nullptr;
    KComboBox *mSendOnCheckCombo = nullptr;
    KComboBox *mSendMethodCombo = nullptr;
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
    KLDAP::LdapConfigureWidget *mLdapConfigureWidget = nullptr;
};

class AccountsPageReceivingTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit AccountsPageReceivingTab(QWidget *parent = nullptr);
    ~AccountsPageReceivingTab() override;
    QString helpAnchor() const;
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

    QHash<QString, QSharedPointer<RetrievalOptions> > mRetrievalHash;

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
    explicit AccountsPage(QWidget *parent = nullptr);
    QString helpAnchor() const override;

    // hrmpf. moc doesn't like nested classes with slots/signals...:
    typedef AccountsPageSendingTab SendingTab;
    typedef AccountsPageReceivingTab ReceivingTab;

Q_SIGNALS:
    void accountListChanged(const QStringList &);
};

#endif // CONFIGUREACCOUNTPAGE_H
