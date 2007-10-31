/*   -*- mode: C++; c-file-style: "gnu" -*-
 *   kmail: KDE mail client
 *   This file: Copyright (C) 2000 Espen Sand, espen@kde.org
 *              Copyright (C) 2001-2003 Marc Mutz, mutz@kde.org
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

// This must be first
#include <config-kmail.h>
// my headers:
#include "configuredialog.h"
#include "configuredialog_p.h"

#include "globalsettings.h"
#include "replyphrases.h"
#include "templatesconfiguration_kfg.h"

// other KMail headers:
#include "kmkernel.h"
#include "simplestringlisteditor.h"
#include "accountdialog.h"
#include <libkdepim/colorlistbox.h>
#include "kmacctseldlg.h"
using KMail::AccountDialog;
#include "messagesender.h"
#include "kmfoldermgr.h"
#include <kpimidentities/identitymanager.h>
#include "identitylistview.h"
using KMail::IdentityListView;
using KMail::IdentityListViewItem;
#include "kcursorsaver.h"
#include "accountmanager.h"
#include "folderrequester.h"
using KMail::FolderRequester;
#include "accountcombobox.h"
#include "imapaccountbase.h"
using KMail::ImapAccountBase;
#include "folderstorage.h"
#include "kmfolder.h"
#include "kmmainwidget.h"
#include "kmmessagetag.h"
#include "recentaddresses.h"
using KPIM::RecentAddresses;
#include "completionordereditor.h"
#include "ldapclient.h"

#include "templatesconfiguration.h"
#include "customtemplates.h"

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
#include "libkleo/ui/backendconfigwidget.h"
#include "libkleo/ui/keyrequester.h"
#include "libkleo/ui/keyselectiondialog.h"

#include <mailtransport/transportmanagementwidget.h>
using MailTransport::TransportManagementWidget;

// other KDE headers:
#include <klocale.h>
#include <kcharsets.h>
#include <kascii.h>
#include <knuminput.h>
#include <kfontchooser.h>
#include <kmessagebox.h>
#include <kurlrequester.h>
#include <kseparator.h>
#include <kiconloader.h>
#include <kstandarddirs.h>
#include <kwindowsystem.h>
#include <kconfig.h>
#include <kcmultidialog.h>
#include <knotifyconfigwidget.h>
#include <k3resolver.h>
#include <kconfiggroup.h>
#include <kbuttongroup.h>
#include <kcolorcombo.h>
#include <kfontrequester.h>
#include <kicondialog.h>
#include <kkeysequencewidget.h>
#include <KIconButton>
#include <KRandom>
#include <KColorScheme>

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
#include <QLineEdit>
#include <QGroupBox>
#include <QToolTip>
#include <QListWidget>

#include <QValidator>
#include <QVBoxLayout>
#include <QWhatsThis>
#include <QDBusConnection>

//Added by qt3to4:
#include <q3buttongroup.h>
#include <QTextCodec>
#include <q3header.h>
#include <QMenu>

// other headers:
#include <assert.h>
#include <stdlib.h>
#include <kvbox.h>

#ifndef _PATH_SENDMAIL
#define _PATH_SENDMAIL  "/usr/sbin/sendmail"
#endif

#ifdef DIM
#undef DIM
#endif
#define DIM(x) sizeof(x) / sizeof(*x)

namespace {

  struct EnumConfigEntryItem {
    const char * key; // config key value, as appears in config file
    const char * desc; // description, to be i18n()ized
  };
  struct EnumConfigEntry {
    const char * group;
    const char * key;
    const char * desc;
    const EnumConfigEntryItem * items;
    int numItems;
    int defaultItem;
  };
  struct BoolConfigEntry {
    const char * group;
    const char * key;
    const char * desc;
    bool defaultValue;
  };

  static const char * lockedDownWarning =
    I18N_NOOP("<qt><p>This setting has been fixed by your administrator.</p>"
              "<p>If you think this is an error, please contact him.</p></qt>");

  void checkLockDown( QWidget * w, const KConfigGroup & c, const char * key ) {
    if ( c.isEntryImmutable( key ) ) {
      w->setEnabled( false );
      w->setToolTip( i18n( lockedDownWarning ) );
    } else {
	  w->setToolTip("");
    }
  }

  void populateButtonGroup( QGroupBox * box, QButtonGroup * group, int orientation, const EnumConfigEntry & e ) {
    box->setTitle( i18n(e.desc) );
    if (orientation == Qt::Horizontal) {
      box->setLayout( new QHBoxLayout() );
    } else {
      box->setLayout( new QVBoxLayout() );
    }
    box->layout()->setSpacing( KDialog::spacingHint() );
    for (int i = 0; i < e.numItems; ++i) {
      QRadioButton *button = new QRadioButton( i18n(e.items[i].desc), box );
      group->addButton( button, i );
      box->layout()->addWidget( button );
    }
  }

  void populateCheckBox( QCheckBox * b, const BoolConfigEntry & e ) {
    b->setText( i18n( e.desc ) );
  }

  void loadWidget( QCheckBox * b, const KConfigGroup & c, const BoolConfigEntry & e ) {
    Q_ASSERT( c.name() == e.group );
    checkLockDown( b, c, e.key );
    b->setChecked( c.readEntry( e.key, e.defaultValue ) );
  }

  void loadWidget( QGroupBox * box, QButtonGroup * group, const KConfigGroup & c, const EnumConfigEntry & e ) {
    Q_ASSERT( c.name() == e.group );
    Q_ASSERT( group->buttons().size() == e.numItems );
    checkLockDown( box, c, e.key );
    const QString s = c.readEntry( e.key, e.items[e.defaultItem].key );
    for (int i = 0; i < e.numItems; i++) {
      if (s == e.items[i].key) {
        group->buttons()[i]->animateClick();
        return;
      }
    }
    group->buttons()[e.defaultItem]->animateClick();
  }

  void saveCheckBox( QCheckBox * b, KConfigGroup & c, const BoolConfigEntry & e ) {
    Q_ASSERT( c.name() == e.group );
    c.writeEntry( e.key, b->isChecked() );
  }

  void saveButtonGroup( QButtonGroup * group, KConfigGroup & c, const EnumConfigEntry & e ) {
    Q_ASSERT( c.name() == e.group );
    Q_ASSERT( group->buttons().size() == e.numItems );
    if (group->checkedId() != -1) {
      c.writeEntry( e.key, e.items[group->checkedId()].key );
    }
  }

  template <typename T_Widget, typename T_Entry>
  inline void loadProfile( T_Widget * g, const KConfigGroup & c, const T_Entry & e ) {
    if ( c.hasKey( e.key ) )
      loadWidget( g, c, e );
  }

  inline void loadProfile( QGroupBox * box, QButtonGroup * group, const KConfigGroup & c, const EnumConfigEntry & e ) {
    if (c.hasKey( e.key )) {
      loadWidget( box, group, c, e );
    }
  }
}


ConfigureDialog::ConfigureDialog( QWidget *parent, bool modal )
  : KCMultiDialog( parent )
  , mProfileDialog( 0 )
{
  setFaceType( List );
  setButtons( Help | Default | Cancel | Apply | Ok | User1 | User2 );
  setButtonGuiItem( User1, KStandardGuiItem::reset() );
  setButtonGuiItem( User2, KGuiItem( i18n( "&Load Profile..." ) ) );
  setModal( modal );
#ifdef Q_OS_UNIX
  KWindowSystem::setIcons( winId(), qApp->windowIcon().pixmap( IconSize( KIconLoader::Desktop ), IconSize( KIconLoader::Desktop ) ),
                  qApp->windowIcon().pixmap(IconSize( KIconLoader::Small ), IconSize( KIconLoader::Small ) ) );
#endif
  addModule ( "kmail_config_identity" );
  addModule ( "kmail_config_accounts" );
  addModule ( "kmail_config_appearance" );
  addModule ( "kmail_config_composer" );
  addModule ( "kmail_config_security" );
  addModule ( "kmail_config_misc" );

  connect( this, SIGNAL(okClicked()), SLOT(slotOk()) );
  connect( this, SIGNAL(applyClicked()), SLOT(slotApply()) );
  connect( this, SIGNAL(user2Clicked()), SLOT(slotUser2()) );

  // We store the size of the dialog on hide, because otherwise
  // the KCMultiDialog starts with the size of the first kcm, not
  // the largest one. This way at least after the first showing of
  // the largest kcm the size is kept.
  KConfigGroup geometry( KMKernel::config(), "Geometry" );
  int width = geometry.readEntry( "ConfigureDialogWidth", 0 );
  int height = geometry.readEntry( "ConfigureDialogHeight", 0 );
  if ( width != 0 && height != 0 ) {
     setMinimumSize( width, height );
  }

}

void ConfigureDialog::hideEvent( QHideEvent *ev ) {
  KConfigGroup geometry( KMKernel::config(), "Geometry" );
  geometry.writeEntry( "ConfigureDialogWidth", width() );
  geometry.writeEntry( "ConfigureDialogHeight",height() );
  KDialog::hideEvent( ev );
}

ConfigureDialog::~ConfigureDialog() {
}

void ConfigureDialog::slotApply() {
  GlobalSettings::self()->writeConfig();
}

void ConfigureDialog::slotOk() {
  GlobalSettings::self()->writeConfig();
}

void ConfigureDialog::slotUser2() {
  if ( mProfileDialog ) {
    mProfileDialog->raise();
    return;
  }
  mProfileDialog = new ProfileDialog( this );
  mProfileDialog->setObjectName( "mProfileDialog" );
  connect( mProfileDialog, SIGNAL(profileSelected(KConfig*)),
                this, SIGNAL(installProfile(KConfig*)) );
  mProfileDialog->show();
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
  QHBoxLayout *hlay = new QHBoxLayout( this );
  hlay->setSpacing( KDialog::spacingHint() );
  hlay->setMargin( 0 );

  mIdentityList = new IdentityListView( this );
  connect( mIdentityList, SIGNAL( itemSelectionChanged() ),
           SLOT( slotIdentitySelectionChanged() ) );
  connect( this, SIGNAL( changed(bool) ),
           SLOT( slotIdentitySelectionChanged() ) );
  connect( mIdentityList, SIGNAL( rename( KMail::IdentityListViewItem *, const QString & ) ),
           SLOT( slotRenameIdentity(KMail::IdentityListViewItem *, const QString & ) ) );
  connect( mIdentityList, SIGNAL( itemDoubleClicked( QTreeWidgetItem*, int ) ),
           SLOT( slotModifyIdentity() ) );
  connect( mIdentityList, SIGNAL( contextMenu( KMail::IdentityListViewItem *, const QPoint & ) ),
           SLOT( slotContextMenu( KMail::IdentityListViewItem *, const QPoint & ) ) );
  // ### connect dragged(...), ...

  hlay->addWidget( mIdentityList, 1 );

  QVBoxLayout *vlay = new QVBoxLayout(); // inherits spacing
  hlay->addLayout( vlay );

  QPushButton *button = new QPushButton( i18n( "&Add..." ), this );
  mModifyButton = new QPushButton( i18n( "&Modify..." ), this );
  mRenameButton = new QPushButton( i18n( "&Rename" ), this );
  mRemoveButton = new QPushButton( i18n( "Remo&ve" ), this );
  mSetAsDefaultButton = new QPushButton( i18n( "Set as &Default" ), this );
  button->setAutoDefault( false );
  mModifyButton->setAutoDefault( false );
  mModifyButton->setEnabled( false );
  mRenameButton->setAutoDefault( false );
  mRenameButton->setEnabled( false );
  mRemoveButton->setAutoDefault( false );
  mRemoveButton->setEnabled( false );
  mSetAsDefaultButton->setAutoDefault( false );
  mSetAsDefaultButton->setEnabled( false );
  connect( button, SIGNAL( clicked() ),
           this, SLOT( slotNewIdentity() ) );
  connect( mModifyButton, SIGNAL( clicked() ),
           this, SLOT( slotModifyIdentity() ) );
  connect( mRenameButton, SIGNAL( clicked() ),
           this, SLOT( slotRenameIdentity() ) );
  connect( mRemoveButton, SIGNAL( clicked() ),
           this, SLOT( slotRemoveIdentity() ) );
  connect( mSetAsDefaultButton, SIGNAL( clicked() ),
           this, SLOT( slotSetAsDefault() ) );
  vlay->addWidget( button );
  vlay->addWidget( mModifyButton );
  vlay->addWidget( mRenameButton );
  vlay->addWidget( mRemoveButton );
  vlay->addWidget( mSetAsDefaultButton );
  vlay->addStretch( 1 );
  load();
}

void IdentityPage::load()
{
  KPIMIdentities::IdentityManager *im = kmkernel->identityManager();
  mOldNumberOfIdentities = im->shadowIdentities().count();
  // Fill the list:
  mIdentityList->clear();
  QTreeWidgetItem *item = 0;
  for ( KPIMIdentities::IdentityManager::Iterator it = im->modifyBegin(); it != im->modifyEnd(); ++it ) {
    item = new IdentityListViewItem( mIdentityList, item, *it );
  }
  if ( mIdentityList->currentItem() ) {
    mIdentityList->currentItem()->setSelected( true );
  }
}

void IdentityPage::save()
{
  Q_ASSERT( !mIdentityDialog );

  kmkernel->identityManager()->sort();
  kmkernel->identityManager()->commit();

  if( mOldNumberOfIdentities < 2 && mIdentityList->topLevelItemCount() > 1 ) {
    // have more than one identity, so better show the combo in the
    // composer now:
    KConfigGroup composer( KMKernel::config(), "Composer" );
    int showHeaders = composer.readEntry( "headers", HDR_STANDARD );
    showHeaders |= HDR_IDENTITY;
    composer.writeEntry( "headers", showHeaders );
  }
  // and now the reverse
  if( mOldNumberOfIdentities > 1 && mIdentityList->topLevelItemCount() < 2 ) {
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
  NewIdentityDialog dialog( im->shadowIdentities(), this );
  dialog.setObjectName( "new" );

  if( dialog.exec() == QDialog::Accepted ) {
    QString identityName = dialog.identityName().trimmed();
    Q_ASSERT( !identityName.isEmpty() );

    //
    // Construct a new Identity:
    //
    switch ( dialog.duplicateMode() ) {
    case NewIdentityDialog::ExistingEntry:
      {
        KPIMIdentities::Identity &dupThis = im->modifyIdentityForName( dialog.duplicateIdentity() );
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
    if ( mIdentityList->selectedItems().size() > 0 ) {
      item = mIdentityList->selectedItems()[0];
    }

    QTreeWidgetItem * newItem = 0;
    if ( item ) {
      newItem = new IdentityListViewItem( mIdentityList, mIdentityList->itemAbove( item ), newIdent );
    } else {
      newItem = new IdentityListViewItem( mIdentityList, newIdent );
    }

    mIdentityList->selectionModel()->clearSelection();
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
  if ( mIdentityList->selectedItems().size() > 0 ) {
    item = dynamic_cast<IdentityListViewItem*>( mIdentityList->selectedItems()[0] );
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
  kFatal( im->shadowIdentities().count() < 2, 5006 )
    << "Attempted to remove the last identity!";

  IdentityListViewItem *item = 0;
  if ( mIdentityList->selectedItems().size() > 0 ) {
    item = dynamic_cast<IdentityListViewItem*>( mIdentityList->selectedItems()[0] );
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
      if ( mIdentityList->currentItem() ) {
        mIdentityList->currentItem()->setSelected( true );
      }
      refreshList();
    }
  }
}

void IdentityPage::slotRenameIdentity()
{
  Q_ASSERT( !mIdentityDialog );

  QTreeWidgetItem *item = 0;

  if ( mIdentityList->selectedItems().size() > 0 ) {
    item = mIdentityList->selectedItems()[0];
  }
  if ( !item ) return;

  mIdentityList->editItem( item );
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
    if ( mIdentityList->topLevelItemCount() > 1 ) {
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
  if ( mIdentityList->selectedItems().size() > 0 ) {
    item = dynamic_cast<IdentityListViewItem*>( mIdentityList->selectedItems()[0] );
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
  for ( int i = 0; i < mIdentityList->topLevelItemCount(); ++i ) {
    IdentityListViewItem *item = dynamic_cast<IdentityListViewItem*>( mIdentityList->topLevelItem( i ) );
    if ( item ) {
      item->redisplay();
    }
  }
  emit changed( true );
}

void IdentityPage::slotIdentitySelectionChanged()
{
  IdentityListViewItem *item = 0;
  if ( mIdentityList->selectedItems().size() >  0 ) {
    item = dynamic_cast<IdentityListViewItem*>( mIdentityList->selectedItems()[0] );
  }

  mRemoveButton->setEnabled( item && mIdentityList->topLevelItemCount() > 1 );
  mModifyButton->setEnabled( item );
  mRenameButton->setEnabled( item );
  mSetAsDefaultButton->setEnabled( item && !item->identity().isDefault() );
}

// *************************************************************
// *                                                           *
// *                       AccountsPage                         *
// *                                                           *
// *************************************************************
QString AccountsPage::helpAnchor() const {
  return QString::fromLatin1("configure-accounts");
}

AccountsPage::AccountsPage( const KComponentData &instance, QWidget *parent )
  : ConfigModuleWithTabs( instance, parent )
{
  //
  // "Receiving" tab:
  //
  mReceivingTab = new ReceivingTab();
  addTab( mReceivingTab, i18n( "&Receiving" ) );
  connect( mReceivingTab, SIGNAL(accountListChanged(const QStringList &)),
           this, SIGNAL(accountListChanged(const QStringList &)) );

  //
  // "Sending" tab:
  //
  mSendingTab = new SendingTab();
  addTab( mSendingTab, i18n( "&Sending" ) );

  load();
}

AccountsPageSendingTab::~AccountsPageSendingTab()
{
}

QString AccountsPage::SendingTab::helpAnchor() const {
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
  mSendOnCheckCombo = new QComboBox( group );
  mSendOnCheckCombo->setEditable( false );
  mSendOnCheckCombo->addItems( QStringList()
                                      << i18n("Never Automatically")
                                      << i18n("On Manual Mail Checks")
                                      << i18n("On All Mail Checks") );
  glay->addWidget( mSendOnCheckCombo, 1, 1 );
  connect( mSendOnCheckCombo, SIGNAL( activated( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // "default send method" combo:
  mSendMethodCombo = new QComboBox( group );
  mSendMethodCombo->setEditable( false );
  mSendMethodCombo->addItems( QStringList()
                                      << i18n("Send Now")
                                      << i18n("Send Later") );
  glay->addWidget( mSendMethodCombo, 2, 1 );
  connect( mSendMethodCombo, SIGNAL( activated( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );


  // "message property" combo:
  // ### FIXME: remove completely?
  mMessagePropertyCombo = new QComboBox( group );
  mMessagePropertyCombo->setEditable( false );
  mMessagePropertyCombo->addItems( QStringList()
                     << i18n("Allow 8-bit")
                     << i18n("MIME Compliant (Quoted Printable)") );
  glay->addWidget( mMessagePropertyCombo, 3, 1 );
  connect( mMessagePropertyCombo, SIGNAL( activated( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // "default domain" input field:
  mDefaultDomainEdit = new KLineEdit( group );
  glay->addWidget( mDefaultDomainEdit, 4, 1, 1, 2 );
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
  l = new QLabel( i18n("Message &property:"), group );
  l->setBuddy( mMessagePropertyCombo );
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


// adds a number to @p name to make the name unique
static inline QString uniqueName( const QStringList & list,
                                  const QString & name )
{
  int suffix = 1;
  QString result = name;
  while ( list.contains( result )  ) {
    result = i18nc("%1: name; %2: number appended to it to make it unique "
                  "among a list of names", "%1 %2",
        name, suffix );
    suffix++;
  }
  return result;
}

void AccountsPage::SendingTab::doLoadFromGlobalSettings() {
  mSendOnCheckCombo->setCurrentIndex( GlobalSettings::self()->sendOnCheck() );
}

void AccountsPage::SendingTab::doLoadOther() {
  KConfigGroup general( KMKernel::config(), "General");
  KConfigGroup composer( KMKernel::config(), "Composer");

  mSendMethodCombo->setCurrentIndex(
                kmkernel->msgSender()->sendImmediate() ? 0 : 1 );
  mMessagePropertyCombo->setCurrentIndex(
                kmkernel->msgSender()->sendQuotedPrintable() ? 1 : 0 );

  mConfirmSendCheck->setChecked(
      composer.readEntry( "confirm-before-send", false ) );
  QString str = general.readEntry( "Default domain" );
  if( str.isEmpty() ) {
    str = KNetwork::KResolver::localHostName();
  }
  mDefaultDomainEdit->setText( str );
}

void AccountsPage::SendingTab::save() {
  KConfigGroup general( KMKernel::config(), "General" );
  KConfigGroup composer( KMKernel::config(), "Composer" );

  // Save common options:
  GlobalSettings::self()->setSendOnCheck( mSendOnCheckCombo->currentIndex() );
  kmkernel->msgSender()->setSendImmediate(
                             mSendMethodCombo->currentIndex() == 0 );
  kmkernel->msgSender()->setSendQuotedPrintable(
                             mMessagePropertyCombo->currentIndex() == 1 );
  kmkernel->msgSender()->writeConfig( false ); // don't sync
  composer.writeEntry("confirm-before-send", mConfirmSendCheck->isChecked() );
  general.writeEntry( "Default domain", mDefaultDomainEdit->text() );
}

QString AccountsPage::ReceivingTab::helpAnchor() const {
  return QString::fromLatin1("configure-accounts-receiving");
}

AccountsPageReceivingTab::AccountsPageReceivingTab( QWidget * parent )
  : ConfigModuleTab( parent )
{
  // temp. vars:
  QVBoxLayout *vlay;
  QVBoxLayout *btn_vlay;
  QHBoxLayout *hlay;
  QPushButton *button;
  QGroupBox   *group;

  vlay = new QVBoxLayout( this );
  vlay->setSpacing( KDialog::spacingHint() );
  vlay->setMargin( KDialog::marginHint() );

  // label: zero stretch
  vlay->addWidget( new QLabel( i18n("Incoming accounts (add at least one):"), this ) );

  // hbox layout: stretch 10, spacing inherited from vlay
  hlay = new QHBoxLayout();
  vlay->addLayout( hlay, 10 ); // high stretch to suppress groupbox's growing

  // account list: left widget in hlay; stretch 1
  mAccountList = new ListView( this );
  mAccountList->setObjectName( "accountList" );
  mAccountList->setHeaderLabels( QStringList() << "Name" << "Type" << "Folder" );
  mAccountList->setSortingEnabled( false );

  connect( mAccountList->selectionModel(),
           SIGNAL(
             selectionChanged( const QItemSelection &, const QItemSelection &)
           ),
           this,
           SLOT( slotAccountSelected() )
         );
  connect( mAccountList, SIGNAL( itemDoubleClicked( QTreeWidgetItem *, int ) ),
           this, SLOT( slotModifySelectedAccount() ) );
  hlay->addWidget( mAccountList, 1 );

  // a vbox layout for the buttons: zero stretch, spacing inherited from hlay
  btn_vlay = new QVBoxLayout();
  hlay->addLayout( btn_vlay );

  // "add..." button: stretch 0
  button = new QPushButton( i18n("A&dd..."), this );
  button->setAutoDefault( false );
  connect( button, SIGNAL(clicked()),
           this, SLOT(slotAddAccount()) );
  btn_vlay->addWidget( button );

  // "modify..." button: stretch 0
  mModifyAccountButton = new QPushButton( i18n("&Modify..."), this );
  mModifyAccountButton->setAutoDefault( false );
  mModifyAccountButton->setEnabled( false ); // b/c no item is selected yet
  connect( mModifyAccountButton, SIGNAL(clicked()),
           this, SLOT(slotModifySelectedAccount()) );
  btn_vlay->addWidget( mModifyAccountButton );

  // "remove..." button: stretch 0
  mRemoveAccountButton = new QPushButton( i18n("R&emove"), this );
  mRemoveAccountButton->setAutoDefault( false );
  mRemoveAccountButton->setEnabled( false ); // b/c no item is selected yet
  connect( mRemoveAccountButton, SIGNAL(clicked()),
           this, SLOT(slotRemoveSelectedAccount()) );
  btn_vlay->addWidget( mRemoveAccountButton );
  btn_vlay->addStretch( 1 ); // spacer

  mCheckmailStartupCheck = new QCheckBox( i18n("Chec&k mail on startup"), this );
  vlay->addWidget( mCheckmailStartupCheck );
  connect( mCheckmailStartupCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // "New Mail Notification" group box: stretch 0
  group = new QGroupBox( i18n( "New Mail Notification" ), this );
  vlay->addWidget( group );

  group->setLayout( new QVBoxLayout );
  group->layout()->setMargin( KDialog::marginHint() );
  group->layout()->setSpacing( KDialog::spacingHint() );

  // "beep on new mail" check box:
  mBeepNewMailCheck = new QCheckBox(i18n("&Beep"), group );
  mBeepNewMailCheck->setSizePolicy( QSizePolicy( QSizePolicy::MinimumExpanding,
                                                 QSizePolicy::Fixed ) );
  connect( mBeepNewMailCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // "Detailed new mail notification" check box
  mVerboseNotificationCheck =
    new QCheckBox( i18n( "Deta&iled new mail notification" ), group );
  mVerboseNotificationCheck->setSizePolicy( QSizePolicy( QSizePolicy::MinimumExpanding,
                                                         QSizePolicy::Fixed ) );
  mVerboseNotificationCheck->setToolTip(
                 i18n( "Show for each folder the number of newly arrived "
                       "messages" ) );
  mVerboseNotificationCheck->setWhatsThis(
    GlobalSettings::self()->verboseNewMailNotificationItem()->whatsThis() );
  connect( mVerboseNotificationCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged() ) );

  // "Other Actions" button:
  mOtherNewMailActionsButton = new QPushButton( i18n("Other Actio&ns"), group );
  mOtherNewMailActionsButton->setSizePolicy( QSizePolicy( QSizePolicy::Fixed,
                                                          QSizePolicy::Fixed ) );
  connect( mOtherNewMailActionsButton, SIGNAL(clicked()),
           this, SLOT(slotEditNotifications()) );

  group->layout()->addWidget( mBeepNewMailCheck );
  group->layout()->addWidget( mVerboseNotificationCheck );
  group->layout()->addWidget( mOtherNewMailActionsButton );
}

AccountsPageReceivingTab::~AccountsPageReceivingTab()
{
  // When hitting Cancel or closing the dialog with the window-manager-button,
  // we have a number of things to clean up:

  // The newly created accounts
  QList< QPointer<KMAccount> >::Iterator it;
  for (it = mNewAccounts.begin(); it != mNewAccounts.end(); ++it ) {
    delete (*it);
  }
  mNewAccounts.clear();

  // The modified accounts
  QList<ModifiedAccountsType*>::Iterator j;
  for ( j = mModifiedAccounts.begin() ; j != mModifiedAccounts.end() ; ++j ) {
    delete (*j)->newAccount;
    delete (*j);
  }
  mModifiedAccounts.clear();


}

void AccountsPage::ReceivingTab::slotAccountSelected()
{
  QTreeWidgetItem *item = mAccountList->currentItem();
  mModifyAccountButton->setEnabled( item );
  mRemoveAccountButton->setEnabled( item );
}

QStringList AccountsPage::ReceivingTab::occupiedNames()
{
  QStringList accountNames = kmkernel->acctMgr()->getAccounts();

  QList<ModifiedAccountsType*>::Iterator k;
  for (k = mModifiedAccounts.begin(); k != mModifiedAccounts.end(); ++k )
    if ((*k)->oldAccount)
      accountNames.removeAll( (*k)->oldAccount->name() );

  QList< QPointer<KMAccount> >::Iterator l;
  for (l = mAccountsToDelete.begin(); l != mAccountsToDelete.end(); ++l )
    if (*l)
      accountNames.removeAll( (*l)->name() );

  QList< QPointer<KMAccount> >::Iterator it;
  for (it = mNewAccounts.begin(); it != mNewAccounts.end(); ++it )
    if (*it)
      accountNames += (*it)->name();

  QList<ModifiedAccountsType*>::Iterator j;
  for (j = mModifiedAccounts.begin(); j != mModifiedAccounts.end(); ++j )
    accountNames += (*j)->newAccount->name();

  return accountNames;
}

void AccountsPage::ReceivingTab::slotAddAccount() {
  KMAcctSelDlg accountSelectorDialog( this );
  if( accountSelectorDialog.exec() != QDialog::Accepted ) return;

  KAccount::Type accountType = accountSelectorDialog.selected();
  KMAccount *account = kmkernel->acctMgr()->create( accountType );
  if ( !account ) {
    // ### FIXME: Give the user more information. Is this error
    // recoverable?
    KMessageBox::sorry( this, i18n("Unable to create account") );
    return;
  }

  account->init(); // fill the account fields with good default values

  AccountDialog dialog( i18n("Add Account"), account, this );

  QStringList accountNames = occupiedNames();

  if( dialog.exec() != QDialog::Accepted ) {
    delete account;
    return;
  }

  account->deinstallTimer();
  account->setName( uniqueName( accountNames, account->name() ) );

  QTreeWidgetItem *after = mAccountList->topLevelItemCount() > 0 ?
      mAccountList->topLevelItem( mAccountList->topLevelItemCount() - 1 ) :
      0;

  QTreeWidgetItem *listItem = new QTreeWidgetItem( mAccountList, after );
  listItem->setText( 0, account->name() );
  listItem->setText( 1, KAccount::displayNameForType( account->type() ) );
  if( account->folder() )
    listItem->setText( 2, account->folder()->label() );

  mNewAccounts.append( account );
  emit changed( true );
}



void AccountsPage::ReceivingTab::slotModifySelectedAccount()
{
  QTreeWidgetItem *listItem = mAccountList->currentItem();
  if( !listItem ) return;

  KMAccount *account = 0;
  QList<ModifiedAccountsType*>::Iterator j;
  for (j = mModifiedAccounts.begin(); j != mModifiedAccounts.end(); ++j )
    if ( (*j)->newAccount->name() == listItem->text( 0 ) ) {
      account = (*j)->newAccount;
      break;
    }

  if ( !account ) {
    QList< QPointer<KMAccount> >::Iterator it;
    for ( it = mNewAccounts.begin() ; it != mNewAccounts.end() ; ++it )
      if ( (*it)->name() == listItem->text( 0 ) ) {
        account = *it;
        break;
      }

    if ( !account ) {
      account = kmkernel->acctMgr()->findByName( listItem->text( 0 ) );
      if( !account ) {
        // ### FIXME: How should this happen? See above.
        KMessageBox::sorry( this, i18n("Unable to locate account") );
        return;
      }
      if ( account->type() == KAccount::Imap ||
           account->type() == KAccount::DImap )
      {
        ImapAccountBase* ai = static_cast<ImapAccountBase*>( account );
        if ( ai->namespaces().isEmpty() || ai->namespaceToDelimiter().isEmpty() )
        {
          // connect to server - the namespaces are fetched automatically
          kDebug(5006) <<"slotModifySelectedAccount - connect";
          ai->makeConnection();
        }
      }

      ModifiedAccountsType *mod = new ModifiedAccountsType;
      mod->oldAccount = account;
      mod->newAccount = kmkernel->acctMgr()->create( account->type(),
                                                   account->name() );
      mod->newAccount->pseudoAssign( account );
      mModifiedAccounts.append( mod );
      account = mod->newAccount;
    }
  }

  QStringList accountNames = occupiedNames();
  accountNames.removeAll( account->name() );

  AccountDialog dialog( i18n("Modify Account"), account, this );

  if( dialog.exec() != QDialog::Accepted ) return;

  account->setName( uniqueName( accountNames, account->name() ) );

  listItem->setText( 0, account->name() );
  listItem->setText( 1, KAccount::displayNameForType( account->type() ) );
  if( account->folder() )
    listItem->setText( 2, account->folder()->label() );

  emit changed( true );
}



void AccountsPage::ReceivingTab::slotRemoveSelectedAccount() {
  QTreeWidgetItem *listItem = mAccountList->currentItem();
  if( !listItem ) return;

  KMAccount *acct = 0;
  QList<ModifiedAccountsType*>::Iterator j;
  for ( j = mModifiedAccounts.begin() ; j != mModifiedAccounts.end() ; ++j )
    if ( (*j)->newAccount->name() == listItem->text(0) ) {
      acct = (*j)->oldAccount;
      mAccountsToDelete.append( acct );
      mModifiedAccounts.erase( j );
      break;
    }
  if ( !acct ) {
    QList< QPointer<KMAccount> >::Iterator it;
    for ( it = mNewAccounts.begin() ; it != mNewAccounts.end() ; ++it )
      if ( (*it)->name() == listItem->text(0) ) {
        acct = *it;
        mNewAccounts.erase( it );
        break;
      }
  }
  if ( !acct ) {
    acct = kmkernel->acctMgr()->findByName( listItem->text(0) );
    if ( acct )
      mAccountsToDelete.append( acct );
  }
  if ( !acct ) {
    // ### FIXME: see above
    KMessageBox::sorry( this, i18n("<qt>Unable to locate account <b>%1</b>.</qt>",
                         listItem->text(0)) );
    return;
  }

  QTreeWidgetItem *item = mAccountList->itemBelow( listItem );
  if ( !item ) item = mAccountList->itemAbove( listItem );
  delete listItem;

  if ( item ) {
    mAccountList->setCurrentItem( item );
  }

  emit changed( true );
}

void AccountsPage::ReceivingTab::slotEditNotifications()
{
  if(kmkernel->xmlGuiInstance().isValid())
    KNotifyConfigWidget::configure(this,  kmkernel->xmlGuiInstance().componentName());
  else
    KNotifyConfigWidget::configure(this);
}

void AccountsPage::ReceivingTab::doLoadFromGlobalSettings() {
  mVerboseNotificationCheck->setChecked( GlobalSettings::self()->verboseNewMailNotification() );
}

void AccountsPage::ReceivingTab::doLoadOther() {
  KConfigGroup general( KMKernel::config(), "General" );

  mAccountList->clear();
  QTreeWidgetItem *top = 0;

  for( KMAccount *a = kmkernel->acctMgr()->first(); a!=0;
       a = kmkernel->acctMgr()->next() ) {
    QTreeWidgetItem *listItem = new QTreeWidgetItem( mAccountList, top );
    listItem->setText( 0, a->name() );
    listItem->setText( 1, KAccount::displayNameForType( a->type() ) );
    if( a->folder() )
      listItem->setText( 2, a->folder()->label() );
    top = listItem;
  }
  QTreeWidgetItem *listItem = mAccountList->topLevelItemCount() == 0 ?
      0 : mAccountList->topLevelItem( 0 );
  if ( listItem ) {
    mAccountList->setCurrentItem( listItem );
  }

  mBeepNewMailCheck->setChecked( general.readEntry( "beep-on-mail", false ) );
  mCheckmailStartupCheck->setChecked(
      general.readEntry( "checkmail-startup", false ) );
}

void AccountsPage::ReceivingTab::save() {
  // Add accounts marked as new
  QList< QPointer<KMAccount> >::Iterator it;
  for (it = mNewAccounts.begin(); it != mNewAccounts.end(); ++it ) {
    kmkernel->acctMgr()->add( *it ); // calls installTimer too
  }

  // Update accounts that have been modified
  QList<ModifiedAccountsType*>::Iterator j;
  for ( j = mModifiedAccounts.begin() ; j != mModifiedAccounts.end() ; ++j ) {
    (*j)->oldAccount->pseudoAssign( (*j)->newAccount );
    delete (*j)->newAccount;
    delete (*j);
  }
  mModifiedAccounts.clear();

  // Delete accounts marked for deletion
  for ( it = mAccountsToDelete.begin() ;
        it != mAccountsToDelete.end() ; ++it ) {
    kmkernel->acctMgr()->writeConfig( true );
    if ( (*it) && !kmkernel->acctMgr()->remove(*it) )
      KMessageBox::sorry( this, i18n("<qt>Unable to locate account <b>%1</b>.</qt>",
                            (*it)->name() ) );
  }
  mAccountsToDelete.clear();

  // Incoming mail
  kmkernel->acctMgr()->writeConfig( false );
  kmkernel->cleanupImapFolders();

  // Save Mail notification settings
  KConfigGroup general( KMKernel::config(), "General" );
  general.writeEntry( "beep-on-mail", mBeepNewMailCheck->isChecked() );
  GlobalSettings::self()->setVerboseNewMailNotification( mVerboseNotificationCheck->isChecked() );

  general.writeEntry( "checkmail-startup", mCheckmailStartupCheck->isChecked() );

  // Sync new IMAP accounts ASAP:
  for (it = mNewAccounts.begin(); it != mNewAccounts.end(); ++it ) {
    KMAccount *macc = (*it);
    ImapAccountBase *acc = dynamic_cast<ImapAccountBase*> (macc);
    if ( acc ) {
      AccountUpdater *au = new AccountUpdater( acc );
      au->update();
    }
  }
  mNewAccounts.clear();

}

// *************************************************************
// *                                                           *
// *                     AppearancePage                        *
// *                                                           *
// *************************************************************
QString AppearancePage::helpAnchor() const {
  return QString::fromLatin1("configure-appearance");
}

AppearancePage::AppearancePage( const KComponentData &instance, QWidget *parent )
  : ConfigModuleWithTabs( instance, parent )
{
  //
  // "Fonts" tab:
  //
  mFontsTab = new FontsTab();
  addTab( mFontsTab, i18n("&Fonts") );

  //
  // "Colors" tab:
  //
  mColorsTab = new ColorsTab();
  addTab( mColorsTab, i18n("Color&s") );

  //
  // "Layout" tab:
  //
  mLayoutTab = new LayoutTab();
  addTab( mLayoutTab, i18n("La&yout") );

  //
  // "Headers" tab:
  //
  mHeadersTab = new HeadersTab();
  addTab( mHeadersTab, i18n("M&essage List") );

  //
  // "Reader window" tab:
  //
  mReaderTab = new ReaderTab();
  addTab( mReaderTab, i18n("Message W&indow") );

  //
  // "System Tray" tab:
  //
  mSystemTrayTab = new SystemTrayTab();
  addTab( mSystemTrayTab, i18n("System &Tray") );

  //
  // "Message Tag" tab:
  //
  mMessageTagTab = new MessageTagTab();
  addTab( mMessageTagTab, i18n("&Message Tags") );

  load();
}


QString AppearancePage::FontsTab::helpAnchor() const {
  return QString::fromLatin1("configure-appearance-fonts");
}

static const struct {
  const char * configName;
  const char * displayName;
  bool   enableFamilyAndSize;
  bool   onlyFixed;
} fontNames[] = {
  { "body-font", I18N_NOOP("Message Body"), true, false },
  { "list-font", I18N_NOOP("Message List"), true, false },
  { "list-new-font", I18N_NOOP("Message List - New Messages"), true, false },
  { "list-unread-font", I18N_NOOP("Message List - Unread Messages"), true, false },
  { "list-important-font", I18N_NOOP("Message List - Important Messages"), true, false },
  { "list-todo-font", I18N_NOOP("Message List - Todo Messages"), true, false },
  { "list-date-font", I18N_NOOP("Message List - Date Field"), true, false },
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
  mFontLocationCombo = new QComboBox( this );
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
  kDebug(5006) <<"slotFontSelectorChanged() called";
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

void AppearancePage::FontsTab::doLoadOther() {
  KConfigGroup fonts( KMKernel::config(), "Fonts" );

  mFont[0] = KGlobalSettings::generalFont();
  QFont fixedFont = KGlobalSettings::fixedFont();
  for ( int i = 0 ; i < numFontNames ; i++ )
    mFont[i] = fonts.readEntry( fontNames[i].configName,
      (fontNames[i].onlyFixed) ? fixedFont : mFont[0] );

  mCustomFontCheck->setChecked( !fonts.readEntry( "defaultFonts", true ) );
  mFontLocationCombo->setCurrentIndex( 0 );
  slotFontSelectorChanged( 0 );
}

void AppearancePage::FontsTab::installProfile( KConfig * profile ) {
  KConfigGroup fonts( profile, "Fonts" );

  // read fonts that are defined in the profile:
  bool needChange = false;
  for ( int i = 0 ; i < numFontNames ; i++ )
    if ( fonts.hasKey( fontNames[i].configName ) ) {
      needChange = true;
      mFont[i] = fonts.readEntry( fontNames[i].configName, QFont() );
      kDebug(5006) <<"got font \"" << fontNames[i].configName
                << "\" thusly: \"" << mFont[i].toString() << "\"";
    }
  if ( needChange && mFontLocationCombo->currentIndex() > 0 )
    mFontChooser->setFont( mFont[ mFontLocationCombo->currentIndex() ],
      fontNames[ mFontLocationCombo->currentIndex() ].onlyFixed );

  if ( fonts.hasKey( "defaultFonts" ) )
    mCustomFontCheck->setChecked( !fonts.readEntry( "defaultFonts", false ) );
}

void AppearancePage::FontsTab::save() {
  KConfigGroup fonts( KMKernel::config(), "Fonts" );

  // read the current font (might have been modified)
  if ( mActiveFontIndex >= 0 )
    mFont[ mActiveFontIndex ] = mFontChooser->font();

  bool customFonts = mCustomFontCheck->isChecked();
  fonts.writeEntry( "defaultFonts", !customFonts );
  for ( int i = 0 ; i < numFontNames ; i++ )
    if ( customFonts || fonts.hasKey( fontNames[i].configName ) )
      // Don't write font info when we use default fonts, but write
      // if it's already there:
      fonts.writeEntry( fontNames[i].configName, mFont[i] );
}

QString AppearancePage::ColorsTab::helpAnchor() const {
  return QString::fromLatin1("configure-appearance-colors");
}


static const struct {
  const char * configName;
  const char * displayName;
} colorNames[] = { // adjust setup() if you change this:
  { "BackgroundColor", I18N_NOOP("Composer Background") },
  { "AltBackgroundColor", I18N_NOOP("Alternative Background Color") },
  { "ForegroundColor", I18N_NOOP("Normal Text") },
  { "QuotedText1", I18N_NOOP("Quoted Text - First Level") },
  { "QuotedText2", I18N_NOOP("Quoted Text - Second Level") },
  { "QuotedText3", I18N_NOOP("Quoted Text - Third Level") },
  { "LinkColor", I18N_NOOP("Link") },
  { "FollowedColor", I18N_NOOP("Followed Link") },
  { "MisspelledColor", I18N_NOOP("Misspelled Words") },
  { "NewMessage", I18N_NOOP("New Message") },
  { "UnreadMessage", I18N_NOOP("Unread Message") },
  { "FlagMessage", I18N_NOOP("Important Message") },
  { "TodoMessage", I18N_NOOP("Todo Message") },
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
  mColorList = new KPIM::ColorListBox( this );
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
  QLabel *l = new QLabel( i18n("Close to quota threshold"), this );
  hbox->addWidget( l );
  l->setEnabled( false );
  mCloseToQuotaThreshold = new QSpinBox( this );
  mCloseToQuotaThreshold->setRange( 0, 100 );
  mCloseToQuotaThreshold->setSingleStep( 1 );
  connect( mCloseToQuotaThreshold, SIGNAL( valueChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
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

  connect( mCustomColorCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
}

void AppearancePage::ColorsTab::doLoadOther() {
  KConfigGroup reader( KMKernel::config(), "Reader" );

  mCustomColorCheck->setChecked( !reader.readEntry( "defaultColors", true ) );
  mRecycleColorCheck->setChecked( reader.readEntry( "RecycleQuoteColors", false ) );
  mCloseToQuotaThreshold->setValue( GlobalSettings::closeToQuotaThreshold() );

  static const QColor defaultColor[ numColorNames ] = {
    qApp->palette().color( QPalette::Base ), // bg
    KColorScheme( QPalette::Normal, KColorScheme::View ).background(
                  KColorScheme::AlternateBackground ).color() , // alt bg
    qApp->palette().color( QPalette::Text ), // fg
    QColor( 0x00, 0x80, 0x00 ), // quoted l1
    QColor( 0x00, 0x70, 0x00 ), // quoted l2
    QColor( 0x00, 0x60, 0x00 ), // quoted l3
    KColorScheme( QPalette::Normal, KColorScheme::View ).foreground(
                  KColorScheme::LinkText ).color(), // link
    KColorScheme( QPalette::Normal, KColorScheme::View ).foreground(
                  KColorScheme::VisitedText ).color(),// visited link
    Qt::red, // misspelled words
    Qt::red, // new msg
    Qt::blue, // unread mgs
    QColor( 0x00, 0x7F, 0x00 ), // important msg
    Qt::blue, // todo mgs
    QColor( 0x00, 0x80, 0xFF ), // light blue // pgp encrypted
    QColor( 0x40, 0xFF, 0x40 ), // light green // pgp ok, trusted key
    QColor( 0xFF, 0xFF, 0x40 ), // light yellow // pgp ok, untrusted key
    QColor( 0xFF, 0xFF, 0x40 ), // light yellow // pgp unchk
    Qt::red, // pgp bad
    QColor( 0xFF, 0x40, 0x40 ), // warning text color: light red
    Qt::red, // close to quota
    Qt::lightGray, // colorbar plain bg
    Qt::black,     // colorbar plain fg
    Qt::black,     // colorbar html  bg
    Qt::white      // colorbar html  fg
  };

  for ( int i = 0 ; i < numColorNames ; i++ ) {
    mColorList->setColor( i,
      reader.readEntry( colorNames[i].configName, defaultColor[i] ));
  }
  connect( mColorList, SIGNAL( changed( ) ),
           this, SLOT( slotEmitChanged( void ) ) );
}

void AppearancePage::ColorsTab::installProfile( KConfig * profile ) {
  KConfigGroup reader( profile, "Reader" );

  if ( reader.hasKey( "defaultColors" ) )
    mCustomColorCheck->setChecked( !reader.readEntry( "defaultColors", false ) );
  if ( reader.hasKey( "RecycleQuoteColors" ) )
    mRecycleColorCheck->setChecked( reader.readEntry( "RecycleQuoteColors", false ) );

  for ( int i = 0 ; i < numColorNames ; i++ )
    if ( reader.hasKey( colorNames[i].configName ) )
      mColorList->setColor( i,
          reader.readEntry( colorNames[i].configName, QColor() ));
}

void AppearancePage::ColorsTab::save() {
  KConfigGroup reader( KMKernel::config(), "Reader" );

  bool customColors = mCustomColorCheck->isChecked();
  reader.writeEntry( "defaultColors", !customColors );

  for ( int i = 0 ; i < numColorNames ; i++ )
    // Don't write color info when we use default colors, but write
    // if it's already there:
    if ( customColors || reader.hasKey( colorNames[i].configName ) )
      reader.writeEntry( colorNames[i].configName, mColorList->color(i) );

  reader.writeEntry( "RecycleQuoteColors", mRecycleColorCheck->isChecked() );
  GlobalSettings::setCloseToQuotaThreshold( mCloseToQuotaThreshold->value() );
}

QString AppearancePage::LayoutTab::helpAnchor() const {
  return QString::fromLatin1("configure-appearance-layout");
}

static const EnumConfigEntryItem folderListModes[] = {
  { "long", I18N_NOOP("Lon&g folder list") },
  { "short", I18N_NOOP("Shor&t folder list" ) }
};
static const EnumConfigEntry folderListMode = {
  "Geometry", "FolderList", I18N_NOOP("Folder List"),
  folderListModes, DIM(folderListModes), 0
};


static const EnumConfigEntryItem mimeTreeLocations[] = {
  { "top", I18N_NOOP("Abo&ve the message pane") },
  { "bottom", I18N_NOOP("&Below the message pane") }
};
static const EnumConfigEntry mimeTreeLocation = {
  "Reader", "MimeTreeLocation", I18N_NOOP("Message Structure Viewer Placement"),
  mimeTreeLocations, DIM(mimeTreeLocations), 1
};

static const EnumConfigEntryItem mimeTreeModes[] = {
  { "never", I18N_NOOP("Show &never") },
  { "smart", I18N_NOOP("Show only for non-plaintext &messages") },
  { "always", I18N_NOOP("Show alway&s") }
};
static const EnumConfigEntry mimeTreeMode = {
  "Reader", "MimeTreeMode", I18N_NOOP("Message Structure Viewer"),
  mimeTreeModes, DIM(mimeTreeModes), 1
};


static const EnumConfigEntryItem readerWindowModes[] = {
  { "hide", I18N_NOOP("&Do not show a message preview pane") },
  { "below", I18N_NOOP("Show the message preview pane belo&w the message list") },
  { "right", I18N_NOOP("Show the message preview pane ne&xt to the message list") }
};
static const EnumConfigEntry readerWindowMode = {
  "Geometry", "readerWindowMode", I18N_NOOP("Message Preview Pane"),
  readerWindowModes, DIM(readerWindowModes), 1
};

AppearancePageLayoutTab::AppearancePageLayoutTab( QWidget * parent )
  : ConfigModuleTab( parent )
{
  // tmp. vars:
  QVBoxLayout * vlay;

  vlay = new QVBoxLayout( this );
  vlay->setSpacing( KDialog::spacingHint() );
  vlay->setMargin( KDialog::marginHint() );

  // "folder list" radio buttons:
  populateButtonGroup( mFolderListGroupBox = new QGroupBox(this), mFolderListGroup = new QButtonGroup(this), Qt::Horizontal, folderListMode );
  vlay->addWidget( mFolderListGroupBox );
  connect( mFolderListGroup, SIGNAL ( buttonClicked( int ) ),
           this, SLOT( slotEmitChanged() ) );

  mFavoriteFolderViewCB = new QCheckBox( i18n("Show favorite folder view"), this );
  connect( mFavoriteFolderViewCB, SIGNAL(toggled(bool)), SLOT(slotEmitChanged()) );
  vlay->addWidget( mFavoriteFolderViewCB );

  // "show reader window" radio buttons:
  populateButtonGroup( mReaderWindowModeGroupBox = new QGroupBox(this), mReaderWindowModeGroup = new QButtonGroup(this), Qt::Vertical, readerWindowMode );
  vlay->addWidget( mReaderWindowModeGroupBox );
  connect( mReaderWindowModeGroup, SIGNAL ( buttonClicked( int ) ),
           this, SLOT( slotEmitChanged() ) );

  // "Show MIME Tree" radio buttons:
  populateButtonGroup( mMIMETreeModeGroupBox = new QGroupBox(this), mMIMETreeModeGroup = new QButtonGroup(this), Qt::Vertical, mimeTreeMode );
  vlay->addWidget( mMIMETreeModeGroupBox );
  connect( mMIMETreeModeGroup, SIGNAL ( buttonClicked( int ) ),
           this, SLOT( slotEmitChanged() ) );

  // "MIME Tree Location" radio buttons:
  populateButtonGroup( mMIMETreeLocationGroupBox = new QGroupBox(this), mMIMETreeLocationGroup = new QButtonGroup(this), Qt::Horizontal, mimeTreeLocation );
  vlay->addWidget( mMIMETreeLocationGroupBox );
  connect( mMIMETreeLocationGroup, SIGNAL ( buttonClicked( int ) ),
           this, SLOT( slotEmitChanged() ) );

  vlay->addStretch( 10 ); // spacer
}

void AppearancePage::LayoutTab::doLoadOther() {
  const KConfigGroup reader( KMKernel::config(), "Reader" );
  const KConfigGroup geometry( KMKernel::config(), "Geometry" );

  loadWidget( mFolderListGroupBox, mFolderListGroup, geometry, folderListMode );
  loadWidget( mMIMETreeLocationGroupBox, mMIMETreeLocationGroup, reader, mimeTreeLocation );
  loadWidget( mMIMETreeModeGroupBox, mMIMETreeModeGroup, reader, mimeTreeMode );
  loadWidget( mReaderWindowModeGroupBox, mReaderWindowModeGroup, geometry, readerWindowMode );
  mFavoriteFolderViewCB->setChecked( GlobalSettings::self()->enableFavoriteFolderView() );
}

void AppearancePage::LayoutTab::installProfile( KConfig * profile ) {
  const KConfigGroup reader( profile, "Reader" );
  const KConfigGroup geometry( profile, "Geometry" );

  loadProfile( mFolderListGroupBox, mFolderListGroup, geometry, folderListMode );
  loadProfile( mMIMETreeLocationGroupBox, mMIMETreeLocationGroup, reader, mimeTreeLocation );
  loadProfile( mMIMETreeModeGroupBox, mMIMETreeModeGroup, reader, mimeTreeMode );
  loadProfile( mReaderWindowModeGroupBox, mReaderWindowModeGroup, geometry, readerWindowMode );
}

void AppearancePage::LayoutTab::save() {
  KConfigGroup reader( KMKernel::config(), "Reader" );
  KConfigGroup geometry( KMKernel::config(), "Geometry" );

  saveButtonGroup( mFolderListGroup, geometry, folderListMode );
  saveButtonGroup( mMIMETreeLocationGroup, reader, mimeTreeLocation );
  saveButtonGroup( mMIMETreeModeGroup, reader, mimeTreeMode );
  saveButtonGroup( mReaderWindowModeGroup, geometry, readerWindowMode );
  GlobalSettings::self()->setEnableFavoriteFolderView( mFavoriteFolderViewCB->isChecked() );
}

//
// Appearance Message List
//

QString AppearancePage::HeadersTab::helpAnchor() const {
  return QString::fromLatin1("configure-appearance-headers");
}

static const struct {
  const char * displayName;
  DateFormatter::FormatType dateDisplay;
} dateDisplayConfig[] = {
  { I18N_NOOP("Sta&ndard format (%1)"), KMime::DateFormatter::CTime },
  { I18N_NOOP("Locali&zed format (%1)"), KMime::DateFormatter::Localized },
  { I18N_NOOP("Fancy for&mat (%1)"), KMime::DateFormatter::Fancy },
  { I18N_NOOP("C&ustom format (Shift+F1 for help):"),
    KMime::DateFormatter::Custom }
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
  group = new QGroupBox( i18n( "General Options" ), this );
  //  group->layout()->setSpacing( KDialog::spacingHint() );
  QVBoxLayout *gvlay = new QVBoxLayout( group );
  gvlay->setSpacing( KDialog::spacingHint() );

  mMessageSizeCheck = new QCheckBox( i18n("Display messa&ge sizes"), group );
  gvlay->addWidget( mMessageSizeCheck );

  mCryptoIconsCheck = new QCheckBox( i18n( "Show crypto &icons" ), group );
  gvlay->addWidget( mCryptoIconsCheck );

  mAttachmentCheck = new QCheckBox( i18n("Show attachment icon"), group );
  gvlay->addWidget( mAttachmentCheck );

  mNestedMessagesCheck = new QCheckBox( i18n("&Threaded message list"), group );
  gvlay->addWidget( mNestedMessagesCheck );

  connect( mMessageSizeCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  connect( mAttachmentCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  connect( mCryptoIconsCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  connect( mNestedMessagesCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  vlay->addWidget( group );

  // "Message Header Threading Options" group:
  mNestingPolicy = new KButtonGroup( this );
  mNestingPolicy->setTitle( i18n("Threaded Message List Options") );
  //mNestingPolicy->layout()->setSpacing( KDialog::spacingHint() );
  gvlay = new QVBoxLayout( mNestingPolicy );
  gvlay->setSpacing( KDialog::spacingHint() );

  gvlay->addWidget( new QRadioButton( i18n("Always &keep threads open"), mNestingPolicy ) );
  gvlay->addWidget( new QRadioButton( i18n("Threads default to o&pen"), mNestingPolicy ) );
  gvlay->addWidget( new QRadioButton( i18n("Threads default to closed"), mNestingPolicy ) );
  gvlay->addWidget( new QRadioButton( i18n("Open threads that contain ne&w, unread "
                           "or important messages and open watched threads."), mNestingPolicy ) );

  vlay->addWidget( mNestingPolicy );

  connect( mNestingPolicy, SIGNAL( changed( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // "Date Display" group:
  mDateDisplay = new KButtonGroup( this );
  mDateDisplay->setTitle( i18n("Date Display") );
  //mDateDisplay->layout()->setSpacing( KDialog::spacingHint() );
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
      mCustomDateFormatEdit = new KLineEdit( mDateDisplay );
      mCustomDateFormatEdit->setEnabled( false );
      gvlay->addWidget( mCustomDateFormatEdit );
      connect( radio, SIGNAL(toggled(bool)),
               mCustomDateFormatEdit, SLOT(setEnabled(bool)) );
      connect( mCustomDateFormatEdit, SIGNAL(textChanged(const QString&)),
               this, SLOT(slotEmitChanged(void)) );
      QString customDateWhatsThis =
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
             "</string></p> "
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
      mCustomDateFormatEdit->setWhatsThis( customDateWhatsThis );
      radio->setWhatsThis( customDateWhatsThis );
    }
  } // end for loop populating mDateDisplay

  vlay->addWidget( mDateDisplay );
  connect( mDateDisplay, SIGNAL( clicked( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );


  vlay->addStretch( 10 ); // spacer
}

void AppearancePage::HeadersTab::doLoadOther() {
  KConfigGroup general( KMKernel::config(), "General" );
  KConfigGroup geometry( KMKernel::config(), "Geometry" );

  // "General Options":
  mNestedMessagesCheck->setChecked( geometry.readEntry( "nestedMessages", false ) );
  mMessageSizeCheck->setChecked( general.readEntry( "showMessageSize", false ) );
  mCryptoIconsCheck->setChecked( general.readEntry( "showCryptoIcons", false ) );
  mAttachmentCheck->setChecked( general.readEntry( "showAttachmentIcon", true ) );

  // "Message Header Threading Options":
  int num = geometry.readEntry( "nestingPolicy", 3 );
  if ( num < 0 || num > 3 ) num = 3;
  mNestingPolicy->setSelected( num );

  // "Date Display":
  setDateDisplay( general.readEntry( "dateFormat",
                                     (int) DateFormatter::Fancy ),
                  general.readEntry( "customDateFormat" ) );
}

void AppearancePage::HeadersTab::setDateDisplay( int num, const QString & format ) {
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

void AppearancePage::HeadersTab::installProfile( KConfig * profile ) {
  KConfigGroup general( profile, "General" );
  KConfigGroup geometry( profile, "Geometry" );

  if ( geometry.hasKey( "nestedMessages" ) )
    mNestedMessagesCheck->setChecked( geometry.readEntry( "nestedMessages", false ) );
  if ( general.hasKey( "showMessageSize" ) )
    mMessageSizeCheck->setChecked( general.readEntry( "showMessageSize", false ) );

  if( general.hasKey( "showCryptoIcons" ) )
    mCryptoIconsCheck->setChecked( general.readEntry( "showCryptoIcons", false ) );
  if ( general.hasKey( "showAttachmentIcon" ) )
    mAttachmentCheck->setChecked( general.readEntry( "showAttachmentIcon", false ) );

  if ( geometry.hasKey( "nestingPolicy" ) ) {
    int num = geometry.readEntry( "nestingPolicy", 0 );
    if ( num < 0 || num > 3 ) num = 3;
    mNestingPolicy->setSelected( num );
  }

  if ( general.hasKey( "dateFormat" ) )
    setDateDisplay( general.readEntry( "dateFormat", 0 ),
                   general.readEntry( "customDateFormat" ) );
}

void AppearancePage::HeadersTab::save() {
  KConfigGroup general( KMKernel::config(), "General" );
  KConfigGroup geometry( KMKernel::config(), "Geometry" );

  if ( geometry.readEntry( "nestedMessages", false )
       != mNestedMessagesCheck->isChecked() ) {
    int result = KMessageBox::warningContinueCancel( this,
                   i18n("Changing the global threading setting will override "
                        "all folder specific values."),
                   QString(), KStandardGuiItem::cont(), KStandardGuiItem::cancel(),
                   "threadOverride" );
    if ( result == KMessageBox::Continue ) {
      geometry.writeEntry( "nestedMessages", mNestedMessagesCheck->isChecked() );
      // remove all threadMessagesOverride keys from all [Folder-*] groups:
      QStringList groups = KMKernel::config()->groupList().filter( QRegExp("^Folder-") );
      kDebug(5006) <<"groups.count() ==" << groups.count();
      for ( QStringList::const_iterator it = groups.begin() ; it != groups.end() ; ++it ) {
        KConfigGroup group( KMKernel::config(), *it );
        group.deleteEntry( "threadMessagesOverride" );
      }
    }
  }

  geometry.writeEntry( "nestingPolicy", mNestingPolicy->selected() );
  general.writeEntry( "showMessageSize", mMessageSizeCheck->isChecked() );
  general.writeEntry( "showCryptoIcons", mCryptoIconsCheck->isChecked() );
  general.writeEntry( "showAttachmentIcon", mAttachmentCheck->isChecked() );

  int dateDisplayID = mDateDisplay->selected();
  // check bounds:
  assert( dateDisplayID >= 0 ); assert( dateDisplayID < numDateDisplayConfig );
  general.writeEntry( "dateFormat",
                      (int)dateDisplayConfig[ dateDisplayID ].dateDisplay );
  general.writeEntry( "customDateFormat", mCustomDateFormatEdit->text() );
}


//
// Message Window
//


static const BoolConfigEntry showColorbarMode = {
  "Reader", "showColorbar", I18N_NOOP("Show HTML stat&us bar"), false
};

static const BoolConfigEntry showSpamStatusMode = {
  "Reader", "showSpamStatus", I18N_NOOP("Show s&pam status in fancy headers"), true
};

static const BoolConfigEntry showEmoticons = {
  "Reader", "ShowEmoticons", I18N_NOOP("Replace smileys by emoticons"), true
};

static const BoolConfigEntry shrinkQuotes = {
  "Reader", "ShrinkQuotes", I18N_NOOP("Use smaller font for quoted text"), false
};

static const BoolConfigEntry showExpandQuotesMark= {
  "Reader", "ShowExpandQuotesMark", I18N_NOOP("Show expand/collapse quote marks"), false
};


QString AppearancePage::ReaderTab::helpAnchor() const {
  return QString::fromLatin1("configure-appearance-reader");
}

AppearancePageReaderTab::AppearancePageReaderTab( QWidget * parent )
  : ConfigModuleTab( parent )
{
  QVBoxLayout *vlay = new QVBoxLayout( this );
  vlay->setSpacing( KDialog::spacingHint() );
  vlay->setMargin( KDialog::marginHint() );

  // "show colorbar" check box:
  populateCheckBox( mShowColorbarCheck = new QCheckBox( this ), showColorbarMode );
  vlay->addWidget( mShowColorbarCheck );
  connect( mShowColorbarCheck, SIGNAL ( stateChanged( int ) ),
           this, SLOT( slotEmitChanged() ) );

  // "show spam status" check box;
  populateCheckBox( mShowSpamStatusCheck = new QCheckBox( this ), showSpamStatusMode );
  vlay->addWidget( mShowSpamStatusCheck );
  connect( mShowSpamStatusCheck, SIGNAL ( stateChanged( int ) ),
           this, SLOT( slotEmitChanged() ) );

  // "replace smileys by emoticons" check box;
  populateCheckBox( mShowEmoticonsCheck = new QCheckBox( this ), showEmoticons );
  vlay->addWidget( mShowEmoticonsCheck );
  connect( mShowEmoticonsCheck, SIGNAL ( stateChanged( int ) ),
           this, SLOT( slotEmitChanged() ) );

  // "Use smaller font for quoted text" check box
  mShrinkQuotesCheck = new QCheckBox( i18n( shrinkQuotes.desc ), this);
  mShrinkQuotesCheck->setObjectName("kcfg_ShrinkQuotes" );
  vlay->addWidget( mShrinkQuotesCheck );
  connect( mShrinkQuotesCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged() ) );

  // "Show expand/collaps quote marks" check box;
  QHBoxLayout *hlay= new QHBoxLayout(); // inherits spacing
  vlay->addLayout( hlay );
  populateCheckBox( mShowExpandQuotesMark= new QCheckBox( this ), showExpandQuotesMark);
  hlay->addWidget( mShowExpandQuotesMark);
  connect( mShowExpandQuotesMark, SIGNAL ( stateChanged( int ) ),
           this, SLOT( slotEmitChanged() ) );

  hlay->addStretch( 1 );
  mCollapseQuoteLevelSpin = new KIntSpinBox( 0/*min*/,10/*max*/,1/*step*/,
      3/*init*/,this );

  QLabel *label = new QLabel(
           GlobalSettings::self()->collapseQuoteLevelSpinItem()->label(), this );
  label->setBuddy( mCollapseQuoteLevelSpin );

  hlay->addWidget( label );

  mCollapseQuoteLevelSpin->setEnabled( false ); //since !mShowExpandQuotesMark->isCheckec()
  connect(  mCollapseQuoteLevelSpin, SIGNAL( valueChanged( int ) ),
      this, SLOT( slotEmitChanged( void ) ) );
  hlay->addWidget( mCollapseQuoteLevelSpin);

  connect( mShowExpandQuotesMark, SIGNAL( toggled( bool ) ),
      mCollapseQuoteLevelSpin, SLOT( setEnabled( bool ) ) );

  // Fallback Character Encoding
  hlay = new QHBoxLayout(); // inherits spacing
  vlay->addLayout( hlay );
  mCharsetCombo = new QComboBox( this );
  mCharsetCombo->addItems( KMMsgBase::supportedEncodings( false ) );

  connect( mCharsetCombo, SIGNAL( activated( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  QString fallbackCharsetWhatsThis =
    i18n( GlobalSettings::self()->fallbackCharacterEncodingItem()->whatsThis().toUtf8() );
  mCharsetCombo->setWhatsThis( fallbackCharsetWhatsThis );

  label = new QLabel( i18n("Fallback ch&aracter encoding:"), this );
  label->setBuddy( mCharsetCombo );

  hlay->addWidget( label );
  hlay->addWidget( mCharsetCombo );

  // Override Character Encoding
  QHBoxLayout *hlay2 = new QHBoxLayout(); // inherits spacing
  vlay->addLayout( hlay2 );
  mOverrideCharsetCombo = new QComboBox( this );
  QStringList encodings = KMMsgBase::supportedEncodings( false );
  encodings.prepend( i18n( "Auto" ) );
  mOverrideCharsetCombo->addItems( encodings );
  mOverrideCharsetCombo->setCurrentIndex(0);

  connect( mOverrideCharsetCombo, SIGNAL( activated( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  QString overrideCharsetWhatsThis =
    i18n( GlobalSettings::self()->overrideCharacterEncodingItem()->whatsThis().toUtf8() );
  mOverrideCharsetCombo->setWhatsThis( overrideCharsetWhatsThis );

  label = new QLabel( i18n("&Override character encoding:"), this );
  label->setBuddy( mOverrideCharsetCombo );

  hlay2->addWidget( label );
  hlay2->addWidget( mOverrideCharsetCombo );

  vlay->addStretch( 100 ); // spacer
}


void AppearancePage::ReaderTab::readCurrentFallbackCodec()
{
  QStringList encodings = KMMsgBase::supportedEncodings( false );
  QStringList::ConstIterator it( encodings.begin() );
  QStringList::ConstIterator end( encodings.end() );
  QString currentEncoding = GlobalSettings::self()->fallbackCharacterEncoding();
  currentEncoding = currentEncoding.replace( "iso ", "iso-", Qt::CaseInsensitive );
  ///kDebug(5006) <<"Looking for encoding:" << currentEncoding;
  uint i = 0;
  int indexOfLatin9 = 0;
  bool found = false;
  for( ; it != end; ++it)
  {
    const QString encoding = KGlobal::charsets()->encodingForName(*it);
    if ( encoding == "iso-8859-15" )
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
  const QString &currentOverrideEncoding = GlobalSettings::self()->overrideCharacterEncoding();
  if ( currentOverrideEncoding.isEmpty() ) {
    mOverrideCharsetCombo->setCurrentIndex( 0 );
    return;
  }
  QStringList encodings = KMMsgBase::supportedEncodings( false );
  encodings.prepend( i18n( "Auto" ) );
  QStringList::Iterator it( encodings.begin() );
  QStringList::Iterator end( encodings.end() );
  int i = 0;
  for( ; it != end; ++it)
  {
    if( KGlobal::charsets()->encodingForName(*it) == currentOverrideEncoding )
    {
      mOverrideCharsetCombo->setCurrentIndex( i );
      break;
    }
    i++;
  }
  if ( i == encodings.size() ) {
    // the current value of overrideCharacterEncoding is an unknown encoding => reset to Auto
    kWarning(5006) <<"Unknown override character encoding \"" << currentOverrideEncoding
                   << "\". Resetting to Auto.";
    mOverrideCharsetCombo->setCurrentIndex( 0 );
    GlobalSettings::self()->setOverrideCharacterEncoding( QString() );
  }
}

void AppearancePage::ReaderTab::doLoadFromGlobalSettings()
{
  mShowEmoticonsCheck->setChecked( GlobalSettings::self()->showEmoticons() );
  mShrinkQuotesCheck->setChecked( GlobalSettings::self()->shrinkQuotes() );
  mShowExpandQuotesMark->setChecked( GlobalSettings::self()->showExpandQuotesMark() );
  mCollapseQuoteLevelSpin->setValue( GlobalSettings::self()->collapseQuoteLevelSpin() );
  readCurrentFallbackCodec();
  readCurrentOverrideCodec();
}

void AppearancePage::ReaderTab::doLoadOther()
{
  const KConfigGroup reader( KMKernel::config(), "Reader" );
  loadWidget( mShowColorbarCheck, reader, showColorbarMode );
  loadWidget( mShowSpamStatusCheck, reader, showSpamStatusMode );
}


void AppearancePage::ReaderTab::save() {
  KConfigGroup reader( KMKernel::config(), "Reader" );
  saveCheckBox( mShowColorbarCheck, reader, showColorbarMode );
  saveCheckBox( mShowSpamStatusCheck, reader, showSpamStatusMode );
  GlobalSettings::self()->setShowEmoticons( mShowEmoticonsCheck->isChecked() );
  GlobalSettings::self()->setShrinkQuotes( mShrinkQuotesCheck->isChecked() );
  GlobalSettings::self()->setShowExpandQuotesMark( mShowExpandQuotesMark->isChecked() );

  GlobalSettings::self()->setCollapseQuoteLevelSpin( mCollapseQuoteLevelSpin->value() );
  GlobalSettings::self()->setFallbackCharacterEncoding(
      KGlobal::charsets()->encodingForName( mCharsetCombo->currentText() ) );
  GlobalSettings::self()->setOverrideCharacterEncoding(
      mOverrideCharsetCombo->currentIndex() == 0 ?
        QString() :
        KGlobal::charsets()->encodingForName( mOverrideCharsetCombo->currentText() ) );
}


void AppearancePage::ReaderTab::installProfile( KConfig * /* profile */ ) {
  const KConfigGroup reader( KMKernel::config(), "Reader" );
  loadProfile( mShowColorbarCheck, reader, showColorbarMode );
  loadProfile( mShowSpamStatusCheck, reader, showSpamStatusMode );
  loadProfile( mShowEmoticonsCheck, reader, showEmoticons );
  loadProfile( mShrinkQuotesCheck, reader, shrinkQuotes );
  loadProfile( mShowExpandQuotesMark, reader, showExpandQuotesMark);
}


QString AppearancePage::SystemTrayTab::helpAnchor() const {
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

void AppearancePage::SystemTrayTab::doLoadFromGlobalSettings() {
  mSystemTrayCheck->setChecked( GlobalSettings::self()->systemTrayEnabled() );
  mSystemTrayGroup->setSelected( GlobalSettings::self()->systemTrayPolicy() );
  mSystemTrayGroup->setEnabled( mSystemTrayCheck->isChecked() );
}

void AppearancePage::SystemTrayTab::installProfile( KConfig * profile ) {
  KConfigGroup general( profile, "General" );

  if ( general.hasKey( "SystemTrayEnabled" ) ) {
    mSystemTrayCheck->setChecked(
        general.readEntry( "SystemTrayEnabled", false ) );
  }
  if ( general.hasKey( "SystemTrayPolicy" ) ) {
    mSystemTrayGroup->setSelected( general.readEntry( "SystemTrayPolicy", 0 ) );
  }
  mSystemTrayGroup->setEnabled( mSystemTrayCheck->isChecked() );
}

void AppearancePage::SystemTrayTab::save() {
  GlobalSettings::self()->setSystemTrayEnabled( mSystemTrayCheck->isChecked() );
  GlobalSettings::self()->setSystemTrayPolicy( mSystemTrayGroup->selected() );
}

QString AppearancePage::MessageTagTab::helpAnchor() const {
  return QString::fromLatin1("configure-appearance-messagetag");
}

AppearancePageMessageTagTab::AppearancePageMessageTagTab( QWidget * parent )
  : ConfigModuleTab( parent )
{
  mEmitChanges = true;
  mPreviousTag = -1;
  mMsgTagDict = new QHash<QString,KMMessageTagDescription*>();
  mMsgTagList = new QList<KMMessageTagDescription*>();

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

  mTagAddLineEdit = new QLineEdit( mTagsGroupBox );
  addremovegrid->addWidget( mTagAddLineEdit );

  mTagAddButton = new QPushButton( mTagsGroupBox );
  mTagAddButton->setToolTip( i18n("Add new tag") );
  mTagAddButton->setIcon( KIcon( "list-add" ) );
  addremovegrid->addWidget( mTagAddButton );

  mTagRemoveButton = new QPushButton( mTagsGroupBox );
  mTagRemoveButton->setToolTip( i18n("Remove selected tag") );
  mTagRemoveButton->setIcon( KIcon( "list-remove" ) );
  addremovegrid->addWidget( mTagRemoveButton );

  //Up and down buttons
  QHBoxLayout *updowngrid = new QHBoxLayout();
  tageditgrid->addLayout( updowngrid );

  mTagUpButton = new QPushButton( mTagsGroupBox );
  mTagUpButton->setToolTip( i18n("Increase tag priority") );
  mTagUpButton->setIcon( KIcon( "arrow-up" ) );
  mTagUpButton->setAutoRepeat( true );
  updowngrid->addWidget( mTagUpButton );

  mTagDownButton = new QPushButton( mTagsGroupBox );
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
  mTagNameLineEdit = new QLineEdit( mTagSettingGroupBox );
  settings->addWidget( mTagNameLineEdit, 1, 1 );

  QLabel *namelabel = new QLabel( i18n("Name") , mTagSettingGroupBox );
  namelabel->setBuddy( mTagNameLineEdit );
  settings->addWidget( namelabel , 1 , 0 );

  connect( mTagNameLineEdit, SIGNAL( textChanged( const QString& ) ),
            this, SLOT( slotEmitChangeCheck( void ) ) );

  //Second row for text color
  mTextColorCheck = new QCheckBox( i18n("Change Te&xt Color"),
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

  //Third for font selection
  mTextFontCheck = new QCheckBox( i18n("Change Fo&nt"), mTagSettingGroupBox );
  settings->addWidget( mTextFontCheck, 3, 0 );

  mFontRequester = new KFontRequester( mTagSettingGroupBox );
  settings->addWidget( mFontRequester, 3, 1 );

  connect( mTextFontCheck, SIGNAL( toggled( bool ) ),
          mFontRequester, SLOT( setEnabled( bool ) ) );
  connect( mTextFontCheck, SIGNAL( stateChanged( int ) ),
          this, SLOT( slotEmitChangeCheck( void ) ) );
  connect( mFontRequester, SIGNAL( fontSelected( const QFont& ) ),
          this, SLOT( slotEmitChangeCheck( void ) ) );

  //Fourth for toolbar icon
  mIconButton = new KIconButton( mTagSettingGroupBox );
  mIconButton->setIconSize( 16 );
  mIconButton->setIconType( KIconLoader::NoGroup, KIconLoader::Action );
  settings->addWidget( mIconButton, 5, 1 );

  QLabel *iconlabel = new QLabel( i18n("Message Tag &Icon"),
                                  mTagSettingGroupBox );
  iconlabel->setBuddy( mIconButton );
  settings->addWidget( iconlabel, 5, 0 );

  //We do not connect the checkbox to icon selector since icons are used in the
  //menus as well
  connect( mIconButton, SIGNAL( iconChanged( QString ) ),
          this, SLOT( slotEmitChangeCheck( void ) ) );

  //Fifth for shortcut
  mKeySequenceWidget = new KKeySequenceWidget( mTagSettingGroupBox );
  settings->addWidget( mKeySequenceWidget, 6, 1 );
  QLabel *sclabel = new QLabel( i18n("Shortc&ut") , mTagSettingGroupBox );
  sclabel->setBuddy( mKeySequenceWidget );
  settings->addWidget( sclabel, 6, 0 );
  mKeySequenceWidget->setCheckActionList(kmkernel->getKMMainWidget()->actionList());

  connect( mKeySequenceWidget, SIGNAL( keySequenceChanged( const QKeySequence & ) ),
           this, SLOT( slotEmitChangeCheck() ) );

  //Sixth for Toolbar checkbox
  mInToolbarCheck = new QCheckBox( i18n("Enable &Toolbar Button"),
                                   mTagSettingGroupBox );
  settings->addWidget( mInToolbarCheck, 7, 0, 1, 2 );
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
  qDeleteAll(*mMsgTagDict);
  delete mMsgTagDict;
  delete mMsgTagList;
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
  QString tmp_label = mTagListBox->item( first )->text();
  KMMessageTagDescription *tmp_ptr = mMsgTagList->at( first );

  mMsgTagList->replace( first, mMsgTagList->at( second ) );
  mMsgTagList->replace( second, tmp_ptr );

  disconnect( mTagListBox, SIGNAL( itemSelectionChanged() ),
          this, SLOT( slotSelectionChanged() ) );
  mTagListBox->item( first )->setText( mTagListBox->item( second )->text() );
  mTagListBox->item( second )->setText( tmp_label );
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

  KMMessageTagDescription *tmp_desc = mMsgTagList->at( aIndex );

  //First row
  tmp_desc->setName( mTagListBox->item( aIndex )->text() );
  //Second row
  tmp_desc->setTextColor( mTextColorCheck->isChecked() ?
                          mTextColorCombo->color() : QColor() );
  //Third row
  tmp_desc->setTextFont( mTextFontCheck->isChecked() ?
                          mFontRequester->font() : QFont() );
  //Fourth row
  tmp_desc->setIconName( mIconButton->icon() );
  //Fifth row
  mKeySequenceWidget->applyStealShortcut();
  tmp_desc->setShortcut( KShortcut(mKeySequenceWidget->keySequence()) );
  //Sixth row
  tmp_desc->setInToolbar( mInToolbarCheck->isChecked() );
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
    mTextFontCheck->setEnabled( false );
    mInToolbarCheck->setEnabled( false );
    mTextColorCombo->setEnabled( false );
    mFontRequester->setEnabled( false );
    mIconButton->setEnabled( false );
    mKeySequenceWidget->setEnabled( false );

    mEmitChanges = true;
    return;
  }

  mTagRemoveButton->setEnabled( true );
  mTagUpButton->setEnabled( ( 0 != aIndex ) );
  mTagDownButton->setEnabled(
                          ( ( int( mMsgTagList->count() ) - 1 ) != aIndex ) );

  KMMessageTagDescription *tmp_desc = mMsgTagList->at( mTagListBox->currentRow() );

  //1st row
  mTagNameLineEdit->setEnabled( true );
  mTagNameLineEdit->setText( tmp_desc->name() );

  //2nd row
  QColor tmp_color = tmp_desc->textColor();
  mTextColorCheck->setEnabled( true );
  if ( tmp_color.isValid() ) {
    mTextColorCombo->setColor( tmp_color );
    mTextColorCheck->setChecked( true );
  } else {
    mTextColorCheck->setChecked( false );
  }

  //3rd row
  QFont tmp_font = tmp_desc->textFont();
  mTextFontCheck->setEnabled( true );
  mTextFontCheck->setChecked( ( tmp_font != QFont() ) );
  mFontRequester->setFont( tmp_font );

  //4th row
  mIconButton->setEnabled( true );
  mIconButton->setIcon( tmp_desc->toolbarIconName() );

  //5th row
  mKeySequenceWidget->setEnabled( true );
  mKeySequenceWidget->setKeySequence( tmp_desc->shortcut().primary(),
                                      KKeySequenceWidget::NoValidate );

  //6th row
  mInToolbarCheck->setEnabled( true );
  mInToolbarCheck->setChecked( tmp_desc->inToolbar() );

  mEmitChanges = true;
}

void AppearancePage::MessageTagTab::slotSelectionChanged()
{
  //monur: Shouldn't be calling in this case
  //if (aIndex == mPreviousTag)
  //return;
  slotRecordTagSettings( mPreviousTag );
  slotUpdateTagSettingWidgets( mTagListBox->currentRow() );
  mPreviousTag = mTagListBox->currentRow();
}

void AppearancePage::MessageTagTab::slotRemoveTag()
{
  int tmp_index = mTagListBox->currentRow();
  if ( !( tmp_index < 0 ) ) {
    KMMessageTagDescription *tmp_desc = mMsgTagList->takeAt( tmp_index );
    mMsgTagDict->remove( tmp_desc->label() );
    delete tmp_desc;
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
  if ( aText.isEmpty() )
    return;

  //Disconnect so the tag information is not saved and reloaded with every
  //letter
  disconnect( mTagListBox, SIGNAL( itemSelectionChanged() ),
              this, SLOT( slotSelectionChanged() ) );
  mTagListBox->currentItem()->setText( aText );
  connect( mTagListBox, SIGNAL( itemSelectionChanged () ),
           this, SLOT( slotSelectionChanged() ) );
}

void AppearancePage::MessageTagTab::slotAddLineTextChanged( const QString
                                                            &aText )
{
  mTagAddButton->setEnabled( ! aText.isEmpty() );
}

void AppearancePage::MessageTagTab::slotAddNewTag()
{
  int tmp_priority = mMsgTagList->count();
  QString tmp_label = KRandom::randomString( 10 );
    //Very unlikely, but if the tag already exists, regenerate label
  while ( kmkernel->msgTagMgr()->find( tmp_label ) )
    tmp_label = KRandom::randomString( 10 );

  KMMessageTagDescription *tmp_desc = new KMMessageTagDescription( tmp_label,
                                             mTagAddLineEdit->text(),
                                             tmp_priority );
  mMsgTagDict->insert( tmp_desc->label() , tmp_desc );
  mMsgTagList->append( tmp_desc );
  slotEmitChangeCheck();
  mTagAddLineEdit->setText( QString() );
  QListWidgetItem *newItem = new QListWidgetItem( mTagListBox );
  newItem->setText( tmp_desc->name() );
  mTagListBox->addItem( newItem );
  mTagListBox->setCurrentItem( newItem );
}

void AppearancePage::MessageTagTab::doLoadFromGlobalSettings()
{
  mMsgTagDict->clear();
  mMsgTagList->clear();
  mTagListBox->clear();

  //Used for sorting, go through this routine so that mMsgTagList is properly
  //ordered
  KMMessageTagList tmp_list;
  QHashIterator<QString,KMMessageTagDescription*> it( *( kmkernel->msgTagMgr()->msgTagDict() ) );
  while (it.hasNext()) {
    it.next();
    tmp_list.append( it.value()->label() );
  }
  tmp_list.prioritySort();

  //Disconnect so that insertItem's do not trigger an update procedure
  disconnect( mTagListBox, SIGNAL( itemSelectionChanged() ),
              this, SLOT( slotSelectionChanged() ) );
  for ( KMMessageTagList::Iterator itl = tmp_list.begin();
        itl != tmp_list.end(); ++itl ) {
    const KMMessageTagDescription *tmp_desc = kmkernel->msgTagMgr()->find( *itl );
    if ( tmp_desc ) {
      mTagListBox->addItem( tmp_desc->name() );
      KMMessageTagDescription *insert_desc = new KMMessageTagDescription( *tmp_desc );
      mMsgTagDict->insert( *itl , insert_desc );
      mMsgTagList->append( insert_desc );
    }
  }
  connect( mTagListBox, SIGNAL( itemSelectionChanged() ),
           this, SLOT( slotSelectionChanged() ) );
  slotUpdateTagSettingWidgets( -1 );
  //Needed since the previous function doesn't affect add button
  mTagAddButton->setEnabled( false );
}

//Profiling support complicates matters too much
void AppearancePage::MessageTagTab::installProfile( KConfig * /*profile*/ ) { }

void AppearancePage::MessageTagTab::save() {
  slotRecordTagSettings( mTagListBox->currentRow() );
  kmkernel->msgTagMgr()->fillTagsFromList( mMsgTagList );
}


// *************************************************************
// *                                                           *
// *                      ComposerPage                         *
// *                                                           *
// *************************************************************

QString ComposerPage::helpAnchor() const {
  return QString::fromLatin1("configure-composer");
}

ComposerPage::ComposerPage( const KComponentData &instance, QWidget *parent )
  : ConfigModuleWithTabs( instance, parent )
{
  //
  // "General" tab:
  //
  mGeneralTab = new GeneralTab();
  addTab( mGeneralTab, i18n("&General") );
  addConfig( GlobalSettings::self(), mGeneralTab );

  //
  // "Phrases" tab:
  //
  // mPhrasesTab = new PhrasesTab();
  // addTab( mPhrasesTab, i18n("&Phrases") );

  //
  // "Templates" tab:
  //
  mTemplatesTab = new TemplatesTab();
  addTab( mTemplatesTab, i18n("Standard &Templates") );

  //
  // "Custom Templates" tab:
  //
  mCustomTemplatesTab = new CustomTemplatesTab();
  addTab( mCustomTemplatesTab, i18n("&Custom Templates") );

  //
  // "Subject" tab:
  //
  mSubjectTab = new SubjectTab();
  addTab( mSubjectTab, i18n("&Subject") );
  addConfig( GlobalSettings::self(), mSubjectTab );

  //
  // "Charset" tab:
  //
  mCharsetTab = new CharsetTab();
  addTab( mCharsetTab, i18n("Cha&rset") );

  //
  // "Headers" tab:
  //
  mHeadersTab = new HeadersTab();
  addTab( mHeadersTab, i18n("H&eaders") );

  //
  // "Attachments" tab:
  //
  mAttachmentsTab = new AttachmentsTab();
  addTab( mAttachmentsTab, i18nc("Config->Composer->Attachments", "A&ttachments") );
  load();
}

QString ComposerPage::GeneralTab::helpAnchor() const {
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

  mSmartQuoteCheck = new QCheckBox(
           GlobalSettings::self()->smartQuoteItem()->label(), this);
  mSmartQuoteCheck->setObjectName( "kcfg_SmartQuote" );
  vlay->addWidget( mSmartQuoteCheck );
  connect( mSmartQuoteCheck, SIGNAL( stateChanged(int) ),
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

void ComposerPage::GeneralTab::doLoadFromGlobalSettings() {
  // various check boxes:

  mAutoAppSignFileCheck->setChecked(
           GlobalSettings::self()->autoTextSignature()=="auto" );
  mSmartQuoteCheck->setChecked( GlobalSettings::self()->smartQuote() );
  mAutoRequestMDNCheck->setChecked( GlobalSettings::self()->requestMDN() );
  mWordWrapCheck->setChecked( GlobalSettings::self()->wordWrap() );

  mWrapColumnSpin->setValue( GlobalSettings::self()->lineWrapWidth() );
  mAutoSave->setValue( GlobalSettings::self()->autosaveInterval() );

  // editor group:
  mExternalEditorCheck->setChecked( GlobalSettings::self()->useExternalEditor() );
  mEditorRequester->setPath( GlobalSettings::self()->externalEditor() );
}

void ComposerPage::GeneralTab::installProfile( KConfig * profile ) {
  KConfigGroup composer( profile, "Composer" );
  KConfigGroup general( profile, "General" );

  if ( composer.hasKey( "signature" ) ) {
    bool state = composer.readEntry( "signature", false );
    mAutoAppSignFileCheck->setChecked( state );
  }
  if ( composer.hasKey( "smart-quote" ) )
    mSmartQuoteCheck->setChecked( composer.readEntry( "smart-quote", false ) );
  if ( composer.hasKey( "request-mdn" ) )
    mAutoRequestMDNCheck->setChecked( composer.readEntry( "request-mdn", false ) );
  if ( composer.hasKey( "word-wrap" ) )
    mWordWrapCheck->setChecked( composer.readEntry( "word-wrap", false ) );
  if ( composer.hasKey( "break-at" ) )
    mWrapColumnSpin->setValue( composer.readEntry( "break-at", 0 ) );
  if ( composer.hasKey( "autosave" ) )
    mAutoSave->setValue( composer.readEntry( "autosave", 0 ) );

  if ( general.hasKey( "use-external-editor" )
       && general.hasKey( "external-editor" ) ) {
    mExternalEditorCheck->setChecked(
        general.readEntry( "use-external-editor", false ) );
    mEditorRequester->setPath( general.readPathEntry( "external-editor", QString() ) );
  }
}

void ComposerPage::GeneralTab::save() {
  GlobalSettings::self()->setAutoTextSignature(
         mAutoAppSignFileCheck->isChecked() ? "auto" : "manual" );
  GlobalSettings::self()->setSmartQuote( mSmartQuoteCheck->isChecked() );
  GlobalSettings::self()->setRequestMDN( mAutoRequestMDNCheck->isChecked() );
  GlobalSettings::self()->setWordWrap( mWordWrapCheck->isChecked() );

  GlobalSettings::self()->setLineWrapWidth( mWrapColumnSpin->value() );
  GlobalSettings::self()->setAutosaveInterval( mAutoSave->value() );

  // editor group:
  GlobalSettings::self()->setUseExternalEditor( mExternalEditorCheck->isChecked() );
  GlobalSettings::self()->setExternalEditor( mEditorRequester->url().path() );
}

void ComposerPage::GeneralTab::slotConfigureRecentAddresses( )
{
  KPIM::RecentAddressDialog dlg( this );
  dlg.setAddresses( RecentAddresses::self( KMKernel::config() )->addresses() );
  if ( dlg.exec() ) {
    RecentAddresses::self( KMKernel::config() )->clear();
    const QStringList &addrList = dlg.addresses();
    QStringList::ConstIterator it;
    for ( it = addrList.constBegin(); it != addrList.constEnd(); ++it )
      RecentAddresses::self( KMKernel::config() )->add( *it );
  }
}

void ComposerPage::GeneralTab::slotConfigureCompletionOrder( )
{
  KPIM::LdapSearch search;
  KPIM::CompletionOrderEditor editor( &search, this );
  editor.exec();
}

QString ComposerPage::PhrasesTab::helpAnchor() const {
  return QString::fromLatin1("configure-composer-phrases");
}

ComposerPagePhrasesTab::ComposerPagePhrasesTab( QWidget * parent )
  : ConfigModuleTab( parent )
{
  // tmp. vars:
  QGridLayout *glay;
  QPushButton *button;

  glay = new QGridLayout( this );
  glay->setSpacing( KDialog::spacingHint() );
  glay->setMargin( KDialog::marginHint() );
  glay->setColumnStretch( 1, 1 );
  glay->setColumnStretch( 2, 1 );
  glay->setRowStretch( 7, 1 );

  // row 0: help text
  glay->addWidget( new QLabel( i18n("<qt>The following placeholders are "
				    "supported in the reply phrases:<br />"
				    "<table>"
				    "<tr><td><b>%D</b>: date</td>"
					"<td><b>%S</b>: subject</td>"
					"<td></td></tr>"
				    "<tr><td><b>%e</b>: sender's address</td>"
					"<td><b>%F</b>: sender's name</td>"
					"<td><b>%f</b>: sender's initials</td></tr>"
				    "<tr><td><b>%T</b>: recipient's name</td>"
					"<td><b>%t</b>: recipient's name and address</td>"
					"<td></td></tr>"
				    "<tr><td><b>%C</b>: carbon copy names</td>"
					"<td><b>%c</b>: carbon copy names and addresses</td>"
					"<td></td></tr>"
				    "<tr><td><b>%%</b>: percent sign</td>"
					"<td><b>%_</b>: space</td>"
					"<td><b>%L</b>: linebreak</td></tr>"
				    "</table></qt>"), this ),
                            0, 0, 0, 3 ); // row 0; cols 0..2

  // row 1: label and language combo box:
  mPhraseLanguageCombo = new LanguageComboBox( this );
  mPhraseLanguageCombo->setEditable( false );
  QLabel *l = new QLabel( i18n("Lang&uage:"), this );
  l->setBuddy( mPhraseLanguageCombo );
  glay->addWidget( l, 1, 0 );
  glay->addWidget( mPhraseLanguageCombo, 1, 1, 1, 2 );
  connect( mPhraseLanguageCombo, SIGNAL(activated(const QString&)),
           this, SLOT(slotLanguageChanged(const QString&)) );

  // row 2: "add..." and "remove" push buttons:
  button = new QPushButton( i18n("A&dd..."), this );
  button->setAutoDefault( false );
  glay->addWidget( button, 2, 1 );
  mRemoveButton = new QPushButton( i18n("Re&move"), this );
  mRemoveButton->setAutoDefault( false );
  mRemoveButton->setEnabled( false ); // combo doesn't contain anything...
  glay->addWidget( mRemoveButton, 2, 2 );
  connect( button, SIGNAL(clicked()),
           this, SLOT(slotNewLanguage()) );
  connect( mRemoveButton, SIGNAL(clicked()),
           this, SLOT(slotRemoveLanguage()) );

  // row 3: "reply to sender" line edit and label:
  mPhraseReplyEdit = new KLineEdit( this );
  connect( mPhraseReplyEdit, SIGNAL( textChanged( const QString& ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  l = new QLabel( i18n("Reply to se&nder:"), this );
  l->setBuddy( mPhraseReplyEdit );
  glay->addWidget( l, 3, 0 );
  glay->addWidget( mPhraseReplyEdit, 3, 1, 1, 2 ); // cols 1..2

  // row 4: "reply to all" line edit and label:
  mPhraseReplyAllEdit = new KLineEdit( this );
  connect( mPhraseReplyAllEdit, SIGNAL( textChanged( const QString& ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  l = new QLabel(  i18n("Repl&y to all:"), this );
  l->setBuddy( mPhraseReplyAllEdit );
  glay->addWidget( l, 4, 0 );
  glay->addWidget( mPhraseReplyAllEdit, 4, 1, 1, 2 ); // cols 1..2

  // row 5: "forward" line edit and label:
  mPhraseForwardEdit = new KLineEdit( this );
  connect( mPhraseForwardEdit, SIGNAL( textChanged( const QString& ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  l = new QLabel( i18n("&Forward:"), this );
  l->setBuddy( mPhraseForwardEdit );
  glay->addWidget( l, 5, 0 );
  glay->addWidget( mPhraseForwardEdit, 5, 1, 1, 2 ); // cols 1..2

  // row 6: "quote indicator" line edit and label:
  mPhraseIndentPrefixEdit = new KLineEdit( this );
  connect( mPhraseIndentPrefixEdit, SIGNAL( textChanged( const QString& ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  l = new QLabel( i18n("&Quote indicator:"), this );
  l->setBuddy( mPhraseIndentPrefixEdit );
  glay->addWidget( l, 6, 0 );
  glay->addWidget( mPhraseIndentPrefixEdit, 6, 1, 1, 2 );

  // row 7: spacer
}


void ComposerPage::PhrasesTab::setLanguageItemInformation( int index ) {
  assert( 0 <= index && index < (int)mLanguageList.count() );

  LanguageItem l = mLanguageList.at( index );

  mPhraseReplyEdit->setText( l.mReply );
  mPhraseReplyAllEdit->setText( l.mReplyAll );
  mPhraseForwardEdit->setText( l.mForward );
  mPhraseIndentPrefixEdit->setText( l.mIndentPrefix );
}

void ComposerPage::PhrasesTab::saveActiveLanguageItem() {
  int index = mActiveLanguageItem;
  if (index == -1) return;
  assert( 0 <= index && index < (int)mLanguageList.count() );

  LanguageItem l = mLanguageList.at( index );

  l.mReply = mPhraseReplyEdit->text();
  l.mReplyAll = mPhraseReplyAllEdit->text();
  l.mForward = mPhraseForwardEdit->text();
  l.mIndentPrefix = mPhraseIndentPrefixEdit->text();
}

void ComposerPage::PhrasesTab::slotNewLanguage()
{
  NewLanguageDialog dialog( mLanguageList, parentWidget() );
  dialog.setObjectName( "New" );
  if ( dialog.exec() == QDialog::Accepted ) slotAddNewLanguage( dialog.language() );
}

void ComposerPage::PhrasesTab::slotAddNewLanguage( const QString& lang )
{
  mPhraseLanguageCombo->setCurrentIndex(
    mPhraseLanguageCombo->insertLanguage( lang ) );
  KLocale locale("kmail", lang);
  mLanguageList.append(
     LanguageItem( lang,
                   ki18n("On %D, you wrote:").toString( &locale ),
                   ki18n("On %D, %F wrote:").toString( &locale ),
                   ki18n("Forwarded Message").toString( &locale ),
                   ki18n(">%_").toString( &locale ) ) );
  mRemoveButton->setEnabled( true );
  slotLanguageChanged( QString() );
}

void ComposerPage::PhrasesTab::slotRemoveLanguage()
{
  assert( mPhraseLanguageCombo->count() > 1 );
  int index = mPhraseLanguageCombo->currentIndex();
  assert( 0 <= index && index < (int)mLanguageList.count() );

  // remove current item from internal list and combobox:
  mLanguageList.removeAt(  index );
  mPhraseLanguageCombo->removeItem( index );

  if ( index >= (int)mLanguageList.count() ) index--;

  mActiveLanguageItem = index;
  setLanguageItemInformation( index );
  mRemoveButton->setEnabled( mLanguageList.count() > 1 );
  emit changed( true );
}

void ComposerPage::PhrasesTab::slotLanguageChanged( const QString& )
{
  int index = mPhraseLanguageCombo->currentIndex();
  assert( index < (int)mLanguageList.count() );
  saveActiveLanguageItem();
  mActiveLanguageItem = index;
  setLanguageItemInformation( index );
  emit changed( true );
}


void ComposerPage::PhrasesTab::doLoadFromGlobalSettings() {
  mLanguageList.clear();
  mPhraseLanguageCombo->clear();
  mActiveLanguageItem = -1;

  int numLang = GlobalSettings::self()->replyLanguagesCount();
  int currentNr = GlobalSettings::self()->replyCurrentLanguage();

  // build mLanguageList and mPhraseLanguageCombo:
  for ( int i = 0 ; i < numLang ; i++ ) {
    ReplyPhrases replyPhrases( QString::number(i) );
    replyPhrases.readConfig();
    QString lang = replyPhrases.language();
    mLanguageList.append(
         LanguageItem( lang,
                       replyPhrases.phraseReplySender(),
                       replyPhrases.phraseReplyAll(),
                       replyPhrases.phraseForward(),
                       replyPhrases.indentPrefix() ) );
    mPhraseLanguageCombo->insertLanguage( lang );
  }

  if ( currentNr >= numLang || currentNr < 0 )
    currentNr = 0;

  if ( numLang == 0 ) {
    slotAddNewLanguage( KGlobal::locale()->language() );
  }

  mPhraseLanguageCombo->setCurrentIndex( currentNr );
  mActiveLanguageItem = currentNr;
  setLanguageItemInformation( currentNr );
  mRemoveButton->setEnabled( mLanguageList.count() > 1 );
}

void ComposerPage::PhrasesTab::save() {
  GlobalSettings::self()->setReplyLanguagesCount( mLanguageList.count() );
  GlobalSettings::self()->setReplyCurrentLanguage( mPhraseLanguageCombo->currentIndex() );

  saveActiveLanguageItem();
  LanguageItemList::Iterator it = mLanguageList.begin();
  for ( int i = 0 ; it != mLanguageList.end() ; ++it, ++i ) {
    ReplyPhrases replyPhrases( QString::number(i) );
    replyPhrases.setLanguage( (*it).mLanguage );
    replyPhrases.setPhraseReplySender( (*it).mReply );
    replyPhrases.setPhraseReplyAll( (*it).mReplyAll );
    replyPhrases.setPhraseForward( (*it).mForward );
    replyPhrases.setIndentPrefix( (*it).mIndentPrefix );
    replyPhrases.writeConfig();
  }
}

QString ComposerPage::TemplatesTab::helpAnchor() const {
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

void ComposerPage::TemplatesTab::doLoadFromGlobalSettings() {
    mWidget->loadFromGlobal();
}

void ComposerPage::TemplatesTab::save() {
    mWidget->saveToGlobal();
}

QString ComposerPage::CustomTemplatesTab::helpAnchor() const {
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

void ComposerPage::CustomTemplatesTab::doLoadFromGlobalSettings() {
    mWidget->load();
}

void ComposerPage::CustomTemplatesTab::save() {
    mWidget->save();
}

QString ComposerPage::SubjectTab::helpAnchor() const {
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

void ComposerPage::SubjectTab::doLoadFromGlobalSettings() {
  mReplyListEditor->setStringList( GlobalSettings::self()->replyPrefixes() );
  mReplaceReplyPrefixCheck->setChecked( GlobalSettings::self()->replaceReplyPrefix() );
  mForwardListEditor->setStringList( GlobalSettings::self()->forwardPrefixes() );
  mReplaceForwardPrefixCheck->setChecked( GlobalSettings::self()->replaceForwardPrefix() );
}

void ComposerPage::SubjectTab::save() {
  GlobalSettings::self()->setReplyPrefixes( mReplyListEditor->stringList() );
  GlobalSettings::self()->setForwardPrefixes( mForwardListEditor->stringList() );
}

QString ComposerPage::CharsetTab::helpAnchor() const {
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

void ComposerPage::CharsetTab::slotVerifyCharset( QString & charset ) {
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

void ComposerPage::CharsetTab::doLoadOther() {
  KConfigGroup composer( KMKernel::config(), "Composer" );

  QStringList charsets = composer.readEntry( "pref-charsets" , QStringList() );
  for ( QStringList::Iterator it = charsets.begin() ;
        it != charsets.end() ; ++it )
    if ( (*it) == QString::fromLatin1("locale") ) {
      QByteArray cset = kmkernel->networkCodec()->name();
      kAsciiToLower( cset.data() );
      (*it) = QString("%1 (locale)").arg( QString::fromLatin1( cset ) );
    }

  mCharsetListEditor->setStringList( charsets );
  mKeepReplyCharsetCheck->setChecked(
      !composer.readEntry( "force-reply-charset", false ) );
}

void ComposerPage::CharsetTab::save() {
  KConfigGroup composer( KMKernel::config(), "Composer" );

  QStringList charsetList = mCharsetListEditor->stringList();
  QStringList::Iterator it = charsetList.begin();
  for ( ; it != charsetList.end() ; ++it )
    if ( (*it).endsWith("(locale)") )
      (*it) = "locale";
  composer.writeEntry( "pref-charsets", charsetList );
  composer.writeEntry( "force-reply-charset",
                       !mKeepReplyCharsetCheck->isChecked() );
}

QString ComposerPage::HeadersTab::helpAnchor() const {
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
  mTagList->setHeaderLabels( QStringList() << i18n("Name") << i18n("Value") );
  mTagList->setSortingEnabled( false );
  connect( mTagList, SIGNAL(itemSelectionChanged()),
           this, SLOT(slotMimeHeaderSelectionChanged()) );
  glay->addWidget( mTagList, 0, 0, 3, 2 );

  // "new" and "remove" buttons:
  button = new QPushButton( i18n("Ne&w"), this );
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
  mTagNameLabel = new QLabel( i18n("&Name:"), this );
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


void ComposerPage::HeadersTab::slotMimeHeaderNameChanged( const QString & text ) {
  // is called on ::setup(), when clearing the line edits. So be
  // prepared to not find a selection:
  QTreeWidgetItem * item = mTagList->currentItem();
  if ( item )
    item->setText( 0, text );
  emit changed( true );
}


void ComposerPage::HeadersTab::slotMimeHeaderValueChanged( const QString & text ) {
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
    kDebug(5006) <<"=================================================="
                  << "Error: Remove button was pressed although no custom header was selected\n"
                  << "==================================================\n";
    return;
  }

  QTreeWidgetItem *below = mTagList->itemBelow( item );

  if ( below ) {
    kDebug(5006) <<"below";
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

void ComposerPage::HeadersTab::doLoadOther() {
  KConfigGroup general( KMKernel::config(), "General" );

  QString suffix = general.readEntry( "myMessageIdSuffix" );
  mMessageIdSuffixEdit->setText( suffix );
  bool state = ( !suffix.isEmpty() &&
      general.readEntry( "useCustomMessageIdSuffix", false ) );
  mCreateOwnMessageIdCheck->setChecked( state );

  mTagList->clear();
  mTagNameEdit->clear();
  mTagValueEdit->clear();

  QTreeWidgetItem * item = 0;

  int count = general.readEntry( "mime-header-count", 0 );
  for( int i = 0 ; i < count ; i++ ) {
    KConfigGroup config( KMKernel::config(),
                         QByteArray("Mime #") + QByteArray().setNum(i) );
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

void ComposerPage::HeadersTab::save() {
  KConfigGroup general( KMKernel::config(), "General" );

  general.writeEntry( "useCustomMessageIdSuffix",
                      mCreateOwnMessageIdCheck->isChecked() );
  general.writeEntry( "myMessageIdSuffix",
                      mMessageIdSuffixEdit->text() );

  int numValidEntries = 0;
  QTreeWidgetItem *item = 0;
  for ( int i = 0; i < mTagList->topLevelItemCount(); ++i ) {
    item = mTagList->topLevelItem( i );
    if( !item->text(0).isEmpty() ) {
      KConfigGroup config( KMKernel::config(), QByteArray("Mime #")
                             + QByteArray().setNum( numValidEntries ) );
      config.writeEntry( "name",  item->text( 0 ) );
      config.writeEntry( "value", item->text( 1 ) );
      numValidEntries++;
    }
  }
  general.writeEntry( "mime-header-count", numValidEntries );
}

QString ComposerPage::AttachmentsTab::helpAnchor() const {
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

void ComposerPage::AttachmentsTab::doLoadFromGlobalSettings() {
  mOutlookCompatibleCheck->setChecked(
    GlobalSettings::self()->outlookCompatibleAttachments() );
  mMissingAttachmentDetectionCheck->setChecked(
    GlobalSettings::self()->showForgottenAttachmentWarning() );
  QStringList attachWordsList = GlobalSettings::self()->attachmentKeywords();
  if ( attachWordsList.isEmpty() ) {
    // default value
    attachWordsList << QString::fromLatin1("attachment")
                    << QString::fromLatin1("attached");
    if ( QString::fromLatin1("attachment") != i18n("attachment") )
      attachWordsList << i18n("attachment");
    if ( QString::fromLatin1("attached") != i18n("attached") )
      attachWordsList << i18n("attached");
  }

  mAttachWordsListEditor->setStringList( attachWordsList );
}

void ComposerPage::AttachmentsTab::save() {
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
QString SecurityPage::helpAnchor() const {
  return QString::fromLatin1("configure-security");
}

SecurityPage::SecurityPage( const KComponentData &instance, QWidget *parent )
  : ConfigModuleWithTabs( instance, parent )
{
  //
  // "Reading" tab:
  //
  mGeneralTab = new GeneralTab(); //  @TODO: rename
  addTab( mGeneralTab, i18n("&Reading") );

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
  addTab( mSMimeTab, i18n("S/MIME &Validation") );

  //
  // "Crypto Backends" tab:
  //
  mCryptPlugTab = new CryptPlugTab();
  addTab( mCryptPlugTab, i18n("Crypto Backe&nds") );
  load();
}


void SecurityPage::installProfile( KConfig * profile ) {
  mGeneralTab->installProfile( profile );
  mComposerCryptoTab->installProfile( profile );
  mWarningTab->installProfile( profile );
  mSMimeTab->installProfile( profile );
}

QString SecurityPage::GeneralTab::helpAnchor() const {
  return QString::fromLatin1("configure-security-reading");
}

SecurityPageGeneralTab::SecurityPageGeneralTab( QWidget * parent )
  : ConfigModuleTab( parent )
{
  // tmp. vars:
  QVBoxLayout  *vlay;
  QGroupBox    *group;
  QVBoxLayout  *vboxlayout;
  QRadioButton *radio;
  QLabel       *label;
  QString       msg;

  vlay = new QVBoxLayout( this );
  vlay->setSpacing( KDialog::spacingHint() );
  vlay->setMargin( KDialog::marginHint() );

  // QWhat'sThis texts
  mHtmlWhatsThis = i18n( "<qt><p>Messages sometimes come in both formats. "
              "This option controls whether you want the HTML part or the plain "
              "text part to be displayed.</p>"
              "<p>Displaying the HTML part makes the message look better, "
              "but at the same time increases the risk of security holes "
              "being exploited.</p>"
              "<p>Displaying the plain text part loses much of the message's "
              "formatting, but makes it almost <em>impossible</em> "
              "to exploit security holes in the HTML renderer (Konqueror).</p>"
              "<p>The option below guards against one common misuse of HTML "
              "messages, but it cannot guard against security issues that were "
              "not known at the time this version of KMail was written.</p>"
              "<p>It is therefore advisable to <em>not</em> prefer HTML to "
              "plain text.</p>"
              "<p><b>Note:</b> You can set this option on a per-folder basis "
              "from the <i>Folder</i> menu of KMail's main window.</p></qt>" );
  mExternalWhatsThis = i18n( "<qt><p>Some mail advertisements are in HTML "
              "and contain references to, for example, images that the advertisers"
              " employ to find out that you have read their message "
              "(&quot;web bugs&quot;).</p>"
              "<p>There is no valid reason to load images off the Internet like "
              "this, since the sender can always attach the required images "
              "directly to the message.</p>"
              "<p>To guard from such a misuse of the HTML displaying feature "
              "of KMail, this option is <em>disabled</em> by default.</p>"
              "<p>However, if you wish to, for example, view images in HTML "
              "messages that were not attached to it, you can enable this "
              "option, but you should be aware of the possible problem.</p></qt>" );
  mReceiptWhatsThis = i18n( "<qt><h3>Message Disposition Notification Policy</h3>"
              "<p>MDNs are a generalization of what is commonly called <b>read "
              "receipt</b>. The message author requests a disposition "
              "notification to be sent and the receiver's mail program "
              "generates a reply from which the author can learn what "
              "happened to his message. Common disposition types include "
              "<b>displayed</b> (i.e. read), <b>deleted</b> and <b>dispatched</b> "
              "(e.g. forwarded).</p>"
              "<p>The following options are available to control KMail's "
              "sending of MDNs:</p>"
              "<ul>"
              "<li><em>Ignore</em>: Ignores any request for disposition "
              "notifications. No MDN will ever be sent automatically "
              "(recommended).</li>"
              "<li><em>Ask</em>: Answers requests only after asking the user "
              "for permission. This way, you can send MDNs for selected "
              "messages while denying or ignoring them for others.</li>"
              "<li><em>Deny</em>: Always sends a <b>denied</b> notification. This "
              "is only <em>slightly</em> better than always sending MDNs. "
              "The author will still know that the messages has been acted "
              "upon, he just cannot tell whether it was deleted or read etc.</li>"
              "<li><em>Always send</em>: Always sends the requested "
              "disposition notification. That means that the author of the "
              "message gets to know when the message was acted upon and, "
              "in addition, what happened to it (displayed, deleted, "
              "etc.). This option is strongly discouraged, but since it "
              "makes much sense e.g. for customer relationship management, "
              "it has been made available.</li>"
              "</ul></qt>" );

  // "HTML Messages" group box:
  group = new QGroupBox( i18n("HTML Messages"), this );
  vboxlayout = new QVBoxLayout( group );

  mHtmlMailCheck = new QCheckBox( i18n("Prefer HTML to plain text"), group );
  mHtmlMailCheck->setWhatsThis( mHtmlWhatsThis );
  connect( mHtmlMailCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  vboxlayout->addWidget( mHtmlMailCheck );

  mExternalReferences = new QCheckBox( i18n("Allow messages to load external "
                                            "references from the Internet" ), group );
  mExternalReferences->setWhatsThis( mExternalWhatsThis );

  connect( mExternalReferences, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  vboxlayout->addWidget( mExternalReferences );

  label = new QLabel( i18n("<b>WARNING:</b> Allowing HTML in email may "
                           "increase the risk that your system will be "
                           "compromised by present and anticipated security "
                           "exploits. <a href=\"whatsthis1\">More about "
                           "HTML mails...</a> <a href=\"whatsthis2\">More "
                           "about external references...</a>"),
                           group );
  connect(label, SIGNAL(linkActivated ( const QString& )),
          SLOT(slotLinkClicked( const QString& )));

  label->setWordWrap( true );
  label->setTextInteractionFlags( Qt::LinksAccessibleByMouse |
          Qt::LinksAccessibleByKeyboard);
  vboxlayout->addWidget( label );
  vlay->addWidget( group );

  // encrypted messages group
  group = new QGroupBox( i18n("Encrypted Messages"), this );
  vboxlayout = new QVBoxLayout( group );
  mAlwaysDecrypt = new QCheckBox( i18n( "Attempt decryption of encrypted messages when viewing" ), group );
  connect( mAlwaysDecrypt, SIGNAL(stateChanged(int)), this, SLOT(slotEmitChanged()) );
  vboxlayout->addWidget( mAlwaysDecrypt );
  vlay->addWidget( group );

  // "Message Disposition Notification" groupbox:
  group = new QGroupBox( i18n("Message Disposition Notifications"), this );

  QGridLayout *grlayout = new QGridLayout( group );
  grlayout->setSpacing( KDialog::spacingHint() );

  // "ignore", "ask", "deny", "always send" radiobutton line:
  mMDNGroup = new QButtonGroup( group );
  connect( mMDNGroup, SIGNAL( buttonClicked( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  grlayout->addWidget( new QLabel( i18n("Send policy:"), group ), 0, 0 );

  radio = new QRadioButton( i18n("Ignore"), group );
  radio->setWhatsThis( mReceiptWhatsThis );
  grlayout->addWidget( radio, 0, 1, Qt::AlignLeft );
  mMDNGroup->addButton( radio, 0 );

  radio = new QRadioButton( i18n("Ask"), group );
  radio->setWhatsThis( mReceiptWhatsThis );
  grlayout->addWidget( radio, 0, 2, Qt::AlignLeft );
  mMDNGroup->addButton( radio, 1 );

  radio = new QRadioButton( i18n("Deny"), group );
  radio->setWhatsThis( mReceiptWhatsThis );
  grlayout->addWidget( radio, 0, 3, Qt::AlignLeft );
  mMDNGroup->addButton( radio, 2 );

  radio = new QRadioButton( i18n("Always send"), group );
  radio->setWhatsThis( mReceiptWhatsThis );
  grlayout->addWidget( radio, 0, 4, Qt::AlignLeft );
  mMDNGroup->addButton( radio, 3 );

  // "Original Message quote" radiobutton line:
  mOrigQuoteGroup = new QButtonGroup( group );
  connect( mOrigQuoteGroup, SIGNAL( buttonClicked( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  grlayout->addWidget( new QLabel( i18n("Quote original message:"), group ), 1, 0 );

  radio = new QRadioButton( i18n("Nothing"), group );
  radio->setWhatsThis( mReceiptWhatsThis );
  grlayout->addWidget( radio, 1, 1, Qt::AlignLeft );
  mOrigQuoteGroup->addButton( radio, 0 );

  radio = new QRadioButton( i18n("Full message"), group );
  radio->setWhatsThis( mReceiptWhatsThis );
  grlayout->addWidget( radio, 1, 2, Qt::AlignLeft );
  mOrigQuoteGroup->addButton( radio, 1 );

  radio = new QRadioButton( i18n("Only headers"), group );
  radio->setWhatsThis( mReceiptWhatsThis );
  grlayout->addWidget( radio, 1, 3, Qt::AlignLeft );
  mOrigQuoteGroup->addButton( radio, 2 );

  mNoMDNsWhenEncryptedCheck = new QCheckBox(
          i18n("Do not send MDNs in response to encrypted messages"), this );
  connect( mNoMDNsWhenEncryptedCheck, SIGNAL(toggled(bool)), SLOT(slotEmitChanged()) );
  grlayout->addWidget( mNoMDNsWhenEncryptedCheck, 2, 0, 1, -1 );

  // Warning label:
  label = new QLabel( i18n("<b>WARNING:</b> Unconditionally returning "
                           "confirmations undermines your privacy. "
                           "<a href=\"whatsthis3\">More about MDNs...</a>"),
                           this );
  connect(label, SIGNAL(linkActivated ( const QString& )),
          SLOT(slotLinkClicked( const QString& )));
  label->setWordWrap(true);
  label->setTextInteractionFlags( Qt::LinksAccessibleByMouse |
          Qt::LinksAccessibleByKeyboard);
  grlayout->addWidget( label, 3, 0, 1, -1 );

  vlay->addWidget( group );

  // "Attached keys" group box:
  group = new QGroupBox(i18n( "Certificate && Key Bundle Attachments" ), this );
  vboxlayout = new QVBoxLayout(group);
  mAutomaticallyImportAttachedKeysCheck = new QCheckBox(
          i18n("Automatically import keys and certificates"), group );
  connect( mAutomaticallyImportAttachedKeysCheck, SIGNAL(toggled(bool)),
           SLOT(slotEmitChanged()) );
  vboxlayout->addWidget( mAutomaticallyImportAttachedKeysCheck );

  vlay->addWidget( group );
  vlay->addStretch( 10 ); // spacer
}

void SecurityPageGeneralTab::slotLinkClicked( const QString & link ) {
    if ( link == "whatsthis1" )
        QWhatsThis::showText( QCursor::pos(), mHtmlWhatsThis );
    else if (link == "whatsthis2")
        QWhatsThis::showText( QCursor::pos(), mExternalWhatsThis );
    else if ( link == "whatsthis3" )
        QWhatsThis::showText( QCursor::pos(), mReceiptWhatsThis );
}

void SecurityPage::GeneralTab::doLoadOther() {
  const KConfigGroup reader( KMKernel::config(), "Reader" );

  mHtmlMailCheck->setChecked( reader.readEntry( "htmlMail", false ) );
  mExternalReferences->setChecked(
      reader.readEntry( "htmlLoadExternal", false ) );
  mAutomaticallyImportAttachedKeysCheck->setChecked(
      reader.readEntry( "AutoImportKeys", false ) );

  mAlwaysDecrypt->setChecked( GlobalSettings::self()->alwaysDecrypt() );

  const KConfigGroup mdn( KMKernel::config(), "MDN" );

  int num = mdn.readEntry( "default-policy", 0 );
  if ( num < 0 || num >= mMDNGroup->buttons().count() ) num = 0;
  mMDNGroup->button(num)->setChecked(true);
  num = mdn.readEntry( "quote-message", 0 );
  if ( num < 0 || num >= mOrigQuoteGroup->buttons().count() ) num = 0;
  mOrigQuoteGroup->button(num)->setChecked(true);
  mNoMDNsWhenEncryptedCheck->setChecked(
      mdn.readEntry( "not-send-when-encrypted", true ) );
}

void SecurityPage::GeneralTab::installProfile( KConfig * profile ) {
  const KConfigGroup reader( profile, "Reader" );
  const KConfigGroup mdn( profile, "MDN" );

  if ( reader.hasKey( "htmlMail" ) )
    mHtmlMailCheck->setChecked(
        reader.readEntry( "htmlMail", false ) );
  if ( reader.hasKey( "htmlLoadExternal" ) )
    mExternalReferences->setChecked(
        reader.readEntry( "htmlLoadExternal", false ) );
  if ( reader.hasKey( "AutoImportKeys" ) )
    mAutomaticallyImportAttachedKeysCheck->setChecked(
        reader.readEntry( "AutoImportKeys", false ) );

  if ( mdn.hasKey( "default-policy" ) ) {
      int num = mdn.readEntry( "default-policy", 0 );
      if ( num < 0 || num >= mMDNGroup->buttons().count() ) num = 0;
      mMDNGroup->button(num)->setChecked(true);
  }
  if ( mdn.hasKey( "quote-message" ) ) {
      int num = mdn.readEntry( "quote-message", 0 );
      if ( num < 0 || num >= mOrigQuoteGroup->buttons().count() ) num = 0;
      mOrigQuoteGroup->button(num)->setChecked(true);
  }
  if ( mdn.hasKey( "not-send-when-encrypted" ) )
      mNoMDNsWhenEncryptedCheck->setChecked(
          mdn.readEntry( "not-send-when-encrypted", false ) );
}

void SecurityPage::GeneralTab::save() {
  KConfigGroup reader( KMKernel::config(), "Reader" );
  KConfigGroup mdn( KMKernel::config(), "MDN" );

  if (reader.readEntry( "htmlMail", false ) != mHtmlMailCheck->isChecked())
  {
    if (KMessageBox::warningContinueCancel(this, i18n("Changing the global "
      "HTML setting will override all folder specific values."), QString(),
      KStandardGuiItem::cont(), KStandardGuiItem::cancel(), "htmlMailOverride") == KMessageBox::Continue)
    {
      reader.writeEntry( "htmlMail", mHtmlMailCheck->isChecked() );
      QStringList names;
      QList<QPointer<KMFolder> > folders;
      kmkernel->folderMgr()->createFolderList(&names, &folders);
      kmkernel->imapFolderMgr()->createFolderList(&names, &folders);
      kmkernel->dimapFolderMgr()->createFolderList(&names, &folders);
      kmkernel->searchFolderMgr()->createFolderList(&names, &folders);
      for (QList<QPointer<KMFolder> >::iterator it = folders.begin();
        it != folders.end(); ++it)
      {
        if (*it)
        {
          KConfigGroup config(KMKernel::config(),
            "Folder-" + (*it)->idString());
          config.writeEntry("htmlMailOverride", false);
        }
      }
    }
  }
  reader.writeEntry( "htmlLoadExternal", mExternalReferences->isChecked() );
  reader.writeEntry( "AutoImportKeys", mAutomaticallyImportAttachedKeysCheck->isChecked() );
  mdn.writeEntry( "default-policy", mMDNGroup->checkedId() );
  mdn.writeEntry( "quote-message", mOrigQuoteGroup->checkedId() );
  mdn.writeEntry( "not-send-when-encrypted", mNoMDNsWhenEncryptedCheck->isChecked() );
  GlobalSettings::self()->setAlwaysDecrypt( mAlwaysDecrypt->isChecked() );
}


QString SecurityPage::ComposerCryptoTab::helpAnchor() const {
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

void SecurityPage::ComposerCryptoTab::doLoadOther() {
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

void SecurityPage::ComposerCryptoTab::installProfile( KConfig * profile ) {
  const KConfigGroup composer( profile, "Composer" );

  if ( composer.hasKey( "pgp-auto-sign" ) )
    mWidget->mAutoSignature->setChecked(
        composer.readEntry( "pgp-auto-sign", false ) );

  if ( composer.hasKey( "crypto-encrypt-to-self" ) )
    mWidget->mEncToSelf->setChecked(
        composer.readEntry( "crypto-encrypt-to-self", false ) );
  if ( composer.hasKey( "crypto-show-encryption-result" ) )
    mWidget->mShowEncryptionResult->setChecked(
        composer.readEntry( "crypto-show-encryption-result", false ) );
  if ( composer.hasKey( "crypto-show-keys-for-approval" ) )
    mWidget->mShowKeyApprovalDlg->setChecked(
        composer.readEntry( "crypto-show-keys-for-approval", false ) );
  if ( composer.hasKey( "pgp-auto-encrypt" ) )
    mWidget->mAutoEncrypt->setChecked(
        composer.readEntry( "pgp-auto-encrypt", false ) );
  if ( composer.hasKey( "never-encrypt-drafts" ) )
    mWidget->mNeverEncryptWhenSavingInDrafts->setChecked(
        composer.readEntry( "never-encrypt-drafts", false ) );

  if ( composer.hasKey( "crypto-store-encrypted" ) )
    mWidget->mStoreEncrypted->setChecked(
        composer.readEntry( "crypto-store-encrypted", false ) );
}

void SecurityPage::ComposerCryptoTab::save() {
  KConfigGroup composer( KMKernel::config(), "Composer" );

  composer.writeEntry( "pgp-auto-sign", mWidget->mAutoSignature->isChecked() );

  composer.writeEntry( "crypto-encrypt-to-self", mWidget->mEncToSelf->isChecked() );
  composer.writeEntry( "crypto-show-encryption-result", mWidget->mShowEncryptionResult->isChecked() );
  composer.writeEntry( "crypto-show-keys-for-approval", mWidget->mShowKeyApprovalDlg->isChecked() );

  composer.writeEntry( "pgp-auto-encrypt", mWidget->mAutoEncrypt->isChecked() );
  composer.writeEntry( "never-encrypt-drafts", mWidget->mNeverEncryptWhenSavingInDrafts->isChecked() );

  composer.writeEntry( "crypto-store-encrypted", mWidget->mStoreEncrypted->isChecked() );
}

QString SecurityPage::WarningTab::helpAnchor() const {
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
  connect( mWidget->mWarnSignKeyExpiresSB, SIGNAL( valueChanged( int ) ), SLOT( slotEmitChanged() ) );
  connect( mWidget->mWarnSignChainCertExpiresSB, SIGNAL( valueChanged( int ) ), SLOT( slotEmitChanged() ) );
  connect( mWidget->mWarnSignRootCertExpiresSB, SIGNAL( valueChanged( int ) ), SLOT( slotEmitChanged() ) );

  connect( mWidget->mWarnEncrKeyExpiresSB, SIGNAL( valueChanged( int ) ), SLOT( slotEmitChanged() ) );
  connect( mWidget->mWarnEncrChainCertExpiresSB, SIGNAL( valueChanged( int ) ), SLOT( slotEmitChanged() ) );
  connect( mWidget->mWarnEncrRootCertExpiresSB, SIGNAL( valueChanged( int ) ), SLOT( slotEmitChanged() ) );

  connect( mWidget->enableAllWarningsPB, SIGNAL(clicked()),
           SLOT(slotReenableAllWarningsClicked()) );
}

void SecurityPage::WarningTab::doLoadOther() {
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
  mWidget->mWarnSignChainCertExpiresSB->setValue(
      composer.readEntry( "crypto-warn-sign-chaincert-near-expire-int", 14 ) );
  mWidget->mWarnSignRootCertExpiresSB->setValue(
      composer.readEntry( "crypto-warn-sign-root-near-expire-int", 14 ) );

  mWidget->mWarnEncrKeyExpiresSB->setValue(
      composer.readEntry( "crypto-warn-encr-key-near-expire-int", 14 ) );
  mWidget->mWarnEncrChainCertExpiresSB->setValue(
      composer.readEntry( "crypto-warn-encr-chaincert-near-expire-int", 14 ) );
  mWidget->mWarnEncrRootCertExpiresSB->setValue(
      composer.readEntry( "crypto-warn-encr-root-near-expire-int", 14 ) );

  mWidget->enableAllWarningsPB->setEnabled( true );
}

void SecurityPage::WarningTab::installProfile( KConfig * profile ) {
  const KConfigGroup composer( profile, "Composer" );

  if ( composer.hasKey( "crypto-warning-unencrypted" ) )
    mWidget->warnUnencryptedCB->setChecked(
        composer.readEntry( "crypto-warning-unencrypted", false ) );
  if ( composer.hasKey( "crypto-warning-unsigned" ) )
    mWidget->mWarnUnsigned->setChecked(
        composer.readEntry( "crypto-warning-unsigned", false ) );
  if ( composer.hasKey( "crypto-warn-recv-not-in-cert" ) )
    mWidget->warnReceiverNotInCertificateCB->setChecked(
        composer.readEntry( "crypto-warn-recv-not-in-cert", false ) );

  if ( composer.hasKey( "crypto-warn-when-near-expire" ) )
    mWidget->warnGroupBox->setChecked(
        composer.readEntry( "crypto-warn-when-near-expire", false ) );

  if ( composer.hasKey( "crypto-warn-sign-key-near-expire-int" ) )
    mWidget->mWarnSignKeyExpiresSB->setValue(
        composer.readEntry( "crypto-warn-sign-key-near-expire-int", 0 ) );
  if ( composer.hasKey( "crypto-warn-sign-chaincert-near-expire-int" ) )
    mWidget->mWarnSignChainCertExpiresSB->setValue(
        composer.readEntry( "crypto-warn-sign-chaincert-near-expire-int", 0 ) );
  if ( composer.hasKey( "crypto-warn-sign-root-near-expire-int" ) )
    mWidget->mWarnSignRootCertExpiresSB->setValue(
        composer.readEntry( "crypto-warn-sign-root-near-expire-int", 0 ) );

  if ( composer.hasKey( "crypto-warn-encr-key-near-expire-int" ) )
    mWidget->mWarnEncrKeyExpiresSB->setValue(
        composer.readEntry( "crypto-warn-encr-key-near-expire-int", 0 ) );
  if ( composer.hasKey( "crypto-warn-encr-chaincert-near-expire-int" ) )
    mWidget->mWarnEncrChainCertExpiresSB->setValue(
        composer.readEntry( "crypto-warn-encr-chaincert-near-expire-int", 0 ) );
  if ( composer.hasKey( "crypto-warn-encr-root-near-expire-int" ) )
    mWidget->mWarnEncrRootCertExpiresSB->setValue(
        composer.readEntry( "crypto-warn-encr-root-near-expire-int", 0 ) );
}

void SecurityPage::WarningTab::save() {
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

void SecurityPage::WarningTab::slotReenableAllWarningsClicked() {
  KMessageBox::enableAllMessages();
  mWidget->enableAllWarningsPB->setEnabled( false );
}

////

QString SecurityPage::SMimeTab::helpAnchor() const {
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

  mWidget->OCSPGroupBox->layout()->setMargin( KDialog::marginHint() );
  mWidget->HTTPGroupBox->layout()->setMargin( KDialog::marginHint() );
  mWidget->LDAPGroupBox->layout()->setMargin( KDialog::marginHint() );

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

static void disableDirmngrWidget( QWidget* w ) {
  w->setEnabled( false );
  w->setWhatsThis( i18n( "This option requires dirmngr >= 0.9.0" ) );
}

static void initializeDirmngrCheckbox( QCheckBox* cb, Kleo::CryptoConfigEntry* entry ) {
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

void SecurityPage::SMimeTab::doLoadOther() {
  if ( !mConfig ) {
    setEnabled( false );
    return;
  }

  // Force re-parsing gpgconf data, in case e.g. kleopatra or "configure backend" was used
  // (which ends up calling us via dcop)
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
    QString systemProxy = QString::fromLocal8Bit( getenv( "http_proxy" ) );
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

void SecurityPage::SMimeTab::slotUpdateHTTPActions() {
  mWidget->ignoreHTTPDPCB->setEnabled( !mWidget->disableHTTPCB->isChecked() );

  // The proxy settings only make sense when "Ignore HTTP CRL DPs of certificate" is checked.
  bool enableProxySettings = !mWidget->disableHTTPCB->isChecked()
                          && mWidget->ignoreHTTPDPCB->isChecked();
  mWidget->systemHTTPProxy->setEnabled( enableProxySettings );
  mWidget->useCustomHTTPProxyRB->setEnabled( enableProxySettings );
  mWidget->honorHTTPProxyRB->setEnabled( enableProxySettings );
  mWidget->customHTTPProxy->setEnabled( enableProxySettings );

  if ( !mWidget->useCustomHTTPProxyRB->isChecked() &&
       !mWidget->honorHTTPProxyRB->isChecked() )
    mWidget->honorHTTPProxyRB->setChecked( true );
}

void SecurityPage::SMimeTab::installProfile( KConfig * ) {
}

static void saveCheckBoxToKleoEntry( QCheckBox* cb, Kleo::CryptoConfigEntry* entry ) {
  const bool b = cb->isChecked();
  if ( entry && entry->boolValue() != b )
    entry->setBoolValue( b );
}

void SecurityPage::SMimeTab::save() {
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

#ifdef __GNUC__
#warning WTF?!?! Hand-made DCOP stuff?!?!
#endif
/*bool SecurityPageSMimeTab::process(const DCOPCString &fun, const QByteArray &data, DCOPCString& replyType, QByteArray &replyData)
{
    if ( fun == "load()" ) {
        replyType = "void";
        load();
    } else {
        return DCOPObject::process( fun, data, replyType, replyData );
    }
    return true;
}

DCOPCStringList SecurityPageSMimeTab::interfaces()
{
  DCOPCStringList ifaces = DCOPObject::interfaces();
  ifaces += "SecurityPageSMimeTab";
  return ifaces;
}

DCOPCStringList SecurityPageSMimeTab::functions()
{
  // Hide our slot, just because it's simpler to do so.
  return DCOPObject::functions();
}*/

Kleo::CryptoConfigEntry* SMIMECryptoConfigEntries::configEntry( const char* componentName,
                                                                const char* groupName,
                                                                const char* entryName,
                                                                int /*Kleo::CryptoConfigEntry::ArgType*/ argType,
                                                                bool isList )
{
    Kleo::CryptoConfigEntry* entry = mConfig->entry( componentName, groupName, entryName );
    if ( !entry ) {
        kWarning(5006) << QString("Backend error: gpgconf doesn't seem to know the entry for %1/%2/%3" ).arg( componentName, groupName, entryName );
        return 0;
    }
    if( entry->argType() != argType || entry->isList() != isList ) {
        kWarning(5006) << QString("Backend error: gpgconf has wrong type for %1/%2/%3: %4 %5" ).arg( componentName, groupName, entryName ).arg( entry->argType() ).arg( entry->isList() );
        return 0;
    }
    return entry;
}

////

QString SecurityPage::CryptPlugTab::helpAnchor() const {
  return QString::fromLatin1("configure-security-crypto-backends");
}

SecurityPageCryptPlugTab::SecurityPageCryptPlugTab( QWidget * parent )
  : ConfigModuleTab( parent )
{
  QVBoxLayout * vlay = new QVBoxLayout( this );
  vlay->setSpacing( KDialog::spacingHint() );
  vlay->setMargin( KDialog::marginHint() );

  mBackendConfig = Kleo::CryptoBackendFactory::instance()->configWidget( this, "mBackendConfig" );
  connect( mBackendConfig, SIGNAL( changed( bool ) ), this, SIGNAL( changed( bool ) ) );

  vlay->addWidget( mBackendConfig );
}

SecurityPageCryptPlugTab::~SecurityPageCryptPlugTab()
{

}

void SecurityPage::CryptPlugTab::doLoadOther() {
  mBackendConfig->load();
}

void SecurityPage::CryptPlugTab::save() {
  mBackendConfig->save();
}

// *************************************************************
// *                                                           *
// *                        MiscPage                           *
// *                                                           *
// *************************************************************
QString MiscPage::helpAnchor() const {
  return QString::fromLatin1("configure-misc");
}

MiscPage::MiscPage( const KComponentData &instance, QWidget *parent )
  : ConfigModuleWithTabs( instance, parent )
{
  mFolderTab = new FolderTab();
  addTab( mFolderTab, i18n("&Folders") );

  mGroupwareTab = new GroupwareTab();
  addTab( mGroupwareTab, i18n("&Groupware") );
  load();
}

QString MiscPage::FolderTab::helpAnchor() const {
  return QString::fromLatin1("configure-misc-folders");
}

MiscPageFolderTab::MiscPageFolderTab( QWidget * parent )
  : ConfigModuleTab( parent )
{
  // temp. vars:
  QVBoxLayout *vlay;
  QHBoxLayout *hlay;
  QLabel      *label;

  vlay = new QVBoxLayout( this );
  vlay->setSpacing( KDialog::spacingHint() );
  vlay->setMargin( KDialog::marginHint() );

  // "confirm before emptying folder" check box: stretch 0
  mEmptyFolderConfirmCheck =
    new QCheckBox( i18nc("Corresponds to Folder->Move All Messages to Trash",
                        "Ask for co&nfirmation before moving all messages to "
                        "trash"),
                   this );
  vlay->addWidget( mEmptyFolderConfirmCheck );
  connect( mEmptyFolderConfirmCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  mExcludeImportantFromExpiry =
    new QCheckBox( i18n("E&xclude important messages from expiry"), this );
  mExcludeImportantFromExpiry->setWhatsThis(
    i18n( GlobalSettings::self()->excludeImportantMailFromExpiryItem()->whatsThis().toUtf8() ) );
  vlay->addWidget( mExcludeImportantFromExpiry );
  connect( mExcludeImportantFromExpiry, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // "when trying to find unread messages" combo + label: stretch 0
  hlay = new QHBoxLayout(); // inherits spacing
  vlay->addLayout( hlay );
  mLoopOnGotoUnread = new QComboBox( this );
  mLoopOnGotoUnread->setEditable( false );
  label = new QLabel(
           i18nc("to be continued with \"do not loop\", \"loop in current folder\", "
                "and \"loop in all folders\".",
                "When trying to find unread messages:"), this );
  label->setBuddy( mLoopOnGotoUnread );
  mLoopOnGotoUnread->addItems( QStringList()
      << i18nc("continuation of \"When trying to find unread messages:\"",
              "Do not Loop")
      << i18nc("continuation of \"When trying to find unread messages:\"",
              "Loop in Current Folder")
      << i18nc("continuation of \"When trying to find unread messages:\"",
              "Loop in All Folders"));
  hlay->addWidget( label );
  hlay->addWidget( mLoopOnGotoUnread, 1 );
  connect( mLoopOnGotoUnread, SIGNAL( activated( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // when entering a folder
  hlay = new QHBoxLayout(); // inherits spacing
  vlay->addLayout( hlay );
  mActionEnterFolder = new QComboBox( this );
  mActionEnterFolder->setEditable( false );
  label = new QLabel(
           i18nc("to be continued with \"jump to first new message\", "
                "\"jump to first unread or new message\","
                "and \"jump to last selected message\".",
                "When entering a folder:"), this );
  label->setBuddy( mActionEnterFolder );
  mActionEnterFolder->addItems( QStringList()
      << i18nc("continuation of \"When entering a folder:\"",
              "Jump to First New Message")
      << i18nc("continuation of \"When entering a folder:\"",
              "Jump to First Unread or New Message")
      << i18nc("continuation of \"When entering a folder:\"",
              "Jump to Last Selected Message"));
  hlay->addWidget( label );
  hlay->addWidget( mActionEnterFolder, 1 );
  connect( mActionEnterFolder, SIGNAL( activated( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  hlay = new QHBoxLayout(); // inherits spacing
  vlay->addLayout( hlay );
  mDelayedMarkAsRead = new QCheckBox( i18n("Mar&k selected message as read after"), this );
  hlay->addWidget( mDelayedMarkAsRead );
  mDelayedMarkTime = new KIntSpinBox( 0 /*min*/, 60 /*max*/, 1/*step*/,
                                      0 /*init*/, this);
  mDelayedMarkTime->setSuffix( i18n(" sec") );
  mDelayedMarkTime->setEnabled( false ); // since mDelayedMarkAsREad is off
  hlay->addWidget( mDelayedMarkTime );
  hlay->addStretch( 1 );
  connect( mDelayedMarkTime, SIGNAL( valueChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  connect( mDelayedMarkAsRead, SIGNAL(toggled(bool)),
           mDelayedMarkTime, SLOT(setEnabled(bool)));
  connect( mDelayedMarkAsRead, SIGNAL(toggled(bool)),
           this , SLOT(slotEmitChanged( void )));

  // "show popup after Drag'n'Drop" checkbox: stretch 0
  mShowPopupAfterDnD =
    new QCheckBox( i18n("Ask for action after &dragging messages to another folder"), this );
  vlay->addWidget( mShowPopupAfterDnD );
  connect( mShowPopupAfterDnD, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // "default mailbox format" combo + label: stretch 0
  hlay = new QHBoxLayout(); // inherits spacing
  vlay->addLayout( hlay );
  mMailboxPrefCombo = new QComboBox( this );
  mMailboxPrefCombo->setEditable( false );
  label = new QLabel(i18nc("to be continued with \"flat files\" and "
                           "\"directories\", resp.",
                           "By default, &message folders on disk are:"), this );
  label->setBuddy( mMailboxPrefCombo );
  mMailboxPrefCombo->addItems( QStringList()
          << i18nc("continuation of \"By default, &message folders on disk are\"",
                  "Flat Files (\"mbox\" format)")
          << i18nc("continuation of \"By default, &message folders on disk are\"",
                  "Directories (\"maildir\" format)") );
  // and now: add QWhatsThis:
  QString msg = i18nc( "what's this help",
                      "<qt><p>This selects which mailbox format will be "
                      "the default for local folders:</p>"
                      "<p><b>mbox:</b> KMail's mail "
                      "folders are represented by a single file each. "
                      "Individual messages are separated from each other by a "
                      "line starting with \"From \". This saves space on "
                      "disk, but may be less robust, e.g. when moving messages "
                      "between folders.</p>"
                      "<p><b>maildir:</b> KMail's mail folders are "
                      "represented by real folders on disk. Individual messages "
                      "are separate files. This may waste a bit of space on "
                      "disk, but should be more robust, e.g. when moving "
                      "messages between folders.</p></qt>");
  mMailboxPrefCombo->setWhatsThis( msg );
  label->setWhatsThis( msg );
  hlay->addWidget( label );
  hlay->addWidget( mMailboxPrefCombo, 1 );
  connect( mMailboxPrefCombo, SIGNAL( activated( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // "On startup..." option:
  hlay = new QHBoxLayout(); // inherits spacing
  vlay->addLayout( hlay );
  mOnStartupOpenFolder = new FolderRequester( this,
      kmkernel->getKMMainWidget()->folderTree() );
  label = new QLabel( i18n("Open this folder on startup:"), this );
  label->setBuddy( mOnStartupOpenFolder );
  hlay->addWidget( label );
  hlay->addWidget( mOnStartupOpenFolder, 1 );
  connect( mOnStartupOpenFolder, SIGNAL( folderChanged( KMFolder* ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // "Empty &trash on program exit" option:
  hlay = new QHBoxLayout();
  vlay->addLayout( hlay );
  mEmptyTrashCheck = new QCheckBox( i18n("Empty local &trash folder on program exit"),
                                    this );
  hlay->addWidget( mEmptyTrashCheck );
  connect( mEmptyTrashCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // "Quota Units"
  hlay = new QHBoxLayout();
  vlay->addLayout( hlay );
  mQuotaCmbBox = new QComboBox( this );
  mQuotaCmbBox->setEditable( false );
  label = new QLabel( i18n("Quota units: "), this );
  label->setBuddy( mQuotaCmbBox );
  mQuotaCmbBox->insertItems( 0, QStringList()
                   << i18n("KB")
                   << i18n("MB")
                   << i18n("GB") );
  hlay->addWidget( label );
  hlay->addWidget( mQuotaCmbBox, 1 );
  connect( mQuotaCmbBox, SIGNAL( activated( int )  ), this, SLOT( slotEmitChanged( void ) ) );

  vlay->addStretch( 1 );

  // @TODO: Till, move into .kcgc file
  msg = i18nc( "what's this help",
            "<qt><p>When jumping to the next unread message, it may occur "
            "that no more unread messages are below the current message.</p>"
            "<p><b>Do not loop:</b> The search will stop at the last message in "
            "the current folder.</p>"
            "<p><b>Loop in current folder:</b> The search will continue at the "
            "top of the message list, but not go to another folder.</p>"
            "<p><b>Loop in all folders:</b> The search will continue at the top of "
            "the message list. If no unread messages are found it will then continue "
            "to the next folder.</p>"
            "<p>Similarly, when searching for the previous unread message, "
            "the search will start from the bottom of the message list and continue to "
            "the previous folder depending on which option is selected.</p></qt>" );
  mLoopOnGotoUnread->setWhatsThis( msg );
}

void MiscPage::FolderTab::doLoadFromGlobalSettings() {
  mExcludeImportantFromExpiry->setChecked( GlobalSettings::self()->excludeImportantMailFromExpiry() );
  // default = "Loop in current folder"
  mLoopOnGotoUnread->setCurrentIndex( GlobalSettings::self()->loopOnGotoUnread() );
  mActionEnterFolder->setCurrentIndex( GlobalSettings::self()->actionEnterFolder() );
  mDelayedMarkAsRead->setChecked( GlobalSettings::self()->delayedMarkAsRead() );
  mDelayedMarkTime->setValue( GlobalSettings::self()->delayedMarkTime() );
  mShowPopupAfterDnD->setChecked( GlobalSettings::self()->showPopupAfterDnD() );
  mQuotaCmbBox->setCurrentIndex( GlobalSettings::self()->quotaUnit() );
}

void MiscPage::FolderTab::doLoadOther() {
  KConfigGroup general( KMKernel::config(), "General" );

  mEmptyTrashCheck->setChecked(
      general.readEntry( "empty-trash-on-exit", true ) );
  mOnStartupOpenFolder->setFolder( general.readEntry( "startupFolder",
      kmkernel->inboxFolder()->idString() ) );
  mEmptyFolderConfirmCheck->setChecked(
      general.readEntry( "confirm-before-empty", true ) );

  int num = general.readEntry("default-mailbox-format", 1 );
  if ( num < 0 || num > 1 ) num = 1;
  mMailboxPrefCombo->setCurrentIndex( num );
}

void MiscPage::FolderTab::save() {
  KConfigGroup general( KMKernel::config(), "General" );

  general.writeEntry( "empty-trash-on-exit", mEmptyTrashCheck->isChecked() );
  general.writeEntry( "confirm-before-empty", mEmptyFolderConfirmCheck->isChecked() );
  general.writeEntry( "default-mailbox-format", mMailboxPrefCombo->currentIndex() );
  general.writeEntry( "startupFolder", mOnStartupOpenFolder->folder() ?
                                  mOnStartupOpenFolder->folder()->idString() : QString() );

  GlobalSettings::self()->setDelayedMarkAsRead( mDelayedMarkAsRead->isChecked() );
  GlobalSettings::self()->setDelayedMarkTime( mDelayedMarkTime->value() );
  GlobalSettings::self()->setActionEnterFolder( mActionEnterFolder->currentIndex() );
  GlobalSettings::self()->setLoopOnGotoUnread( mLoopOnGotoUnread->currentIndex() );
  GlobalSettings::self()->setShowPopupAfterDnD( mShowPopupAfterDnD->isChecked() );
  GlobalSettings::self()->setExcludeImportantMailFromExpiry(
        mExcludeImportantFromExpiry->isChecked() );
  GlobalSettings::self()->setQuotaUnit( mQuotaCmbBox->currentIndex() );
}

QString MiscPage::GroupwareTab::helpAnchor() const {
  return QString::fromLatin1("configure-misc-groupware");
}

MiscPageGroupwareTab::MiscPageGroupwareTab( QWidget* parent )
  : ConfigModuleTab( parent )
{
  QBoxLayout* vlay = new QVBoxLayout( this );
  vlay->setSpacing( KDialog::spacingHint() );
  vlay->setMargin( KDialog::marginHint() );

  // IMAP resource setup
  QGroupBox * b1 = new QGroupBox( i18n("&IMAP Resource Folder Options"), this );
  QLayout *layout = new QVBoxLayout( b1 );

  mEnableImapResCB =
    new QCheckBox( i18n("&Enable IMAP resource functionality"), b1 );
  mEnableImapResCB->setToolTip(  i18n( "This enables the IMAP storage for "
                                          "the Kontact applications" ) );
  mEnableImapResCB->setWhatsThis(
        i18n( GlobalSettings::self()->theIMAPResourceEnabledItem()->whatsThis().toUtf8() ) );
  connect( mEnableImapResCB, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  mBox = new QWidget( b1 );
  QGridLayout* grid = new QGridLayout( mBox );
  grid->setSpacing( KDialog::spacingHint() );
  grid->setMargin( 0 );
  grid->setColumnStretch( 1, 1 );
  connect( mEnableImapResCB, SIGNAL( toggled(bool) ),
           mBox, SLOT( setEnabled(bool) ) );

  QLabel* storageFormatLA = new QLabel( i18n("&Format used for the groupware folders:"),
                                        mBox );
  QString toolTip = i18n( "Choose the format to use to store the contents of the groupware folders." );
  QString whatsThis = i18n( GlobalSettings::self()
        ->theIMAPResourceStorageFormatItem()->whatsThis().toUtf8() );
  grid->addWidget( storageFormatLA, 0, 0 );
  storageFormatLA->setToolTip( toolTip );
  storageFormatLA->setWhatsThis( whatsThis );
  mStorageFormatCombo = new QComboBox( mBox );
  mStorageFormatCombo->setEditable( false );
  storageFormatLA->setBuddy( mStorageFormatCombo );
  QStringList formatLst;
  formatLst << i18n("Standard (Ical / Vcard)") << i18n("Kolab (XML)");
  mStorageFormatCombo->addItems( formatLst );
  grid->addWidget( mStorageFormatCombo, 0, 1 );
  mStorageFormatCombo->setToolTip( toolTip );
  mStorageFormatCombo->setWhatsThis( whatsThis );
  connect( mStorageFormatCombo, SIGNAL( activated( int ) ),
           this, SLOT( slotStorageFormatChanged( int ) ) );

  QLabel* languageLA = new QLabel( i18n("&Language of the groupware folders:"),
                                   mBox );

  toolTip = i18n( "Set the language of the folder names" );
  whatsThis = i18n( GlobalSettings::self()
        ->theIMAPResourceFolderLanguageItem()->whatsThis().toUtf8() );
  grid->addWidget( languageLA, 1, 0 );
  languageLA->setToolTip( toolTip );
  languageLA->setWhatsThis( whatsThis );
  mLanguageCombo = new QComboBox( mBox );
  mLanguageCombo->setEditable( false );
  languageLA->setBuddy( mLanguageCombo );
  QStringList lst;
  lst << i18n("English") << i18n("German") << i18n("French") << i18n("Dutch");
  mLanguageCombo->addItems( lst );
  grid->addWidget( mLanguageCombo, 1, 1 );
  mLanguageCombo->setToolTip( toolTip );
  mLanguageCombo->setWhatsThis( whatsThis );
  connect( mLanguageCombo, SIGNAL( activated( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  mFolderComboLabel = new QLabel( mBox ); // text depends on storage format
  toolTip = i18n( "Set the parent of the resource folders" );
  whatsThis = i18n( GlobalSettings::self()->theIMAPResourceFolderParentItem()->whatsThis().toUtf8() );
  mFolderComboLabel->setToolTip( toolTip );
  mFolderComboLabel->setWhatsThis( whatsThis );
  grid->addWidget( mFolderComboLabel, 2, 0 );

  mFolderComboStack = new QStackedWidget( mBox );
  grid->addWidget( mFolderComboStack, 2, 1 );

  // First possibility in the widgetstack: a combo showing the list of all folders
  // This is used with the ical/vcard storage
  mFolderCombo = new FolderRequester( mBox,
      kmkernel->getKMMainWidget()->folderTree() );
  mFolderComboStack->insertWidget( 0,mFolderCombo );
  mFolderCombo->setToolTip( toolTip );
  mFolderCombo->setWhatsThis( whatsThis );
  connect( mFolderCombo, SIGNAL( folderChanged( KMFolder* ) ),
           this, SLOT( slotEmitChanged() ) );

  // Second possibility in the widgetstack: a combo showing the list of accounts
  // This is used with the kolab xml storage since the groupware folders
  // are always under the inbox.
  mAccountCombo = new KMail::AccountComboBox( mBox );
  mFolderComboStack->insertWidget( 1, mAccountCombo );
  mAccountCombo->setToolTip( toolTip );
  mAccountCombo->setWhatsThis( whatsThis );
  connect( mAccountCombo, SIGNAL( activated( int ) ),
           this, SLOT( slotEmitChanged() ) );

  mHideGroupwareFolders = new QCheckBox( i18n( "&Hide groupware folders" ), mBox );
  mHideGroupwareFolders->setObjectName( "HideGroupwareFoldersBox" );
  grid->addWidget( mHideGroupwareFolders, 3, 0, 1, 1 );
  mHideGroupwareFolders->setToolTip(
                 i18n( "When this is checked, you will not see the IMAP "
                       "resource folders in the folder tree." ) );
  mHideGroupwareFolders->setWhatsThis( i18n( GlobalSettings::self()
           ->hideGroupwareFoldersItem()->whatsThis().toUtf8() ) );
  connect( mHideGroupwareFolders, SIGNAL( toggled( bool ) ),
           this, SLOT( slotEmitChanged() ) );
  layout->addWidget( mEnableImapResCB );
  layout->addWidget( mBox );
  vlay->addWidget( b1 );

  mOnlyShowGroupwareFolders = new QCheckBox( i18n( "&Only show groupware folders for this account" ), mBox );
  mOnlyShowGroupwareFolders->setObjectName( "OnlyGroupwareFoldersBox" );
  grid->addWidget( mOnlyShowGroupwareFolders, 3, 1, 1, 1 );
  mOnlyShowGroupwareFolders->setToolTip(
                 i18n( "When this is checked, you will not see normal  "
                       "mail folders in the folder tree for the account "
                       "configured for groupware." ) );
  mOnlyShowGroupwareFolders->setWhatsThis( i18n( GlobalSettings::self()
           ->showOnlyGroupwareFoldersForGroupwareAccountItem()->whatsThis().toUtf8() ) );
  connect( mOnlyShowGroupwareFolders, SIGNAL( toggled( bool ) ),
           this, SLOT( slotEmitChanged() ) );

  // Groupware functionality compatibility setup
  b1 = new QGroupBox( i18n("Groupware Compatibility && Legacy Options"), this );
  layout = new QVBoxLayout( b1 );

  gBox = new KVBox( b1 );
#if 0
  // Currently believed to be disused.
  mEnableGwCB = new QCheckBox( i18n("&Enable groupware functionality"), b1 );
  gBox->setSpacing( KDialog::spacingHint() );
  connect( mEnableGwCB, SIGNAL( toggled(bool) ),
           gBox, SLOT( setEnabled(bool) ) );
  connect( mEnableGwCB, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
#endif
  mEnableGwCB = 0;
  mLegacyMangleFromTo = new QCheckBox( i18n( "Mangle From:/To: headers in replies to invitations" ), gBox );
  mLegacyMangleFromTo->setToolTip( i18n( "Turn this option on in order to make Outlook(tm) understand your answers to invitation replies" ) );
  mLegacyMangleFromTo->setWhatsThis( i18n( GlobalSettings::self()->
           legacyMangleFromToHeadersItem()->whatsThis().toUtf8() ) );
  connect( mLegacyMangleFromTo, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  mLegacyBodyInvites = new QCheckBox( i18n( "Send invitations in the mail body" ), gBox );
  mLegacyBodyInvites->setToolTip( i18n( "Turn this option on in order to make Outlook(tm) understand your answers to invitations" ) );
  mLegacyMangleFromTo->setWhatsThis( i18n( GlobalSettings::self()->
           legacyBodyInvitesItem()->whatsThis().toUtf8() ) );
  connect( mLegacyBodyInvites, SIGNAL( toggled( bool ) ),
           this, SLOT( slotLegacyBodyInvitesToggled( bool ) ) );
  connect( mLegacyBodyInvites, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  mExchangeCompatibleInvitations = new QCheckBox( i18n( "Exchange compatible invitation naming" ), gBox );
  mExchangeCompatibleInvitations->setToolTip( i18n( "Microsoft Outlook, when used in combination with a Microsoft Exchange server, has a problem understanding standards-compliant groupware e-mail. Turn this option on to send groupware invitations in a way that Microsoft Exchange understands." ) );
  mExchangeCompatibleInvitations->setWhatsThis( i18n( GlobalSettings::self()->
           exchangeCompatibleInvitationsItem()->whatsThis().toUtf8() ) );
  connect( mExchangeCompatibleInvitations, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  mAutomaticSending = new QCheckBox( i18n( "Automatic invitation sending" ), gBox );
  mAutomaticSending->setToolTip( i18n( "When this is on, the user will not see the mail composer window. Invitation mails are sent automatically" ) );
  mAutomaticSending->setWhatsThis( i18n( GlobalSettings::self()->
           automaticSendingItem()->whatsThis().toUtf8() ) );
  connect( mAutomaticSending, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  layout->addWidget( gBox );
  vlay->addWidget( b1 );
  vlay->addStretch( 10 ); // spacer
}

void MiscPageGroupwareTab::slotLegacyBodyInvitesToggled( bool on )
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
    KMessageBox::information( this, txt, QString(),
                              "LegacyBodyInvitesWarning" );
  }
  // Invitations in the body are autosent in any case (no point in editing raw ICAL)
  // So the autosend option is only available if invitations are sent as attachment.
  mAutomaticSending->setEnabled( !mLegacyBodyInvites->isChecked() );
}

void MiscPage::GroupwareTab::doLoadFromGlobalSettings() {
  if ( mEnableGwCB ) {
    mEnableGwCB->setChecked( GlobalSettings::self()->groupwareEnabled() );
    gBox->setEnabled( mEnableGwCB->isChecked() );
  }

  mLegacyMangleFromTo->setChecked( GlobalSettings::self()->legacyMangleFromToHeaders() );

  mLegacyBodyInvites->blockSignals( true );
  mLegacyBodyInvites->setChecked( GlobalSettings::self()->legacyBodyInvites() );
  mLegacyBodyInvites->blockSignals( false );

  mAutomaticSending->setChecked( GlobalSettings::self()->automaticSending() );
  mAutomaticSending->setEnabled( !mLegacyBodyInvites->isChecked() );

  // Read the IMAP resource config
  mEnableImapResCB->setChecked( GlobalSettings::self()->theIMAPResourceEnabled() );
  mBox->setEnabled( mEnableImapResCB->isChecked() );

  mHideGroupwareFolders->setChecked( GlobalSettings::self()->hideGroupwareFolders() );
  int i = GlobalSettings::self()->theIMAPResourceFolderLanguage();
  mLanguageCombo->setCurrentIndex(i);
  i = GlobalSettings::self()->theIMAPResourceStorageFormat();
  mStorageFormatCombo->setCurrentIndex(i);
  slotStorageFormatChanged( i );
  mOnlyShowGroupwareFolders->setChecked( GlobalSettings::self()->showOnlyGroupwareFoldersForGroupwareAccount() );

  QString folderId( GlobalSettings::self()->theIMAPResourceFolderParent() );
  if( !folderId.isNull() && kmkernel->findFolderById( folderId ) ) {
    mFolderCombo->setFolder( folderId );
  } else {
    // Folder was deleted, we have to choose a new one
    mFolderCombo->setFolder( i18n( "<Choose a Folder>" ) );
  }

  KMAccount* selectedAccount = 0;
  int accountId = GlobalSettings::self()->theIMAPResourceAccount();
  if ( accountId )
    selectedAccount = kmkernel->acctMgr()->find( accountId );
  else {
    // Fallback: iterate over accounts to select folderId if found (as an inbox folder)
    for( KMAccount *a = kmkernel->acctMgr()->first(); a!=0;
         a = kmkernel->acctMgr()->next() ) {
      if( a->folder() && a->folder()->child() ) {
        // Look inside that folder for an INBOX
        KMFolderNode *node = 0;
        QList<KMFolderNode*>::const_iterator it;
        for ( it = a->folder()->child()->constBegin();
              it != a->folder()->child()->constEnd();
              ++it ) {
          node = *it;
          if (!node->isDir() && node->name() == "INBOX")
            break;
        }

        if ( node && static_cast<KMFolder*>(node)->idString() == folderId ) {
          selectedAccount = a;
          break;
        }
      }
    }
  }
  if ( selectedAccount )
    mAccountCombo->setCurrentAccount( selectedAccount );
  else if ( GlobalSettings::self()->theIMAPResourceStorageFormat() == 1 )
    kDebug(5006) <<"Folder" << folderId <<" not found as an account's inbox";
}

void MiscPage::GroupwareTab::save()
{
  KConfigGroup groupware( KMKernel::config(), "Groupware" );

  // Write the groupware config
  if ( mEnableGwCB ) {
    groupware.writeEntry( "GroupwareEnabled", mEnableGwCB->isChecked() );
  }
  groupware.writeEntry( "LegacyMangleFromToHeaders", mLegacyMangleFromTo->isChecked() );
  groupware.writeEntry( "LegacyBodyInvites", mLegacyBodyInvites->isChecked() );
  groupware.writeEntry( "ExchangeCompatibleInvitations", mExchangeCompatibleInvitations->isChecked() );
  groupware.writeEntry( "AutomaticSending", mAutomaticSending->isChecked() );

  if ( mEnableGwCB ) {
    GlobalSettings::self()->setGroupwareEnabled( mEnableGwCB->isChecked() );
  }
  GlobalSettings::self()->setLegacyMangleFromToHeaders( mLegacyMangleFromTo->isChecked() );
  GlobalSettings::self()->setLegacyBodyInvites( mLegacyBodyInvites->isChecked() );
  GlobalSettings::self()->setExchangeCompatibleInvitations( mExchangeCompatibleInvitations->isChecked() );
  GlobalSettings::self()->setAutomaticSending( mAutomaticSending->isChecked() );

  int format = mStorageFormatCombo->currentIndex();
  GlobalSettings::self()->setTheIMAPResourceStorageFormat( format );

  // Write the IMAP resource config
  GlobalSettings::self()->setHideGroupwareFolders( mHideGroupwareFolders->isChecked() );
  GlobalSettings::self()->setShowOnlyGroupwareFoldersForGroupwareAccount( mOnlyShowGroupwareFolders->isChecked() );

  // If there is a leftover folder in the foldercombo, getFolder can
  // return 0. In that case we really don't have it enabled
  QString folderId;
  if (  format == 0 ) {
    KMFolder* folder = mFolderCombo->folder();
    if (  folder )
      folderId = folder->idString();
  } else {
    // Inbox folder of the selected account
    KMAccount* acct = mAccountCombo->currentAccount();
    if (  acct ) {
      folderId = QString( ".%1.directory/INBOX" ).arg( acct->id() );
      GlobalSettings::self()->setTheIMAPResourceAccount( acct->id() );
    }
  }

  bool enabled = mEnableImapResCB->isChecked() && !folderId.isEmpty();
  GlobalSettings::self()->setTheIMAPResourceEnabled( enabled );
  GlobalSettings::self()->setTheIMAPResourceFolderLanguage( mLanguageCombo->currentIndex() );
  GlobalSettings::self()->setTheIMAPResourceFolderParent( folderId );
}

void MiscPage::GroupwareTab::slotStorageFormatChanged( int format )
{
  mLanguageCombo->setEnabled( format == 0 ); // only ical/vcard needs the language hack
  mFolderComboStack->setCurrentIndex( format );
  if ( format == 0 ) {
    mFolderComboLabel->setText( i18n("&Resource folders are subfolders of:") );
    mFolderComboLabel->setBuddy( mFolderCombo );
  } else {
    mFolderComboLabel->setText( i18n("&Resource folders are in account:") );
    mFolderComboLabel->setBuddy( mAccountCombo );
  }
  slotEmitChanged();
}


// *************************************************************
// *                                                           *
// *                     AccountUpdater                        *
// *                                                           *
// *************************************************************
AccountUpdater::AccountUpdater(ImapAccountBase *account)
    : QObject()
{
  mAccount = account;
}

void AccountUpdater::update()
{
  connect( mAccount, SIGNAL( connectionResult(int, const QString&) ),
          this, SLOT( namespacesFetched() ) );
  mAccount->makeConnection();
}

void AccountUpdater::namespacesFetched()
{
  mAccount->setCheckingMail( true );
  mAccount->processNewMail( false );
  deleteLater();
}

#undef DIM

//----------------------------
#include "configuredialog.moc"
