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

#include "globalsettings.h"
#include "templatesconfiguration_kfg.h"
#include "configuredialoglistview.h"

// other KMail headers:
#include "kmkernel.h"
#include "simplestringlisteditor.h"
#include "colorlistbox.h"
#include "messagesender.h"
#include <kpimidentities/identitymanager.h>
#include "identitylistview.h"
using KMail::IdentityListView;
using KMail::IdentityListViewItem;
#include "folderrequester.h"
using KMail::FolderRequester;
#include "kmmainwidget.h"
#include "tagging.h"

#include "folderselectiontreeview.h"

#include "recentaddresses.h"
using KPIM::RecentAddresses;
#include "completionordereditor.h"
#include "messagelist/utils/aggregationcombobox.h"
#include "messagelist/utils/aggregationconfigbutton.h"
#include "messagelist/utils/themecombobox.h"
#include "messagelist/utils/themeconfigbutton.h"

#include "templatesconfiguration.h"
#include "customtemplates.h"
#include "messageviewer/autoqpointer.h"
#include "messageviewer/nodehelper.h"
#include "messageviewer/globalsettings.h"

using KMail::IdentityListView;
using KMail::IdentityListViewItem;
#include "identitydialog.h"
using KMail::IdentityDialog;

// other kdenetwork headers:
#include <kpimidentities/identity.h>
#include <kmime/kmime_dateformatter.h>
using KMime::DateFormatter;
#include "kleo/cryptoconfig.h"
#include "kleo/cryptobackendfactory.h"
#include "libkleo/ui/keyrequester.h"
#include "libkleo/ui/keyselectiondialog.h"

#include <mailtransport/transportmanagementwidget.h>
using MailTransport::TransportManagementWidget;
// other KDE headers:
#include <kldap/ldapclient.h>
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
#include <kconfig.h>
#include <kcmultidialog.h>
#include <knotifyconfigwidget.h>
#include <kconfiggroup.h>
#include <kbuttongroup.h>
#include <kcolorcombo.h>
#include <kfontrequester.h>
#include <kicondialog.h>
#include <kkeysequencewidget.h>
#include <KIconButton>
#include <KRandom>
#include <KColorScheme>
#include <KComboBox>

// Qt headers:
#include <QBoxLayout>
#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QLabel>
#include <QLayout>
#include <QList>
#include <QRadioButton>
#include <QGroupBox>
#include <QToolTip>
#include <QListWidget>
#include <QValidator>
#include <QVBoxLayout>
#include <QWhatsThis>
#include <QDBusConnection>
#include <QHostInfo>
#include <QTextCodec>
#include <QMenu>

// other headers:
#include <assert.h>
#include <stdlib.h>
#include <kvbox.h>


#include <akonadi/agentfilterproxymodel.h>
#include <akonadi/agentinstancemodel.h>
#include <akonadi/agenttype.h>
#include <akonadi/agentmanager.h>
#include <akonadi/agenttypedialog.h>
#include <akonadi/agentinstancecreatejob.h>
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
    for (int i = 0; i < e->choices().size(); ++i) {
      QRadioButton *button = new QRadioButton( e->choices()[i].label, box );
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
    group->buttons()[e->value()]->animateClick();
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
  setButtons( Help | Default | Cancel | Apply | Ok | User1 );
  setButtonGuiItem( User1, KStandardGuiItem::reset() );
  setModal( modal );
  KWindowSystem::setIcons( winId(), qApp->windowIcon().pixmap( IconSize( KIconLoader::Desktop ), IconSize( KIconLoader::Desktop ) ), qApp->windowIcon().pixmap(IconSize( KIconLoader::Small ), IconSize( KIconLoader::Small ) ) );
  addModule( "kmail_config_identity" );
  addModule( "kmail_config_accounts" );
  addModule( "kmail_config_appearance" );
  addModule( "kmail_config_composer" );
  addModule( "kmail_config_security" );
  addModule( "kleopatra_config_gnupgsystem" );
  addModule( "kmail_config_misc" );

  connect( this, SIGNAL(okClicked()), SLOT(slotOk()) );
  connect( this, SIGNAL(applyClicked()), SLOT(slotApply()) );

  // We store the size of the dialog on hide, because otherwise
  // the KCMultiDialog starts with the size of the first kcm, not
  // the largest one. This way at least after the first showing of
  // the largest kcm the size is kept.
  KConfigGroup geometry( KMKernel::config(), "Geometry" );
  int width = geometry.readEntry( "ConfigureDialogWidth", 0 );
  int height = geometry.readEntry( "ConfigureDialogHeight", 0 );
  if ( width != 0 && height != 0 ) {
     resize( width, height );
  }

}

void ConfigureDialog::hideEvent( QHideEvent *ev )
{
  KConfigGroup geometry( KMKernel::config(), "Geometry" );
  geometry.writeEntry( "ConfigureDialogWidth", width() );
  geometry.writeEntry( "ConfigureDialogHeight",height() );
  KDialog::hideEvent( ev );
}

ConfigureDialog::~ConfigureDialog()
{
}

void ConfigureDialog::slotApply()
{
  slotApplyClicked();
  GlobalSettings::self()->writeConfig();
  emit configChanged();
}

void ConfigureDialog::slotOk()
{
  slotOkClicked();
  GlobalSettings::self()->writeConfig();
  emit configChanged();
}

// *************************************************************
// *                                                           *
// *                      IdentityPage                         *
// *                                                           *
// *************************************************************
QString IdentityPage::helpAnchor() const
{
  return QString::fromLatin1( "configure-identity" );
}

IdentityPage::IdentityPage( const KComponentData &instance, QWidget *parent )
  : ConfigModule( instance, parent ),
    mIdentityDialog( 0 )
{
  mIPage.setupUi( this );

  connect( mIPage.mIdentityList, SIGNAL( itemSelectionChanged() ),
           SLOT( slotIdentitySelectionChanged() ) );
  connect( this, SIGNAL( changed(bool) ),
           SLOT( slotIdentitySelectionChanged() ) );
  connect( mIPage.mIdentityList, SIGNAL( rename( KMail::IdentityListViewItem *, const QString & ) ),
           SLOT( slotRenameIdentity(KMail::IdentityListViewItem *, const QString & ) ) );
  connect( mIPage.mIdentityList, SIGNAL( itemDoubleClicked( QTreeWidgetItem*, int ) ),
           SLOT( slotModifyIdentity() ) );
  connect( mIPage.mIdentityList, SIGNAL( contextMenu( KMail::IdentityListViewItem *, const QPoint & ) ),
           SLOT( slotContextMenu( KMail::IdentityListViewItem *, const QPoint & ) ) );
  // ### connect dragged(...), ...

  connect( mIPage.mButtonAdd, SIGNAL( clicked() ),
           this, SLOT( slotNewIdentity() ) );
  connect( mIPage.mModifyButton, SIGNAL( clicked() ),
           this, SLOT( slotModifyIdentity() ) );
  connect( mIPage.mRenameButton, SIGNAL( clicked() ),
           this, SLOT( slotRenameIdentity() ) );
  connect( mIPage.mRemoveButton, SIGNAL( clicked() ),
           this, SLOT( slotRemoveIdentity() ) );
  connect( mIPage.mSetAsDefaultButton, SIGNAL( clicked() ),
           this, SLOT( slotSetAsDefault() ) );
}

void IdentityPage::load()
{
  KPIMIdentities::IdentityManager *im = kmkernel->identityManager();
  mOldNumberOfIdentities = im->shadowIdentities().count();
  // Fill the list:
  mIPage.mIdentityList->clear();
  QTreeWidgetItem *item = 0;
  for ( KPIMIdentities::IdentityManager::Iterator it = im->modifyBegin(); it != im->modifyEnd(); ++it ) {
    item = new IdentityListViewItem( mIPage.mIdentityList, item, *it );
  }
  if ( mIPage.mIdentityList->currentItem() ) {
    mIPage.mIdentityList->currentItem()->setSelected( true );
  }
}

void IdentityPage::save()
{
  Q_ASSERT( !mIdentityDialog );

  kmkernel->identityManager()->sort();
  kmkernel->identityManager()->commit();

  if( mOldNumberOfIdentities < 2 && mIPage.mIdentityList->topLevelItemCount() > 1 ) {
    // have more than one identity, so better show the combo in the
    // composer now:
    KConfigGroup composer( KMKernel::config(), "Composer" );
    int showHeaders = composer.readEntry( "headers", HDR_STANDARD );
    showHeaders |= HDR_IDENTITY;
    composer.writeEntry( "headers", showHeaders );
  }
  // and now the reverse
  if( mOldNumberOfIdentities > 1 && mIPage.mIdentityList->topLevelItemCount() < 2 ) {
    // have only one identity, so remove the combo in the composer:
    KConfigGroup composer( KMKernel::config(), "Composer" );
    int showHeaders = composer.readEntry( "headers", HDR_STANDARD );
    showHeaders &= ~HDR_IDENTITY;
    composer.writeEntry( "headers", showHeaders );
  }
}

void IdentityPage::slotNewIdentity()
{
  Q_ASSERT( !mIdentityDialog );

  KPIMIdentities::IdentityManager *im = kmkernel->identityManager();
  MessageViewer::AutoQPointer<NewIdentityDialog> dialog( new NewIdentityDialog(
      im->shadowIdentities(), this ) );
  dialog->setObjectName( "new" );

  if ( dialog->exec() == QDialog::Accepted && dialog ) {
    QString identityName = dialog->identityName().trimmed();
    Q_ASSERT( !identityName.isEmpty() );

    //
    // Construct a new Identity:
    //
    switch ( dialog->duplicateMode() ) {
    case NewIdentityDialog::ExistingEntry:
      {
        KPIMIdentities::Identity &dupThis = im->modifyIdentityForName( dialog->duplicateIdentity() );
        im->newFromExisting( dupThis, identityName );
        break;
      }
    case NewIdentityDialog::ControlCenter:
      im->newFromControlCenter( identityName );
      break;
    case NewIdentityDialog::Empty:
      im->newFromScratch( identityName );
    default: ;
    }

    //
    // Insert into listview:
    //
    KPIMIdentities::Identity &newIdent = im->modifyIdentityForName( identityName );
    QTreeWidgetItem *item = 0;
    if ( mIPage.mIdentityList->selectedItems().size() > 0 ) {
      item = mIPage.mIdentityList->selectedItems()[0];
    }

    QTreeWidgetItem * newItem = 0;
    if ( item ) {
      newItem = new IdentityListViewItem( mIPage.mIdentityList, mIPage.mIdentityList->itemAbove( item ), newIdent );
    } else {
      newItem = new IdentityListViewItem( mIPage.mIdentityList, newIdent );
    }

    mIPage.mIdentityList->selectionModel()->clearSelection();
    if ( newItem ) {
      newItem->setSelected( true );
    }

    slotModifyIdentity();
  }
}

void IdentityPage::slotModifyIdentity()
{
  Q_ASSERT( !mIdentityDialog );

  IdentityListViewItem *item = 0;
  if ( mIPage.mIdentityList->selectedItems().size() > 0 ) {
    item = dynamic_cast<IdentityListViewItem*>( mIPage.mIdentityList->selectedItems()[0] );
  }
  if ( !item ) {
    return;
  }

  mIdentityDialog = new IdentityDialog( this );
  mIdentityDialog->setIdentity( item->identity() );

  // Hmm, an unmodal dialog would be nicer, but a modal one is easier ;-)
  if ( mIdentityDialog->exec() == QDialog::Accepted ) {
    mIdentityDialog->updateIdentity( item->identity() );
    item->redisplay();
    emit changed( true );
  }

  delete mIdentityDialog;
  mIdentityDialog = 0;
}

void IdentityPage::slotRemoveIdentity()
{
  Q_ASSERT( !mIdentityDialog );

  KPIMIdentities::IdentityManager *im = kmkernel->identityManager();
  if ( im->shadowIdentities().count() < 2 ) {
    kFatal() << "Attempted to remove the last identity!";
  }

  IdentityListViewItem *item = 0;
  if ( mIPage.mIdentityList->selectedItems().size() > 0 ) {
    item = dynamic_cast<IdentityListViewItem*>( mIPage.mIdentityList->selectedItems()[0] );
  }
  if ( !item ) {
    return;
  }

  QString msg = i18n( "<qt>Do you really want to remove the identity named "
                      "<b>%1</b>?</qt>", item->identity().identityName() );
  if( KMessageBox::warningContinueCancel( this, msg, i18n("Remove Identity"),
                                          KGuiItem(i18n("&Remove"),
                                          "edit-delete") )
      == KMessageBox::Continue ) {
    if ( im->removeIdentity( item->identity().identityName() ) ) {
      delete item;
      if ( mIPage.mIdentityList->currentItem() ) {
        mIPage.mIdentityList->currentItem()->setSelected( true );
      }
      refreshList();
    }
  }
}

void IdentityPage::slotRenameIdentity()
{
  Q_ASSERT( !mIdentityDialog );

  QTreeWidgetItem *item = 0;

  if ( mIPage.mIdentityList->selectedItems().size() > 0 ) {
    item = mIPage.mIdentityList->selectedItems()[0];
  }
  if ( !item ) return;

  mIPage.mIdentityList->editItem( item );
}

void IdentityPage::slotRenameIdentity( KMail::IdentityListViewItem *item , const QString &text )
{
  if ( !item ) return;

  QString newName = text.trimmed();
  if ( !newName.isEmpty() &&
       !kmkernel->identityManager()->shadowIdentities().contains( newName ) ) {
    KPIMIdentities::Identity &ident = item->identity();
    ident.setIdentityName( newName );
    emit changed( true );
  }
  item->redisplay();
}

void IdentityPage::slotContextMenu( IdentityListViewItem *item, const QPoint &pos )
{
  QMenu *menu = new QMenu( this );
  menu->addAction( i18n( "Add..." ), this, SLOT( slotNewIdentity() ) );
  if ( item ) {
    menu->addAction( i18n( "Modify..." ), this, SLOT( slotModifyIdentity() ) );
    if ( mIPage.mIdentityList->topLevelItemCount() > 1 ) {
      menu->addAction( i18n( "Remove" ), this, SLOT( slotRemoveIdentity() ) );
    }
    if ( !item->identity().isDefault() ) {
      menu->addAction( i18n( "Set as Default" ), this, SLOT( slotSetAsDefault() ) );
    }
  }
  menu->exec( pos );
  delete menu;
}


void IdentityPage::slotSetAsDefault()
{
  Q_ASSERT( !mIdentityDialog );

  IdentityListViewItem *item = 0;
  if ( mIPage.mIdentityList->selectedItems().size() > 0 ) {
    item = dynamic_cast<IdentityListViewItem*>( mIPage.mIdentityList->selectedItems()[0] );
  }
  if ( !item ) {
    return;
  }

  KPIMIdentities::IdentityManager *im = kmkernel->identityManager();
  im->setAsDefault( item->identity().uoid() );
  refreshList();
}

void IdentityPage::refreshList()
{
  for ( int i = 0; i < mIPage.mIdentityList->topLevelItemCount(); ++i ) {
    IdentityListViewItem *item = dynamic_cast<IdentityListViewItem*>( mIPage.mIdentityList->topLevelItem( i ) );
    if ( item ) {
      item->redisplay();
    }
  }
  emit changed( true );
}

