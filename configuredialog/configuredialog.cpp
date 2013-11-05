/*   -*- mode: C++; c-file-style: "gnu" -*-
 *   kmail: KDE mail client
 *   Copyright (C) 2000 Espen Sand, espen@kde.org
 *   Copyright (C) 2001-2003 Marc Mutz, mutz@kde.org
 *   Contains code segments and ideas from earlier kmail dialog code.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include <config-enterprise.h>

// my headers:
#include "configuredialog.h"
#include "configuredialog_p.h"
#include "ui_accountspagereceivingtab.h"
#include "identity/identitypage.h"

#include "settings/globalsettings.h"
#include "templatesconfiguration_kfg.h"
#include "configuredialoglistview.h"
#include "configagentdelegate.h"

// other KMail headers:
#include "kmkernel.h"
#include "kernel/mailkernel.h"
#include "pimcommon/widgets/simplestringlisteditor.h"
#include "colorlistbox.h"
#include <kpimidentities/identitymanager.h>
#include "folderrequester.h"
#include "kmmainwidget.h"
#include "accountconfigorderdialog.h"
#include "util.h"

#include "foldertreewidget.h"

#include "dialog/kmknotify.h"

#include "newmailnotifierinterface.h"

#include "addressline/recentaddresses.h"
using KPIM::RecentAddresses;
#include "addressline/completionordereditor.h"
#include "messagelist/utils/aggregationcombobox.h"
#include "messagelist/utils/aggregationconfigbutton.h"
#include "messagelist/utils/themecombobox.h"
#include "messagelist/utils/themeconfigbutton.h"

#include "messagecomposer/autocorrection/composerautocorrectionwidget.h"
#include "messagecomposer/imagescaling/imagescalingwidget.h"

#include "messageviewer/utils/autoqpointer.h"
#include "messageviewer/viewer/nodehelper.h"
#include "messageviewer/widgets/configurewidget.h"
#include "messageviewer/settings/globalsettings.h"
#include "messageviewer/widgets/invitationsettings.h"
#include "messageviewer/header/customheadersettingwidget.h"
#include "messageviewer/widgets/printingsettings.h"
#include "messageviewer/adblock/adblocksettingwidget.h"
#include "messagelist/core/settings.h"
#include "messagelist/messagelistutil.h"
#include "messagecore/settings/globalsettings.h"

#include "templateparser/templatesconfiguration_kfg.h"
#include "templateparser/templatesconfiguration.h"
#include "templateparser/customtemplates.h"
#include "templateparser/globalsettings_base.h"
#include "mailcommon/util/mailutil.h"
#include "mailcommon/tag/tagwidget.h"
#include "mailcommon/tag/tag.h"
#include "mailcommon/mailcommonsettings_base.h"

#include "messagecomposer/settings/messagecomposersettings.h"
#include "configureagentswidget.h"
#include <soprano/nao.h>

// other kdenetwork headers:
#include <kpimidentities/identity.h>
#include <kmime/kmime_dateformatter.h>
using KMime::DateFormatter;
#include "kleo/cryptoconfig.h"
#include "kleo/cryptobackendfactory.h"
#include "libkleo/ui/keyrequester.h"
#include "libkleo/ui/keyselectiondialog.h"
#include "libkleo/ui/cryptoconfigdialog.h"

#include <mailtransport/transportmanagementwidget.h>
using MailTransport::TransportManagementWidget;
// other KDE headers:
#include <libkdepim/ldap/ldapclientsearch.h>
#include <klineedit.h>
#include <klocale.h>
#include <kcharsets.h>
#include <kascii.h>
#include <knuminput.h>
#include <kfontchooser.h>
#include <kmessagebox.h>
#include <kurlrequester.h>
#include <kseparator.h>
#include <kiconloader.h>
#include <kwindowsystem.h>
#include <kcmultidialog.h>
#include <knotifyconfigwidget.h>
#include <kconfiggroup.h>
#include <kbuttongroup.h>
#include <kcolorcombo.h>
#include <kfontrequester.h>
#include <kicondialog.h>
#include <kkeysequencewidget.h>
#include <KColorScheme>
#include <KComboBox>
#include <Nepomuk2/Tag>
#include <KCModuleProxy>

// Qt headers:
#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QLabel>
#include <QRadioButton>
#include <QGroupBox>
#include <QListWidget>
#include <QVBoxLayout>
#include <QWhatsThis>
#include <QDBusConnection>
#include <QHostInfo>
#include <QTextCodec>
#include <QMenu>

// other headers:
#include <assert.h>
#include <stdlib.h>


#include <akonadi/agentfilterproxymodel.h>
#include <akonadi/agentinstancemodel.h>
#include <akonadi/agenttype.h>
#include <akonadi/agentmanager.h>
#include <akonadi/agenttypedialog.h>
#include <akonadi/agentinstancecreatejob.h>

#include <nepomuk2/resourcemanager.h>

#include <libkdepim/widgets/nepomukwarning.h>
using namespace MailCommon;
using namespace KMail;
namespace {

static const char * lockedDownWarning =
        I18N_NOOP("<qt><p>This setting has been fixed by your administrator.</p>"
                  "<p>If you think this is an error, please contact him.</p></qt>");

void checkLockDown( QWidget * w, const KConfigSkeletonItem *item ) {
    if ( item->isImmutable() ) {
        w->setEnabled( false );
        w->setToolTip( i18n( lockedDownWarning ) );
    } else {
        w->setToolTip( QString() );
    }
}

void populateButtonGroup( QGroupBox * box, QButtonGroup * group, int orientation,
                          const KCoreConfigSkeleton::ItemEnum *e ) {
    box->setTitle( e->label() );
    if (orientation == Qt::Horizontal) {
        box->setLayout( new QHBoxLayout() );
    } else {
        box->setLayout( new QVBoxLayout() );
    }
    box->layout()->setSpacing( KDialog::spacingHint() );
    const int numberChoices(e->choices().size());
    for (int i = 0; i < numberChoices; ++i) {
        QRadioButton *button = new QRadioButton( e->choices().at(i).label, box );
        group->addButton( button, i );
        box->layout()->addWidget( button );
    }
}

void populateCheckBox( QCheckBox * b, const KCoreConfigSkeleton::ItemBool *e ) {
    b->setText( e->label() );
}

void loadWidget( QCheckBox * b, const KCoreConfigSkeleton::ItemBool *e ) {
    checkLockDown( b, e );
    b->setChecked( e->value() );
}

void loadWidget( QGroupBox * box, QButtonGroup * group,
                 const KCoreConfigSkeleton::ItemEnum *e ) {
    Q_ASSERT( group->buttons().size() == e->choices().size() );
    checkLockDown( box, e );
    group->buttons()[e->value()]->setChecked( true );
}

void saveCheckBox( QCheckBox * b, KCoreConfigSkeleton::ItemBool *e ) {
    e->setValue( b->isChecked() );
}

void saveButtonGroup( QButtonGroup * group, KCoreConfigSkeleton::ItemEnum *e ) {
    Q_ASSERT( group->buttons().size() == e->choices().size() );
    if ( group->checkedId() != -1 ) {
        e->setValue( group->checkedId() );
    }
}
}

ConfigureDialog::ConfigureDialog( QWidget *parent, bool modal )
    : KCMultiDialog( parent )
{
    setFaceType( List );
    setButtons( Help | Default | Cancel | Apply | Ok | Reset );
    setModal( modal );
    KWindowSystem::setIcons( winId(), qApp->windowIcon().pixmap( IconSize( KIconLoader::Desktop ), IconSize( KIconLoader::Desktop ) ), qApp->windowIcon().pixmap(IconSize( KIconLoader::Small ), IconSize( KIconLoader::Small ) ) );
    addModule( QLatin1String("kmail_config_identity") );
    addModule( QLatin1String("kmail_config_accounts") );
    addModule( QLatin1String("kmail_config_appearance") );
    addModule( QLatin1String("kmail_config_composer") );
    addModule( QLatin1String("kmail_config_security") );
    addModule( QLatin1String("kmail_config_misc") );

    connect( this, SIGNAL(okClicked()), SLOT(slotOk()) );
    connect( this, SIGNAL(applyClicked()), SLOT(slotApply()) );

    // We store the size of the dialog on hide, because otherwise
    // the KCMultiDialog starts with the size of the first kcm, not
    // the largest one. This way at least after the first showing of
    // the largest kcm the size is kept.
    const int width = GlobalSettings::self()->configureDialogWidth();
    const int height = GlobalSettings::self()->configureDialogHeight();
    if ( width != 0 && height != 0 ) {
        resize( width, height );
    }

}

void ConfigureDialog::hideEvent( QHideEvent *ev )
{
    GlobalSettings::self()->setConfigureDialogWidth( width() );
    GlobalSettings::self()->setConfigureDialogHeight( height() );
    KDialog::hideEvent( ev );
}

ConfigureDialog::~ConfigureDialog()
{
}

void ConfigureDialog::slotApply()
{
    slotApplyClicked();
    KMKernel::self()->slotRequestConfigSync();
    emit configChanged();
}

void ConfigureDialog::slotOk()
{
    slotOkClicked();
    KMKernel::self()->slotRequestConfigSync();
    emit configChanged();
}

// *************************************************************
// *                                                           *
// *                       AccountsPage                         *
// *                                                           *
// *************************************************************
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
    AccountConfigOrderDialog dlg(this);
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

// *************************************************************
// *                                                           *
// *                     AppearancePage                        *
// *                                                           *
// *************************************************************
QString AppearancePage::helpAnchor() const
{
    return QString::fromLatin1("configure-appearance");
}

AppearancePage::AppearancePage( const KComponentData &instance, QWidget *parent )
    : ConfigModuleWithTabs( instance, parent )
{
    //
    // "Fonts" tab:
    //
    mFontsTab = new FontsTab();
    addTab( mFontsTab, i18n("Fonts") );

    //
    // "Colors" tab:
    //
    mColorsTab = new ColorsTab();
    addTab( mColorsTab, i18n("Colors") );

    //
    // "Layout" tab:
    //
    mLayoutTab = new LayoutTab();
    addTab( mLayoutTab, i18n("Layout") );

    //
    // "Headers" tab:
    //
    mHeadersTab = new HeadersTab();
    addTab( mHeadersTab, i18n("Message List") );

    //
    // "Reader window" tab:
    //
    mReaderTab = new ReaderTab();
    addTab( mReaderTab, i18n("Message Window") );
    addConfig( MessageViewer::GlobalSettings::self(), mReaderTab );

    //
    // "System Tray" tab:
    //
    mSystemTrayTab = new SystemTrayTab();
    addTab( mSystemTrayTab, i18n("System Tray") );

    //
    // "Message Tag" tab:
    //
    mMessageTagTab = new MessageTagTab();
    addTab( mMessageTagTab, i18n("Message Tags") );
}


QString AppearancePage::FontsTab::helpAnchor() const
{
    return QString::fromLatin1("configure-appearance-fonts");
}

static const struct {
    const char * configName;
    const char * displayName;
    bool   enableFamilyAndSize;
    bool   onlyFixed;
} fontNames[] = {
    { "body-font", I18N_NOOP("Message Body"), true, false },
    { "MessageListFont", I18N_NOOP("Message List"), true, false },
    { "UnreadMessageFont", I18N_NOOP("Message List - Unread Messages"), true, false },
    { "ImportantMessageFont", I18N_NOOP("Message List - Important Messages"), true, false },
    { "TodoMessageFont", I18N_NOOP("Message List - Action Item Messages"), true, false },
    { "folder-font", I18N_NOOP("Folder List"), true, false },
    { "quote1-font", I18N_NOOP("Quoted Text - First Level"), false, false },
    { "quote2-font", I18N_NOOP("Quoted Text - Second Level"), false, false },
    { "quote3-font", I18N_NOOP("Quoted Text - Third Level"), false, false },
    { "fixed-font", I18N_NOOP("Fixed Width Font"), true, true },
    { "composer-font", I18N_NOOP("Composer"), true, false },
    { "print-font",  I18N_NOOP("Printing Output"), true, false },
};
static const int numFontNames = sizeof fontNames / sizeof *fontNames;

AppearancePageFontsTab::AppearancePageFontsTab( QWidget * parent )
    : ConfigModuleTab( parent ), mActiveFontIndex( -1 )
{
    assert( numFontNames == sizeof mFont / sizeof *mFont );
    // tmp. vars:
    QVBoxLayout *vlay;
    QHBoxLayout *hlay;
    QLabel      *label;

    // "Use custom fonts" checkbox, followed by <hr>
    vlay = new QVBoxLayout( this );
    vlay->setSpacing( KDialog::spacingHint() );
    vlay->setMargin( KDialog::marginHint() );
    mCustomFontCheck = new QCheckBox( i18n("&Use custom fonts"), this );
    vlay->addWidget( mCustomFontCheck );
    vlay->addWidget( new KSeparator( Qt::Horizontal, this ) );
    connect ( mCustomFontCheck, SIGNAL(stateChanged(int)),
              this, SLOT(slotEmitChanged()) );

    // "font location" combo box and label:
    hlay = new QHBoxLayout(); // inherites spacing
    vlay->addLayout( hlay );
    mFontLocationCombo = new KComboBox( this );
    mFontLocationCombo->setEditable( false );
    mFontLocationCombo->setEnabled( false ); // !mCustomFontCheck->isChecked()

    QStringList fontDescriptions;
    for ( int i = 0 ; i < numFontNames ; ++i )
        fontDescriptions << i18n( fontNames[i].displayName );
    mFontLocationCombo->addItems( fontDescriptions );

    label = new QLabel( i18n("Apply &to:"), this );
    label->setBuddy( mFontLocationCombo );
    label->setEnabled( false ); // since !mCustomFontCheck->isChecked()
    hlay->addWidget( label );

    hlay->addWidget( mFontLocationCombo );
    hlay->addStretch( 10 );
    vlay->addSpacing( KDialog::spacingHint() );
    mFontChooser = new KFontChooser( this, KFontChooser::DisplayFrame,
                                     QStringList(), 4 );
    mFontChooser->setEnabled( false ); // since !mCustomFontCheck->isChecked()
    vlay->addWidget( mFontChooser );
    connect ( mFontChooser, SIGNAL(fontSelected(QFont)),
              this, SLOT(slotEmitChanged()) );


    // {en,dis}able widgets depending on the state of mCustomFontCheck:
    connect( mCustomFontCheck, SIGNAL(toggled(bool)),
             label, SLOT(setEnabled(bool)) );
    connect( mCustomFontCheck, SIGNAL(toggled(bool)),
             mFontLocationCombo, SLOT(setEnabled(bool)) );
    connect( mCustomFontCheck, SIGNAL(toggled(bool)),
             mFontChooser, SLOT(setEnabled(bool)) );
    // load the right font settings into mFontChooser:
    connect( mFontLocationCombo, SIGNAL(activated(int)),
             this, SLOT(slotFontSelectorChanged(int)) );
}


void AppearancePage::FontsTab::slotFontSelectorChanged( int index )
{
    kDebug() << "slotFontSelectorChanged() called";
    if( index < 0 || index >= mFontLocationCombo->count() )
        return; // Should never happen, but it is better to check.

    // Save current fontselector setting before we install the new:
    if( mActiveFontIndex == 0 ) {
        mFont[0] = mFontChooser->font();
        // hardcode the family and size of "message body" dependant fonts:
        for ( int i = 0 ; i < numFontNames ; ++i )
            if ( !fontNames[i].enableFamilyAndSize ) {
                // ### shall we copy the font and set the save and re-set
                // {regular,italic,bold,bold italic} property or should we
                // copy only family and pointSize?
                mFont[i].setFamily( mFont[0].family() );
                mFont[i].setPointSize/*Float?*/( mFont[0].pointSize/*Float?*/() );
            }
    } else if ( mActiveFontIndex > 0 )
        mFont[ mActiveFontIndex ] = mFontChooser->font();
    mActiveFontIndex = index;

    // Disonnect so the "Apply" button is not activated by the change
    disconnect ( mFontChooser, SIGNAL(fontSelected(QFont)),
                 this, SLOT(slotEmitChanged()) );

    // Display the new setting:
    mFontChooser->setFont( mFont[index], fontNames[index].onlyFixed );

    connect ( mFontChooser, SIGNAL(fontSelected(QFont)),
              this, SLOT(slotEmitChanged()) );

    // Disable Family and Size list if we have selected a quote font:
    mFontChooser->enableColumn( KFontChooser::FamilyList|KFontChooser::SizeList,
                                fontNames[ index ].enableFamilyAndSize );
}

void AppearancePage::FontsTab::doLoadOther()
{
    KConfigGroup fonts( KMKernel::self()->config(), "Fonts" );
    KConfigGroup messagelistFont( KMKernel::self()->config(), "MessageListView::Fonts" );

    mFont[0] = KGlobalSettings::generalFont();
    QFont fixedFont = KGlobalSettings::fixedFont();

    for ( int i = 0 ; i < numFontNames ; ++i ) {
        const QString configName = QLatin1String(fontNames[i].configName);
        if ( configName == QLatin1String( "MessageListFont" ) ||
             configName == QLatin1String( "UnreadMessageFont" ) ||
             configName == QLatin1String( "ImportantMessageFont" ) ||
             configName == QLatin1String( "TodoMessageFont" ) ) {
            mFont[i] = messagelistFont.readEntry( configName,
                                                  (fontNames[i].onlyFixed) ? fixedFont : mFont[0] );
        } else {
            mFont[i] = fonts.readEntry( configName,
                                        (fontNames[i].onlyFixed) ? fixedFont : mFont[0] );
        }
    }
    mCustomFontCheck->setChecked( !MessageCore::GlobalSettings::self()->useDefaultFonts() );
    mFontLocationCombo->setCurrentIndex( 0 );
    slotFontSelectorChanged( 0 );
}

void AppearancePage::FontsTab::save()
{
    KConfigGroup fonts( KMKernel::self()->config(), "Fonts" );
    KConfigGroup messagelistFont( KMKernel::self()->config(), "MessageListView::Fonts" );

    // read the current font (might have been modified)
    if ( mActiveFontIndex >= 0 )
        mFont[ mActiveFontIndex ] = mFontChooser->font();

    const bool customFonts = mCustomFontCheck->isChecked();
    MessageCore::GlobalSettings::self()->setUseDefaultFonts( !customFonts );

    for ( int i = 0 ; i < numFontNames ; ++i ) {
        const QString configName = QLatin1String(fontNames[i].configName);
        if ( configName == QLatin1String( "MessageListFont" ) ||
             configName == QLatin1String( "UnreadMessageFont" ) ||
             configName == QLatin1String( "ImportantMessageFont" ) ||
             configName == QLatin1String( "TodoMessageFont" ) ) {
            if ( customFonts || messagelistFont.hasKey( configName ) ) {
                // Don't write font info when we use default fonts, but write
                // if it's already there:
                messagelistFont.writeEntry( configName, mFont[i] );
            }
        } else {
            if ( customFonts || fonts.hasKey( configName ) )
                // Don't write font info when we use default fonts, but write
                // if it's already there:
                fonts.writeEntry( configName, mFont[i] );
        }
    }
}

void AppearancePage::FontsTab::doResetToDefaultsOther()
{
    mCustomFontCheck->setChecked( false );
}


QString AppearancePage::ColorsTab::helpAnchor() const
{
    return QString::fromLatin1("configure-appearance-colors");
}

static const struct {
    const char * configName;
    const char * displayName;
} colorNames[] = { // adjust doLoadOther if you change this:
                   { "QuotedText1", I18N_NOOP("Quoted Text - First Level") },
                   { "QuotedText2", I18N_NOOP("Quoted Text - Second Level") },
                   { "QuotedText3", I18N_NOOP("Quoted Text - Third Level") },
                   { "LinkColor", I18N_NOOP("Link") },
                   { "FollowedColor", I18N_NOOP("Followed Link") },
                   { "MisspelledColor", I18N_NOOP("Misspelled Words") },
                   { "UnreadMessageColor", I18N_NOOP("Unread Message") },
                   { "ImportantMessageColor", I18N_NOOP("Important Message") },
                   { "TodoMessageColor", I18N_NOOP("Action Item Message") },
                   { "PGPMessageEncr", I18N_NOOP("OpenPGP Message - Encrypted") },
                   { "PGPMessageOkKeyOk", I18N_NOOP("OpenPGP Message - Valid Signature with Trusted Key") },
                   { "PGPMessageOkKeyBad", I18N_NOOP("OpenPGP Message - Valid Signature with Untrusted Key") },
                   { "PGPMessageWarn", I18N_NOOP("OpenPGP Message - Unchecked Signature") },
                   { "PGPMessageErr", I18N_NOOP("OpenPGP Message - Bad Signature") },
                   { "HTMLWarningColor", I18N_NOOP("Border Around Warning Prepending HTML Messages") },
                   { "CloseToQuotaColor", I18N_NOOP("Folder Name and Size When Close to Quota") },
                   { "ColorbarBackgroundPlain", I18N_NOOP("HTML Status Bar Background - No HTML Message") },
                   { "ColorbarForegroundPlain", I18N_NOOP("HTML Status Bar Foreground - No HTML Message") },
                   { "ColorbarBackgroundHTML",  I18N_NOOP("HTML Status Bar Background - HTML Message") },
                   { "ColorbarForegroundHTML",  I18N_NOOP("HTML Status Bar Foreground - HTML Message") },
                   { "BrokenAccountColor",  I18N_NOOP("Broken Account - Folder Text Color") },
                   { "BackgroundColor",  I18N_NOOP("Background Color") },
                 };
static const int numColorNames = sizeof colorNames / sizeof *colorNames;

