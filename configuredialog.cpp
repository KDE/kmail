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
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

// This must be first
#include <config.h>

// my headers:
#include "configuredialog.h"
#include "configuredialog_p.h"

#include "globalsettings.h"
#include "replyphrases.h"

// other KMail headers:
#include "simplestringlisteditor.h"
#include "accountdialog.h"
#include "colorlistbox.h"
#include "kmacctmgr.h"
#include "kmacctseldlg.h"
#include "kmsender.h"
#include "kmtransport.h"
#include "kmfoldermgr.h"
#include <libkpimidentities/identitymanager.h>
#include "identitylistview.h"
#include "kcursorsaver.h"
#include "kmkernel.h"
#include <composercryptoconfiguration.h>
#include <warningconfiguration.h>
#include <smimeconfiguration.h>
#include "folderrequester.h"
using KMail::FolderRequester;
#include "accountcombobox.h"
#include "imapaccountbase.h"
#include "folderstorage.h"
#include "kmfolder.h"
#include "kmmainwidget.h"

using KMail::IdentityListView;
using KMail::IdentityListViewItem;
#include "identitydialog.h"
using KMail::IdentityDialog;
using KMail::ImapAccountBase;

// other kdenetwork headers:
#include <libkpimidentities/identity.h>
#include <kmime_util.h>
using KMime::DateFormatter;
#include <kleo/cryptoconfig.h>
#include <kleo/cryptobackendfactory.h>
#include <ui/backendconfigwidget.h>
#include <ui/keyrequester.h>
#include <ui/keyselectiondialog.h>

// other KDE headers:
#include <klocale.h>
#include <kapplication.h>
#include <kcharsets.h>
#include <kdebug.h>
#include <knuminput.h>
#include <kfontdialog.h>
#include <kmessagebox.h>
#include <kurlrequester.h>
#include <kseparator.h>
#include <kiconloader.h>
#include <kstandarddirs.h>
#include <kwin.h>
#include <knotifydialog.h>
#include <kconfig.h>
#include <kactivelabel.h>
#include <kcmultidialog.h>

// Qt headers:
#include <qvalidator.h>
#include <qwhatsthis.h>
#include <qvgroupbox.h>
#include <qvbox.h>
#include <qvbuttongroup.h>
#include <qhbuttongroup.h>
#include <qtooltip.h>
#include <qlabel.h>
#include <qtextcodec.h>
#include <qheader.h>
#include <qpopupmenu.h>
#include <qradiobutton.h>
#include <qlayout.h>
#include <qcheckbox.h>
#include <qwidgetstack.h>

// other headers:
#include <assert.h>

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

  void checkLockDown( QWidget * w, const KConfigBase & c, const char * key ) {
    if ( c.entryIsImmutable( key ) ) {
      w->setEnabled( false );
      QToolTip::add( w, i18n( lockedDownWarning ) );
    } else {
      QToolTip::remove( w );
    }
  }

  void populateButtonGroup( QButtonGroup * g, const EnumConfigEntry & e ) {
    g->setTitle( i18n( e.desc ) );
    g->layout()->setSpacing( KDialog::spacingHint() );
    for ( int i = 0 ; i < e.numItems ; ++i )
      g->insert( new QRadioButton( i18n( e.items[i].desc ), g ), i );
  }

  void populateCheckBox( QCheckBox * b, const BoolConfigEntry & e ) {
    b->setText( i18n( e.desc ) );
  }

  void loadWidget( QCheckBox * b, const KConfigBase & c, const BoolConfigEntry & e ) {
    Q_ASSERT( c.group() == e.group );
    checkLockDown( b, c, e.key );
    b->setChecked( c.readBoolEntry( e.key, e.defaultValue ) );
  }

  void loadWidget( QButtonGroup * g, const KConfigBase & c, const EnumConfigEntry & e ) {
    Q_ASSERT( c.group() == e.group );
    Q_ASSERT( g->count() == e.numItems );
    checkLockDown( g, c, e.key );
    const QString s = c.readEntry( e.key, e.items[e.defaultItem].key );
    for ( int i = 0 ; i < e.numItems ; ++i )
      if ( s == e.items[i].key ) {
	g->setButton( i );
	return;
      }
    g->setButton( e.defaultItem );
  }

  void saveCheckBox( QCheckBox * b, KConfigBase & c, const BoolConfigEntry & e ) {
    Q_ASSERT( c.group() == e.group );
    c.writeEntry( e.key, b->isChecked() );
  }

  void saveButtonGroup( QButtonGroup * g, KConfigBase & c, const EnumConfigEntry & e ) {
    Q_ASSERT( c.group() == e.group );
    Q_ASSERT( g->count() == e.numItems );
    c.writeEntry( e.key, e.items[ g->id( g->selected() ) ].key );
  }

  template <typename T_Widget, typename T_Entry>
  inline void loadProfile( T_Widget * g, const KConfigBase & c, const T_Entry & e ) {
    if ( c.hasKey( e.key ) )
      loadWidget( g, c, e );
  }
}


ConfigureDialog::ConfigureDialog( QWidget *parent, const char *name, bool modal )
  : KCMultiDialog( KDialogBase::IconList, KGuiItem( i18n( "&Load Profile..." ) ),
                   KGuiItem(), User2, i18n( "Configure" ), parent, name, modal )
  , mProfileDialog( 0 )
{
  KWin::setIcons( winId(), kapp->icon(), kapp->miniIcon() );
  showButton( User1, true );

  addModule ( "kmail_config_identity", false );
  addModule ( "kmail_config_accounts", false );
  addModule ( "kmail_config_appearance", false );
  addModule ( "kmail_config_composer", false );
  addModule ( "kmail_config_security", false );
  addModule ( "kmail_config_misc", false );

  // We store the size of the dialog on hide, because otherwise
  // the KCMultiDialog starts with the size of the first kcm, not
  // the largest one. This way at least after the first showing of
  // the largest kcm the size is kept.
  KConfigGroup geometry( KMKernel::config(), "Geometry" );
  int width = geometry.readNumEntry( "ConfigureDialogWidth" );
  int height = geometry.readNumEntry( "ConfigureDialogHeight" );
  if ( width != 0 && height != 0 ) {
     setMinimumSize( width, height );
  }

}

void ConfigureDialog::hideEvent( QHideEvent *ev ) {
  KConfigGroup geometry( KMKernel::config(), "Geometry" );
  geometry.writeEntry( "ConfigureDialogWidth", width() );
  geometry.writeEntry( "ConfigureDialogHeight",height() );
  KDialogBase::hideEvent( ev );
}

ConfigureDialog::~ConfigureDialog() {
}

void ConfigureDialog::slotApply() {
  KCMultiDialog::slotApply();
  GlobalSettings::writeConfig();
}

void ConfigureDialog::slotOk() {
  KCMultiDialog::slotOk();
  GlobalSettings::writeConfig();
}

void ConfigureDialog::slotUser2() {
  if ( mProfileDialog ) {
    mProfileDialog->raise();
    return;
  }
  mProfileDialog = new ProfileDialog( this, "mProfileDialog" );
  connect( mProfileDialog, SIGNAL(profileSelected(KConfig*)),
	        this, SIGNAL(installProfile(KConfig*)) );
  mProfileDialog->show();
}

// *************************************************************
// *                                                           *
// *                      IdentityPage                         *
// *                                                           *
// *************************************************************
QString IdentityPage::helpAnchor() const {
  return QString::fromLatin1("configure-identity");
}

IdentityPage::IdentityPage( QWidget * parent, const char * name )
  : ConfigModule( parent, name ),
    mIdentityDialog( 0 )
{
  QHBoxLayout * hlay = new QHBoxLayout( this, 0, KDialog::spacingHint() );

  mIdentityList = new IdentityListView( this );
  connect( mIdentityList, SIGNAL(selectionChanged()),
	   SLOT(slotIdentitySelectionChanged()) );
  connect( mIdentityList, SIGNAL(itemRenamed(QListViewItem*,const QString&,int)),
	   SLOT(slotRenameIdentity(QListViewItem*,const QString&,int)) );
  connect( mIdentityList, SIGNAL(doubleClicked(QListViewItem*,const QPoint&,int)),
	   SLOT(slotModifyIdentity()) );
  connect( mIdentityList, SIGNAL(contextMenu(KListView*,QListViewItem*,const QPoint&)),
	   SLOT(slotContextMenu(KListView*,QListViewItem*,const QPoint&)) );
  // ### connect dragged(...), ...

  hlay->addWidget( mIdentityList, 1 );

  QVBoxLayout * vlay = new QVBoxLayout( hlay ); // inherits spacing

  QPushButton * button = new QPushButton( i18n("&Add..."), this );
  mModifyButton = new QPushButton( i18n("&Modify..."), this );
  mRenameButton = new QPushButton( i18n("&Rename"), this );
  mRemoveButton = new QPushButton( i18n("Remo&ve"), this );
  mSetAsDefaultButton = new QPushButton( i18n("Set as &Default"), this );
  button->setAutoDefault( false );
  mModifyButton->setAutoDefault( false );
  mModifyButton->setEnabled( false );
  mRenameButton->setAutoDefault( false );
  mRenameButton->setEnabled( false );
  mRemoveButton->setAutoDefault( false );
  mRemoveButton->setEnabled( false );
  mSetAsDefaultButton->setAutoDefault( false );
  mSetAsDefaultButton->setEnabled( false );
  connect( button, SIGNAL(clicked()),
	   this, SLOT(slotNewIdentity()) );
  connect( mModifyButton, SIGNAL(clicked()),
	   this, SLOT(slotModifyIdentity()) );
  connect( mRenameButton, SIGNAL(clicked()),
	   this, SLOT(slotRenameIdentity()) );
  connect( mRemoveButton, SIGNAL(clicked()),
	   this, SLOT(slotRemoveIdentity()) );
  connect( mSetAsDefaultButton, SIGNAL(clicked()),
	   this, SLOT(slotSetAsDefault()) );
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
  KPIM::IdentityManager * im = kmkernel->identityManager();
  mOldNumberOfIdentities = im->shadowIdentities().count();
  // Fill the list:
  mIdentityList->clear();
  QListViewItem * item = 0;
  for ( KPIM::IdentityManager::Iterator it = im->modifyBegin() ; it != im->modifyEnd() ; ++it )
    item = new IdentityListViewItem( mIdentityList, item, *it  );
  mIdentityList->setSelected( mIdentityList->currentItem(), true );
}

void IdentityPage::save() {
  assert( !mIdentityDialog );

  kmkernel->identityManager()->sort();
  kmkernel->identityManager()->commit();

  if( mOldNumberOfIdentities < 2 && mIdentityList->childCount() > 1 ) {
    // have more than one identity, so better show the combo in the
    // composer now:
    KConfigGroup composer( KMKernel::config(), "Composer" );
    int showHeaders = composer.readNumEntry( "headers", HDR_STANDARD );
    showHeaders |= HDR_IDENTITY;
    composer.writeEntry( "headers", showHeaders );
  }
  // and now the reverse
  if( mOldNumberOfIdentities > 1 && mIdentityList->childCount() < 2 ) {
    // have only one identity, so remove the combo in the composer:
    KConfigGroup composer( KMKernel::config(), "Composer" );
    int showHeaders = composer.readNumEntry( "headers", HDR_STANDARD );
    showHeaders &= ~HDR_IDENTITY;
    composer.writeEntry( "headers", showHeaders );
  }
}

