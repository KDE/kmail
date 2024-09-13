/*
  SPDX-FileCopyrightText: 2013-2024 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "configureaccountpage.h"
using namespace Qt::Literals::StringLiterals;

#include "config-kmail.h"
#include "configagentdelegate.h"
#include "kmkernel.h"
#include "newmailnotifierinterface.h"
#include "settings/kmailsettings.h"
#include "undosend/undosendcombobox.h"
#include <MailCommon/AccountConfigOrderDialog>
#include <MailTransport/TransportManagementWidgetNg>
#include <MessageComposer/MessageComposerSettings>
using MailTransport::TransportManagementWidgetNg;
#include <MailCommon/MailUtil>

#include "identity/identityngpage.h"
#include "identity/identitypage.h"
#include <Akonadi/AgentConfigurationDialog>
#include <Akonadi/AgentManager>
#include <Akonadi/AgentType>
#include <Akonadi/AgentTypeDialog>
#include <KConfigGroup>
#include <KLDAPWidgets/LdapConfigureWidget>
#include <KLocalizedString>
#include <KMessageBox>
#include <QComboBox>

#include <QAbstractItemView>
#include <QFormLayout>
#include <QLabel>
#include <QMenu>
#include <QProcess>

#if KMAIL_HAVE_ACTIVITY_SUPPORT
#include "activities/activitiesmanager.h"
#include "activities/transportactivities.h"
#endif
#include <memory>

QString AccountsPage::helpAnchor() const
{
    return QStringLiteral("configure-accounts");
}

AccountsPage::AccountsPage(QObject *parent, const KPluginMetaData &data)
    : ConfigModuleWithTabs(parent, data)
{
    // Identity Tab:
    auto identityTab = new KMail::IdentityPage();
    addTab(identityTab, i18nc("@title:tab Tab page where the user configures identities", "Identities"));

    // Identity Tab:
    auto identityNgTab = new KMail::IdentityNgPage();
    addTab(identityNgTab, QStringLiteral("Test identity ng"));

    //
    // "Receiving" tab:
    //
    auto receivingTab = new AccountsPageReceivingTab();
    addTab(receivingTab, i18nc("@title:tab Tab page where the user configures accounts to receive mail", "Receiving"));
    connect(receivingTab, &AccountsPageReceivingTab::accountListChanged, this, &AccountsPage::accountListChanged);

    //
    // "Sending" tab:
    //
    auto sendingTab = new AccountsPageSendingTab();
    addTab(sendingTab, i18nc("@title:tab Tab page where the user configures accounts to send mail", "Sending"));

    //
    // "Sending" tab:
    //
    auto ldapCompletionTab = new LdapCompetionTab();
    addTab(ldapCompletionTab, i18nc("@title:tab Tab page where the user configures ldap server", "LDAP server"));
}

AccountsPageSendingTab::~AccountsPageSendingTab() = default;

QString AccountsPageSendingTab::helpAnchor() const
{
    return QStringLiteral("configure-accounts-sending");
}

AccountsPageSendingTab::AccountsPageSendingTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    auto formLayout = new QFormLayout(this);
    // label: zero stretch ### FIXME more
    formLayout->addRow(new QLabel(i18nc("@label:textbox", "Outgoing accounts (add at least one):"), this));

    auto tmw = new TransportManagementWidgetNg(this);
    tmw->layout()->setContentsMargins({});
    formLayout->addRow(tmw);
#if KMAIL_HAVE_ACTIVITY_SUPPORT
    tmw->setEnablePlasmaActivities(KMailSettings::self()->plasmaActivitySupport());
    tmw->setTransportActivitiesAbstract(ActivitiesManager::self()->transportActivities());
#endif

    // "confirm before send" check box:
    mConfirmSendCheck = new QCheckBox(i18nc("@option:check", "&Confirm action"), this);
    mConfirmSendCheck->setObjectName(u"kcfg_ConfirmBeforeSend"_s);
    formLayout->addRow(i18n("Before sending:"), mConfirmSendCheck);

    mCheckSpellingBeforeSending = new QCheckBox(i18nc("@option:check", "Check spelling"), this);
    mCheckSpellingBeforeSending->setObjectName(u"kcfg_CheckSpellingBeforeSend"_s);
    formLayout->addRow(QString(), mCheckSpellingBeforeSending);

    // "send on check" combo:
    mSendOnCheckCombo = new QComboBox(this);
    mSendOnCheckCombo->setEditable(false);
    mSendOnCheckCombo->addItems(QStringList() << i18n("Never Automatically") << i18n("On Manual Mail Checks") << i18n("On All Mail Checks"));
    mSendOnCheckCombo->setWhatsThis(i18n(KMailSettings::self()->sendOnCheckItem()->whatsThis().toUtf8().constData()));
    formLayout->addRow(i18n("Send &messages in outbox folder:"), mSendOnCheckCombo);
    connect(mSendOnCheckCombo, &QComboBox::activated, this, &AccountsPageSendingTab::slotEmitChanged);

    // "default send method" combo:
    mSendMethodCombo = new QComboBox(this);
    mSendMethodCombo->setEditable(false);
    mSendMethodCombo->addItems(QStringList() << i18n("Send Now") << i18n("Send Later"));
    formLayout->addRow(i18n("Defa&ult send method:"), mSendMethodCombo);
    connect(mSendMethodCombo, &QComboBox::activated, this, &AccountsPageSendingTab::slotEmitChanged);

    auto hLayout = new QHBoxLayout;
    mUndoSend = new QCheckBox(i18nc("@option:check", "Enable Undo Send"), this);
    mUndoSend->setObjectName(u"kcfg_EnabledUndoSend"_s);
    hLayout->addWidget(mUndoSend);
    connect(mUndoSend, &QCheckBox::toggled, this, [this](bool state) {
        mUndoSendComboBox->setEnabled(state);
    });

    mUndoSendComboBox = new UndoSendCombobox(this);
    mUndoSendComboBox->setEnabled(false);
    hLayout->addWidget(mUndoSendComboBox);
    formLayout->addRow(QString(), hLayout);
    connect(mUndoSendComboBox, &QComboBox::activated, this, &AccountsPageSendingTab::slotEmitChanged);
}

void AccountsPageSendingTab::doLoadFromGlobalSettings()
{
    mSendOnCheckCombo->setCurrentIndex(KMailSettings::self()->sendOnCheck());
    mUndoSendComboBox->setDelay(KMailSettings::self()->undoSendDelay());
}

void AccountsPageSendingTab::doLoadOther()
{
    mSendMethodCombo->setCurrentIndex(MessageComposer::MessageComposerSettings::self()->sendImmediate() ? 0 : 1);
    mUndoSendComboBox->setDelay(KMailSettings::self()->undoSendDelay());
}

void AccountsPageSendingTab::save()
{
    KMailSettings::self()->setSendOnCheck(mSendOnCheckCombo->currentIndex());
    MessageComposer::MessageComposerSettings::self()->setSendImmediate(mSendMethodCombo->currentIndex() == 0);
    KMailSettings::self()->setUndoSendDelay(mUndoSendComboBox->delay());
}

QString AccountsPageReceivingTab::helpAnchor() const
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

    connect(mAccountsReceiving.mVerboseNotificationCheck, &QCheckBox::checkStateChanged, this, &ConfigModuleTab::slotEmitChanged);

    connect(mAccountsReceiving.mOtherNewMailActionsButton, &QAbstractButton::clicked, this, &AccountsPageReceivingTab::slotEditNotifications);
    connect(mAccountsReceiving.customizeAccountOrder, &QAbstractButton::clicked, this, &AccountsPageReceivingTab::slotCustomizeAccountOrder);
    mAccountsReceiving.mAccountsReceiving->disconnectAddAccountButton();
    auto accountMenu = new QMenu(this);
    accountMenu->addAction(i18n("Add Mail Account…"), this, &AccountsPageReceivingTab::slotAddMailAccount);
    accountMenu->addAction(i18n("Custom Account…"), this, &AccountsPageReceivingTab::slotAddCustomAccount);
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
    const QString path = QStandardPaths::findExecutable(QStringLiteral("accountwizard"));
    if (path.isEmpty() || !QProcess::startDetached(path, {})) {
        KMessageBox::error(this,
                           i18n("Could not start the account wizard. "
                                "Please make sure you have AccountWizard properly installed."),
                           i18nc("@title:window", "Unable to start account wizard"));
    }
}

void AccountsPageReceivingTab::slotCustomizeAccountOrder()
{
    if (KMKernel::self()) {
        MailCommon::AccountConfigOrderDialog dlg(KMKernel::self()->mailCommonSettings(), this);
        dlg.exec();
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
        OfflineOnShutdown = group.readEntry("OfflineOnShutdown", ident.startsWith("akonadi_pop3_resource"_L1) ? true : false);

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

    auto checkOnStartup = new QAction(i18nc("@action", "Check mail on startup"), &menu);
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

void AccountsPageReceivingTab::slotEditNotifications()
{
    const auto instance = Akonadi::AgentManager::self()->instance(QStringLiteral("akonadi_newmailnotifier_agent"));
    if (instance.isValid()) {
        std::make_unique<Akonadi::AgentConfigurationDialog>(instance, this)->exec();
    } else {
        KMessageBox::error(this, i18n("New Mail Notifier Agent not registered. Please contact your administrator."));
    }
}

void AccountsPageReceivingTab::doLoadFromGlobalSettings()
{
    if (mNewMailNotifierInterface) {
        mAccountsReceiving.mVerboseNotificationCheck->setChecked(mNewMailNotifierInterface->verboseMailNotification());
    }
}

void AccountsPageReceivingTab::save()
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
    , mLdapConfigureWidget(new KLDAPWidgets::LdapConfigureWidget(this))
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins({});

    layout->addWidget(mLdapConfigureWidget);

    connect(mLdapConfigureWidget, &KLDAPWidgets::LdapConfigureWidget::changed, this, qOverload<bool>(&LdapCompetionTab::changed));
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

#include "moc_configureaccountpage.cpp"