AppearancePageColorsTab::AppearancePageColorsTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    // tmp. vars:
    QVBoxLayout *vlay;

    // "use custom colors" check box
    vlay = new QVBoxLayout( this );
    vlay->setSpacing( KDialog::spacingHint() );
    vlay->setMargin( KDialog::marginHint() );
    mCustomColorCheck = new QCheckBox( i18n("&Use custom colors"), this );
    vlay->addWidget( mCustomColorCheck );
    connect( mCustomColorCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );

    // color list box:
    mColorList = new ColorListBox( this );
    mColorList->setEnabled( false ); // since !mCustomColorCheck->isChecked()
    for ( int i = 0 ; i < numColorNames ; ++i )
        mColorList->addColor( i18n(colorNames[i].displayName) );
    vlay->addWidget( mColorList, 1 );

    // "recycle colors" check box:
    mRecycleColorCheck =
            new QCheckBox( i18n("Recycle colors on deep &quoting"), this );
    mRecycleColorCheck->setEnabled( false );
    vlay->addWidget( mRecycleColorCheck );
    connect( mRecycleColorCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );

    // close to quota threshold
    QHBoxLayout *hbox = new QHBoxLayout();
    vlay->addLayout( hbox );
    QLabel *l = new QLabel( i18n("Close to quota threshold:"), this );
    hbox->addWidget( l );
    mCloseToQuotaThreshold = new QSpinBox( this );
    mCloseToQuotaThreshold->setRange( 0, 100 );
    mCloseToQuotaThreshold->setSingleStep( 1 );
    connect( mCloseToQuotaThreshold, SIGNAL(valueChanged(int)),
             this, SLOT(slotEmitChanged()) );
    mCloseToQuotaThreshold->setSuffix( i18n("%"));

    hbox->addWidget( mCloseToQuotaThreshold );
    hbox->addWidget( new QWidget(this), 2 );

    // {en,dir}able widgets depending on the state of mCustomColorCheck:
    connect( mCustomColorCheck, SIGNAL(toggled(bool)),
             mColorList, SLOT(setEnabled(bool)) );
    connect( mCustomColorCheck, SIGNAL(toggled(bool)),
             mRecycleColorCheck, SLOT(setEnabled(bool)) );
    connect( mCustomColorCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mColorList, SIGNAL(changed()),
             this, SLOT(slotEmitChanged()) );
}

void AppearancePage::ColorsTab::doLoadOther()
{
    mCustomColorCheck->setChecked( !MessageCore::GlobalSettings::self()->useDefaultColors() );
    mRecycleColorCheck->setChecked( MessageViewer::GlobalSettings::self()->recycleQuoteColors() );
    mCloseToQuotaThreshold->setValue( GlobalSettings::self()->closeToQuotaThreshold() );
    loadColor( true );
}

void AppearancePage::ColorsTab::loadColor( bool loadFromConfig )
{
    KColorScheme scheme( QPalette::Active, KColorScheme::View );

    KConfigGroup reader( KMKernel::self()->config(), "Reader" );

    KConfigGroup messageListView( KMKernel::self()->config(), "MessageListView::Colors" );

    KConfigGroup collectionFolderView( KMKernel::self()->config(), "CollectionFolderView" );

    static const QColor defaultColor[ numColorNames ] = {
        KMail::Util::quoteL1Color(),
        KMail::Util::quoteL2Color(),
        KMail::Util::quoteL3Color(),
        scheme.foreground( KColorScheme::LinkText ).color(), // link
        scheme.foreground( KColorScheme::VisitedText ).color(),// visited link
        scheme.foreground( KColorScheme::NegativeText ).color(), // misspelled words
        MessageList::Util::unreadDefaultMessageColor(), // unread mgs
        MessageList::Util::importantDefaultMessageColor(), // important msg
        MessageList::Util::todoDefaultMessageColor(), // action item mgs
        QColor( 0x00, 0x80, 0xFF ), // pgp encrypted
        scheme.background( KColorScheme::PositiveBackground ).color(), // pgp ok, trusted key
        QColor( 0xFF, 0xFF, 0x40 ), // pgp ok, untrusted key
        QColor( 0xFF, 0xFF, 0x40 ), // pgp unchk
        Qt::red, // pgp bad
        QColor( 0xFF, 0x40, 0x40 ), // warning text color
        MailCommon::Util::defaultQuotaColor(), // close to quota
        Qt::lightGray, // colorbar plain bg
        Qt::black,     // colorbar plain fg
        Qt::black,     // colorbar html  bg
        Qt::white,     // colorbar html  fg
        scheme.foreground(KColorScheme::NegativeText).color(),  //Broken Account Color
        scheme.background().color() // reader background color
    };

    for ( int i = 0 ; i < numColorNames ; ++i ) {
        if ( loadFromConfig ) {
            const QString configName = QLatin1String(colorNames[i].configName);
            if ( configName == QLatin1String( "UnreadMessageColor" ) ||
                 configName == QLatin1String( "ImportantMessageColor" ) ||
                 configName == QLatin1String( "TodoMessageColor" ) ) {
                mColorList->setColorSilently( i, messageListView.readEntry( configName, defaultColor[i] ) );
            } else if( configName == QLatin1String("BrokenAccountColor")) {
                mColorList->setColorSilently( i, collectionFolderView.readEntry(configName,defaultColor[i]));
            } else {
                mColorList->setColorSilently( i, reader.readEntry( configName, defaultColor[i] ) );
            }
        } else {
            mColorList->setColorSilently( i, defaultColor[i] );
        }
    }
}

void AppearancePage::ColorsTab::doResetToDefaultsOther()
{
    mCustomColorCheck->setChecked( false );
    mRecycleColorCheck->setChecked( false );
    mCloseToQuotaThreshold->setValue( 80 );
    loadColor( false );
}

void AppearancePage::ColorsTab::save()
{
    KConfigGroup reader( KMKernel::self()->config(), "Reader" );
    KConfigGroup messageListView( KMKernel::self()->config(), "MessageListView::Colors" );
    KConfigGroup collectionFolderView( KMKernel::self()->config(), "CollectionFolderView" );
    bool customColors = mCustomColorCheck->isChecked();
    MessageCore::GlobalSettings::self()->setUseDefaultColors( !customColors );

    KColorScheme scheme( QPalette::Active, KColorScheme::View );

    for ( int i = 0 ; i < numColorNames ; ++i ) {
        // Don't write color info when we use default colors, but write
        // if it's already there:
        const QString configName = QLatin1String(colorNames[i].configName);
        if ( configName == QLatin1String( "UnreadMessageColor" ) ||
             configName == QLatin1String( "ImportantMessageColor" ) ||
             configName == QLatin1String( "TodoMessageColor" ) ) {
            if ( customColors || messageListView.hasKey( configName ) )
                messageListView.writeEntry( configName, mColorList->color(i) );

        } else if( configName == QLatin1String("BrokenAccountColor")) {
            if ( customColors || collectionFolderView.hasKey( configName ) )
                collectionFolderView.writeEntry(configName,mColorList->color(i));
        } else if (configName == QLatin1String("BackgroundColor")) {
            if (customColors && (mColorList->color(i) != scheme.background().color() ))
                reader.writeEntry(configName, mColorList->color(i));
        } else {
            if ( customColors || reader.hasKey( configName ) )
                reader.writeEntry( configName, mColorList->color(i) );
        }
    }
    MessageViewer::GlobalSettings::self()->setRecycleQuoteColors( mRecycleColorCheck->isChecked() );
    GlobalSettings::self()->setCloseToQuotaThreshold( mCloseToQuotaThreshold->value() );
}

QString AppearancePage::LayoutTab::helpAnchor() const
{
    return QString::fromLatin1("configure-appearance-layout");
}

AppearancePageLayoutTab::AppearancePageLayoutTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    // tmp. vars:
    QVBoxLayout * vlay;

    vlay = new QVBoxLayout( this );
    vlay->setSpacing( KDialog::spacingHint() );
    vlay->setMargin( KDialog::marginHint() );

    // "folder list" radio buttons:
    populateButtonGroup( mFolderListGroupBox = new QGroupBox( this ),
                         mFolderListGroup = new QButtonGroup( this ),
                         Qt::Horizontal, GlobalSettings::self()->folderListItem() );
    vlay->addWidget( mFolderListGroupBox );
    connect( mFolderListGroup, SIGNAL (buttonClicked(int)),
             this, SLOT(slotEmitChanged()) );

    QHBoxLayout* folderCBHLayout = new QHBoxLayout();
    mFolderQuickSearchCB = new QCheckBox( i18n("Show folder quick search field"), this );
    connect( mFolderQuickSearchCB, SIGNAL(toggled(bool)), SLOT(slotEmitChanged()) );
    folderCBHLayout->addWidget( mFolderQuickSearchCB );
    vlay->addLayout( folderCBHLayout );
    vlay->addSpacing( KDialog::spacingHint() );   // space before next box

    // "favorite folders view mode" radio buttons:
    mFavoriteFoldersViewGroupBox = new QGroupBox( this );
    mFavoriteFoldersViewGroupBox->setTitle( i18n( "Show Favorite Folders View" ) );
    mFavoriteFoldersViewGroupBox->setLayout( new QHBoxLayout() );
    mFavoriteFoldersViewGroupBox->layout()->setSpacing( KDialog::spacingHint() );
    mFavoriteFoldersViewGroup = new QButtonGroup( this );
    connect( mFavoriteFoldersViewGroup, SIGNAL(buttonClicked(int)),
             this, SLOT(slotEmitChanged()) );

    QRadioButton* favoriteFoldersViewHiddenRadio = new QRadioButton( i18n( "Never" ), mFavoriteFoldersViewGroupBox );
    mFavoriteFoldersViewGroup->addButton( favoriteFoldersViewHiddenRadio, static_cast<int>( MailCommon::MailCommonSettings::EnumFavoriteCollectionViewMode::HiddenMode ) );
    mFavoriteFoldersViewGroupBox->layout()->addWidget( favoriteFoldersViewHiddenRadio );

    QRadioButton* favoriteFoldersViewIconsRadio = new QRadioButton( i18n( "As icons" ), mFavoriteFoldersViewGroupBox );
    mFavoriteFoldersViewGroup->addButton( favoriteFoldersViewIconsRadio, static_cast<int>( MailCommon::MailCommonSettings::EnumFavoriteCollectionViewMode::IconMode ) );
    mFavoriteFoldersViewGroupBox->layout()->addWidget( favoriteFoldersViewIconsRadio );

    QRadioButton* favoriteFoldersViewListRadio = new QRadioButton( i18n( "As list" ), mFavoriteFoldersViewGroupBox );
    mFavoriteFoldersViewGroup->addButton( favoriteFoldersViewListRadio,  static_cast<int>( MailCommon::MailCommonSettings::EnumFavoriteCollectionViewMode::ListMode ) );
    mFavoriteFoldersViewGroupBox->layout()->addWidget( favoriteFoldersViewListRadio );

    vlay->addWidget( mFavoriteFoldersViewGroupBox );


    // "folder tooltips" radio buttons:
    mFolderToolTipsGroupBox = new QGroupBox( this );
    mFolderToolTipsGroupBox->setTitle( i18n( "Folder Tooltips" ) );
    mFolderToolTipsGroupBox->setLayout( new QHBoxLayout() );
    mFolderToolTipsGroupBox->layout()->setSpacing( KDialog::spacingHint() );
    mFolderToolTipsGroup = new QButtonGroup( this );
    connect( mFolderToolTipsGroup, SIGNAL(buttonClicked(int)),
             this, SLOT(slotEmitChanged()) );

    QRadioButton* folderToolTipsAlwaysRadio = new QRadioButton( i18n( "Always" ), mFolderToolTipsGroupBox );
    mFolderToolTipsGroup->addButton( folderToolTipsAlwaysRadio, static_cast< int >( FolderTreeWidget::DisplayAlways ) );
    mFolderToolTipsGroupBox->layout()->addWidget( folderToolTipsAlwaysRadio );

    QRadioButton* folderToolTipsNeverRadio = new QRadioButton( i18n( "Never" ), mFolderToolTipsGroupBox );
    mFolderToolTipsGroup->addButton( folderToolTipsNeverRadio, static_cast< int >( FolderTreeWidget::DisplayNever ) );
    mFolderToolTipsGroupBox->layout()->addWidget( folderToolTipsNeverRadio );

    vlay->addWidget( mFolderToolTipsGroupBox );

    // "show reader window" radio buttons:
    populateButtonGroup( mReaderWindowModeGroupBox = new QGroupBox( this ),
                         mReaderWindowModeGroup = new QButtonGroup( this ),
                         Qt::Vertical, GlobalSettings::self()->readerWindowModeItem() );
    vlay->addWidget( mReaderWindowModeGroupBox );
    connect( mReaderWindowModeGroup, SIGNAL (buttonClicked(int)),
             this, SLOT(slotEmitChanged()) );

    vlay->addStretch( 10 ); // spacer
}

void AppearancePage::LayoutTab::doLoadOther()
{
    loadWidget( mFolderListGroupBox, mFolderListGroup, GlobalSettings::self()->folderListItem() );
    loadWidget( mReaderWindowModeGroupBox, mReaderWindowModeGroup, GlobalSettings::self()->readerWindowModeItem() );
    loadWidget( mFavoriteFoldersViewGroupBox, mFavoriteFoldersViewGroup, MailCommon::MailCommonSettings::self()->favoriteCollectionViewModeItem() );
    mFolderQuickSearchCB->setChecked( GlobalSettings::self()->enableFolderQuickSearch() );
    const int checkedFolderToolTipsPolicy = GlobalSettings::self()->toolTipDisplayPolicy();
    if ( checkedFolderToolTipsPolicy < mFolderToolTipsGroup->buttons().size() && checkedFolderToolTipsPolicy >= 0 )
        mFolderToolTipsGroup->buttons()[ checkedFolderToolTipsPolicy ]->setChecked( true );
}

void AppearancePage::LayoutTab::save()
{
    saveButtonGroup( mFolderListGroup, GlobalSettings::self()->folderListItem() );
    saveButtonGroup( mReaderWindowModeGroup, GlobalSettings::self()->readerWindowModeItem() );
    saveButtonGroup( mFavoriteFoldersViewGroup, MailCommon::MailCommonSettings::self()->favoriteCollectionViewModeItem() );
    GlobalSettings::self()->setEnableFolderQuickSearch( mFolderQuickSearchCB->isChecked() );
    GlobalSettings::self()->setToolTipDisplayPolicy( mFolderToolTipsGroup->checkedId() );
}

//
// Appearance Message List
//

QString AppearancePage::HeadersTab::helpAnchor() const
{
    return QString::fromLatin1("configure-appearance-headers");
}

static const struct {
    const char * displayName;
    DateFormatter::FormatType dateDisplay;
} dateDisplayConfig[] = {
    { I18N_NOOP("Sta&ndard format (%1)"), KMime::DateFormatter::CTime },
    { I18N_NOOP("Locali&zed format (%1)"), KMime::DateFormatter::Localized },
    { I18N_NOOP("Smart for&mat (%1)"), KMime::DateFormatter::Fancy },
    { I18N_NOOP("C&ustom format:"), KMime::DateFormatter::Custom }
};
static const int numDateDisplayConfig =
        sizeof dateDisplayConfig / sizeof *dateDisplayConfig;

AppearancePageHeadersTab::AppearancePageHeadersTab( QWidget * parent )
    : ConfigModuleTab( parent ),
      mCustomDateFormatEdit( 0 )
{
    // tmp. vars:
    QGroupBox * group;

    QVBoxLayout * vlay = new QVBoxLayout( this );
    vlay->setSpacing( KDialog::spacingHint() );
    vlay->setMargin( KDialog::marginHint() );

    // "General Options" group:
    group = new QGroupBox( i18nc( "General options for the message list.", "General" ), this );
    //  group->layout()->setSpacing( KDialog::spacingHint() );
    QVBoxLayout *gvlay = new QVBoxLayout( group );
    gvlay->setSpacing( KDialog::spacingHint() );

    mDisplayMessageToolTips = new QCheckBox(
                MessageList::Core::Settings::self()->messageToolTipEnabledItem()->label(), group );
    gvlay->addWidget( mDisplayMessageToolTips );

    connect( mDisplayMessageToolTips, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );

    mHideTabBarWithSingleTab = new QCheckBox(
                MessageList::Core::Settings::self()->autoHideTabBarWithSingleTabItem()->label(), group );
    gvlay->addWidget( mHideTabBarWithSingleTab );

    connect( mHideTabBarWithSingleTab, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );

    mTabsHaveCloseButton = new QCheckBox(
                MessageList::Core::Settings::self()->tabsHaveCloseButtonItem()->label(), group );
    gvlay->addWidget(  mTabsHaveCloseButton );

    connect( mTabsHaveCloseButton, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );

    // "Aggregation"
    using MessageList::Utils::AggregationComboBox;
    mAggregationComboBox = new AggregationComboBox( group );

    QLabel* aggregationLabel = new QLabel( i18n( "Default aggregation:" ), group );
    aggregationLabel->setBuddy( mAggregationComboBox );

    using MessageList::Utils::AggregationConfigButton;
    AggregationConfigButton * aggregationConfigButton = new AggregationConfigButton( group, mAggregationComboBox );

    QHBoxLayout * aggregationLayout = new QHBoxLayout();
    aggregationLayout->addWidget( aggregationLabel, 1 );
    aggregationLayout->addWidget( mAggregationComboBox, 1 );
    aggregationLayout->addWidget( aggregationConfigButton, 0 );
    gvlay->addLayout( aggregationLayout );

    connect( aggregationConfigButton, SIGNAL(configureDialogCompleted()),
             this, SLOT(slotSelectDefaultAggregation()) );
    connect( mAggregationComboBox, SIGNAL(activated(int)),
             this, SLOT(slotEmitChanged()) );

    // "Theme"
    using MessageList::Utils::ThemeComboBox;
    mThemeComboBox = new ThemeComboBox( group );

    QLabel *themeLabel = new QLabel( i18n( "Default theme:" ), group );
    themeLabel->setBuddy( mThemeComboBox );

    using MessageList::Utils::ThemeConfigButton;
    ThemeConfigButton * themeConfigButton = new ThemeConfigButton( group, mThemeComboBox );

    QHBoxLayout * themeLayout = new QHBoxLayout();
    themeLayout->addWidget( themeLabel, 1 );
    themeLayout->addWidget( mThemeComboBox, 1 );
    themeLayout->addWidget( themeConfigButton, 0 );
    gvlay->addLayout( themeLayout );

    connect( themeConfigButton, SIGNAL(configureDialogCompleted()),
             this, SLOT(slotSelectDefaultTheme()) );
    connect( mThemeComboBox, SIGNAL(activated(int)),
             this, SLOT(slotEmitChanged()) );

    vlay->addWidget( group );

    // "Date Display" group:
    mDateDisplay = new KButtonGroup( this );
    mDateDisplay->setTitle( i18n("Date Display") );
    gvlay = new QVBoxLayout( mDateDisplay );
    gvlay->setSpacing( KDialog::spacingHint() );

    for ( int i = 0 ; i < numDateDisplayConfig ; ++i ) {
        const char *label = dateDisplayConfig[i].displayName;
        QString buttonLabel;
        if ( QString::fromLatin1(label).contains(QLatin1String("%1")) )
            buttonLabel = i18n( label, DateFormatter::formatCurrentDate( dateDisplayConfig[i].dateDisplay) );
        else
            buttonLabel = i18n( label );
        QRadioButton *radio = new QRadioButton( buttonLabel, mDateDisplay );
        gvlay->addWidget( radio );

        if ( dateDisplayConfig[i].dateDisplay == DateFormatter::Custom ) {
            KHBox *hbox = new KHBox( mDateDisplay );
            hbox->setSpacing( KDialog::spacingHint() );

            mCustomDateFormatEdit = new KLineEdit( hbox );
            mCustomDateFormatEdit->setEnabled( false );
            hbox->setStretchFactor( mCustomDateFormatEdit, 1 );

            connect( radio, SIGNAL(toggled(bool)),
                     mCustomDateFormatEdit, SLOT(setEnabled(bool)) );
            connect( mCustomDateFormatEdit, SIGNAL(textChanged(QString)),
                     this, SLOT(slotEmitChanged()) );

            QLabel *formatHelp = new QLabel(
                        i18n( "<qt><a href=\"whatsthis1\">Custom format information...</a></qt>"), hbox );
            connect( formatHelp, SIGNAL(linkActivated(QString)),
                     SLOT(slotLinkClicked(QString)) );

            mCustomDateWhatsThis =
                    i18n("<qt><p><strong>These expressions may be used for the date:"
                         "</strong></p>"
                         "<ul>"
                         "<li>d - the day as a number without a leading zero (1-31)</li>"
                         "<li>dd - the day as a number with a leading zero (01-31)</li>"
                         "<li>ddd - the abbreviated day name (Mon - Sun)</li>"
                         "<li>dddd - the long day name (Monday - Sunday)</li>"
                         "<li>M - the month as a number without a leading zero (1-12)</li>"
                         "<li>MM - the month as a number with a leading zero (01-12)</li>"
                         "<li>MMM - the abbreviated month name (Jan - Dec)</li>"
                         "<li>MMMM - the long month name (January - December)</li>"
                         "<li>yy - the year as a two digit number (00-99)</li>"
                         "<li>yyyy - the year as a four digit number (0000-9999)</li>"
                         "</ul>"
                         "<p><strong>These expressions may be used for the time:"
                         "</strong></p> "
                         "<ul>"
                         "<li>h - the hour without a leading zero (0-23 or 1-12 if AM/PM display)</li>"
                         "<li>hh - the hour with a leading zero (00-23 or 01-12 if AM/PM display)</li>"
                         "<li>m - the minutes without a leading zero (0-59)</li>"
                         "<li>mm - the minutes with a leading zero (00-59)</li>"
                         "<li>s - the seconds without a leading zero (0-59)</li>"
                         "<li>ss - the seconds with a leading zero (00-59)</li>"
                         "<li>z - the milliseconds without leading zeroes (0-999)</li>"
                         "<li>zzz - the milliseconds with leading zeroes (000-999)</li>"
                         "<li>AP - switch to AM/PM display. AP will be replaced by either \"AM\" or \"PM\".</li>"
                         "<li>ap - switch to AM/PM display. ap will be replaced by either \"am\" or \"pm\".</li>"
                         "<li>Z - time zone in numeric form (-0500)</li>"
                         "</ul>"
                         "<p><strong>All other input characters will be ignored."
                         "</strong></p></qt>");
            mCustomDateFormatEdit->setWhatsThis( mCustomDateWhatsThis );
            radio->setWhatsThis( mCustomDateWhatsThis );
            gvlay->addWidget( hbox );
        }  } // end for loop populating mDateDisplay

    vlay->addWidget( mDateDisplay );
    connect( mDateDisplay, SIGNAL(clicked(int)),
             this, SLOT(slotEmitChanged()) );


    vlay->addStretch( 10 ); // spacer
}

void AppearancePageHeadersTab::slotLinkClicked( const QString & link )
{
    if ( link == QLatin1String( "whatsthis1" ) )
        QWhatsThis::showText( QCursor::pos(), mCustomDateWhatsThis );
}

void AppearancePage::HeadersTab::slotSelectDefaultAggregation()
{
    // Select current default aggregation.
    mAggregationComboBox->selectDefault();
}

void AppearancePage::HeadersTab::slotSelectDefaultTheme()
{
    // Select current default theme.
    mThemeComboBox->selectDefault();
}

