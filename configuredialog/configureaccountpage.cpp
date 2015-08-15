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

#include "configureaccountpage.h"
#include "dialog/kmknotify.h"
#include "newmailnotifierinterface.h"
#include "kmkernel.h"
#include "settings/globalsettings.h"
#include "configagentdelegate.h"
#include "messagecomposer/settings/messagecomposersettings.h"
#include "mailcommon/folder/accountconfigorderdialog.h"
#include "pimcommon/widgets/configureimmutablewidgetutils.h"
using namespace PimCommon::ConfigureImmutableWidgetUtils;
#include <MailTransport/mailtransport/transportmanagementwidget.h>
using MailTransport::TransportManagementWidget;
#include "ui_accountspagereceivingtab.h"
#include "mailcommon/util/mailutil.h"

#include <AkonadiCore/agentfilterproxymodel.h>
#include <AkonadiCore/agentinstancemodel.h>
#include <AkonadiCore/agenttype.h>
#include <AkonadiCore/agentmanager.h>
#include <AkonadiWidgets/agenttypedialog.h>
#include <AkonadiCore/agentinstancecreatejob.h>

#include <KLocalizedString>
#include <KMessageBox>
#include <KComboBox>
#include <KWindowSystem>
#include "kmail_debug.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QMenu>
#include <KConfigGroup>
#include <QLabel>

QString AccountsPage::helpAnchor() const
{
    return QStringLiteral("configure-accounts");
}

AccountsPage::AccountsPage(QWidget *parent)
    : ConfigModuleWithTabs(parent)
{
    //
    // "Receiving" tab:
    //
    mReceivingTab = new ReceivingTab();
    addTab(mReceivingTab, i18nc("@title:tab Tab page where the user configures accounts to receive mail", "Receiving"));
    connect(mReceivingTab, &ReceivingTab::accountListChanged, this, &AccountsPage::accountListChanged);

    //
    // "Sending" tab:
    //
    mSendingTab = new SendingTab();
    addTab(mSendingTab, i18nc("@title:tab Tab page where the user configures accounts to send mail", "Sending"));
}

AccountsPageSendingTab::~AccountsPageSendingTab()
{
}

QString AccountsPage::SendingTab::helpAnchor() const
{
    return QStringLiteral("configure-accounts-sending");
}

AccountsPageSendingTab::AccountsPageSendingTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    QVBoxLayout *vlay = new QVBoxLayout(this);
    // label: zero stretch ### FIXME more
    vlay->addWidget(new QLabel(i18n("Outgoing accounts (add at least one):"), this));

    TransportManagementWidget *tmw = new TransportManagementWidget(this);
    tmw->layout()->setContentsMargins(0, 0, 0, 0);
    vlay->addWidget(tmw);

    // "Common options" groupbox:
    QGroupBox *group = new QGroupBox(i18n("Common Options"), this);
    vlay->addWidget(group);

    // a grid layout for the contents of the "common options" group box
    QGridLayout *glay = new QGridLayout();
    group->setLayout(glay);
    glay->setColumnStretch(2, 10);

    // "confirm before send" check box:
    mConfirmSendCheck = new QCheckBox(i18n("Confirm &before send"), group);
    glay->addWidget(mConfirmSendCheck, 0, 0, 1, 2);
    connect(mConfirmSendCheck, &QCheckBox::stateChanged, this, &AccountsPageSendingTab::slotEmitChanged);

    mCheckSpellingBeforeSending = new QCheckBox(i18n("Check spelling before sending"), group);
    glay->addWidget(mCheckSpellingBeforeSending, 1, 0, 1, 2);
    connect(mCheckSpellingBeforeSending, &QCheckBox::stateChanged, this, &AccountsPageSendingTab::slotEmitChanged);

    // "send on check" combo:
    mSendOnCheckCombo = new KComboBox(group);
    mSendOnCheckCombo->setEditable(false);
    mSendOnCheckCombo->addItems(QStringList()
                                << i18n("Never Automatically")
                                << i18n("On Manual Mail Checks")
                                << i18n("On All Mail Checks"));
    glay->addWidget(mSendOnCheckCombo, 2, 1);
    connect(mSendOnCheckCombo, static_cast<void (KComboBox::*)(int)>(&KComboBox::activated), this, &AccountsPageSendingTab::slotEmitChanged);

    // "default send method" combo:
    mSendMethodCombo = new KComboBox(group);
    mSendMethodCombo->setEditable(false);
    mSendMethodCombo->addItems(QStringList()
                               << i18n("Send Now")
                               << i18n("Send Later"));
    glay->addWidget(mSendMethodCombo, 3, 1);
    connect(mSendMethodCombo, static_cast<void (KComboBox::*)(int)>(&KComboBox::activated), this, &AccountsPageSendingTab::slotEmitChanged);

    // labels:
    QLabel *l =  new QLabel(i18n("Send &messages in outbox folder:"), group);
    l->setBuddy(mSendOnCheckCombo);
    glay->addWidget(l, 2, 0);

    QString msg = i18n(GlobalSettings::self()->sendOnCheckItem()->whatsThis().toUtf8());
    l->setWhatsThis(msg);
    mSendOnCheckCombo->setWhatsThis(msg);

    l = new QLabel(i18n("Defa&ult send method:"), group);
    l->setBuddy(mSendMethodCombo);
    glay->addWidget(l, 3, 0);
}

