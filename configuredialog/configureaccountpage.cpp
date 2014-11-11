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
#include <QDebug>

#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QMenu>
#include <KConfigGroup>
#include <QLabel>

QString AccountsPage::helpAnchor() const
{
    return QString::fromLatin1("configure-accounts");
}

AccountsPage::AccountsPage(QWidget *parent)
    : ConfigModuleWithTabs(parent)
{
    //
    // "Receiving" tab:
    //
    mReceivingTab = new ReceivingTab();
    addTab(mReceivingTab, i18nc("@title:tab Tab page where the user configures accounts to receive mail", "Receiving"));
    connect(mReceivingTab, SIGNAL(accountListChanged(QStringList)),
            this, SIGNAL(accountListChanged(QStringList)));

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
    return QString::fromLatin1("configure-accounts-sending");
}

AccountsPageSendingTab::AccountsPageSendingTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    QVBoxLayout *vlay = new QVBoxLayout(this);
//TODO PORT QT5     vlay->setSpacing( QDialog::spacingHint() );
//TODO PORT QT5     vlay->setMargin( QDialog::marginHint() );
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
//TODO PORT QT5     glay->setSpacing( QDialog::spacingHint() );
    glay->setColumnStretch(2, 10);

    // "confirm before send" check box:
    mConfirmSendCheck = new QCheckBox(i18n("Confirm &before send"), group);
    glay->addWidget(mConfirmSendCheck, 0, 0, 1, 2);
    connect(mConfirmSendCheck, SIGNAL(stateChanged(int)),
            this, SLOT(slotEmitChanged()));

    mCheckSpellingBeforeSending = new QCheckBox(i18n("Check spelling before sending"), group);
    glay->addWidget(mCheckSpellingBeforeSending, 1, 0, 1, 2);
    connect(mCheckSpellingBeforeSending, SIGNAL(stateChanged(int)),
            this, SLOT(slotEmitChanged()));

    // "send on check" combo:
    mSendOnCheckCombo = new KComboBox(group);
    mSendOnCheckCombo->setEditable(false);
    mSendOnCheckCombo->addItems(QStringList()
                                << i18n("Never Automatically")
                                << i18n("On Manual Mail Checks")
                                << i18n("On All Mail Checks"));
    glay->addWidget(mSendOnCheckCombo, 2, 1);
    connect(mSendOnCheckCombo, SIGNAL(activated(int)),
            this, SLOT(slotEmitChanged()));

    // "default send method" combo:
    mSendMethodCombo = new KComboBox(group);
    mSendMethodCombo->setEditable(false);
    mSendMethodCombo->addItems(QStringList()
                               << i18n("Send Now")
                               << i18n("Send Later"));
    glay->addWidget(mSendMethodCombo, 3, 1);
    connect(mSendMethodCombo, SIGNAL(activated(int)),
            this, SLOT(slotEmitChanged()));

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
    return QString::fromLatin1("configure-accounts-receiving");
}

AccountsPageReceivingTab::AccountsPageReceivingTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    mNewMailNotifierInterface = new OrgFreedesktopAkonadiNewMailNotifierInterface(QLatin1String("org.freedesktop.Akonadi.NewMailNotifierAgent"), QLatin1String("/NewMailNotifierAgent"), QDBusConnection::sessionBus(), this);
    if (!mNewMailNotifierInterface->isValid()) {
        qDebug() << " org.freedesktop.Akonadi.NewMailNotifierAgent not found. Please verify your installation";
    }
    mAccountsReceiving.setupUi(this);

//TODO PORT QT5     mAccountsReceiving.vlay->setSpacing( QDialog::spacingHint() );
//TODO PORT QT5     mAccountsReceiving.vlay->setMargin( QDialog::marginHint() );

    mAccountsReceiving.mAccountsReceiving->setMimeTypeFilter(QStringList() << KMime::Message::mimeType());
    mAccountsReceiving.mAccountsReceiving->setCapabilityFilter(QStringList() << QLatin1String("Resource"));
    mAccountsReceiving.mAccountsReceiving->setExcludeCapabilities(QStringList() << QLatin1String("MailTransport") << QLatin1String("Notes"));

    KConfig specialMailCollection(QLatin1String("specialmailcollectionsrc"));
    if (specialMailCollection.hasGroup(QLatin1String("SpecialCollections"))) {
        KConfigGroup grp = specialMailCollection.group(QLatin1String("SpecialCollections"));
        mAccountsReceiving.mAccountsReceiving->setSpecialCollectionIdentifier(grp.readEntry(QLatin1String("DefaultResourceId")));
    }
    ConfigAgentDelegate *configDelegate = new ConfigAgentDelegate(mAccountsReceiving.mAccountsReceiving->view());
    mAccountsReceiving.mAccountsReceiving->setItemDelegate(configDelegate);
    connect(configDelegate, SIGNAL(optionsClicked(QString,QPoint)), this, SLOT(slotShowMailCheckMenu(QString,QPoint)));