void AppearancePage::HeadersTab::doLoadOther()
{
    // "General Options":
    mDisplayMessageToolTips->setChecked(
                MessageList::Core::Settings::self()->messageToolTipEnabled() );

    mHideTabBarWithSingleTab->setChecked(
                MessageList::Core::Settings::self()->autoHideTabBarWithSingleTab() );

    mTabsHaveCloseButton->setChecked(
                MessageList::Core::Settings::self()->tabsHaveCloseButton() );

    // "Aggregation":
    slotSelectDefaultAggregation();

    // "Theme":
    slotSelectDefaultTheme();

    // "Date Display":
    setDateDisplay( MessageCore::GlobalSettings::self()->dateFormat(),
                    MessageCore::GlobalSettings::self()->customDateFormat() );
}

void AppearancePage::HeadersTab::doLoadFromGlobalSettings()
{
    mDisplayMessageToolTips->setChecked(
                MessageList::Core::Settings::self()->messageToolTipEnabled() );

    mHideTabBarWithSingleTab->setChecked(
                MessageList::Core::Settings::self()->autoHideTabBarWithSingleTab() );

    mTabsHaveCloseButton->setChecked(
                MessageList::Core::Settings::self()->tabsHaveCloseButton() );

    // "Aggregation":
    slotSelectDefaultAggregation();

    // "Theme":
    slotSelectDefaultTheme();

    setDateDisplay( MessageCore::GlobalSettings::self()->dateFormat(),
                    MessageCore::GlobalSettings::self()->customDateFormat() );
}


void AppearancePage::HeadersTab::setDateDisplay( int num, const QString & format )
{
    DateFormatter::FormatType dateDisplay =
            static_cast<DateFormatter::FormatType>( num );

    // special case: needs text for the line edit:
    if ( dateDisplay == DateFormatter::Custom )
        mCustomDateFormatEdit->setText( format );

    for ( int i = 0 ; i < numDateDisplayConfig ; ++i )
        if ( dateDisplay == dateDisplayConfig[i].dateDisplay ) {
            mDateDisplay->setSelected( i );
            return;
        }
    // fell through since none found:
    mDateDisplay->setSelected( numDateDisplayConfig - 2 ); // default
}

void AppearancePage::HeadersTab::save()
{
    MessageList::Core::Settings::self()->
            setMessageToolTipEnabled( mDisplayMessageToolTips->isChecked() );

    MessageList::Core::Settings::self()->
            setAutoHideTabBarWithSingleTab( mHideTabBarWithSingleTab->isChecked() );

    MessageList::Core::Settings::self()->
            setTabsHaveCloseButton( mTabsHaveCloseButton->isChecked() );

    KMKernel::self()->savePaneSelection();
    // "Aggregation"
    mAggregationComboBox->writeDefaultConfig();

    // "Theme"
    mThemeComboBox->writeDefaultConfig();

    const int dateDisplayID = mDateDisplay->selected();
    // check bounds:
    if ( ( dateDisplayID >= 0 ) && ( dateDisplayID < numDateDisplayConfig ) ) {
        MessageCore::GlobalSettings::self()->setDateFormat(
                    static_cast<int>( dateDisplayConfig[ dateDisplayID ].dateDisplay ) );
    }
    MessageCore::GlobalSettings::self()->setCustomDateFormat( mCustomDateFormatEdit->text() );
}


//
// Message Window
//

QString AppearancePage::ReaderTab::helpAnchor() const
{
    return QString::fromLatin1("configure-appearance-reader");
}

AppearancePageReaderTab::AppearancePageReaderTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    QVBoxLayout *topLayout = new QVBoxLayout(this);
    topLayout->setSpacing( KDialog::spacingHint() );
    topLayout->setMargin( KDialog::marginHint() );

    // "Close message window after replying or forwarding" check box:
    populateCheckBox( mCloseAfterReplyOrForwardCheck = new QCheckBox( this ),
                      GlobalSettings::self()->closeAfterReplyOrForwardItem() );
    mCloseAfterReplyOrForwardCheck->setToolTip(
                i18n( "Close the standalone message window after replying or forwarding the message" ) );
    topLayout->addWidget( mCloseAfterReplyOrForwardCheck );
    connect( mCloseAfterReplyOrForwardCheck, SIGNAL (stateChanged(int)),
             this, SLOT(slotEmitChanged()) );

    mViewerSettings = new MessageViewer::ConfigureWidget;
    connect( mViewerSettings, SIGNAL(settingsChanged()),
             this, SLOT(slotEmitChanged()) );
    topLayout->addWidget( mViewerSettings );

    topLayout->addStretch( 100 ); // spacer
}

void AppearancePage::ReaderTab::doResetToDefaultsOther()
{
}

void AppearancePage::ReaderTab::doLoadOther()
{
    loadWidget( mCloseAfterReplyOrForwardCheck, GlobalSettings::self()->closeAfterReplyOrForwardItem() );
    mViewerSettings->readConfig();
}


void AppearancePage::ReaderTab::save()
{
    saveCheckBox( mCloseAfterReplyOrForwardCheck, GlobalSettings::self()->closeAfterReplyOrForwardItem() );
    mViewerSettings->writeConfig();
}

QString AppearancePage::SystemTrayTab::helpAnchor() const
{
    return QString::fromLatin1("configure-appearance-systemtray");
}

AppearancePageSystemTrayTab::AppearancePageSystemTrayTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    QVBoxLayout * vlay = new QVBoxLayout( this );
    vlay->setSpacing( KDialog::spacingHint() );
    vlay->setMargin( KDialog::marginHint() );

    // "Enable system tray applet" check box
    mSystemTrayCheck = new QCheckBox( i18n("Enable system tray icon"), this );
    vlay->addWidget( mSystemTrayCheck );
    connect( mSystemTrayCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );

    mSystemTrayShowUnreadMail = new QCheckBox( i18n("Show unread mail in tray icon"), this );
    vlay->addWidget( mSystemTrayShowUnreadMail );
    mSystemTrayShowUnreadMail->setEnabled(false);
    connect( mSystemTrayShowUnreadMail, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mSystemTrayCheck, SIGNAL(toggled(bool)),
             mSystemTrayShowUnreadMail, SLOT(setEnabled(bool)) );


    // System tray modes
    mSystemTrayGroup = new KButtonGroup( this );
    mSystemTrayGroup->setTitle( i18n("System Tray Mode" ) );
    QVBoxLayout *gvlay = new QVBoxLayout( mSystemTrayGroup );
    gvlay->setSpacing( KDialog::spacingHint() );

    connect( mSystemTrayGroup, SIGNAL(clicked(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mSystemTrayCheck, SIGNAL(toggled(bool)),
             mSystemTrayGroup, SLOT(setEnabled(bool)) );

    gvlay->addWidget( new QRadioButton( i18n("Always show KMail in system tray"), mSystemTrayGroup ) );
    gvlay->addWidget( new QRadioButton( i18n("Only show KMail in system tray if there are unread messages"), mSystemTrayGroup ) );

    vlay->addWidget( mSystemTrayGroup );
    vlay->addStretch( 10 ); // spacer
}

void AppearancePage::SystemTrayTab::doLoadFromGlobalSettings()
{
    mSystemTrayCheck->setChecked( GlobalSettings::self()->systemTrayEnabled() );
    mSystemTrayShowUnreadMail->setChecked( GlobalSettings::self()->systemTrayShowUnread() );
    mSystemTrayGroup->setSelected( GlobalSettings::self()->systemTrayPolicy() );
    mSystemTrayGroup->setEnabled( mSystemTrayCheck->isChecked() );
}

void AppearancePage::SystemTrayTab::save()
{
    GlobalSettings::self()->setSystemTrayEnabled( mSystemTrayCheck->isChecked() );
    GlobalSettings::self()->setSystemTrayPolicy( mSystemTrayGroup->selected() );
    GlobalSettings::self()->setSystemTrayShowUnread( mSystemTrayShowUnreadMail->isChecked() );
    GlobalSettings::self()->writeConfig();
}

QString AppearancePage::MessageTagTab::helpAnchor() const
{
    return QString::fromLatin1("configure-appearance-messagetag");
}


TagListWidgetItem::TagListWidgetItem(QListWidget *parent)
    : QListWidgetItem(parent), mTag( 0 )
{
}

TagListWidgetItem::TagListWidgetItem(const QIcon & icon, const QString & text, QListWidget * parent )
    : QListWidgetItem(icon, text, parent), mTag( 0 )
{
}

TagListWidgetItem::~TagListWidgetItem()
{
}

void TagListWidgetItem::setKMailTag( const MailCommon::Tag::Ptr& tag )
{
    mTag = tag;
}

MailCommon::Tag::Ptr TagListWidgetItem::kmailTag() const
{
    return mTag;
}


AppearancePageMessageTagTab::AppearancePageMessageTagTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    mPreviousTag = -1;
    QHBoxLayout *maingrid = new QHBoxLayout( this );
    maingrid->setMargin( KDialog::marginHint() );
    maingrid->setSpacing( KDialog::spacingHint() );

    mNepomukActive = Nepomuk2::ResourceManager::instance()->initialized();
    if ( mNepomukActive ) {

        //Lefthand side Listbox and friends

        //Groupbox frame
        mTagsGroupBox = new QGroupBox( i18n("A&vailable Tags"), this );
        maingrid->addWidget( mTagsGroupBox );
        QVBoxLayout *tageditgrid = new QVBoxLayout( mTagsGroupBox );
        tageditgrid->setMargin( KDialog::marginHint() );
        tageditgrid->setSpacing( KDialog::spacingHint() );
        tageditgrid->addSpacing( 2 * KDialog::spacingHint() );

        //Listbox, add, remove row
        QHBoxLayout *addremovegrid = new QHBoxLayout();
        tageditgrid->addLayout( addremovegrid );

        mTagAddLineEdit = new KLineEdit( mTagsGroupBox );
        mTagAddLineEdit->setTrapReturnKey( true );
        addremovegrid->addWidget( mTagAddLineEdit );

        mTagAddButton = new KPushButton( mTagsGroupBox );
        mTagAddButton->setToolTip( i18n("Add new tag") );
        mTagAddButton->setIcon( KIcon( QLatin1String("list-add") ) );
        addremovegrid->addWidget( mTagAddButton );

        mTagRemoveButton = new KPushButton( mTagsGroupBox );
        mTagRemoveButton->setToolTip( i18n("Remove selected tag") );
        mTagRemoveButton->setIcon( KIcon( QLatin1String("list-remove") ) );
        addremovegrid->addWidget( mTagRemoveButton );

        //Up and down buttons
        QHBoxLayout *updowngrid = new QHBoxLayout();
        tageditgrid->addLayout( updowngrid );

        mTagUpButton = new KPushButton( mTagsGroupBox );
        mTagUpButton->setToolTip( i18n("Increase tag priority") );
        mTagUpButton->setIcon( KIcon( QLatin1String("arrow-up") ) );
        mTagUpButton->setAutoRepeat( true );
        updowngrid->addWidget( mTagUpButton );

        mTagDownButton = new KPushButton( mTagsGroupBox );
        mTagDownButton->setToolTip( i18n("Decrease tag priority") );
        mTagDownButton->setIcon( KIcon( QLatin1String("arrow-down") ) );
        mTagDownButton->setAutoRepeat( true );
        updowngrid->addWidget( mTagDownButton );

        //Listbox for tag names
        QHBoxLayout *listboxgrid = new QHBoxLayout();
        tageditgrid->addLayout( listboxgrid );
        mTagListBox = new QListWidget( mTagsGroupBox );
        mTagListBox->setDragDropMode(QAbstractItemView::InternalMove);
        connect( mTagListBox->model(),SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),SLOT(slotRowsMoved(QModelIndex,int,int,QModelIndex,int)) );

        mTagListBox->setMinimumWidth( 150 );
        listboxgrid->addWidget( mTagListBox );

        //RHS for individual tag settings

        //Extra VBoxLayout for stretchers around settings
        QVBoxLayout *tagsettinggrid = new QVBoxLayout();
        maingrid->addLayout( tagsettinggrid );
        //tagsettinggrid->addStretch( 10 );

        //Groupbox frame
        mTagSettingGroupBox = new QGroupBox( i18n("Ta&g Settings"),
                                             this );
        tagsettinggrid->addWidget( mTagSettingGroupBox );
        QList<KActionCollection *> actionCollections;
        if( kmkernel->getKMMainWidget() )
            actionCollections = kmkernel->getKMMainWidget()->actionCollections();

        QHBoxLayout *lay = new QHBoxLayout(mTagSettingGroupBox);
        mTagWidget = new MailCommon::TagWidget(actionCollections,this);
        lay->addWidget(mTagWidget);

        connect(mTagWidget,SIGNAL(changed()),this, SLOT(slotEmitChangeCheck()));

        //For enabling the add button in case box is non-empty
        connect( mTagAddLineEdit, SIGNAL(textChanged(QString)),
                 this, SLOT(slotAddLineTextChanged(QString)) );

        //For on-the-fly updating of tag name in editbox
        connect( mTagWidget->tagNameLineEdit(), SIGNAL(textChanged(QString)),
                 this, SLOT(slotNameLineTextChanged(QString)) );

        connect( mTagWidget, SIGNAL(iconNameChanged(QString)), SLOT(slotIconNameChanged(QString)) );

        connect(  mTagAddLineEdit, SIGNAL(returnPressed()),
                  this, SLOT(slotAddNewTag()) );


        connect( mTagAddButton, SIGNAL(clicked()),
                 this, SLOT(slotAddNewTag()) );

        connect( mTagRemoveButton, SIGNAL(clicked()),
                 this, SLOT(slotRemoveTag()) );

        connect( mTagUpButton, SIGNAL(clicked()),
                 this, SLOT(slotMoveTagUp()) );

        connect( mTagDownButton, SIGNAL(clicked()),
                 this, SLOT(slotMoveTagDown()) );

        connect( mTagListBox, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
                 this, SLOT(slotSelectionChanged()) );
        //Adjust widths for columns
        maingrid->setStretchFactor( mTagsGroupBox, 1 );
        maingrid->setStretchFactor( lay, 1 );
        tagsettinggrid->addStretch( 10 );


    } else {
        QLabel *lab = new QLabel;
        lab->setText( i18n( "The Nepomuk semantic search service is not available. We cannot configure tags. You can enable it in \"System Settings\"" ) );
        lab->setWordWrap(true);
        maingrid->addWidget( lab );
    }
}

AppearancePageMessageTagTab::~AppearancePageMessageTagTab()
{
}

void AppearancePage::MessageTagTab::slotEmitChangeCheck()
{
    slotEmitChanged();
}

void AppearancePage::MessageTagTab::slotRowsMoved( const QModelIndex &,
                                                   int sourcestart, int sourceEnd,
                                                   const QModelIndex &, int destinationRow )
{
    Q_UNUSED( sourceEnd );
    Q_UNUSED( sourcestart );
    Q_UNUSED( destinationRow );
    updateButtons();
    slotEmitChangeCheck();
}


void AppearancePage::MessageTagTab::updateButtons()
{
    const int currentIndex = mTagListBox->currentRow();

    const bool theFirst = ( currentIndex == 0 );
    const bool theLast = ( currentIndex >= (int)mTagListBox->count() - 1 );
    const bool aFilterIsSelected = ( currentIndex >= 0 );

    mTagUpButton->setEnabled( aFilterIsSelected && !theFirst );
    mTagDownButton->setEnabled( aFilterIsSelected && !theLast );
}

void AppearancePage::MessageTagTab::slotMoveTagUp()
{
    const int tmp_index = mTagListBox->currentRow();
    if ( tmp_index <= 0 )
        return;
    swapTagsInListBox( tmp_index, tmp_index - 1 );
    updateButtons();
}

void AppearancePage::MessageTagTab::slotMoveTagDown()
{
    const int tmp_index = mTagListBox->currentRow();
    if ( ( tmp_index < 0 )
         || ( tmp_index >= int( mTagListBox->count() ) - 1 ) )
        return;
    swapTagsInListBox( tmp_index, tmp_index + 1 );
    updateButtons();
}

void AppearancePage::MessageTagTab::swapTagsInListBox( const int first,
                                                       const int second )
{
    disconnect( mTagListBox, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
                this, SLOT(slotSelectionChanged()) );
    QListWidgetItem *item = mTagListBox->takeItem( first );
    // now selected item is at idx(idx-1), so
    // insert the other item at idx, ie. above(below).
    mPreviousTag = second;
    mTagListBox->insertItem( second, item );
    mTagListBox->setCurrentRow( second );
    connect( mTagListBox, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
             this, SLOT(slotSelectionChanged()) );
    slotEmitChangeCheck();
}

void AppearancePage::MessageTagTab::slotRecordTagSettings( int aIndex )
{
    if ( ( aIndex < 0 ) || ( aIndex >= int( mTagListBox->count() ) )  )
        return;
    QListWidgetItem *item = mTagListBox->item( aIndex );
    TagListWidgetItem *tagItem = static_cast<TagListWidgetItem*>( item );

    MailCommon::Tag::Ptr tmp_desc = tagItem->kmailTag();

    tmp_desc->tagName = tagItem->text();
    mTagWidget->recordTagSettings(tmp_desc);
}

void AppearancePage::MessageTagTab::slotUpdateTagSettingWidgets( int aIndex )
{
    //Check if selection is valid
    if ( ( aIndex < 0 ) || ( mTagListBox->currentRow() < 0 )  ) {
        mTagRemoveButton->setEnabled( false );
        mTagUpButton->setEnabled( false );
        mTagDownButton->setEnabled( false );

        mTagWidget->setEnabled(false);
        return;
    }
    mTagWidget->setEnabled(true);

    mTagRemoveButton->setEnabled( true );
    mTagUpButton->setEnabled( ( 0 != aIndex ) );
    mTagDownButton->setEnabled(( ( int( mTagListBox->count() ) - 1 ) != aIndex ) );
    QListWidgetItem * item = mTagListBox->currentItem();
    TagListWidgetItem *tagItem = static_cast<TagListWidgetItem*>( item );
    MailCommon::Tag::Ptr tmp_desc = tagItem->kmailTag();

    mTagRemoveButton->setEnabled( !tmp_desc->tagStatus );

    disconnect( mTagWidget->tagNameLineEdit(), SIGNAL(textChanged(QString)),
                this, SLOT(slotNameLineTextChanged(QString)) );

    mTagWidget->tagNameLineEdit()->setEnabled(!tmp_desc->tagStatus);
    mTagWidget->tagNameLineEdit()->setText( tmp_desc->tagName );
    connect( mTagWidget->tagNameLineEdit(), SIGNAL(textChanged(QString)),
             this, SLOT(slotNameLineTextChanged(QString)) );


    mTagWidget->setTagTextColor(tmp_desc->textColor);

    mTagWidget->setTagBackgroundColor(tmp_desc->backgroundColor);

    mTagWidget->setTagTextFont(tmp_desc->textFont);

    mTagWidget->iconButton()->setEnabled( !tmp_desc->tagStatus );
    mTagWidget->iconButton()->setIcon( tmp_desc->iconName );

    mTagWidget->keySequenceWidget()->setEnabled( true );
    mTagWidget->keySequenceWidget()->setKeySequence( tmp_desc->shortcut.primary(),
                                                     KKeySequenceWidget::NoValidate );

    mTagWidget->inToolBarCheck()->setEnabled( true );
    mTagWidget->inToolBarCheck()->setChecked( tmp_desc->inToolbar );
}

void AppearancePage::MessageTagTab::slotSelectionChanged()
{
    mEmitChanges = false;
    slotRecordTagSettings( mPreviousTag );
    slotUpdateTagSettingWidgets( mTagListBox->currentRow() );
    mPreviousTag = mTagListBox->currentRow();
    mEmitChanges = true;
}

void AppearancePage::MessageTagTab::slotRemoveTag()
{
    int tmp_index = mTagListBox->currentRow();
    if ( tmp_index >= 0 ) {
        QListWidgetItem * item = mTagListBox->takeItem( mTagListBox->currentRow() );
        TagListWidgetItem *tagItem = static_cast<TagListWidgetItem*>( item );
        MailCommon::Tag::Ptr tmp_desc = tagItem->kmailTag();
        Nepomuk2::Tag nepomukTag( tmp_desc->nepomukResourceUri );
        nepomukTag.remove();
        mPreviousTag = -1;

        //Before deleting the current item, make sure the selectionChanged signal
        //is disconnected, so that the widgets will not get updated while the
        //deletion takes place.
        disconnect( mTagListBox, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
                    this, SLOT(slotSelectionChanged()) );

        delete item;
        connect( mTagListBox, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
                 this, SLOT(slotSelectionChanged()) );

        slotSelectionChanged();
        slotEmitChangeCheck();
    }
}

void AppearancePage::MessageTagTab::slotNameLineTextChanged( const QString
                                                             &aText )
{
    //If deleted all, leave the first character for the sake of not having an
    //empty tag name
    if ( aText.isEmpty() && !mTagListBox->currentItem())
        return;

    const int count = mTagListBox->count();
    for ( int i=0; i < count; ++i ) {
        if(mTagListBox->item(i)->text() == aText) {
            KMessageBox::error(this,i18n("We cannot create tag. A tag with same name already exists."));
            disconnect( mTagWidget->tagNameLineEdit(), SIGNAL(textChanged(QString)),
                        this, SLOT(slotNameLineTextChanged(QString)) );
            mTagWidget->tagNameLineEdit()->setText(mTagListBox->currentItem()->text());
            connect( mTagWidget->tagNameLineEdit(), SIGNAL(textChanged(QString)),
                     this, SLOT(slotNameLineTextChanged(QString)) );
            return;
        }
    }


    //Disconnect so the tag information is not saved and reloaded with every
    //letter
    disconnect( mTagListBox, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
                this, SLOT(slotSelectionChanged()) );

    mTagListBox->currentItem()->setText( aText );
    connect( mTagListBox, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
             this, SLOT(slotSelectionChanged()) );
}

void AppearancePage::MessageTagTab::slotIconNameChanged( const QString &iconName )
{
    mTagListBox->currentItem()->setIcon( KIcon( iconName ) );
}

void AppearancePage::MessageTagTab::slotAddLineTextChanged( const QString &aText )
{
    mTagAddButton->setEnabled( !aText.isEmpty() );
}

void AppearancePage::MessageTagTab::slotAddNewTag()
{  
    const QString newTagName = mTagAddLineEdit->text();
    const int count = mTagListBox->count();
    for ( int i=0; i < count; ++i ) {
        if(mTagListBox->item(i)->text() == newTagName) {
            KMessageBox::error(this,i18n("We cannot create tag. A tag with same name already exists."));
            return;
        }
    }


    const int tmp_priority = mTagListBox->count();
    Nepomuk2::Tag nepomukTag( newTagName );
    nepomukTag.setLabel( newTagName );

    MailCommon::Tag::Ptr tag = MailCommon::Tag::fromNepomuk( nepomukTag );
    tag->priority = tmp_priority;
    slotEmitChangeCheck();
    TagListWidgetItem *newItem = new TagListWidgetItem( KIcon( tag->iconName ), newTagName,  mTagListBox );
    newItem->setKMailTag( tag );
    mTagListBox->addItem( newItem );
    mTagListBox->setCurrentItem( newItem );
    mTagAddLineEdit->setText( QString() );
}