void IdentityPage::slotNewIdentity()
{
  assert( !mIdentityDialog );

  KPIM::IdentityManager * im = kmkernel->identityManager();
  NewIdentityDialog dialog( im->shadowIdentities(), this, "new", true );

  if( dialog.exec() == QDialog::Accepted ) {
    QString identityName = dialog.identityName().stripWhiteSpace();
    assert( !identityName.isEmpty() );

    //
    // Construct a new Identity:
    //
    switch ( dialog.duplicateMode() ) {
    case NewIdentityDialog::ExistingEntry:
      {
	KPIM::Identity & dupThis = im->modifyIdentityForName( dialog.duplicateIdentity() );
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
    KPIM::Identity & newIdent = im->modifyIdentityForName( identityName );
    QListViewItem * item = mIdentityList->selectedItem();
    if ( item )
      item = item->itemAbove();
    mIdentityList->setSelected( new IdentityListViewItem( mIdentityList,
							  /*after*/ item,
							  newIdent ), true );
    slotModifyIdentity();
  }
}

void IdentityPage::slotModifyIdentity() {
  assert( !mIdentityDialog );

  IdentityListViewItem * item =
    dynamic_cast<IdentityListViewItem*>( mIdentityList->selectedItem() );
  if ( !item ) return;

  mIdentityDialog = new IdentityDialog( this );
  mIdentityDialog->setIdentity( item->identity() );

  // Hmm, an unmodal dialog would be nicer, but a modal one is easier ;-)
  if ( mIdentityDialog->exec() == QDialog::Accepted ) {
    mIdentityDialog->updateIdentity( item->identity() );
    item->redisplay();
    emit changed(true);
  }

  delete mIdentityDialog;
  mIdentityDialog = 0;
}

void IdentityPage::slotRemoveIdentity()
{
  assert( !mIdentityDialog );

  KPIM::IdentityManager * im = kmkernel->identityManager();
  kdFatal( im->shadowIdentities().count() < 2 )
    << "Attempted to remove the last identity!" << endl;

  IdentityListViewItem * item =
    dynamic_cast<IdentityListViewItem*>( mIdentityList->selectedItem() );
  if ( !item ) return;

  QString msg = i18n("<qt>Do you really want to remove the identity named "
		     "<b>%1</b>?</qt>").arg( item->identity().identityName() );
  if( KMessageBox::warningContinueCancel( this, msg, i18n("Remove Identity"),
   KGuiItem(i18n("&Remove"),"editdelete") ) == KMessageBox::Continue )
    if ( im->removeIdentity( item->identity().identityName() ) ) {
      delete item;
      mIdentityList->setSelected( mIdentityList->currentItem(), true );
      refreshList();
    }
}

void IdentityPage::slotRenameIdentity() {
  assert( !mIdentityDialog );

  QListViewItem * item = mIdentityList->selectedItem();
  if ( !item ) return;

  mIdentityList->rename( item, 0 );
}

void IdentityPage::slotRenameIdentity( QListViewItem * i,
				       const QString & s, int col ) {
  assert( col == 0 );
  Q_UNUSED( col );

  IdentityListViewItem * item = dynamic_cast<IdentityListViewItem*>( i );
  if ( !item ) return;

  QString newName = s.stripWhiteSpace();
  if ( !newName.isEmpty() &&
       !kmkernel->identityManager()->shadowIdentities().contains( newName ) ) {
    KPIM::Identity & ident = item->identity();
    ident.setIdentityName( newName );
    emit changed(true);
  }
  item->redisplay();
}

void IdentityPage::slotContextMenu( KListView *, QListViewItem * i,
				    const QPoint & pos ) {
  IdentityListViewItem * item = dynamic_cast<IdentityListViewItem*>( i );

  QPopupMenu * menu = new QPopupMenu( this );
  menu->insertItem( i18n("Add..."), this, SLOT(slotNewIdentity()) );
  if ( item ) {
    menu->insertItem( i18n("Modify..."), this, SLOT(slotModifyIdentity()) );
    if ( mIdentityList->childCount() > 1 )
      menu->insertItem( i18n("Remove"), this, SLOT(slotRemoveIdentity()) );
    if ( !item->identity().isDefault() )
      menu->insertItem( i18n("Set as Default"), this, SLOT(slotSetAsDefault()) );
  }
  menu->exec( pos );
  delete menu;
}


void IdentityPage::slotSetAsDefault() {
  assert( !mIdentityDialog );

  IdentityListViewItem * item =
    dynamic_cast<IdentityListViewItem*>( mIdentityList->selectedItem() );
  if ( !item ) return;

  KPIM::IdentityManager * im = kmkernel->identityManager();
  im->setAsDefault( item->identity().identityName() );
  refreshList();
}

void IdentityPage::refreshList() {
  for ( QListViewItemIterator it( mIdentityList ) ; it.current() ; ++it ) {
    IdentityListViewItem * item =
      dynamic_cast<IdentityListViewItem*>(it.current());
    if ( item )
      item->redisplay();
  }
  emit changed(true);
}

void IdentityPage::slotIdentitySelectionChanged()
{
  IdentityListViewItem *item =
    dynamic_cast<IdentityListViewItem*>( mIdentityList->selectedItem() );

  mRemoveButton->setEnabled( item && mIdentityList->childCount() > 1 );
  mModifyButton->setEnabled( item );
  mRenameButton->setEnabled( item );
  mSetAsDefaultButton->setEnabled( item && !item->identity().isDefault() );
}

void IdentityPage::slotUpdateTransportCombo( const QStringList & sl )
{
  if ( mIdentityDialog ) mIdentityDialog->slotUpdateTransportCombo( sl );
}



// *************************************************************
// *                                                           *
// *                       AccountsPage                         *
// *                                                           *
// *************************************************************
QString AccountsPage::helpAnchor() const {
  return QString::fromLatin1("configure-accounts");
}

AccountsPage::AccountsPage( QWidget * parent, const char * name )
  : ConfigModuleWithTabs( parent, name )
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
  connect( mSendingTab, SIGNAL(transportListChanged(const QStringList&)),
	   this, SIGNAL(transportListChanged(const QStringList&)) );

  load();
}

QString AccountsPage::SendingTab::helpAnchor() const {
  return QString::fromLatin1("configure-accounts-sending");
}

AccountsPageSendingTab::AccountsPageSendingTab( QWidget * parent, const char * name )
  : ConfigModuleTab( parent, name )
{
  mTransportInfoList.setAutoDelete( true );
  // temp. vars:
  QVBoxLayout *vlay;
  QVBoxLayout *btn_vlay;
  QHBoxLayout *hlay;
  QGridLayout *glay;
  QPushButton *button;
  QGroupBox   *group;

  vlay = new QVBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint() );
  // label: zero stretch ### FIXME more
  vlay->addWidget( new QLabel( i18n("Outgoing accounts (add at least one):"), this ) );

  // hbox layout: stretch 10, spacing inherited from vlay
  hlay = new QHBoxLayout();
  vlay->addLayout( hlay, 10 ); // high stretch b/c of the groupbox's sizeHint

  // transport list: left widget in hlay; stretch 1
  // ### FIXME: allow inline renaming of the account:
  mTransportList = new ListView( this, "transportList", 5 );
  mTransportList->addColumn( i18n("Name") );
  mTransportList->addColumn( i18n("Type") );
  mTransportList->setAllColumnsShowFocus( true );
  mTransportList->setFrameStyle( QFrame::WinPanel + QFrame::Sunken );
  mTransportList->setSorting( -1 );
  connect( mTransportList, SIGNAL(selectionChanged()),
           this, SLOT(slotTransportSelected()) );
  connect( mTransportList, SIGNAL(doubleClicked( QListViewItem *)),
           this, SLOT(slotModifySelectedTransport()) );
  hlay->addWidget( mTransportList, 1 );

  // a vbox layout for the buttons: zero stretch, spacing inherited from hlay
  btn_vlay = new QVBoxLayout( hlay );

  // "add..." button: stretch 0
  button = new QPushButton( i18n("A&dd..."), this );
  button->setAutoDefault( false );
  connect( button, SIGNAL(clicked()),
	   this, SLOT(slotAddTransport()) );
  btn_vlay->addWidget( button );

  // "modify..." button: stretch 0
  mModifyTransportButton = new QPushButton( i18n("&Modify..."), this );
  mModifyTransportButton->setAutoDefault( false );
  mModifyTransportButton->setEnabled( false ); // b/c no item is selected yet
  connect( mModifyTransportButton, SIGNAL(clicked()),
	   this, SLOT(slotModifySelectedTransport()) );
  btn_vlay->addWidget( mModifyTransportButton );

  // "remove" button: stretch 0
  mRemoveTransportButton = new QPushButton( i18n("R&emove"), this );
  mRemoveTransportButton->setAutoDefault( false );
  mRemoveTransportButton->setEnabled( false ); // b/c no item is selected yet
  connect( mRemoveTransportButton, SIGNAL(clicked()),
	   this, SLOT(slotRemoveSelectedTransport()) );
  btn_vlay->addWidget( mRemoveTransportButton );

  // "up" button: stretch 0
  // ### FIXME: shouldn't this be a QToolButton?
  mTransportUpButton = new QPushButton( QString::null, this );
  mTransportUpButton->setIconSet( BarIconSet( "up", KIcon::SizeSmall ) );
  //  mTransportUpButton->setPixmap( BarIcon( "up", KIcon::SizeSmall ) );
  mTransportUpButton->setAutoDefault( false );
  mTransportUpButton->setEnabled( false ); // b/c no item is selected yet
  connect( mTransportUpButton, SIGNAL(clicked()),
           this, SLOT(slotTransportUp()) );
  btn_vlay->addWidget( mTransportUpButton );

  // "down" button: stretch 0
  // ### FIXME: shouldn't this be a QToolButton?
  mTransportDownButton = new QPushButton( QString::null, this );
  mTransportDownButton->setIconSet( BarIconSet( "down", KIcon::SizeSmall ) );
  //  mTransportDownButton->setPixmap( BarIcon( "down", KIcon::SizeSmall ) );
  mTransportDownButton->setAutoDefault( false );
  mTransportDownButton->setEnabled( false ); // b/c no item is selected yet
  connect( mTransportDownButton, SIGNAL(clicked()),
           this, SLOT(slotTransportDown()) );
  btn_vlay->addWidget( mTransportDownButton );
  btn_vlay->addStretch( 1 ); // spacer

  // "Common options" groupbox:
  group = new QGroupBox( 0, Qt::Vertical,
			 i18n("Common Options"), this );
  vlay->addWidget(group);

  // a grid layout for the contents of the "common options" group box
  glay = new QGridLayout( group->layout(), 5, 3, KDialog::spacingHint() );
  glay->setColStretch( 2, 10 );

  // "confirm before send" check box:
  mConfirmSendCheck = new QCheckBox( i18n("Confirm &before send"), group );
  glay->addMultiCellWidget( mConfirmSendCheck, 0, 0, 0, 1 );
  connect( mConfirmSendCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // "send on check" combo:
  mSendOnCheckCombo = new QComboBox( false, group );
  mSendOnCheckCombo->insertStringList( QStringList()
				      << i18n("Never Automatically")
				      << i18n("On Manual Mail Checks")
                                      << i18n("On All Mail Checks") );
  glay->addWidget( mSendOnCheckCombo, 1, 1 );
  connect( mSendOnCheckCombo, SIGNAL( activated( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // "default send method" combo:
  mSendMethodCombo = new QComboBox( false, group );
  mSendMethodCombo->insertStringList( QStringList()
				      << i18n("Send Now")
				      << i18n("Send Later") );
  glay->addWidget( mSendMethodCombo, 2, 1 );
  connect( mSendMethodCombo, SIGNAL( activated( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );


  // "message property" combo:
  // ### FIXME: remove completely?
  mMessagePropertyCombo = new QComboBox( false, group );
  mMessagePropertyCombo->insertStringList( QStringList()
		     << i18n("Allow 8-bit")
		     << i18n("MIME Compliant (Quoted Printable)") );
  glay->addWidget( mMessagePropertyCombo, 3, 1 );
  connect( mMessagePropertyCombo, SIGNAL( activated( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // "default domain" input field:
  mDefaultDomainEdit = new KLineEdit( group );
  glay->addMultiCellWidget( mDefaultDomainEdit, 4, 4, 1, 2 );
  connect( mDefaultDomainEdit, SIGNAL( textChanged( const QString& ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // labels:
  QLabel *l =  new QLabel( mSendOnCheckCombo, /*buddy*/
                            i18n("Send &messages in outbox folder:"), group );
  glay->addWidget( l, 1, 0 );

  QString msg = i18n( GlobalSettings::self()->sendOnCheckItem()->whatsThis().utf8() );
  QWhatsThis::add( l, msg );
  QWhatsThis::add( mSendOnCheckCombo, msg );

  glay->addWidget( new QLabel( mSendMethodCombo, /*buddy*/
			       i18n("Defa&ult send method:"), group ), 2, 0 );
  glay->addWidget( new QLabel( mMessagePropertyCombo, /*buddy*/
			       i18n("Message &property:"), group ), 3, 0 );
  l = new QLabel( mDefaultDomainEdit, /*buddy*/
                          i18n("Defaul&t domain:"), group );
  glay->addWidget( l, 4, 0 );

  // and now: add QWhatsThis:
  msg = i18n( "<qt><p>The default domain is used to complete email "
              "addresses that only consist of the user's name."
              "</p></qt>" );
  QWhatsThis::add( l, msg );
  QWhatsThis::add( mDefaultDomainEdit, msg );
}


void AccountsPage::SendingTab::slotTransportSelected()
{
  QListViewItem *cur = mTransportList->selectedItem();
  mModifyTransportButton->setEnabled( cur );
  mRemoveTransportButton->setEnabled( cur );
  mTransportDownButton->setEnabled( cur && cur->itemBelow() );
  mTransportUpButton->setEnabled( cur && cur->itemAbove() );
}

// adds a number to @p name to make the name unique
static inline QString uniqueName( const QStringList & list,
				  const QString & name )
{
  int suffix = 1;
  QString result = name;
  while ( list.find( result ) != list.end() ) {
    result = i18n("%1: name; %2: number appended to it to make it unique "
		  "among a list of names", "%1 %2")
      .arg( name ).arg( suffix );
    suffix++;
  }
  return result;
}

void AccountsPage::SendingTab::slotAddTransport()
{
  int transportType;

  { // limit scope of selDialog
    KMTransportSelDlg selDialog( this );
    if ( selDialog.exec() != QDialog::Accepted ) return;
    transportType = selDialog.selected();
  }

  KMTransportInfo *transportInfo = new KMTransportInfo();
  switch ( transportType ) {
  case 0: // smtp
    transportInfo->type = QString::fromLatin1("smtp");
    break;
  case 1: // sendmail
    transportInfo->type = QString::fromLatin1("sendmail");
    transportInfo->name = i18n("Sendmail");
    transportInfo->host = _PATH_SENDMAIL; // ### FIXME: use const, not #define
    break;
  default:
    assert( 0 );
  }

  KMTransportDialog dialog( i18n("Add Transport"), transportInfo, this );

  // create list of names:
  // ### move behind dialog.exec()?
  QStringList transportNames;
  QPtrListIterator<KMTransportInfo> it( mTransportInfoList );
  for ( it.toFirst() ; it.current() ; ++it )
    transportNames << (*it)->name;

  if( dialog.exec() != QDialog::Accepted ) {
    delete transportInfo;
    return;
  }

  // disambiguate the name by appending a number:
  // ### FIXME: don't allow this error to happen in the first place!
  transportInfo->name = uniqueName( transportNames, transportInfo->name );
  // append to names and transportinfo lists:
  transportNames << transportInfo->name;
  mTransportInfoList.append( transportInfo );

  // append to listview:
  // ### FIXME: insert before the selected item, append on empty selection
  QListViewItem *lastItem = mTransportList->firstChild();
  QString typeDisplayName;
  if ( lastItem )
    while ( lastItem->nextSibling() )
      lastItem = lastItem->nextSibling();
  if ( lastItem )
    typeDisplayName = transportInfo->type;
  else
    typeDisplayName = i18n("%1: type of transport. Result used in "
			   "Configure->Accounts->Sending listview, \"type\" "
			   "column, first row, to indicate that this is the "
			   "default transport", "%1 (Default)")
      .arg( transportInfo->type );
  (void) new QListViewItem( mTransportList, lastItem, transportInfo->name,
			    typeDisplayName );

  // notify anyone who cares:
  emit transportListChanged( transportNames );
  emit changed( true );
}

void AccountsPage::SendingTab::slotModifySelectedTransport()
{
  QListViewItem *item = mTransportList->selectedItem();
  if ( !item ) return;

  QPtrListIterator<KMTransportInfo> it( mTransportInfoList );
  for ( it.toFirst() ; it.current() ; ++it )
    if ( (*it)->name == item->text(0) ) break;
  if ( !it.current() ) return;

  KMTransportDialog dialog( i18n("Modify Transport"), (*it), this );

  if ( dialog.exec() != QDialog::Accepted ) return;

  // create the list of names of transports, but leave out the current
  // item:
  QStringList transportNames;
  QPtrListIterator<KMTransportInfo> jt( mTransportInfoList );
  int entryLocation = -1;
  for ( jt.toFirst() ; jt.current() ; ++jt )
    if ( jt != it )
      transportNames << (*jt)->name;
    else
      entryLocation = transportNames.count();
  assert( entryLocation >= 0 );

  // make the new name unique by appending a high enough number:
  (*it)->name = uniqueName( transportNames, (*it)->name );
  // change the list item to the new name
  item->setText( 0, (*it)->name );
  // and insert the new name at the position of the old in the list of
  // strings; then broadcast the new list:
  transportNames.insert( transportNames.at( entryLocation ), (*it)->name );
  emit transportListChanged( transportNames );
  emit changed( true );
}


void AccountsPage::SendingTab::slotRemoveSelectedTransport()
{
  QListViewItem *item = mTransportList->selectedItem();
  if ( !item ) return;

  QPtrListIterator<KMTransportInfo> it( mTransportInfoList );
  for ( it.toFirst() ; it.current() ; ++it )
    if ( (*it)->name == item->text(0) ) break;
  if ( !it.current() ) return;

  QListViewItem *newCurrent = item->itemBelow();
  if ( !newCurrent ) newCurrent = item->itemAbove();
  //mTransportList->removeItem( item );
  if ( newCurrent ) {
    mTransportList->setCurrentItem( newCurrent );
    mTransportList->setSelected( newCurrent, true );
  }

  delete item;
  mTransportInfoList.remove( it );

  QStringList transportNames;
  for ( it.toFirst() ; it.current() ; ++it )
    transportNames << (*it)->name;
  emit transportListChanged( transportNames );
  emit changed( true );
}


void AccountsPage::SendingTab::slotTransportUp()
{
  QListViewItem *item = mTransportList->selectedItem();
  if ( !item ) return;
  QListViewItem *above = item->itemAbove();
  if ( !above ) return;

  // swap in the transportInfo list:
  // ### FIXME: use value-based list. This is ugly.
  KMTransportInfo *ti, *ti2 = 0;
  int i = 0;
  for (ti = mTransportInfoList.first(); ti;
    ti2 = ti, ti = mTransportInfoList.next(), i++)
      if (ti->name == item->text(0)) break;
  if (!ti || !ti2) return;
  ti = mTransportInfoList.take(i);
  mTransportInfoList.insert(i-1, ti);

  // swap in the display
  item->setText(0, ti2->name);
  item->setText(1, ti2->type);
  above->setText(0, ti->name);
  if ( above->itemAbove() )
    // not first:
    above->setText( 1, ti->type );
  else
    // first:
    above->setText( 1, i18n("%1: type of transport. Result used in "
			    "Configure->Accounts->Sending listview, \"type\" "
			    "column, first row, to indicate that this is the "
			    "default transport", "%1 (Default)")
		    .arg( ti->type ) );

  mTransportList->setCurrentItem( above );
  mTransportList->setSelected( above, true );
  emit changed( true );
}


void AccountsPage::SendingTab::slotTransportDown()
{
  QListViewItem * item = mTransportList->selectedItem();
  if ( !item ) return;
  QListViewItem * below = item->itemBelow();
  if ( !below ) return;

  KMTransportInfo *ti, *ti2 = 0;
  int i = 0;
  for (ti = mTransportInfoList.first(); ti;
       ti = mTransportInfoList.next(), i++)
    if (ti->name == item->text(0)) break;
  ti2 = mTransportInfoList.next();
  if (!ti || !ti2) return;
  ti = mTransportInfoList.take(i);
  mTransportInfoList.insert(i+1, ti);

  item->setText(0, ti2->name);
  below->setText(0, ti->name);
  below->setText(1, ti->type);
  if ( item->itemAbove() )
    item->setText( 1, ti2->type );
  else
    item->setText( 1, i18n("%1: type of transport. Result used in "
			   "Configure->Accounts->Sending listview, \"type\" "
			   "column, first row, to indicate that this is the "
			   "default transport", "%1 (Default)")
		   .arg( ti2->type ) );


  mTransportList->setCurrentItem(below);
  mTransportList->setSelected(below, TRUE);
  emit changed( true );
}

void AccountsPage::SendingTab::load() {
  KConfigGroup general( KMKernel::config(), "General");
  KConfigGroup composer( KMKernel::config(), "Composer");

  int numTransports = general.readNumEntry("transports", 0);

  QListViewItem *top = 0;
  mTransportInfoList.clear();
  mTransportList->clear();
  QStringList transportNames;
  for ( int i = 1 ; i <= numTransports ; i++ ) {
    KMTransportInfo *ti = new KMTransportInfo();
    ti->readConfig(i);
    mTransportInfoList.append( ti );
    transportNames << ti->name;
    top = new QListViewItem( mTransportList, top, ti->name, ti->type );
  }
  emit transportListChanged( transportNames );

  QListViewItem *listItem = mTransportList->firstChild();
  if ( listItem ) {
    listItem->setText( 1, i18n("%1: type of transport. Result used in "
			       "Configure->Accounts->Sending listview, "
			       "\"type\" column, first row, to indicate "
			       "that this is the default transport",
			       "%1 (Default)").arg( listItem->text(1) ) );
    mTransportList->setCurrentItem( listItem );
    mTransportList->setSelected( listItem, true );
  }

  mSendMethodCombo->setCurrentItem(
		kmkernel->msgSender()->sendImmediate() ? 0 : 1 );
  mMessagePropertyCombo->setCurrentItem(
                kmkernel->msgSender()->sendQuotedPrintable() ? 1 : 0 );

  mConfirmSendCheck->setChecked( composer.readBoolEntry( "confirm-before-send",
							 false ) );
  mSendOnCheckCombo->setCurrentItem( GlobalSettings::sendOnCheck() );
  QString str = general.readEntry( "Default domain" );
  if( str.isEmpty() )
  {
    //### FIXME: Use the global convenience function instead of the homebrewed
    //           solution once we can rely on HEAD kdelibs.
    //str = KGlobal::hostname(); ???????
    char buffer[256];
    if ( !gethostname( buffer, 255 ) )
      // buffer need not be NUL-terminated if it has full length
      buffer[255] = 0;
    else
      buffer[0] = 0;
    str = QString::fromLatin1( *buffer ? buffer : "localhost" );
  }
  mDefaultDomainEdit->setText( str );
}


void AccountsPage::SendingTab::save() {
  KConfigGroup general( KMKernel::config(), "General" );
  KConfigGroup composer( KMKernel::config(), "Composer" );

  // Save transports:
  general.writeEntry( "transports", mTransportInfoList.count() );
  QPtrListIterator<KMTransportInfo> it( mTransportInfoList );
  for ( int i = 1 ; it.current() ; ++it, ++i )
    (*it)->writeConfig(i);

  // Save common options:
  GlobalSettings::setSendOnCheck( mSendOnCheckCombo->currentItem() );
  kmkernel->msgSender()->setSendImmediate(
			     mSendMethodCombo->currentItem() == 0 );
  kmkernel->msgSender()->setSendQuotedPrintable(
			     mMessagePropertyCombo->currentItem() == 1 );
  kmkernel->msgSender()->writeConfig( false ); // don't sync
  composer.writeEntry("confirm-before-send", mConfirmSendCheck->isChecked() );
  general.writeEntry( "Default domain", mDefaultDomainEdit->text() );
}

QString AccountsPage::ReceivingTab::helpAnchor() const {
  return QString::fromLatin1("configure-accounts-receiving");
}

AccountsPageReceivingTab::AccountsPageReceivingTab( QWidget * parent, const char * name )
  : ConfigModuleTab ( parent, name )
{
  // temp. vars:
  QVBoxLayout *vlay;
  QVBoxLayout *btn_vlay;
  QHBoxLayout *hlay;
  QPushButton *button;
  QGroupBox   *group;

  vlay = new QVBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint() );

  // label: zero stretch
  vlay->addWidget( new QLabel( i18n("Incoming accounts (add at least one):"), this ) );

  // hbox layout: stretch 10, spacing inherited from vlay
  hlay = new QHBoxLayout();
  vlay->addLayout( hlay, 10 ); // high stretch to suppress groupbox's growing

  // account list: left widget in hlay; stretch 1
  mAccountList = new ListView( this, "accountList", 5 );
  mAccountList->addColumn( i18n("Name") );
  mAccountList->addColumn( i18n("Type") );
  mAccountList->addColumn( i18n("Folder") );
  mAccountList->setAllColumnsShowFocus( true );
  mAccountList->setFrameStyle( QFrame::WinPanel + QFrame::Sunken );
  mAccountList->setSorting( -1 );
  connect( mAccountList, SIGNAL(selectionChanged()),
	   this, SLOT(slotAccountSelected()) );
  connect( mAccountList, SIGNAL(doubleClicked( QListViewItem *)),
	   this, SLOT(slotModifySelectedAccount()) );
  hlay->addWidget( mAccountList, 1 );

  // a vbox layout for the buttons: zero stretch, spacing inherited from hlay
  btn_vlay = new QVBoxLayout( hlay );

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
  group = new QVGroupBox( i18n("New Mail Notification"), this );
  vlay->addWidget( group );
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
  QToolTip::add( mVerboseNotificationCheck,
                 i18n( "Show for each folder the number of newly arrived "
                       "messages" ) );
  QWhatsThis::add( mVerboseNotificationCheck,
    GlobalSettings::self()->verboseNewMailNotificationItem()->whatsThis() );
  connect( mVerboseNotificationCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged() ) );

  // "Other Actions" button:
  mOtherNewMailActionsButton = new QPushButton( i18n("Other Actio&ns"), group );
  mOtherNewMailActionsButton->setSizePolicy( QSizePolicy( QSizePolicy::Fixed,
                                                          QSizePolicy::Fixed ) );
  connect( mOtherNewMailActionsButton, SIGNAL(clicked()),
           this, SLOT(slotEditNotifications()) );
}


void AccountsPage::ReceivingTab::slotAccountSelected()
{
  QListViewItem * item = mAccountList->selectedItem();
  mModifyAccountButton->setEnabled( item );
  mRemoveAccountButton->setEnabled( item );
}

QStringList AccountsPage::ReceivingTab::occupiedNames()
{
  QStringList accountNames = kmkernel->acctMgr()->getAccounts();

  QValueList<ModifiedAccountsType*>::Iterator k;
  for (k = mModifiedAccounts.begin(); k != mModifiedAccounts.end(); ++k )
    if ((*k)->oldAccount)
      accountNames.remove( (*k)->oldAccount->name() );

  QValueList< QGuardedPtr<KMAccount> >::Iterator l;
  for (l = mAccountsToDelete.begin(); l != mAccountsToDelete.end(); ++l )
    if (*l)
      accountNames.remove( (*l)->name() );

  QValueList< QGuardedPtr<KMAccount> >::Iterator it;
  for (it = mNewAccounts.begin(); it != mNewAccounts.end(); ++it )
    if (*it)
      accountNames += (*it)->name();

  QValueList<ModifiedAccountsType*>::Iterator j;
  for (j = mModifiedAccounts.begin(); j != mModifiedAccounts.end(); ++j )
    accountNames += (*j)->newAccount->name();

  return accountNames;
}

void AccountsPage::ReceivingTab::slotAddAccount() {
  KMAcctSelDlg accountSelectorDialog( this );
  if( accountSelectorDialog.exec() != QDialog::Accepted ) return;

  const char *accountType = 0;
  switch ( accountSelectorDialog.selected() ) {
    case 0: accountType = "local";      break;
    case 1: accountType = "pop";        break;
    case 2: accountType = "imap";       break;
    case 3: accountType = "cachedimap"; break;
    case 4: accountType = "maildir";    break;

    default:
      // ### FIXME: How should this happen???
      // replace with assert.
      KMessageBox::sorry( this, i18n("Unknown account type selected") );
      return;
  }

  KMAccount *account
    = kmkernel->acctMgr()->create( QString::fromLatin1( accountType ) );
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

  account->setName( uniqueName( accountNames, account->name() ) );

  QListViewItem *after = mAccountList->firstChild();
  while ( after && after->nextSibling() )
    after = after->nextSibling();

  QListViewItem *listItem =
    new QListViewItem( mAccountList, after, account->name(), account->type() );
  if( account->folder() )
    listItem->setText( 2, account->folder()->label() );

  mNewAccounts.append( account );
  emit changed( true );
}



void AccountsPage::ReceivingTab::slotModifySelectedAccount()
{
  QListViewItem *listItem = mAccountList->selectedItem();
  if( !listItem ) return;

  KMAccount *account = 0;
  QValueList<ModifiedAccountsType*>::Iterator j;
  for (j = mModifiedAccounts.begin(); j != mModifiedAccounts.end(); ++j )
    if ( (*j)->newAccount->name() == listItem->text(0) ) {
      account = (*j)->newAccount;
      break;
    }

  if ( !account ) {
    QValueList< QGuardedPtr<KMAccount> >::Iterator it;
    for ( it = mNewAccounts.begin() ; it != mNewAccounts.end() ; ++it )
      if ( (*it)->name() == listItem->text(0) ) {
	account = *it;
	break;
      }

    if ( !account ) {
      account = kmkernel->acctMgr()->findByName( listItem->text(0) );
      if( !account ) {
	// ### FIXME: How should this happen? See above.
        KMessageBox::sorry( this, i18n("Unable to locate account") );
        return;
      }

      ModifiedAccountsType *mod = new ModifiedAccountsType;
      mod->oldAccount = account;
      mod->newAccount = kmkernel->acctMgr()->create( account->type(),
						   account->name() );
      mod->newAccount->pseudoAssign( account );
      mModifiedAccounts.append( mod );
      account = mod->newAccount;
    }

    if( !account ) {
      // ### FIXME: See above.
      KMessageBox::sorry( this, i18n("Unable to locate account") );
      return;
    }
  }

  QStringList accountNames = occupiedNames();
  accountNames.remove( account->name() );

  AccountDialog dialog( i18n("Modify Account"), account, this );

  if( dialog.exec() != QDialog::Accepted ) return;

  account->setName( uniqueName( accountNames, account->name() ) );

  listItem->setText( 0, account->name() );
  listItem->setText( 1, account->type() );
  if( account->folder() )
    listItem->setText( 2, account->folder()->label() );

  emit changed( true );
}



void AccountsPage::ReceivingTab::slotRemoveSelectedAccount() {
  QListViewItem *listItem = mAccountList->selectedItem();
  if( !listItem ) return;

  KMAccount *acct = 0;
  QValueList<ModifiedAccountsType*>::Iterator j;
  for ( j = mModifiedAccounts.begin() ; j != mModifiedAccounts.end() ; ++j )
    if ( (*j)->newAccount->name() == listItem->text(0) ) {
      acct = (*j)->oldAccount;
      mAccountsToDelete.append( acct );
      mModifiedAccounts.remove( j );
      break;
    }
  if ( !acct ) {
    QValueList< QGuardedPtr<KMAccount> >::Iterator it;
    for ( it = mNewAccounts.begin() ; it != mNewAccounts.end() ; ++it )
      if ( (*it)->name() == listItem->text(0) ) {
	acct = *it;
	mNewAccounts.remove( it );
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
    KMessageBox::sorry( this, i18n("<qt>Unable to locate account <b>%1</b>.</qt>")
			.arg(listItem->text(0)) );
    return;
  }

  QListViewItem * item = listItem->itemBelow();
  if ( !item ) item = listItem->itemAbove();
  delete listItem;

  if ( item )
    mAccountList->setSelected( item, true );

  emit changed( true );
}

void AccountsPage::ReceivingTab::slotEditNotifications()
{
  if(kmkernel->xmlGuiInstance())
    KNotifyDialog::configure(this, 0, kmkernel->xmlGuiInstance()->aboutData());
  else
    KNotifyDialog::configure(this);
}

void AccountsPage::ReceivingTab::load() {
  KConfigGroup general( KMKernel::config(), "General" );

  mAccountList->clear();
  QListViewItem *top = 0;
  for( KMAccount *a = kmkernel->acctMgr()->first(); a!=0;
       a = kmkernel->acctMgr()->next() ) {
    QListViewItem *listItem =
      new QListViewItem( mAccountList, top, a->name(), a->type() );
    if( a->folder() )
      listItem->setText( 2, a->folder()->label() );
    top = listItem;
  }

  QListViewItem *listItem = mAccountList->firstChild();
  if ( listItem ) {
    mAccountList->setCurrentItem( listItem );
    mAccountList->setSelected( listItem, true );
  }

  mBeepNewMailCheck->setChecked( general.readBoolEntry("beep-on-mail", false ) );
  mVerboseNotificationCheck->setChecked( GlobalSettings::verboseNewMailNotification() );
  mCheckmailStartupCheck->setChecked( general.readBoolEntry("checkmail-startup", false) );
}

void AccountsPage::ReceivingTab::save() {
  // Add accounts marked as new
  QValueList< QGuardedPtr<KMAccount> > newCachedImapAccounts;
  QValueList< QGuardedPtr<KMAccount> >::Iterator it;
  for (it = mNewAccounts.begin(); it != mNewAccounts.end(); ++it ) {
    kmkernel->acctMgr()->add( *it );
    // remember new Disconnected IMAP accounts because they are needed again
    if( (*it)->isA( "KMAcctCachedImap" ) ) {
      newCachedImapAccounts.append( *it );
    }
  }

  mNewAccounts.clear();

  // Update accounts that have been modified
  QValueList<ModifiedAccountsType*>::Iterator j;
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
      KMessageBox::sorry( this, i18n("<qt>Unable to locate account <b>%1</b>.</qt>")
			  .arg( (*it)->name() ) );
  }
  mAccountsToDelete.clear();

  // Incoming mail
  kmkernel->acctMgr()->writeConfig( false );
  kmkernel->cleanupImapFolders();

  // Save Mail notification settings
  KConfigGroup general( KMKernel::config(), "General" );
  general.writeEntry( "beep-on-mail", mBeepNewMailCheck->isChecked() );
  GlobalSettings::setVerboseNewMailNotification( mVerboseNotificationCheck->isChecked() );

  general.writeEntry( "checkmail-startup", mCheckmailStartupCheck->isChecked() );

  // Sync new IMAP accounts ASAP:
  for (it = newCachedImapAccounts.begin(); it != newCachedImapAccounts.end(); ++it ) {
    KMAccount *acc = (*it);
    if ( !acc->checkingMail() ) {
      acc->setCheckingMail( true );
      acc->processNewMail(false);
    }
  }
}

// *************************************************************
// *                                                           *
// *                     AppearancePage                        *
// *                                                           *
// *************************************************************
QString AppearancePage::helpAnchor() const {
  return QString::fromLatin1("configure-appearance");
}

AppearancePage::AppearancePage( QWidget * parent, const char * name )
  : ConfigModuleWithTabs( parent, name )
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
  addTab( mHeadersTab, i18n("H&eaders") );

  //
  // "System Tray" tab:
  //
  mSystemTrayTab = new SystemTrayTab();
  addTab( mSystemTrayTab, i18n("System Tray") );

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

AppearancePageFontsTab::AppearancePageFontsTab( QWidget * parent, const char * name )
  : ConfigModuleTab( parent, name ), mActiveFontIndex( -1 )
{
  assert( numFontNames == sizeof mFont / sizeof *mFont );
  // tmp. vars:
  QVBoxLayout *vlay;
  QHBoxLayout *hlay;
  QLabel      *label;

  // "Use custom fonts" checkbox, followed by <hr>
  vlay = new QVBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint() );
  mCustomFontCheck = new QCheckBox( i18n("&Use custom fonts"), this );
  vlay->addWidget( mCustomFontCheck );
  vlay->addWidget( new KSeparator( KSeparator::HLine, this ) );
  connect ( mCustomFontCheck, SIGNAL( stateChanged( int ) ),
            this, SLOT( slotEmitChanged( void ) ) );

  // "font location" combo box and label:
  hlay = new QHBoxLayout( vlay ); // inherites spacing
  mFontLocationCombo = new QComboBox( false, this );
  mFontLocationCombo->setEnabled( false ); // !mCustomFontCheck->isChecked()

  QStringList fontDescriptions;
  for ( int i = 0 ; i < numFontNames ; i++ )
    fontDescriptions << i18n( fontNames[i].displayName );
  mFontLocationCombo->insertStringList( fontDescriptions );

  label = new QLabel( mFontLocationCombo, i18n("Apply &to:"), this );
  label->setEnabled( false ); // since !mCustomFontCheck->isChecked()
  hlay->addWidget( label );

  hlay->addWidget( mFontLocationCombo );
  hlay->addStretch( 10 );
  vlay->addSpacing( KDialog::spacingHint() );
  mFontChooser = new KFontChooser( this, "font", false, QStringList(),
				   false, 4 );
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
  kdDebug(5006) << "slotFontSelectorChanged() called" << endl;
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

void AppearancePage::FontsTab::load() {
  KConfigGroup fonts( KMKernel::config(), "Fonts" );

  mFont[0] = KGlobalSettings::generalFont();
  QFont fixedFont = KGlobalSettings::fixedFont();
  for ( int i = 0 ; i < numFontNames ; i++ )
    mFont[i] = fonts.readFontEntry( fontNames[i].configName,
      (fontNames[i].onlyFixed) ? &fixedFont : &mFont[0] );

  mCustomFontCheck->setChecked( !fonts.readBoolEntry( "defaultFonts", true ) );
  mFontLocationCombo->setCurrentItem( 0 );
  slotFontSelectorChanged( 0 );
}

void AppearancePage::FontsTab::installProfile( KConfig * profile ) {
  KConfigGroup fonts( profile, "Fonts" );

  // read fonts that are defined in the profile:
  bool needChange = false;
  for ( int i = 0 ; i < numFontNames ; i++ )
    if ( fonts.hasKey( fontNames[i].configName ) ) {
      needChange = true;
      mFont[i] = fonts.readFontEntry( fontNames[i].configName );
      kdDebug(5006) << "got font \"" << fontNames[i].configName
		<< "\" thusly: \"" << mFont[i].toString() << "\"" << endl;
    }
  if ( needChange && mFontLocationCombo->currentItem() > 0 )
    mFontChooser->setFont( mFont[ mFontLocationCombo->currentItem() ],
      fontNames[ mFontLocationCombo->currentItem() ].onlyFixed );

  if ( fonts.hasKey( "defaultFonts" ) )
    mCustomFontCheck->setChecked( !fonts.readBoolEntry( "defaultFonts" ) );
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
  { "PGPMessageEncr", I18N_NOOP("OpenPGP Message - Encrypted") },
  { "PGPMessageOkKeyOk", I18N_NOOP("OpenPGP Message - Valid Signature with Trusted Key") },
  { "PGPMessageOkKeyBad", I18N_NOOP("OpenPGP Message - Valid Signature with Untrusted Key") },
  { "PGPMessageWarn", I18N_NOOP("OpenPGP Message - Unchecked Signature") },
  { "PGPMessageErr", I18N_NOOP("OpenPGP Message - Bad Signature") },
  { "HTMLWarningColor", I18N_NOOP("Border Around Warning Prepending HTML Messages") },
  { "ColorbarBackgroundPlain", I18N_NOOP("HTML Status Bar Background - No HTML Message") },
  { "ColorbarForegroundPlain", I18N_NOOP("HTML Status Bar Foreground - No HTML Message") },
  { "ColorbarBackgroundHTML",  I18N_NOOP("HTML Status Bar Background - HTML Message") },
  { "ColorbarForegroundHTML",  I18N_NOOP("HTML Status Bar Foreground - HTML Message") },
};
static const int numColorNames = sizeof colorNames / sizeof *colorNames;

AppearancePageColorsTab::AppearancePageColorsTab( QWidget * parent, const char * name )
  : ConfigModuleTab( parent, name )
{
  // tmp. vars:
  QVBoxLayout *vlay;

  // "use custom colors" check box
  vlay = new QVBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint() );
  mCustomColorCheck = new QCheckBox( i18n("&Use custom colors"), this );
  vlay->addWidget( mCustomColorCheck );
  connect( mCustomColorCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // color list box:
  mColorList = new ColorListBox( this );
  mColorList->setEnabled( false ); // since !mCustomColorCheck->isChecked()
  QStringList modeList;
  for ( int i = 0 ; i < numColorNames ; i++ )
    mColorList->insertItem( new ColorListItem( i18n( colorNames[i].displayName ) ) );
  vlay->addWidget( mColorList, 1 );

  // "recycle colors" check box:
  mRecycleColorCheck =
    new QCheckBox( i18n("Recycle colors on deep &quoting"), this );
  mRecycleColorCheck->setEnabled( false );
  vlay->addWidget( mRecycleColorCheck );
  connect( mRecycleColorCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // {en,dir}able widgets depending on the state of mCustomColorCheck:
  connect( mCustomColorCheck, SIGNAL(toggled(bool)),
	   mColorList, SLOT(setEnabled(bool)) );
  connect( mCustomColorCheck, SIGNAL(toggled(bool)),
	   mRecycleColorCheck, SLOT(setEnabled(bool)) );
  connect( mCustomColorCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
}

void AppearancePage::ColorsTab::load() {
  KConfigGroup reader( KMKernel::config(), "Reader" );

  mCustomColorCheck->setChecked( !reader.readBoolEntry( "defaultColors", true ) );
  mRecycleColorCheck->setChecked( reader.readBoolEntry( "RecycleQuoteColors", false ) );

  static const QColor defaultColor[ numColorNames ] = {
    kapp->palette().active().base(), // bg
    KGlobalSettings::alternateBackgroundColor(), // alt bg
    kapp->palette().active().text(), // fg
    QColor( 0x00, 0x80, 0x00 ), // quoted l1
    QColor( 0x00, 0x70, 0x00 ), // quoted l2
    QColor( 0x00, 0x60, 0x00 ), // quoted l3
    KGlobalSettings::linkColor(), // link
    KGlobalSettings::visitedLinkColor(), // visited link
    Qt::red, // misspelled words
    Qt::red, // new msg
    Qt::blue, // unread mgs
    QColor( 0x00, 0x7F, 0x00 ), // important msg
    QColor( 0x00, 0x80, 0xFF ), // light blue // pgp encrypted
    QColor( 0x40, 0xFF, 0x40 ), // light green // pgp ok, trusted key
    QColor( 0xFF, 0xFF, 0x40 ), // light yellow // pgp ok, untrusted key
    QColor( 0xFF, 0xFF, 0x40 ), // light yellow // pgp unchk
    Qt::red, // pgp bad
    QColor( 0xFF, 0x40, 0x40 ), // warning text color: light red
    Qt::lightGray, // colorbar plain bg
    Qt::black,     // colorbar plain fg
    Qt::black,     // colorbar html  bg
    Qt::white,     // colorbar html  fg
  };

  for ( int i = 0 ; i < numColorNames ; i++ )
    mColorList->setColor( i,
      reader.readColorEntry( colorNames[i].configName, &defaultColor[i] ) );
  connect( mColorList, SIGNAL( changed( ) ),
           this, SLOT( slotEmitChanged( void ) ) );
}

void AppearancePage::ColorsTab::installProfile( KConfig * profile ) {
  KConfigGroup reader( profile, "Reader" );

  if ( reader.hasKey( "defaultColors" ) )
    mCustomColorCheck->setChecked( !reader.readBoolEntry( "defaultColors" ) );
  if ( reader.hasKey( "RecycleQuoteColors" ) )
    mRecycleColorCheck->setChecked( reader.readBoolEntry( "RecycleQuoteColors" ) );

  for ( int i = 0 ; i < numColorNames ; i++ )
    if ( reader.hasKey( colorNames[i].configName ) )
      mColorList->setColor( i, reader.readColorEntry( colorNames[i].configName ) );
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

static const BoolConfigEntry showColorbarMode = {
  "Reader", "showColorbar", I18N_NOOP("Show HTML stat&us bar"), false
};

static const BoolConfigEntry showSpamStatusMode = {
  "Reader", "showSpamStatus", I18N_NOOP("Show S&pam status in fancy headers"), true
};

AppearancePageLayoutTab::AppearancePageLayoutTab( QWidget * parent, const char * name )
  : ConfigModuleTab( parent, name )
{
  // tmp. vars:
  QVBoxLayout * vlay;

  vlay = new QVBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint() );

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

  // "folder list" radio buttons:
  populateButtonGroup( mFolderListGroup = new QHButtonGroup( this ), folderListMode );
  vlay->addWidget( mFolderListGroup );
  connect( mFolderListGroup, SIGNAL ( clicked( int ) ),
           this, SLOT( slotEmitChanged() ) );

  // "show reader window" radio buttons:
  populateButtonGroup( mReaderWindowModeGroup = new QVButtonGroup( this ), readerWindowMode );
  vlay->addWidget( mReaderWindowModeGroup );
  connect( mReaderWindowModeGroup, SIGNAL ( clicked( int ) ),
           this, SLOT( slotEmitChanged() ) );

  // "Show MIME Tree" radio buttons:
  populateButtonGroup( mMIMETreeModeGroup = new QVButtonGroup( this ), mimeTreeMode );
  vlay->addWidget( mMIMETreeModeGroup );
  connect( mMIMETreeModeGroup, SIGNAL ( clicked( int ) ),
           this, SLOT( slotEmitChanged() ) );

  // "MIME Tree Location" radio buttons:
  populateButtonGroup( mMIMETreeLocationGroup = new QHButtonGroup( this ), mimeTreeLocation );
  vlay->addWidget( mMIMETreeLocationGroup );
  connect( mMIMETreeLocationGroup, SIGNAL ( clicked( int ) ),
           this, SLOT( slotEmitChanged() ) );

  vlay->addStretch( 10 ); // spacer
}

void AppearancePage::LayoutTab::load() {
  const KConfigGroup reader( KMKernel::config(), "Reader" );
  const KConfigGroup geometry( KMKernel::config(), "Geometry" );

  loadWidget( mShowColorbarCheck, reader, showColorbarMode );
  loadWidget( mShowSpamStatusCheck, reader, showSpamStatusMode );
  loadWidget( mFolderListGroup, geometry, folderListMode );
  loadWidget( mMIMETreeLocationGroup, reader, mimeTreeLocation );
  loadWidget( mMIMETreeModeGroup, reader, mimeTreeMode );
  loadWidget( mReaderWindowModeGroup, geometry, readerWindowMode );
}

void AppearancePage::LayoutTab::installProfile( KConfig * profile ) {
  const KConfigGroup reader( profile, "Reader" );
  const KConfigGroup geometry( profile, "Geometry" );

  loadProfile( mShowColorbarCheck, reader, showColorbarMode );
  loadProfile( mShowSpamStatusCheck, reader, showSpamStatusMode );
  loadProfile( mFolderListGroup, geometry, folderListMode );
  loadProfile( mMIMETreeLocationGroup, reader, mimeTreeLocation );
  loadProfile( mMIMETreeModeGroup, reader, mimeTreeMode );
  loadProfile( mReaderWindowModeGroup, geometry, readerWindowMode );
}

void AppearancePage::LayoutTab::save() {
  KConfigGroup reader( KMKernel::config(), "Reader" );
  KConfigGroup geometry( KMKernel::config(), "Geometry" );

  saveCheckBox( mShowColorbarCheck, reader, showColorbarMode );
  saveCheckBox( mShowSpamStatusCheck, reader, showSpamStatusMode );
  saveButtonGroup( mFolderListGroup, geometry, folderListMode );
  saveButtonGroup( mMIMETreeLocationGroup, reader, mimeTreeLocation );
  saveButtonGroup( mMIMETreeModeGroup, reader, mimeTreeMode );
  saveButtonGroup( mReaderWindowModeGroup, geometry, readerWindowMode );
}

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
  { I18N_NOOP("C&ustom format (Shift+F1 for help)"),
    KMime::DateFormatter::Custom }
};
static const int numDateDisplayConfig =
  sizeof dateDisplayConfig / sizeof *dateDisplayConfig;

AppearancePageHeadersTab::AppearancePageHeadersTab( QWidget * parent, const char * name )
  : ConfigModuleTab( parent, name ),
    mCustomDateFormatEdit( 0 )
{
  // tmp. vars:
  QButtonGroup * group;
  QRadioButton * radio;

  QVBoxLayout * vlay = new QVBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint() );

  // "General Options" group:
  group = new QVButtonGroup( i18n( "General Options" ), this );
  group->layout()->setSpacing( KDialog::spacingHint() );

  mMessageSizeCheck = new QCheckBox( i18n("Display messa&ge sizes"), group );

  mCryptoIconsCheck = new QCheckBox( i18n( "Show crypto &icons" ), group );

  mAttachmentCheck = new QCheckBox( i18n("Show attachment icon"), group );

  mNestedMessagesCheck =
    new QCheckBox( i18n("&Thread list of message headers"), group );

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
  mNestingPolicy =
    new QVButtonGroup( i18n("Message Header Threading Options"), this );
  mNestingPolicy->layout()->setSpacing( KDialog::spacingHint() );

  mNestingPolicy->insert(
    new QRadioButton( i18n("Always &keep threads open"),
		      mNestingPolicy ), 0 );
  mNestingPolicy->insert(
    new QRadioButton( i18n("Threads default to o&pen"),
		      mNestingPolicy ), 1 );
  mNestingPolicy->insert(
    new QRadioButton( i18n("Threads default to closed"),
		      mNestingPolicy ), 2 );
  mNestingPolicy->insert(
    new QRadioButton( i18n("Open threads that contain ne&w, unread "
			   "or important messages and open watched threads."),
                      mNestingPolicy ), 3 );

  vlay->addWidget( mNestingPolicy );

  connect( mNestingPolicy, SIGNAL( clicked( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // "Date Display" group:
  mDateDisplay = new QVButtonGroup( i18n("Date Display"), this );
  mDateDisplay->layout()->setSpacing( KDialog::spacingHint() );

  for ( int i = 0 ; i < numDateDisplayConfig ; i++ ) {
    QString buttonLabel = i18n(dateDisplayConfig[i].displayName);
    if ( buttonLabel.contains("%1") )
      buttonLabel = buttonLabel.arg( DateFormatter::formatCurrentDate( dateDisplayConfig[i].dateDisplay ) );
    radio = new QRadioButton( buttonLabel, mDateDisplay );
    mDateDisplay->insert( radio, i );
    if ( dateDisplayConfig[i].dateDisplay == DateFormatter::Custom ) {
      mCustomDateFormatEdit = new KLineEdit( mDateDisplay );
      mCustomDateFormatEdit->setEnabled( false );
      connect( radio, SIGNAL(toggled(bool)),
	       mCustomDateFormatEdit, SLOT(setEnabled(bool)) );
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
      QWhatsThis::add( mCustomDateFormatEdit, customDateWhatsThis );
      QWhatsThis::add( radio, customDateWhatsThis );
    }
  } // end for loop populating mDateDisplay

  vlay->addWidget( mDateDisplay );
  connect( mDateDisplay, SIGNAL( clicked( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );


  vlay->addStretch( 10 ); // spacer
}

void AppearancePage::HeadersTab::load() {
  KConfigGroup general( KMKernel::config(), "General" );
  KConfigGroup geometry( KMKernel::config(), "Geometry" );

  // "General Options":
  mNestedMessagesCheck->setChecked( geometry.readBoolEntry( "nestedMessages", false ) );
  mMessageSizeCheck->setChecked( general.readBoolEntry( "showMessageSize", false ) );
  mCryptoIconsCheck->setChecked( general.readBoolEntry( "showCryptoIcons", false ) );
  mAttachmentCheck->setChecked( general.readBoolEntry( "showAttachmentIcon", true ) );

  // "Message Header Threading Options":
  int num = geometry.readNumEntry( "nestingPolicy", 3 );
  if ( num < 0 || num > 3 ) num = 3;
  mNestingPolicy->setButton( num );

  // "Date Display":
  setDateDisplay( general.readNumEntry( "dateFormat", DateFormatter::Fancy ),
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
      mDateDisplay->setButton( i );
      return;
    }
  // fell through since none found:
  mDateDisplay->setButton( numDateDisplayConfig - 2 ); // default
}

void AppearancePage::HeadersTab::installProfile( KConfig * profile ) {
  KConfigGroup general( profile, "General" );
  KConfigGroup geometry( profile, "Geometry" );

  if ( geometry.hasKey( "nestedMessages" ) )
    mNestedMessagesCheck->setChecked( geometry.readBoolEntry( "nestedMessages" ) );
  if ( general.hasKey( "showMessageSize" ) )
    mMessageSizeCheck->setChecked( general.readBoolEntry( "showMessageSize" ) );

  if( general.hasKey( "showCryptoIcons" ) )
    mCryptoIconsCheck->setChecked( general.readBoolEntry( "showCryptoIcons" ) );
  if ( general.hasKey( "showAttachmentIcon" ) )
    mAttachmentCheck->setChecked( general.readBoolEntry( "showAttachmentIcon" ) );

  if ( geometry.hasKey( "nestingPolicy" ) ) {
    int num = geometry.readNumEntry( "nestingPolicy" );
    if ( num < 0 || num > 3 ) num = 3;
    mNestingPolicy->setButton( num );
  }

  if ( general.hasKey( "dateFormat" ) )
    setDateDisplay( general.readNumEntry( "dateFormat" ),
		   general.readEntry( "customDateFormat" ) );
}

void AppearancePage::HeadersTab::save() {
  KConfigGroup general( KMKernel::config(), "General" );
  KConfigGroup geometry( KMKernel::config(), "Geometry" );

  if ( geometry.readBoolEntry( "nestedMessages", false )
       != mNestedMessagesCheck->isChecked() ) {
    int result = KMessageBox::warningContinueCancel( this,
		   i18n("Changing the global threading setting will override "
			"all folder specific values."),
		   QString::null, QString::null, "threadOverride" );
    if ( result == KMessageBox::Continue ) {
      geometry.writeEntry( "nestedMessages", mNestedMessagesCheck->isChecked() );
      // remove all threadMessagesOverride keys from all [Folder-*] groups:
      QStringList groups = KMKernel::config()->groupList().grep( QRegExp("^Folder-") );
      kdDebug(5006) << "groups.count() == " << groups.count() << endl;
      for ( QStringList::const_iterator it = groups.begin() ; it != groups.end() ; ++it ) {
	KConfigGroup group( KMKernel::config(), *it );
	group.deleteEntry( "threadMessagesOverride" );
      }
    }
  }

  geometry.writeEntry( "nestingPolicy",
		       mNestingPolicy->id( mNestingPolicy->selected() ) );
  general.writeEntry( "showMessageSize", mMessageSizeCheck->isChecked() );
  general.writeEntry( "showCryptoIcons", mCryptoIconsCheck->isChecked() );
  general.writeEntry( "showAttachmentIcon", mAttachmentCheck->isChecked() );

  int dateDisplayID = mDateDisplay->id( mDateDisplay->selected() );
  // check bounds:
  assert( dateDisplayID >= 0 ); assert( dateDisplayID < numDateDisplayConfig );
  general.writeEntry( "dateFormat",
		      dateDisplayConfig[ dateDisplayID ].dateDisplay );
  general.writeEntry( "customDateFormat", mCustomDateFormatEdit->text() );
}


QString AppearancePage::SystemTrayTab::helpAnchor() const {
  return QString::fromLatin1("configure-appearance-systemtray");
}

AppearancePageSystemTrayTab::AppearancePageSystemTrayTab( QWidget * parent,
                                                          const char * name )
  : ConfigModuleTab( parent, name )
{
  QVBoxLayout * vlay = new QVBoxLayout( this, KDialog::marginHint(),
                                        KDialog::spacingHint() );

  // "Enable system tray applet" check box
  mSystemTrayCheck = new QCheckBox( i18n("Enable system tray icon"), this );
  vlay->addWidget( mSystemTrayCheck );
  connect( mSystemTrayCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // System tray modes
  mSystemTrayGroup = new QVButtonGroup( i18n("System Tray Mode"), this );
  mSystemTrayGroup->layout()->setSpacing( KDialog::spacingHint() );
  vlay->addWidget( mSystemTrayGroup );
  connect( mSystemTrayGroup, SIGNAL( clicked( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  connect( mSystemTrayCheck, SIGNAL( toggled( bool ) ),
           mSystemTrayGroup, SLOT( setEnabled( bool ) ) );

  mSystemTrayGroup->insert( new QRadioButton( i18n("Always show KMail in system tray"), mSystemTrayGroup ),
                            GlobalSettings::EnumSystemTrayPolicy::ShowAlways );

  mSystemTrayGroup->insert( new QRadioButton( i18n("Only show KMail in system tray if there are unread messages"), mSystemTrayGroup ),
                            GlobalSettings::EnumSystemTrayPolicy::ShowOnUnread );

  vlay->addStretch( 10 ); // spacer
}

void AppearancePage::SystemTrayTab::load() {
  mSystemTrayCheck->setChecked( GlobalSettings::systemTrayEnabled() );
  mSystemTrayGroup->setButton( GlobalSettings::systemTrayPolicy() );
  mSystemTrayGroup->setEnabled( mSystemTrayCheck->isChecked() );
}

void AppearancePage::SystemTrayTab::installProfile( KConfig * profile ) {
  KConfigGroup general( profile, "General" );

  if ( general.hasKey( "SystemTrayEnabled" ) ) {
    mSystemTrayCheck->setChecked( general.readBoolEntry( "SystemTrayEnabled" ) );
  }
  if ( general.hasKey( "SystemTrayPolicy" ) ) {
    mSystemTrayGroup->setButton( general.readNumEntry( "SystemTrayPolicy" ) );
  }
  mSystemTrayGroup->setEnabled( mSystemTrayCheck->isChecked() );
}

void AppearancePage::SystemTrayTab::save() {
  GlobalSettings::setSystemTrayEnabled( mSystemTrayCheck->isChecked() );
  GlobalSettings::setSystemTrayPolicy( mSystemTrayGroup->id( mSystemTrayGroup->selected() ) );
}


// *************************************************************
// *                                                           *
// *                      ComposerPage                         *
// *                                                           *
// *************************************************************

QString ComposerPage::helpAnchor() const {
  return QString::fromLatin1("configure-composer");
}

ComposerPage::ComposerPage( QWidget * parent, const char * name )
  : ConfigModuleWithTabs( parent, name )
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
  mPhrasesTab = new PhrasesTab();
  addTab( mPhrasesTab, i18n("&Phrases") );

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
  addTab( mAttachmentsTab, i18n("Config->Composer->Attachments", "A&ttachments") );
  load();
}

QString ComposerPage::GeneralTab::helpAnchor() const {
  return QString::fromLatin1("configure-composer-general");
}

ComposerPageGeneralTab::ComposerPageGeneralTab( QWidget * parent, const char * name )
  : ConfigModuleTab( parent, name )
{
  // tmp. vars:
  QVBoxLayout *vlay;
  QHBoxLayout *hlay;
  QGroupBox   *group;
  QLabel      *label;
  QHBox       *hbox;
  QString      msg;

  vlay = new QVBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint() );

  // some check buttons...
  mAutoAppSignFileCheck = new QCheckBox(
           GlobalSettings::self()->autoTextSignatureItem()->label(),
           this );
  vlay->addWidget( mAutoAppSignFileCheck );
  connect( mAutoAppSignFileCheck, SIGNAL( stateChanged(int) ),
           this, SLOT( slotEmitChanged( void ) ) );

  mSmartQuoteCheck = new QCheckBox(
           GlobalSettings::self()->smartQuoteItem()->label(),
           this, "kcfg_SmartQuote" );
  vlay->addWidget( mSmartQuoteCheck );
  connect( mSmartQuoteCheck, SIGNAL( stateChanged(int) ),
           this, SLOT( slotEmitChanged( void ) ) );

  mAutoRequestMDNCheck = new QCheckBox(
           GlobalSettings::self()->requestMDNItem()->label(),
           this, "kcfg_RequestMDN" );
  vlay->addWidget( mAutoRequestMDNCheck );
  connect( mAutoRequestMDNCheck, SIGNAL( stateChanged(int) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // a checkbox for "word wrap" and a spinbox for the column in
  // which to wrap:
  hlay = new QHBoxLayout( vlay ); // inherits spacing
  mWordWrapCheck = new QCheckBox(
           GlobalSettings::self()->wordWrapItem()->label(),
           this, "kcfg_WordWrap" );
  hlay->addWidget( mWordWrapCheck );
  connect( mWordWrapCheck, SIGNAL( stateChanged(int) ),
           this, SLOT( slotEmitChanged( void ) ) );

  mWrapColumnSpin = new KIntSpinBox( 30/*min*/, 78/*max*/, 1/*step*/,
           78/*init*/, 10 /*base*/, this, "kcfg_LineWrapWidth" );
  mWrapColumnSpin->setEnabled( false ); // since !mWordWrapCheck->isChecked()
  connect( mWrapColumnSpin, SIGNAL( valueChanged(int) ),
           this, SLOT( slotEmitChanged( void ) ) );

  hlay->addWidget( mWrapColumnSpin );
  hlay->addStretch( 1 );
  // only enable the spinbox if the checkbox is checked:
  connect( mWordWrapCheck, SIGNAL(toggled(bool)),
	   mWrapColumnSpin, SLOT(setEnabled(bool)) );

  hlay = new QHBoxLayout( vlay ); // inherits spacing
  mAutoSave = new KIntSpinBox( 0, 60, 1, 1, 10, this, "kcfg_AutosaveInterval" );
  label = new QLabel( mAutoSave,
           GlobalSettings::self()->autosaveIntervalItem()->label(), this );
  hlay->addWidget( label );
  hlay->addWidget( mAutoSave );
  mAutoSave->setSpecialValueText( i18n("No autosave") );
  mAutoSave->setSuffix( i18n(" min") );
  hlay->addStretch( 1 );
  connect( mAutoSave, SIGNAL( valueChanged(int) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // The "external editor" group:
  group = new QVGroupBox( i18n("External Editor"), this );
  group->layout()->setSpacing( KDialog::spacingHint() );

  mExternalEditorCheck = new QCheckBox(
           GlobalSettings::self()->useExternalEditorItem()->label(),
           group, "kcfg_UseExternalEditor" );
  connect( mExternalEditorCheck, SIGNAL( toggled( bool ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  hbox = new QHBox( group );
  label = new QLabel( GlobalSettings::self()->externalEditorItem()->label(),
                   hbox );
  mEditorRequester = new KURLRequester( hbox, "kcfg_ExternalEditor" );
  connect( mEditorRequester, SIGNAL( urlSelected(const QString&) ),
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

  vlay->addWidget( group );
  vlay->addStretch( 100 );
}

void ComposerPage::GeneralTab::load() {
  // various check boxes:

  mAutoAppSignFileCheck->setChecked(
           GlobalSettings::autoTextSignature()=="auto" );
  mSmartQuoteCheck->setChecked( GlobalSettings::smartQuote() );
  mAutoRequestMDNCheck->setChecked( GlobalSettings::requestMDN() );
  mWordWrapCheck->setChecked( GlobalSettings::wordWrap() );

  mWrapColumnSpin->setValue( GlobalSettings::lineWrapWidth() );
  mAutoSave->setValue( GlobalSettings::autosaveInterval() );

  // editor group:
  mExternalEditorCheck->setChecked( GlobalSettings::useExternalEditor() );
  mEditorRequester->setURL( GlobalSettings::externalEditor() );
}

void ComposerPageGeneralTab::defaults()
{
  mAutoAppSignFileCheck->setChecked( true );
}

void ComposerPage::GeneralTab::installProfile( KConfig * profile ) {
  KConfigGroup composer( profile, "Composer" );
  KConfigGroup general( profile, "General" );

  if ( composer.hasKey( "signature" ) ) {
    bool state = composer.readBoolEntry("signature");
    mAutoAppSignFileCheck->setChecked( state );
  }
  if ( composer.hasKey( "smart-quote" ) )
    mSmartQuoteCheck->setChecked( composer.readBoolEntry( "smart-quote" ) );
  if ( composer.hasKey( "request-mdn" ) )
    mAutoRequestMDNCheck->setChecked( composer.readBoolEntry( "request-mdn" ) );
  if ( composer.hasKey( "word-wrap" ) )
    mWordWrapCheck->setChecked( composer.readBoolEntry( "word-wrap" ) );
  if ( composer.hasKey( "break-at" ) )
    mWrapColumnSpin->setValue( composer.readNumEntry( "break-at" ) );
  if ( composer.hasKey( "autosave" ) )
    mAutoSave->setValue( composer.readNumEntry( "autosave" ) );

  if ( general.hasKey( "use-external-editor" )
       && general.hasKey( "external-editor" ) ) {
    mExternalEditorCheck->setChecked( general.readBoolEntry( "use-external-editor" ) );
    mEditorRequester->setURL( general.readPathEntry( "external-editor" ) );
  }
}

void ComposerPage::GeneralTab::save() {
  GlobalSettings::setAutoTextSignature(
         mAutoAppSignFileCheck->isChecked() ? "auto" : "manual" );
}

QString ComposerPage::PhrasesTab::helpAnchor() const {
  return QString::fromLatin1("configure-composer-phrases");
}

ComposerPagePhrasesTab::ComposerPagePhrasesTab( QWidget * parent, const char * name )
  : ConfigModuleTab( parent, name )
{
  // tmp. vars:
  QGridLayout *glay;
  QPushButton *button;

  glay = new QGridLayout( this, 7, 3, KDialog::spacingHint() );
  glay->setMargin( KDialog::marginHint() );
  glay->setColStretch( 1, 1 );
  glay->setColStretch( 2, 1 );
  glay->setRowStretch( 7, 1 );

  // row 0: help text
  glay->addMultiCellWidget( new QLabel( i18n("<qt>The following placeholders are "
					     "supported in the reply phrases:<br>"
					     "<b>%D</b>: date, <b>%S</b>: subject,<br>"
                                             "<b>%e</b>: sender's address, <b>%F</b>: sender's name, <b>%f</b>: sender's initials,<br>"
                                             "<b>%T</b>: recipient's name, <b>%t</b>: recipient's name and address,<br>"
                                             "<b>%C</b>: carbon copy names, <b>%c</b>: carbon copy names and addresses,<br>"
					     "<b>%%</b>: percent sign, <b>%_</b>: space, "
					     "<b>%L</b>: linebreak</qt>"), this ),
			    0, 0, 0, 2 ); // row 0; cols 0..2

  // row 1: label and language combo box:
  mPhraseLanguageCombo = new LanguageComboBox( false, this );
  glay->addWidget( new QLabel( mPhraseLanguageCombo,
			       i18n("Lang&uage:"), this ), 1, 0 );
  glay->addMultiCellWidget( mPhraseLanguageCombo, 1, 1, 1, 2 );
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
  glay->addWidget( new QLabel( mPhraseReplyEdit,
			       i18n("Reply to se&nder:"), this ), 3, 0 );
  glay->addMultiCellWidget( mPhraseReplyEdit, 3, 3, 1, 2 ); // cols 1..2

  // row 4: "reply to all" line edit and label:
  mPhraseReplyAllEdit = new KLineEdit( this );
  connect( mPhraseReplyAllEdit, SIGNAL( textChanged( const QString& ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  glay->addWidget( new QLabel( mPhraseReplyAllEdit,
			       i18n("Repl&y to all:"), this ), 4, 0 );
  glay->addMultiCellWidget( mPhraseReplyAllEdit, 4, 4, 1, 2 ); // cols 1..2

  // row 5: "forward" line edit and label:
  mPhraseForwardEdit = new KLineEdit( this );
  connect( mPhraseForwardEdit, SIGNAL( textChanged( const QString& ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  glay->addWidget( new QLabel( mPhraseForwardEdit,
			       i18n("&Forward:"), this ), 5, 0 );
  glay->addMultiCellWidget( mPhraseForwardEdit, 5, 5, 1, 2 ); // cols 1..2

  // row 6: "quote indicator" line edit and label:
  mPhraseIndentPrefixEdit = new KLineEdit( this );
  connect( mPhraseIndentPrefixEdit, SIGNAL( textChanged( const QString& ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  glay->addWidget( new QLabel( mPhraseIndentPrefixEdit,
			       i18n("&Quote indicator:"), this ), 6, 0 );
  glay->addMultiCellWidget( mPhraseIndentPrefixEdit, 6, 6, 1, 2 );

  // row 7: spacer
}


void ComposerPage::PhrasesTab::setLanguageItemInformation( int index ) {
  assert( 0 <= index && index < (int)mLanguageList.count() );

  LanguageItem &l = *mLanguageList.at( index );

  mPhraseReplyEdit->setText( l.mReply );
  mPhraseReplyAllEdit->setText( l.mReplyAll );
  mPhraseForwardEdit->setText( l.mForward );
  mPhraseIndentPrefixEdit->setText( l.mIndentPrefix );
}

void ComposerPage::PhrasesTab::saveActiveLanguageItem() {
  int index = mActiveLanguageItem;
  if (index == -1) return;
  assert( 0 <= index && index < (int)mLanguageList.count() );

  LanguageItem &l = *mLanguageList.at( index );

  l.mReply = mPhraseReplyEdit->text();
  l.mReplyAll = mPhraseReplyAllEdit->text();
  l.mForward = mPhraseForwardEdit->text();
  l.mIndentPrefix = mPhraseIndentPrefixEdit->text();
}

void ComposerPage::PhrasesTab::slotNewLanguage()
{
  NewLanguageDialog dialog( mLanguageList, parentWidget(), "New", true );
  if ( dialog.exec() == QDialog::Accepted ) slotAddNewLanguage( dialog.language() );
}

void ComposerPage::PhrasesTab::slotAddNewLanguage( const QString& lang )
{
  mPhraseLanguageCombo->setCurrentItem(
    mPhraseLanguageCombo->insertLanguage( lang ) );
  KLocale locale("kmail");
  locale.setLanguage( lang );
  mLanguageList.append(
     LanguageItem( lang,
		   locale.translate("On %D, you wrote:"),
		   locale.translate("On %D, %F wrote:"),
		   locale.translate("Forwarded Message"),
		   locale.translate(">%_") ) );
  mRemoveButton->setEnabled( true );
  slotLanguageChanged( QString::null );
}

void ComposerPage::PhrasesTab::slotRemoveLanguage()
{
  assert( mPhraseLanguageCombo->count() > 1 );
  int index = mPhraseLanguageCombo->currentItem();
  assert( 0 <= index && index < (int)mLanguageList.count() );

  // remove current item from internal list and combobox:
  mLanguageList.remove( mLanguageList.at( index ) );
  mPhraseLanguageCombo->removeItem( index );

  if ( index >= (int)mLanguageList.count() ) index--;

  mActiveLanguageItem = index;
  setLanguageItemInformation( index );
  mRemoveButton->setEnabled( mLanguageList.count() > 1 );
  emit changed( true );
}

void ComposerPage::PhrasesTab::slotLanguageChanged( const QString& )
{
  int index = mPhraseLanguageCombo->currentItem();
  assert( index < (int)mLanguageList.count() );
  saveActiveLanguageItem();
  mActiveLanguageItem = index;
  setLanguageItemInformation( index );
  emit changed( true );
}


void ComposerPage::PhrasesTab::load() {
  mLanguageList.clear();
  mPhraseLanguageCombo->clear();
  mActiveLanguageItem = -1;

  int numLang = GlobalSettings::replyLanguagesCount();
  int currentNr = GlobalSettings::replyCurrentLanguage();

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

  mPhraseLanguageCombo->setCurrentItem( currentNr );
  mActiveLanguageItem = currentNr;
  setLanguageItemInformation( currentNr );
  mRemoveButton->setEnabled( mLanguageList.count() > 1 );
}

void ComposerPage::PhrasesTab::save() {
  GlobalSettings::setReplyLanguagesCount( mLanguageList.count() );
  GlobalSettings::setReplyCurrentLanguage( mPhraseLanguageCombo->currentItem() );

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

QString ComposerPage::SubjectTab::helpAnchor() const {
  return QString::fromLatin1("configure-composer-subject");
}

ComposerPageSubjectTab::ComposerPageSubjectTab( QWidget * parent, const char * name )
  : ConfigModuleTab( parent, name )
{
  // tmp. vars:
  QVBoxLayout *vlay;
  QGroupBox   *group;
  QLabel      *label;


  vlay = new QVBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint() );

  group = new QVGroupBox( i18n("Repl&y Subject Prefixes"), this );
  group->layout()->setSpacing( KDialog::spacingHint() );

  // row 0: help text:
  label = new QLabel( i18n("Recognize any sequence of the following prefixes\n"
			   "(entries are case-insensitive regular expressions):"), group );
  label->setAlignment( AlignLeft|WordBreak );

  // row 1, string list editor:
  SimpleStringListEditor::ButtonCode buttonCode =
    static_cast<SimpleStringListEditor::ButtonCode>( SimpleStringListEditor::Add | SimpleStringListEditor::Remove | SimpleStringListEditor::Modify );
  mReplyListEditor =
    new SimpleStringListEditor( group, 0, buttonCode,
				i18n("A&dd..."), i18n("Re&move"),
				i18n("Mod&ify..."),
				i18n("Enter new reply prefix:") );
  connect( mReplyListEditor, SIGNAL( changed( void ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // row 2: "replace [...]" check box:
  mReplaceReplyPrefixCheck = new QCheckBox(
     GlobalSettings::self()->replaceReplyPrefixItem()->label(),
     group, "kcfg_ReplaceReplyPrefix" );
  connect( mReplaceReplyPrefixCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  vlay->addWidget( group );


  group = new QVGroupBox( i18n("For&ward Subject Prefixes"), this );
  group->layout()->setSpacing( KDialog::marginHint() );

  // row 0: help text:
  label= new QLabel( i18n("Recognize any sequence of the following prefixes\n"
			  "(entries are case-insensitive regular expressions):"), group );
  label->setAlignment( AlignLeft|WordBreak );

  // row 1: string list editor
  mForwardListEditor =
    new SimpleStringListEditor( group, 0, buttonCode,
				i18n("Add..."),
				i18n("Remo&ve"),
                                i18n("Modify..."),
				i18n("Enter new forward prefix:") );
  connect( mForwardListEditor, SIGNAL( changed( void ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // row 3: "replace [...]" check box:
  mReplaceForwardPrefixCheck = new QCheckBox(
       GlobalSettings::self()->replaceForwardPrefixItem()->label(),
       group, "kcfg_ReplaceForwardPrefix" );
  connect( mReplaceForwardPrefixCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  vlay->addWidget( group );
}

void ComposerPage::SubjectTab::load() {
  mReplyListEditor->setStringList( GlobalSettings::replyPrefixes() );
  mReplaceReplyPrefixCheck->setChecked( GlobalSettings::replaceReplyPrefix() );
  mForwardListEditor->setStringList( GlobalSettings::forwardPrefixes() );
  mReplaceForwardPrefixCheck->setChecked( GlobalSettings::replaceForwardPrefix() );
}

void ComposerPage::SubjectTab::save() {
  GlobalSettings::setReplyPrefixes( mReplyListEditor->stringList() );
  GlobalSettings::setForwardPrefixes( mForwardListEditor->stringList() );
}

QString ComposerPage::CharsetTab::helpAnchor() const {
  return QString::fromLatin1("configure-composer-charset");
}

ComposerPageCharsetTab::ComposerPageCharsetTab( QWidget * parent, const char * name )
  : ConfigModuleTab( parent, name )
{
  // tmp. vars:
  QVBoxLayout *vlay;
  QLabel      *label;

  vlay = new QVBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint() );

  label = new QLabel( i18n("This list is checked for every outgoing message "
                           "from the top to the bottom for a charset that "
                           "contains all required characters."), this );
  label->setAlignment( WordBreak);
  vlay->addWidget( label );

  mCharsetListEditor =
    new SimpleStringListEditor( this, 0, SimpleStringListEditor::All,
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
  if ( charset.lower() == QString::fromLatin1("us-ascii") ) {
    charset = QString::fromLatin1("us-ascii");
    return;
  }

  if ( charset.lower() == QString::fromLatin1("locale") ) {
    charset =  QString::fromLatin1("%1 (locale)")
      .arg( QCString( kmkernel->networkCodec()->mimeName() ).lower() );
    return;
  }

  bool ok = false;
  QTextCodec *codec = KGlobal::charsets()->codecForName( charset, ok );
  if ( ok && codec ) {
    charset = QString::fromLatin1( codec->mimeName() ).lower();
    return;
  }

  KMessageBox::sorry( this, i18n("This charset is not supported.") );
  charset = QString::null;
}

void ComposerPage::CharsetTab::load() {
  KConfigGroup composer( KMKernel::config(), "Composer" );

  QStringList charsets = composer.readListEntry( "pref-charsets" );
  for ( QStringList::Iterator it = charsets.begin() ;
	it != charsets.end() ; ++it )
      if ( (*it) == QString::fromLatin1("locale") )
	(*it) = QString("%1 (locale)")
	  .arg( QCString( kmkernel->networkCodec()->mimeName() ).lower() );

  mCharsetListEditor->setStringList( charsets );
  mKeepReplyCharsetCheck->setChecked( !composer.readBoolEntry( "force-reply-charset", false ) );
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

ComposerPageHeadersTab::ComposerPageHeadersTab( QWidget * parent, const char * name )
  : ConfigModuleTab( parent, name )
{
  // tmp. vars:
  QVBoxLayout *vlay;
  QHBoxLayout *hlay;
  QGridLayout *glay;
  QLabel      *label;
  QPushButton *button;

  vlay = new QVBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint() );

  // "Use custom Message-Id suffix" checkbox:
  mCreateOwnMessageIdCheck =
    new QCheckBox( i18n("&Use custom message-id suffix"), this );
  connect( mCreateOwnMessageIdCheck, SIGNAL ( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  vlay->addWidget( mCreateOwnMessageIdCheck );

  // "Message-Id suffix" line edit and label:
  hlay = new QHBoxLayout( vlay ); // inherits spacing
  mMessageIdSuffixEdit = new KLineEdit( this );
  // only ASCII letters, digits, plus, minus and dots are allowed
  mMessageIdSuffixValidator =
    new QRegExpValidator( QRegExp( "[a-zA-Z0-9+-]+(?:\\.[a-zA-Z0-9+-]+)*" ), this );
  mMessageIdSuffixEdit->setValidator( mMessageIdSuffixValidator );
  label = new QLabel( mMessageIdSuffixEdit,
		      i18n("Custom message-&id suffix:"), this );
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
  vlay->addWidget( new KSeparator( KSeparator::HLine, this ) );
  vlay->addWidget( new QLabel( i18n("Define custom mime header fields:"), this) );

  // "custom header fields" listbox:
  glay = new QGridLayout( vlay, 5, 3 ); // inherits spacing
  glay->setRowStretch( 2, 1 );
  glay->setColStretch( 1, 1 );
  mTagList = new ListView( this, "tagList" );
  mTagList->addColumn( i18n("Name") );
  mTagList->addColumn( i18n("Value") );
  mTagList->setAllColumnsShowFocus( true );
  mTagList->setFrameStyle( QFrame::WinPanel | QFrame::Sunken );
  mTagList->setSorting( -1 );
  connect( mTagList, SIGNAL(selectionChanged()),
	   this, SLOT(slotMimeHeaderSelectionChanged()) );
  glay->addMultiCellWidget( mTagList, 0, 2, 0, 1 );

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
  mTagNameLabel = new QLabel( mTagNameEdit, i18n("&Name:"), this );
  mTagNameLabel->setEnabled( false );
  glay->addWidget( mTagNameLabel, 3, 0 );
  glay->addWidget( mTagNameEdit, 3, 1 );
  connect( mTagNameEdit, SIGNAL(textChanged(const QString&)),
	   this, SLOT(slotMimeHeaderNameChanged(const QString&)) );

  mTagValueEdit = new KLineEdit( this );
  mTagValueEdit->setEnabled( false );
  mTagValueLabel = new QLabel( mTagValueEdit, i18n("&Value:"), this );
  mTagValueLabel->setEnabled( false );
  glay->addWidget( mTagValueLabel, 4, 0 );
  glay->addWidget( mTagValueEdit, 4, 1 );
  connect( mTagValueEdit, SIGNAL(textChanged(const QString&)),
	   this, SLOT(slotMimeHeaderValueChanged(const QString&)) );
}

void ComposerPage::HeadersTab::slotMimeHeaderSelectionChanged()
{
  QListViewItem * item = mTagList->selectedItem();

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
  QListViewItem * item = mTagList->selectedItem();
  if ( item )
    item->setText( 0, text );
  emit changed( true );
}


void ComposerPage::HeadersTab::slotMimeHeaderValueChanged( const QString & text ) {
  // is called on ::setup(), when clearing the line edits. So be
  // prepared to not find a selection:
  QListViewItem * item = mTagList->selectedItem();
  if ( item )
    item->setText( 1, text );
  emit changed( true );
}


void ComposerPage::HeadersTab::slotNewMimeHeader()
{
  QListViewItem *listItem = new QListViewItem( mTagList );
  mTagList->setCurrentItem( listItem );
  mTagList->setSelected( listItem, true );
  emit changed( true );
}


void ComposerPage::HeadersTab::slotRemoveMimeHeader()
{
  // calling this w/o selection is a programming error:
  QListViewItem * item = mTagList->selectedItem();
  if ( !item ) {
    kdDebug(5006) << "==================================================\n"
                  << "Error: Remove button was pressed although no custom header was selected\n"
                  << "==================================================\n";
    return;
  }

  QListViewItem * below = item->nextSibling();
  delete item;

  if ( below )
    mTagList->setSelected( below, true );
  else if ( mTagList->lastItem() )
    mTagList->setSelected( mTagList->lastItem(), true );
  emit changed( true );
}

void ComposerPage::HeadersTab::load() {
  KConfigGroup general( KMKernel::config(), "General" );

  QString suffix = general.readEntry( "myMessageIdSuffix" );
  mMessageIdSuffixEdit->setText( suffix );
  bool state = ( !suffix.isEmpty() &&
	    general.readBoolEntry( "useCustomMessageIdSuffix", false ) );
  mCreateOwnMessageIdCheck->setChecked( state );

  mTagList->clear();
  mTagNameEdit->clear();
  mTagValueEdit->clear();

  QListViewItem * item = 0;

  int count = general.readNumEntry( "mime-header-count", 0 );
  for( int i = 0 ; i < count ; i++ ) {
    KConfigGroup config( KMKernel::config(),
			 QCString("Mime #") + QCString().setNum(i) );
    QString name  = config.readEntry( "name" );
    QString value = config.readEntry( "value" );
    if( !name.isEmpty() )
      item = new QListViewItem( mTagList, item, name, value );
  }
  if ( mTagList->childCount() ) {
    mTagList->setCurrentItem( mTagList->firstChild() );
    mTagList->setSelected( mTagList->firstChild(), true );
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
  QListViewItem * item = mTagList->firstChild();
  for ( ; item ; item = item->itemBelow() )
    if( !item->text(0).isEmpty() ) {
      KConfigGroup config( KMKernel::config(), QCString("Mime #")
			     + QCString().setNum( numValidEntries ) );
      config.writeEntry( "name",  item->text( 0 ) );
      config.writeEntry( "value", item->text( 1 ) );
      numValidEntries++;
    }
  general.writeEntry( "mime-header-count", numValidEntries );
}

QString ComposerPage::AttachmentsTab::helpAnchor() const {
  return QString::fromLatin1("configure-composer-attachments");
}

ComposerPageAttachmentsTab::ComposerPageAttachmentsTab( QWidget * parent,
                                                        const char * name )
  : ConfigModuleTab( parent, name ) {
  // tmp. vars:
  QVBoxLayout *vlay;
  QLabel      *label;

  vlay = new QVBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint() );

  // "Outlook compatible attachment naming" check box
  mOutlookCompatibleCheck =
    new QCheckBox( i18n( "Outlook-compatible attachment naming" ), this );
  mOutlookCompatibleCheck->setChecked( false );
  QToolTip::add( mOutlookCompatibleCheck, i18n(
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
  label->setAlignment( AlignLeft|WordBreak );
  vlay->addWidget( label );

  SimpleStringListEditor::ButtonCode buttonCode =
    static_cast<SimpleStringListEditor::ButtonCode>( SimpleStringListEditor::Add | SimpleStringListEditor::Remove | SimpleStringListEditor::Modify );
  mAttachWordsListEditor =
    new SimpleStringListEditor( this, 0, buttonCode,
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

void ComposerPage::AttachmentsTab::load() {
  KConfigGroup composer( KMKernel::config(), "Composer" );

  mOutlookCompatibleCheck->setChecked(
    composer.readBoolEntry( "outlook-compatible-attachments", false ) );
  mMissingAttachmentDetectionCheck->setChecked(
    composer.readBoolEntry( "showForgottenAttachmentWarning", true ) );
  QStringList attachWordsList =
    composer.readListEntry( "attachment-keywords" );
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
  KConfigGroup composer( KMKernel::config(), "Composer" );
  composer.writeEntry( "outlook-compatible-attachments",
                       mOutlookCompatibleCheck->isChecked() );
  composer.writeEntry( "showForgottenAttachmentWarning",
                       mMissingAttachmentDetectionCheck->isChecked() );
  composer.writeEntry( "attachment-keywords",
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

SecurityPage::SecurityPage( QWidget * parent, const char * name )
  : ConfigModuleWithTabs( parent, name )
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

SecurityPageGeneralTab::SecurityPageGeneralTab( QWidget * parent, const char * name )
  : ConfigModuleTab ( parent, name )
{
  // tmp. vars:
  QVBoxLayout  *vlay;
  QHBox        *hbox;
  QGroupBox    *group;
  QRadioButton *radio;
  KActiveLabel *label;
  QWidget      *w;
  QString       msg;

  vlay = new QVBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint() );

  // QWhat'sThis texts
  QString htmlWhatsThis = i18n( "<qt><p>Messages sometimes come in both formats. "
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

  QString externalWhatsThis = i18n( "<qt><p>Some mail advertisements are in HTML "
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

  QString receiptWhatsThis = i18n( "<qt><h3>Message Disposition "
              "Notification Policy</h3>"
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
  group = new QVGroupBox( i18n( "HTML Messages" ), this );
  group->layout()->setSpacing( KDialog::spacingHint() );

  mHtmlMailCheck = new QCheckBox( i18n("Prefer H&TML to plain text"), group );
  QWhatsThis::add( mHtmlMailCheck, htmlWhatsThis );
  connect( mHtmlMailCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  mExternalReferences = new QCheckBox( i18n("Allow messages to load e&xternal "
					    "references from the Internet" ), group );
  QWhatsThis::add( mExternalReferences, externalWhatsThis );
  connect( mExternalReferences, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  label = new KActiveLabel( i18n("<b>WARNING:</b> Allowing HTML in email may "
			   "increase the risk that your system will be "
			   "compromised by present and anticipated security "
			   "exploits. <a href=\"whatsthis:%1\">More about "
			   "HTML mails...</a> <a href=\"whatsthis:%2\">More "
			   "about external references...</a>")
			   .arg(htmlWhatsThis).arg(externalWhatsThis),
			   group );

  vlay->addWidget( group );

  // "Message Disposition Notification" groupbox:
  group = new QVGroupBox( i18n("Message Disposition Notifications"), this );
  group->layout()->setSpacing( KDialog::spacingHint() );


  // "ignore", "ask", "deny", "always send" radiobutton line:
  mMDNGroup = new QButtonGroup( group );
  mMDNGroup->hide();
  connect( mMDNGroup, SIGNAL( clicked( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  hbox = new QHBox( group );
  hbox->setSpacing( KDialog::spacingHint() );

  (void)new QLabel( i18n("Send policy:"), hbox );

  radio = new QRadioButton( i18n("&Ignore"), hbox );
  mMDNGroup->insert( radio );

  radio = new QRadioButton( i18n("As&k"), hbox );
  mMDNGroup->insert( radio );

  radio = new QRadioButton( i18n("&Deny"), hbox );
  mMDNGroup->insert( radio );

  radio = new QRadioButton( i18n("Al&ways send"), hbox );
  mMDNGroup->insert( radio );

  for ( int i = 0 ; i < mMDNGroup->count() ; ++i )
      QWhatsThis::add( mMDNGroup->find( i ), receiptWhatsThis );

  w = new QWidget( hbox ); // spacer
  hbox->setStretchFactor( w, 1 );

  // "Original Message quote" radiobutton line:
  mOrigQuoteGroup = new QButtonGroup( group );
  mOrigQuoteGroup->hide();
  connect( mOrigQuoteGroup, SIGNAL( clicked( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  hbox = new QHBox( group );
  hbox->setSpacing( KDialog::spacingHint() );

  (void)new QLabel( i18n("Quote original message:"), hbox );

  radio = new QRadioButton( i18n("Nothin&g"), hbox );
  mOrigQuoteGroup->insert( radio );

  radio = new QRadioButton( i18n("&Full message"), hbox );
  mOrigQuoteGroup->insert( radio );

  radio = new QRadioButton( i18n("Onl&y headers"), hbox );
  mOrigQuoteGroup->insert( radio );

  w = new QWidget( hbox );
  hbox->setStretchFactor( w, 1 );

  mNoMDNsWhenEncryptedCheck = new QCheckBox( i18n("Do not send MDNs in response to encrypted messages"), group );
  connect( mNoMDNsWhenEncryptedCheck, SIGNAL(toggled(bool)), SLOT(slotEmitChanged()) );

  // Warning label:
  label = new KActiveLabel( i18n("<b>WARNING:</b> Unconditionally returning "
			   "confirmations undermines your privacy. "
			   "<a href=\"whatsthis:%1\">More...</a>")
			     .arg(receiptWhatsThis),
			   group );

  vlay->addWidget( group );

  // "Attached keys" group box:
  group = new QVGroupBox( i18n( "Certificate && Key Bundle Attachments" ), this );
  group->layout()->setSpacing( KDialog::spacingHint() );

  mAutomaticallyImportAttachedKeysCheck = new QCheckBox( i18n("Automatically import keys and certificates"), group );
  connect( mAutomaticallyImportAttachedKeysCheck, SIGNAL(toggled(bool)), SLOT(slotEmitChanged()) );

  vlay->addWidget( group );



  vlay->addStretch( 10 ); // spacer
}

void SecurityPage::GeneralTab::load() {
  const KConfigGroup reader( KMKernel::config(), "Reader" );

  mHtmlMailCheck->setChecked( reader.readBoolEntry( "htmlMail", false ) );
  mExternalReferences->setChecked( reader.readBoolEntry( "htmlLoadExternal", false ) );
  mAutomaticallyImportAttachedKeysCheck->setChecked( reader.readBoolEntry( "AutoImportKeys", false ) );

  const KConfigGroup mdn( KMKernel::config(), "MDN" );

  int num = mdn.readNumEntry( "default-policy", 0 );
  if ( num < 0 || num >= mMDNGroup->count() ) num = 0;
  mMDNGroup->setButton( num );
  num = mdn.readNumEntry( "quote-message", 0 );
  if ( num < 0 || num >= mOrigQuoteGroup->count() ) num = 0;
  mOrigQuoteGroup->setButton( num );
  mNoMDNsWhenEncryptedCheck->setChecked(mdn.readBoolEntry( "not-send-when-encrypted", true ));
}

void SecurityPage::GeneralTab::installProfile( KConfig * profile ) {
  const KConfigGroup reader( profile, "Reader" );
  const KConfigGroup mdn( profile, "MDN" );

  if ( reader.hasKey( "htmlMail" ) )
    mHtmlMailCheck->setChecked( reader.readBoolEntry( "htmlMail" ) );
  if ( reader.hasKey( "htmlLoadExternal" ) )
    mExternalReferences->setChecked( reader.readBoolEntry( "htmlLoadExternal" ) );
  if ( reader.hasKey( "AutoImportKeys" ) )
    mAutomaticallyImportAttachedKeysCheck->setChecked( reader.readBoolEntry( "AutoImportKeys" ) );

  if ( mdn.hasKey( "default-policy" ) ) {
      int num = mdn.readNumEntry( "default-policy" );
      if ( num < 0 || num >= mMDNGroup->count() ) num = 0;
      mMDNGroup->setButton( num );
  }
  if ( mdn.hasKey( "quote-message" ) ) {
      int num = mdn.readNumEntry( "quote-message" );
      if ( num < 0 || num >= mOrigQuoteGroup->count() ) num = 0;
      mOrigQuoteGroup->setButton( num );
  }
  if ( mdn.hasKey( "not-send-when-encrypted" ) )
      mNoMDNsWhenEncryptedCheck->setChecked(mdn.readBoolEntry( "not-send-when-encrypted" ));
}

void SecurityPage::GeneralTab::save() {
  KConfigGroup reader( KMKernel::config(), "Reader" );
  KConfigGroup mdn( KMKernel::config(), "MDN" );

  if (reader.readBoolEntry( "htmlMail", false ) != mHtmlMailCheck->isChecked())
  {
    if (KMessageBox::warningContinueCancel(this, i18n("Changing the global "
      "HTML setting will override all folder specific values."), QString::null,
      QString::null, "htmlMailOverride") == KMessageBox::Continue)
    {
      reader.writeEntry( "htmlMail", mHtmlMailCheck->isChecked() );
      QStringList names;
      QValueList<QGuardedPtr<KMFolder> > folders;
      kmkernel->folderMgr()->createFolderList(&names, &folders);
      kmkernel->imapFolderMgr()->createFolderList(&names, &folders);
      kmkernel->dimapFolderMgr()->createFolderList(&names, &folders);
      kmkernel->searchFolderMgr()->createFolderList(&names, &folders);
      for (QValueList<QGuardedPtr<KMFolder> >::iterator it = folders.begin();
        it != folders.end(); ++it)
      {
        if (*it)
        {
          KConfigGroupSaver saver(KMKernel::config(),
            "Folder-" + (*it)->idString());
          KMKernel::config()->writeEntry("htmlMailOverride", false);
        }
      }
    }
  }
  reader.writeEntry( "htmlLoadExternal", mExternalReferences->isChecked() );
  reader.writeEntry( "AutoImportKeys", mAutomaticallyImportAttachedKeysCheck->isChecked() );
  mdn.writeEntry( "default-policy", mMDNGroup->id( mMDNGroup->selected() ) );
  mdn.writeEntry( "quote-message", mOrigQuoteGroup->id( mOrigQuoteGroup->selected() ) );
  mdn.writeEntry( "not-send-when-encrypted", mNoMDNsWhenEncryptedCheck->isChecked() );
}


QString SecurityPage::ComposerCryptoTab::helpAnchor() const {
  return QString::fromLatin1("configure-security-composing");
}

SecurityPageComposerCryptoTab::SecurityPageComposerCryptoTab( QWidget * parent, const char * name )
  : ConfigModuleTab ( parent, name )
{
  // the margins are inside mWidget itself
  QVBoxLayout* vlay = new QVBoxLayout( this, 0, 0 );

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

void SecurityPage::ComposerCryptoTab::load() {
  const KConfigGroup composer( KMKernel::config(), "Composer" );

  // If you change default values, sync messagecomposer.cpp too

  mWidget->mAutoSignature->setChecked( composer.readBoolEntry( "pgp-auto-sign", false ) );

  mWidget->mEncToSelf->setChecked( composer.readBoolEntry( "crypto-encrypt-to-self", true ) );
  mWidget->mShowEncryptionResult->setChecked( false ); //composer.readBoolEntry( "crypto-show-encryption-result", true ) );
  mWidget->mShowEncryptionResult->hide();
  mWidget->mShowKeyApprovalDlg->setChecked( composer.readBoolEntry( "crypto-show-keys-for-approval", true ) );

  mWidget->mAutoEncrypt->setChecked( composer.readBoolEntry( "pgp-auto-encrypt", false ) );
  mWidget->mNeverEncryptWhenSavingInDrafts->setChecked( composer.readBoolEntry( "never-encrypt-drafts", true ) );

  mWidget->mStoreEncrypted->setChecked( composer.readBoolEntry( "crypto-store-encrypted", true ) );
}

void SecurityPage::ComposerCryptoTab::installProfile( KConfig * profile ) {
  const KConfigGroup composer( profile, "Composer" );

  if ( composer.hasKey( "pgp-auto-sign" ) )
    mWidget->mAutoSignature->setChecked( composer.readBoolEntry( "pgp-auto-sign" ) );

  if ( composer.hasKey( "crypto-encrypt-to-self" ) )
    mWidget->mEncToSelf->setChecked( composer.readBoolEntry( "crypto-encrypt-to-self" ) );
  if ( composer.hasKey( "crypto-show-encryption-result" ) )
    mWidget->mShowEncryptionResult->setChecked( composer.readBoolEntry( "crypto-show-encryption-result" ) );
  if ( composer.hasKey( "crypto-show-keys-for-approval" ) )
    mWidget->mShowKeyApprovalDlg->setChecked( composer.readBoolEntry( "crypto-show-keys-for-approval" ) );
  if ( composer.hasKey( "pgp-auto-encrypt" ) )
    mWidget->mAutoEncrypt->setChecked( composer.readBoolEntry( "pgp-auto-encrypt" ) );
  if ( composer.hasKey( "never-encrypt-drafts" ) )
    mWidget->mNeverEncryptWhenSavingInDrafts->setChecked( composer.readBoolEntry( "never-encrypt-drafts" ) );

  if ( composer.hasKey( "crypto-store-encrypted" ) )
    mWidget->mStoreEncrypted->setChecked( composer.readBoolEntry( "crypto-store-encrypted" ) );
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

SecurityPageWarningTab::SecurityPageWarningTab( QWidget * parent, const char * name )
  : ConfigModuleTab( parent, name )
{
  // the margins are inside mWidget itself
  QVBoxLayout* vlay = new QVBoxLayout( this, 0, 0 );

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

void SecurityPage::WarningTab::load() {
  const KConfigGroup composer( KMKernel::config(), "Composer" );

  mWidget->warnUnencryptedCB->setChecked( composer.readBoolEntry( "crypto-warning-unencrypted", false ) );
  mWidget->mWarnUnsigned->setChecked( composer.readBoolEntry( "crypto-warning-unsigned", false ) );
  mWidget->warnReceiverNotInCertificateCB->setChecked( composer.readBoolEntry( "crypto-warn-recv-not-in-cert", true ) );

  // The "-int" part of the key name is because there used to be a separate boolean
  // config entry for enabling/disabling. This is done with the single bool value now.
  mWidget->warnGroupBox->setChecked( composer.readBoolEntry( "crypto-warn-when-near-expire", true ) );

  mWidget->mWarnSignKeyExpiresSB->setValue( composer.readNumEntry( "crypto-warn-sign-key-near-expire-int", 14 ) );
  mWidget->mWarnSignChainCertExpiresSB->setValue( composer.readNumEntry( "crypto-warn-sign-chaincert-near-expire-int", 14 ) );
  mWidget->mWarnSignRootCertExpiresSB->setValue( composer.readNumEntry( "crypto-warn-sign-root-near-expire-int", 14 ) );

  mWidget->mWarnEncrKeyExpiresSB->setValue( composer.readNumEntry( "crypto-warn-encr-key-near-expire-int", 14 ) );
  mWidget->mWarnEncrChainCertExpiresSB->setValue( composer.readNumEntry( "crypto-warn-encr-chaincert-near-expire-int", 14 ) );
  mWidget->mWarnEncrRootCertExpiresSB->setValue( composer.readNumEntry( "crypto-warn-encr-root-near-expire-int", 14 ) );

  mWidget->enableAllWarningsPB->setEnabled( true );
}

void SecurityPage::WarningTab::installProfile( KConfig * profile ) {
  const KConfigGroup composer( profile, "Composer" );

  if ( composer.hasKey( "crypto-warning-unencrypted" ) )
    mWidget->warnUnencryptedCB->setChecked( composer.readBoolEntry( "crypto-warning-unencrypted" ) );
  if ( composer.hasKey( "crypto-warning-unsigned" ) )
    mWidget->mWarnUnsigned->setChecked( composer.readBoolEntry( "crypto-warning-unsigned" ) );
  if ( composer.hasKey( "crypto-warn-recv-not-in-cert" ) )
    mWidget->warnReceiverNotInCertificateCB->setChecked( composer.readBoolEntry( "crypto-warn-recv-not-in-cert" ) );

  if ( composer.hasKey( "crypto-warn-when-near-expire" ) )
    mWidget->warnGroupBox->setChecked( composer.readBoolEntry( "crypto-warn-when-near-expire" ) );

  if ( composer.hasKey( "crypto-warn-sign-key-near-expire-int" ) )
    mWidget->mWarnSignKeyExpiresSB->setValue( composer.readNumEntry( "crypto-warn-sign-key-near-expire-int" ) );
  if ( composer.hasKey( "crypto-warn-sign-chaincert-near-expire-int" ) )
    mWidget->mWarnSignChainCertExpiresSB->setValue( composer.readNumEntry( "crypto-warn-sign-chaincert-near-expire-int" ) );
  if ( composer.hasKey( "crypto-warn-sign-root-near-expire-int" ) )
    mWidget->mWarnSignRootCertExpiresSB->setValue( composer.readNumEntry( "crypto-warn-sign-root-near-expire-int" ) );

  if ( composer.hasKey( "crypto-warn-encr-key-near-expire-int" ) )
    mWidget->mWarnEncrKeyExpiresSB->setValue( composer.readNumEntry( "crypto-warn-encr-key-near-expire-int" ) );
  if ( composer.hasKey( "crypto-warn-encr-chaincert-near-expire-int" ) )
    mWidget->mWarnEncrChainCertExpiresSB->setValue( composer.readNumEntry( "crypto-warn-encr-chaincert-near-expire-int" ) );
  if ( composer.hasKey( "crypto-warn-encr-root-near-expire-int" ) )
    mWidget->mWarnEncrRootCertExpiresSB->setValue( composer.readNumEntry( "crypto-warn-encr-root-near-expire-int" ) );
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

SecurityPageSMimeTab::SecurityPageSMimeTab( QWidget * parent, const char * name )
  : ConfigModuleTab( parent, name )
{
  // the margins are inside mWidget itself
  QVBoxLayout* vlay = new QVBoxLayout( this, 0, 0 );

  mWidget = new SMimeConfiguration( this );
  vlay->addWidget( mWidget );

  // Button-group for exclusive radiobuttons
  QButtonGroup* bg = new QButtonGroup( mWidget );
  bg->hide();
  bg->insert( mWidget->CRLRB );
  bg->insert( mWidget->OCSPRB );

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
}

void SecurityPage::SMimeTab::load() {
  if ( !mConfig ) {
    setEnabled( false );
    return;
  }
  // Checkboxes
  mCheckUsingOCSPConfigEntry = configEntry( "gpgsm", "Security", "enable-ocsp", Kleo::CryptoConfigEntry::ArgType_None, false );
  mEnableOCSPsendingConfigEntry = configEntry( "dirmngr", "OCSP", "allow-ocsp", Kleo::CryptoConfigEntry::ArgType_None, false );
  mDoNotCheckCertPolicyConfigEntry = configEntry( "gpgsm", "Security", "disable-policy-checks", Kleo::CryptoConfigEntry::ArgType_None, false );
  mNeverConsultConfigEntry = configEntry( "gpgsm", "Security", "disable-crl-checks", Kleo::CryptoConfigEntry::ArgType_None, false );
  mFetchMissingConfigEntry = configEntry( "gpgsm", "Security", "auto-issuer-key-retrieve", Kleo::CryptoConfigEntry::ArgType_None, false );
  // Other widgets
  mOCSPResponderURLConfigEntry = configEntry( "dirmngr", "OCSP", "ocsp-responder", Kleo::CryptoConfigEntry::ArgType_String, false );
  mOCSPResponderSignature = configEntry( "dirmngr", "OCSP", "ocsp-signer", Kleo::CryptoConfigEntry::ArgType_String, false );

  // Initialize GUI items from the config entries

  if ( mCheckUsingOCSPConfigEntry ) {
    bool b = mCheckUsingOCSPConfigEntry->boolValue();
    mWidget->OCSPRB->setChecked( b );
    mWidget->CRLRB->setChecked( !b );
    mWidget->OCSPGroupBox->setEnabled( b );
  }
  if ( mDoNotCheckCertPolicyConfigEntry )
    mWidget->doNotCheckCertPolicyCB->setChecked( mDoNotCheckCertPolicyConfigEntry->boolValue() );
  if ( mNeverConsultConfigEntry )
    mWidget->neverConsultCB->setChecked( mNeverConsultConfigEntry->boolValue() );
  if ( mFetchMissingConfigEntry )
    mWidget->fetchMissingCB->setChecked( mFetchMissingConfigEntry->boolValue() );

  if ( mOCSPResponderURLConfigEntry )
    mWidget->OCSPResponderURL->setText( mOCSPResponderURLConfigEntry->stringValue() );
  if ( mOCSPResponderSignature ) {
    mWidget->OCSPResponderSignature->setFingerprint( mOCSPResponderSignature->stringValue() );
  }
}

void SecurityPage::SMimeTab::installProfile( KConfig * ) {
}

void SecurityPage::SMimeTab::save() {
  if ( !mConfig ) {
    return;
  }
  bool b = mWidget->OCSPRB->isChecked();
  if ( mCheckUsingOCSPConfigEntry && mCheckUsingOCSPConfigEntry->boolValue() != b )
    mCheckUsingOCSPConfigEntry->setBoolValue( b );
  // Set allow-ocsp together with enable-ocsp
  if ( mEnableOCSPsendingConfigEntry && mEnableOCSPsendingConfigEntry->boolValue() != b )
    mEnableOCSPsendingConfigEntry->setBoolValue( b );

  b = mWidget->doNotCheckCertPolicyCB->isChecked();
  if ( mDoNotCheckCertPolicyConfigEntry && mDoNotCheckCertPolicyConfigEntry->boolValue() != b )
    mDoNotCheckCertPolicyConfigEntry->setBoolValue( b );

  b = mWidget->neverConsultCB->isChecked();
  if ( mNeverConsultConfigEntry && mNeverConsultConfigEntry->boolValue() != b )
    mNeverConsultConfigEntry->setBoolValue( b );

  b = mWidget->fetchMissingCB->isChecked();
  if ( mFetchMissingConfigEntry && mFetchMissingConfigEntry->boolValue() != b )
    mFetchMissingConfigEntry->setBoolValue( b );

  QString txt = mWidget->OCSPResponderURL->text();
  if ( mOCSPResponderURLConfigEntry && mOCSPResponderURLConfigEntry->stringValue() != txt )
    mOCSPResponderURLConfigEntry->setStringValue( txt );

  txt = mWidget->OCSPResponderSignature->fingerprint();
  if ( mOCSPResponderSignature && mOCSPResponderSignature->stringValue() != txt ) {
    mOCSPResponderSignature->setStringValue( txt );
  }
  mConfig->sync( true );
}

Kleo::CryptoConfigEntry* SecurityPage::SMimeTab::configEntry( const char* componentName,
                                                              const char* groupName,
                                                              const char* entryName,
                                                              int /*Kleo::CryptoConfigEntry::ArgType*/ argType,
                                                              bool isList )
{
    Kleo::CryptoConfigEntry* entry = mConfig->entry( componentName, groupName, entryName );
    if ( !entry ) {
        kdWarning(5006) << QString( "Backend error: gpgconf doesn't seem to know the entry for %1/%2/%3" ).arg( componentName, groupName, entryName ) << endl;
        return 0;
    }
    if( entry->argType() != argType || entry->isList() != isList ) {
        kdWarning(5006) << QString( "Backend error: gpgconf has wrong type for %1/%2/%3: %4 %5" ).arg( componentName, groupName, entryName ).arg( entry->argType() ).arg( entry->isList() ) << endl;
        return 0;
    }
    return entry;
}

QString SecurityPage::CryptPlugTab::helpAnchor() const {
  return QString::fromLatin1("configure-security-crypto-backends");
}

SecurityPageCryptPlugTab::SecurityPageCryptPlugTab( QWidget * parent, const char * name )
  : ConfigModuleTab( parent, name )
{
  QVBoxLayout * vlay = new QVBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint() );

  mBackendConfig = Kleo::CryptoBackendFactory::instance()->configWidget( this, "mBackendConfig" );
  connect( mBackendConfig, SIGNAL( changed( bool ) ), this, SIGNAL( changed( bool ) ) );

  vlay->addWidget( mBackendConfig );
}

SecurityPageCryptPlugTab::~SecurityPageCryptPlugTab()
{

}

void SecurityPage::CryptPlugTab::load() {
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

MiscPage::MiscPage( QWidget * parent, const char * name )
  : ConfigModuleWithTabs( parent, name )
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

MiscPageFolderTab::MiscPageFolderTab( QWidget * parent, const char * name )
  : ConfigModuleTab( parent, name )
{
  // temp. vars:
  QVBoxLayout *vlay;
  QHBoxLayout *hlay;
  QLabel      *label;

  vlay = new QVBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint() );

  // "confirm before emptying folder" check box: stretch 0
  mEmptyFolderConfirmCheck =
    new QCheckBox( i18n("Corresponds to Folder->Move All Messages to Trash",
                        "Ask for co&nfirmation before moving all messages to "
                        "trash"),
                   this );
  vlay->addWidget( mEmptyFolderConfirmCheck );
  connect( mEmptyFolderConfirmCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  mExcludeImportantFromExpiry =
    new QCheckBox( i18n("E&xclude important messages from expiry"), this );
  vlay->addWidget( mExcludeImportantFromExpiry );
  connect( mExcludeImportantFromExpiry, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // "when trying to find unread messages" combo + label: stretch 0
  hlay = new QHBoxLayout( vlay ); // inherits spacing
  mLoopOnGotoUnread = new QComboBox( false, this );
  label = new QLabel( mLoopOnGotoUnread,
           i18n("to be continued with \"do not loop\", \"loop in current folder\", "
                "and \"loop in all folders\".",
                "When trying to find unread messages:"), this );
  mLoopOnGotoUnread->insertStringList( QStringList()
      << i18n("continuation of \"When trying to find unread messages:\"",
              "Do not Loop")
      << i18n("continuation of \"When trying to find unread messages:\"",
              "Loop in Current Folder")
      << i18n("continuation of \"When trying to find unread messages:\"",
              "Loop in All Folders"));
  hlay->addWidget( label );
  hlay->addWidget( mLoopOnGotoUnread, 1 );
  connect( mLoopOnGotoUnread, SIGNAL( activated( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // when entering a folder
  hlay = new QHBoxLayout( vlay ); // inherits spacing
  mActionEnterFolder = new QComboBox( false, this );
  label = new QLabel( mActionEnterFolder,
           i18n("to be continued with \"jump to first new message\", "
                "\"jump to first unread or new message\","
                "and \"jump to last selected message\".",
                "When entering a folder:"), this );
  mActionEnterFolder->insertStringList( QStringList()
      << i18n("continuation of \"When entering a folder:\"",
              "Jump to first new message")
      << i18n("continuation of \"When entering a folder:\"",
              "Jump to first unread or new message")
      << i18n("continuation of \"When entering a folder:\"",
              "Jump to last selected message"));
  hlay->addWidget( label );
  hlay->addWidget( mActionEnterFolder, 1 );
  connect( mActionEnterFolder, SIGNAL( activated( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  hlay = new QHBoxLayout( vlay ); // inherits spacing
  mDelayedMarkAsRead = new QCheckBox( i18n("Mar&k selected message as read after"), this );
  hlay->addWidget( mDelayedMarkAsRead );
  mDelayedMarkTime = new KIntSpinBox( 0 /*min*/, 60 /*max*/, 1/*step*/,
				      0 /*init*/, 10 /*base*/, this);
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
  hlay = new QHBoxLayout( vlay ); // inherits spacing
  mMailboxPrefCombo = new QComboBox( false, this );
  label = new QLabel( mMailboxPrefCombo,
		      i18n("to be continued with \"flat files\" and "
			   "\"directories\", resp.",
			   "By default, &message folders on disk are:"), this );
  mMailboxPrefCombo->insertStringList( QStringList()
	  << i18n("continuation of \"By default, &message folders on disk are\"",
		  "Flat Files (\"mbox\" format)")
	  << i18n("continuation of \"By default, &message folders on disk are\"",
		  "Directories (\"maildir\" format)") );
  hlay->addWidget( label );
  hlay->addWidget( mMailboxPrefCombo, 1 );
  connect( mMailboxPrefCombo, SIGNAL( activated( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // "On startup..." option:
  hlay = new QHBoxLayout( vlay ); // inherits spacing
  mOnStartupOpenFolder = new FolderRequester( this,
      kmkernel->getKMMainWidget()->folderTree() );
  label = new QLabel( mOnStartupOpenFolder,
                      i18n("Open this folder on startup:"), this );
  hlay->addWidget( label );
  hlay->addWidget( mOnStartupOpenFolder, 1 );
  connect( mOnStartupOpenFolder, SIGNAL( folderChanged( KMFolder* ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // "Empty &trash on program exit" option:
  mEmptyTrashCheck = new QCheckBox( i18n("Empty local &trash folder on program exit"),
                                    this );
  vlay->addWidget( mEmptyTrashCheck );
  connect( mEmptyTrashCheck, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  vlay->addStretch( 1 );

  // and now: add QWhatsThis:
  QString msg = i18n( "what's this help",
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
  QWhatsThis::add( mMailboxPrefCombo, msg );
  QWhatsThis::add( label, msg );
  // @TODO: Till, move into .kcgc file
  msg = i18n( "what's this help",
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
  QWhatsThis::add( mLoopOnGotoUnread, msg );
}

void MiscPage::FolderTab::load() {
  KConfigGroup general( KMKernel::config(), "General" );

  mEmptyTrashCheck->setChecked( general.readBoolEntry( "empty-trash-on-exit", true ) );
  mExcludeImportantFromExpiry->setChecked( GlobalSettings::excludeImportantMailFromExpiry() );
  mOnStartupOpenFolder->setFolder( general.readEntry( "startupFolder",
						  kmkernel->inboxFolder()->idString() ) );
  mEmptyFolderConfirmCheck->setChecked( general.readBoolEntry( "confirm-before-empty", true ) );
  // default = "Loop in current folder"

  mLoopOnGotoUnread->setCurrentItem( GlobalSettings::loopOnGotoUnread() );
  mActionEnterFolder->setCurrentItem( GlobalSettings::actionEnterFolder() );
  mDelayedMarkAsRead->setChecked( GlobalSettings::delayedMarkAsRead() );
  mDelayedMarkTime->setValue( GlobalSettings::delayedMarkTime() );
  mShowPopupAfterDnD->setChecked( GlobalSettings::showPopupAfterDnD() );

  int num = general.readNumEntry("default-mailbox-format", 1 );
  if ( num < 0 || num > 1 ) num = 1;
  mMailboxPrefCombo->setCurrentItem( num );
}

void MiscPage::FolderTab::save() {
  KConfigGroup general( KMKernel::config(), "General" );

  general.writeEntry( "empty-trash-on-exit", mEmptyTrashCheck->isChecked() );
  general.writeEntry( "confirm-before-empty", mEmptyFolderConfirmCheck->isChecked() );
  general.writeEntry( "default-mailbox-format", mMailboxPrefCombo->currentItem() );
  general.writeEntry( "startupFolder", mOnStartupOpenFolder->folder() ?
				  mOnStartupOpenFolder->folder()->idString() : QString::null );

  GlobalSettings::setDelayedMarkAsRead( mDelayedMarkAsRead->isChecked() );
  GlobalSettings::setDelayedMarkTime( mDelayedMarkTime->value() );
  GlobalSettings::setActionEnterFolder( mActionEnterFolder->currentItem() );
  GlobalSettings::setLoopOnGotoUnread( mLoopOnGotoUnread->currentItem() );
  GlobalSettings::setShowPopupAfterDnD( mShowPopupAfterDnD->isChecked() );
  GlobalSettings::setExcludeImportantMailFromExpiry(
        mExcludeImportantFromExpiry->isChecked() );
}

QString MiscPage::GroupwareTab::helpAnchor() const {
  return QString::fromLatin1("configure-misc-groupware");
}

MiscPageGroupwareTab::MiscPageGroupwareTab( QWidget* parent, const char* name )
  : ConfigModuleTab( parent, name )
{
  QBoxLayout* vlay = new QVBoxLayout( this, KDialog::marginHint(),
                                      KDialog::spacingHint() );
  vlay->setAutoAdd( true );

  // IMAP resource setup
  QVGroupBox* b1 = new QVGroupBox( i18n("&IMAP Resource Folder Options"),
                                   this );

  mEnableImapResCB =
    new QCheckBox( i18n("&Enable IMAP resource functionality"), b1 );
  QToolTip::add( mEnableImapResCB,  i18n( "This enables the IMAP storage for "
                                          "the Kontact applications" ) );
  QWhatsThis::add( mEnableImapResCB,
        i18n( GlobalSettings::self()->theIMAPResourceEnabledItem()->whatsThis().utf8() ) );
  connect( mEnableImapResCB, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  mBox = new QWidget( b1 );
  QGridLayout* grid = new QGridLayout( mBox, 4, 2, 0, KDialog::spacingHint() );
  grid->setColStretch( 1, 1 );
  connect( mEnableImapResCB, SIGNAL( toggled(bool) ),
	   mBox, SLOT( setEnabled(bool) ) );

  QLabel* storageFormatLA = new QLabel( i18n("&Format used for the groupware folders:"),
                                        mBox );
  QString toolTip = i18n( "Choose the format to use to store the contents of the groupware folders." );
  QString whatsThis = i18n( GlobalSettings::self()
        ->theIMAPResourceStorageFormatItem()->whatsThis().utf8() );
  grid->addWidget( storageFormatLA, 0, 0 );
  QToolTip::add( storageFormatLA, toolTip );
  QWhatsThis::add( storageFormatLA, whatsThis );
  mStorageFormatCombo = new QComboBox( false, mBox );
  storageFormatLA->setBuddy( mStorageFormatCombo );
  QStringList formatLst;
  formatLst << i18n("Standard (Ical / Vcard)") << i18n("Kolab (XML)");
  mStorageFormatCombo->insertStringList( formatLst );
  grid->addWidget( mStorageFormatCombo, 0, 1 );
  QToolTip::add( mStorageFormatCombo, toolTip );
  QWhatsThis::add( mStorageFormatCombo, whatsThis );
  connect( mStorageFormatCombo, SIGNAL( activated( int ) ),
           this, SLOT( slotStorageFormatChanged( int ) ) );

  QLabel* languageLA = new QLabel( i18n("&Language of the groupware folders:"),
                                   mBox );

  toolTip = i18n( "Set the language of the folder names" );
  whatsThis = i18n( GlobalSettings::self()
        ->theIMAPResourceFolderLanguageItem()->whatsThis().utf8() );
  grid->addWidget( languageLA, 1, 0 );
  QToolTip::add( languageLA, toolTip );
  QWhatsThis::add( languageLA, whatsThis );
  mLanguageCombo = new QComboBox( false, mBox );
  languageLA->setBuddy( mLanguageCombo );
  QStringList lst;
  lst << i18n("English") << i18n("German") << i18n("French") << i18n("Dutch");
  mLanguageCombo->insertStringList( lst );
  grid->addWidget( mLanguageCombo, 1, 1 );
  QToolTip::add( mLanguageCombo, toolTip );
  QWhatsThis::add( mLanguageCombo, whatsThis );
  connect( mLanguageCombo, SIGNAL( activated( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  mFolderComboLabel = new QLabel( mBox ); // text depends on storage format
  toolTip = i18n( "Set the parent of the resource folders" );
  whatsThis = i18n( GlobalSettings::self()->theIMAPResourceFolderParentItem()->whatsThis().utf8() );
  QToolTip::add( mFolderComboLabel, toolTip );
  QWhatsThis::add( mFolderComboLabel, whatsThis );
  grid->addWidget( mFolderComboLabel, 2, 0 );

  mFolderComboStack = new QWidgetStack( mBox );
  grid->addWidget( mFolderComboStack, 2, 1 );

  // First possibility in the widgetstack: a combo showing the list of all folders
  // This is used with the ical/vcard storage
  mFolderCombo = new FolderRequester( mBox,
      kmkernel->getKMMainWidget()->folderTree() );
  mFolderComboStack->addWidget( mFolderCombo, 0 );
  QToolTip::add( mFolderCombo, toolTip );
  QWhatsThis::add( mFolderCombo, whatsThis );
  connect( mFolderCombo, SIGNAL( folderChanged( KMFolder* ) ),
           this, SLOT( slotEmitChanged() ) );

  // Second possibility in the widgetstack: a combo showing the list of accounts
  // This is used with the kolab xml storage since the groupware folders
  // are always under the inbox.
  mAccountCombo = new KMail::AccountComboBox( mBox );
  mFolderComboStack->addWidget( mAccountCombo, 1 );
  QToolTip::add( mAccountCombo, toolTip );
  QWhatsThis::add( mAccountCombo, whatsThis );
  connect( mAccountCombo, SIGNAL( activated( int ) ),
           this, SLOT( slotEmitChanged() ) );

  mHideGroupwareFolders = new QCheckBox( i18n( "&Hide groupware folders" ),
                                         mBox, "HideGroupwareFoldersBox" );
  grid->addMultiCellWidget( mHideGroupwareFolders, 3, 3, 0, 1 );
  QToolTip::add( mHideGroupwareFolders,
                 i18n( "When this is checked, you will not see the IMAP "
                       "resource folders in the folder tree." ) );
  QWhatsThis::add( mHideGroupwareFolders, i18n( GlobalSettings::self()
           ->hideGroupwareFoldersItem()->whatsThis().utf8() ) );
  connect( mHideGroupwareFolders, SIGNAL( toggled( bool ) ),
           this, SLOT( slotEmitChanged() ) );

  // Groupware functionality compatibility setup
  b1 = new QVGroupBox( i18n("Groupware Compatibility && Legacy Options"), this );

  gBox = new QVBox( b1 );
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
  QToolTip::add( mLegacyMangleFromTo, i18n( "Turn this option on in order to make Outlook(tm) understand your answers to invitation replies" ) );
  QWhatsThis::add( mLegacyMangleFromTo, i18n( GlobalSettings::self()->
           legacyMangleFromToHeadersItem()->whatsThis().utf8() ) );
  connect( mLegacyMangleFromTo, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  mLegacyBodyInvites = new QCheckBox( i18n( "Send invitations in the mail body" ), gBox );
  QToolTip::add( mLegacyBodyInvites, i18n( "Turn this option on in order to make Outlook(tm) understand your answers to invitations" ) );
  QWhatsThis::add( mLegacyMangleFromTo, i18n( GlobalSettings::self()->
           legacyBodyInvitesItem()->whatsThis().utf8() ) );
  connect( mLegacyBodyInvites, SIGNAL( toggled( bool ) ),
           this, SLOT( slotLegacyBodyInvitesToggled( bool ) ) );
  connect( mLegacyBodyInvites, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );
  mAutomaticSending = new QCheckBox( i18n( "Automatic invitation sending" ), gBox );
  QToolTip::add( mAutomaticSending, i18n( "When this is on, the user will not see the mail composer window. Invitation mails are sent automatically" ) );
  QWhatsThis::add( mAutomaticSending, i18n( GlobalSettings::self()->
           automaticSendingItem()->whatsThis().utf8() ) );
  connect( mAutomaticSending, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotEmitChanged( void ) ) );

  // Open space padding at the end
  new QLabel( this );
}

void MiscPageGroupwareTab::slotLegacyBodyInvitesToggled( bool on )
{
  if ( on ) {
    QString txt = i18n( "<qt>Invitations are normally sent as attachments to "
                        "a mail. This switch changes the invitation mails to "
                        "be sent in the text of the mail instead; this is "
                        "necessary to send invitations and replies to "
                        "Microsoft Outlook.<br>But, when you do this, you no "
                        "longer get descriptive text that mail programs "
                        "can read; so, to people who have email programs "
                        "that do not understand the invitations, the "
                        "resulting messages look very odd.<br>People that have email "
                        "programs that do understand invitations will still "
                        "be able to work with this.</qt>" );
    KMessageBox::information( this, txt, QString::null,
                              "LegacyBodyInvitesWarning" );
  }
  // Invitations in the body are autosent in any case (no point in editing raw ICAL)
  // So the autosend option is only available if invitations are sent as attachment.
  mAutomaticSending->setEnabled( !mLegacyBodyInvites->isChecked() );
}

void MiscPage::GroupwareTab::load() {
  // Read the groupware config
  if ( mEnableGwCB ) {
    mEnableGwCB->setChecked( GlobalSettings::groupwareEnabled() );
    gBox->setEnabled( mEnableGwCB->isChecked() );
  }
  mLegacyMangleFromTo->setChecked( GlobalSettings::legacyMangleFromToHeaders() );
  mLegacyBodyInvites->blockSignals( true );
  mLegacyBodyInvites->setChecked( GlobalSettings::legacyBodyInvites() );
  mLegacyBodyInvites->blockSignals( false );
  mAutomaticSending->setChecked( GlobalSettings::automaticSending() );
  mAutomaticSending->setEnabled( !mLegacyBodyInvites->isChecked() );

  // Read the IMAP resource config
  mEnableImapResCB->setChecked( GlobalSettings::theIMAPResourceEnabled() );
  mBox->setEnabled( mEnableImapResCB->isChecked() );

  mHideGroupwareFolders->setChecked( GlobalSettings::hideGroupwareFolders() );
  int i = GlobalSettings::theIMAPResourceFolderLanguage();
  mLanguageCombo->setCurrentItem(i);
  i = GlobalSettings::theIMAPResourceStorageFormat();
  mStorageFormatCombo->setCurrentItem(i);
  slotStorageFormatChanged( i );

  QString folderId( GlobalSettings::theIMAPResourceFolderParent() );
  if( !folderId.isNull() && kmkernel->findFolderById( folderId ) ) {
    mFolderCombo->setFolder( folderId );
  } else {
    // Folder was deleted, we have to choose a new one
    mFolderCombo->setFolder( i18n( "<Choose a Folder>" ) );
  }

  KMAccount* selectedAccount = 0;
  int accountId = GlobalSettings::theIMAPResourceAccount();
  if ( accountId )
    selectedAccount = kmkernel->acctMgr()->find( accountId );
  else {
    // Fallback: iterate over accounts to select folderId if found (as an inbox folder)
    for( KMAccount *a = kmkernel->acctMgr()->first(); a!=0;
         a = kmkernel->acctMgr()->next() ) {
      if( a->folder() && a->folder()->child() ) {
        // Look inside that folder for an INBOX
        KMFolderNode *node;
        for (node = a->folder()->child()->first(); node; node = a->folder()->child()->next())
          if (!node->isDir() && node->name() == "INBOX") break;

        if ( node && static_cast<KMFolder*>(node)->idString() == folderId ) {
          selectedAccount = a;
          break;
        }
      }
    }
  }
  if ( selectedAccount )
    mAccountCombo->setCurrentAccount( selectedAccount );
  else if ( GlobalSettings::theIMAPResourceStorageFormat() == 1 )
    kdDebug(5006) << "Folder " << folderId << " not found as an account's inbox" << endl;
}

void MiscPage::GroupwareTab::save() {
  // Write the groupware config
  if ( mEnableGwCB )
    GlobalSettings::setGroupwareEnabled( mEnableGwCB->isChecked() );
  GlobalSettings::setLegacyMangleFromToHeaders( mLegacyMangleFromTo->isChecked() );
  GlobalSettings::setLegacyBodyInvites( mLegacyBodyInvites->isChecked() );
  GlobalSettings::setAutomaticSending( mAutomaticSending->isChecked() );

  int format = mStorageFormatCombo->currentItem();
  GlobalSettings::setTheIMAPResourceStorageFormat( format );

  // Write the IMAP resource config
  GlobalSettings::setHideGroupwareFolders( mHideGroupwareFolders->isChecked() );

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
      GlobalSettings::setTheIMAPResourceAccount( acct->id() );
    }
  }

  bool enabled = mEnableImapResCB->isChecked() && !folderId.isEmpty();
  GlobalSettings::setTheIMAPResourceEnabled( enabled );
  GlobalSettings::setTheIMAPResourceFolderLanguage( mLanguageCombo->currentItem() );
  GlobalSettings::setTheIMAPResourceFolderParent( folderId );
}

void MiscPage::GroupwareTab::slotStorageFormatChanged( int format )
{
  mLanguageCombo->setEnabled( format == 0 ); // only ical/vcard needs the language hack
  mFolderComboStack->raiseWidget( format );
  if ( format == 0 ) {
    mFolderComboLabel->setText( i18n("&Resource folders are subfolders of:") );
    mFolderComboLabel->setBuddy( mFolderCombo );
  } else {
    mFolderComboLabel->setText( i18n("&Resource folders are in account:") );
    mFolderComboLabel->setBuddy( mAccountCombo );
  }
}

#undef DIM


//----------------------------
#include "configuredialog.moc"
