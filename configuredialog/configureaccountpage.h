/*
  Copyright (c) 2013, 2014 Montel Laurent <montel@kde.org>

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

// subclasses: one class per tab:
class AccountsPageSendingTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit AccountsPageSendingTab(QWidget *parent = 0);
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
};

class AccountsPageReceivingTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit AccountsPageReceivingTab(QWidget *parent = 0);
    ~AccountsPageReceivingTab();
    QString helpAnchor() const;
    void save();

signals:
    void accountListChanged(const QStringList &);

private slots:
    void slotEditNotifications();
    void slotShowMailCheckMenu(const QString &, const QPoint &);
    void slotCustomizeAccountOrder();
    void slotIncludeInCheckChanged(bool checked);
    void slotOfflineOnShutdownChanged(bool checked);
    void slotCheckOnStatupChanged(bool checked);

private:
    void doLoadFromGlobalSettings();
    void doLoadOther();

    struct RetrievalOptions {
        RetrievalOptions(bool manualCheck, bool offline, bool checkOnStartup)
            : IncludeInManualChecks(manualCheck)
            , OfflineOnShutdown(offline)
            , CheckOnStartup(checkOnStartup) {}
        bool IncludeInManualChecks;
        bool OfflineOnShutdown;
        bool CheckOnStartup;
    };

    QHash<QString, QSharedPointer<RetrievalOptions> > mRetrievalHash;
    //FIXME virtual void doResetToDefaultsOther();

private:
    Ui_AccountsPageReceivingTab mAccountsReceiving;
    OrgFreedesktopAkonadiNewMailNotifierInterface *mNewMailNotifierInterface;
};

class KMAIL_EXPORT AccountsPage : public ConfigModuleWithTabs
{
    Q_OBJECT
public:
    explicit AccountsPage(QWidget *parent = 0);
    QString helpAnchor() const;

    // hrmpf. moc doesn't like nested classes with slots/signals...:
    typedef AccountsPageSendingTab SendingTab;
    typedef AccountsPageReceivingTab ReceivingTab;

signals:
    void accountListChanged(const QStringList &);

private:
    SendingTab   *mSendingTab;
    ReceivingTab *mReceivingTab;
};

#endif // CONFIGUREACCOUNTPAGE_H