void AppearancePage::MessageTagTab::doLoadFromGlobalSettings()
{
    if ( !mNepomukActive )
        return;

    mTagListBox->clear();
    QList<MailCommon::TagPtr> msgTagList;
    foreach( const Nepomuk2::Tag &nepomukTag, Nepomuk2::Tag::allTags() ) {
        MailCommon::Tag::Ptr tag = MailCommon::Tag::fromNepomuk( nepomukTag );
        msgTagList.append( tag );
    }

    qSort( msgTagList.begin(), msgTagList.end(), MailCommon::Tag::compare );

    foreach( const MailCommon::Tag::Ptr& tag, msgTagList ) {
        TagListWidgetItem *newItem = new TagListWidgetItem( KIcon( tag->iconName ), tag->tagName, mTagListBox );
        newItem->setKMailTag( tag );
        if ( tag->priority == -1 )
            tag->priority = mTagListBox->count() - 1;
    }

    //Disconnect so that insertItem's do not trigger an update procedure
    disconnect( mTagListBox, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
                this, SLOT(slotSelectionChanged()) );

    connect( mTagListBox, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
             this, SLOT(slotSelectionChanged()) );

    slotUpdateTagSettingWidgets( -1 );
    //Needed since the previous function doesn't affect add button
    mTagAddButton->setEnabled( false );

    // Save the original list
    mOriginalMsgTagList.clear();
    foreach( const MailCommon::TagPtr &tag, msgTagList ) {
        mOriginalMsgTagList.append( MailCommon::TagPtr( new MailCommon::Tag( *tag ) ) );
    }
}

void AppearancePage::MessageTagTab::save()
{
    if ( !mNepomukActive ) {
        return;
    }

    const int currentRow = mTagListBox->currentRow();
    if ( currentRow < 0 ) {
        return;
    }

    const int count = mTagListBox->count();
    if ( !count ) {
        return;
    }

    QListWidgetItem *item = mTagListBox->currentItem();
    if ( !item ) {
        return;
    }

    slotRecordTagSettings( currentRow );
    const int numberOfMsgTagList = count;
    for ( int i=0; i < numberOfMsgTagList; ++i ) {
        TagListWidgetItem *tagItem = static_cast<TagListWidgetItem*>( mTagListBox->item(i) );
        if ( ( i>=mOriginalMsgTagList.count() ) || *(tagItem->kmailTag()) != *(mOriginalMsgTagList[i]) ) {
            MailCommon::Tag::Ptr tag = tagItem->kmailTag();
            tag->priority = i;

            MailCommon::Tag::SaveFlags saveFlags = mTagWidget->saveFlags();
            tag->saveToNepomuk( saveFlags );
        }
    }

}


// *************************************************************
// *                                                           *
// *                      ComposerPage                         *
// *                                                           *
// *************************************************************

QString ComposerPage::helpAnchor() const
{
    return QString::fromLatin1("configure-composer");
}

ComposerPage::ComposerPage( const KComponentData &instance, QWidget *parent )
    : ConfigModuleWithTabs( instance, parent )
{
    //
    // "General" tab:
    //
    mGeneralTab = new GeneralTab();
    addTab( mGeneralTab, i18nc("General settings for the composer.", "General") );
    addConfig( GlobalSettings::self(), mGeneralTab );

    //
    // "Templates" tab:
    //
    mTemplatesTab = new TemplatesTab();
    addTab( mTemplatesTab, i18n("Standard Templates") );

    //
    // "Custom Templates" tab:
    //
    mCustomTemplatesTab = new CustomTemplatesTab();
    addTab( mCustomTemplatesTab, i18n("Custom Templates") );

    //
    // "Subject" tab:
    //
    mSubjectTab = new SubjectTab();
    addTab( mSubjectTab, i18nc("Settings regarding the subject when composing a message.","Subject") );
    addConfig( GlobalSettings::self(), mSubjectTab );

    //
    // "Charset" tab:
    //
    mCharsetTab = new CharsetTab();
    addTab( mCharsetTab, i18n("Charset") );

    //
    // "Headers" tab:
    //
    mHeadersTab = new HeadersTab();
    addTab( mHeadersTab, i18n("Headers") );

    //
    // "Attachments" tab:
    //
    mAttachmentsTab = new AttachmentsTab();
    addTab( mAttachmentsTab, i18nc("Config->Composer->Attachments", "Attachments") );

    //
    // "autocorrection" tab:
    //
    mAutoCorrectionTab = new AutoCorrectionTab();
    addTab( mAutoCorrectionTab, i18n("Autocorrection") );

    //
    // "autoresize" tab:
    //
    mAutoImageResizeTab = new AutoImageResizeTab();
    addTab( mAutoImageResizeTab, i18n("Auto Resize Image") );

    //
    // "external editor" tab:
    mExternalEditorTab = new ExternalEditorTab();
    addTab( mExternalEditorTab, i18n("External Editor") );
}

QString ComposerPage::GeneralTab::helpAnchor() const
{
    return QString::fromLatin1("configure-composer-general");
}