//TODO PORT QT5     mAccountsReceiving.group->layout()->setMargin( QDialog::marginHint() );
//TODO PORT QT5     mAccountsReceiving.group->layout()->setSpacing( QDialog::spacingHint() );

    connect(mAccountsReceiving.mBeepNewMailCheck, SIGNAL(stateChanged(int)),
            this, SLOT(slotEmitChanged()));

    connect(mAccountsReceiving.mVerboseNotificationCheck, SIGNAL(stateChanged(int)),
            this, SLOT(slotEmitChanged()));

    connect(mAccountsReceiving.mOtherNewMailActionsButton, SIGNAL(clicked()),
            this, SLOT(slotEditNotifications()));
    connect(mAccountsReceiving.customizeAccountOrder, SIGNAL(clicked()), this, SLOT(slotCustomizeAccountOrder()));
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
        const QString resourceGroupPattern(QLatin1String("Resource %1"));

        KConfigGroup group;
        KConfig *conf = 0;
        if (KMKernel::self()) {
            group = KConfigGroup(KMKernel::self()->config(), resourceGroupPattern.arg(ident));
        } else {
            conf = new KConfig(QLatin1String("kmail2rc"));
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
        QAction *manualMailCheck = new QAction(i18nc("Label to a checkbox, so is either checked/unchecked", "Include in Manual Mail Check"), menu);
        manualMailCheck->setCheckable(true);
        manualMailCheck->setChecked(IncludeInManualChecks);
        manualMailCheck->setData(ident);
        menu->addAction(manualMailCheck);
        connect(manualMailCheck, SIGNAL(toggled(bool)), this, SLOT(slotIncludeInCheckChanged(bool)));
    }

    if (!MailCommon::Util::isLocalCollection(ident)) {
        QAction *switchOffline = new QAction(i18nc("Label to a checkbox, so is either checked/unchecked", "Switch offline on KMail Shutdown"), menu);
        switchOffline->setCheckable(true);
        switchOffline->setChecked(OfflineOnShutdown);
        switchOffline->setData(ident);
        menu->addAction(switchOffline);
        connect(switchOffline, SIGNAL(toggled(bool)), this, SLOT(slotOfflineOnShutdownChanged(bool)));
    }

    QAction *checkOnStartup = new QAction(i18n("Check mail on startup"), menu);
    checkOnStartup->setCheckable(true);
    checkOnStartup->setChecked(CheckOnStartup);
    checkOnStartup->setData(ident);
    menu->addAction(checkOnStartup);

    connect(checkOnStartup, SIGNAL(toggled(bool)), this, SLOT(slotCheckOnStatupChanged(bool)));

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
    QDBusInterface interface(QLatin1String("org.freedesktop.Akonadi.Agent.akonadi_newmailnotifier_agent"), QLatin1String("/NewMailNotifierAgent"));
    if (interface.isValid()) {
        interface.call(QLatin1String("showConfigureDialog"), (qlonglong)winId());
    } else {
        KMessageBox::error(this, i18n("New Mail Notifier Agent not registered. Please contact your administrator."));
    }
}

void AccountsPage::ReceivingTab::doLoadFromGlobalSettings()
{
    mAccountsReceiving.mVerboseNotificationCheck->setChecked(mNewMailNotifierInterface->verboseMailNotification());
}

void AccountsPage::ReceivingTab::doLoadOther()
{
    mAccountsReceiving.mBeepNewMailCheck->setChecked(mNewMailNotifierInterface->beepOnNewMails());
}

void AccountsPage::ReceivingTab::save()
{
    // Save Mail notification settings
    mNewMailNotifierInterface->setBeepOnNewMails(mAccountsReceiving.mBeepNewMailCheck->isChecked());
    mNewMailNotifierInterface->setVerboseMailNotification(mAccountsReceiving.mVerboseNotificationCheck->isChecked());

    const QString resourceGroupPattern(QLatin1String("Resource %1"));
    QHashIterator<QString, QSharedPointer<RetrievalOptions> > it(mRetrievalHash);
    while (it.hasNext()) {
        it.next();
        KConfigGroup group;
        KConfig *conf = 0;
        if (KMKernel::self()) {
            group = KConfigGroup(KMKernel::self()->config(), resourceGroupPattern.arg(it.key()));
        } else {
            conf = new KConfig(QLatin1String("kmail2rc"));
            group = KConfigGroup(conf, resourceGroupPattern.arg(it.key()));
        }
        QSharedPointer<RetrievalOptions> opts = it.value();
        group.writeEntry("IncludeInManualChecks", opts->IncludeInManualChecks);
        group.writeEntry("OfflineOnShutdown", opts->OfflineOnShutdown);
        group.writeEntry("CheckOnStartup", opts->CheckOnStartup);
        delete conf;
    }
}