void AccountsPage::SendingTab::doLoadFromGlobalSettings()
{
    mSendOnCheckCombo->setCurrentIndex(GlobalSettings::self()->sendOnCheck());
}

void AccountsPage::SendingTab::doLoadOther()
{
    mSendMethodCombo->setCurrentIndex(MessageComposer::MessageComposerSettings::self()->sendImmediate() ? 0 : 1);
    loadWidget(mConfirmSendCheck, GlobalSettings::self()->confirmBeforeSendItem());
    loadWidget(mCheckSpellingBeforeSending, GlobalSettings::self()->checkSpellingBeforeSendItem());
}

void AccountsPage::SendingTab::save()
{
    GlobalSettings::self()->setSendOnCheck(mSendOnCheckCombo->currentIndex());
    saveCheckBox(mConfirmSendCheck, GlobalSettings::self()->confirmBeforeSendItem());
    saveCheckBox(mCheckSpellingBeforeSending, GlobalSettings::self()->checkSpellingBeforeSendItem());
    MessageComposer::MessageComposerSettings::self()->setSendImmediate(mSendMethodCombo->currentIndex() == 0);
}

QString AccountsPage::ReceivingTab::helpAnchor() const
{
    return QStringLiteral("configure-accounts-receiving");
}

AccountsPageReceivingTab::AccountsPageReceivingTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    mNewMailNotifierInterface = new OrgFreedesktopAkonadiNewMailNotifierInterface(QStringLiteral("org.freedesktop.Akonadi.NewMailNotifierAgent"), QStringLiteral("/NewMailNotifierAgent"), QDBusConnection::sessionBus(), this);
    if (!mNewMailNotifierInterface->isValid()) {
        qCDebug(KMAIL_LOG) << " org.freedesktop.Akonadi.NewMailNotifierAgent not found. Please verify your installation";
    }
    mAccountsReceiving.setupUi(this);

    mAccountsReceiving.mAccountsReceiving->setMimeTypeFilter(QStringList() << KMime::Message::mimeType());
    mAccountsReceiving.mAccountsReceiving->setCapabilityFilter(QStringList() << QStringLiteral("Resource"));
    mAccountsReceiving.mAccountsReceiving->setExcludeCapabilities(QStringList() << QStringLiteral("MailTransport") << QStringLiteral("Notes"));

    KConfig specialMailCollection(QStringLiteral("specialmailcollectionsrc"));
    if (specialMailCollection.hasGroup(QStringLiteral("SpecialCollections"))) {
        KConfigGroup grp = specialMailCollection.group(QStringLiteral("SpecialCollections"));
        mAccountsReceiving.mAccountsReceiving->setSpecialCollectionIdentifier(grp.readEntry(QStringLiteral("DefaultResourceId")));
    }
    ConfigAgentDelegate *configDelegate = new ConfigAgentDelegate(mAccountsReceiving.mAccountsReceiving->view());
    mAccountsReceiving.mAccountsReceiving->setItemDelegate(configDelegate);
    connect(configDelegate, &ConfigAgentDelegate::optionsClicked, this, &AccountsPageReceivingTab::slotShowMailCheckMenu);

    connect(mAccountsReceiving.mVerboseNotificationCheck, &QCheckBox::stateChanged, this, &ConfigModuleTab::slotEmitChanged);

    connect(mAccountsReceiving.mOtherNewMailActionsButton, &QAbstractButton::clicked, this, &AccountsPageReceivingTab::slotEditNotifications);
    connect(mAccountsReceiving.customizeAccountOrder, &QAbstractButton::clicked, this, &AccountsPageReceivingTab::slotCustomizeAccountOrder);
}