void IdentityPage::slotIdentitySelectionChanged()
{
  IdentityListViewItem *item = 0;
  if ( mIPage.mIdentityList->selectedItems().size() >  0 ) {
    item = dynamic_cast<IdentityListViewItem*>( mIPage.mIdentityList->selectedItems()[0] );
  }

  mIPage.mRemoveButton->setEnabled( item && mIPage.mIdentityList->topLevelItemCount() > 1 );
  mIPage.mModifyButton->setEnabled( item );
  mIPage.mRenameButton->setEnabled( item );
  mIPage.mSetAsDefaultButton->setEnabled( item && !item->identity().isDefault() );
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
  connect( mReceivingTab, SIGNAL(accountListChanged(const QStringList &)),
           this, SIGNAL(accountListChanged(const QStringList &)) );

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
  connect( mConfirmSendCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // "send on check" combo:
  mSendOnCheckCombo = new KComboBox( group );
  mSendOnCheckCombo->setEditable( false );
  mSendOnCheckCombo->addItems( QStringList()
                                      << i18n("Never Automatically")
                                      << i18n("On Manual Mail Checks")
                                      << i18n("On All Mail Checks") );
  glay->addWidget( mSendOnCheckCombo, 1, 1 );
  connect( mSendOnCheckCombo, SIGNAL( activated( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // "default send method" combo:
  mSendMethodCombo = new KComboBox( group );
  mSendMethodCombo->setEditable( false );
  mSendMethodCombo->addItems( QStringList()
                                      << i18n("Send Now")
                                      << i18n("Send Later") );
  glay->addWidget( mSendMethodCombo, 2, 1 );
  connect( mSendMethodCombo, SIGNAL( activated( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // "default domain" input field:
  mDefaultDomainEdit = new KLineEdit( group );
  glay->addWidget( mDefaultDomainEdit, 3, 1 );
  connect( mDefaultDomainEdit, SIGNAL( textChanged( const QString& ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // labels:
  QLabel *l =  new QLabel( i18n("Send &messages in outbox folder:"), group );
  l->setBuddy( mSendOnCheckCombo );
  glay->addWidget( l, 1, 0 );

  QString msg = i18n( GlobalSettings::self()->sendOnCheckItem()->whatsThis().toUtf8() );
  l->setWhatsThis( msg );
  mSendOnCheckCombo->setWhatsThis( msg );

  l = new QLabel( i18n("Defa&ult send method:"), group );
  l->setBuddy( mSendMethodCombo );
  glay->addWidget( l, 2, 0 );
  l = new QLabel( i18n("Defaul&t domain:"), group );
  l->setBuddy( mDefaultDomainEdit );
  glay->addWidget( l, 3, 0 );

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
  mSendMethodCombo->setCurrentIndex( GlobalSettings::self()->sendImmediate() ? 0 : 1 );
  mConfirmSendCheck->setChecked( GlobalSettings::self()->confirmBeforeSend() );
  QString defaultDomain = GlobalSettings::defaultDomain();
  if( defaultDomain.isEmpty() ) {
    defaultDomain = QHostInfo::localHostName();
  }
  mDefaultDomainEdit->setText( defaultDomain );
}

void AccountsPage::SendingTab::save()
{
  GlobalSettings::self()->setSendOnCheck( mSendOnCheckCombo->currentIndex() );
  GlobalSettings::self()->setDefaultDomain( mDefaultDomainEdit->text() );
  GlobalSettings::self()->setConfirmBeforeSend( mConfirmSendCheck->isChecked() );
  GlobalSettings::self()->setSendImmediate( mSendMethodCombo->currentIndex() == 0 );
}

QString AccountsPage::ReceivingTab::helpAnchor() const
{
  return QString::fromLatin1("configure-accounts-receiving");
}

AccountsPageReceivingTab::AccountsPageReceivingTab( QWidget * parent )
  : ConfigModuleTab( parent )
{
  mAccountsReceiving.setupUi( this );

  mAccountsReceiving.vlay->setSpacing( KDialog::spacingHint() );
  mAccountsReceiving.vlay->setMargin( KDialog::marginHint() );

  mAccountsReceiving.mAccountList->agentFilterProxyModel()->addMimeTypeFilter( "message/rfc822" );
  mAccountsReceiving.mAccountList->agentFilterProxyModel()->addCapabilityFilter( "Resource" ); // show only resources, no agents
  mAccountsReceiving.mFilterAccount->setProxy( mAccountsReceiving.mAccountList->agentFilterProxyModel() );
  connect( mAccountsReceiving.mAccountList, SIGNAL( currentChanged( const Akonadi::AgentInstance&, const Akonadi::AgentInstance& ) ),
           SLOT( slotAccountSelected( const Akonadi::AgentInstance& ) ) );
  connect( mAccountsReceiving.mAccountList, SIGNAL(doubleClicked(Akonadi::AgentInstance)),
           this, SLOT(slotModifySelectedAccount()) );

  mAccountsReceiving.hlay->insertWidget(0, mAccountsReceiving.mAccountList);

  connect( mAccountsReceiving.mAddAccountButton, SIGNAL(clicked()),
           this, SLOT(slotAddAccount()) );

  connect( mAccountsReceiving.mModifyAccountButton, SIGNAL(clicked()),
           this, SLOT(slotModifySelectedAccount()) );

  connect( mAccountsReceiving.mRemoveAccountButton, SIGNAL(clicked()),
           this, SLOT(slotRemoveSelectedAccount()) );

  connect( mAccountsReceiving.mCheckmailStartupCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  mAccountsReceiving.group->layout()->setMargin( KDialog::marginHint() );
  mAccountsReceiving.group->layout()->setSpacing( KDialog::spacingHint() );

  connect( mAccountsReceiving.mBeepNewMailCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  mAccountsReceiving.mVerboseNotificationCheck->setWhatsThis(
  GlobalSettings::self()->verboseNewMailNotificationItem()->whatsThis() );
  connect( mAccountsReceiving.mVerboseNotificationCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged() ) );

  connect( mAccountsReceiving.mOtherNewMailActionsButton, SIGNAL(clicked()),
           this, SLOT(slotEditNotifications()) );
  slotAccountSelected( mAccountsReceiving.mAccountList->currentAgentInstance() );

}

AccountsPageReceivingTab::~AccountsPageReceivingTab()
{

}

void AccountsPage::ReceivingTab::slotAccountSelected(const Akonadi::AgentInstance& current)
{
  if ( !current.isValid() ) {
    mAccountsReceiving.mModifyAccountButton->setEnabled( false );
    mAccountsReceiving.mRemoveAccountButton->setEnabled( false );
  } else {
    mAccountsReceiving.mModifyAccountButton->setEnabled( !current.type().capabilities().contains( QLatin1String( "NoConfig" ) ) );
    mAccountsReceiving.mRemoveAccountButton->setEnabled( true );
  }
}

void AccountsPage::ReceivingTab::slotAddAccount()
{
  //TODO verify this dialog box. We can see note etc...
  Akonadi::AgentTypeDialog dlg( this );
  Akonadi::AgentFilterProxyModel* filter = dlg.agentFilterProxyModel();
  filter->addMimeTypeFilter( "message/rfc822" );
  filter->addCapabilityFilter( "Resource" );
  if ( dlg.exec() ) {
    const Akonadi::AgentType agentType = dlg.agentType();

    if ( agentType.isValid() ) {

      Akonadi::AgentInstanceCreateJob *job = new Akonadi::AgentInstanceCreateJob( agentType, this );
      job->configure( this );
      job->start();
    }
  }


  emit changed( true );
}



void AccountsPage::ReceivingTab::slotModifySelectedAccount()
{
  Akonadi::AgentInstance instance = mAccountsReceiving.mAccountList->currentAgentInstance();
  if ( instance.isValid() ) {
    KWindowSystem::allowExternalProcessWindowActivation();
    instance.configure( this );
  }
  emit changed( true );
}



void AccountsPage::ReceivingTab::slotRemoveSelectedAccount()
{
  const Akonadi::AgentInstance instance =  mAccountsReceiving.mAccountList->currentAgentInstance();
  if ( instance.isValid() )
    Akonadi::AgentManager::self()->removeInstance( instance );

  slotAccountSelected( mAccountsReceiving.mAccountList->currentAgentInstance() );

  emit changed( true );

}

void AccountsPage::ReceivingTab::slotEditNotifications()
{
  if(kmkernel->xmlGuiInstance().isValid())
    KNotifyConfigWidget::configure(this,  kmkernel->xmlGuiInstance().componentName());
  else
    KNotifyConfigWidget::configure(this);
}

void AccountsPage::ReceivingTab::doLoadFromGlobalSettings()
{
  mAccountsReceiving.mVerboseNotificationCheck->setChecked( GlobalSettings::self()->verboseNewMailNotification() );
}

void AccountsPage::ReceivingTab::doLoadOther()
{
  KConfigGroup general( KMKernel::config(), "General" );
  mAccountsReceiving.mBeepNewMailCheck->setChecked( general.readEntry( "beep-on-mail", false ) );
  mAccountsReceiving.mCheckmailStartupCheck->setChecked(
      general.readEntry( "checkmail-startup", false ) );
}

void AccountsPage::ReceivingTab::save()
{
#if 0 //TODO port to akonadi
  kmkernel->cleanupImapFolders();
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  // Save Mail notification settings
  KConfigGroup general( KMKernel::config(), "General" );
  general.writeEntry( "beep-on-mail", mAccountsReceiving.mBeepNewMailCheck->isChecked() );
  GlobalSettings::self()->setVerboseNewMailNotification( mAccountsReceiving.mVerboseNotificationCheck->isChecked() );

  general.writeEntry( "checkmail-startup", mAccountsReceiving.mCheckmailStartupCheck->isChecked() );


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
  { "NewMessageFont", I18N_NOOP("Message List - New Messages"), true, false },
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
  connect ( mCustomFontCheck, SIGNAL( stateChanged( int ) ),
            this, SLOT( slotEmitChanged( void ) ) );

  // "font location" combo box and label:
  hlay = new QHBoxLayout(); // inherites spacing
  vlay->addLayout( hlay );
  mFontLocationCombo = new KComboBox( this );
  mFontLocationCombo->setEditable( false );
  mFontLocationCombo->setEnabled( false ); // !mCustomFontCheck->isChecked()

  QStringList fontDescriptions;
  for ( int i = 0 ; i < numFontNames ; i++ )
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
  mFontChooser->setObjectName( "font" );
  mFontChooser->setEnabled( false ); // since !mCustomFontCheck->isChecked()
  vlay->addWidget( mFontChooser );
  connect ( mFontChooser, SIGNAL( fontSelected( const QFont& ) ),
            this, SLOT( slotEmitChanged( void ) ) );


  // {en,dis}able widgets depending on the state of mCustomFontCheck:
  connect( mCustomFontCheck, SIGNAL(toggled(bool)),
           label, SLOT(setEnabled(bool)) );
  connect( mCustomFontCheck, SIGNAL(toggled(bool)),
           mFontLocationCombo, SLOT(setEnabled(bool)) );
  connect( mCustomFontCheck, SIGNAL(toggled(bool)),
           mFontChooser, SLOT(setEnabled(bool)) );
  // load the right font settings into mFontChooser:
  connect( mFontLocationCombo, SIGNAL(activated(int) ),
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
    for ( int i = 0 ; i < numFontNames ; i++ )
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
  disconnect ( mFontChooser, SIGNAL( fontSelected( const QFont& ) ),
            this, SLOT( slotEmitChanged( void ) ) );

  // Display the new setting:
  mFontChooser->setFont( mFont[index], fontNames[index].onlyFixed );

  connect ( mFontChooser, SIGNAL( fontSelected( const QFont& ) ),
            this, SLOT( slotEmitChanged( void ) ) );

  // Disable Family and Size list if we have selected a quote font:
  mFontChooser->enableColumn( KFontChooser::FamilyList|KFontChooser::SizeList,
                              fontNames[ index ].enableFamilyAndSize );
}

void AppearancePage::FontsTab::doLoadOther()
{
  KConfigGroup fonts( KMKernel::config(), "Fonts" );
  KConfigGroup messagelistFont( KMKernel::config(), "MessageListView::Fonts" );

  mFont[0] = KGlobalSettings::generalFont();
  QFont fixedFont = KGlobalSettings::fixedFont();

  for ( int i = 0 ; i < numFontNames ; i++ ) {
    QString configName = fontNames[i].configName;
    if ( configName == "MessageListFont" ||
         configName == "NewMessageFont" ||
         configName == "UnreadMessageFont" ||
         configName == "ImportantMessageFont" ||
         configName == "TodoMessageFont" ) {
      mFont[i] = messagelistFont.readEntry( configName,
                                            (fontNames[i].onlyFixed) ? fixedFont : mFont[0] );
    } else {
      mFont[i] = fonts.readEntry( configName,
                                  (fontNames[i].onlyFixed) ? fixedFont : mFont[0] );
    }
  }
  mCustomFontCheck->setChecked( !fonts.readEntry( "defaultFonts", true ) );
  mFontLocationCombo->setCurrentIndex( 0 );
  slotFontSelectorChanged( 0 );
}

void AppearancePage::FontsTab::save()
{
  KConfigGroup fonts( KMKernel::config(), "Fonts" );
  KConfigGroup messagelistFont( KMKernel::config(), "MessageListView::Fonts" );

  // read the current font (might have been modified)
  if ( mActiveFontIndex >= 0 )
    mFont[ mActiveFontIndex ] = mFontChooser->font();

  bool customFonts = mCustomFontCheck->isChecked();
  fonts.writeEntry( "defaultFonts", !customFonts );
  messagelistFont.writeEntry( "defaultFonts", !customFonts );

  for ( int i = 0 ; i < numFontNames ; i++ ) {
    QString configName = fontNames[i].configName;
    if ( configName == "MessageListFont" ||
         configName == "NewMessageFont" ||
         configName == "UnreadMessageFont" ||
         configName == "ImportantMessageFont" ||
         configName == "TodoMessageFont" ) {
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
  { "NewMessageColor", I18N_NOOP("New Message") },
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
  connect( mCustomColorCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // color list box:
  mColorList = new ColorListBox( this );
  mColorList->setEnabled( false ); // since !mCustomColorCheck->isChecked()
  QStringList modeList;
  for ( int i = 0 ; i < numColorNames ; i++ )
    mColorList->addColor( i18n(colorNames[i].displayName) );
  vlay->addWidget( mColorList, 1 );

  // "recycle colors" check box:
  mRecycleColorCheck =
    new QCheckBox( i18n("Recycle colors on deep &quoting"), this );
  mRecycleColorCheck->setEnabled( false );
  vlay->addWidget( mRecycleColorCheck );
  connect( mRecycleColorCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // close to quota threshold
  QHBoxLayout *hbox = new QHBoxLayout();
  vlay->addLayout( hbox );
  QLabel *l = new QLabel( i18n("Close to quota threshold:"), this );
  hbox->addWidget( l );
  l->setEnabled( false );
  mCloseToQuotaThreshold = new QSpinBox( this );
  mCloseToQuotaThreshold->setRange( 0, 100 );
  mCloseToQuotaThreshold->setSingleStep( 1 );
  connect( mCloseToQuotaThreshold, SIGNAL( valueChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  mCloseToQuotaThreshold->setEnabled( false );
  mCloseToQuotaThreshold->setSuffix( i18n("%"));
  hbox->addWidget( mCloseToQuotaThreshold );
  hbox->addWidget( new QWidget(this), 2 );

  // {en,dir}able widgets depending on the state of mCustomColorCheck:
  connect( mCustomColorCheck, SIGNAL(toggled(bool)),
           mColorList, SLOT(setEnabled(bool)) );
  connect( mCustomColorCheck, SIGNAL(toggled(bool)),
           mRecycleColorCheck, SLOT(setEnabled(bool)) );
  connect( mCustomColorCheck, SIGNAL(toggled(bool)),
           l, SLOT(setEnabled(bool)) );
  connect( mCustomColorCheck, SIGNAL(toggled(bool)),
     mCloseToQuotaThreshold, SLOT(setEnabled(bool)) );

  connect( mCustomColorCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
}

void AppearancePage::ColorsTab::doLoadOther()
{
  KConfigGroup reader( KMKernel::config(), "Reader" );
  KConfigGroup messageListView( KMKernel::config(), "MessageListView::Colors" );

  mCustomColorCheck->setChecked( !reader.readEntry( "defaultColors", true ) );
  mRecycleColorCheck->setChecked( reader.readEntry( "RecycleQuoteColors", false ) );
  mCloseToQuotaThreshold->setValue( GlobalSettings::self()->closeToQuotaThreshold() );
  KColorScheme scheme( QPalette::Active, KColorScheme::View );

  static const QColor defaultColor[ numColorNames ] = {
    QColor( 0x00, 0x80, 0x00 ), // quoted l1
    QColor( 0x00, 0x70, 0x00 ), // quoted l2
    QColor( 0x00, 0x60, 0x00 ), // quoted l3
    scheme.foreground( KColorScheme::LinkText ).color(), // link
    scheme.foreground( KColorScheme::VisitedText ).color(),// visited link
    scheme.foreground( KColorScheme::NegativeText ).color(), // misspelled words
    Qt::red, // new msg
    Qt::blue, // unread mgs
    QColor( 0x00, 0x7F, 0x00 ), // important msg
    scheme.foreground( KColorScheme::LinkText ).color(), // action item mgs
    QColor( 0x00, 0x80, 0xFF ), // pgp encrypted
    scheme.background( KColorScheme::PositiveBackground ).color(), // pgp ok, trusted key
    QColor( 0xFF, 0xFF, 0x40 ), // pgp ok, untrusted key
    QColor( 0xFF, 0xFF, 0x40 ), // pgp unchk
    Qt::red, // pgp bad
    QColor( 0xFF, 0x40, 0x40 ), // warning text color
    scheme.foreground( KColorScheme::NegativeText ).color(), // close to quota
    Qt::lightGray, // colorbar plain bg
    Qt::black,     // colorbar plain fg
    Qt::black,     // colorbar html  bg
    Qt::white      // colorbar html  fg
  };

  for ( int i = 0 ; i < numColorNames ; i++ ) {
    QString configName = colorNames[i].configName;
    if ( configName == "NewMessageColor" ||
         configName == "UnreadMessageColor" ||
         configName == "ImportantMessageColor" ||
         configName == "TodoMessageColor" ) {
      mColorList->setColor( i, messageListView.readEntry( configName, defaultColor[i] ));
    }
    else
      mColorList->setColor( i,reader.readEntry( configName, defaultColor[i] ));
  }
  connect( mColorList, SIGNAL( changed( ) ),
           this, SLOT( slotEmitChanged( void ) ) );
}

void AppearancePage::ColorsTab::save()
{
  KConfigGroup reader( KMKernel::config(), "Reader" );
  KConfigGroup messageListView( KMKernel::config(), "MessageListView::Colors" );
  bool customColors = mCustomColorCheck->isChecked();
  MessageViewer::GlobalSettings::self()->setUseDefaultColors( !customColors );

  messageListView.writeEntry( "defaultColors", !customColors );

  for ( int i = 0 ; i < numColorNames ; i++ ) {
    // Don't write color info when we use default colors, but write
    // if it's already there:
    QString configName = colorNames[i].configName;
    if ( configName == "NewMessageColor" ||
         configName == "UnreadMessageColor" ||
         configName == "ImportantMessageColor" ||
         configName == "TodoMessageColor" ) {
      if ( customColors || messageListView.hasKey( configName ) )
        messageListView.writeEntry( configName, mColorList->color(i) );

    } else {
      if ( customColors || reader.hasKey( configName ) )
        reader.writeEntry( configName, mColorList->color(i) );
    }
  }
  reader.writeEntry( "RecycleQuoteColors", mRecycleColorCheck->isChecked() );
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
  connect( mFolderListGroup, SIGNAL ( buttonClicked( int ) ),
           this, SLOT( slotEmitChanged() ) );

  QHBoxLayout* folderCBHLayout = new QHBoxLayout();
  mFavoriteFolderViewCB = new QCheckBox( i18n("Show favorite folder view"), this );
  connect( mFavoriteFolderViewCB, SIGNAL(toggled(bool)), SLOT(slotEmitChanged()) );
  folderCBHLayout->addWidget( mFavoriteFolderViewCB );
  mFolderQuickSearchCB = new QCheckBox( i18n("Show folder quick search field"), this );
  connect( mFolderQuickSearchCB, SIGNAL(toggled(bool)), SLOT(slotEmitChanged()) );
  folderCBHLayout->addWidget( mFolderQuickSearchCB );
  vlay->addLayout( folderCBHLayout );
  vlay->addSpacing( KDialog::spacingHint() );   // space before next box

  // "folder tooltips" radio buttons:
  mFolderToolTipsGroupBox = new QGroupBox( this );
  mFolderToolTipsGroupBox->setTitle( i18n( "Folder Tooltips" ) );
  mFolderToolTipsGroupBox->setLayout( new QHBoxLayout() );
  mFolderToolTipsGroupBox->layout()->setSpacing( KDialog::spacingHint() );
  mFolderToolTipsGroup = new QButtonGroup( this );
  connect( mFolderToolTipsGroup, SIGNAL( buttonClicked( int ) ),
           this, SLOT( slotEmitChanged() ) );

  QRadioButton* folderToolTipsAlwaysRadio = new QRadioButton( i18n( "Always" ), mFolderToolTipsGroupBox );
  mFolderToolTipsGroup->addButton( folderToolTipsAlwaysRadio, static_cast< int >( FolderSelectionTreeView::DisplayAlways ) );
  mFolderToolTipsGroupBox->layout()->addWidget( folderToolTipsAlwaysRadio );

  QRadioButton* folderToolTipsElidedRadio = new QRadioButton( i18n( "When Text Obscured" ), mFolderToolTipsGroupBox );
  mFolderToolTipsGroup->addButton( folderToolTipsElidedRadio, static_cast< int >( FolderSelectionTreeView::DisplayWhenTextElided ) );
  folderToolTipsElidedRadio->setEnabled( false ); //Disable it until we reimplement it.
  mFolderToolTipsGroupBox->layout()->addWidget( folderToolTipsElidedRadio );

  QRadioButton* folderToolTipsNeverRadio = new QRadioButton( i18n( "Never" ), mFolderToolTipsGroupBox );
  mFolderToolTipsGroup->addButton( folderToolTipsNeverRadio, static_cast< int >( FolderSelectionTreeView::DisplayNever ) );
  mFolderToolTipsGroupBox->layout()->addWidget( folderToolTipsNeverRadio );

  vlay->addWidget( mFolderToolTipsGroupBox );

  // "show reader window" radio buttons:
  populateButtonGroup( mReaderWindowModeGroupBox = new QGroupBox( this ),
                       mReaderWindowModeGroup = new QButtonGroup( this ),
                       Qt::Vertical, GlobalSettings::self()->readerWindowModeItem() );
  vlay->addWidget( mReaderWindowModeGroupBox );
  connect( mReaderWindowModeGroup, SIGNAL ( buttonClicked( int ) ),
           this, SLOT( slotEmitChanged() ) );

  vlay->addStretch( 10 ); // spacer
}

void AppearancePage::LayoutTab::doLoadOther()
{
  loadWidget( mFolderListGroupBox, mFolderListGroup, GlobalSettings::self()->folderListItem() );
  loadWidget( mReaderWindowModeGroupBox, mReaderWindowModeGroup, GlobalSettings::self()->readerWindowModeItem() );
  mFavoriteFolderViewCB->setChecked( GlobalSettings::self()->enableFavoriteFolderView() );
  mFolderQuickSearchCB->setChecked( GlobalSettings::self()->enableFolderQuickSearch() );
  const KConfigGroup mainFolderView( KMKernel::config(), "MainFolderView" );
  const int checkedFolderToolTipsPolicy = mainFolderView.readEntry( "ToolTipDisplayPolicy", 0 );
  if ( checkedFolderToolTipsPolicy < mFolderToolTipsGroup->buttons().size() && checkedFolderToolTipsPolicy >= 0 )
    mFolderToolTipsGroup->buttons()[ checkedFolderToolTipsPolicy ]->setChecked( true );
}

void AppearancePage::LayoutTab::save()
{
  saveButtonGroup( mFolderListGroup, GlobalSettings::self()->folderListItem() );
  saveButtonGroup( mReaderWindowModeGroup, GlobalSettings::self()->readerWindowModeItem() );
  GlobalSettings::self()->setEnableFavoriteFolderView( mFavoriteFolderViewCB->isChecked() );
  GlobalSettings::self()->setEnableFolderQuickSearch( mFolderQuickSearchCB->isChecked() );
  KConfigGroup mainFolderView( KMKernel::config(), "MainFolderView" );
  mainFolderView.writeEntry( "ToolTipDisplayPolicy", mFolderToolTipsGroup->checkedId() );
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
  { I18N_NOOP("Fancy for&mat (%1)"), KMime::DateFormatter::Fancy },
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
  QRadioButton * radio;

  QVBoxLayout * vlay = new QVBoxLayout( this );
  vlay->setSpacing( KDialog::spacingHint() );
  vlay->setMargin( KDialog::marginHint() );

  // "General Options" group:
  group = new QGroupBox( i18nc( "General options for the message list.", "General" ), this );
  //  group->layout()->setSpacing( KDialog::spacingHint() );
  QVBoxLayout *gvlay = new QVBoxLayout( group );
  gvlay->setSpacing( KDialog::spacingHint() );

  mDisplayMessageToolTips = new QCheckBox( GlobalSettings::self()->messageToolTipEnabledItem()->label(), group );
  gvlay->addWidget( mDisplayMessageToolTips );

  connect( mDisplayMessageToolTips, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  mHideTabBarWithSingleTab = new QCheckBox( GlobalSettings::self()->autoHideTabBarWithSingleTabItem()->label(), group );
  gvlay->addWidget( mHideTabBarWithSingleTab );

  connect( mHideTabBarWithSingleTab, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // "Aggregation"
  using MessageList::Utils::AggregationComboBox;
  mAggregationComboBox = new AggregationComboBox( group );

  QLabel* aggregationLabel = new QLabel( i18n( "Default Aggregation:" ), group );
  aggregationLabel->setBuddy( mAggregationComboBox );

  using MessageList::Utils::AggregationConfigButton;
  AggregationConfigButton * aggregationConfigButton = new AggregationConfigButton( group, mAggregationComboBox );

  QHBoxLayout * aggregationLayout = new QHBoxLayout();
  aggregationLayout->addWidget( aggregationLabel, 1 );
  aggregationLayout->addWidget( mAggregationComboBox, 1 );
  aggregationLayout->addWidget( aggregationConfigButton, 0 );
  gvlay->addLayout( aggregationLayout );

  connect( aggregationConfigButton, SIGNAL( configureDialogCompleted() ),
           this, SLOT( slotSelectDefaultAggregation() ) );
  connect( mAggregationComboBox, SIGNAL( activated( int ) ),
           this, SLOT( slotEmitChanged() ) );

  // "Theme"
  using MessageList::Utils::ThemeComboBox;
  mThemeComboBox = new ThemeComboBox( group );

  QLabel *themeLabel = new QLabel( i18n( "Default Theme:" ), group );
  themeLabel->setBuddy( mThemeComboBox );

  using MessageList::Utils::ThemeConfigButton;
  ThemeConfigButton * themeConfigButton = new ThemeConfigButton( group, mThemeComboBox );

  QHBoxLayout * themeLayout = new QHBoxLayout();
  themeLayout->addWidget( themeLabel, 1 );
  themeLayout->addWidget( mThemeComboBox, 1 );
  themeLayout->addWidget( themeConfigButton, 0 );
  gvlay->addLayout( themeLayout );

  connect( themeConfigButton, SIGNAL( configureDialogCompleted() ),
           this, SLOT( slotSelectDefaultTheme() ) );
  connect( mThemeComboBox, SIGNAL( activated( int ) ),
           this, SLOT( slotEmitChanged() ) );

  vlay->addWidget( group );

  // "Date Display" group:
  mDateDisplay = new KButtonGroup( this );
  mDateDisplay->setTitle( i18n("Date Display") );
  gvlay = new QVBoxLayout( mDateDisplay );
  gvlay->setSpacing( KDialog::spacingHint() );

  for ( int i = 0 ; i < numDateDisplayConfig ; i++ ) {
    const char *label = dateDisplayConfig[i].displayName;
    QString buttonLabel;
    if ( QString(label).contains("%1") )
      buttonLabel = i18n( label, DateFormatter::formatCurrentDate( dateDisplayConfig[i].dateDisplay ) );
    else
      buttonLabel = i18n( label );
    radio = new QRadioButton( buttonLabel, mDateDisplay );
    gvlay->addWidget( radio );

    if ( dateDisplayConfig[i].dateDisplay == DateFormatter::Custom ) {
      KHBox *hbox = new KHBox( mDateDisplay );
      hbox->setSpacing( KDialog::spacingHint() );

      mCustomDateFormatEdit = new KLineEdit( hbox );
      mCustomDateFormatEdit->setEnabled( false );
      hbox->setStretchFactor( mCustomDateFormatEdit, 1 );

      connect( radio, SIGNAL(toggled(bool)),
               mCustomDateFormatEdit, SLOT(setEnabled(bool)) );
      connect( mCustomDateFormatEdit, SIGNAL(textChanged(const QString&)),
               this, SLOT(slotEmitChanged(void)) );

      QLabel *formatHelp = new QLabel(
        i18n( "<qt><a href=\"whatsthis1\">Custom format information...</a></qt>"), hbox );
      connect( formatHelp, SIGNAL(linkActivated(const QString &)),
               SLOT(slotLinkClicked(const QString &)) );

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
    }
  } // end for loop populating mDateDisplay

  vlay->addWidget( mDateDisplay );
  connect( mDateDisplay, SIGNAL( clicked( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );


  vlay->addStretch( 10 ); // spacer
}

void AppearancePageHeadersTab::slotLinkClicked( const QString & link )
{
  if ( link == "whatsthis1" )
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
  KConfigGroup general( KMKernel::config(), "General" );
  KConfigGroup geometry( KMKernel::config(), "Geometry" );

  // "General Options":
  mDisplayMessageToolTips->setChecked( GlobalSettings::self()->messageToolTipEnabled() );
  mHideTabBarWithSingleTab->setChecked( GlobalSettings::self()->autoHideTabBarWithSingleTab() );

  // "Aggregation":
  slotSelectDefaultAggregation();

  // "Theme":
  slotSelectDefaultTheme();

  // "Date Display":
  setDateDisplay( general.readEntry( "dateFormat",
                                     (int) DateFormatter::Fancy ),
                  general.readEntry( "customDateFormat" ) );
}

void AppearancePage::HeadersTab::setDateDisplay( int num, const QString & format )
{
  DateFormatter::FormatType dateDisplay =
    static_cast<DateFormatter::FormatType>( num );

  // special case: needs text for the line edit:
  if ( dateDisplay == DateFormatter::Custom )
    mCustomDateFormatEdit->setText( format );

  for ( int i = 0 ; i < numDateDisplayConfig ; i++ )
    if ( dateDisplay == dateDisplayConfig[i].dateDisplay ) {
      mDateDisplay->setSelected( i );
      return;
    }
  // fell through since none found:
  mDateDisplay->setSelected( numDateDisplayConfig - 2 ); // default
}

void AppearancePage::HeadersTab::save()
{
  KConfigGroup general( KMKernel::config(), "General" );
  KConfigGroup geometry( KMKernel::config(), "Geometry" );

  GlobalSettings::self()->setMessageToolTipEnabled( mDisplayMessageToolTips->isChecked() );
  GlobalSettings::self()->setAutoHideTabBarWithSingleTab( mHideTabBarWithSingleTab->isChecked() );

  // "Aggregation"
  mAggregationComboBox->writeDefaultConfig();

  // "Theme"
  mThemeComboBox->writeDefaultConfig();

  int dateDisplayID = mDateDisplay->selected();
  // check bounds:
  if ( ( dateDisplayID >= 0 ) && ( dateDisplayID < numDateDisplayConfig ) ) {
    general.writeEntry( "dateFormat",
                        (int)dateDisplayConfig[ dateDisplayID ].dateDisplay );
  }
  general.writeEntry( "customDateFormat", mCustomDateFormatEdit->text() );
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
  QVBoxLayout *vlay = new QVBoxLayout( this );
  vlay->setSpacing( KDialog::spacingHint() );
  vlay->setMargin( KDialog::marginHint() );

  // "Close message window after replying or forwarding" check box:
  populateCheckBox( mCloseAfterReplyOrForwardCheck = new QCheckBox( this ),
                    GlobalSettings::self()->closeAfterReplyOrForwardItem() );
  mCloseAfterReplyOrForwardCheck->setToolTip(
      i18n( "Close the standalone message window after replying or forwarding the message" ) );
  vlay->addWidget( mCloseAfterReplyOrForwardCheck );
  connect( mCloseAfterReplyOrForwardCheck, SIGNAL ( stateChanged( int ) ),
           this, SLOT( slotEmitChanged() ) );

  // "show colorbar" check box:
  populateCheckBox( mShowColorbarCheck = new QCheckBox( this ),
                    MessageViewer::GlobalSettings::self()->showColorBarItem() );
  vlay->addWidget( mShowColorbarCheck );
  connect( mShowColorbarCheck, SIGNAL ( stateChanged( int ) ),
           this, SLOT( slotEmitChanged() ) );

  // "show spam status" check box;
  populateCheckBox( mShowSpamStatusCheck = new QCheckBox( this ),
                    MessageViewer::GlobalSettings::self()->showSpamStatusItem() );
  vlay->addWidget( mShowSpamStatusCheck );
  connect( mShowSpamStatusCheck, SIGNAL ( stateChanged( int ) ),
           this, SLOT( slotEmitChanged() ) );

  // "replace smileys by emoticons" check box;
  populateCheckBox( mShowEmoticonsCheck = new QCheckBox( this ),
                    MessageViewer::GlobalSettings::self()->showEmoticonsItem() );
  vlay->addWidget( mShowEmoticonsCheck );
  connect( mShowEmoticonsCheck, SIGNAL ( stateChanged( int ) ),
           this, SLOT( slotEmitChanged() ) );

  // "Use smaller font for quoted text" check box
  mShrinkQuotesCheck = new QCheckBox(
      MessageViewer::GlobalSettings::self()->shrinkQuotesItem()->label(), this );
  mShrinkQuotesCheck->setObjectName( "kcfg_ShrinkQuotes" );
  vlay->addWidget( mShrinkQuotesCheck );
  connect( mShrinkQuotesCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged() ) );

  // "Show expand/collaps quote marks" check box;
  populateCheckBox( mShowExpandQuotesMark = new QCheckBox( this ),
                    MessageViewer::GlobalSettings::self()->showExpandQuotesMarkItem() );
  vlay->addWidget( mShowExpandQuotesMark);
  connect( mShowExpandQuotesMark, SIGNAL ( stateChanged( int ) ),
           this, SLOT( slotEmitChanged() ) );

  //New HBoxLayout to show the collapseQuoteLevel spin+label one line down
  QHBoxLayout * hlay = new QHBoxLayout();
  vlay->addLayout( hlay );
  hlay->addSpacing( 40 );

  mCollapseQuoteLevelSpin = new KIntSpinBox( 0/*min*/,10/*max*/,1/*step*/,
      3/*init*/,this );

  QLabel *label = new QLabel(
           MessageViewer::GlobalSettings::self()->collapseQuoteLevelSpinItem()->label(), this );
  label->setBuddy( mCollapseQuoteLevelSpin );

  hlay->addWidget( label );

  mCollapseQuoteLevelSpin->setEnabled( false ); //since !mShowExpandQuotesMark->isCheckec()
  connect(  mCollapseQuoteLevelSpin, SIGNAL( valueChanged( int ) ),
      this, SLOT( slotEmitChanged( void ) ) );
  hlay->addWidget( mCollapseQuoteLevelSpin);

  connect( mShowExpandQuotesMark, SIGNAL( toggled( bool ) ),
      mCollapseQuoteLevelSpin, SLOT( setEnabled( bool ) ) );

  hlay->addStretch();

  // Fallback Character Encoding
  hlay = new QHBoxLayout(); // inherits spacing
  vlay->addLayout( hlay );
  mCharsetCombo = new KComboBox( this );
  mCharsetCombo->addItems(MessageViewer::NodeHelper::supportedEncodings( false ) );

  connect( mCharsetCombo, SIGNAL( activated( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  QString fallbackCharsetWhatsThis = i18n(
     MessageViewer::GlobalSettings::self()->fallbackCharacterEncodingItem()->whatsThis().toUtf8() );
  mCharsetCombo->setWhatsThis( fallbackCharsetWhatsThis );

  label = new QLabel( i18n("Fallback ch&aracter encoding:"), this );
  label->setBuddy( mCharsetCombo );

  hlay->addWidget( label );
  hlay->addWidget( mCharsetCombo );

  // Override Character Encoding
  QHBoxLayout *hlay2 = new QHBoxLayout(); // inherits spacing
  vlay->addLayout( hlay2 );
  mOverrideCharsetCombo = new KComboBox( this );
  QStringList encodings =MessageViewer::NodeHelper::supportedEncodings( false );
  encodings.prepend( i18n( "Auto" ) );
  mOverrideCharsetCombo->addItems( encodings );
  mOverrideCharsetCombo->setCurrentIndex(0);

  connect( mOverrideCharsetCombo, SIGNAL( activated( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  QString overrideCharsetWhatsThis = i18n(
     MessageViewer::GlobalSettings::self()->overrideCharacterEncodingItem()->whatsThis().toUtf8() );
  mOverrideCharsetCombo->setWhatsThis( overrideCharsetWhatsThis );

  label = new QLabel( i18n("&Override character encoding:"), this );
  label->setBuddy( mOverrideCharsetCombo );

  hlay2->addWidget( label );
  hlay2->addWidget( mOverrideCharsetCombo );

  vlay->addStretch( 100 ); // spacer
}


void AppearancePage::ReaderTab::readCurrentFallbackCodec()
{
  QStringList encodings =MessageViewer::NodeHelper::supportedEncodings( false );
  QStringList::ConstIterator it( encodings.begin() );
  QStringList::ConstIterator end( encodings.end() );
  QString currentEncoding = MessageViewer::GlobalSettings::self()->fallbackCharacterEncoding();
  uint i = 0;
  int indexOfLatin9 = 0;
  bool found = false;
  for( ; it != end; ++it)
  {
    const QString encoding = MessageViewer::NodeHelper::encodingForName(*it);
    if ( encoding == "ISO-8859-15" )
        indexOfLatin9 = i;
    if( encoding == currentEncoding )
    {
      mCharsetCombo->setCurrentIndex( i );
      found = true;
      break;
    }
    i++;
  }
  if ( !found ) // nothing matched, use latin9
    mCharsetCombo->setCurrentIndex( indexOfLatin9 );
}

void AppearancePage::ReaderTab::readCurrentOverrideCodec()
{
  const QString &currentOverrideEncoding = MessageViewer::GlobalSettings::self()->overrideCharacterEncoding();
  if ( currentOverrideEncoding.isEmpty() ) {
    mOverrideCharsetCombo->setCurrentIndex( 0 );
    return;
  }
  QStringList encodings =MessageViewer::NodeHelper::supportedEncodings( false );
  encodings.prepend( i18n( "Auto" ) );
  QStringList::ConstIterator it( encodings.constBegin() );
  QStringList::ConstIterator end( encodings.constEnd() );
  int i = 0;
  for( ; it != end; ++it)
  {
    if( MessageViewer::NodeHelper::encodingForName(*it) == currentOverrideEncoding )
    {
      mOverrideCharsetCombo->setCurrentIndex( i );
      break;
    }
    i++;
  }
  if ( i == encodings.size() ) {
    // the current value of overrideCharacterEncoding is an unknown encoding => reset to Auto
    kWarning() <<"Unknown override character encoding \"" << currentOverrideEncoding
                   << "\". Resetting to Auto.";
    mOverrideCharsetCombo->setCurrentIndex( 0 );
    MessageViewer::GlobalSettings::self()->setOverrideCharacterEncoding( QString() );
  }
}

void AppearancePage::ReaderTab::doLoadFromGlobalSettings()
{
  mShowEmoticonsCheck->setChecked( MessageViewer::GlobalSettings::self()->showEmoticons() );
  mShrinkQuotesCheck->setChecked( MessageViewer::GlobalSettings::self()->shrinkQuotes() );
  mShowExpandQuotesMark->setChecked( MessageViewer::GlobalSettings::self()->showExpandQuotesMark() );
  mCollapseQuoteLevelSpin->setValue( MessageViewer::GlobalSettings::self()->collapseQuoteLevelSpin() );
  readCurrentFallbackCodec();
  readCurrentOverrideCodec();
}

void AppearancePage::ReaderTab::doLoadOther()
{
  loadWidget( mCloseAfterReplyOrForwardCheck, GlobalSettings::self()->closeAfterReplyOrForwardItem() );
  loadWidget( mShowColorbarCheck, MessageViewer::GlobalSettings::self()->showColorBarItem() );
  loadWidget( mShowSpamStatusCheck, MessageViewer::GlobalSettings::self()->showSpamStatusItem() );
}


void AppearancePage::ReaderTab::save()
{
  saveCheckBox( mCloseAfterReplyOrForwardCheck, GlobalSettings::self()->closeAfterReplyOrForwardItem() );
  saveCheckBox( mShowColorbarCheck, MessageViewer::GlobalSettings::self()->showColorBarItem() );
  saveCheckBox( mShowSpamStatusCheck, MessageViewer::GlobalSettings::self()->showSpamStatusItem() );
  MessageViewer::GlobalSettings::self()->setShowEmoticons( mShowEmoticonsCheck->isChecked() );
  MessageViewer::GlobalSettings::self()->setShrinkQuotes( mShrinkQuotesCheck->isChecked() );
  MessageViewer::GlobalSettings::self()->setShowExpandQuotesMark( mShowExpandQuotesMark->isChecked() );

  MessageViewer::GlobalSettings::self()->setCollapseQuoteLevelSpin( mCollapseQuoteLevelSpin->value() );
  MessageViewer::GlobalSettings::self()->setFallbackCharacterEncoding(
      MessageViewer::NodeHelper::encodingForName( mCharsetCombo->currentText() ) );
  MessageViewer::GlobalSettings::self()->setOverrideCharacterEncoding(
      mOverrideCharsetCombo->currentIndex() == 0 ?
        QString() :
        MessageViewer::NodeHelper::encodingForName( mOverrideCharsetCombo->currentText() ) );
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
  connect( mSystemTrayCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // System tray modes
  mSystemTrayGroup = new KButtonGroup( this );
  mSystemTrayGroup->setTitle( i18n("System Tray Mode" ) );
  QVBoxLayout *gvlay = new QVBoxLayout( mSystemTrayGroup );
  gvlay->setSpacing( KDialog::spacingHint() );

  connect( mSystemTrayGroup, SIGNAL( clicked( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  connect( mSystemTrayCheck, SIGNAL( toggled( bool ) ),
           mSystemTrayGroup, SLOT( setEnabled( bool ) ) );

  gvlay->addWidget( new QRadioButton( i18n("Always show KMail in system tray"), mSystemTrayGroup ) );
  gvlay->addWidget( new QRadioButton( i18n("Only show KMail in system tray if there are unread messages"), mSystemTrayGroup ) );

  vlay->addWidget( mSystemTrayGroup );
  vlay->addStretch( 10 ); // spacer
}

void AppearancePage::SystemTrayTab::doLoadFromGlobalSettings()
{
  mSystemTrayCheck->setChecked( GlobalSettings::self()->systemTrayEnabled() );
  mSystemTrayGroup->setSelected( GlobalSettings::self()->systemTrayPolicy() );
  mSystemTrayGroup->setEnabled( mSystemTrayCheck->isChecked() );
}

void AppearancePage::SystemTrayTab::save()
{
  GlobalSettings::self()->setSystemTrayEnabled( mSystemTrayCheck->isChecked() );
  GlobalSettings::self()->setSystemTrayPolicy( mSystemTrayGroup->selected() );
}

QString AppearancePage::MessageTagTab::helpAnchor() const
{
  return QString::fromLatin1("configure-appearance-messagetag");
}

AppearancePageMessageTagTab::AppearancePageMessageTagTab( QWidget * parent )
  : ConfigModuleTab( parent )
{
  mEmitChanges = true;
  mPreviousTag = -1;

  QHBoxLayout *maingrid = new QHBoxLayout( this );
  maingrid->setMargin( KDialog::marginHint() );
  maingrid->setSpacing( KDialog::spacingHint() );

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
  addremovegrid->addWidget( mTagAddLineEdit );

  mTagAddButton = new KPushButton( mTagsGroupBox );
  mTagAddButton->setToolTip( i18n("Add new tag") );
  mTagAddButton->setIcon( KIcon( "list-add" ) );
  addremovegrid->addWidget( mTagAddButton );

  mTagRemoveButton = new KPushButton( mTagsGroupBox );
  mTagRemoveButton->setToolTip( i18n("Remove selected tag") );
  mTagRemoveButton->setIcon( KIcon( "list-remove" ) );
  addremovegrid->addWidget( mTagRemoveButton );

  //Up and down buttons
  QHBoxLayout *updowngrid = new QHBoxLayout();
  tageditgrid->addLayout( updowngrid );

  mTagUpButton = new KPushButton( mTagsGroupBox );
  mTagUpButton->setToolTip( i18n("Increase tag priority") );
  mTagUpButton->setIcon( KIcon( "arrow-up" ) );
  mTagUpButton->setAutoRepeat( true );
  updowngrid->addWidget( mTagUpButton );

  mTagDownButton = new KPushButton( mTagsGroupBox );
  mTagDownButton->setToolTip( i18n("Decrease tag priority") );
  mTagDownButton->setIcon( KIcon( "arrow-down" ) );
  mTagDownButton->setAutoRepeat( true );
  updowngrid->addWidget( mTagDownButton );

  //Listbox for tag names
  QHBoxLayout *listboxgrid = new QHBoxLayout();
  tageditgrid->addLayout( listboxgrid );
  mTagListBox = new QListWidget( mTagsGroupBox );
  mTagListBox->setMinimumWidth( 150 );
  listboxgrid->addWidget( mTagListBox );

  //RHS for individual tag settings

  //Extra VBoxLayout for stretchers around settings
  QVBoxLayout *tagsettinggrid = new QVBoxLayout();
  maingrid->addLayout( tagsettinggrid );
  tagsettinggrid->addStretch( 10 );

  //Groupbox frame
  mTagSettingGroupBox = new QGroupBox( i18n("Ta&g Settings"),
                                      this );
  tagsettinggrid->addWidget( mTagSettingGroupBox );
  QGridLayout *settings = new QGridLayout( mTagSettingGroupBox );
  settings->setMargin( KDialog::marginHint() );
  settings->setSpacing( KDialog::spacingHint() );

  //Stretcher layout for adding some space after the label
  QVBoxLayout *spacer = new QVBoxLayout();
  settings->addLayout( spacer, 0, 0, 1, 2 );
  spacer->addSpacing( 2 * KDialog::spacingHint() );

  //First row for renaming
  mTagNameLineEdit = new KLineEdit( mTagSettingGroupBox );
  settings->addWidget( mTagNameLineEdit, 1, 1 );

  QLabel *namelabel = new QLabel( i18nc("@label:listbox Name of the tag", "Name:")
    , mTagSettingGroupBox );
  namelabel->setBuddy( mTagNameLineEdit );
  settings->addWidget( namelabel, 1, 0 );

  connect( mTagNameLineEdit, SIGNAL( textChanged( const QString& ) ),
            this, SLOT( slotEmitChangeCheck( void ) ) );

  //Second row for text color
  mTextColorCheck = new QCheckBox( i18n("Change te&xt color:"),
                                   mTagSettingGroupBox );
  settings->addWidget( mTextColorCheck, 2, 0 );

  mTextColorCombo = new KColorCombo( mTagSettingGroupBox );
  settings->addWidget( mTextColorCombo, 2, 1 );

  connect( mTextColorCheck, SIGNAL( toggled( bool ) ),
          mTextColorCombo, SLOT( setEnabled( bool ) ) );
  connect( mTextColorCheck, SIGNAL( stateChanged( int ) ),
          this, SLOT( slotEmitChangeCheck( void ) ) );
  connect( mTextColorCombo, SIGNAL( activated( int ) ),
          this, SLOT( slotEmitChangeCheck( void ) ) );

  //Third row for text background color
  mBackgroundColorCheck = new QCheckBox( i18n("Change &background color:"),
                                             mTagSettingGroupBox );
  settings->addWidget( mBackgroundColorCheck, 3, 0 );

  mBackgroundColorCombo = new KColorCombo( mTagSettingGroupBox );
  settings->addWidget( mBackgroundColorCombo, 3, 1 );

  connect( mBackgroundColorCheck, SIGNAL( toggled( bool ) ),
          mBackgroundColorCombo, SLOT( setEnabled( bool ) ) );
  connect( mBackgroundColorCheck, SIGNAL( stateChanged( int ) ),
          this, SLOT( slotEmitChangeCheck( void ) ) );
  connect( mBackgroundColorCombo, SIGNAL( activated( int ) ),
          this, SLOT( slotEmitChangeCheck( void ) ) );

  //Fourth for font selection
  mTextFontCheck = new QCheckBox( i18n("Change fo&nt:"), mTagSettingGroupBox );
  settings->addWidget( mTextFontCheck, 4, 0 );

  mFontRequester = new KFontRequester( mTagSettingGroupBox );
  settings->addWidget( mFontRequester, 4, 1 );

  connect( mTextFontCheck, SIGNAL( toggled( bool ) ),
          mFontRequester, SLOT( setEnabled( bool ) ) );
  connect( mTextFontCheck, SIGNAL( stateChanged( int ) ),
          this, SLOT( slotEmitChangeCheck( void ) ) );
  connect( mFontRequester, SIGNAL( fontSelected( const QFont& ) ),
          this, SLOT( slotEmitChangeCheck( void ) ) );

  //Fifth for toolbar icon
  mIconButton = new KIconButton( mTagSettingGroupBox );
  mIconButton->setIconSize( 16 );
  mIconButton->setIconType( KIconLoader::NoGroup, KIconLoader::Action );
  settings->addWidget( mIconButton, 5, 1 );
  connect( mIconButton, SIGNAL(iconChanged(QString)),
           SLOT(slotIconNameChanged(QString)) );

  QLabel *iconlabel = new QLabel( i18n("Message tag &icon:"),
                                  mTagSettingGroupBox );
  iconlabel->setBuddy( mIconButton );
  settings->addWidget( iconlabel, 5, 0 );

  //We do not connect the checkbox to icon selector since icons are used in the
  //menus as well
  connect( mIconButton, SIGNAL( iconChanged( QString ) ),
          this, SLOT( slotEmitChangeCheck( void ) ) );

  //Sixth for shortcut
  mKeySequenceWidget = new KKeySequenceWidget( mTagSettingGroupBox );
  settings->addWidget( mKeySequenceWidget, 6, 1 );
  QLabel *sclabel = new QLabel( i18n("Shortc&ut:") , mTagSettingGroupBox );
  sclabel->setBuddy( mKeySequenceWidget );
  settings->addWidget( sclabel, 6, 0 );
  mKeySequenceWidget->setCheckActionCollections(
      kmkernel->getKMMainWidget()->actionCollections() );

  connect( mKeySequenceWidget, SIGNAL( keySequenceChanged( const QKeySequence & ) ),
           this, SLOT( slotEmitChangeCheck() ) );

  //Seventh for Toolbar checkbox
  mInToolbarCheck = new QCheckBox( i18n("Enable &toolbar button"),
                                    mTagSettingGroupBox );
  settings->addWidget( mInToolbarCheck, 7, 0 );
  connect( mInToolbarCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChangeCheck( void ) ) );

  tagsettinggrid->addStretch( 10 );

  //Adjust widths for columns
  maingrid->setStretchFactor( mTagsGroupBox, 1 );
  maingrid->setStretchFactor( tagsettinggrid, 1 );

  //Other Connections

  //For enabling the add button in case box is non-empty
  connect( mTagAddLineEdit, SIGNAL( textChanged( const QString& ) ),
          this, SLOT( slotAddLineTextChanged( const QString& ) ) );

  //For on-the-fly updating of tag name in editbox
  connect( mTagNameLineEdit, SIGNAL( textChanged( const QString& ) ),
          this, SLOT( slotNameLineTextChanged( const QString& ) ) );

  connect( mTagAddButton, SIGNAL( clicked( void ) ),
          this, SLOT( slotAddNewTag( void ) ) );

  connect( mTagRemoveButton, SIGNAL( clicked( void ) ),
          this, SLOT( slotRemoveTag( void ) ) );

  connect( mTagUpButton, SIGNAL( clicked( void ) ),
          this, SLOT( slotMoveTagUp( void ) ) );

  connect( mTagDownButton, SIGNAL( clicked( void ) ),
          this, SLOT( slotMoveTagDown( void ) ) );

  connect( mTagListBox, SIGNAL( itemSelectionChanged() ),
          this, SLOT( slotSelectionChanged() ) );
}

AppearancePageMessageTagTab::~AppearancePageMessageTagTab()
{
}

void AppearancePage::MessageTagTab::slotEmitChangeCheck()
{
  if ( mEmitChanges )
    slotEmitChanged();
}

void AppearancePage::MessageTagTab::slotMoveTagUp()
{
  int tmp_index = mTagListBox->currentRow();
  if ( tmp_index <= 0 )
    return;
  swapTagsInListBox( tmp_index, tmp_index - 1 );
    //Reached the first row
  if ( 1 == tmp_index )
    mTagUpButton->setEnabled( false );
    //Escaped from last row
  if ( int( mTagListBox->count() ) - 1 == tmp_index )
    mTagDownButton->setEnabled( true );
}

void AppearancePage::MessageTagTab::slotMoveTagDown()
{
  int tmp_index = mTagListBox->currentRow();
  if ( ( tmp_index < 0 )
        || ( tmp_index >= int( mTagListBox->count() ) - 1 ) )
    return;
  swapTagsInListBox( tmp_index, tmp_index + 1 );
    //Reached last row
  if ( int( mTagListBox->count() ) - 2 == tmp_index )
    mTagDownButton->setEnabled( false );
    //Escaped from first row
  if ( 0 == tmp_index )
    mTagUpButton->setEnabled( true );
}

void AppearancePage::MessageTagTab::swapTagsInListBox( const int first,
                                                       const int second )
{
  const QString tmp_label = mTagListBox->item( first )->text();
  const QIcon tmp_icon = mTagListBox->item( first )->icon();
  KMail::Tag::Ptr tmp_ptr = mMsgTagList.at( first );

  mMsgTagList.replace( first, mMsgTagList.at( second ) );
  mMsgTagList.replace( second, tmp_ptr );

  disconnect( mTagListBox, SIGNAL( itemSelectionChanged() ),
              this, SLOT( slotSelectionChanged() ) );
  mTagListBox->item( first )->setText( mTagListBox->item( second )->text() );
  mTagListBox->item( first )->setIcon( mTagListBox->item( second )->icon() );
  mTagListBox->item( second )->setText( tmp_label );
  mTagListBox->item( second )->setIcon( tmp_icon );
  mTagListBox->setCurrentItem( mTagListBox->item( second ) );
  connect( mTagListBox, SIGNAL( itemSelectionChanged() ),
           this, SLOT( slotSelectionChanged() ) );

  mPreviousTag = second;

  slotEmitChangeCheck();
}

void AppearancePage::MessageTagTab::slotRecordTagSettings( int aIndex )
{
  if ( ( aIndex < 0 ) || ( aIndex >= int( mTagListBox->count() ) ) )
    return;

  KMail::Tag::Ptr tmp_desc = mMsgTagList.at( aIndex );

  tmp_desc->tagName = mTagListBox->item( aIndex )->text();

  tmp_desc->textColor = mTextColorCheck->isChecked() ?
                          mTextColorCombo->color() : QColor();

  tmp_desc->backgroundColor = mBackgroundColorCheck->isChecked() ?
                                mBackgroundColorCombo->color() : QColor();

  tmp_desc->textFont = mTextFontCheck->isChecked() ?
                          mFontRequester->font() : QFont();

  tmp_desc->iconName = mIconButton->icon();

  mKeySequenceWidget->applyStealShortcut();
  tmp_desc->shortcut = KShortcut( mKeySequenceWidget->keySequence() );

  tmp_desc->inToolbar = mInToolbarCheck->isChecked();
}

void AppearancePage::MessageTagTab::slotUpdateTagSettingWidgets( int aIndex )
{
  //We are just updating the display, so no need to mark dirty
  mEmitChanges = false;
  //Check if selection is valid
  if ( ( aIndex < 0 ) || ( mTagListBox->currentRow() < 0 ) ) {
    mTagRemoveButton->setEnabled( false );
    mTagUpButton->setEnabled( false );
    mTagDownButton->setEnabled( false );

    mTagNameLineEdit->setEnabled( false );
    mTextColorCheck->setEnabled( false );
    mBackgroundColorCheck->setEnabled( false );
    mTextFontCheck->setEnabled( false );
    mInToolbarCheck->setEnabled( false );
    mTextColorCombo->setEnabled( false );
    mFontRequester->setEnabled( false );
    mIconButton->setEnabled( false );
    mKeySequenceWidget->setEnabled( false );
    mBackgroundColorCombo->setEnabled( false );
    mEmitChanges = true;
    return;
  }

  mTagRemoveButton->setEnabled( true );
  mTagUpButton->setEnabled( ( 0 != aIndex ) );
  mTagDownButton->setEnabled(
                          ( ( int( mMsgTagList.count() ) - 1 ) != aIndex ) );

  KMail::Tag::Ptr tmp_desc = mMsgTagList.at( mTagListBox->currentRow() );

  mTagNameLineEdit->setEnabled( true );
  mTagNameLineEdit->setText( tmp_desc->tagName );

  QColor tmp_color = tmp_desc->textColor;
  mTextColorCheck->setEnabled( true );
  if ( tmp_color.isValid() ) {
    mTextColorCombo->setColor( tmp_color );
    mTextColorCheck->setChecked( true );
  } else {
    mTextColorCheck->setChecked( false );
  }

  tmp_color = tmp_desc->backgroundColor;
  mBackgroundColorCheck->setEnabled( true );
  if ( tmp_color.isValid() ) {
    mBackgroundColorCombo->setColor( tmp_color );
    mBackgroundColorCheck->setChecked( true );
  } else {
    mBackgroundColorCheck->setChecked( false );
  }

  QFont tmp_font = tmp_desc->textFont;
  mTextFontCheck->setEnabled( true );
  mTextFontCheck->setChecked( ( tmp_font != QFont() ) );
  mFontRequester->setFont( tmp_font );

  mIconButton->setEnabled( true );
  mIconButton->setIcon( tmp_desc->iconName );

  mKeySequenceWidget->setEnabled( true );
  mKeySequenceWidget->setKeySequence( tmp_desc->shortcut.primary(),
                                      KKeySequenceWidget::NoValidate );

  mInToolbarCheck->setEnabled( true );
  mInToolbarCheck->setChecked( tmp_desc->inToolbar );

  mEmitChanges = true;
}

void AppearancePage::MessageTagTab::slotSelectionChanged()
{
  slotRecordTagSettings( mPreviousTag );
  slotUpdateTagSettingWidgets( mTagListBox->currentRow() );
  mPreviousTag = mTagListBox->currentRow();
}

void AppearancePage::MessageTagTab::slotRemoveTag()
{
  int tmp_index = mTagListBox->currentRow();
  if ( !( tmp_index < 0 ) ) {
    KMail::Tag::Ptr tmp_desc = mMsgTagList.takeAt( tmp_index );
    mMsgTagDict.remove( tmp_desc->nepomukResourceUri.toString() );
    Nepomuk::Tag nepomukTag( tmp_desc->nepomukResourceUri );
    nepomukTag.remove();
    mPreviousTag = -1;

    //Before deleting the current item, make sure the selectionChanged signal
    //is disconnected, so that the widgets will not get updated while the
    //deletion takes place.
    disconnect( mTagListBox, SIGNAL( itemSelectionChanged() ),
                this, SLOT( slotSelectionChanged() ) );
    delete mTagListBox->takeItem( tmp_index );
    connect( mTagListBox, SIGNAL( itemSelectionChanged() ),
                this, SLOT( slotSelectionChanged() ) );

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

  //Disconnect so the tag information is not saved and reloaded with every
  //letter
  disconnect( mTagListBox, SIGNAL( itemSelectionChanged() ),
              this, SLOT( slotSelectionChanged() ) );
  mTagListBox->currentItem()->setText( aText );
  connect( mTagListBox, SIGNAL( itemSelectionChanged () ),
           this, SLOT( slotSelectionChanged() ) );
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
  const int tmp_priority = mMsgTagList.count();
  const QString newTagName = mTagAddLineEdit->text();
  Nepomuk::Tag nepomukTag( newTagName );
  nepomukTag.setLabel( newTagName );

  KMail::Tag::Ptr tag = KMail::Tag::fromNepomuk( nepomukTag );
  tag->priority = tmp_priority;
  mMsgTagDict.insert( tag->nepomukResourceUri.toString() , tag );
  mMsgTagList.append( tag );
  slotEmitChangeCheck();
  QListWidgetItem *newItem = new QListWidgetItem( mTagListBox );
  newItem->setText( newTagName );
  newItem->setIcon( KIcon( tag->iconName ) );
  mTagListBox->addItem( newItem );
  mTagListBox->setCurrentItem( newItem );
  mTagAddLineEdit->setText( QString() );
}

void AppearancePage::MessageTagTab::doLoadFromGlobalSettings()
{
  mMsgTagDict.clear();
  mMsgTagList.clear();
  mTagListBox->clear();

  foreach( const Nepomuk::Tag &nepomukTag, Nepomuk::Tag::allTags() ) {
    KMail::Tag::Ptr tag = KMail::Tag::fromNepomuk( nepomukTag );
    mMsgTagDict.insert( nepomukTag.resourceUri().toString(), tag );
    mMsgTagList.append( tag );
  }

  qSort( mMsgTagList.begin(), mMsgTagList.end(), KMail::Tag::compare );

  foreach( const KMail::Tag::Ptr tag, mMsgTagList ) {
    new QListWidgetItem( KIcon( tag->iconName ), tag->tagName, mTagListBox );
    if ( tag->priority == -1 )
      tag->priority = mTagListBox->count() - 1;
  }

  //Disconnect so that insertItem's do not trigger an update procedure
  disconnect( mTagListBox, SIGNAL( itemSelectionChanged() ),
              this, SLOT( slotSelectionChanged() ) );

  connect( mTagListBox, SIGNAL( itemSelectionChanged() ),
           this, SLOT( slotSelectionChanged() ) );
  slotUpdateTagSettingWidgets( -1 );
  //Needed since the previous function doesn't affect add button
  mTagAddButton->setEnabled( false );
}

void AppearancePage::MessageTagTab::save()
{
  slotRecordTagSettings( mTagListBox->currentRow() );

  foreach( const KMail::Tag::Ptr &tag, mMsgTagList ) {

    tag->priority = mMsgTagList.indexOf( tag );

    KMail::Tag::SaveFlags saveFlags = 0;
    if ( mTextColorCheck->isChecked() )
      saveFlags |= KMail::Tag::TextColor;
    if ( mBackgroundColorCheck->isChecked() )
      saveFlags |= KMail::Tag::BackgroundColor;
    if ( mTextFontCheck->isChecked() )
      saveFlags |= KMail::Tag::Font;

    tag->saveToNepomuk( saveFlags );
  }

  KMail::TagActionManager::triggerUpdate();
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
}

QString ComposerPage::GeneralTab::helpAnchor() const
{
  return QString::fromLatin1("configure-composer-general");
}

ComposerPageGeneralTab::ComposerPageGeneralTab( QWidget * parent )
  : ConfigModuleTab( parent )
{
  // tmp. vars:
  QVBoxLayout *vlay;
  QHBoxLayout *hlay;
  QGroupBox   *group;
  QLabel      *label;
  KHBox       *hbox;
  QString      msg;

  vlay = new QVBoxLayout( this );
  vlay->setSpacing( KDialog::spacingHint() );
  vlay->setMargin( KDialog::marginHint() );

  // some check buttons...
  mAutoAppSignFileCheck = new QCheckBox(
           GlobalSettings::self()->autoTextSignatureItem()->label(),
           this );
  vlay->addWidget( mAutoAppSignFileCheck );
  connect( mAutoAppSignFileCheck, SIGNAL( stateChanged(int) ),
           this, SLOT( slotEmitChanged( void ) ) );

  mTopQuoteCheck = new QCheckBox(
                GlobalSettings::self()->prependSignatureItem()->label(), this );
  mTopQuoteCheck->setEnabled( false );
  vlay->addWidget( mTopQuoteCheck);
  connect( mTopQuoteCheck, SIGNAL( stateChanged(int) ),
           this, SLOT( slotEmitChanged(void) ) );
  connect( mAutoAppSignFileCheck, SIGNAL( toggled(bool) ),
           mTopQuoteCheck, SLOT( setEnabled(bool) ) );
  mDashDashCheck = new QCheckBox(
               GlobalSettings::self()->dashDashSignatureItem()->label(), this );
  mDashDashCheck->setEnabled( false );
  vlay->addWidget( mDashDashCheck);
  connect( mDashDashCheck, SIGNAL( stateChanged(int) ),
           this, SLOT( slotEmitChanged(void) ) );
  connect( mAutoAppSignFileCheck, SIGNAL( toggled(bool) ),
           mDashDashCheck, SLOT( setEnabled(bool)) );

  mSmartQuoteCheck = new QCheckBox(
           GlobalSettings::self()->smartQuoteItem()->label(), this );
  mSmartQuoteCheck->setObjectName( "kcfg_SmartQuote" );
  mSmartQuoteCheck->setToolTip(
                 i18n( "When replying, add quote signs in front of all lines of the quoted text,\n"
                       "even when the line was created by adding an additional linebreak while\n"
                       "word-wrapping the text." ) );
  vlay->addWidget( mSmartQuoteCheck );
  connect( mSmartQuoteCheck, SIGNAL( stateChanged(int) ),
           this, SLOT( slotEmitChanged( void ) ) );

  mQuoteSelectionOnlyCheck = new QCheckBox( GlobalSettings::self()->quoteSelectionOnlyItem()->label(),
                                            this );
  mQuoteSelectionOnlyCheck->setObjectName( "kcfg_QuoteSelectionOnly" );
  mQuoteSelectionOnlyCheck->setToolTip(
                 i18n( "When replying, only quote the selected text instead of the complete message "
                       "when there is text selected in the message window." ) );
  vlay->addWidget( mQuoteSelectionOnlyCheck );
  connect( mQuoteSelectionOnlyCheck, SIGNAL( stateChanged(int) ),
           this, SLOT( slotEmitChanged(void) ) );

  mStripSignatureCheck = new QCheckBox( GlobalSettings::self()->stripSignatureItem()->label(),
                                        this );
  mStripSignatureCheck->setObjectName( "kcfg_StripSignature" );
  vlay->addWidget( mStripSignatureCheck );
  connect( mStripSignatureCheck, SIGNAL( stateChanged(int) ),
           this, SLOT( slotEmitChanged( void ) ) );

  mAutoRequestMDNCheck = new QCheckBox(
           GlobalSettings::self()->requestMDNItem()->label(), this);
  mAutoRequestMDNCheck->setObjectName( "kcfg_RequestMDN" );
  vlay->addWidget( mAutoRequestMDNCheck );
  connect( mAutoRequestMDNCheck, SIGNAL( stateChanged(int) ),
           this, SLOT( slotEmitChanged( void ) ) );

  mShowRecentAddressesInComposer = new QCheckBox(
           GlobalSettings::self()->showRecentAddressesInComposerItem()->label(),
           this);
  mShowRecentAddressesInComposer->setObjectName( "kcfg_ShowRecentAddressesInComposer" );
  vlay->addWidget( mShowRecentAddressesInComposer );
  connect( mShowRecentAddressesInComposer, SIGNAL( stateChanged(int) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // a checkbox for "word wrap" and a spinbox for the column in
  // which to wrap:
  hlay = new QHBoxLayout(); // inherits spacing
  vlay->addLayout( hlay );
  mWordWrapCheck = new QCheckBox(
           GlobalSettings::self()->wordWrapItem()->label(), this);
  mWordWrapCheck->setObjectName( "kcfg_WordWrap" );
  hlay->addWidget( mWordWrapCheck );
  connect( mWordWrapCheck, SIGNAL( stateChanged(int) ),
           this, SLOT( slotEmitChanged( void ) ) );

  mWrapColumnSpin = new KIntSpinBox( 30/*min*/, 78/*max*/, 1/*step*/,
           78/*init*/, this );
  mWrapColumnSpin->setObjectName( "kcfg_LineWrapWidth" );
  mWrapColumnSpin->setEnabled( false ); // since !mWordWrapCheck->isChecked()
  connect( mWrapColumnSpin, SIGNAL( valueChanged(int) ),
           this, SLOT( slotEmitChanged( void ) ) );

  hlay->addWidget( mWrapColumnSpin );
  hlay->addStretch( 1 );
  // only enable the spinbox if the checkbox is checked:
  connect( mWordWrapCheck, SIGNAL(toggled(bool)),
           mWrapColumnSpin, SLOT(setEnabled(bool)) );

#ifdef KDEPIM_ENTERPRISE_BUILD
  // a checkbox for "too many recipient warning" and a spinbox for the recipient threshold
  hlay = new QHBoxLayout(); // inherits spacing
  vlay->addLayout( hlay );
  mRecipientCheck = new QCheckBox(
           GlobalSettings::self()->tooManyRecipientsItem()->label(), this);
  mRecipientCheck->setObjectName( "kcfg_TooManyRecipients" );
  hlay->addWidget( mRecipientCheck );
  connect( mRecipientCheck, SIGNAL( stateChanged(int) ),
           this, SLOT( slotEmitChanged( void ) ) );

  QString recipientCheckWhatsthis =
      i18n( GlobalSettings::self()->tooManyRecipientsItem()->whatsThis().toUtf8() );
  mRecipientCheck->setWhatsThis( recipientCheckWhatsthis );
  mRecipientCheck->setToolTip( i18n( "Warn if too many recipients are specified" ) );

  mRecipientSpin = new KIntSpinBox( 1/*min*/, 100/*max*/, 1/*step*/,
                                    5/*init*/, this );
  mRecipientSpin->setObjectName( "kcfg_RecipientThreshold" );
  mRecipientSpin->setEnabled( false );
  connect( mRecipientSpin, SIGNAL( valueChanged(int) ),
           this, SLOT( slotEmitChanged( void ) ) );

  QString recipientWhatsthis =
      i18n( GlobalSettings::self()->recipientThresholdItem()->whatsThis().toUtf8() );
  mRecipientSpin->setWhatsThis( recipientWhatsthis );
  mRecipientSpin->setToolTip( i18n( "Warn if more than this many recipients are specified" ) );


  hlay->addWidget( mRecipientSpin );
  hlay->addStretch( 1 );
  // only enable the spinbox if the checkbox is checked:
  connect( mRecipientCheck, SIGNAL(toggled(bool)),
           mRecipientSpin, SLOT(setEnabled(bool)) );
#endif

  hlay = new QHBoxLayout(); // inherits spacing
  vlay->addLayout( hlay );
  mAutoSave = new KIntSpinBox( 0, 60, 1, 1, this );
  mAutoSave->setObjectName( "kcfg_AutosaveInterval" );
  label = new QLabel( GlobalSettings::self()->autosaveIntervalItem()->label(), this );
  label->setBuddy( mAutoSave );
  hlay->addWidget( label );
  hlay->addWidget( mAutoSave );
  mAutoSave->setSpecialValueText( i18n("No autosave") );
  mAutoSave->setSuffix( i18n(" min") );
  hlay->addStretch( 1 );
  connect( mAutoSave, SIGNAL( valueChanged(int) ),
           this, SLOT( slotEmitChanged( void ) ) );

#ifdef KDEPIM_ENTERPRISE_BUILD
  hlay = new QHBoxLayout(); // inherits spacing
  vlay->addLayout( hlay );
  mForwardTypeCombo = new KComboBox( false, this );
  label = new QLabel( i18n( "Default Forwarding Type:" ),
                      this );
  label->setBuddy( mForwardTypeCombo );
  mForwardTypeCombo->addItems( QStringList() << i18nc( "@item:inlistbox Inline mail forwarding",
                                                       "Inline" )
                                             << i18n( "As Attachment" ) );
  hlay->addWidget( label );
  hlay->addWidget( mForwardTypeCombo );
  hlay->addStretch( 1 );
  connect( mForwardTypeCombo, SIGNAL( activated( int ) ),
           this, SLOT( slotEmitChanged() ) );
#endif

  hlay = new QHBoxLayout(); // inherits spacing
  vlay->addLayout( hlay );
  QPushButton *completionOrderBtn = new QPushButton( i18n( "Configure Completion Order..." ), this );
  connect( completionOrderBtn, SIGNAL( clicked() ),
           this, SLOT( slotConfigureCompletionOrder() ) );
  hlay->addWidget( completionOrderBtn );
  hlay->addItem( new QSpacerItem(0, 0) );

  // recent addresses
  hlay = new QHBoxLayout(); // inherits spacing
  vlay->addLayout( hlay );
  QPushButton *recentAddressesBtn = new QPushButton( i18n( "Edit Recent Addresses..." ), this );
  connect( recentAddressesBtn, SIGNAL( clicked() ),
           this, SLOT( slotConfigureRecentAddresses() ) );
  hlay->addWidget( recentAddressesBtn );
  hlay->addItem( new QSpacerItem(0, 0) );

  // The "external editor" group:
  group = new QGroupBox( i18n("External Editor"), this );
  QLayout *layout = new QVBoxLayout( group );
  group->layout()->setSpacing( KDialog::spacingHint() );

  mExternalEditorCheck = new QCheckBox(
           GlobalSettings::self()->useExternalEditorItem()->label(), group);
  mExternalEditorCheck->setObjectName( "kcfg_UseExternalEditor" );
  connect( mExternalEditorCheck, SIGNAL( toggled( bool ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  hbox = new KHBox( group );
  label = new QLabel( GlobalSettings::self()->externalEditorItem()->label(),
                   hbox );
  mEditorRequester = new KUrlRequester( hbox );
  mEditorRequester->setObjectName( "kcfg_ExternalEditor" );
  connect( mEditorRequester, SIGNAL( urlSelected(const KUrl&) ),
           this, SLOT( slotEmitChanged( void ) ) );
  connect( mEditorRequester, SIGNAL( textChanged(const QString&) ),
           this, SLOT( slotEmitChanged( void ) ) );

  hbox->setStretchFactor( mEditorRequester, 1 );
  label->setBuddy( mEditorRequester );
  label->setEnabled( false ); // since !mExternalEditorCheck->isChecked()
  // ### FIXME: allow only executables (x-bit when available..)
  mEditorRequester->setFilter( "application/x-executable "
                               "application/x-shellscript "
                               "application/x-desktop" );
  mEditorRequester->setMode(KFile::File|KFile::ExistingOnly|KFile::LocalOnly);
  mEditorRequester->setEnabled( false ); // !mExternalEditorCheck->isChecked()
  connect( mExternalEditorCheck, SIGNAL(toggled(bool)),
           label, SLOT(setEnabled(bool)) );
  connect( mExternalEditorCheck, SIGNAL(toggled(bool)),
           mEditorRequester, SLOT(setEnabled(bool)) );

  label = new QLabel( i18n("<b>%f</b> will be replaced with the "
                           "filename to edit."), group );
  label->setEnabled( false ); // see above
  connect( mExternalEditorCheck, SIGNAL(toggled(bool)),
           label, SLOT(setEnabled(bool)) );
  layout->addWidget( mExternalEditorCheck );
  layout->addWidget( hbox );
  layout->addWidget( label );

  vlay->addWidget( group );
  vlay->addStretch( 100 );
}

void ComposerPage::GeneralTab::doLoadFromGlobalSettings()
{
  // various check boxes:

  mAutoAppSignFileCheck->setChecked(
           GlobalSettings::self()->autoTextSignature()=="auto" );
  mTopQuoteCheck->setChecked( GlobalSettings::self()->prependSignature() );
  mDashDashCheck->setChecked( GlobalSettings::self()->dashDashSignature() );
  mSmartQuoteCheck->setChecked( GlobalSettings::self()->smartQuote() );
  mQuoteSelectionOnlyCheck->setChecked( GlobalSettings::self()->quoteSelectionOnly() );
  mStripSignatureCheck->setChecked( GlobalSettings::self()->stripSignature() );
  mAutoRequestMDNCheck->setChecked( GlobalSettings::self()->requestMDN() );
  mWordWrapCheck->setChecked( GlobalSettings::self()->wordWrap() );
  mWrapColumnSpin->setValue( GlobalSettings::self()->lineWrapWidth() );
  mAutoSave->setValue( GlobalSettings::self()->autosaveInterval() );

#ifdef KDEPIM_ENTERPRISE_BUILD
  mRecipientCheck->setChecked( GlobalSettings::self()->tooManyRecipients() );
  mRecipientSpin->setValue( GlobalSettings::self()->recipientThreshold() );
  if ( GlobalSettings::self()->forwardingInlineByDefault() )
    mForwardTypeCombo->setCurrentIndex( 0 );
  else
    mForwardTypeCombo->setCurrentIndex( 1 );
#endif

  // editor group:
  mExternalEditorCheck->setChecked( GlobalSettings::self()->useExternalEditor() );
  mEditorRequester->setText( GlobalSettings::self()->externalEditor() );
}

void ComposerPage::GeneralTab::save() {
  GlobalSettings::self()->setAutoTextSignature(
         mAutoAppSignFileCheck->isChecked() ? "auto" : "manual" );
  GlobalSettings::self()->setPrependSignature( mTopQuoteCheck->isChecked() );
  GlobalSettings::self()->setDashDashSignature( mDashDashCheck->isChecked() );
  GlobalSettings::self()->setSmartQuote( mSmartQuoteCheck->isChecked() );
  GlobalSettings::self()->setQuoteSelectionOnly( mQuoteSelectionOnlyCheck->isChecked() );
  GlobalSettings::self()->setStripSignature( mStripSignatureCheck->isChecked() );
  GlobalSettings::self()->setRequestMDN( mAutoRequestMDNCheck->isChecked() );
  GlobalSettings::self()->setWordWrap( mWordWrapCheck->isChecked() );
  GlobalSettings::self()->setLineWrapWidth( mWrapColumnSpin->value() );
  GlobalSettings::self()->setAutosaveInterval( mAutoSave->value() );

#ifdef KDEPIM_ENTERPRISE_BUILD
  GlobalSettings::self()->setTooManyRecipients( mRecipientCheck->isChecked() );
  GlobalSettings::self()->setRecipientThreshold( mRecipientSpin->value() );
  GlobalSettings::self()->setForwardingInlineByDefault( mForwardTypeCombo->currentIndex() == 0 );
#endif

  // editor group:
  GlobalSettings::self()->setUseExternalEditor( mExternalEditorCheck->isChecked() );
  GlobalSettings::self()->setExternalEditor( mEditorRequester->text() );
}

void ComposerPage::GeneralTab::slotConfigureRecentAddresses()
{
  MessageViewer::AutoQPointer<KPIM::RecentAddressDialog> dlg( new KPIM::RecentAddressDialog( this ) );
  dlg->setAddresses( RecentAddresses::self( KMKernel::config().data() )->addresses() );
  if ( dlg->exec() && dlg ) {
    RecentAddresses::self( KMKernel::config().data() )->clear();
    const QStringList &addrList = dlg->addresses();
    QStringList::ConstIterator it;
    for ( it = addrList.constBegin(); it != addrList.constEnd(); ++it )
      RecentAddresses::self( KMKernel::config().data() )->add( *it );
  }
}

void ComposerPage::GeneralTab::slotConfigureCompletionOrder()
{
  KLDAP::LdapClientSearch search;
  KPIM::CompletionOrderEditor editor( &search, this );
  editor.exec();
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

  mWidget = new TemplatesConfiguration( this );
  vlay->addWidget( mWidget );

  connect( mWidget, SIGNAL( changed() ),
           this, SLOT( slotEmitChanged( void ) ) );
}

void ComposerPage::TemplatesTab::doLoadFromGlobalSettings()
{
    mWidget->loadFromGlobal();
}

void ComposerPage::TemplatesTab::save()
{
    mWidget->saveToGlobal();
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

  mWidget = new CustomTemplates( this );
  vlay->addWidget( mWidget );

  connect( mWidget, SIGNAL( changed() ),
           this, SLOT( slotEmitChanged( void ) ) );
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
  SimpleStringListEditor::ButtonCode buttonCode =
    static_cast<SimpleStringListEditor::ButtonCode>( SimpleStringListEditor::Add | SimpleStringListEditor::Remove | SimpleStringListEditor::Modify );
  mReplyListEditor =
    new SimpleStringListEditor( group, buttonCode,
                                i18n("A&dd..."), i18n("Re&move"),
                                i18n("Mod&ify..."),
                                i18n("Enter new reply prefix:") );
  connect( mReplyListEditor, SIGNAL( changed( void ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // row 2: "replace [...]" check box:
  mReplaceReplyPrefixCheck = new QCheckBox(
     GlobalSettings::self()->replaceReplyPrefixItem()->label(),
     group);
  mReplaceReplyPrefixCheck->setObjectName( "kcfg_ReplaceReplyPrefix" );
  connect( mReplaceReplyPrefixCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
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
    new SimpleStringListEditor( group, buttonCode,
                                i18n("Add..."),
                                i18n("Remo&ve"),
                                i18n("Modify..."),
                                i18n("Enter new forward prefix:") );
  connect( mForwardListEditor, SIGNAL( changed( void ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // row 3: "replace [...]" check box:
  mReplaceForwardPrefixCheck = new QCheckBox(
       GlobalSettings::self()->replaceForwardPrefixItem()->label(),
       group);
  mReplaceForwardPrefixCheck->setObjectName( "kcfg_ReplaceForwardPrefix" );
  connect( mReplaceForwardPrefixCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  layout->addWidget( label );
  layout->addWidget( mForwardListEditor );
  layout->addWidget( mReplaceForwardPrefixCheck );
  vlay->addWidget( group );
}

void ComposerPage::SubjectTab::doLoadFromGlobalSettings()
{
  mReplyListEditor->setStringList( GlobalSettings::self()->replyPrefixes() );
  mReplaceReplyPrefixCheck->setChecked( GlobalSettings::self()->replaceReplyPrefix() );
  mForwardListEditor->setStringList( GlobalSettings::self()->forwardPrefixes() );
  mReplaceForwardPrefixCheck->setChecked( GlobalSettings::self()->replaceForwardPrefix() );
}

void ComposerPage::SubjectTab::save()
{
  GlobalSettings::self()->setReplyPrefixes( mReplyListEditor->stringList() );
  GlobalSettings::self()->setForwardPrefixes( mForwardListEditor->stringList() );
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
    new SimpleStringListEditor( this, SimpleStringListEditor::All,
                                i18n("A&dd..."), i18n("Remo&ve"),
                                i18n("&Modify..."), i18n("Enter charset:") );
  connect( mCharsetListEditor, SIGNAL( changed( void ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  vlay->addWidget( mCharsetListEditor, 1 );

  mKeepReplyCharsetCheck = new QCheckBox( i18n("&Keep original charset when "
                                                "replying or forwarding (if "
                                                "possible)"), this );
  connect( mKeepReplyCharsetCheck, SIGNAL ( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
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
      .arg( QString( kmkernel->networkCodec()->name() ).toLower() );
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
  KConfigGroup composer( KMKernel::config(), "Composer" );

  QStringList charsets = GlobalSettings::preferedCharsets();
  for ( QStringList::Iterator it = charsets.begin() ;
        it != charsets.end() ; ++it )
    if ( (*it) == QString::fromLatin1("locale") ) {
      QByteArray cset = kmkernel->networkCodec()->name();
      kAsciiToLower( cset.data() );
      (*it) = QString("%1 (locale)").arg( QString::fromLatin1( cset ) );
    }

  mCharsetListEditor->setStringList( charsets );
  mKeepReplyCharsetCheck->setChecked( GlobalSettings::forceReplyCharset() );
}

void ComposerPage::CharsetTab::save()
{
  KConfigGroup composer( KMKernel::config(), "Composer" );

  QStringList charsetList = mCharsetListEditor->stringList();
  QStringList::Iterator it = charsetList.begin();
  for ( ; it != charsetList.end() ; ++it )
    if ( (*it).endsWith( QLatin1String("(locale)") ) )
      (*it) = "locale";
  GlobalSettings::setPreferedCharsets( charsetList );
  GlobalSettings::setForceReplyCharset( mKeepReplyCharsetCheck->isChecked() );
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
  connect( mCreateOwnMessageIdCheck, SIGNAL ( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  vlay->addWidget( mCreateOwnMessageIdCheck );

  // "Message-Id suffix" line edit and label:
  hlay = new QHBoxLayout(); // inherits spacing
  vlay->addLayout( hlay );
  mMessageIdSuffixEdit = new KLineEdit( this );
  mMessageIdSuffixEdit->setClearButtonShown( true );
  // only ASCII letters, digits, plus, minus and dots are allowed
  mMessageIdSuffixValidator =
    new QRegExpValidator( QRegExp( "[a-zA-Z0-9+-]+(?:\\.[a-zA-Z0-9+-]+)*" ), this );
  mMessageIdSuffixEdit->setValidator( mMessageIdSuffixValidator );
  label = new QLabel(i18n("Custom message-&id suffix:"), this );
  label->setBuddy( mMessageIdSuffixEdit );
  label->setEnabled( false ); // since !mCreateOwnMessageIdCheck->isChecked()
  mMessageIdSuffixEdit->setEnabled( false );
  hlay->addWidget( label );
  hlay->addWidget( mMessageIdSuffixEdit, 1 );
  connect( mCreateOwnMessageIdCheck, SIGNAL(toggled(bool) ),
           label, SLOT(setEnabled(bool)) );
  connect( mCreateOwnMessageIdCheck, SIGNAL(toggled(bool) ),
           mMessageIdSuffixEdit, SLOT(setEnabled(bool)) );
  connect( mMessageIdSuffixEdit, SIGNAL( textChanged( const QString& ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // horizontal rule and "custom header fields" label:
  vlay->addWidget( new KSeparator( Qt::Horizontal, this ) );
  vlay->addWidget( new QLabel( i18n("Define custom mime header fields:"), this) );

  // "custom header fields" listbox:
  glay = new QGridLayout(); // inherits spacing
  vlay->addLayout( glay );
  glay->setRowStretch( 2, 1 );
  glay->setColumnStretch( 1, 1 );
  mTagList = new ListView( this );
  mTagList->setObjectName( "tagList" );
  mTagList->setHeaderLabels( QStringList() << i18nc("@title:column Name of the mime header.","Name")
    << i18nc("@title:column Value of the mimeheader.","Value") );
  mTagList->setSortingEnabled( false );
  connect( mTagList, SIGNAL(itemSelectionChanged()),
           this, SLOT(slotMimeHeaderSelectionChanged()) );
  glay->addWidget( mTagList, 0, 0, 3, 2 );

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
  mTagNameEdit->setEnabled( false );
  mTagNameLabel = new QLabel( i18nc("@label:textbox Name of the mime header.","&Name:"), this );
  mTagNameLabel->setBuddy( mTagNameEdit );
  mTagNameLabel->setEnabled( false );
  glay->addWidget( mTagNameLabel, 3, 0 );
  glay->addWidget( mTagNameEdit, 3, 1 );
  connect( mTagNameEdit, SIGNAL(textChanged(const QString&)),
           this, SLOT(slotMimeHeaderNameChanged(const QString&)) );

  mTagValueEdit = new KLineEdit( this );
  mTagValueEdit->setEnabled( false );
  mTagValueLabel = new QLabel( i18n("&Value:"), this );
  mTagValueLabel->setBuddy( mTagValueEdit );
  mTagValueLabel->setEnabled( false );
  glay->addWidget( mTagValueLabel, 4, 0 );
  glay->addWidget( mTagValueEdit, 4, 1 );
  connect( mTagValueEdit, SIGNAL(textChanged(const QString&)),
           this, SLOT(slotMimeHeaderValueChanged(const QString&)) );
}

void ComposerPage::HeadersTab::slotMimeHeaderSelectionChanged()
{
  QTreeWidgetItem * item = mTagList->currentItem();

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
}


void ComposerPage::HeadersTab::slotMimeHeaderNameChanged( const QString & text )
{
  // is called on ::setup(), when clearing the line edits. So be
  // prepared to not find a selection:
  QTreeWidgetItem * item = mTagList->currentItem();
  if ( item )
    item->setText( 0, text );
  emit changed( true );
}


void ComposerPage::HeadersTab::slotMimeHeaderValueChanged( const QString & text )
{
  // is called on ::setup(), when clearing the line edits. So be
  // prepared to not find a selection:
  QTreeWidgetItem * item = mTagList->currentItem();
  if ( item )
    item->setText( 1, text );
  emit changed( true );
}


void ComposerPage::HeadersTab::slotNewMimeHeader()
{
  QTreeWidgetItem *listItem = new QTreeWidgetItem( mTagList );
  mTagList->setCurrentItem( listItem );
  emit changed( true );
}


void ComposerPage::HeadersTab::slotRemoveMimeHeader()
{
  // calling this w/o selection is a programming error:
  QTreeWidgetItem *item = mTagList->currentItem();
  if ( !item ) {
    kDebug() << "=================================================="
                  << "Error: Remove button was pressed although no custom header was selected\n"
                  << "==================================================\n";
    return;
  }

  QTreeWidgetItem *below = mTagList->itemBelow( item );

  if ( below ) {
    kDebug() << "below";
    mTagList->setCurrentItem( below );
    delete item;
    item = 0;
  } else if ( mTagList->topLevelItemCount() > 0 ) {
    delete item;
    item = 0;
    mTagList->setCurrentItem(
      mTagList->topLevelItem( mTagList->topLevelItemCount() - 1 )
    );
  }

  emit changed( true );
}

void ComposerPage::HeadersTab::doLoadOther()
{
  mMessageIdSuffixEdit->setText( GlobalSettings::customMsgIDSuffix() );
  const bool state = ( !GlobalSettings::customMsgIDSuffix().isEmpty() &&
                       GlobalSettings::useCustomMessageIdSuffix() );
  mCreateOwnMessageIdCheck->setChecked( state );

  mTagList->clear();
  mTagNameEdit->clear();
  mTagValueEdit->clear();

  QTreeWidgetItem * item = 0;

  const KConfigGroup general( KMKernel::config(), "General" );
  int count = general.readEntry( "mime-header-count", 0 );
  for( int i = 0 ; i < count ; i++ ) {
    KConfigGroup config( KMKernel::config(),
                         QString("Mime #") + QString::number(i) );
    QString name  = config.readEntry( "name" );
    QString value = config.readEntry( "value" );
    if( !name.isEmpty() ) {
      item = new QTreeWidgetItem( mTagList, item );
      item->setText( 0, name );
      item->setText( 1, value );
    }
  }
  if ( mTagList->topLevelItemCount() > 0 ) {
    mTagList->setCurrentItem( mTagList->topLevelItem( 0 ) );
  }
  else {
    // disable the "Remove" button
    mRemoveHeaderButton->setEnabled( false );
  }
}

void ComposerPage::HeadersTab::save()
{
  GlobalSettings::self()->setCustomMsgIDSuffix( mMessageIdSuffixEdit->text() );
  GlobalSettings::self()->setUseCustomMessageIdSuffix( mCreateOwnMessageIdCheck->isChecked() );

  int numValidEntries = 0;
  QTreeWidgetItem *item = 0;
  for ( int i = 0; i < mTagList->topLevelItemCount(); ++i ) {
    item = mTagList->topLevelItem( i );
    if( !item->text(0).isEmpty() ) {
      KConfigGroup config( KMKernel::config(), QString("Mime #")
                             + QString::number( numValidEntries ) );
      config.writeEntry( "name",  item->text( 0 ) );
      config.writeEntry( "value", item->text( 1 ) );
      numValidEntries++;
    }
  }
  KConfigGroup general( KMKernel::config(), "General" );
  general.writeEntry( "mime-header-count", numValidEntries );
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
  connect( mOutlookCompatibleCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  connect( mOutlookCompatibleCheck, SIGNAL( clicked() ),
           this, SLOT( slotOutlookCompatibleClicked() ) );
  vlay->addWidget( mOutlookCompatibleCheck );
  vlay->addSpacing( 5 );

  // "Enable detection of missing attachments" check box
  mMissingAttachmentDetectionCheck =
    new QCheckBox( i18n("E&nable detection of missing attachments"), this );
  mMissingAttachmentDetectionCheck->setChecked( true );
  connect( mMissingAttachmentDetectionCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  vlay->addWidget( mMissingAttachmentDetectionCheck );

  // "Attachment key words" label and string list editor
  label = new QLabel( i18n("Recognize any of the following key words as "
                           "intention to attach a file:"), this );
  label->setAlignment( Qt::AlignLeft );
  label->setWordWrap( true );

  vlay->addWidget( label );

  SimpleStringListEditor::ButtonCode buttonCode =
    static_cast<SimpleStringListEditor::ButtonCode>( SimpleStringListEditor::Add | SimpleStringListEditor::Remove | SimpleStringListEditor::Modify );
  mAttachWordsListEditor =
    new SimpleStringListEditor( this, buttonCode,
                                i18n("A&dd..."), i18n("Re&move"),
                                i18n("Mod&ify..."),
                                i18n("Enter new key word:") );
  connect( mAttachWordsListEditor, SIGNAL( changed( void ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  vlay->addWidget( mAttachWordsListEditor );

  connect( mMissingAttachmentDetectionCheck, SIGNAL(toggled(bool) ),
           label, SLOT(setEnabled(bool)) );
  connect( mMissingAttachmentDetectionCheck, SIGNAL(toggled(bool) ),
           mAttachWordsListEditor, SLOT(setEnabled(bool)) );
}

void ComposerPage::AttachmentsTab::doLoadFromGlobalSettings()
{
  mOutlookCompatibleCheck->setChecked(
    GlobalSettings::self()->outlookCompatibleAttachments() );
  mMissingAttachmentDetectionCheck->setChecked(
    GlobalSettings::self()->showForgottenAttachmentWarning() );

  QStringList attachWordsList = GlobalSettings::self()->attachmentKeywords();
  mAttachWordsListEditor->setStringList( attachWordsList );
}

void ComposerPage::AttachmentsTab::save()
{
  GlobalSettings::self()->setOutlookCompatibleAttachments(
    mOutlookCompatibleCheck->isChecked() );
  GlobalSettings::self()->setShowForgottenAttachmentWarning(
    mMissingAttachmentDetectionCheck->isChecked() );
  GlobalSettings::self()->setAttachmentKeywords(
    mAttachWordsListEditor->stringList() );
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

  //
  // "Composing" tab:
  //
  mComposerCryptoTab = new ComposerCryptoTab();
  addTab( mComposerCryptoTab, i18n("Composing") );

  //
  // "Warnings" tab:
  //
  mWarningTab = new WarningTab();
  addTab( mWarningTab, i18n("Warnings") );

  //
  // "S/MIME Validation" tab:
  //
  mSMimeTab = new SMimeTab();
  addTab( mSMimeTab, i18n("S/MIME Validation") );
}

QString SecurityPage::GeneralTab::helpAnchor() const
{
  return QString::fromLatin1("configure-security-reading");
}

SecurityPageGeneralTab::SecurityPageGeneralTab( QWidget * parent )
  : ConfigModuleTab( parent )
{
  mSGTab.setupUi( this );

  connect( mSGTab.mHtmlMailCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  connect( mSGTab.mExternalReferences, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  connect(mSGTab.labelWarnHTML, SIGNAL(linkActivated ( const QString& )),
          SLOT(slotLinkClicked( const QString& )));

  connect( mSGTab.mAlwaysDecrypt, SIGNAL( stateChanged(int) ),
           this, SLOT( slotEmitChanged() ) );

  // "ignore", "ask", "deny", "always send" radiobutton line:
  mMDNGroup = new QButtonGroup( mSGTab.groupMessageDisp );
  connect( mMDNGroup, SIGNAL( buttonClicked( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  mMDNGroup->addButton( mSGTab.radioIgnore, 0 );
  mMDNGroup->addButton( mSGTab.radioAsk, 1 );
  mMDNGroup->addButton( mSGTab.radioDeny, 2 );
  mMDNGroup->addButton( mSGTab.radioAlways, 3 );

  // "Original Message quote" radiobutton line:
  mOrigQuoteGroup = new QButtonGroup( mSGTab.groupMessageDisp );
  connect( mOrigQuoteGroup, SIGNAL( buttonClicked( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  mOrigQuoteGroup->addButton( mSGTab.radioNothing, 0 );
  mOrigQuoteGroup->addButton( mSGTab.radioFull, 1 );
  mOrigQuoteGroup->addButton( mSGTab.radioHeaders, 2 );

  connect( mSGTab.mNoMDNsWhenEncryptedCheck, SIGNAL(toggled(bool)), SLOT(slotEmitChanged()) );
  connect( mSGTab.labelWarning, SIGNAL(linkActivated ( const QString& ) ),
           SLOT(slotLinkClicked( const QString& )) );

  connect( mSGTab.mAutomaticallyImportAttachedKeysCheck, SIGNAL(toggled(bool)),
           SLOT(slotEmitChanged()) );
}

void SecurityPageGeneralTab::slotLinkClicked( const QString & link )
{
    if ( link == "whatsthis1" )
        QWhatsThis::showText( QCursor::pos(), mSGTab.mHtmlMailCheck->whatsThis() );
    else if (link == "whatsthis2")
        QWhatsThis::showText( QCursor::pos(), mSGTab.mExternalReferences->whatsThis() );
    else if ( link == "whatsthis3" )
        QWhatsThis::showText( QCursor::pos(), mSGTab.radioIgnore->whatsThis() );
}

void SecurityPage::GeneralTab::doLoadOther()
{
  const KConfigGroup reader( KMKernel::config(), "Reader" );

  mSGTab.mHtmlMailCheck->setChecked( MessageViewer::GlobalSettings::self()->htmlMail() );
  mSGTab.mExternalReferences->setChecked( MessageViewer::GlobalSettings::self()->htmlLoadExternal() );
  mSGTab.mAutomaticallyImportAttachedKeysCheck->setChecked(
      reader.readEntry( "AutoImportKeys", false ) );

  mSGTab.mAlwaysDecrypt->setChecked( MessageViewer::GlobalSettings::self()->alwaysDecrypt() );

  const KConfigGroup mdn( KMKernel::config(), "MDN" );

  int num = mdn.readEntry( "default-policy", 0 );
  if ( num < 0 || num >= mMDNGroup->buttons().count() ) num = 0;
  mMDNGroup->button(num)->setChecked(true);
  num = mdn.readEntry( "quote-message", 0 );
  if ( num < 0 || num >= mOrigQuoteGroup->buttons().count() ) num = 0;
  mOrigQuoteGroup->button(num)->setChecked(true);
  mSGTab.mNoMDNsWhenEncryptedCheck->setChecked( MessageViewer::GlobalSettings::self()->notSendWhenEncrypted() );
}

void SecurityPage::GeneralTab::save()
{
  KConfigGroup reader( KMKernel::config(), "Reader" );
  KConfigGroup mdn( KMKernel::config(), "MDN" );

  if ( MessageViewer::GlobalSettings::self()->htmlMail() != mSGTab.mHtmlMailCheck->isChecked())
  {
    if (KMessageBox::warningContinueCancel(this, i18n("Changing the global "
      "HTML setting will override all folder specific values."), QString(),
      KStandardGuiItem::cont(), KStandardGuiItem::cancel(), "htmlMailOverride") == KMessageBox::Continue)
    {
      MessageViewer::GlobalSettings::self()->setHtmlMail( mSGTab.mHtmlMailCheck->isChecked() );
      QList<Akonadi::Collection> collections = kmkernel->allFoldersCollection();
      for (int i = 0; i < collections.size(); ++i)
      {
        QSharedPointer<FolderCollection> fd = FolderCollection::forCollection( collections.at( i ) );
        KConfigGroup config( KMKernel::config(), fd->configGroupName() );
        config.writeEntry("htmlMailOverride", false);
      }
    }
  }
  MessageViewer::GlobalSettings::self()->setHtmlLoadExternal( mSGTab.mExternalReferences->isChecked() );
  reader.writeEntry( "AutoImportKeys", mSGTab.mAutomaticallyImportAttachedKeysCheck->isChecked() );
  mdn.writeEntry( "default-policy", mMDNGroup->checkedId() );
  mdn.writeEntry( "quote-message", mOrigQuoteGroup->checkedId() );
  MessageViewer::GlobalSettings::self()->setNotSendWhenEncrypted( mSGTab.mNoMDNsWhenEncryptedCheck->isChecked() );
  MessageViewer::GlobalSettings::self()->setAlwaysDecrypt( mSGTab.mAlwaysDecrypt->isChecked() );
}


QString SecurityPage::ComposerCryptoTab::helpAnchor() const
{
  return QString::fromLatin1("configure-security-composing");
}

SecurityPageComposerCryptoTab::SecurityPageComposerCryptoTab( QWidget * parent )
  : ConfigModuleTab( parent )
{
  // the margins are inside mWidget itself
  QVBoxLayout* vlay = new QVBoxLayout( this );
  vlay->setSpacing( 0 );
  vlay->setMargin( 0 );

  mWidget = new ComposerCryptoConfiguration( this );
  connect( mWidget->mAutoSignature, SIGNAL( toggled(bool) ), this, SLOT( slotEmitChanged() ) );
  connect( mWidget->mEncToSelf, SIGNAL( toggled(bool) ), this, SLOT( slotEmitChanged() ) );
  connect( mWidget->mShowEncryptionResult, SIGNAL( toggled(bool) ), this, SLOT( slotEmitChanged() ) );
  connect( mWidget->mShowKeyApprovalDlg, SIGNAL( toggled(bool) ), this, SLOT( slotEmitChanged() ) );
  connect( mWidget->mAutoEncrypt, SIGNAL( toggled(bool) ), this, SLOT( slotEmitChanged() ) );
  connect( mWidget->mNeverEncryptWhenSavingInDrafts, SIGNAL( toggled(bool) ), this, SLOT( slotEmitChanged() ) );
  connect( mWidget->mStoreEncrypted, SIGNAL( toggled(bool) ), this, SLOT( slotEmitChanged() ) );
  vlay->addWidget( mWidget );
}

void SecurityPage::ComposerCryptoTab::doLoadOther()
{
  const KConfigGroup composer( KMKernel::config(), "Composer" );

  // If you change default values, sync messagecomposer.cpp too

  mWidget->mAutoSignature->setChecked(
      composer.readEntry( "pgp-auto-sign", false ) );

  mWidget->mEncToSelf->setChecked(
      composer.readEntry( "crypto-encrypt-to-self", true ) );
  mWidget->mShowEncryptionResult->setChecked( false ); //composer.readBoolEntry( "crypto-show-encryption-result", true ) );
  mWidget->mShowEncryptionResult->hide();
  mWidget->mShowKeyApprovalDlg->setChecked(
      composer.readEntry( "crypto-show-keys-for-approval", true ) );

  mWidget->mAutoEncrypt->setChecked(
      composer.readEntry( "pgp-auto-encrypt", false ) );
  mWidget->mNeverEncryptWhenSavingInDrafts->setChecked(
      composer.readEntry( "never-encrypt-drafts", true ) );

  mWidget->mStoreEncrypted->setChecked(
      composer.readEntry( "crypto-store-encrypted", true ) );
}

void SecurityPage::ComposerCryptoTab::save()
{
  KConfigGroup composer( KMKernel::config(), "Composer" );

  composer.writeEntry( "pgp-auto-sign", mWidget->mAutoSignature->isChecked() );

  composer.writeEntry( "crypto-encrypt-to-self", mWidget->mEncToSelf->isChecked() );
  composer.writeEntry( "crypto-show-encryption-result", mWidget->mShowEncryptionResult->isChecked() );
  composer.writeEntry( "crypto-show-keys-for-approval", mWidget->mShowKeyApprovalDlg->isChecked() );

  composer.writeEntry( "pgp-auto-encrypt", mWidget->mAutoEncrypt->isChecked() );
  composer.writeEntry( "never-encrypt-drafts", mWidget->mNeverEncryptWhenSavingInDrafts->isChecked() );

  composer.writeEntry( "crypto-store-encrypted", mWidget->mStoreEncrypted->isChecked() );
}

QString SecurityPage::WarningTab::helpAnchor() const
{
  return QString::fromLatin1("configure-security-warnings");
}

SecurityPageWarningTab::SecurityPageWarningTab( QWidget * parent )
  : ConfigModuleTab( parent )
{
  // the margins are inside mWidget itself
  QVBoxLayout* vlay = new QVBoxLayout( this );
  vlay->setSpacing( 0 );
  vlay->setMargin( 0 );

  mWidget = new WarningConfiguration( this );
  vlay->addWidget( mWidget );

  connect( mWidget->warnGroupBox, SIGNAL(toggled(bool)), SLOT(slotEmitChanged()) );
  connect( mWidget->mWarnUnsigned, SIGNAL(toggled(bool)), SLOT(slotEmitChanged()) );
  connect( mWidget->warnUnencryptedCB, SIGNAL(toggled(bool)), SLOT(slotEmitChanged()) );
  connect( mWidget->warnReceiverNotInCertificateCB, SIGNAL(toggled(bool)), SLOT(slotEmitChanged()) );

  connect( mWidget->enableAllWarningsPB, SIGNAL(clicked()), SLOT(slotReenableAllWarningsClicked()) );
}

void SecurityPage::WarningTab::doLoadOther()
{
  const KConfigGroup composer( KMKernel::config(), "Composer" );

  mWidget->warnUnencryptedCB->setChecked(
      composer.readEntry( "crypto-warning-unencrypted", false ) );
  mWidget->mWarnUnsigned->setChecked(
      composer.readEntry( "crypto-warning-unsigned", false ) );
  mWidget->warnReceiverNotInCertificateCB->setChecked(
      composer.readEntry( "crypto-warn-recv-not-in-cert", true ) );

  // The "-int" part of the key name is because there used to be a separate boolean
  // config entry for enabling/disabling. This is done with the single bool value now.
  mWidget->warnGroupBox->setChecked(
      composer.readEntry( "crypto-warn-when-near-expire", true ) );

  mWidget->mWarnSignKeyExpiresSB->setValue(
      composer.readEntry( "crypto-warn-sign-key-near-expire-int", 14 ) );
  mWidget->mWarnSignKeyExpiresSB->setSuffix(ki18np(" day", " days"));
  mWidget->mWarnSignChainCertExpiresSB->setValue(
      composer.readEntry( "crypto-warn-sign-chaincert-near-expire-int", 14 ) );
  mWidget->mWarnSignChainCertExpiresSB->setSuffix(ki18np(" day", " days"));
  mWidget->mWarnSignRootCertExpiresSB->setValue(
      composer.readEntry( "crypto-warn-sign-root-near-expire-int", 14 ) );
  mWidget->mWarnSignRootCertExpiresSB->setSuffix(ki18np(" day", " days"));

  mWidget->mWarnEncrKeyExpiresSB->setValue(
      composer.readEntry( "crypto-warn-encr-key-near-expire-int", 14 ) );
  mWidget->mWarnEncrKeyExpiresSB->setSuffix(ki18np(" day", " days"));
  mWidget->mWarnEncrChainCertExpiresSB->setValue(
      composer.readEntry( "crypto-warn-encr-chaincert-near-expire-int", 14 ) );
  mWidget->mWarnEncrChainCertExpiresSB->setSuffix(ki18np(" day", " days"));
  mWidget->mWarnEncrRootCertExpiresSB->setValue(
      composer.readEntry( "crypto-warn-encr-root-near-expire-int", 14 ) );
  mWidget->mWarnEncrRootCertExpiresSB->setSuffix(ki18np(" day", " days"));

  mWidget->enableAllWarningsPB->setEnabled( true );
}

void SecurityPage::WarningTab::save()
{
  KConfigGroup composer( KMKernel::config(), "Composer" );

  composer.writeEntry( "crypto-warn-recv-not-in-cert", mWidget->warnReceiverNotInCertificateCB->isChecked() );
  composer.writeEntry( "crypto-warning-unencrypted", mWidget->warnUnencryptedCB->isChecked() );
  composer.writeEntry( "crypto-warning-unsigned", mWidget->mWarnUnsigned->isChecked() );

  composer.writeEntry( "crypto-warn-when-near-expire", mWidget->warnGroupBox->isChecked() );
  composer.writeEntry( "crypto-warn-sign-key-near-expire-int",
                       mWidget->mWarnSignKeyExpiresSB->value() );
  composer.writeEntry( "crypto-warn-sign-chaincert-near-expire-int",
                       mWidget->mWarnSignChainCertExpiresSB->value() );
  composer.writeEntry( "crypto-warn-sign-root-near-expire-int",
                       mWidget->mWarnSignRootCertExpiresSB->value() );

  composer.writeEntry( "crypto-warn-encr-key-near-expire-int",
                       mWidget->mWarnEncrKeyExpiresSB->value() );
  composer.writeEntry( "crypto-warn-encr-chaincert-near-expire-int",
                       mWidget->mWarnEncrChainCertExpiresSB->value() );
  composer.writeEntry( "crypto-warn-encr-root-near-expire-int",
                       mWidget->mWarnEncrRootCertExpiresSB->value() );
}

void SecurityPage::WarningTab::slotReenableAllWarningsClicked()
{
  KMessageBox::enableAllMessages();
  mWidget->enableAllWarningsPB->setEnabled( false );
}

////

QString SecurityPage::SMimeTab::helpAnchor() const
{
  return QString::fromLatin1("configure-security-smime-validation");
}

SecurityPageSMimeTab::SecurityPageSMimeTab( QWidget * parent )
  : ConfigModuleTab( parent )
{
  // the margins are inside mWidget itself
  QVBoxLayout* vlay = new QVBoxLayout( this );
  vlay->setSpacing( 0 );
  vlay->setMargin( 0 );

  mWidget = new SMimeConfiguration( this );
  vlay->addWidget( mWidget );

  // Button-group for exclusive radiobuttons
  QButtonGroup* bg = new QButtonGroup( mWidget );
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

  connect( mWidget->CRLRB, SIGNAL( toggled( bool ) ), this, SLOT( slotEmitChanged() ) );
  connect( mWidget->OCSPRB, SIGNAL( toggled( bool ) ), this, SLOT( slotEmitChanged() ) );
  connect( mWidget->OCSPResponderURL, SIGNAL( textChanged( const QString& ) ), this, SLOT( slotEmitChanged() ) );
  connect( mWidget->OCSPResponderSignature, SIGNAL( changed() ), this, SLOT( slotEmitChanged() ) );
  connect( mWidget->doNotCheckCertPolicyCB, SIGNAL( toggled( bool ) ), this, SLOT( slotEmitChanged() ) );
  connect( mWidget->neverConsultCB, SIGNAL( toggled( bool ) ), this, SLOT( slotEmitChanged() ) );
  connect( mWidget->fetchMissingCB, SIGNAL( toggled( bool ) ), this, SLOT( slotEmitChanged() ) );

  connect( mWidget->ignoreServiceURLCB, SIGNAL( toggled( bool ) ), this, SLOT( slotEmitChanged() ) );
  connect( mWidget->ignoreHTTPDPCB, SIGNAL( toggled( bool ) ), this, SLOT( slotEmitChanged() ) );
  connect( mWidget->disableHTTPCB, SIGNAL( toggled( bool ) ), this, SLOT( slotEmitChanged() ) );
  connect( mWidget->honorHTTPProxyRB, SIGNAL( toggled( bool ) ), this, SLOT( slotEmitChanged() ) );
  connect( mWidget->useCustomHTTPProxyRB, SIGNAL( toggled( bool ) ), this, SLOT( slotEmitChanged() ) );
  connect( mWidget->customHTTPProxy, SIGNAL( textChanged( const QString& ) ), this, SLOT( slotEmitChanged() ) );
  connect( mWidget->ignoreLDAPDPCB, SIGNAL( toggled( bool ) ), this, SLOT( slotEmitChanged() ) );
  connect( mWidget->disableLDAPCB, SIGNAL( toggled( bool ) ), this, SLOT( slotEmitChanged() ) );
  connect( mWidget->customLDAPProxy, SIGNAL( textChanged( const QString& ) ), this, SLOT( slotEmitChanged() ) );

  connect( mWidget->disableHTTPCB, SIGNAL( toggled( bool ) ),
           this, SLOT( slotUpdateHTTPActions() ) );
  connect( mWidget->ignoreHTTPDPCB, SIGNAL( toggled( bool ) ),
           this, SLOT( slotUpdateHTTPActions() ) );

  // Button-group for exclusive radiobuttons
  QButtonGroup* bgHTTPProxy = new QButtonGroup( mWidget );
  bgHTTPProxy->addButton( mWidget->honorHTTPProxyRB );
  bgHTTPProxy->addButton( mWidget->useCustomHTTPProxyRB );

  QDBusConnection::sessionBus().connect(QString(), QString(), "org.kde.kleo.CryptoConfig", "changed",this, SLOT(load()) );
}

SecurityPageSMimeTab::~SecurityPageSMimeTab()
{
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
    mCheckUsingOCSPConfigEntry = configEntry( "gpgsm", "Security", "enable-ocsp", Kleo::CryptoConfigEntry::ArgType_None, false );
    mEnableOCSPsendingConfigEntry = configEntry( "dirmngr", "OCSP", "allow-ocsp", Kleo::CryptoConfigEntry::ArgType_None, false );
    mDoNotCheckCertPolicyConfigEntry = configEntry( "gpgsm", "Security", "disable-policy-checks", Kleo::CryptoConfigEntry::ArgType_None, false );
    mNeverConsultConfigEntry = configEntry( "gpgsm", "Security", "disable-crl-checks", Kleo::CryptoConfigEntry::ArgType_None, false );
    mFetchMissingConfigEntry = configEntry( "gpgsm", "Security", "auto-issuer-key-retrieve", Kleo::CryptoConfigEntry::ArgType_None, false );
    // dirmngr-0.9.0 options
    mIgnoreServiceURLEntry = configEntry( "dirmngr", "OCSP", "ignore-ocsp-service-url", Kleo::CryptoConfigEntry::ArgType_None, false );
    mIgnoreHTTPDPEntry = configEntry( "dirmngr", "HTTP", "ignore-http-dp", Kleo::CryptoConfigEntry::ArgType_None, false );
    mDisableHTTPEntry = configEntry( "dirmngr", "HTTP", "disable-http", Kleo::CryptoConfigEntry::ArgType_None, false );
    mHonorHTTPProxy = configEntry( "dirmngr", "HTTP", "honor-http-proxy", Kleo::CryptoConfigEntry::ArgType_None, false );

    mIgnoreLDAPDPEntry = configEntry( "dirmngr", "LDAP", "ignore-ldap-dp", Kleo::CryptoConfigEntry::ArgType_None, false );
    mDisableLDAPEntry = configEntry( "dirmngr", "LDAP", "disable-ldap", Kleo::CryptoConfigEntry::ArgType_None, false );
    // Other widgets
    mOCSPResponderURLConfigEntry = configEntry( "dirmngr", "OCSP", "ocsp-responder", Kleo::CryptoConfigEntry::ArgType_String, false );
    mOCSPResponderSignature = configEntry( "dirmngr", "OCSP", "ocsp-signer", Kleo::CryptoConfigEntry::ArgType_String, false );
    mCustomHTTPProxy = configEntry( "dirmngr", "HTTP", "http-proxy", Kleo::CryptoConfigEntry::ArgType_String, false );
    mCustomLDAPProxy = configEntry( "dirmngr", "LDAP", "ldap-proxy", Kleo::CryptoConfigEntry::ArgType_String, false );
  }

  Kleo::CryptoConfigEntry* configEntry( const char* componentName,
                                        const char* groupName,
                                        const char* entryName,
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

Kleo::CryptoConfigEntry* SMIMECryptoConfigEntries::configEntry( const char* componentName,
                                                                const char* groupName,
                                                                const char* entryName,
                                                                int /*Kleo::CryptoConfigEntry::ArgType*/ argType,
                                                                bool isList )
{
    Kleo::CryptoConfigEntry* entry = mConfig->entry( componentName, groupName, entryName );
    if ( !entry ) {
        kWarning() << QString("Backend error: gpgconf doesn't seem to know the entry for %1/%2/%3" ).arg( componentName, groupName, entryName );
        return 0;
    }
    if( entry->argType() != argType || entry->isList() != isList ) {
        kWarning() << QString("Backend error: gpgconf has wrong type for %1/%2/%3: %4 %5" ).arg( componentName, groupName, entryName ).arg( entry->argType() ).arg( entry->isList() );
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
}

QString MiscPage::FolderTab::helpAnchor() const
{
  return QString::fromLatin1("Sconfigure-misc-folders");
}

MiscPageFolderTab::MiscPageFolderTab( QWidget * parent )
  : ConfigModuleTab( parent )
{
  mMMTab.setupUi( this );
  mMMTab.gridLayout->setSpacing( KDialog::spacingHint() );
  mMMTab.gridLayout->setMargin( KDialog::marginHint() );
  mMMTab.mStartUpFolderLabel->setBuddy( mMMTab.mOnStartupOpenFolder );
  mMMTab.mExcludeImportantFromExpiry->setWhatsThis(
      i18n( GlobalSettings::self()->excludeImportantMailFromExpiryItem()->whatsThis().toUtf8() ) );

  connect( mMMTab.mEmptyFolderConfirmCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  connect( mMMTab.mExcludeImportantFromExpiry, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  connect( mMMTab.mLoopOnGotoUnread, SIGNAL( activated( int ) ),
          this, SLOT( slotEmitChanged( void ) ) );
  connect( mMMTab.mActionEnterFolder, SIGNAL( activated( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  connect( mMMTab.mDelayedMarkTime, SIGNAL( valueChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  connect( mMMTab.mDelayedMarkAsRead, SIGNAL(toggled(bool)),
           mMMTab.mDelayedMarkTime, SLOT(setEnabled(bool)));
  connect( mMMTab.mDelayedMarkAsRead, SIGNAL(toggled(bool)),
           this , SLOT(slotEmitChanged( void )) );
  connect( mMMTab.mShowPopupAfterDnD, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  connect( mMMTab.mOnStartupOpenFolder, SIGNAL( folderChanged( const Akonadi::Collection& ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  connect( mMMTab.mEmptyTrashCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

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
}

void MiscPage::FolderTab::doLoadOther()
{
  KConfigGroup general( KMKernel::config(), "General" );

  mMMTab.mEmptyTrashCheck->setChecked(
      general.readEntry( "empty-trash-on-exit", false ) );
  mMMTab.mOnStartupOpenFolder->setFolder( general.readEntry( "startupFolder",
      QString::number(kmkernel->inboxCollectionFolder().id() )) );
  mMMTab.mEmptyFolderConfirmCheck->setChecked(
      general.readEntry( "confirm-before-empty", true ) );

}

void MiscPage::FolderTab::save()
{
  KConfigGroup general( KMKernel::config(), "General" );

  general.writeEntry( "empty-trash-on-exit", mMMTab.mEmptyTrashCheck->isChecked() );
  general.writeEntry( "confirm-before-empty", mMMTab.mEmptyFolderConfirmCheck->isChecked() );
  general.writeEntry( "startupFolder", mMMTab.mOnStartupOpenFolder->folderCollection().isValid() ?
                                  QString::number(mMMTab.mOnStartupOpenFolder->folderCollection().id()) : QString() );

  MessageViewer::GlobalSettings::self()->setDelayedMarkAsRead( mMMTab.mDelayedMarkAsRead->isChecked() );
  MessageViewer::GlobalSettings::self()->setDelayedMarkTime( mMMTab.mDelayedMarkTime->value() );
  GlobalSettings::self()->setActionEnterFolder( mMMTab.mActionEnterFolder->currentIndex() );
  GlobalSettings::self()->setLoopOnGotoUnread( mMMTab.mLoopOnGotoUnread->currentIndex() );
  GlobalSettings::self()->setShowPopupAfterDnD( mMMTab.mShowPopupAfterDnD->isChecked() );
  GlobalSettings::self()->setExcludeImportantMailFromExpiry(
        mMMTab.mExcludeImportantFromExpiry->isChecked() );
}

QString MiscPage::InviteTab::helpAnchor() const
{
  return QString::fromLatin1("configure-misc-invites");
}

MiscPageInviteTab::MiscPageInviteTab( QWidget* parent )
  : ConfigModuleTab( parent )
{
  mMITab.setupUi( this );

  mMITab.mDeleteInvitations->setText(
             i18n( GlobalSettings::self()->deleteInvitationEmailsAfterSendingReplyItem()->label().toUtf8() ) );
  mMITab.mDeleteInvitations->setWhatsThis( i18n( GlobalSettings::self()
             ->deleteInvitationEmailsAfterSendingReplyItem()->whatsThis().toUtf8() ) );
  connect( mMITab.mDeleteInvitations, SIGNAL( toggled(bool) ),
           SLOT( slotEmitChanged() ) );

  mMITab.mLegacyMangleFromTo->setWhatsThis( i18n( GlobalSettings::self()->
           legacyMangleFromToHeadersItem()->whatsThis().toUtf8() ) );
  connect( mMITab.mLegacyMangleFromTo, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  mMITab.mLegacyMangleFromTo->setWhatsThis( i18n( GlobalSettings::self()->
           legacyBodyInvitesItem()->whatsThis().toUtf8() ) );
  connect( mMITab.mLegacyBodyInvites, SIGNAL( toggled( bool ) ),
           this, SLOT( slotLegacyBodyInvitesToggled( bool ) ) );
  connect( mMITab.mLegacyBodyInvites, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  mMITab.mExchangeCompatibleInvitations->setWhatsThis( i18n( GlobalSettings::self()->
           exchangeCompatibleInvitationsItem()->whatsThis().toUtf8() ) );
  connect( mMITab.mExchangeCompatibleInvitations, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  mMITab.mOutlookCompatibleInvitationComments->setWhatsThis( i18n( GlobalSettings::self()->
           outlookCompatibleInvitationReplyCommentsItem()->whatsThis().toUtf8() ) );
  connect( mMITab.mOutlookCompatibleInvitationComments, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  mMITab.mAutomaticSending->setWhatsThis( i18n( GlobalSettings::self()->
           automaticSendingItem()->whatsThis().toUtf8() ) );
  connect( mMITab.mAutomaticSending, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
}

void MiscPageInviteTab::slotLegacyBodyInvitesToggled( bool on )
{
  if ( on ) {
    QString txt = i18n( "<qt>Invitations are normally sent as attachments to "
                        "a mail. This switch changes the invitation mails to "
                        "be sent in the text of the mail instead; this is "
                        "necessary to send invitations and replies to "
                        "Microsoft Outlook.<br />But, when you do this, you no "
                        "longer get descriptive text that mail programs "
                        "can read; so, to people who have email programs "
                        "that do not understand the invitations, the "
                        "resulting messages look very odd.<br />People that have email "
                        "programs that do understand invitations will still "
                        "be able to work with this.</qt>" );
    KMessageBox::information( this, txt, QString(), "LegacyBodyInvitesWarning" );
  }
  // Invitations in the body are autosent in any case (no point in editing raw ICAL)
  // So the autosend option is only available if invitations are sent as attachment.
  mMITab.mAutomaticSending->setEnabled( !mMITab.mLegacyBodyInvites->isChecked() );
}

void MiscPage::InviteTab::doLoadFromGlobalSettings()
{
  mMITab.mLegacyMangleFromTo->setChecked( GlobalSettings::self()->legacyMangleFromToHeaders() );
  mMITab.mExchangeCompatibleInvitations->setChecked( GlobalSettings::self()->exchangeCompatibleInvitations() );

  mMITab.mLegacyBodyInvites->blockSignals( true );
  mMITab.mLegacyBodyInvites->setChecked( GlobalSettings::self()->legacyBodyInvites() );
  mMITab.mLegacyBodyInvites->blockSignals( false );

  mMITab.mOutlookCompatibleInvitationComments->setChecked( GlobalSettings::self()->outlookCompatibleInvitationReplyComments() );

  mMITab.mAutomaticSending->setChecked( GlobalSettings::self()->automaticSending() );
  mMITab.mAutomaticSending->setEnabled( !mMITab.mLegacyBodyInvites->isChecked() );
  mMITab.mDeleteInvitations->setChecked(  GlobalSettings::self()->deleteInvitationEmailsAfterSendingReply() );
}

void MiscPage::InviteTab::save()
{
  KConfigGroup groupware( KMKernel::config(), "Groupware" );

  // Write the groupware config
  GlobalSettings::self()->setLegacyMangleFromToHeaders( mMITab.mLegacyMangleFromTo->isChecked() );
  GlobalSettings::self()->setLegacyBodyInvites( mMITab.mLegacyBodyInvites->isChecked() );
  GlobalSettings::self()->setExchangeCompatibleInvitations( mMITab.mExchangeCompatibleInvitations->isChecked() );
  GlobalSettings::self()->setOutlookCompatibleInvitationReplyComments( mMITab.mOutlookCompatibleInvitationComments->isChecked() );
  GlobalSettings::self()->setAutomaticSending( mMITab.mAutomaticSending->isChecked() );
  GlobalSettings::self()->setDeleteInvitationEmailsAfterSendingReply( mMITab.mDeleteInvitations->isChecked() );
}


//----------------------------
#include "configuredialog.moc"