ComposerPageGeneralTab::ComposerPageGeneralTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    // Temporary variables
    QLabel *label;
    QGroupBox *groupBox;
    QVBoxLayout *groupVBoxLayout;
    QGridLayout *groupGridLayout;
    QString helpText;
    int row;

    // Main layout
    QHBoxLayout *hb1 = new QHBoxLayout();			// box with 2 columns
    QVBoxLayout *vb1 = new QVBoxLayout();			// first with 2 groupboxes
    QVBoxLayout *vb2 = new QVBoxLayout();			// second with 1 groupbox

    // "Signature" group
    groupBox = new QGroupBox( i18nc( "@title:group", "Signature" ) );
    groupVBoxLayout = new QVBoxLayout();

    // "Automatically insert signature" checkbox
    mAutoAppSignFileCheck = new QCheckBox(
                MessageComposer::MessageComposerSettings::self()->autoTextSignatureItem()->label(),
                this );

    helpText = i18n( "Automatically insert the configured signature\n"
                     "when starting to compose a message" );
    mAutoAppSignFileCheck->setToolTip( helpText );
    mAutoAppSignFileCheck->setWhatsThis( helpText );

    connect( mAutoAppSignFileCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    groupVBoxLayout->addWidget( mAutoAppSignFileCheck );

    // "Insert signature above quoted text" checkbox
    mTopQuoteCheck = new QCheckBox(
                MessageComposer::MessageComposerSettings::self()->prependSignatureItem()->label(), this );
    mTopQuoteCheck->setEnabled( false );

    helpText = i18n( "Insert the signature above any quoted text" );
    mTopQuoteCheck->setToolTip( helpText );
    mTopQuoteCheck->setWhatsThis( helpText );

    connect( mTopQuoteCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mAutoAppSignFileCheck, SIGNAL(toggled(bool)),
             mTopQuoteCheck, SLOT(setEnabled(bool)) );
    groupVBoxLayout->addWidget( mTopQuoteCheck );

    // "Prepend separator to signature" checkbox
    mDashDashCheck = new QCheckBox(
                MessageComposer::MessageComposerSettings::self()->dashDashSignatureItem()->label(), this );
    mDashDashCheck->setEnabled( false );

    helpText = i18n( "Insert the RFC-compliant signature separator\n"
                     "(two dashes and a space on a line) before the signature" );
    mDashDashCheck->setToolTip( helpText );
    mDashDashCheck->setWhatsThis( helpText );

    connect( mDashDashCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mAutoAppSignFileCheck, SIGNAL(toggled(bool)),
             mDashDashCheck, SLOT(setEnabled(bool)) );
    groupVBoxLayout->addWidget( mDashDashCheck);

    // "Remove signature when replying" checkbox
    mStripSignatureCheck = new QCheckBox( TemplateParser::GlobalSettings::self()->stripSignatureItem()->label(),
                                          this );

    helpText = i18n( "When replying, do not quote any existing signature" );
    mStripSignatureCheck->setToolTip( helpText );
    mStripSignatureCheck->setWhatsThis( helpText );

    connect( mStripSignatureCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    groupVBoxLayout->addWidget( mStripSignatureCheck );

    groupBox->setLayout( groupVBoxLayout );
    vb1->addWidget( groupBox );

    // "Format" group
    groupBox = new QGroupBox( i18nc( "@title:group", "Format" ) );
    groupGridLayout = new QGridLayout();
    row = 0;

    // "Only quote selected text when replying" checkbox
    mQuoteSelectionOnlyCheck = new QCheckBox( MessageComposer::MessageComposerSettings::self()->quoteSelectionOnlyItem()->label(),
                                              this );
    helpText = i18n( "When replying, only quote the selected text\n"
                     "(instead of the complete message), if\n"
                     "there is text selected in the message window." );
    mQuoteSelectionOnlyCheck->setToolTip( helpText );
    mQuoteSelectionOnlyCheck->setWhatsThis( helpText );

    connect( mQuoteSelectionOnlyCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    groupGridLayout->addWidget( mQuoteSelectionOnlyCheck, row, 0, 1, -1 );
    ++row;

    // "Use smart quoting" checkbox
    mSmartQuoteCheck = new QCheckBox(
                TemplateParser::GlobalSettings::self()->smartQuoteItem()->label(), this );
    helpText = i18n( "When replying, add quote signs in front of all lines of the quoted text,\n"
                     "even when the line was created by adding an additional line break while\n"
                     "word-wrapping the text." );
    mSmartQuoteCheck->setToolTip( helpText );
    mSmartQuoteCheck->setWhatsThis( helpText );

    connect( mSmartQuoteCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    groupGridLayout->addWidget( mSmartQuoteCheck, row, 0, 1, -1 );
    ++row;

    // "Word wrap at column" checkbox/spinbox
    mWordWrapCheck = new QCheckBox(
                MessageComposer::MessageComposerSettings::self()->wordWrapItem()->label(), this);

    helpText = i18n( "Enable automatic word wrapping at the specified width" );
    mWordWrapCheck->setToolTip( helpText );
    mWordWrapCheck->setWhatsThis( helpText );

    mWrapColumnSpin = new KIntSpinBox( 30/*min*/, 78/*max*/, 1/*step*/,
                                       78/*init*/, this );
    mWrapColumnSpin->setEnabled( false ); // since !mWordWrapCheck->isChecked()

    helpText = i18n( "Set the text width for automatic word wrapping" );
    mWrapColumnSpin->setToolTip( helpText );
    mWrapColumnSpin->setWhatsThis( helpText );

    connect( mWordWrapCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mWrapColumnSpin, SIGNAL(valueChanged(int)),
             this, SLOT(slotEmitChanged()) );
    // only enable the spinbox if the checkbox is checked
    connect( mWordWrapCheck, SIGNAL(toggled(bool)),
             mWrapColumnSpin, SLOT(setEnabled(bool)) );

    groupGridLayout->addWidget( mWordWrapCheck, row, 0 );
    groupGridLayout->addWidget( mWrapColumnSpin, row, 1 );
    ++row;

    // Spacing
    groupGridLayout->setRowMinimumHeight( row, KDialog::spacingHint() );
    ++row;

    // "Reply/Forward using HTML if present" checkbox
    mReplyUsingHtml = new QCheckBox( TemplateParser::GlobalSettings::self()->replyUsingHtmlItem()->label(), this );
    helpText = i18n( "When replying or forwarding, quote the message\n"
                     "in the original format it was received.\n"
                     "If unchecked, the reply will be as plain text by default." );
    mReplyUsingHtml->setToolTip( helpText );
    mReplyUsingHtml->setWhatsThis( helpText );

    connect( mReplyUsingHtml, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    groupGridLayout->addWidget( mReplyUsingHtml, row, 0, 1, -1 );
    ++row;

    // "Improve plain text of HTML" checkbox
    mImprovePlainTextOfHtmlMessage = new QCheckBox( MessageComposer::MessageComposerSettings::self()->improvePlainTextOfHtmlMessageItem()->label(), this );

    // For what is supported see http://www.grantlee.org/apidox/classGrantlee_1_1PlainTextMarkupBuilder.html
    helpText = i18n( "Format the plain text part of a message from the HTML markup.\n"
                     "Bold, italic and underlined text, lists, and external references\n"
                     "are supported." );
    mImprovePlainTextOfHtmlMessage->setToolTip( helpText );
    mImprovePlainTextOfHtmlMessage->setWhatsThis( helpText );

    connect( mImprovePlainTextOfHtmlMessage, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    groupGridLayout->addWidget( mImprovePlainTextOfHtmlMessage, row, 0, 1, -1 );
    ++row;

    // Spacing
    groupGridLayout->setRowMinimumHeight( row, KDialog::spacingHint() );
    ++row;

    // "Autosave interval" spinbox
    mAutoSave = new KIntSpinBox( 0, 60, 1, 1, this );
    mAutoSave->setObjectName( QLatin1String("kcfg_AutosaveInterval") );
    mAutoSave->setSpecialValueText( i18n("No autosave") );
    mAutoSave->setSuffix( ki18ncp( "Interval suffix", " minute", " minutes" ) );

    helpText = i18n( "Automatically save the message at this specified interval" );
    mAutoSave->setToolTip( helpText );
    mAutoSave->setWhatsThis( helpText );

    label = new QLabel( GlobalSettings::self()->autosaveIntervalItem()->label(), this );
    label->setBuddy( mAutoSave );

    connect( mAutoSave, SIGNAL(valueChanged(int)),
             this, SLOT(slotEmitChanged()) );

    groupGridLayout->addWidget( label, row, 0 );
    groupGridLayout->addWidget( mAutoSave, row, 1 );
    ++row;

#ifdef KDEPIM_ENTERPRISE_BUILD
    // "Default forwarding type" combobox
    mForwardTypeCombo = new KComboBox( false, this );
    mForwardTypeCombo->addItems( QStringList() << i18nc( "@item:inlistbox Inline mail forwarding",
                                                         "Inline" )
                                 << i18n( "As Attachment" ) );

    helpText = i18n( "Set the default forwarded message format" );
    mForwardTypeCombo->setToolTip( helpText );
    mForwardTypeCombo->setWhatsThis( helpText );

    label = new QLabel( i18n( "Default forwarding type:" ), this );
    label->setBuddy( mForwardTypeCombo );

    connect( mForwardTypeCombo, SIGNAL(activated(int)),
             this, SLOT(slotEmitChanged()) );

    groupGridLayout->addWidget( label, row, 0 );
    groupGridLayout->addWidget( mForwardTypeCombo, row, 1 );
    ++row;
#endif

    groupBox->setLayout( groupGridLayout );
    vb1->addWidget( groupBox );

    // "Recipients" group
    groupBox = new QGroupBox( i18nc( "@title:group", "Recipients" ) );
    groupGridLayout = new QGridLayout();
    row = 0;

    // "Automatically request MDNs" checkbox
    mAutoRequestMDNCheck = new QCheckBox( GlobalSettings::self()->requestMDNItem()->label(),
                                          this);

    helpText = i18n( "By default, request an MDN when starting to compose a message.\n"
                     "You can select this on a per-message basis using \"Options - Request Disposition Notification\"" );
    mAutoRequestMDNCheck->setToolTip( helpText );
    mAutoRequestMDNCheck->setWhatsThis( helpText );

    connect( mAutoRequestMDNCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    groupGridLayout->addWidget( mAutoRequestMDNCheck, row, 0, 1, -1 );
    ++row;

    // Spacing
    groupGridLayout->setRowMinimumHeight( row, KDialog::spacingHint() );
    ++row;

#ifdef KDEPIM_ENTERPRISE_BUILD
    // "Warn if too many recipients" checkbox/spinbox
    mRecipientCheck = new QCheckBox(
                GlobalSettings::self()->tooManyRecipientsItem()->label(), this);
    mRecipientCheck->setObjectName( QLatin1String("kcfg_TooManyRecipients") );
    helpText = i18n( GlobalSettings::self()->tooManyRecipientsItem()->whatsThis().toUtf8() );
    mRecipientCheck->setWhatsThis( helpText );
    mRecipientCheck->setToolTip( i18n( "Warn if too many recipients are specified" ) );

    mRecipientSpin = new KIntSpinBox( 1/*min*/, 100/*max*/, 1/*step*/,
                                      5/*init*/, this );
    mRecipientSpin->setObjectName( QLatin1String("kcfg_RecipientThreshold") );
    mRecipientSpin->setEnabled( false );
    helpText = i18n( GlobalSettings::self()->recipientThresholdItem()->whatsThis().toUtf8() );
    mRecipientSpin->setWhatsThis( helpText );
    mRecipientSpin->setToolTip( i18n( "Set the maximum number of recipients for the warning" ) );

    connect( mRecipientCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mRecipientSpin, SIGNAL(valueChanged(int)),
             this, SLOT(slotEmitChanged()) );
    // only enable the spinbox if the checkbox is checked
    connect( mRecipientCheck, SIGNAL(toggled(bool)),
             mRecipientSpin, SLOT(setEnabled(bool)) );

    groupGridLayout->addWidget( mRecipientCheck, row, 0, 1, 2);
    groupGridLayout->addWidget( mRecipientSpin, row, 2 );
    ++row;
#endif

    // "Maximum Reply-to-All recipients" spinbox
    mMaximumRecipients = new KIntSpinBox( 0, 9999, 1, 1, this );

    helpText = i18n( "Only allow this many recipients to be specified for the message.\n"
                     "This applies to doing a \"Reply to All\", entering recipients manually\n"
                     "or using the \"Select...\" picker.  Setting this limit helps you to\n"
                     "avoid accidentally sending a message to too many people.  Note,\n"
                     "however, that it does not take account of distribution lists or\n"
                     "mailing lists." );
    mMaximumRecipients->setToolTip( helpText );
    mMaximumRecipients->setWhatsThis( helpText );

    label = new QLabel( MessageComposer::MessageComposerSettings::self()->maximumRecipientsItem()->label(), this );
    label->setBuddy(mMaximumRecipients);

    connect( mMaximumRecipients, SIGNAL(valueChanged(int)),
             this, SLOT(slotEmitChanged()) );

    groupGridLayout->addWidget( label, row, 0, 1, 2 );
    groupGridLayout->addWidget( mMaximumRecipients, row, 2 );
    ++row;

    // Spacing
    groupGridLayout->setRowMinimumHeight( row, KDialog::spacingHint() );
    ++row;

    // "Use recent addresses for autocompletion" checkbox
    mShowRecentAddressesInComposer = new QCheckBox(
                MessageComposer::MessageComposerSettings::self()->showRecentAddressesInComposerItem()->label(),
                this);

    helpText = i18n( "Remember recent addresses entered,\n"
                     "and offer them for recipient completion" );
    mShowRecentAddressesInComposer->setToolTip( helpText );
    mShowRecentAddressesInComposer->setWhatsThis( helpText );

    connect( mShowRecentAddressesInComposer, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    groupGridLayout->addWidget( mShowRecentAddressesInComposer, row, 0, 1, -1 );
    ++row;

    // "Maximum recent addresses retained" spinbox
    mMaximumRecentAddress = new KIntNumInput( this );
    mMaximumRecentAddress->setSliderEnabled( false );
    mMaximumRecentAddress->setMinimum( 0 );
    mMaximumRecentAddress->setMaximum( 999 );
    mMaximumRecentAddress->setSpecialValueText( i18nc( "No addresses are retained", "No save" ) );
    mMaximumRecentAddress->setEnabled( false );

    label = new QLabel( i18n( "Maximum recent addresses retained:" ) );
    label->setBuddy( mMaximumRecentAddress );
    label->setEnabled( false );

    helpText = i18n( "The maximum number of recently entered addresses that will\n"
                     "be remembered for completion" );
    mMaximumRecentAddress->setToolTip( helpText );
    mMaximumRecentAddress->setWhatsThis( helpText );

    connect( mMaximumRecentAddress, SIGNAL(valueChanged(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mShowRecentAddressesInComposer, SIGNAL(toggled(bool)),
             mMaximumRecentAddress, SLOT(setEnabled(bool)) );
    connect( mShowRecentAddressesInComposer, SIGNAL(toggled(bool)),
             label, SLOT(setEnabled(bool)) );

    groupGridLayout->addWidget( label, row, 0, 1, 2 );
    groupGridLayout->addWidget( mMaximumRecentAddress, row, 2 );
    ++row;

    // "Edit Recent Addresses" button
    QPushButton *recentAddressesBtn = new QPushButton( i18n( "Edit Recent Addresses..." ), this );
    helpText = i18n( "Edit, add or remove recent addresses" );
    recentAddressesBtn->setToolTip( helpText );
    recentAddressesBtn->setWhatsThis( helpText );

    connect( recentAddressesBtn, SIGNAL(clicked()),
             this, SLOT(slotConfigureRecentAddresses()) );
    groupGridLayout->addWidget( recentAddressesBtn, row, 1, 1, 2 );
    ++row;

    // Spacing
    groupGridLayout->setRowMinimumHeight( row, KDialog::spacingHint() );
    ++row;

    // "Configure Completion Order" button
    QPushButton *completionOrderBtn = new QPushButton( i18n( "Configure Completion Order..." ), this );
    helpText = i18n( "Configure the order in which address books\n"
                     "will be used when doing address completion" );
    completionOrderBtn->setToolTip( helpText );
    completionOrderBtn->setWhatsThis( helpText );

    connect( completionOrderBtn, SIGNAL(clicked()),
             this, SLOT(slotConfigureCompletionOrder()) );
    groupGridLayout->addWidget( completionOrderBtn, row, 1, 1, 2 );

    groupBox->setLayout( groupGridLayout );
    vb2->addWidget( groupBox );

    // Finish up main layout
    vb1->addStretch( 1 );
    vb2->addStretch( 1 );

    hb1->addLayout( vb1 );
    hb1->addLayout( vb2 );
    setLayout( hb1 );
}


void ComposerPage::GeneralTab::doResetToDefaultsOther()
{
    const bool bUseDefaults = MessageComposer::MessageComposerSettings::self()->useDefaults( true );

    const bool autoAppSignFile = MessageComposer::MessageComposerSettings::self()->autoTextSignature()==QLatin1String( "auto" );
    const bool topQuoteCheck = MessageComposer::MessageComposerSettings::self()->prependSignature();
    const bool dashDashSignature = MessageComposer::MessageComposerSettings::self()->dashDashSignature();
    const bool smartQuoteCheck = MessageComposer::MessageComposerSettings::self()->quoteSelectionOnly();
    const bool wordWrap = MessageComposer::MessageComposerSettings::self()->wordWrap();
    const int wrapColumn = MessageComposer::MessageComposerSettings::self()->lineWrapWidth();
    const bool showRecentAddress = MessageComposer::MessageComposerSettings::self()->showRecentAddressesInComposer();
    const int maximumRecipient = MessageComposer::MessageComposerSettings::self()->maximumRecipients();
    const bool improvePlainText = MessageComposer::MessageComposerSettings::self()->improvePlainTextOfHtmlMessage();

    MessageComposer::MessageComposerSettings::self()->useDefaults( bUseDefaults );

    mAutoAppSignFileCheck->setChecked( autoAppSignFile );
    mTopQuoteCheck->setChecked( topQuoteCheck );
    mDashDashCheck->setChecked( dashDashSignature );
    mQuoteSelectionOnlyCheck->setChecked( smartQuoteCheck );
    mWordWrapCheck->setChecked( wordWrap );
    mWrapColumnSpin->setValue( wrapColumn );
    mMaximumRecipients->setValue( maximumRecipient );
    mShowRecentAddressesInComposer->setChecked( showRecentAddress );
    mImprovePlainTextOfHtmlMessage->setChecked(improvePlainText);

    mMaximumRecentAddress->setValue( 40 );
}

void ComposerPage::GeneralTab::doLoadFromGlobalSettings()
{
    // various check boxes:

    mAutoAppSignFileCheck->setChecked(
                MessageComposer::MessageComposerSettings::self()->autoTextSignature()==QLatin1String( "auto" ) );
    mTopQuoteCheck->setChecked( MessageComposer::MessageComposerSettings::self()->prependSignature() );
    mDashDashCheck->setChecked( MessageComposer::MessageComposerSettings::self()->dashDashSignature() );
    mSmartQuoteCheck->setChecked( TemplateParser::GlobalSettings::self()->smartQuote() );
    mQuoteSelectionOnlyCheck->setChecked( MessageComposer::MessageComposerSettings::self()->quoteSelectionOnly() );
    mReplyUsingHtml->setChecked( TemplateParser::GlobalSettings::self()->replyUsingHtml() );
    mStripSignatureCheck->setChecked( TemplateParser::GlobalSettings::self()->stripSignature() );
    mAutoRequestMDNCheck->setChecked( GlobalSettings::self()->requestMDN() );
    mWordWrapCheck->setChecked( MessageComposer::MessageComposerSettings::self()->wordWrap() );
    mWrapColumnSpin->setValue( MessageComposer::MessageComposerSettings::self()->lineWrapWidth() );
    mMaximumRecipients->setValue( MessageComposer::MessageComposerSettings::self()->maximumRecipients() );
    mAutoSave->setValue( GlobalSettings::self()->autosaveInterval() );
    mShowRecentAddressesInComposer->setChecked( MessageComposer::MessageComposerSettings::self()->showRecentAddressesInComposer() );
    mImprovePlainTextOfHtmlMessage->setChecked(MessageComposer::MessageComposerSettings::self()->improvePlainTextOfHtmlMessage());

#ifdef KDEPIM_ENTERPRISE_BUILD
    mRecipientCheck->setChecked( GlobalSettings::self()->tooManyRecipients() );
    mRecipientSpin->setValue( GlobalSettings::self()->recipientThreshold() );
    if ( GlobalSettings::self()->forwardingInlineByDefault() )
        mForwardTypeCombo->setCurrentIndex( 0 );
    else
        mForwardTypeCombo->setCurrentIndex( 1 );
#endif

    mMaximumRecentAddress->setValue(RecentAddresses::self(  MessageComposer::MessageComposerSettings::self()->config() )->maxCount());
}

void ComposerPage::GeneralTab::save() {
    MessageComposer::MessageComposerSettings::self()->setAutoTextSignature(
                mAutoAppSignFileCheck->isChecked() ? QLatin1String("auto") : QLatin1String("manual") );
    MessageComposer::MessageComposerSettings::self()->setPrependSignature( mTopQuoteCheck->isChecked() );
    MessageComposer::MessageComposerSettings::self()->setDashDashSignature( mDashDashCheck->isChecked() );
    TemplateParser::GlobalSettings::self()->setSmartQuote( mSmartQuoteCheck->isChecked() );
    MessageComposer::MessageComposerSettings::self()->setQuoteSelectionOnly( mQuoteSelectionOnlyCheck->isChecked() );
    TemplateParser::GlobalSettings::self()->setReplyUsingHtml( mReplyUsingHtml->isChecked() );
    TemplateParser::GlobalSettings::self()->setStripSignature( mStripSignatureCheck->isChecked() );
    GlobalSettings::self()->setRequestMDN( mAutoRequestMDNCheck->isChecked() );
    MessageComposer::MessageComposerSettings::self()->setWordWrap( mWordWrapCheck->isChecked() );
    MessageComposer::MessageComposerSettings::self()->setLineWrapWidth( mWrapColumnSpin->value() );
    MessageComposer::MessageComposerSettings::self()->setMaximumRecipients( mMaximumRecipients->value() );
    GlobalSettings::self()->setAutosaveInterval( mAutoSave->value() );
    MessageComposer::MessageComposerSettings::self()->setShowRecentAddressesInComposer( mShowRecentAddressesInComposer->isChecked() );
    MessageComposer::MessageComposerSettings::self()->setImprovePlainTextOfHtmlMessage( mImprovePlainTextOfHtmlMessage->isChecked() );
#ifdef KDEPIM_ENTERPRISE_BUILD
    GlobalSettings::self()->setTooManyRecipients( mRecipientCheck->isChecked() );
    GlobalSettings::self()->setRecipientThreshold( mRecipientSpin->value() );
    GlobalSettings::self()->setForwardingInlineByDefault( mForwardTypeCombo->currentIndex() == 0 );
#endif

    RecentAddresses::self(  MessageComposer::MessageComposerSettings::self()->config() )->setMaxCount( mMaximumRecentAddress->value() );

    MessageComposer::MessageComposerSettings::self()->requestSync();
}

void ComposerPage::GeneralTab::slotConfigureRecentAddresses()
{
    MessageViewer::AutoQPointer<KPIM::RecentAddressDialog> dlg( new KPIM::RecentAddressDialog( this ) );
    dlg->setAddresses( RecentAddresses::self(  MessageComposer::MessageComposerSettings::self()->config() )->addresses() );
    if ( dlg->exec() && dlg ) {
        RecentAddresses::self(  MessageComposer::MessageComposerSettings::self()->config() )->clear();
        const QStringList &addrList = dlg->addresses();
        QStringList::ConstIterator it;
        QStringList::ConstIterator end( addrList.constEnd() );

        for ( it = addrList.constBegin(); it != end; ++it )
            RecentAddresses::self(  MessageComposer::MessageComposerSettings::self()->config() )->add( *it );
    }
}

void ComposerPage::GeneralTab::slotConfigureCompletionOrder()
{
    KLDAP::LdapClientSearch search;
    KPIM::CompletionOrderEditor editor( &search, this );
    editor.exec();
}

QString ComposerPage::ExternalEditorTab::helpAnchor() const
{
    return QString::fromLatin1("configure-composer-externaleditor");
}

ComposerPageExternalEditorTab::ComposerPageExternalEditorTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    QVBoxLayout *layout = new QVBoxLayout( this );

    mExternalEditorCheck = new QCheckBox(
                GlobalSettings::self()->useExternalEditorItem()->label(), this);
    mExternalEditorCheck->setObjectName( QLatin1String("kcfg_UseExternalEditor") );
    connect( mExternalEditorCheck, SIGNAL(toggled(bool)),
             this, SLOT(slotEmitChanged()) );

    KHBox *hbox = new KHBox( this );
    QLabel *label = new QLabel( GlobalSettings::self()->externalEditorItem()->label(),
                                hbox );
    mEditorRequester = new KUrlRequester( hbox );
    //Laurent 25/10/2011 fix #Bug 256655 - A "save changes?" dialog appears ALWAYS when leaving composer settings, even when unchanged.
    //mEditorRequester->setObjectName( "kcfg_ExternalEditor" );
    connect( mEditorRequester, SIGNAL(urlSelected(KUrl)),
             this, SLOT(slotEmitChanged()) );
    connect( mEditorRequester, SIGNAL(textChanged(QString)),
             this, SLOT(slotEmitChanged()) );

    hbox->setStretchFactor( mEditorRequester, 1 );
    label->setBuddy( mEditorRequester );
    label->setEnabled( false ); // since !mExternalEditorCheck->isChecked()
    // ### FIXME: allow only executables (x-bit when available..)
    mEditorRequester->setFilter( QLatin1String("application/x-executable "
                                               "application/x-shellscript "
                                               "application/x-desktop") );
    mEditorRequester->setMode(KFile::File|KFile::ExistingOnly|KFile::LocalOnly);
    mEditorRequester->setEnabled( false ); // !mExternalEditorCheck->isChecked()
    connect( mExternalEditorCheck, SIGNAL(toggled(bool)),
             label, SLOT(setEnabled(bool)) );
    connect( mExternalEditorCheck, SIGNAL(toggled(bool)),
             mEditorRequester, SLOT(setEnabled(bool)) );

    label = new QLabel( i18n("<b>%f</b> will be replaced with the "
                             "filename to edit.<br />"
                             "<b>%w</b> will be replaced with the window id.<br />"
                             "<b>%l</b> will be replaced with the line number."), this );
    label->setEnabled( false ); // see above
    connect( mExternalEditorCheck, SIGNAL(toggled(bool)),
             label, SLOT(setEnabled(bool)) );
    layout->addWidget( mExternalEditorCheck );
    layout->addWidget( hbox );
    layout->addWidget( label );
    layout->addStretch();
}

void ComposerPage::ExternalEditorTab::doLoadFromGlobalSettings()
{
    mExternalEditorCheck->setChecked( GlobalSettings::self()->useExternalEditor() );
    mEditorRequester->setText( GlobalSettings::self()->externalEditor() );
}

void ComposerPage::ExternalEditorTab::save()
{
    GlobalSettings::self()->setUseExternalEditor( mExternalEditorCheck->isChecked() );
    GlobalSettings::self()->setExternalEditor( mEditorRequester->text() );
}

QString ComposerPage::TemplatesTab::helpAnchor() const
{
    return QString::fromLatin1("configure-composer-templates");
}

ComposerPageTemplatesTab::ComposerPageTemplatesTab( QWidget * parent )
    : ConfigModuleTab ( parent )
{
    QVBoxLayout* vlay = new QVBoxLayout( this );
    vlay->setMargin( 0 );
    vlay->setSpacing( KDialog::spacingHint() );

    mWidget = new TemplateParser::TemplatesConfiguration( this );
    vlay->addWidget( mWidget );

    connect( mWidget, SIGNAL(changed()),
             this, SLOT(slotEmitChanged()) );
}

void ComposerPage::TemplatesTab::doLoadFromGlobalSettings()
{
    mWidget->loadFromGlobal();
}

void ComposerPage::TemplatesTab::save()
{
    mWidget->saveToGlobal();
}

void ComposerPage::TemplatesTab::doResetToDefaultsOther()
{
    mWidget->resetToDefault();
}

QString ComposerPage::CustomTemplatesTab::helpAnchor() const
{
    return QString::fromLatin1("configure-composer-custom-templates");
}

ComposerPageCustomTemplatesTab::ComposerPageCustomTemplatesTab( QWidget * parent )
    : ConfigModuleTab ( parent )
{
    QVBoxLayout* vlay = new QVBoxLayout( this );
    vlay->setMargin( 0 );
    vlay->setSpacing( KDialog::spacingHint() );

    mWidget = new TemplateParser::CustomTemplates( kmkernel->getKMMainWidget() ? kmkernel->getKMMainWidget()->actionCollections() : QList<KActionCollection*>(), this );
    vlay->addWidget( mWidget );

    connect( mWidget, SIGNAL(changed()),
             this, SLOT(slotEmitChanged()) );
    if( KMKernel::self() )
        connect( mWidget, SIGNAL(templatesUpdated()), KMKernel::self(), SLOT(updatedTemplates()) );
}

void ComposerPage::CustomTemplatesTab::doLoadFromGlobalSettings()
{
    mWidget->load();
}

void ComposerPage::CustomTemplatesTab::save()
{
    mWidget->save();
}

QString ComposerPage::SubjectTab::helpAnchor() const
{
    return QString::fromLatin1("configure-composer-subject");
}

ComposerPageSubjectTab::ComposerPageSubjectTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    // tmp. vars:
    QVBoxLayout *vlay;
    QGroupBox   *group;
    QLabel      *label;


    vlay = new QVBoxLayout( this );
    vlay->setSpacing( KDialog::spacingHint() );
    vlay->setMargin( KDialog::marginHint() );

    group = new QGroupBox( i18n("Repl&y Subject Prefixes"), this );
    QLayout *layout = new QVBoxLayout( group );
    group->layout()->setSpacing( KDialog::spacingHint() );

    // row 0: help text:
    label = new QLabel( i18n("Recognize any sequence of the following prefixes\n"
                             "(entries are case-insensitive regular expressions):"), group );
    label->setWordWrap( true );
    label->setAlignment( Qt::AlignLeft );

    // row 1, string list editor:
    PimCommon::SimpleStringListEditor::ButtonCode buttonCode =
            static_cast<PimCommon::SimpleStringListEditor::ButtonCode>( PimCommon::SimpleStringListEditor::Add | PimCommon::SimpleStringListEditor::Remove | PimCommon::SimpleStringListEditor::Modify );
    mReplyListEditor =
            new PimCommon::SimpleStringListEditor( group, buttonCode,
                                                   i18n("A&dd..."), i18n("Re&move"),
                                                   i18n("Mod&ify..."),
                                                   i18n("Enter new reply prefix:") );
    connect( mReplyListEditor, SIGNAL(changed()),
             this, SLOT(slotEmitChanged()) );

    // row 2: "replace [...]" check box:
    mReplaceReplyPrefixCheck = new QCheckBox(
                MessageComposer::MessageComposerSettings::self()->replaceReplyPrefixItem()->label(),
                group);
    connect( mReplaceReplyPrefixCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    layout->addWidget( label );
    layout->addWidget( mReplyListEditor );
    layout->addWidget( mReplaceReplyPrefixCheck );

    vlay->addWidget( group );


    group = new QGroupBox( i18n("For&ward Subject Prefixes"), this );
    layout = new QVBoxLayout( group );
    group->layout()->setSpacing( KDialog::marginHint() );

    // row 0: help text:
    label= new QLabel( i18n("Recognize any sequence of the following prefixes\n"
                            "(entries are case-insensitive regular expressions):"), group );
    label->setAlignment( Qt::AlignLeft );
    label->setWordWrap( true );

    // row 1: string list editor
    mForwardListEditor =
            new PimCommon::SimpleStringListEditor( group, buttonCode,
                                                   i18n("Add..."),
                                                   i18n("Remo&ve"),
                                                   i18n("Modify..."),
                                                   i18n("Enter new forward prefix:") );
    connect( mForwardListEditor, SIGNAL(changed()),
             this, SLOT(slotEmitChanged()) );

    // row 3: "replace [...]" check box:
    mReplaceForwardPrefixCheck = new QCheckBox(
                MessageComposer::MessageComposerSettings::self()->replaceForwardPrefixItem()->label(),
                group);
    connect( mReplaceForwardPrefixCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    layout->addWidget( label );
    layout->addWidget( mForwardListEditor );
    layout->addWidget( mReplaceForwardPrefixCheck );
    vlay->addWidget( group );
}

void ComposerPage::SubjectTab::doLoadFromGlobalSettings()
{
    mReplyListEditor->setStringList( MessageComposer::MessageComposerSettings::self()->replyPrefixes() );
    mReplaceReplyPrefixCheck->setChecked( MessageComposer::MessageComposerSettings::self()->replaceReplyPrefix() );
    mForwardListEditor->setStringList( MessageComposer::MessageComposerSettings::self()->forwardPrefixes() );
    mReplaceForwardPrefixCheck->setChecked( MessageComposer::MessageComposerSettings::self()->replaceForwardPrefix() );
}

void ComposerPage::SubjectTab::save()
{
    MessageComposer::MessageComposerSettings::self()->setReplyPrefixes( mReplyListEditor->stringList() );
    MessageComposer::MessageComposerSettings::self()->setForwardPrefixes( mForwardListEditor->stringList() );
    MessageComposer::MessageComposerSettings::self()->setReplaceForwardPrefix( mReplaceForwardPrefixCheck->isChecked() );
    MessageComposer::MessageComposerSettings::self()->setReplaceReplyPrefix( mReplaceReplyPrefixCheck->isChecked() );
}

void ComposerPage::SubjectTab::doResetToDefaultsOther()
{
    const bool bUseDefaults = MessageComposer::MessageComposerSettings::self()->useDefaults( true );
    const QStringList messageReplyPrefixes = MessageComposer::MessageComposerSettings::replyPrefixes();
    const bool useMessageReplyPrefixes = MessageComposer::MessageComposerSettings::replaceReplyPrefix();

    const QStringList messageForwardPrefixes = MessageComposer::MessageComposerSettings::forwardPrefixes();
    const bool useMessageForwardPrefixes = MessageComposer::MessageComposerSettings::replaceForwardPrefix();

    MessageComposer::MessageComposerSettings::self()->useDefaults( bUseDefaults );
    mReplyListEditor->setStringList( messageReplyPrefixes );
    mReplaceReplyPrefixCheck->setChecked( useMessageReplyPrefixes );
    mForwardListEditor->setStringList( messageForwardPrefixes );
    mReplaceForwardPrefixCheck->setChecked( useMessageForwardPrefixes );
}


QString ComposerPage::CharsetTab::helpAnchor() const
{
    return QString::fromLatin1("configure-composer-charset");
}

ComposerPageCharsetTab::ComposerPageCharsetTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    // tmp. vars:
    QVBoxLayout *vlay;
    QLabel      *label;

    vlay = new QVBoxLayout( this );
    vlay->setSpacing( KDialog::spacingHint() );
    vlay->setMargin( KDialog::marginHint() );

    label = new QLabel( i18n("This list is checked for every outgoing message "
                             "from the top to the bottom for a charset that "
                             "contains all required characters."), this );
    label->setWordWrap(true);
    vlay->addWidget( label );

    mCharsetListEditor =
            new PimCommon::SimpleStringListEditor( this, PimCommon::SimpleStringListEditor::All,
                                                   i18n("A&dd..."), i18n("Remo&ve"),
                                                   i18n("&Modify..."), i18n("Enter charset:") );
    mCharsetListEditor->setUpDownAutoRepeat(true);
    connect( mCharsetListEditor, SIGNAL(changed()),
             this, SLOT(slotEmitChanged()) );

    vlay->addWidget( mCharsetListEditor, 1 );

    mKeepReplyCharsetCheck = new QCheckBox( i18n("&Keep original charset when "
                                                 "replying or forwarding (if "
                                                 "possible)"), this );
    connect( mKeepReplyCharsetCheck, SIGNAL (stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    vlay->addWidget( mKeepReplyCharsetCheck );

    connect( mCharsetListEditor, SIGNAL(aboutToAdd(QString&)),
             this, SLOT(slotVerifyCharset(QString&)) );
}

void ComposerPage::CharsetTab::slotVerifyCharset( QString & charset )
{
    if ( charset.isEmpty() ) return;

    // KCharsets::codecForName("us-ascii") returns "iso-8859-1" (cf. Bug #49812)
    // therefore we have to treat this case specially
    if ( charset.toLower() == QString::fromLatin1("us-ascii") ) {
        charset = QString::fromLatin1("us-ascii");
        return;
    }

    if ( charset.toLower() == QString::fromLatin1("locale") ) {
        charset =  QString::fromLatin1("%1 (locale)")
                .arg( QString::fromLatin1(kmkernel->networkCodec()->name() ).toLower() );
        return;
    }

    bool ok = false;
    QTextCodec *codec = KGlobal::charsets()->codecForName( charset, ok );
    if ( ok && codec ) {
        charset = QString::fromLatin1( codec->name() ).toLower();
        return;
    }

    KMessageBox::sorry( this, i18n("This charset is not supported.") );
    charset.clear();
}

void ComposerPage::CharsetTab::doLoadOther()
{
    QStringList charsets = MessageComposer::MessageComposerSettings::preferredCharsets();
    QStringList::Iterator end( charsets.end() );
    for ( QStringList::Iterator it = charsets.begin() ;
          it != end ; ++it )
        if ( (*it) == QString::fromLatin1("locale") ) {
            QByteArray cset = kmkernel->networkCodec()->name();
            kAsciiToLower( cset.data() );
            (*it) = QString::fromLatin1("%1 (locale)").arg( QString::fromLatin1( cset ) );
        }

    mCharsetListEditor->setStringList( charsets );
    mKeepReplyCharsetCheck->setChecked( MessageComposer::MessageComposerSettings::forceReplyCharset() );
}


void ComposerPage::CharsetTab::doResetToDefaultsOther()
{
    const bool bUseDefaults = MessageComposer::MessageComposerSettings::self()->useDefaults( true );
    const QStringList charsets = MessageComposer::MessageComposerSettings::preferredCharsets();
    const bool keepReplyCharsetCheck = MessageComposer::MessageComposerSettings::forceReplyCharset();
    MessageComposer::MessageComposerSettings::self()->useDefaults( bUseDefaults );
    mCharsetListEditor->setStringList( charsets );
    mKeepReplyCharsetCheck->setChecked( keepReplyCharsetCheck );
    slotEmitChanged();
}

void ComposerPage::CharsetTab::save()
{
    QStringList charsetList = mCharsetListEditor->stringList();
    QStringList::Iterator it = charsetList.begin();
    QStringList::Iterator end = charsetList.end();

    for ( ; it != end ; ++it )
        if ( (*it).endsWith( QLatin1String("(locale)") ) )
            (*it) = QLatin1String("locale");
    MessageComposer::MessageComposerSettings::setPreferredCharsets( charsetList );
    MessageComposer::MessageComposerSettings::setForceReplyCharset( mKeepReplyCharsetCheck->isChecked() );
}

QString ComposerPage::HeadersTab::helpAnchor() const
{
    return QString::fromLatin1("configure-composer-headers");
}

ComposerPageHeadersTab::ComposerPageHeadersTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    // tmp. vars:
    QVBoxLayout *vlay;
    QHBoxLayout *hlay;
    QGridLayout *glay;
    QLabel      *label;
    QPushButton *button;

    vlay = new QVBoxLayout( this );
    vlay->setSpacing( KDialog::spacingHint() );
    vlay->setMargin( KDialog::marginHint() );

    // "Use custom Message-Id suffix" checkbox:
    mCreateOwnMessageIdCheck =
            new QCheckBox( i18n("&Use custom message-id suffix"), this );
    connect( mCreateOwnMessageIdCheck, SIGNAL (stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    vlay->addWidget( mCreateOwnMessageIdCheck );

    // "Message-Id suffix" line edit and label:
    hlay = new QHBoxLayout(); // inherits spacing
    vlay->addLayout( hlay );
    mMessageIdSuffixEdit = new KLineEdit( this );
    mMessageIdSuffixEdit->setClearButtonShown( true );
    // only ASCII letters, digits, plus, minus and dots are allowed
    QRegExpValidator *messageIdSuffixValidator =
            new QRegExpValidator( QRegExp( QLatin1String("[a-zA-Z0-9+-]+(?:\\.[a-zA-Z0-9+-]+)*") ), this );
    mMessageIdSuffixEdit->setValidator( messageIdSuffixValidator );
    label = new QLabel(i18n("Custom message-&id suffix:"), this );
    label->setBuddy( mMessageIdSuffixEdit );
    label->setEnabled( false ); // since !mCreateOwnMessageIdCheck->isChecked()
    mMessageIdSuffixEdit->setEnabled( false );
    hlay->addWidget( label );
    hlay->addWidget( mMessageIdSuffixEdit, 1 );
    connect( mCreateOwnMessageIdCheck, SIGNAL(toggled(bool)),
             label, SLOT(setEnabled(bool)) );
    connect( mCreateOwnMessageIdCheck, SIGNAL(toggled(bool)),
             mMessageIdSuffixEdit, SLOT(setEnabled(bool)) );
    connect( mMessageIdSuffixEdit, SIGNAL(textChanged(QString)),
             this, SLOT(slotEmitChanged()) );

    // horizontal rule and "custom header fields" label:
    vlay->addWidget( new KSeparator( Qt::Horizontal, this ) );
    vlay->addWidget( new QLabel( i18n("Define custom mime header fields:"), this) );

    // "custom header fields" listbox:
    glay = new QGridLayout(); // inherits spacing
    vlay->addLayout( glay );
    glay->setRowStretch( 2, 1 );
    glay->setColumnStretch( 1, 1 );
    mHeaderList = new ListView( this );
    mHeaderList->setHeaderLabels( QStringList() << i18nc("@title:column Name of the mime header.","Name")
                                  << i18nc("@title:column Value of the mimeheader.","Value") );
    mHeaderList->setSortingEnabled( false );
    connect( mHeaderList, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
             this, SLOT(slotMimeHeaderSelectionChanged()) );
    connect( mHeaderList, SIGNAL(addHeader()), SLOT(slotNewMimeHeader()));
    connect( mHeaderList, SIGNAL(removeHeader()), SLOT(slotRemoveMimeHeader()));
    glay->addWidget( mHeaderList, 0, 0, 3, 2 );

    // "new" and "remove" buttons:
    button = new QPushButton( i18nc("@action:button Add new mime header field.","Ne&w"), this );
    connect( button, SIGNAL(clicked()), this, SLOT(slotNewMimeHeader()) );
    button->setAutoDefault( false );
    glay->addWidget( button, 0, 2 );
    mRemoveHeaderButton = new QPushButton( i18n("Re&move"), this );
    connect( mRemoveHeaderButton, SIGNAL(clicked()),
             this, SLOT(slotRemoveMimeHeader()) );
    button->setAutoDefault( false );
    glay->addWidget( mRemoveHeaderButton, 1, 2 );

    // "name" and "value" line edits and labels:
    mTagNameEdit = new KLineEdit( this );
    mTagNameEdit->setClearButtonShown(true);
    mTagNameEdit->setEnabled( false );
    mTagNameLabel = new QLabel( i18nc("@label:textbox Name of the mime header.","&Name:"), this );
    mTagNameLabel->setBuddy( mTagNameEdit );
    mTagNameLabel->setEnabled( false );
    glay->addWidget( mTagNameLabel, 3, 0 );
    glay->addWidget( mTagNameEdit, 3, 1 );
    connect( mTagNameEdit, SIGNAL(textChanged(QString)),
             this, SLOT(slotMimeHeaderNameChanged(QString)) );

    mTagValueEdit = new KLineEdit( this );
    mTagValueEdit->setClearButtonShown(true);
    mTagValueEdit->setEnabled( false );
    mTagValueLabel = new QLabel( i18n("&Value:"), this );
    mTagValueLabel->setBuddy( mTagValueEdit );
    mTagValueLabel->setEnabled( false );
    glay->addWidget( mTagValueLabel, 4, 0 );
    glay->addWidget( mTagValueEdit, 4, 1 );
    connect( mTagValueEdit, SIGNAL(textChanged(QString)),
             this, SLOT(slotMimeHeaderValueChanged(QString)) );
}

void ComposerPage::HeadersTab::slotMimeHeaderSelectionChanged()
{
    mEmitChanges = false;
    QTreeWidgetItem * item = mHeaderList->currentItem();

    if ( item ) {
        mTagNameEdit->setText( item->text( 0 ) );
        mTagValueEdit->setText( item->text( 1 ) );
    } else {
        mTagNameEdit->clear();
        mTagValueEdit->clear();
    }
    mRemoveHeaderButton->setEnabled( item );
    mTagNameEdit->setEnabled( item );
    mTagValueEdit->setEnabled( item );
    mTagNameLabel->setEnabled( item );
    mTagValueLabel->setEnabled( item );
    mEmitChanges = true;
}


void ComposerPage::HeadersTab::slotMimeHeaderNameChanged( const QString & text )
{
    // is called on ::setup(), when clearing the line edits. So be
    // prepared to not find a selection:
    QTreeWidgetItem * item = mHeaderList->currentItem();
    if ( item )
        item->setText( 0, text );
    slotEmitChanged();
}


void ComposerPage::HeadersTab::slotMimeHeaderValueChanged( const QString & text )
{
    // is called on ::setup(), when clearing the line edits. So be
    // prepared to not find a selection:
    QTreeWidgetItem * item = mHeaderList->currentItem();
    if ( item )
        item->setText( 1, text );
    slotEmitChanged();
}


void ComposerPage::HeadersTab::slotNewMimeHeader()
{
    QTreeWidgetItem *listItem = new QTreeWidgetItem( mHeaderList );
    mHeaderList->setCurrentItem( listItem );
    slotEmitChanged();
}


void ComposerPage::HeadersTab::slotRemoveMimeHeader()
{
    // calling this w/o selection is a programming error:
    QTreeWidgetItem *item = mHeaderList->currentItem();
    if ( !item ) {
        kDebug() << "=================================================="
                 << "Error: Remove button was pressed although no custom header was selected\n"
                 << "==================================================\n";
        return;
    }

    QTreeWidgetItem *below = mHeaderList->itemBelow( item );

    if ( below ) {
        kDebug() << "below";
        mHeaderList->setCurrentItem( below );
        delete item;
        item = 0;
    } else if ( mHeaderList->topLevelItemCount() > 0 ) {
        delete item;
        item = 0;
        mHeaderList->setCurrentItem(
                    mHeaderList->topLevelItem( mHeaderList->topLevelItemCount() - 1 )
                    );
    }

    slotEmitChanged();
}

void ComposerPage::HeadersTab::doLoadOther()
{
    mMessageIdSuffixEdit->setText( MessageComposer::MessageComposerSettings::customMsgIDSuffix() );
    const bool state = ( !MessageComposer::MessageComposerSettings::customMsgIDSuffix().isEmpty() &&
                         MessageComposer::MessageComposerSettings::useCustomMessageIdSuffix() );
    mCreateOwnMessageIdCheck->setChecked( state );

    mHeaderList->clear();
    mTagNameEdit->clear();
    mTagValueEdit->clear();

    QTreeWidgetItem * item = 0;

    const int count = GlobalSettings::self()->customMessageHeadersCount();
    for ( int i = 0 ; i < count ; ++i ) {
        KConfigGroup config( KMKernel::self()->config(),
                             QLatin1String("Mime #") + QString::number(i) );
        const QString name  = config.readEntry( "name" );
        const QString value = config.readEntry( "value" );
        if( !name.isEmpty() ) {
            item = new QTreeWidgetItem( mHeaderList, item );
            item->setText( 0, name );
            item->setText( 1, value );
        }
    }
    if ( mHeaderList->topLevelItemCount() > 0 ) {
        mHeaderList->setCurrentItem( mHeaderList->topLevelItem( 0 ) );
    }
    else {
        // disable the "Remove" button
        mRemoveHeaderButton->setEnabled( false );
    }
}

void ComposerPage::HeadersTab::save()
{
    MessageComposer::MessageComposerSettings::self()->setCustomMsgIDSuffix( mMessageIdSuffixEdit->text() );
    MessageComposer::MessageComposerSettings::self()->setUseCustomMessageIdSuffix( mCreateOwnMessageIdCheck->isChecked() );

    //Clean config
    const int oldHeadersCount = GlobalSettings::self()->customMessageHeadersCount();
    for ( int i = 0; i < oldHeadersCount; ++i ) {
        const QString groupMimeName = QString::fromLatin1( "Mime #%1" ).arg( i );
        if ( KMKernel::self()->config()->hasGroup( groupMimeName ) ) {
            KConfigGroup config( KMKernel::self()->config(), groupMimeName);
            config.deleteGroup();
        }
    }


    int numValidEntries = 0;
    QTreeWidgetItem *item = 0;
    const int numberOfEntry = mHeaderList->topLevelItemCount();
    for ( int i = 0; i < numberOfEntry; ++i ) {
        item = mHeaderList->topLevelItem( i );
        if( !item->text(0).isEmpty() ) {
            KConfigGroup config( KMKernel::self()->config(), QString::fromLatin1("Mime #%1").arg( numValidEntries ) );
            config.writeEntry( "name",  item->text( 0 ) );
            config.writeEntry( "value", item->text( 1 ) );
            numValidEntries++;
        }
    }
    GlobalSettings::self()->setCustomMessageHeadersCount( numValidEntries );
}

void ComposerPage::HeadersTab::doResetToDefaultsOther()
{
    const bool bUseDefaults = MessageComposer::MessageComposerSettings::self()->useDefaults( true );
    const QString messageIdSuffix = MessageComposer::MessageComposerSettings::customMsgIDSuffix();
    const bool useCustomMessageIdSuffix = MessageComposer::MessageComposerSettings::useCustomMessageIdSuffix();
    MessageComposer::MessageComposerSettings::self()->useDefaults( bUseDefaults );

    mMessageIdSuffixEdit->setText( messageIdSuffix );
    const bool state = ( !messageIdSuffix.isEmpty() && useCustomMessageIdSuffix );
    mCreateOwnMessageIdCheck->setChecked( state );

    mHeaderList->clear();
    mTagNameEdit->clear();
    mTagValueEdit->clear();
    // disable the "Remove" button
    mRemoveHeaderButton->setEnabled( false );
}

QString ComposerPage::AttachmentsTab::helpAnchor() const
{
    return QString::fromLatin1("configure-composer-attachments");
}

ComposerPageAttachmentsTab::ComposerPageAttachmentsTab( QWidget * parent )
    : ConfigModuleTab( parent ) {
    // tmp. vars:
    QVBoxLayout *vlay;
    QLabel      *label;

    vlay = new QVBoxLayout( this );
    vlay->setSpacing( KDialog::spacingHint() );
    vlay->setMargin( KDialog::marginHint() );

    // "Outlook compatible attachment naming" check box
    mOutlookCompatibleCheck =
            new QCheckBox( i18n( "Outlook-compatible attachment naming" ), this );
    mOutlookCompatibleCheck->setChecked( false );
    mOutlookCompatibleCheck->setToolTip( i18n(
                                             "Turn this option on to make Outlook(tm) understand attachment names "
                                             "containing non-English characters" ) );
    connect( mOutlookCompatibleCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mOutlookCompatibleCheck, SIGNAL(clicked()),
             this, SLOT(slotOutlookCompatibleClicked()) );
    vlay->addWidget( mOutlookCompatibleCheck );
    vlay->addSpacing( 5 );

    // "Enable detection of missing attachments" check box
    mMissingAttachmentDetectionCheck =
            new QCheckBox( i18n("E&nable detection of missing attachments"), this );
    mMissingAttachmentDetectionCheck->setChecked( true );
    connect( mMissingAttachmentDetectionCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    vlay->addWidget( mMissingAttachmentDetectionCheck );


    QHBoxLayout * layAttachment = new QHBoxLayout;
    label = new QLabel( i18n("Warn when inserting attachments larger than:"), this );
    label->setAlignment( Qt::AlignLeft );
    layAttachment->addWidget(label);

    mMaximumAttachmentSize = new KIntNumInput( this );
    mMaximumAttachmentSize->setRange( -1, 99999 );
    mMaximumAttachmentSize->setSingleStep( 100 );
    mMaximumAttachmentSize->setSuffix(i18nc("spinbox suffix: unit for kilobyte", " kB"));
    connect( mMaximumAttachmentSize, SIGNAL(valueChanged(int)),
             this, SLOT(slotEmitChanged()) );
    mMaximumAttachmentSize->setSpecialValueText(i18n("No limit"));
    layAttachment->addWidget(mMaximumAttachmentSize);
    vlay->addLayout(layAttachment);


    // "Attachment key words" label and string list editor
    label = new QLabel( i18n("Recognize any of the following key words as "
                             "intention to attach a file:"), this );
    label->setAlignment( Qt::AlignLeft );
    label->setWordWrap( true );

    vlay->addWidget( label );

    PimCommon::SimpleStringListEditor::ButtonCode buttonCode =
            static_cast<PimCommon::SimpleStringListEditor::ButtonCode>( PimCommon::SimpleStringListEditor::Add | PimCommon::SimpleStringListEditor::Remove | PimCommon::SimpleStringListEditor::Modify );
    mAttachWordsListEditor =
            new PimCommon::SimpleStringListEditor( this, buttonCode,
                                                   i18n("A&dd..."), i18n("Re&move"),
                                                   i18n("Mod&ify..."),
                                                   i18n("Enter new key word:") );
    connect( mAttachWordsListEditor, SIGNAL(changed()),
             this, SLOT(slotEmitChanged()) );
    vlay->addWidget( mAttachWordsListEditor );

    connect( mMissingAttachmentDetectionCheck, SIGNAL(toggled(bool)),
             label, SLOT(setEnabled(bool)) );
    connect( mMissingAttachmentDetectionCheck, SIGNAL(toggled(bool)),
             mAttachWordsListEditor, SLOT(setEnabled(bool)) );
}

void ComposerPage::AttachmentsTab::doLoadFromGlobalSettings()
{
    mOutlookCompatibleCheck->setChecked(
                MessageComposer::MessageComposerSettings::self()->outlookCompatibleAttachments() );
    mMissingAttachmentDetectionCheck->setChecked(
                GlobalSettings::self()->showForgottenAttachmentWarning() );

    const QStringList attachWordsList = GlobalSettings::self()->attachmentKeywords();
    mAttachWordsListEditor->setStringList( attachWordsList );
    const int maximumAttachmentSize(MessageComposer::MessageComposerSettings::self()->maximumAttachmentSize());
    mMaximumAttachmentSize->setValue(maximumAttachmentSize == -1 ? -1 : MessageComposer::MessageComposerSettings::self()->maximumAttachmentSize()/1024);
}

void ComposerPage::AttachmentsTab::save()
{
    MessageComposer::MessageComposerSettings::self()->setOutlookCompatibleAttachments(
                mOutlookCompatibleCheck->isChecked() );
    GlobalSettings::self()->setShowForgottenAttachmentWarning(
                mMissingAttachmentDetectionCheck->isChecked() );
    GlobalSettings::self()->setAttachmentKeywords(
                mAttachWordsListEditor->stringList() );

    KMime::setUseOutlookAttachmentEncoding( mOutlookCompatibleCheck->isChecked() );
    const int maximumAttachmentSize(mMaximumAttachmentSize->value());
    MessageComposer::MessageComposerSettings::self()->setMaximumAttachmentSize(maximumAttachmentSize == -1 ? -1 : maximumAttachmentSize*1024);

}

void ComposerPageAttachmentsTab::slotOutlookCompatibleClicked()
{
    if (mOutlookCompatibleCheck->isChecked()) {
        KMessageBox::information(0,i18n("You have chosen to "
                                        "encode attachment names containing non-English characters in a way that "
                                        "is understood by Outlook(tm) and other mail clients that do not "
                                        "support standard-compliant encoded attachment names.\n"
                                        "Note that KMail may create non-standard compliant messages, "
                                        "and consequently it is possible that your messages will not be "
                                        "understood by standard-compliant mail clients; so, unless you have no "
                                        "other choice, you should not enable this option." ) );
    }
}

ComposerPageAutoCorrectionTab::ComposerPageAutoCorrectionTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    QVBoxLayout *vlay = new QVBoxLayout( this );
    vlay->setSpacing( 0 );
    vlay->setMargin( 0 );
    autocorrectionWidget = new MessageComposer::ComposerAutoCorrectionWidget(this);
    if(KMKernel::self())
        autocorrectionWidget->setAutoCorrection(KMKernel::self()->composerAutoCorrection());
    vlay->addWidget(autocorrectionWidget);
    setLayout(vlay);
    connect( autocorrectionWidget, SIGNAL(changed()), this, SLOT(slotEmitChanged()) );

}

QString ComposerPageAutoCorrectionTab::helpAnchor() const
{
    return QString::fromLatin1("configure-autocorrection");
}

void ComposerPageAutoCorrectionTab::save()
{
    autocorrectionWidget->writeConfig();
}

void ComposerPageAutoCorrectionTab::doLoadFromGlobalSettings()
{
    autocorrectionWidget->loadConfig();
}

void ComposerPageAutoCorrectionTab::doResetToDefaultsOther()
{
    autocorrectionWidget->resetToDefault();
}


ComposerPageAutoImageResizeTab::ComposerPageAutoImageResizeTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    QVBoxLayout *vlay = new QVBoxLayout( this );
    vlay->setSpacing( 0 );
    vlay->setMargin( 0 );
    autoResizeWidget = new MessageComposer::ImageScalingWidget(this);
    vlay->addWidget(autoResizeWidget);
    setLayout(vlay);
    connect( autoResizeWidget, SIGNAL(changed()), this, SLOT(slotEmitChanged()) );

}

QString ComposerPageAutoImageResizeTab::helpAnchor() const
{
    return QString::fromLatin1("configure-image-resize");
}

void ComposerPageAutoImageResizeTab::save()
{
    autoResizeWidget->writeConfig();
}

void ComposerPageAutoImageResizeTab::doLoadFromGlobalSettings()
{
    autoResizeWidget->loadConfig();
}

void ComposerPageAutoImageResizeTab::doResetToDefaultsOther()
{
    autoResizeWidget->resetToDefault();
}


// *************************************************************
// *                                                           *
// *                      SecurityPage                         *
// *                                                           *
// *************************************************************
QString SecurityPage::helpAnchor() const
{
    return QString::fromLatin1("configure-security");
}

SecurityPage::SecurityPage( const KComponentData &instance, QWidget *parent )
    : ConfigModuleWithTabs( instance, parent )
{
    //
    // "Reading" tab:
    //
    mGeneralTab = new GeneralTab(); //  @TODO: rename
    addTab( mGeneralTab, i18n("Reading") );

    addTab( new MDNTab(), i18n("Message Disposition Notifications") );

    //
    // "Composing" tab:
    //
    mComposerCryptoTab = new ComposerCryptoTab();
    addTab( mComposerCryptoTab, i18n("Composing") );

    //
    // "Warnings" tab:
    //
    mWarningTab = new WarningTab();
    addTab( mWarningTab, i18n("Miscellaneous") );

    //
    // "S/MIME Validation" tab:
    //
    mSMimeTab = new SMimeTab();
    addTab( mSMimeTab, i18n("S/MIME Validation") );

    mSAdBlockTab = new SecurityPageAdBlockTab;
    addTab( mSAdBlockTab, i18n("Ad block") );
}

QString SecurityPage::GeneralTab::helpAnchor() const
{
    return QString::fromLatin1("configure-security-reading");
}

SecurityPageGeneralTab::SecurityPageGeneralTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    mSGTab.setupUi( this );

    connect( mSGTab.mHtmlMailCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mSGTab.mExternalReferences, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    connect(mSGTab.labelWarnHTML, SIGNAL(linkActivated(QString)),
            SLOT(slotLinkClicked(QString)));

    connect( mSGTab.mAlwaysDecrypt, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );

    connect( mSGTab.mAutomaticallyImportAttachedKeysCheck, SIGNAL(toggled(bool)),
             SLOT(slotEmitChanged()) );

    connect( mSGTab.mScamDetection, SIGNAL(toggled(bool)),
             SLOT(slotEmitChanged()) );

    connect( mSGTab.scamWhiteList, SIGNAL(changed()),
             SLOT(slotEmitChanged()) );
}

void SecurityPageGeneralTab::slotLinkClicked( const QString & link )
{
    if ( link == QLatin1String( "whatsthis1" ) )
        QWhatsThis::showText( QCursor::pos(), mSGTab.mHtmlMailCheck->whatsThis() );
    else if (link == QLatin1String( "whatsthis2" ) )
        QWhatsThis::showText( QCursor::pos(), mSGTab.mExternalReferences->whatsThis() );
}

void SecurityPage::GeneralTab::doLoadOther()
{
    mSGTab.mHtmlMailCheck->setChecked( MessageViewer::GlobalSettings::self()->htmlMail() );
    mSGTab.mExternalReferences->setChecked( MessageViewer::GlobalSettings::self()->htmlLoadExternal() );
    mSGTab.mAutomaticallyImportAttachedKeysCheck->setChecked(
                MessageViewer::GlobalSettings::self()->autoImportKeys() );

    mSGTab.mAlwaysDecrypt->setChecked( MessageViewer::GlobalSettings::self()->alwaysDecrypt() );

    mSGTab.mScamDetection->setChecked( MessageViewer::GlobalSettings::self()->scamDetectionEnabled());
    mSGTab.scamWhiteList->setStringList( MessageViewer::GlobalSettings::self()->scamDetectionWhiteList() );
}

void SecurityPage::GeneralTab::save()
{
    if ( MessageViewer::GlobalSettings::self()->htmlMail() != mSGTab.mHtmlMailCheck->isChecked())
    {
        if (KMessageBox::warningContinueCancel(this, i18n("Changing the global "
                                                          "HTML setting will override all folder specific values."), QString(),
                                               KStandardGuiItem::cont(), KStandardGuiItem::cancel(), QLatin1String("htmlMailOverride")) == KMessageBox::Continue)
        {
            MessageViewer::GlobalSettings::self()->setHtmlMail( mSGTab.mHtmlMailCheck->isChecked() );
            foreach( const Akonadi::Collection &collection, kmkernel->allFolders() ) {
                KConfigGroup config( KMKernel::self()->config(), MailCommon::FolderCollection::configGroupName(collection) );
                config.writeEntry("htmlMailOverride", false);
            }
        }
    }
    MessageViewer::GlobalSettings::self()->setHtmlLoadExternal( mSGTab.mExternalReferences->isChecked() );
    MessageViewer::GlobalSettings::self()->setAutoImportKeys(
                mSGTab.mAutomaticallyImportAttachedKeysCheck->isChecked() );
    MessageViewer::GlobalSettings::self()->setAlwaysDecrypt( mSGTab.mAlwaysDecrypt->isChecked() );
    MessageViewer::GlobalSettings::self()->setScamDetectionEnabled( mSGTab.mScamDetection->isChecked() );
    MessageViewer::GlobalSettings::self()->setScamDetectionWhiteList( mSGTab.scamWhiteList->stringList() );
}

//Adblock

QString SecurityPageAdBlockTab::helpAnchor() const
{
    return QString();
}

SecurityPageAdBlockTab::SecurityPageAdBlockTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    QHBoxLayout *lay = new QHBoxLayout;
    lay->setMargin(0);

    mWidget = new MessageViewer::AdBlockSettingWidget;
    lay->addWidget(mWidget);
    connect(mWidget, SIGNAL(changed(bool)), SLOT(slotEmitChanged()));
    setLayout(lay);
}

SecurityPageAdBlockTab::~SecurityPageAdBlockTab()
{
}

void SecurityPageAdBlockTab::save()
{
    mWidget->save();
}

void SecurityPageAdBlockTab::doLoadFromGlobalSettings()
{
    mWidget->doLoadFromGlobalSettings();
}

void SecurityPageAdBlockTab::doLoadOther()
{

}

void SecurityPageAdBlockTab::doResetToDefaultsOther()
{
    mWidget->doResetToDefaultsOther();
}


QString SecurityPage::MDNTab::helpAnchor() const
{
    return QString::fromLatin1("configure-security-mdn");
}

SecurityPageMDNTab::SecurityPageMDNTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    mUi.setupUi( this );

    // "ignore", "ask", "deny", "always send" radiobuttons
    mMDNGroup = new QButtonGroup( this );
    connect( mMDNGroup, SIGNAL(buttonClicked(int)),
             this, SLOT(slotEmitChanged()) );
    mMDNGroup->addButton( mUi.radioIgnore, 0 );
    mMDNGroup->addButton( mUi.radioAsk, 1 );
    mMDNGroup->addButton( mUi.radioDeny, 2 );
    mMDNGroup->addButton( mUi.radioAlways, 3 );

    // "Original Message quote" radiobuttons
    mOrigQuoteGroup = new QButtonGroup( this );
    connect( mOrigQuoteGroup, SIGNAL(buttonClicked(int)),
             this, SLOT(slotEmitChanged()) );
    mOrigQuoteGroup->addButton( mUi.radioNothing, 0 );
    mOrigQuoteGroup->addButton( mUi.radioFull, 1 );
    mOrigQuoteGroup->addButton( mUi.radioHeaders, 2 );

    connect( mUi.mNoMDNsWhenEncryptedCheck, SIGNAL(toggled(bool)), SLOT(slotEmitChanged()) );
    connect( mUi.labelWarning, SIGNAL(linkActivated(QString)),
             SLOT(slotLinkClicked(QString)) );
}

void SecurityPageMDNTab::slotLinkClicked( const QString & link )
{
    if ( link == QLatin1String( "whatsthis-mdn" ) )
        QWhatsThis::showText( QCursor::pos(), mUi.radioIgnore->whatsThis() );
}

void SecurityPage::MDNTab::doLoadOther()
{
    int num = MessageViewer::GlobalSettings::self()->defaultPolicy();
    if ( num < 0 || num >= mMDNGroup->buttons().count() ) num = 0;
    mMDNGroup->button(num)->setChecked(true);
    num = MessageViewer::GlobalSettings::self()->quoteMessage();
    if ( num < 0 || num >= mOrigQuoteGroup->buttons().count() ) num = 0;
    mOrigQuoteGroup->button(num)->setChecked(true);
    mUi.mNoMDNsWhenEncryptedCheck->setChecked( MessageViewer::GlobalSettings::self()->notSendWhenEncrypted() );
}

void SecurityPage::MDNTab::save()
{
    MessageViewer::GlobalSettings::self()->setDefaultPolicy( mMDNGroup->checkedId() );
    MessageViewer::GlobalSettings::self()->setQuoteMessage( mOrigQuoteGroup->checkedId() );
    MessageViewer::GlobalSettings::self()->setNotSendWhenEncrypted( mUi.mNoMDNsWhenEncryptedCheck->isChecked() );
}

QString SecurityPage::ComposerCryptoTab::helpAnchor() const
{
    return QString::fromLatin1("configure-security-composing");
}

SecurityPageComposerCryptoTab::SecurityPageComposerCryptoTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    mWidget = new Ui::ComposerCryptoConfiguration;
    mWidget->setupUi( this );
    connect( mWidget->mEncToSelf, SIGNAL(toggled(bool)), this, SLOT(slotEmitChanged()) );
    connect( mWidget->mShowEncryptionResult, SIGNAL(toggled(bool)), this, SLOT(slotEmitChanged()) );
    connect( mWidget->mShowKeyApprovalDlg, SIGNAL(toggled(bool)), this, SLOT(slotEmitChanged()) );
    connect( mWidget->mAutoEncrypt, SIGNAL(toggled(bool)), this, SLOT(slotEmitChanged()) );
    connect( mWidget->mNeverEncryptWhenSavingInDrafts, SIGNAL(toggled(bool)), this, SLOT(slotEmitChanged()) );
    connect( mWidget->mStoreEncrypted, SIGNAL(toggled(bool)), this, SLOT(slotEmitChanged()) );
}

SecurityPageComposerCryptoTab::~SecurityPageComposerCryptoTab()
{
    delete mWidget;
}

void SecurityPage::ComposerCryptoTab::doLoadOther()
{
    // If you change default values, sync messagecomposer.cpp too

    mWidget->mEncToSelf->setChecked( MessageComposer::MessageComposerSettings::self()->cryptoEncryptToSelf() );
    mWidget->mShowEncryptionResult->setChecked( false ); //composer.readBoolEntry( "crypto-show-encryption-result", true ) );
    mWidget->mShowEncryptionResult->hide();
    mWidget->mShowKeyApprovalDlg->setChecked( MessageComposer::MessageComposerSettings::self()->cryptoShowKeysForApproval() );

    mWidget->mAutoEncrypt->setChecked( MessageComposer::MessageComposerSettings::self()->pgpAutoEncrypt() ) ;
    mWidget->mNeverEncryptWhenSavingInDrafts->setChecked(
                GlobalSettings::self()->neverEncryptDrafts() );

    mWidget->mStoreEncrypted->setChecked( GlobalSettings::self()->cryptoStoreEncrypted() );
}

void SecurityPage::ComposerCryptoTab::save()
{
    MessageComposer::MessageComposerSettings::self()->setCryptoEncryptToSelf( mWidget->mEncToSelf->isChecked() );
    GlobalSettings::self()->setCryptoShowEncryptionResult( mWidget->mShowEncryptionResult->isChecked() );
    MessageComposer::MessageComposerSettings::self()->setCryptoShowKeysForApproval( mWidget->mShowKeyApprovalDlg->isChecked() );

    MessageComposer::MessageComposerSettings::self()->setPgpAutoEncrypt( mWidget->mAutoEncrypt->isChecked() );
    GlobalSettings::self()->setNeverEncryptDrafts( mWidget->mNeverEncryptWhenSavingInDrafts->isChecked() );

    GlobalSettings::self()->setCryptoStoreEncrypted( mWidget->mStoreEncrypted->isChecked() );
}

void SecurityPage::ComposerCryptoTab::doLoadFromGlobalSettings()
{
    mWidget->mEncToSelf->setChecked( MessageComposer::MessageComposerSettings::self()->cryptoEncryptToSelf() );
    mWidget->mShowEncryptionResult->setChecked( GlobalSettings::self()->cryptoShowEncryptionResult() );
    mWidget->mShowKeyApprovalDlg->setChecked(MessageComposer::MessageComposerSettings::self()->cryptoShowKeysForApproval() );

    mWidget->mAutoEncrypt->setChecked(MessageComposer::MessageComposerSettings::self()->pgpAutoEncrypt() );
    mWidget->mNeverEncryptWhenSavingInDrafts->setChecked( GlobalSettings::self()->neverEncryptDrafts() );

    mWidget->mStoreEncrypted->setChecked(GlobalSettings::self()->cryptoStoreEncrypted() );

}


QString SecurityPage::WarningTab::helpAnchor() const
{
    return QString::fromLatin1("configure-security-warnings");
}

SecurityPageWarningTab::SecurityPageWarningTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    mWidget = new Ui::WarningConfiguration;
    mWidget->setupUi( this );

    mWidget->chiasmusButton->hide();

    connect( mWidget->warnGroupBox, SIGNAL(toggled(bool)), SLOT(slotEmitChanged()) );
    connect( mWidget->mWarnUnsigned, SIGNAL(toggled(bool)), SLOT(slotEmitChanged()) );
    connect( mWidget->warnUnencryptedCB, SIGNAL(toggled(bool)), SLOT(slotEmitChanged()) );
    connect( mWidget->warnReceiverNotInCertificateCB, SIGNAL(toggled(bool)), SLOT(slotEmitChanged()) );

    connect( mWidget->gnupgButton, SIGNAL(clicked()), SLOT(slotConfigureGnupg()) );
    //connect( mWidget->chiasmusButton, SIGNAL(clicked()), SLOT(slotConfigureChiasmus()) );
    connect( mWidget->enableAllWarningsPB, SIGNAL(clicked()), SLOT(slotReenableAllWarningsClicked()) );
}

SecurityPageWarningTab::~SecurityPageWarningTab()
{
    delete mWidget;
}

void SecurityPage::WarningTab::doLoadFromGlobalSettings()
{
    mWidget->warnUnencryptedCB->setChecked(
                MessageComposer::MessageComposerSettings::self()->cryptoWarningUnencrypted() );
    mWidget->mWarnUnsigned->setChecked(
                MessageComposer::MessageComposerSettings::self()->cryptoWarningUnsigned() );
    mWidget->warnReceiverNotInCertificateCB->setChecked(
                MessageComposer::MessageComposerSettings::self()->cryptoWarnRecvNotInCert() );

    // The "-int" part of the key name is because there used to be a separate boolean
    // config entry for enabling/disabling. This is done with the single bool value now.
    mWidget->warnGroupBox->setChecked(
                MessageComposer::MessageComposerSettings::self()->cryptoWarnWhenNearExpire() );
    mWidget->mWarnSignKeyExpiresSB->setValue(
                MessageComposer::MessageComposerSettings::self()->cryptoWarnSignKeyNearExpiryThresholdDays() );
    mWidget->mWarnSignChainCertExpiresSB->setValue(
                MessageComposer::MessageComposerSettings::self()->cryptoWarnSignChaincertNearExpiryThresholdDays() );
    mWidget->mWarnSignRootCertExpiresSB->setValue(
                MessageComposer::MessageComposerSettings::self()->cryptoWarnSignRootNearExpiryThresholdDays() );

    mWidget->mWarnEncrKeyExpiresSB->setValue(
                MessageComposer::MessageComposerSettings::self()->cryptoWarnEncrKeyNearExpiryThresholdDays() );
    mWidget->mWarnEncrChainCertExpiresSB->setValue(
                MessageComposer::MessageComposerSettings::self()->cryptoWarnEncrChaincertNearExpiryThresholdDays() );
    mWidget->mWarnEncrRootCertExpiresSB->setValue(
                MessageComposer::MessageComposerSettings::self()->cryptoWarnEncrRootNearExpiryThresholdDays() );
}

void SecurityPage::WarningTab::doLoadOther()
{
    mWidget->warnUnencryptedCB->setChecked(
                MessageComposer::MessageComposerSettings::self()->cryptoWarningUnencrypted() );
    mWidget->mWarnUnsigned->setChecked(
                MessageComposer::MessageComposerSettings::self()->cryptoWarningUnsigned() );
    mWidget->warnReceiverNotInCertificateCB->setChecked(
                MessageComposer::MessageComposerSettings::self()->cryptoWarnRecvNotInCert() );

    // The "-int" part of the key name is because there used to be a separate boolean
    // config entry for enabling/disabling. This is done with the single bool value now.
    mWidget->warnGroupBox->setChecked(
                MessageComposer::MessageComposerSettings::self()->cryptoWarnWhenNearExpire() );

    mWidget->mWarnSignKeyExpiresSB->setValue(
                MessageComposer::MessageComposerSettings::self()->cryptoWarnSignKeyNearExpiryThresholdDays() );
    mWidget->mWarnSignKeyExpiresSB->setSuffix(ki18np(" day", " days"));
    mWidget->mWarnSignChainCertExpiresSB->setValue(
                MessageComposer::MessageComposerSettings::self()->cryptoWarnSignChaincertNearExpiryThresholdDays() );
    mWidget->mWarnSignChainCertExpiresSB->setSuffix(ki18np(" day", " days"));
    mWidget->mWarnSignRootCertExpiresSB->setValue(
                MessageComposer::MessageComposerSettings::self()->cryptoWarnSignRootNearExpiryThresholdDays() );
    mWidget->mWarnSignRootCertExpiresSB->setSuffix(ki18np(" day", " days"));

    mWidget->mWarnEncrKeyExpiresSB->setValue(
                MessageComposer::MessageComposerSettings::self()->cryptoWarnEncrKeyNearExpiryThresholdDays() );
    mWidget->mWarnEncrKeyExpiresSB->setSuffix(ki18np(" day", " days"));
    mWidget->mWarnEncrChainCertExpiresSB->setValue(
                MessageComposer::MessageComposerSettings::self()->cryptoWarnEncrChaincertNearExpiryThresholdDays() );
    mWidget->mWarnEncrChainCertExpiresSB->setSuffix(ki18np(" day", " days"));
    mWidget->mWarnEncrRootCertExpiresSB->setValue(
                MessageComposer::MessageComposerSettings::self()->cryptoWarnEncrRootNearExpiryThresholdDays() );
    mWidget->mWarnEncrRootCertExpiresSB->setSuffix(ki18np(" day", " days"));

    mWidget->enableAllWarningsPB->setEnabled( true );
}

void SecurityPage::WarningTab::save()
{
    MessageComposer::MessageComposerSettings::self()->setCryptoWarnRecvNotInCert(
                mWidget->warnReceiverNotInCertificateCB->isChecked() );
    MessageComposer::MessageComposerSettings::self()->setCryptoWarningUnencrypted(
                mWidget->warnUnencryptedCB->isChecked() );
    MessageComposer::MessageComposerSettings::self()->setCryptoWarningUnsigned(
                mWidget->mWarnUnsigned->isChecked() );

    MessageComposer::MessageComposerSettings::self()->setCryptoWarnWhenNearExpire(
                mWidget->warnGroupBox->isChecked() );
    MessageComposer::MessageComposerSettings::self()->setCryptoWarnSignKeyNearExpiryThresholdDays(
                mWidget->mWarnSignKeyExpiresSB->value() );
    MessageComposer::MessageComposerSettings::self()->setCryptoWarnSignChaincertNearExpiryThresholdDays(
                mWidget->mWarnSignChainCertExpiresSB->value() );
    MessageComposer::MessageComposerSettings::self()->setCryptoWarnSignRootNearExpiryThresholdDays(
                mWidget->mWarnSignRootCertExpiresSB->value() );

    MessageComposer::MessageComposerSettings::self()->setCryptoWarnEncrKeyNearExpiryThresholdDays(
                mWidget->mWarnEncrKeyExpiresSB->value() );
    MessageComposer::MessageComposerSettings::self()->setCryptoWarnEncrChaincertNearExpiryThresholdDays(
                mWidget->mWarnEncrChainCertExpiresSB->value() );
    MessageComposer::MessageComposerSettings::self()->setCryptoWarnEncrRootNearExpiryThresholdDays(
                mWidget->mWarnEncrRootCertExpiresSB->value() );
}

void SecurityPage::WarningTab::slotReenableAllWarningsClicked()
{
    KMessageBox::enableAllMessages();

    //Nepomuk composer.
    const QString groupName = KPIM::NepomukWarning::nepomukWarningGroupName();
    if ( KMKernel::self()->config()->hasGroup( groupName ) ) {
        KConfigGroup cfgGroup( KMKernel::self()->config(), groupName );
        cfgGroup.deleteGroup();
    }

    mWidget->enableAllWarningsPB->setEnabled( false );
}

void SecurityPage::WarningTab::slotConfigureGnupg()
{
    QPointer<KCMultiDialog> dlg( new KCMultiDialog( this ) );
    dlg->addModule( QLatin1String("kleopatra_config_gnupgsystem") );
    dlg->exec();
    delete dlg;
}

#if 0
void SecurityPage::WarningTab::slotConfigureChiasmus()
{
    using namespace Kleo;
    // Find Chiasmus backend:
    if ( const CryptoBackendFactory * const bf = Kleo::CryptoBackendFactory::instance() )
        for ( unsigned int i = 0 ; const CryptoBackend * const b = bf->backend( i ) ; ++i )
            if ( b->name() == QLatin1String( "Chiasmus" ) )
                if ( CryptoConfig * const c = b->config() ) {
                    QPointer<CryptoConfigDialog> dlg( new CryptoConfigDialog( c, this ) );
                    dlg->exec();
                    delete dlg;
                    break;
                } else {
                    kWarning() << "Found Chiasmus backend, but there doesn't seem to be a config object available from it.";
                }
            else
                kDebug() << "Skipping" << b->name() << "backend (not \"Chiasmus\")";
    else
        kDebug() << "Kleo::CryptoBackendFactory::instance() returned NULL!";
}
#endif
////

QString SecurityPage::SMimeTab::helpAnchor() const
{
    return QString::fromLatin1("configure-security-smime-validation");
}

SecurityPageSMimeTab::SecurityPageSMimeTab( QWidget * parent )
    : ConfigModuleTab( parent )
{

    mWidget = new Ui::SMimeConfiguration;
    mWidget->setupUi( this );

    // Button-group for exclusive radiobuttons
    QButtonGroup* bg = new QButtonGroup( this );
    bg->addButton( mWidget->CRLRB );
    bg->addButton( mWidget->OCSPRB );

    // Settings for the keyrequester custom widget
    mWidget->OCSPResponderSignature->setAllowedKeys(
                Kleo::KeySelectionDialog::SMIMEKeys
                | Kleo::KeySelectionDialog::TrustedKeys
                | Kleo::KeySelectionDialog::ValidKeys
                | Kleo::KeySelectionDialog::SigningKeys
                | Kleo::KeySelectionDialog::PublicKeys );
    mWidget->OCSPResponderSignature->setMultipleKeysEnabled( false );

    mConfig = Kleo::CryptoBackendFactory::instance()->config();

    connect( mWidget->CRLRB, SIGNAL(toggled(bool)), this, SLOT(slotEmitChanged()) );
    connect( mWidget->OCSPRB, SIGNAL(toggled(bool)), this, SLOT(slotEmitChanged()) );
    connect( mWidget->OCSPResponderURL, SIGNAL(textChanged(QString)), this, SLOT(slotEmitChanged()) );
    connect( mWidget->OCSPResponderSignature, SIGNAL(changed()), this, SLOT(slotEmitChanged()) );
    connect( mWidget->doNotCheckCertPolicyCB, SIGNAL(toggled(bool)), this, SLOT(slotEmitChanged()) );
    connect( mWidget->neverConsultCB, SIGNAL(toggled(bool)), this, SLOT(slotEmitChanged()) );
    connect( mWidget->fetchMissingCB, SIGNAL(toggled(bool)), this, SLOT(slotEmitChanged()) );

    connect( mWidget->ignoreServiceURLCB, SIGNAL(toggled(bool)), this, SLOT(slotEmitChanged()) );
    connect( mWidget->ignoreHTTPDPCB, SIGNAL(toggled(bool)), this, SLOT(slotEmitChanged()) );
    connect( mWidget->disableHTTPCB, SIGNAL(toggled(bool)), this, SLOT(slotEmitChanged()) );
    connect( mWidget->honorHTTPProxyRB, SIGNAL(toggled(bool)), this, SLOT(slotEmitChanged()) );
    connect( mWidget->useCustomHTTPProxyRB, SIGNAL(toggled(bool)), this, SLOT(slotEmitChanged()) );
    connect( mWidget->customHTTPProxy, SIGNAL(textChanged(QString)), this, SLOT(slotEmitChanged()) );
    connect( mWidget->ignoreLDAPDPCB, SIGNAL(toggled(bool)), this, SLOT(slotEmitChanged()) );
    connect( mWidget->disableLDAPCB, SIGNAL(toggled(bool)), this, SLOT(slotEmitChanged()) );
    connect( mWidget->customLDAPProxy, SIGNAL(textChanged(QString)), this, SLOT(slotEmitChanged()) );

    connect( mWidget->disableHTTPCB, SIGNAL(toggled(bool)),
             this, SLOT(slotUpdateHTTPActions()) );
    connect( mWidget->ignoreHTTPDPCB, SIGNAL(toggled(bool)),
             this, SLOT(slotUpdateHTTPActions()) );

    // Button-group for exclusive radiobuttons
    QButtonGroup* bgHTTPProxy = new QButtonGroup( this );
    bgHTTPProxy->addButton( mWidget->honorHTTPProxyRB );
    bgHTTPProxy->addButton( mWidget->useCustomHTTPProxyRB );

    QDBusConnection::sessionBus().connect(QString(), QString(), QLatin1String("org.kde.kleo.CryptoConfig"), QLatin1String("changed"),this, SLOT(load()) );
}

SecurityPageSMimeTab::~SecurityPageSMimeTab()
{
    delete mWidget;
}

static void disableDirmngrWidget( QWidget* w )
{
    w->setEnabled( false );
    w->setWhatsThis( i18n( "This option requires dirmngr >= 0.9.0" ) );
}

static void initializeDirmngrCheckbox( QCheckBox* cb, Kleo::CryptoConfigEntry* entry )
{
    if ( entry )
        cb->setChecked( entry->boolValue() );
    else
        disableDirmngrWidget( cb );
}

struct SMIMECryptoConfigEntries {
    SMIMECryptoConfigEntries( Kleo::CryptoConfig* config )
        : mConfig( config ) {

        // Checkboxes
        mCheckUsingOCSPConfigEntry = configEntry( QLatin1String("gpgsm"), QLatin1String("Security"), QLatin1String("enable-ocsp"), Kleo::CryptoConfigEntry::ArgType_None, false );
        mEnableOCSPsendingConfigEntry = configEntry( QLatin1String("dirmngr"), QLatin1String("OCSP"), QLatin1String("allow-ocsp"), Kleo::CryptoConfigEntry::ArgType_None, false );
        mDoNotCheckCertPolicyConfigEntry = configEntry( QLatin1String("gpgsm"), QLatin1String("Security"), QLatin1String("disable-policy-checks"), Kleo::CryptoConfigEntry::ArgType_None, false );
        mNeverConsultConfigEntry = configEntry( QLatin1String("gpgsm"), QLatin1String("Security"), QLatin1String("disable-crl-checks"), Kleo::CryptoConfigEntry::ArgType_None, false );
        mFetchMissingConfigEntry = configEntry( QLatin1String("gpgsm"), QLatin1String("Security"), QLatin1String("auto-issuer-key-retrieve"), Kleo::CryptoConfigEntry::ArgType_None, false );
        // dirmngr-0.9.0 options
        mIgnoreServiceURLEntry = configEntry( QLatin1String("dirmngr"), QLatin1String("OCSP"), QLatin1String("ignore-ocsp-service-url"), Kleo::CryptoConfigEntry::ArgType_None, false );
        mIgnoreHTTPDPEntry = configEntry( QLatin1String("dirmngr"), QLatin1String("HTTP"), QLatin1String("ignore-http-dp"), Kleo::CryptoConfigEntry::ArgType_None, false );
        mDisableHTTPEntry = configEntry(QLatin1String( "dirmngr"), QLatin1String("HTTP"), QLatin1String("disable-http"), Kleo::CryptoConfigEntry::ArgType_None, false );
        mHonorHTTPProxy = configEntry( QLatin1String("dirmngr"), QLatin1String("HTTP"), QLatin1String("honor-http-proxy"), Kleo::CryptoConfigEntry::ArgType_None, false );

        mIgnoreLDAPDPEntry = configEntry( QLatin1String("dirmngr"), QLatin1String("LDAP"), QLatin1String("ignore-ldap-dp"), Kleo::CryptoConfigEntry::ArgType_None, false );
        mDisableLDAPEntry = configEntry( QLatin1String("dirmngr"), QLatin1String("LDAP"), QLatin1String("disable-ldap"), Kleo::CryptoConfigEntry::ArgType_None, false );
        // Other widgets
        mOCSPResponderURLConfigEntry = configEntry( QLatin1String("dirmngr"), QLatin1String("OCSP"), QLatin1String("ocsp-responder"), Kleo::CryptoConfigEntry::ArgType_String, false );
        mOCSPResponderSignature = configEntry( QLatin1String("dirmngr"), QLatin1String("OCSP"), QLatin1String("ocsp-signer"), Kleo::CryptoConfigEntry::ArgType_String, false );
        mCustomHTTPProxy = configEntry( QLatin1String("dirmngr"), QLatin1String("HTTP"), QLatin1String("http-proxy"), Kleo::CryptoConfigEntry::ArgType_String, false );
        mCustomLDAPProxy = configEntry( QLatin1String("dirmngr"), QLatin1String("LDAP"), QLatin1String("ldap-proxy"), Kleo::CryptoConfigEntry::ArgType_String, false );
    }

    Kleo::CryptoConfigEntry* configEntry(const QString &componentName,
                                         const QString &groupName,
                                         const QString &entryName,
                                         int argType,
                                         bool isList );

    // Checkboxes
    Kleo::CryptoConfigEntry* mCheckUsingOCSPConfigEntry;
    Kleo::CryptoConfigEntry* mEnableOCSPsendingConfigEntry;
    Kleo::CryptoConfigEntry* mDoNotCheckCertPolicyConfigEntry;
    Kleo::CryptoConfigEntry* mNeverConsultConfigEntry;
    Kleo::CryptoConfigEntry* mFetchMissingConfigEntry;
    Kleo::CryptoConfigEntry* mIgnoreServiceURLEntry;
    Kleo::CryptoConfigEntry* mIgnoreHTTPDPEntry;
    Kleo::CryptoConfigEntry* mDisableHTTPEntry;
    Kleo::CryptoConfigEntry* mHonorHTTPProxy;
    Kleo::CryptoConfigEntry* mIgnoreLDAPDPEntry;
    Kleo::CryptoConfigEntry* mDisableLDAPEntry;
    // Other widgets
    Kleo::CryptoConfigEntry* mOCSPResponderURLConfigEntry;
    Kleo::CryptoConfigEntry* mOCSPResponderSignature;
    Kleo::CryptoConfigEntry* mCustomHTTPProxy;
    Kleo::CryptoConfigEntry* mCustomLDAPProxy;

    Kleo::CryptoConfig* mConfig;
};

void SecurityPage::SMimeTab::doLoadOther()
{
    if ( !mConfig ) {
        setEnabled( false );
        return;
    }

    // Force re-parsing gpgconf data, in case e.g. kleopatra or "configure backend" was used
    // (which ends up calling us via D-Bus)
    mConfig->clear();

    // Create config entries
    // Don't keep them around, they'll get deleted by clear(), which could be
    // done by the "configure backend" button even before we save().
    SMIMECryptoConfigEntries e( mConfig );

    // Initialize GUI items from the config entries

    if ( e.mCheckUsingOCSPConfigEntry ) {
        bool b = e.mCheckUsingOCSPConfigEntry->boolValue();
        mWidget->OCSPRB->setChecked( b );
        mWidget->CRLRB->setChecked( !b );
        mWidget->OCSPGroupBox->setEnabled( b );
    } else {
        mWidget->OCSPGroupBox->setEnabled( false );
    }
    if ( e.mDoNotCheckCertPolicyConfigEntry )
        mWidget->doNotCheckCertPolicyCB->setChecked( e.mDoNotCheckCertPolicyConfigEntry->boolValue() );
    if ( e.mNeverConsultConfigEntry )
        mWidget->neverConsultCB->setChecked( e.mNeverConsultConfigEntry->boolValue() );
    if ( e.mFetchMissingConfigEntry )
        mWidget->fetchMissingCB->setChecked( e.mFetchMissingConfigEntry->boolValue() );

    if ( e.mOCSPResponderURLConfigEntry )
        mWidget->OCSPResponderURL->setText( e.mOCSPResponderURLConfigEntry->stringValue() );
    if ( e.mOCSPResponderSignature ) {
        mWidget->OCSPResponderSignature->setFingerprint( e.mOCSPResponderSignature->stringValue() );
    }

    // dirmngr-0.9.0 options
    initializeDirmngrCheckbox( mWidget->ignoreServiceURLCB, e.mIgnoreServiceURLEntry );
    initializeDirmngrCheckbox( mWidget->ignoreHTTPDPCB, e.mIgnoreHTTPDPEntry );
    initializeDirmngrCheckbox( mWidget->disableHTTPCB, e.mDisableHTTPEntry );
    initializeDirmngrCheckbox( mWidget->ignoreLDAPDPCB, e.mIgnoreLDAPDPEntry );
    initializeDirmngrCheckbox( mWidget->disableLDAPCB, e.mDisableLDAPEntry );
    if ( e.mCustomHTTPProxy ) {
        QString systemProxy = QString::fromLocal8Bit( qgetenv( "http_proxy" ) );
        if ( systemProxy.isEmpty() )
            systemProxy = i18n( "no proxy" );
        mWidget->systemHTTPProxy->setText( i18n( "(Current system setting: %1)", systemProxy ) );
        bool honor = e.mHonorHTTPProxy && e.mHonorHTTPProxy->boolValue();
        mWidget->honorHTTPProxyRB->setChecked( honor );
        mWidget->useCustomHTTPProxyRB->setChecked( !honor );
        mWidget->customHTTPProxy->setText( e.mCustomHTTPProxy->stringValue() );
    } else {
        disableDirmngrWidget( mWidget->honorHTTPProxyRB );
        disableDirmngrWidget( mWidget->useCustomHTTPProxyRB );
        disableDirmngrWidget( mWidget->systemHTTPProxy );
        disableDirmngrWidget( mWidget->customHTTPProxy );
    }
    if ( e.mCustomLDAPProxy )
        mWidget->customLDAPProxy->setText( e.mCustomLDAPProxy->stringValue() );
    else {
        disableDirmngrWidget( mWidget->customLDAPProxy );
        disableDirmngrWidget( mWidget->customLDAPLabel );
    }
    slotUpdateHTTPActions();
}

void SecurityPage::SMimeTab::slotUpdateHTTPActions()
{
    mWidget->ignoreHTTPDPCB->setEnabled( !mWidget->disableHTTPCB->isChecked() );

    // The proxy settings only make sense when "Ignore HTTP CRL DPs of certificate" is checked.
    bool enableProxySettings = !mWidget->disableHTTPCB->isChecked()
            && mWidget->ignoreHTTPDPCB->isChecked();
    mWidget->systemHTTPProxy->setEnabled( enableProxySettings );
    mWidget->useCustomHTTPProxyRB->setEnabled( enableProxySettings );
    mWidget->honorHTTPProxyRB->setEnabled( enableProxySettings );
    mWidget->customHTTPProxy->setEnabled( enableProxySettings && mWidget->useCustomHTTPProxyRB->isChecked());

    if ( !mWidget->useCustomHTTPProxyRB->isChecked() &&
         !mWidget->honorHTTPProxyRB->isChecked() )
        mWidget->honorHTTPProxyRB->setChecked( true );
}

static void saveCheckBoxToKleoEntry( QCheckBox* cb, Kleo::CryptoConfigEntry* entry )
{
    const bool b = cb->isChecked();
    if ( entry && entry->boolValue() != b )
        entry->setBoolValue( b );
}

void SecurityPage::SMimeTab::save()
{
    if ( !mConfig ) {
        return;
    }
    // Create config entries
    // Don't keep them around, they'll get deleted by clear(), which could be done by the
    // "configure backend" button.
    SMIMECryptoConfigEntries e( mConfig );

    bool b = mWidget->OCSPRB->isChecked();
    if ( e.mCheckUsingOCSPConfigEntry && e.mCheckUsingOCSPConfigEntry->boolValue() != b )
        e.mCheckUsingOCSPConfigEntry->setBoolValue( b );
    // Set allow-ocsp together with enable-ocsp
    if ( e.mEnableOCSPsendingConfigEntry && e.mEnableOCSPsendingConfigEntry->boolValue() != b )
        e.mEnableOCSPsendingConfigEntry->setBoolValue( b );

    saveCheckBoxToKleoEntry( mWidget->doNotCheckCertPolicyCB, e.mDoNotCheckCertPolicyConfigEntry );
    saveCheckBoxToKleoEntry( mWidget->neverConsultCB, e.mNeverConsultConfigEntry );
    saveCheckBoxToKleoEntry( mWidget->fetchMissingCB, e.mFetchMissingConfigEntry );

    QString txt = mWidget->OCSPResponderURL->text();
    if ( e.mOCSPResponderURLConfigEntry && e.mOCSPResponderURLConfigEntry->stringValue() != txt )
        e.mOCSPResponderURLConfigEntry->setStringValue( txt );

    txt = mWidget->OCSPResponderSignature->fingerprint();
    if ( e.mOCSPResponderSignature && e.mOCSPResponderSignature->stringValue() != txt ) {
        e.mOCSPResponderSignature->setStringValue( txt );
    }

    //dirmngr-0.9.0 options
    saveCheckBoxToKleoEntry( mWidget->ignoreServiceURLCB, e.mIgnoreServiceURLEntry );
    saveCheckBoxToKleoEntry( mWidget->ignoreHTTPDPCB, e.mIgnoreHTTPDPEntry );
    saveCheckBoxToKleoEntry( mWidget->disableHTTPCB, e.mDisableHTTPEntry );
    saveCheckBoxToKleoEntry( mWidget->ignoreLDAPDPCB, e.mIgnoreLDAPDPEntry );
    saveCheckBoxToKleoEntry( mWidget->disableLDAPCB, e.mDisableLDAPEntry );
    if ( e.mCustomHTTPProxy ) {
        const bool honor = mWidget->honorHTTPProxyRB->isChecked();
        if ( e.mHonorHTTPProxy && e.mHonorHTTPProxy->boolValue() != honor )
            e.mHonorHTTPProxy->setBoolValue( honor );

        QString chosenProxy = mWidget->customHTTPProxy->text();
        if ( chosenProxy != e.mCustomHTTPProxy->stringValue() )
            e.mCustomHTTPProxy->setStringValue( chosenProxy );
    }
    txt = mWidget->customLDAPProxy->text();
    if ( e.mCustomLDAPProxy && e.mCustomLDAPProxy->stringValue() != txt )
        e.mCustomLDAPProxy->setStringValue( mWidget->customLDAPProxy->text() );

    mConfig->sync( true );
}

Kleo::CryptoConfigEntry* SMIMECryptoConfigEntries::configEntry( const QString &componentName,
                                                                const QString &groupName,
                                                                const QString &entryName,
                                                                int /*Kleo::CryptoConfigEntry::ArgType*/ argType,
                                                                bool isList )
{
    Kleo::CryptoConfigEntry* entry = mConfig->entry( componentName, groupName, entryName );
    if ( !entry ) {
        kWarning() << QString::fromLatin1("Backend error: gpgconf doesn't seem to know the entry for %1/%2/%3" ).arg( componentName, groupName, entryName );
        return 0;
    }
    if( entry->argType() != argType || entry->isList() != isList ) {
        kWarning() << QString::fromLatin1("Backend error: gpgconf has wrong type for %1/%2/%3: %4 %5" ).arg( componentName, groupName, entryName ).arg( entry->argType() ).arg( entry->isList() );
        return 0;
    }
    return entry;
}

// *************************************************************
// *                                                           *
// *                        MiscPage                           *
// *                                                           *
// *************************************************************
QString MiscPage::helpAnchor() const
{
    return QString::fromLatin1("configure-misc");
}

MiscPage::MiscPage( const KComponentData &instance, QWidget *parent )
    : ConfigModuleWithTabs( instance, parent )
{
    mFolderTab = new FolderTab();
    addTab( mFolderTab, i18n("Folders") );

    mInviteTab = new InviteTab();
    addTab( mInviteTab, i18n("Invitations" ) );

    mProxyTab = new ProxyTab();
    addTab( mProxyTab, i18n("Proxy" ) );

    mAgentSettingsTab = new MiscPageAgentSettingsTab();
    addTab( mAgentSettingsTab, i18n("Agent Settings" ) );

    mPrintingTab = new MiscPagePrintingTab();
    addTab( mPrintingTab, i18n("Printing" ) );
}

QString MiscPageFolderTab::helpAnchor() const
{
    return QString::fromLatin1("Sconfigure-misc-folders");
}

MiscPageFolderTab::MiscPageFolderTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    mMMTab.setupUi( this );
    //replace QWidget with FolderRequester. Promote to doesn't work due to the custom constructor
    QHBoxLayout* layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    mMMTab.mOnStartupOpenFolder->setLayout( layout );
    mOnStartupOpenFolder = new FolderRequester( mMMTab.mOnStartupOpenFolder );
    layout->addWidget( mOnStartupOpenFolder );

    mMMTab.gridLayout->setSpacing( KDialog::spacingHint() );
    mMMTab.gridLayout->setMargin( KDialog::marginHint() );
    mMMTab.mExcludeImportantFromExpiry->setWhatsThis(
                i18n( GlobalSettings::self()->excludeImportantMailFromExpiryItem()->whatsThis().toUtf8() ) );

    connect( mMMTab.mEmptyFolderConfirmCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mMMTab.mExcludeImportantFromExpiry, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mMMTab.mLoopOnGotoUnread, SIGNAL(activated(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mMMTab.mActionEnterFolder, SIGNAL(activated(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mMMTab.mDelayedMarkTime, SIGNAL(valueChanged(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mMMTab.mDelayedMarkAsRead, SIGNAL(toggled(bool)),
             mMMTab.mDelayedMarkTime, SLOT(setEnabled(bool)));
    connect( mMMTab.mDelayedMarkAsRead, SIGNAL(toggled(bool)),
             this , SLOT(slotEmitChanged()) );
    connect( mMMTab.mShowPopupAfterDnD, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mOnStartupOpenFolder, SIGNAL(folderChanged(Akonadi::Collection)),
             this, SLOT(slotEmitChanged()) );
    connect( mMMTab.mEmptyTrashCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mMMTab.mStartUpFolderCheck, SIGNAL(toggled(bool)),
             this, SLOT(slotEmitChanged()) );
    connect( mMMTab.mStartUpFolderCheck, SIGNAL(toggled(bool)),
             mOnStartupOpenFolder, SLOT(setEnabled(bool)) );
}

void MiscPage::FolderTab::doLoadFromGlobalSettings()
{
    mMMTab.mExcludeImportantFromExpiry->setChecked( GlobalSettings::self()->excludeImportantMailFromExpiry() );
    // default = "Loop in current folder"
    mMMTab.mLoopOnGotoUnread->setCurrentIndex( GlobalSettings::self()->loopOnGotoUnread() );
    mMMTab.mActionEnterFolder->setCurrentIndex( GlobalSettings::self()->actionEnterFolder() );
    mMMTab.mDelayedMarkAsRead->setChecked( MessageViewer::GlobalSettings::self()->delayedMarkAsRead() );
    mMMTab.mDelayedMarkTime->setValue( MessageViewer::GlobalSettings::self()->delayedMarkTime() );
    mMMTab.mShowPopupAfterDnD->setChecked( GlobalSettings::self()->showPopupAfterDnD() );
    mMMTab.mStartUpFolderCheck->setChecked( GlobalSettings::self()->startSpecificFolderAtStartup() );
    mOnStartupOpenFolder->setEnabled(GlobalSettings::self()->startSpecificFolderAtStartup());
    doLoadOther();
}

void MiscPage::FolderTab::doLoadOther()
{
    mMMTab.mEmptyTrashCheck->setChecked( GlobalSettings::self()->emptyTrashOnExit() );
    mOnStartupOpenFolder->setCollection( Akonadi::Collection( GlobalSettings::self()->startupFolder() ) );
    mMMTab.mEmptyFolderConfirmCheck->setChecked( GlobalSettings::self()->confirmBeforeEmpty() );
}

void MiscPage::FolderTab::save()
{
    GlobalSettings::self()->setEmptyTrashOnExit( mMMTab.mEmptyTrashCheck->isChecked() );
    GlobalSettings::self()->setConfirmBeforeEmpty( mMMTab.mEmptyFolderConfirmCheck->isChecked() );
    GlobalSettings::self()->setStartupFolder( mOnStartupOpenFolder->collection().id() );

    MessageViewer::GlobalSettings::self()->setDelayedMarkAsRead( mMMTab.mDelayedMarkAsRead->isChecked() );
    MessageViewer::GlobalSettings::self()->setDelayedMarkTime( mMMTab.mDelayedMarkTime->value() );
    GlobalSettings::self()->setActionEnterFolder( mMMTab.mActionEnterFolder->currentIndex() );
    GlobalSettings::self()->setLoopOnGotoUnread( mMMTab.mLoopOnGotoUnread->currentIndex() );
    GlobalSettings::self()->setShowPopupAfterDnD( mMMTab.mShowPopupAfterDnD->isChecked() );
    GlobalSettings::self()->setExcludeImportantMailFromExpiry(
                mMMTab.mExcludeImportantFromExpiry->isChecked() );
    GlobalSettings::self()->setStartSpecificFolderAtStartup(mMMTab.mStartUpFolderCheck->isChecked() );
}

MiscPageAgentSettingsTab::MiscPageAgentSettingsTab( QWidget* parent )
    : ConfigModuleTab( parent )
{
    QHBoxLayout *l = new QHBoxLayout( this );
    l->setContentsMargins( 0 , 0, 0, 0 );
    mConfigureAgent = new ConfigureAgentsWidget;
    l->addWidget( mConfigureAgent );

    connect( mConfigureAgent, SIGNAL(changed()), this, SLOT(slotEmitChanged()) );
}

void MiscPageAgentSettingsTab::doLoadFromGlobalSettings()
{
    mConfigureAgent->doLoadFromGlobalSettings();
}

void MiscPageAgentSettingsTab::save()
{
    mConfigureAgent->save();
}

void MiscPageAgentSettingsTab::doResetToDefaultsOther()
{
    mConfigureAgent->doResetToDefaultsOther();
}

QString MiscPageAgentSettingsTab::helpAnchor() const
{
    return mConfigureAgent->helpAnchor();
}

MiscPageInviteTab::MiscPageInviteTab( QWidget* parent )
    : ConfigModuleTab( parent )
{
    mInvitationUi = new MessageViewer::InvitationSettings( this );
    QHBoxLayout *l = new QHBoxLayout( this );
    l->setContentsMargins( 0 , 0, 0, 0 );
    l->addWidget( mInvitationUi );
    connect( mInvitationUi, SIGNAL(changed()), this, SLOT(slotEmitChanged()) );
}

void MiscPage::InviteTab::doLoadFromGlobalSettings()
{
    mInvitationUi->doLoadFromGlobalSettings();
}

void MiscPage::InviteTab::save()
{
    mInvitationUi->save();
}

void MiscPage::InviteTab::doResetToDefaultsOther()
{
    mInvitationUi->doResetToDefaultsOther();
}


MiscPageProxyTab::MiscPageProxyTab( QWidget* parent )
    : ConfigModuleTab( parent )
{
    KCModuleInfo proxyInfo(QLatin1String("proxy.desktop"));
    mProxyModule = new KCModuleProxy(proxyInfo, parent);
    QHBoxLayout *l = new QHBoxLayout( this );
    l->addWidget( mProxyModule );
    connect(mProxyModule,SIGNAL(changed(bool)), this, SLOT(slotEmitChanged()));
}

void MiscPage::ProxyTab::save()
{
    mProxyModule->save();
}

MiscPagePrintingTab::MiscPagePrintingTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    mPrintingUi = new MessageViewer::PrintingSettings( this );
    QHBoxLayout *l = new QHBoxLayout( this );
    l->setContentsMargins( 0 , 0, 0, 0 );
    l->addWidget( mPrintingUi );
    connect( mPrintingUi, SIGNAL(changed()), this, SLOT(slotEmitChanged()) );
}

void MiscPagePrintingTab::doLoadFromGlobalSettings()
{
    mPrintingUi->doLoadFromGlobalSettings();
}

void MiscPagePrintingTab::doResetToDefaultsOther()
{
    mPrintingUi->doResetToDefaultsOther();
}


void MiscPagePrintingTab::save()
{
    mPrintingUi->save();
}

//----------------------------
#include "configuredialog.moc"