AccountsPageReceivingTab::~AccountsPageReceivingTab()
{
    delete mNewMailNotifierInterface;
    mRetrievalHash.clear();
}

void AccountsPageReceivingTab::slotCustomizeAccountOrder()
{
    MailCommon::AccountConfigOrderDialog dlg(this);
    dlg.exec();
}

void AccountsPageReceivingTab::slotShowMailCheckMenu(const QString &ident, const QPoint &pos)
{
    QMenu *menu = new QMenu(this);

    bool IncludeInManualChecks;
    bool OfflineOnShutdown;
    bool CheckOnStartup;
    if (!mRetrievalHash.contains(ident)) {
        const QString resourceGroupPattern(QStringLiteral("Resource %1"));

        KConfigGroup group;
        KConfig *conf = Q_NULLPTR;
        if (KMKernel::self()) {
            group = KConfigGroup(KMKernel::self()->config(), resourceGroupPattern.arg(ident));
        } else {
            conf = new KConfig(QStringLiteral("kmail2rc"));
            group = KConfigGroup(conf, resourceGroupPattern.arg(ident));
        }

        IncludeInManualChecks = group.readEntry("IncludeInManualChecks", true);

        // Keep sync with kmkernel, don't forget to change there.
        OfflineOnShutdown = group.readEntry("OfflineOnShutdown", ident.startsWith(QStringLiteral("akonadi_pop3_resource")) ? true : false);

        CheckOnStartup = group.readEntry("CheckOnStartup", false);
        QSharedPointer<RetrievalOptions> opts(new RetrievalOptions(IncludeInManualChecks, OfflineOnShutdown, CheckOnStartup));
        mRetrievalHash.insert(ident, opts);
        delete conf;
    } else {
        QSharedPointer<RetrievalOptions> opts = mRetrievalHash.value(ident);
        IncludeInManualChecks = opts->IncludeInManualChecks;
        OfflineOnShutdown = opts->OfflineOnShutdown;
        CheckOnStartup = opts->CheckOnStartup;
    }

    if (!MailCommon::Util::isVirtualCollection(ident)) {
        QAction *manualMailCheck = new QAction(i18nc("Label to a checkbox, so is either checked/unchecked", "Include in Manual Mail Check"), menu);
        manualMailCheck->setCheckable(true);
        manualMailCheck->setChecked(IncludeInManualChecks);
        manualMailCheck->setData(ident);
        menu->addAction(manualMailCheck);
        connect(manualMailCheck, &QAction::toggled, this, &AccountsPageReceivingTab::slotIncludeInCheckChanged);
    }

    if (!MailCommon::Util::isLocalCollection(ident)) {
        QAction *switchOffline = new QAction(i18nc("Label to a checkbox, so is either checked/unchecked", "Switch offline on KMail Shutdown"), menu);
        switchOffline->setCheckable(true);
        switchOffline->setChecked(OfflineOnShutdown);
        switchOffline->setData(ident);
        menu->addAction(switchOffline);
        connect(switchOffline, &QAction::toggled, this, &AccountsPageReceivingTab::slotOfflineOnShutdownChanged);
    }

    QAction *checkOnStartup = new QAction(i18n("Check mail on startup"), menu);
    checkOnStartup->setCheckable(true);
    checkOnStartup->setChecked(CheckOnStartup);
    checkOnStartup->setData(ident);
    menu->addAction(checkOnStartup);

    connect(checkOnStartup, &QAction::toggled, this, &AccountsPageReceivingTab::slotCheckOnStatupChanged);

    menu->exec(mAccountsReceiving.mAccountsReceiving->view()->mapToGlobal(pos));
    delete menu;
}

