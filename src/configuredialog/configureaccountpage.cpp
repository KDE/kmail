/*
  SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "configureaccountpage.h"
#include "configagentdelegate.h"
#include "dialog/kmknotify.h"
#include "kmkernel.h"
#include "newmailnotifierinterface.h"
#include "settings/kmailsettings.h"
#include "undosend/undosendcombobox.h"
#include <MailCommon/AccountConfigOrderDialog>
#include <MessageComposer/MessageComposerSettings>
#include <PimCommon/ConfigureImmutableWidgetUtils>
using namespace PimCommon::ConfigureImmutableWidgetUtils;
#include <MailTransport/TransportManagementWidget>
using MailTransport::TransportManagementWidget;
#include <MailCommon/MailUtil>

#include "kmail_debug.h"
#include <AkonadiCore/agentfilterproxymodel.h>
#include <AkonadiCore/agentinstancecreatejob.h>
#include <AkonadiCore/agentinstancemodel.h>
#include <AkonadiCore/agentmanager.h>
#include <AkonadiCore/agenttype.h>
#include <AkonadiWidgets/AgentConfigurationDialog>
#include <AkonadiWidgets/agenttypedialog.h>
#include <KConfigGroup>
#include <KLDAP/LdapConfigureWidget>
#include <KLocalizedString>
#include <KMessageBox>
#include <QComboBox>
#include <identity/identitypage.h>

#include <QAbstractItemView>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMenu>
#include <QProcess>
#include <QVBoxLayout>

#include <memory>

QString AccountsPage::helpAnchor() const
{
    return QStringLiteral("configure-accounts");
}

AccountsPage::AccountsPage(QWidget *parent, const QVariantList &args)
    : ConfigModuleWithTabs(parent, args)
{
    // Identity Tab:
    auto identityTab = new KMail::IdentityPage();
    addTab(identityTab, i18nc("@title:tab Tab page where the user configures identities", "Identities"));

    //
    // "Receiving" tab:
    //
    auto receivingTab = new ReceivingTab();
    addTab(receivingTab, i18nc("@title:tab Tab page where the user configures accounts to receive mail", "Receiving"));
    connect(receivingTab, &ReceivingTab::accountListChanged, this, &AccountsPage::accountListChanged);

    //
    // "Sending" tab:
    //
    auto sendingTab = new SendingTab();
    addTab(sendingTab, i18nc("@title:tab Tab page where the user configures accounts to send mail", "Sending"));

    //
    // "Sending" tab:
    //
    auto ldapCompletionTab = new LdapCompetionTab();
    addTab(ldapCompletionTab, i18nc("@title:tab Tab page where the user configures ldap server", "LDAP server"));
}

AccountsPageSendingTab::~AccountsPageSendingTab() = default;

QString AccountsPage::SendingTab::helpAnchor() const
{
    return QStringLiteral("configure-accounts-sending");
}

AccountsPageSendingTab::AccountsPageSendingTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    auto vlay = new QVBoxLayout(this);
    // label: zero stretch ### FIXME more
    vlay->addWidget(new QLabel(i18n("Outgoing accounts (add at least one):"), this));

    auto tmw = new TransportManagementWidget(this);
    tmw->layout()->setContentsMargins({});
    vlay->addWidget(tmw);

    // "Common options" groupbox:
    auto group = new QGroupBox(i18n("Common Options"), this);
    vlay->addWidget(group);

    // a grid layout for the contents of the "common options" group box
    auto glay = new QGridLayout();
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
    mSendOnCheckCombo = new QComboBox(group);
    mSendOnCheckCombo->setEditable(false);
    mSendOnCheckCombo->addItems(QStringList() << i18n("Never Automatically") << i18n("On Manual Mail Checks") << i18n("On All Mail Checks"));
    glay->addWidget(mSendOnCheckCombo, 2, 1);
    connect(mSendOnCheckCombo, qOverload<int>(&QComboBox::activated), this, &AccountsPageSendingTab::slotEmitChanged);

    // "default send method" combo:
    mSendMethodCombo = new QComboBox(group);
    mSendMethodCombo->setEditable(false);
    mSendMethodCombo->addItems(QStringList() << i18n("Send Now") << i18n("Send Later"));
    glay->addWidget(mSendMethodCombo, 3, 1);
    connect(mSendMethodCombo, qOverload<int>(&QComboBox::activated), this, &AccountsPageSendingTab::slotEmitChanged);

    // labels:
    auto l = new QLabel(i18n("Send &messages in outbox folder:"), group);
    l->setBuddy(mSendOnCheckCombo);
    glay->addWidget(l, 2, 0);

    QString msg = i18n(KMailSettings::self()->sendOnCheckItem()->whatsThis().toUtf8().constData());
    l->setWhatsThis(msg);
    mSendOnCheckCombo->setWhatsThis(msg);

    l = new QLabel(i18n("Defa&ult send method:"), group);
    l->setBuddy(mSendMethodCombo);
    glay->addWidget(l, 3, 0);

    mUndoSend = new QCheckBox(i18n("Enable Undo Send"), this);
    glay->addWidget(mUndoSend, 4, 0);
    connect(mUndoSend, &QCheckBox::toggled, this, [this](bool state) {
        mUndoSendComboBox->setEnabled(state);
        slotEmitChanged();
    });

    mUndoSendComboBox = new UndoSendCombobox(this);
    mUndoSendComboBox->setEnabled(false);
    glay->addWidget(mUndoSendComboBox, 4, 1);
    connect(mUndoSendComboBox, qOverload<int>(&QComboBox::activated), this, &AccountsPageSendingTab::slotEmitChanged);
}

void AccountsPage::SendingTab::doLoadFromGlobalSettings()
{
    mSendOnCheckCombo->setCurrentIndex(KMailSettings::self()->sendOnCheck());
    loadWidget(mConfirmSendCheck, KMailSettings::self()->confirmBeforeSendItem());
    loadWidget(mCheckSpellingBeforeSending, KMailSettings::self()->checkSpellingBeforeSendItem());
    loadWidget(mUndoSend, KMailSettings::self()->enabledUndoSendItem());
    mUndoSendComboBox->setDelay(KMailSettings::self()->undoSendDelay());
}

void AccountsPage::SendingTab::doLoadOther()
{
    mSendMethodCombo->setCurrentIndex(MessageComposer::MessageComposerSettings::self()->sendImmediate() ? 0 : 1);
    loadWidget(mConfirmSendCheck, KMailSettings::self()->confirmBeforeSendItem());
    loadWidget(mCheckSpellingBeforeSending, KMailSettings::self()->checkSpellingBeforeSendItem());
    loadWidget(mUndoSend, KMailSettings::self()->enabledUndoSendItem());
    mUndoSendComboBox->setDelay(KMailSettings::self()->undoSendDelay());
}

void AccountsPage::SendingTab::save()
{
    KMailSettings::self()->setSendOnCheck(mSendOnCheckCombo->currentIndex());
    saveCheckBox(mConfirmSendCheck, KMailSettings::self()->confirmBeforeSendItem());
    saveCheckBox(mCheckSpellingBeforeSending, KMailSettings::self()->checkSpellingBeforeSendItem());
    saveCheckBox(mUndoSend, KMailSettings::self()->enabledUndoSendItem());
    MessageComposer::MessageComposerSettings::self()->setSendImmediate(mSendMethodCombo->currentIndex() == 0);
    KMailSettings::self()->setUndoSendDelay(mUndoSendComboBox->delay());
}

QString AccountsPage::ReceivingTab::helpAnchor() const
{
    return QStringLiteral("configure-accounts-receiving");
}

AccountsPageReceivingTab::AccountsPageReceivingTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    const auto service = Akonadi::ServerManager::self()->agentServiceName(Akonadi::ServerManager::Agent, QStringLiteral("akonadi_newmailnotifier_agent"));
    mNewMailNotifierInterface =
        new OrgFreedesktopAkonadiNewMailNotifierInterface(service, QStringLiteral("/NewMailNotifierAgent"), QDBusConnection::sessionBus(), this);
    if (!mNewMailNotifierInterface->isValid()) {
        qCDebug(KMAIL_LOG) << " org.freedesktop.Akonadi.NewMailNotifierAgent not found. Please verify your installation";
        delete mNewMailNotifierInterface;
        mNewMailNotifierInterface = nullptr;
    }
    mAccountsReceiving.setupUi(this);

    mAccountsReceiving.mAccountsReceiving->setMimeTypeFilter(QStringList() << KMime::Message::mimeType());
    mAccountsReceiving.mAccountsReceiving->setCapabilityFilter(QStringList() << QStringLiteral("Resource"));
    mAccountsReceiving.mAccountsReceiving->setExcludeCapabilities(QStringList()
                                                                  << QStringLiteral("MailTransport") << QStringLiteral("Notes") << QStringLiteral("Autostart"));

    KConfig specialMailCollection(QStringLiteral("specialmailcollectionsrc"));
    if (specialMailCollection.hasGroup(QStringLiteral("SpecialCollections"))) {
        KConfigGroup grp = specialMailCollection.group(QStringLiteral("SpecialCollections"));
        mAccountsReceiving.mAccountsReceiving->setSpecialCollectionIdentifier(grp.readEntry(QStringLiteral("DefaultResourceId")));
    }
    auto configDelegate = new ConfigAgentDelegate(mAccountsReceiving.mAccountsReceiving->view());
    mAccountsReceiving.mAccountsReceiving->setItemDelegate(configDelegate);
    connect(configDelegate, &ConfigAgentDelegate::optionsClicked, this, &AccountsPageReceivingTab::slotShowMailCheckMenu);

    connect(mAccountsReceiving.mVerboseNotificationCheck, &QCheckBox::stateChanged, this, &ConfigModuleTab::slotEmitChanged);

    connect(mAccountsReceiving.mOtherNewMailActionsButton, &QAbstractButton::clicked, this, &AccountsPageReceivingTab::slotEditNotifications);
    connect(mAccountsReceiving.customizeAccountOrder, &QAbstractButton::clicked, this, &AccountsPageReceivingTab::slotCustomizeAccountOrder);
    mAccountsReceiving.mAccountsReceiving->disconnectAddAccountButton();
    auto accountMenu = new QMenu(this);
    accountMenu->addAction(i18n("Add Mail Account..."), this, &AccountsPageReceivingTab::slotAddMailAccount);
    accountMenu->addAction(i18n("Custom Account..."), this, &AccountsPageReceivingTab::slotAddCustomAccount);
    mAccountsReceiving.mAccountsReceiving->addAccountButton()->setMenu(accountMenu);
}

AccountsPageReceivingTab::~AccountsPageReceivingTab()
{
    delete mNewMailNotifierInterface;
    mRetrievalHash.clear();
}

void AccountsPageReceivingTab::slotAddCustomAccount()
{
    mAccountsReceiving.mAccountsReceiving->slotAddAccount();
}

void AccountsPageReceivingTab::slotAddMailAccount()
{
    const QStringList lst = {QStringLiteral("--type"), QStringLiteral("message/rfc822")};

    const QString path = QStandardPaths::findExecutable(QStringLiteral("accountwizard"));
    if (!QProcess::startDetached(path, lst)) {
        KMessageBox::error(this,
                           i18n("Could not start the account wizard. "
                                "Please make sure you have AccountWizard properly installed."),
                           i18n("Unable to start account wizard"));
    }
}

void AccountsPageReceivingTab::slotCustomizeAccountOrder()
{
    if (KMKernel::self()) {
        QPointer<MailCommon::AccountConfigOrderDialog> dlg = new MailCommon::AccountConfigOrderDialog(KMKernel::self()->mailCommonSettings(), this);
        dlg->exec();
        delete dlg;
    }
}

void AccountsPageReceivingTab::slotShowMailCheckMenu(const QString &ident, const QPoint &pos)
{
    QMenu menu(this);

    bool IncludeInManualChecks;
    bool OfflineOnShutdown;
    bool CheckOnStartup;
    if (!mRetrievalHash.contains(ident)) {
        const QString resourceGroupPattern(QStringLiteral("Resource %1"));

        KConfigGroup group;
        KConfig *conf = nullptr;
        if (KMKernel::self()) {
            group = KConfigGroup(KMKernel::self()->config(), resourceGroupPattern.arg(ident));
        } else {
            conf = new KConfig(QStringLiteral("kmail2rc"));
            group = KConfigGroup(conf, resourceGroupPattern.arg(ident));
        }

        IncludeInManualChecks = group.readEntry("IncludeInManualChecks", true);

        // Keep sync with kmkernel, don't forget to change there.
        OfflineOnShutdown = group.readEntry("OfflineOnShutdown", ident.startsWith(QLatin1String("akonadi_pop3_resource")) ? true : false);

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
        auto manualMailCheck = new QAction(i18nc("Label to a checkbox, so is either checked/unchecked", "Include in Manual Mail Check"), &menu);
        manualMailCheck->setCheckable(true);
        manualMailCheck->setChecked(IncludeInManualChecks);
        manualMailCheck->setData(ident);
        menu.addAction(manualMailCheck);
        connect(manualMailCheck, &QAction::toggled, this, &AccountsPageReceivingTab::slotIncludeInCheckChanged);
    }

    auto switchOffline = new QAction(i18nc("Label to a checkbox, so is either checked/unchecked", "Switch offline on KMail Shutdown"), &menu);
    switchOffline->setCheckable(true);
    switchOffline->setChecked(OfflineOnShutdown);
    switchOffline->setData(ident);
    menu.addAction(switchOffline);
    connect(switchOffline, &QAction::toggled, this, &AccountsPageReceivingTab::slotOfflineOnShutdownChanged);

    auto checkOnStartup = new QAction(i18n("Check mail on startup"), &menu);
    checkOnStartup->setCheckable(true);
    checkOnStartup->setChecked(CheckOnStartup);
    checkOnStartup->setData(ident);
    menu.addAction(checkOnStartup);

    connect(checkOnStartup, &QAction::toggled, this, &AccountsPageReceivingTab::slotCheckOnStatupChanged);

    menu.exec(mAccountsReceiving.mAccountsReceiving->view()->mapToGlobal(pos));
}

void AccountsPageReceivingTab::slotCheckOnStatupChanged(bool checked)
{
    auto action = qobject_cast<QAction *>(sender());
    const QString ident = action->data().toString();

    QSharedPointer<RetrievalOptions> opts = mRetrievalHash.value(ident);
    opts->CheckOnStartup = checked;
    slotEmitChanged();
}

void AccountsPageReceivingTab::slotIncludeInCheckChanged(bool checked)
{
    auto action = qobject_cast<QAction *>(sender());
    const QString ident = action->data().toString();

    QSharedPointer<RetrievalOptions> opts = mRetrievalHash.value(ident);
    opts->IncludeInManualChecks = checked;
    slotEmitChanged();
}

void AccountsPageReceivingTab::slotOfflineOnShutdownChanged(bool checked)
{
    auto action = qobject_cast<QAction *>(sender());
    const QString ident = action->data().toString();

    QSharedPointer<RetrievalOptions> opts = mRetrievalHash.value(ident);
    opts->OfflineOnShutdown = checked;
    slotEmitChanged();
}

void AccountsPage::ReceivingTab::slotEditNotifications()
{
    const auto instance = Akonadi::AgentManager::self()->instance(QStringLiteral("akonadi_newmailnotifier_agent"));
    if (instance.isValid()) {
        std::make_unique<Akonadi::AgentConfigurationDialog>(instance, this)->exec();
    } else {
        KMessageBox::error(this, i18n("New Mail Notifier Agent not registered. Please contact your administrator."));
    }
}

void AccountsPage::ReceivingTab::doLoadFromGlobalSettings()
{
    if (mNewMailNotifierInterface) {
        mAccountsReceiving.mVerboseNotificationCheck->setChecked(mNewMailNotifierInterface->verboseMailNotification());
    }
}

void AccountsPage::ReceivingTab::save()
{
    // Save Mail notification settings
    if (mNewMailNotifierInterface) {
        mNewMailNotifierInterface->setVerboseMailNotification(mAccountsReceiving.mVerboseNotificationCheck->isChecked());
    }

    const QString resourceGroupPattern(QStringLiteral("Resource %1"));
    QHash<QString, QSharedPointer<RetrievalOptions>>::const_iterator it = mRetrievalHash.cbegin();
    const QHash<QString, QSharedPointer<RetrievalOptions>>::const_iterator itEnd = mRetrievalHash.cend();
    for (; it != itEnd; ++it) {
        KConfigGroup group;
        KConfig *conf = nullptr;
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

LdapCompetionTab::LdapCompetionTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins({});

    mLdapConfigureWidget = new KLDAP::LdapConfigureWidget(this);
    layout->addWidget(mLdapConfigureWidget);

    connect(mLdapConfigureWidget, &KLDAP::LdapConfigureWidget::changed, this, qOverload<bool>(&LdapCompetionTab::changed));
}

LdapCompetionTab::~LdapCompetionTab() = default;

QString LdapCompetionTab::helpAnchor() const
{
    return {};
}

void LdapCompetionTab::save()
{
    mLdapConfigureWidget->save();
}

void LdapCompetionTab::doLoadOther()
{
    mLdapConfigureWidget->load();
}
