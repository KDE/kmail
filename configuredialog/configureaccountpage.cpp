/*
  Copyright (c) 2013 Montel Laurent <montel@kde.org>

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
#include "accountconfigorderdialog.h"
#include "dialog/kmknotify.h"
#include "newmailnotifierinterface.h"
#include "kmkernel.h"
#include "settings/globalsettings.h"
#include "configagentdelegate.h"
#include "messagecomposer/settings/messagecomposersettings.h"
#include "configuredialog/accountconfigorderdialog.h"
#include <mailtransport/transportmanagementwidget.h>
using MailTransport::TransportManagementWidget;
#include "accountconfigorderdialog.h"
#include <kpimidentities/identitymanager.h>
#include "ui_accountspagereceivingtab.h"
#include "mailcommon/util/mailutil.h"

#include <akonadi/agentfilterproxymodel.h>
#include <akonadi/agentinstancemodel.h>
#include <akonadi/agenttype.h>
#include <akonadi/agentmanager.h>
#include <akonadi/agenttypedialog.h>
#include <akonadi/agentinstancecreatejob.h>


#include <KLocale>
#include <KMessageBox>
#include <KComboBox>
#include <KWindowSystem>
#include <KLineEdit>

#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHostInfo>
#include <QMenu>

QString AccountsPage::helpAnchor() const
{
    return QString::fromLatin1("configure-accounts");
}

AccountsPage::AccountsPage( const KComponentData &instance, QWidget *parent )
    : ConfigModuleWithTabs( instance, parent )
{
    //
    // "Receiving" tab:
    //
    mReceivingTab = new ReceivingTab();
    addTab( mReceivingTab, i18nc( "@title:tab Tab page where the user configures accounts to receive mail", "Receiving" ) );
    connect( mReceivingTab, SIGNAL(accountListChanged(QStringList)),
             this, SIGNAL(accountListChanged(QStringList)) );

    //
    // "Sending" tab:
    //
    mSendingTab = new SendingTab();
    addTab( mSendingTab, i18nc( "@title:tab Tab page where the user configures accounts to send mail", "Sending" ) );
}

AccountsPageSendingTab::~AccountsPageSendingTab()
{
}

QString AccountsPage::SendingTab::helpAnchor() const
{
    return QString::fromLatin1("configure-accounts-sending");
}

AccountsPageSendingTab::AccountsPageSendingTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    // temp. vars:
    QVBoxLayout *vlay;
    QGridLayout *glay;
    QGroupBox   *group;

    vlay = new QVBoxLayout( this );
    vlay->setSpacing( KDialog::spacingHint() );
    vlay->setMargin( KDialog::marginHint() );
    // label: zero stretch ### FIXME more
    vlay->addWidget( new QLabel( i18n("Outgoing accounts (add at least one):"), this ) );

    TransportManagementWidget *tmw = new TransportManagementWidget( this );
    tmw->layout()->setContentsMargins( 0, 0, 0, 0 );
    vlay->addWidget( tmw );

    // "Common options" groupbox:
    group = new QGroupBox( i18n("Common Options"), this );
    vlay->addWidget(group);

    // a grid layout for the contents of the "common options" group box
    glay = new QGridLayout();
    group->setLayout( glay );
    glay->setSpacing( KDialog::spacingHint() );
    glay->setColumnStretch( 2, 10 );

    // "confirm before send" check box:
    mConfirmSendCheck = new QCheckBox( i18n("Confirm &before send"), group );
    glay->addWidget( mConfirmSendCheck, 0, 0, 1, 2 );
    connect( mConfirmSendCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );

    mCheckSpellingBeforeSending = new QCheckBox( i18n("Check spelling before sending"), group );
    glay->addWidget( mCheckSpellingBeforeSending, 1, 0, 1, 2 );
    connect( mCheckSpellingBeforeSending, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );

    // "send on check" combo:
    mSendOnCheckCombo = new KComboBox( group );
    mSendOnCheckCombo->setEditable( false );
    mSendOnCheckCombo->addItems( QStringList()
                                 << i18n("Never Automatically")
                                 << i18n("On Manual Mail Checks")
                                 << i18n("On All Mail Checks") );
    glay->addWidget( mSendOnCheckCombo, 2, 1 );
    connect( mSendOnCheckCombo, SIGNAL(activated(int)),
             this, SLOT(slotEmitChanged()) );

    // "default send method" combo:
    mSendMethodCombo = new KComboBox( group );
    mSendMethodCombo->setEditable( false );
    mSendMethodCombo->addItems( QStringList()
                                << i18n("Send Now")
                                << i18n("Send Later") );
    glay->addWidget( mSendMethodCombo, 3, 1 );
    connect( mSendMethodCombo, SIGNAL(activated(int)),
             this, SLOT(slotEmitChanged()) );

    // "default domain" input field:
    mDefaultDomainEdit = new KLineEdit( group );
    glay->addWidget( mDefaultDomainEdit, 4, 1 );
    connect( mDefaultDomainEdit, SIGNAL(textChanged(QString)),
             this, SLOT(slotEmitChanged()) );

    // labels:
    QLabel *l =  new QLabel( i18n("Send &messages in outbox folder:"), group );
    l->setBuddy( mSendOnCheckCombo );
    glay->addWidget( l, 2, 0 );

    QString msg = i18n( GlobalSettings::self()->sendOnCheckItem()->whatsThis().toUtf8() );
    l->setWhatsThis( msg );
    mSendOnCheckCombo->setWhatsThis( msg );

    l = new QLabel( i18n("Defa&ult send method:"), group );
    l->setBuddy( mSendMethodCombo );
    glay->addWidget( l, 3, 0 );
    l = new QLabel( i18n("Defaul&t domain:"), group );
    l->setBuddy( mDefaultDomainEdit );
    glay->addWidget( l, 4, 0 );

    // and now: add QWhatsThis:
    msg = i18n( "<qt><p>The default domain is used to complete email "
                "addresses that only consist of the user's name."
                "</p></qt>" );
    l->setWhatsThis( msg );
    mDefaultDomainEdit->setWhatsThis( msg );
}

void AccountsPage::SendingTab::doLoadFromGlobalSettings()
{
    mSendOnCheckCombo->setCurrentIndex( GlobalSettings::self()->sendOnCheck() );
}

void AccountsPage::SendingTab::doLoadOther()
{
    mSendMethodCombo->setCurrentIndex( MessageComposer::MessageComposerSettings::self()->sendImmediate() ? 0 : 1 );
    mConfirmSendCheck->setChecked( GlobalSettings::self()->confirmBeforeSend() );
    mCheckSpellingBeforeSending->setChecked(GlobalSettings::self()->checkSpellingBeforeSend());
    QString defaultDomain = MessageComposer::MessageComposerSettings::defaultDomain();
    if( defaultDomain.isEmpty() ) {
        defaultDomain = QHostInfo::localHostName();
    }
    mDefaultDomainEdit->setText( defaultDomain );
}

void AccountsPage::SendingTab::save()
{
    GlobalSettings::self()->setSendOnCheck( mSendOnCheckCombo->currentIndex() );
    MessageComposer::MessageComposerSettings::self()->setDefaultDomain( mDefaultDomainEdit->text() );
    GlobalSettings::self()->setConfirmBeforeSend( mConfirmSendCheck->isChecked() );
    GlobalSettings::self()->setCheckSpellingBeforeSend(mCheckSpellingBeforeSending->isChecked());
    MessageComposer::MessageComposerSettings::self()->setSendImmediate( mSendMethodCombo->currentIndex() == 0 );
}

QString AccountsPage::ReceivingTab::helpAnchor() const
{
    return QString::fromLatin1("configure-accounts-receiving");
}

AccountsPageReceivingTab::AccountsPageReceivingTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    mNewMailNotifierInterface = new OrgFreedesktopAkonadiNewMailNotifierInterface(QLatin1String("org.freedesktop.Akonadi.NewMailNotifierAgent"), QLatin1String("/NewMailNotifierAgent"), QDBusConnection::sessionBus(), this);
    if (!mNewMailNotifierInterface->isValid()) {
        kDebug()<<" org.freedesktop.Akonadi.NewMailNotifierAgent not found. Please verify your installation";
    }
    mAccountsReceiving.setupUi( this );

    mAccountsReceiving.vlay->setSpacing( KDialog::spacingHint() );
    mAccountsReceiving.vlay->setMargin( KDialog::marginHint() );

    mAccountsReceiving.mAccountList->agentFilterProxyModel()->addMimeTypeFilter( KMime::Message::mimeType() );
#ifdef MERGE_KNODE_IN_KMAIL
    mAccountsReceiving.mAccountList->agentFilterProxyModel()->addMimeTypeFilter( QString::fromLatin1("message/news") );
#endif

    mAccountsReceiving.mAccountList->agentFilterProxyModel()->addCapabilityFilter( QLatin1String("Resource") ); // show only resources, no agents
    mAccountsReceiving.mAccountList->agentFilterProxyModel()->excludeCapabilities( QLatin1String("MailTransport") );
    mAccountsReceiving.mAccountList->agentFilterProxyModel()->excludeCapabilities( QLatin1String("Notes") );
    mAccountsReceiving.mAccountList->view()->setSelectionMode( QAbstractItemView::SingleSelection );

    mAccountsReceiving.mFilterAccount->setProxy( mAccountsReceiving.mAccountList->agentFilterProxyModel() );
    mAccountsReceiving.mFilterAccount->lineEdit()->setTrapReturnKey( true );

    KConfig specialMailCollection(QLatin1String("specialmailcollectionsrc"));
    if(specialMailCollection.hasGroup(QLatin1String("SpecialCollections"))) {
        KConfigGroup grp = specialMailCollection.group(QLatin1String("SpecialCollections"));
        mSpecialMailCollectionIdentifier = grp.readEntry(QLatin1String("DefaultResourceId"));
    }

    ConfigAgentDelegate *configDelegate = new ConfigAgentDelegate( mAccountsReceiving.mAccountList->view() );
    mAccountsReceiving.mAccountList->view()->setItemDelegate( configDelegate );
    connect( configDelegate, SIGNAL(optionsClicked(QString,QPoint)), this, SLOT(slotShowMailCheckMenu(QString,QPoint)) );

    connect( mAccountsReceiving.mAccountList, SIGNAL(clicked(Akonadi::AgentInstance)),
             SLOT(slotAccountSelected(Akonadi::AgentInstance)) );
    connect( mAccountsReceiving.mAccountList, SIGNAL(doubleClicked(Akonadi::AgentInstance)),
             this, SLOT(slotModifySelectedAccount()) );

    mAccountsReceiving.hlay->insertWidget(0, mAccountsReceiving.mAccountList);

    connect( mAccountsReceiving.mAddAccountButton, SIGNAL(clicked()),
             this, SLOT(slotAddAccount()) );

    connect( mAccountsReceiving.mModifyAccountButton, SIGNAL(clicked()),
             this, SLOT(slotModifySelectedAccount()) );

    connect( mAccountsReceiving.mRemoveAccountButton, SIGNAL(clicked()),
             this, SLOT(slotRemoveSelectedAccount()) );
    connect( mAccountsReceiving.mRestartAccountButton, SIGNAL(clicked()),
             this, SLOT(slotRestartSelectedAccount()) );

    mAccountsReceiving.group->layout()->setMargin( KDialog::marginHint() );
    mAccountsReceiving.group->layout()->setSpacing( KDialog::spacingHint() );

    connect( mAccountsReceiving.mBeepNewMailCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );

    connect( mAccountsReceiving.mVerboseNotificationCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );

    connect( mAccountsReceiving.mOtherNewMailActionsButton, SIGNAL(clicked()),
             this, SLOT(slotEditNotifications()) );
    connect( mAccountsReceiving.customizeAccountOrder,SIGNAL(clicked()),this,SLOT(slotCustomizeAccountOrder()));

    slotAccountSelected( mAccountsReceiving.mAccountList->currentAgentInstance() );

}

AccountsPageReceivingTab::~AccountsPageReceivingTab()
{
    delete mNewMailNotifierInterface;
    mRetrievalHash.clear();
}

void AccountsPageReceivingTab::slotCustomizeAccountOrder()
{
    KMail::AccountConfigOrderDialog dlg(this);
    dlg.exec();
}

void AccountsPageReceivingTab::slotShowMailCheckMenu( const QString &ident, const QPoint & pos )
{
    QMenu *menu = new QMenu( this );

    bool IncludeInManualChecks;
    bool OfflineOnShutdown;
    bool CheckOnStartup;
    if( !mRetrievalHash.contains( ident ) ) {

        const QString resourceGroupPattern( QLatin1String("Resource %1") );
        KConfigGroup group( KMKernel::self()->config(), resourceGroupPattern.arg( ident ) );

        IncludeInManualChecks = group.readEntry( "IncludeInManualChecks", true );

        // "false" is also hardcoded in kmkernel, don't forget to change there.
        OfflineOnShutdown = group.readEntry( "OfflineOnShutdown", false );

        CheckOnStartup = group.readEntry( "CheckOnStartup", false );
        QSharedPointer<RetrievalOptions> opts( new RetrievalOptions( IncludeInManualChecks, OfflineOnShutdown, CheckOnStartup ) );
        mRetrievalHash.insert( ident, opts );
    } else {
        QSharedPointer<RetrievalOptions> opts = mRetrievalHash.value( ident );
        IncludeInManualChecks = opts->IncludeInManualChecks;
        OfflineOnShutdown = opts->OfflineOnShutdown;
        CheckOnStartup = opts->CheckOnStartup;
    }

    if ( !MailCommon::Util::isVirtualCollection( ident ) ) {
        QAction *manualMailCheck = new QAction( i18nc( "Label to a checkbox, so is either checked/unchecked", "Include in Manual Mail Check" ), menu );
        manualMailCheck->setCheckable( true );
        manualMailCheck->setChecked( IncludeInManualChecks );
        manualMailCheck->setData( ident );
        menu->addAction( manualMailCheck );
        connect( manualMailCheck, SIGNAL(toggled(bool)), this, SLOT(slotIncludeInCheckChanged(bool)) );
    }

    if( !MailCommon::Util::isLocalCollection( ident ) ) {
        QAction *switchOffline = new QAction( i18nc( "Label to a checkbox, so is either checked/unchecked", "Switch offline on KMail Shutdown" ), menu );
        switchOffline->setCheckable( true );
        switchOffline->setChecked( OfflineOnShutdown );
        switchOffline->setData( ident );
        menu->addAction( switchOffline );
        connect( switchOffline, SIGNAL(toggled(bool)), this, SLOT(slotOfflineOnShutdownChanged(bool)) );
    }


    QAction *checkOnStartup = new QAction( i18n( "Check mail on startup" ), menu );
    checkOnStartup->setCheckable( true );
    checkOnStartup->setChecked( CheckOnStartup );
    checkOnStartup->setData( ident );
    menu->addAction( checkOnStartup );

    connect( checkOnStartup, SIGNAL(toggled(bool)), this, SLOT(slotCheckOnStatupChanged(bool)) );

    menu->exec(  mAccountsReceiving.mAccountList->view()->mapToGlobal( pos ) );
    delete menu;
}

void AccountsPageReceivingTab::slotCheckOnStatupChanged( bool checked )
{
    QAction* action = qobject_cast< QAction* >( sender() );
    const QString ident = action->data().toString();

    QSharedPointer<RetrievalOptions> opts = mRetrievalHash.value( ident );
    opts->CheckOnStartup = checked;
    slotEmitChanged();
}


void AccountsPageReceivingTab::slotIncludeInCheckChanged( bool checked )
{
    QAction* action = qobject_cast< QAction* >( sender() );
    const QString ident = action->data().toString();

    QSharedPointer<RetrievalOptions> opts = mRetrievalHash.value( ident );
    opts->IncludeInManualChecks = checked;
    slotEmitChanged();
}

void AccountsPageReceivingTab::slotOfflineOnShutdownChanged( bool checked )
{
    QAction* action = qobject_cast< QAction* >( sender() );
    QString ident = action->data().toString();

    QSharedPointer<RetrievalOptions> opts = mRetrievalHash.value( ident );
    opts->OfflineOnShutdown = checked;
    slotEmitChanged();
}

void AccountsPage::ReceivingTab::slotAccountSelected(const Akonadi::AgentInstance& current)
{
    if ( !current.isValid() ) {
        mAccountsReceiving.mModifyAccountButton->setEnabled( false );
        mAccountsReceiving.mRemoveAccountButton->setEnabled( false );
        mAccountsReceiving.mRestartAccountButton->setEnabled( false );
    } else {
        mAccountsReceiving.mModifyAccountButton->setEnabled( !current.type().capabilities().contains( QLatin1String( "NoConfig" ) ) );
        mAccountsReceiving.mRemoveAccountButton->setEnabled( mSpecialMailCollectionIdentifier != current.identifier() );
        // Restarting an agent is not possible if it's in Running status... (see AgentProcessInstance::restartWhenIdle)
        mAccountsReceiving.mRestartAccountButton->setEnabled( ( current.status() != 1 ) );
    }
}

void AccountsPage::ReceivingTab::slotAddAccount()
{
    Akonadi::AgentTypeDialog dlg( this );
    Akonadi::AgentFilterProxyModel* filter = dlg.agentFilterProxyModel();
    filter->addMimeTypeFilter( QLatin1String("message/rfc822") );
    filter->addCapabilityFilter( QLatin1String("Resource") );
    filter->excludeCapabilities( QLatin1String("MailTransport") );
    filter->excludeCapabilities( QLatin1String("Notes") );
    if ( dlg.exec() ) {
        const Akonadi::AgentType agentType = dlg.agentType();

        if ( agentType.isValid() ) {

            Akonadi::AgentInstanceCreateJob *job = new Akonadi::AgentInstanceCreateJob( agentType, this );
            job->configure( this );
            job->start();
        }
    }
}

void AccountsPage::ReceivingTab::slotModifySelectedAccount()
{
    Akonadi::AgentInstance instance = mAccountsReceiving.mAccountList->currentAgentInstance();
    if ( instance.isValid() ) {
        KWindowSystem::allowExternalProcessWindowActivation();
        instance.configure( this );
    }
}



void AccountsPage::ReceivingTab::slotRestartSelectedAccount()
{
    const Akonadi::AgentInstance instance =  mAccountsReceiving.mAccountList->currentAgentInstance();
    if ( instance.isValid() ) {
        instance.restart();
    }
}

void AccountsPage::ReceivingTab::slotRemoveSelectedAccount()
{
    const Akonadi::AgentInstance instance =  mAccountsReceiving.mAccountList->currentAgentInstance();

    int rc = KMessageBox::questionYesNo( this,
                                         i18n("Do you want to remove account '%1'?", instance.name()),
                                         i18n("Remove account?"));
    if ( rc == KMessageBox::No ) {
        return;
    }

    if ( instance.isValid() )
        Akonadi::AgentManager::self()->removeInstance( instance );

    slotAccountSelected( mAccountsReceiving.mAccountList->currentAgentInstance() );

}

void AccountsPage::ReceivingTab::slotEditNotifications()
{
    QDBusInterface interface( QLatin1String("org.freedesktop.Akonadi.Agent.akonadi_newmailnotifier_agent"), QLatin1String("/NewMailNotifierAgent") );
    if (interface.isValid()) {
        interface.call(QLatin1String("showConfigureDialog"), (qlonglong)winId());
    } else {
        KMessageBox::error(this, i18n("New Mail Notifier Agent not registered. Please contact your administrator."));
    }
}

void AccountsPage::ReceivingTab::doLoadFromGlobalSettings()
{
    mAccountsReceiving.mVerboseNotificationCheck->setChecked( mNewMailNotifierInterface->verboseMailNotification() );
}

void AccountsPage::ReceivingTab::doLoadOther()
{
    mAccountsReceiving.mBeepNewMailCheck->setChecked( mNewMailNotifierInterface->beepOnNewMails() );
}

void AccountsPage::ReceivingTab::save()
{
    // Save Mail notification settings
    mNewMailNotifierInterface->setBeepOnNewMails( mAccountsReceiving.mBeepNewMailCheck->isChecked() );
    mNewMailNotifierInterface->setVerboseMailNotification( mAccountsReceiving.mVerboseNotificationCheck->isChecked() );

    const QString resourceGroupPattern( QLatin1String("Resource %1") );
    QHashIterator<QString, QSharedPointer<RetrievalOptions> > it( mRetrievalHash );
    while( it.hasNext() ) {
        it.next();
        KConfigGroup group( KMKernel::self()->config(), resourceGroupPattern.arg( it.key() ) );
        QSharedPointer<RetrievalOptions> opts = it.value();
        group.writeEntry( "IncludeInManualChecks", opts->IncludeInManualChecks);
        group.writeEntry( "OfflineOnShutdown", opts->OfflineOnShutdown);
        group.writeEntry( "CheckOnStartup", opts->CheckOnStartup);
    }
}