void AccountsPageReceivingTab::slotCheckOnStatupChanged(bool checked)
{
    QAction *action = qobject_cast< QAction * >(sender());
    const QString ident = action->data().toString();

    QSharedPointer<RetrievalOptions> opts = mRetrievalHash.value(ident);
    opts->CheckOnStartup = checked;
    slotEmitChanged();
}

void AccountsPageReceivingTab::slotIncludeInCheckChanged(bool checked)
{
    QAction *action = qobject_cast< QAction * >(sender());
    const QString ident = action->data().toString();

    QSharedPointer<RetrievalOptions> opts = mRetrievalHash.value(ident);
    opts->IncludeInManualChecks = checked;
    slotEmitChanged();
}

void AccountsPageReceivingTab::slotOfflineOnShutdownChanged(bool checked)
{
    QAction *action = qobject_cast< QAction * >(sender());
    QString ident = action->data().toString();

    QSharedPointer<RetrievalOptions> opts = mRetrievalHash.value(ident);
    opts->OfflineOnShutdown = checked;
    slotEmitChanged();
}

void AccountsPage::ReceivingTab::slotEditNotifications()
{
    QDBusInterface interface(QStringLiteral("org.freedesktop.Akonadi.Agent.akonadi_newmailnotifier_agent"), QStringLiteral("/NewMailNotifierAgent"));
    if (interface.isValid()) {
        interface.call(QStringLiteral("showConfigureDialog"), (qlonglong)winId());
    } else {
        KMessageBox::error(this, i18n("New Mail Notifier Agent not registered. Please contact your administrator."));
    }
}

void AccountsPage::ReceivingTab::doLoadFromGlobalSettings()
{
    mAccountsReceiving.mVerboseNotificationCheck->setChecked(mNewMailNotifierInterface->verboseMailNotification());
}

void AccountsPage::ReceivingTab::save()
{
    // Save Mail notification settings
    mNewMailNotifierInterface->setVerboseMailNotification(mAccountsReceiving.mVerboseNotificationCheck->isChecked());

    const QString resourceGroupPattern(QStringLiteral("Resource %1"));
    QHashIterator<QString, QSharedPointer<RetrievalOptions> > it(mRetrievalHash);
    while (it.hasNext()) {
        it.next();
        KConfigGroup group;
        KConfig *conf = Q_NULLPTR;
        if (KMKernel::self()) {
            group = KConfigGroup(KMKernel::self()->config(), resourceGroupPattern.arg(it.key()));
        } else {
            conf = new KConfig(QStringLiteral("kmail2rc"));
            group = KConfigGroup(conf, resourceGroupPattern.arg(it.key()));
        }
        QSharedPointer<RetrievalOptions> opts = it.value();
        group.writeEntry("IncludeInManualChecks", opts->IncludeInManualChecks);
        group.writeEntry("OfflineOnShutdown", opts->OfflineOnShutdown);
        group.writeEntry("CheckOnStartup", opts->CheckOnStartup);
        delete conf;
    }
}

