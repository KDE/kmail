/*
 *   kmail: KDE mail client
 *   This file: Copyright (C) 2000 Espen Sand, espen@kde.org
 *              Copyright (C) 2001-2002 Marc Mutz, mutz@kde.org
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

// other KMail headers:
#include "simplestringlisteditor.h"
#include "accountdialog.h"
#include "colorlistbox.h"
#include "kmacctmgr.h"
#include "kmacctseldlg.h"
#include "kmsender.h"
#include "kmtopwidget.h"
#include "kmtransport.h"
#include "kmfoldermgr.h"
#include "kmgroupware.h"
#include "cryptplugconfigdialog.h"
#include "kmidentity.h"
#include "identitymanager.h"
#include "identitylistview.h"
#include "kcursorsaver.h"
#include "cryptplugwrapperlist.h"
#include "kmkernel.h"

using KMail::IdentityListView;
using KMail::IdentityListViewItem;
#include "identitydialog.h"
using KMail::IdentityDialog;

// other kdenetwork headers:
#include <kpgpui.h>
#include <kmime_util.h>
using KMime::DateFormatter;


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

// Qt headers:
#include <qvalidator.h>
#include <qwhatsthis.h>
#include <qvgroupbox.h>
#include <qvbuttongroup.h>
#include <qhbuttongroup.h>
#include <qtooltip.h>
#include <qlabel.h>
#include <qtextcodec.h>
#include <qheader.h>
#include <qpopupmenu.h>

// other headers:
#include <assert.h>

#ifndef _PATH_SENDMAIL
#define _PATH_SENDMAIL  "/usr/sbin/sendmail"
#endif


// little helper:
static inline QPixmap loadIcon( const char * name ) {
  return KGlobal::instance()->iconLoader()
    ->loadIcon( QString::fromLatin1(name), KIcon::NoGroup, KIcon::SizeMedium );
}


ConfigureDialog::ConfigureDialog( QWidget *parent, const char *name,
                                  bool modal )
  : KDialogBase( IconList, i18n("Configure"), Help|Apply|Ok|Cancel,
                 Ok, parent, name, modal, true )
{
  KWin::setIcons( winId(), kapp->icon(), kapp->miniIcon() );
  // setHelp() not needed, since we override slotHelp() anyway...

  setIconListAllVisible( true );

  connect( this, SIGNAL(cancelClicked()), this, SLOT(slotCancelOrClose()) );

  QWidget *page;
  QVBoxLayout *vlay;
  ConfigurationPage *configPage;

  // Identity Page:
  page = addPage( IdentityPage::iconLabel(), IdentityPage::title(),
		  loadIcon( IdentityPage::iconName() ) );
  vlay = new QVBoxLayout( page, 0, spacingHint() );
  configPage = new IdentityPage( page );
  vlay->addWidget( configPage );
  configPage->setPageIndex( pageIndex( page ) );
  mPages.append( configPage );

  // Network Page:
  page = addPage( NetworkPage::iconLabel(), NetworkPage::title(),
		  loadIcon( NetworkPage::iconName() ) );
  vlay = new QVBoxLayout( page, 0, spacingHint() );
  configPage = new NetworkPage( page );
  vlay->addWidget( configPage );
  configPage->setPageIndex( pageIndex( page ) );
  mPages.append( configPage );

  // ### FIXME: We need a KMTransportCombo...  It's also no good to
  // allow non-applied transports to be presented in the identity
  // settings...
  connect( configPage, SIGNAL(transportListChanged(const QStringList &)),
	   mPages.getFirst(), SLOT(slotUpdateTransportCombo(const QStringList &)) );

  // Appearance Page:
  page = addPage( AppearancePage::iconLabel(), AppearancePage::title(),
		  loadIcon( AppearancePage::iconName() ) );
  vlay = new QVBoxLayout( page, 0, spacingHint() );
  configPage = new AppearancePage( page );
  vlay->addWidget( configPage );
  configPage->setPageIndex( pageIndex( page ) );
  mPages.append( configPage );

  // ### FIXME: Extract profile managing out of the normal config
  // pages system...
  mPageWithProfiles = configPage;
  connect( configPage, SIGNAL(profileSelected(KConfig*)),
	   this, SLOT(slotInstallProfile(KConfig*)) );

  // Composer Page:
  page = addPage( ComposerPage::iconLabel(), ComposerPage::title(),
		  loadIcon( ComposerPage::iconName() ) );
  vlay = new QVBoxLayout( page, 0, spacingHint() );
  configPage = new ComposerPage( page );
  vlay->addWidget( configPage );
  configPage->setPageIndex( pageIndex( page ) );
  mPages.append( configPage );

  // Security Page:
  page = addPage( SecurityPage::iconLabel(), SecurityPage::title(),
		  loadIcon( SecurityPage::iconName() ) );
  vlay = new QVBoxLayout( page, 0, spacingHint() );
  configPage = new SecurityPage( page );
  vlay->addWidget( configPage );
  configPage->setPageIndex( pageIndex( page ) );
  mPages.append( configPage );

  // Folder Page:
  page = addPage( FolderPage::iconLabel(), FolderPage::title(),
		  loadIcon( FolderPage::iconName() ) );
  vlay = new QVBoxLayout( page, 0, spacingHint() );
  configPage = new FolderPage( page );
  vlay->addWidget( configPage );
  configPage->setPageIndex( pageIndex( page ) );
  mPages.append( configPage );

  // Groupware Page:
  page = addPage( GroupwarePage::iconLabel(), GroupwarePage::title(),
		  loadIcon( FolderPage::iconName() ) );
  vlay = new QVBoxLayout( page, 0, spacingHint() );
  configPage = new GroupwarePage( page );
  vlay->addWidget( configPage );
  configPage->setPageIndex( pageIndex( page ) );
  mPages.append( configPage );
}


ConfigureDialog::~ConfigureDialog()
{
}


void ConfigureDialog::show()
{
  // ### try to move the setup into the *Page::show() methods?
  if( !isVisible() )
    setup();
  KDialogBase::show();
}

void ConfigureDialog::slotCancelOrClose()
{
  for ( QPtrListIterator<ConfigurationPage> it( mPages ) ; it.current() ; ++it )
    it.current()->dismiss();
}

void ConfigureDialog::slotOk()
{
  apply( true );
  accept();
}


void ConfigureDialog::slotApply() {
  apply( false );
}

void ConfigureDialog::slotHelp() {
  int activePage = activePageIndex();
  if ( activePage >= 0 && activePage < (int)mPages.count() )
    kapp->invokeHelp( mPages.at( activePage )->helpAnchor() );
  else
    kdDebug(5006) << "ConfigureDialog::slotHelp(): no page selected???"
		  << endl;
}

void ConfigureDialog::setup()
{
  for ( QPtrListIterator<ConfigurationPage> it( mPages ) ; it.current() ; ++it )
    it.current()->setup();
}

void ConfigureDialog::slotInstallProfile( KConfig * profile ) {
  for ( QPtrListIterator<ConfigurationPage> it( mPages ) ; it.current() ; ++it )
    it.current()->installProfile( profile );
}

void ConfigureDialog::apply( bool everything ) {
  int activePage = activePageIndex();

  if ( !everything )
    mPages.at( activePage )->apply();
  else {
    // must be fiirst since it may install profiles!
    mPageWithProfiles->apply();
    // loop through the rest:
    for ( QPtrListIterator<ConfigurationPage> it( mPages ) ; it.current() ; ++it )
      if ( it.current() != mPageWithProfiles )
        it.current()->apply();
  }

  //
  // Make other components read the new settings
  //
  KMMessage::readConfig();
  KCursorSaver busy(KBusyPtr::busy()); // this can take some time when a large folder is open
  QPtrListIterator<KMainWindow> it( *KMainWindow::memberList );
  for ( it.toFirst() ; it.current() ; ++it )
    // ### FIXME: use dynamic_cast.
    if ( (*it)->inherits( "KMTopLevelWidget" ) )
      ((KMTopLevelWidget*)(*it))->readConfig();
}



// *************************************************************
// *                                                           *
// *                      IdentityPage                         *
// *                                                           *
// *************************************************************

QString IdentityPage::iconLabel() {
  return i18n("Identities");
}

QString IdentityPage::title() {
  return i18n("Manage Identities");
}

const char * IdentityPage::iconName() {
  return "identity";
}

QString IdentityPage::helpAnchor() const {
  return QString::fromLatin1("configure-identity");
}

IdentityPage::IdentityPage( QWidget * parent, const char * name )
  : ConfigurationPage( parent, name ),
    mIdentityDialog( 0 )
{
  QHBoxLayout * hlay = new QHBoxLayout( this, 0, KDialog::spacingHint() );

  mIdentityList = new IdentityListView( this );
  connect( mIdentityList, SIGNAL(selectionChanged(QListViewItem*)),
	   SLOT(slotIdentitySelectionChanged(QListViewItem*)) );
  connect( mIdentityList, SIGNAL(itemRenamed(QListViewItem*,const QString&,int)),
	   SLOT(slotRenameIdentity(QListViewItem*,const QString&,int)) );
  connect( mIdentityList, SIGNAL(doubleClicked(QListViewItem*,const QPoint&,int)),
	   SLOT(slotModifyIdentity()) );
  connect( mIdentityList, SIGNAL(contextMenu(KListView*,QListViewItem*,const QPoint&)),
	   SLOT(slotContextMenu(KListView*,QListViewItem*,const QPoint&)) );
  // ### connect dragged(...), ...

  hlay->addWidget( mIdentityList, 1 );

  QVBoxLayout * vlay = new QVBoxLayout( hlay ); // inherits spacing

  QPushButton * button = new QPushButton( i18n("&New..."), this );
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
}

void IdentityPage::setup()
{
  kdDebug(5006) << "IdentityPage::setup()" << endl;
  IdentityManager * im = kernel->identityManager();
  mOldNumberOfIdentities = im->shadowIdentities().count();
  // Fill the list:
  mIdentityList->clear();
  // Don't use ConstIterator here - it iterates over the wrong list!
  QListViewItem * item = 0;
  for ( IdentityManager::Iterator it = im->begin() ; it != im->end() ; ++it )
    item = new IdentityListViewItem( mIdentityList, item, *it  );
  mIdentityList->setSelected( mIdentityList->currentItem(), true );
}

void IdentityPage::apply() {
  assert( !mIdentityDialog );

  kernel->identityManager()->sort();
  kernel->identityManager()->commit();

  if( mOldNumberOfIdentities < 2 && mIdentityList->childCount() > 1 ) {
    // have more than one identity, so better show the combo in the
    // composer now:
    KConfigGroup composer( KMKernel::config(), "Composer" );
    int showHeaders = composer.readNumEntry( "headers", HDR_STANDARD );
    showHeaders |= HDR_IDENTITY;
    composer.writeEntry( "headers", showHeaders );
  }
}

void IdentityPage::dismiss() {
  assert( !mIdentityDialog );
  kernel->identityManager()->rollback();
}

void IdentityPage::slotNewIdentity()
{
  assert( !mIdentityDialog );

  IdentityManager * im = kernel->identityManager();
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
	KMIdentity & dupThis = im->identityForName( dialog.duplicateIdentity() );
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
    KMIdentity & newIdent = im->identityForName( identityName );
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
  }

  delete mIdentityDialog;
  mIdentityDialog = 0;
}

void IdentityPage::slotRemoveIdentity()
{
  assert( !mIdentityDialog );

  IdentityManager * im = kernel->identityManager();
  kdFatal( im->shadowIdentities().count() < 2 )
    << "Attempted to remove the last identity!" << endl;

  IdentityListViewItem * item =
    dynamic_cast<IdentityListViewItem*>( mIdentityList->selectedItem() );
  if ( !item ) return;

  QString msg = i18n("<qt>Do you really want to remove the identity named "
		     "<b>%1</b>?</qt>").arg( item->identity().identityName() );
  if( KMessageBox::warningYesNo( this, msg ) == KMessageBox::Yes )
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
       !kernel->identityManager()->shadowIdentities().contains( newName ) ) {
    KMIdentity & ident = item->identity();
    ident.setIdentityName( newName );
  }
  item->redisplay();
}

void IdentityPage::slotContextMenu( KListView *, QListViewItem * i,
				    const QPoint & pos ) {
  IdentityListViewItem * item = dynamic_cast<IdentityListViewItem*>( i );

  QPopupMenu * menu = new QPopupMenu( this );
  menu->insertItem( i18n("New..."), this, SLOT(slotNewIdentity()) );
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

  IdentityManager * im = kernel->identityManager();
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
}

void IdentityPage::slotIdentitySelectionChanged( QListViewItem * i ) {
  kdDebug(5006) << "IdentityPage::slotIdentitySelectionChanged( " << i << " )" << endl;

  IdentityListViewItem * item = dynamic_cast<IdentityListViewItem*>( i );

  if ( !item ) return;

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
// *                       NetworkPage                         *
// *                                                           *
// *************************************************************

QString NetworkPage::iconLabel() {
  return i18n("Network");
}

QString NetworkPage::title() {
  return i18n("Setup for Sending and Receiving Messages");
}

const char * NetworkPage::iconName() {
  return "network";
}

QString NetworkPage::helpAnchor() const {
  return QString::fromLatin1("configure-network");
}

NetworkPage::NetworkPage( QWidget * parent, const char * name )
  : TabbedConfigurationPage( parent, name )
{
  //
  // "Sending" tab:
  //
  mSendingTab = new SendingTab();
  addTab( mSendingTab, mSendingTab->title() );
  connect( mSendingTab, SIGNAL(transportListChanged(const QStringList&)),
	   this, SIGNAL(transportListChanged(const QStringList&)) );

  //
  // "Receiving" tab:
  //
  mReceivingTab = new ReceivingTab();
  addTab( mReceivingTab, mReceivingTab->title() );

  connect( mReceivingTab, SIGNAL(accountListChanged(const QStringList &)),
	   this, SIGNAL(accountListChanged(const QStringList &)) );
}

QString NetworkPage::SendingTab::title() {
  return i18n("&Sending");
}

QString NetworkPage::SendingTab::helpAnchor() const {
  return QString::fromLatin1("configure-network-sending");
}

NetworkPageSendingTab::NetworkPageSendingTab( QWidget * parent, const char * name )
  : ConfigurationPage( parent, name )
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
  mTransportUpButton->setPixmap( BarIcon( "up", KIcon::SizeSmall ) );
  //  mTransportUpButton->setPixmap( BarIcon( "up", KIcon::SizeSmall ) );
  mTransportUpButton->setAutoDefault( false );
  mTransportUpButton->setEnabled( false ); // b/c no item is selected yet
  connect( mTransportUpButton, SIGNAL(clicked()),
           this, SLOT(slotTransportUp()) );
  btn_vlay->addWidget( mTransportUpButton );

  // "down" button: stretch 0
  // ### FIXME: shouldn't this be a QToolButton?
  mTransportDownButton = new QPushButton( QString::null, this );
  mTransportDownButton->setPixmap( BarIcon( "down", KIcon::SizeSmall ) );
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

  // "send messages in outbox on check" check box:
  mSendOutboxCheck =
    new QCheckBox(i18n("Send messages in outbox &folder on check"), group );
  glay->addMultiCellWidget( mSendOutboxCheck, 1, 1, 0, 1 );

  // "default send method" combo:
  mSendMethodCombo = new QComboBox( false, group );
  mSendMethodCombo->insertStringList( QStringList()
				      << i18n("Send Now")
				      << i18n("Send Later") );
  glay->addWidget( mSendMethodCombo, 2, 1 );

  // "message property" combo:
  // ### FIXME: remove completely?
  mMessagePropertyCombo = new QComboBox( false, group );
  mMessagePropertyCombo->insertStringList( QStringList()
		     << i18n("Allow 8-bit")
		     << i18n("MIME Compliant (Quoted Printable)") );
  glay->addWidget( mMessagePropertyCombo, 3, 1 );

  // "default domain" input field:
  mDefaultDomainEdit = new KLineEdit( group );
  glay->addMultiCellWidget( mDefaultDomainEdit, 4, 4, 1, 2 );

  // labels:
  glay->addWidget( new QLabel( mSendMethodCombo, /*buddy*/
			       i18n("Defa&ult send method:"), group ), 2, 0 );
  glay->addWidget( new QLabel( mMessagePropertyCombo, /*buddy*/
			       i18n("Message &property:"), group ), 3, 0 );
  QLabel *l = new QLabel( mDefaultDomainEdit, /*buddy*/
                          i18n("Default domain:"), group );
  glay->addWidget( l, 4, 0 );

  // and now: add QWhatsThis:
  QString msg = i18n( "<qt><p>The default domain is used to complete email "
                      "addresses that only consist of the user's name."
                      "</p></qt>" );
  QWhatsThis::add( l, msg );
  QWhatsThis::add( mDefaultDomainEdit, msg );
}


void NetworkPage::SendingTab::slotTransportSelected()
{
  QListViewItem *cur = mTransportList->currentItem();
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

void NetworkPage::SendingTab::slotAddTransport()
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
			   "Configure->Network->Sending listview, \"type\" "
			   "column, first row, to indicate that this is the "
			   "default transport", "%1 (Default)")
      .arg( transportInfo->type );
  (void) new QListViewItem( mTransportList, lastItem, transportInfo->name,
			    typeDisplayName );

  // notify anyone who cares:
  emit transportListChanged( transportNames );
}

void NetworkPage::SendingTab::slotModifySelectedTransport()
{
  QListViewItem *item = mTransportList->currentItem();
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
}


void NetworkPage::SendingTab::slotRemoveSelectedTransport()
{
  QListViewItem *item = mTransportList->currentItem();
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
}


void NetworkPage::SendingTab::slotTransportUp()
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
			    "Configure->Network->Sending listview, \"type\" "
			    "column, first row, to indicate that this is the "
			    "default transport", "%1 (Default)")
		    .arg( ti->type ) );

  mTransportList->setCurrentItem( above );
  mTransportList->setSelected( above, true );
}


void NetworkPage::SendingTab::slotTransportDown()
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
			   "Configure->Network->Sending listview, \"type\" "
			   "column, first row, to indicate that this is the "
			   "default transport", "%1 (Default)")
		   .arg( ti2->type ) );


  mTransportList->setCurrentItem(below);
  mTransportList->setSelected(below, TRUE);
}

void NetworkPage::SendingTab::setup() {
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
			       "Configure->Network->Sending listview, "
			       "\"type\" column, first row, to indicate "
			       "that this is the default transport",
			       "%1 (Default)").arg( listItem->text(1) ) );
    mTransportList->setCurrentItem( listItem );
    mTransportList->setSelected( listItem, true );
  }

  mSendMethodCombo->setCurrentItem(
		kernel->msgSender()->sendImmediate() ? 0 : 1 );
  mMessagePropertyCombo->setCurrentItem(
                kernel->msgSender()->sendQuotedPrintable() ? 1 : 0 );
  mSendOutboxCheck->setChecked( general.readBoolEntry( "sendOnCheck",
						       false ) );

  mConfirmSendCheck->setChecked( composer.readBoolEntry( "confirm-before-send",
							 false ) );
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


void NetworkPage::SendingTab::apply() {
  KConfigGroup general( KMKernel::config(), "General" );
  KConfigGroup composer( KMKernel::config(), "Composer" );

  // Save transports:
  general.writeEntry( "transports", mTransportInfoList.count() );
  QPtrListIterator<KMTransportInfo> it( mTransportInfoList );
  for ( int i = 1 ; it.current() ; ++it, ++i )
    (*it)->writeConfig(i);

  // Save common options:
  general.writeEntry( "sendOnCheck", mSendOutboxCheck->isChecked() );
  kernel->msgSender()->setSendImmediate(
			     mSendMethodCombo->currentItem() == 0 );
  kernel->msgSender()->setSendQuotedPrintable(
			     mMessagePropertyCombo->currentItem() == 1 );
  kernel->msgSender()->writeConfig( false ); // don't sync
  composer.writeEntry("confirm-before-send", mConfirmSendCheck->isChecked() );
  general.writeEntry( "Default domain", mDefaultDomainEdit->text() );
}



QString NetworkPage::ReceivingTab::title() {
  return i18n("&Receiving");
}

QString NetworkPage::ReceivingTab::helpAnchor() const {
  return QString::fromLatin1("configure-network-receiving");
}

NetworkPageReceivingTab::NetworkPageReceivingTab( QWidget * parent, const char * name )
  : ConfigurationPage ( parent, name )
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

  // "New Mail Notification" group box: stretch 0
  group = new QVGroupBox( i18n("New Mail Notification"), this );
  vlay->addWidget( group );
  group->layout()->setSpacing( KDialog::spacingHint() );

  // "beep on new mail" check box:
  mBeepNewMailCheck = new QCheckBox(i18n("&Beep"), group );
  mBeepNewMailCheck->setSizePolicy( QSizePolicy( QSizePolicy::MinimumExpanding,
                                                 QSizePolicy::Fixed ) );
  // "Systray" notification check box
  mSystrayCheck = new QCheckBox( i18n("S&ystem tray notification"), group );

  // System tray modes
  QButtonGroup *bgroup = new QButtonGroup(i18n("System Tray Modes"), group);
  bgroup->setColumnLayout(0, Qt::Horizontal);
  bgroup->layout()->setSpacing( 0 );
  bgroup->layout()->setMargin( 0 );
  QGridLayout *bgroupLayout = new QGridLayout( bgroup->layout() );
  bgroupLayout->setAlignment( Qt::AlignTop );
  bgroupLayout->setSpacing( 6 );
  bgroupLayout->setMargin( 11 );

  mBlinkingSystray = new QRadioButton( i18n("Always show system tray"), bgroup);
  bgroupLayout->addWidget(mBlinkingSystray, 0, 0);

  mSystrayOnNew = new QRadioButton( i18n("Show system tray on new mail"), bgroup);
  bgroupLayout->addWidget(mSystrayOnNew, 0, 1);

  bgroup->setEnabled( false ); // since !mSystrayCheck->isChecked()
  connect( mSystrayCheck, SIGNAL(toggled(bool)),
           bgroup, SLOT(setEnabled(bool)) );

  mCheckmailStartupCheck = new QCheckBox( i18n("Check mail on startup"), group );

  // "display message box" check box:
  mOtherNewMailActionsButton = new QPushButton( i18n("Other Actio&ns"), group );
  mOtherNewMailActionsButton->setSizePolicy( QSizePolicy( QSizePolicy::Fixed,
                                                          QSizePolicy::Fixed ) );
  connect( mOtherNewMailActionsButton, SIGNAL(clicked()),
           this, SLOT(slotEditNotifications()) );
}


void NetworkPage::ReceivingTab::slotAccountSelected()
{
  QListViewItem * item = mAccountList->selectedItem();
  mModifyAccountButton->setEnabled( item );
  mRemoveAccountButton->setEnabled( item );
}

QStringList NetworkPage::ReceivingTab::occupiedNames()
{
  QStringList accountNames = kernel->acctMgr()->getAccounts();

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

void NetworkPage::ReceivingTab::slotAddAccount() {
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
    = kernel->acctMgr()->create( QString::fromLatin1( accountType ),
				 i18n("Unnamed") );
  if ( !account ) {
    // ### FIXME: Give the user more information. Is this error
    // recoverable?
    KMessageBox::sorry( this, i18n("Unable to create account") );
    return;
  }

  account->init(); // fill the account fields with good default values

  AccountDialog dialog( i18n("Add account"), account, this );

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
}



void NetworkPage::ReceivingTab::slotModifySelectedAccount()
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
      account = kernel->acctMgr()->find( listItem->text(0) );
      if( !account ) {
	// ### FIXME: How should this happen? See above.
        KMessageBox::sorry( this, i18n("Unable to locate account") );
        return;
      }

      ModifiedAccountsType *mod = new ModifiedAccountsType;
      mod->oldAccount = account;
      mod->newAccount = kernel->acctMgr()->create( account->type(),
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
}



void NetworkPage::ReceivingTab::slotRemoveSelectedAccount() {
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
    acct = kernel->acctMgr()->find( listItem->text(0) );
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
}

void NetworkPage::ReceivingTab::slotEditNotifications()
{
  KNotifyDialog::configure(this);
}

void NetworkPage::ReceivingTab::setup() {
  KConfigGroup general( KMKernel::config(), "General" );

  mAccountList->clear();
  QListViewItem *top = 0;
  for( KMAccount *a = kernel->acctMgr()->first(); a!=0;
       a = kernel->acctMgr()->next() ) {
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

  mSystrayCheck->setChecked( general.readBoolEntry("systray-on-mail", false) );
  mBlinkingSystray->setChecked( !general.readBoolEntry("systray-on-new", true) );
  mSystrayOnNew->setChecked( general.readBoolEntry("systray-on-new", true) );
  mCheckmailStartupCheck->setChecked( general.readBoolEntry("checkmail-startup", false) );
}

void NetworkPage::ReceivingTab::apply() {
  // Add accounts marked as new
  QValueList< QGuardedPtr<KMAccount> > newCachedImapAccounts;
  QValueList< QGuardedPtr<KMAccount> >::Iterator it;
  for (it = mNewAccounts.begin(); it != mNewAccounts.end(); ++it ) {
    kernel->acctMgr()->add( *it );
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
    // ### FIXME: KConfig has now deleteGroup()!
    // The old entries will never really disappear, so better at least
    // clear the password:
    (*it)->clearPasswd();
    kernel->acctMgr()->writeConfig( true );
    if ( !(*it) || !kernel->acctMgr()->remove(*it) )
      KMessageBox::sorry( this, i18n("<qt>Unable to locate account <b>%1</b>.</qt>")
			  .arg( (*it)->name() ) );
  }
  mAccountsToDelete.clear();

  // Incoming mail
  kernel->acctMgr()->writeConfig( false );
  kernel->cleanupImapFolders();

  // Save Mail notification settings
  KConfigGroup general( KMKernel::config(), "General" );
  general.writeEntry( "beep-on-mail", mBeepNewMailCheck->isChecked() );
  general.writeEntry( "systray-on-mail", mSystrayCheck->isChecked() );
  general.writeEntry( "systray-on-new", mSystrayOnNew->isChecked() );

  general.writeEntry( "checkmail-startup", mCheckmailStartupCheck->isChecked() );

  // Sync new IMAP accounts ASAP:
  for (it = newCachedImapAccounts.begin(); it != newCachedImapAccounts.end(); ++it ) {
    (*it)->processNewMail(false);
  }
}

void NetworkPage::ReceivingTab::dismiss() {
  // dismiss new accounts:
  for ( QValueList< QGuardedPtr<KMAccount> >::Iterator
	  it = mNewAccounts.begin() ;
	it != mNewAccounts.end() ; ++it )
    delete *it;

  // dismiss modifications of accounts:
  for ( QValueList< ModifiedAccountsType* >::Iterator
	  it = mModifiedAccounts.begin() ;
	it != mModifiedAccounts.end() ; ++it ) {
    delete (*it)->newAccount;
    delete (*it);
  }

  // cancel deletion of accounts:
  mAccountsToDelete.clear();

  mNewAccounts.clear(); // ### Why that? didn't we just delete all items?
  mModifiedAccounts.clear(); // ### see above...
}

// *************************************************************
// *                                                           *
// *                     AppearancePage                        *
// *                                                           *
// *************************************************************

QString AppearancePage::iconLabel() {
  return i18n("Appearance");
}

QString AppearancePage::title() {
  return i18n("Customize Visual Appearance");
}

const char * AppearancePage::iconName() {
  return "appearance";
}

QString AppearancePage::helpAnchor() const {
  return QString::fromLatin1("configure-appearance");
}

AppearancePage::AppearancePage( QWidget * parent, const char * name )
  : TabbedConfigurationPage( parent, name )
{
  //
  // "Fonts" tab:
  //
  mFontsTab = new FontsTab();
  addTab( mFontsTab, mFontsTab->title() );

  //
  // "Colors" tab:
  //
  mColorsTab = new ColorsTab();
  addTab( mColorsTab, mColorsTab->title() );

  //
  // "Layout" tab:
  //
  mLayoutTab = new LayoutTab();
  addTab( mLayoutTab, mLayoutTab->title() );

  //
  // "Headers" tab:
  //
  mHeadersTab = new HeadersTab();
  addTab( mHeadersTab, mHeadersTab->title() );

  //
  // "Profile" tab:
  //
  mProfileTab = new ProfileTab();
  addTab( mProfileTab, mProfileTab->title() );

  connect( mProfileTab, SIGNAL(profileSelected(KConfig*)),
	   this, SIGNAL(profileSelected(KConfig*)) );
}

void AppearancePage::apply() {
  mProfileTab->apply(); // must be first, since it may install profiles!
  mFontsTab->apply();
  mColorsTab->apply();
  mLayoutTab->apply();
  mHeadersTab->apply();
}


QString AppearancePage::FontsTab::title() {
  return i18n("&Fonts");
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
  : ConfigurationPage( parent, name ), mActiveFontIndex( -1 )
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


  // Display the new setting:
  mFontChooser->setFont( mFont[index], fontNames[index].onlyFixed );

  // Disable Family and Size list if we have selected a quote font:
  mFontChooser->enableColumn( KFontChooser::FamilyList|KFontChooser::SizeList,
			      fontNames[ index ].enableFamilyAndSize );
}

void AppearancePage::FontsTab::setup() {
  KConfigGroup fonts( KMKernel::config(), "Fonts" );

  mFont[0] = KGlobalSettings::generalFont();
  QFont fixedFont = KGlobalSettings::fixedFont();
  for ( int i = 0 ; i < numFontNames ; i++ )
    mFont[i] = fonts.readFontEntry( fontNames[i].configName,
      (fontNames[i].onlyFixed) ? &fixedFont : &mFont[0] );

  mCustomFontCheck->setChecked( !fonts.readBoolEntry( "defaultFonts", true ) );
  mFontLocationCombo->setCurrentItem( 0 );
  // ### FIXME: possible Qt bug: setCurrentItem doesn't emit activated(int).
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

void AppearancePage::FontsTab::apply() {
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


QString AppearancePage::ColorsTab::title() {
  return i18n("Colo&rs");
}

QString AppearancePage::ColorsTab::helpAnchor() const {
  return QString::fromLatin1("configure-appearance-colors");
}


static const struct {
  const char * configName;
  const char * displayName;
} colorNames[] = { // adjust setup() if you change this:
  { "BackgroundColor", I18N_NOOP("Composer background") },
  { "AltBackgroundColor", I18N_NOOP("Alternative background color") },
  { "ForegroundColor", I18N_NOOP("Normal text") },
  { "QuotedText1", I18N_NOOP("Quoted text - first level") },
  { "QuotedText2", I18N_NOOP("Quoted text - second level") },
  { "QuotedText3", I18N_NOOP("Quoted text - third level") },
  { "LinkColor", I18N_NOOP("Link") },
  { "FollowedColor", I18N_NOOP("Followed link") },
  { "NewMessage", I18N_NOOP("New message") },
  { "UnreadMessage", I18N_NOOP("Unread message") },
  { "FlagMessage", I18N_NOOP("Important message") },
  { "PGPMessageEncr", I18N_NOOP("OpenPGP message - encrypted") },
  { "PGPMessageOkKeyOk", I18N_NOOP("OpenPGP message - valid signature with trusted key") },
  { "PGPMessageOkKeyBad", I18N_NOOP("OpenPGP message - valid signature with untrusted key") },
  { "PGPMessageWarn", I18N_NOOP("OpenPGP message - unchecked signature") },
  { "PGPMessageErr", I18N_NOOP("OpenPGP message - bad signature") },
  { "HTMLWarningColor", I18N_NOOP("Border around warning prepending HTML messages") },
  { "ColorbarBackgroundPlain", I18N_NOOP("HTML status bar background - No HTML message") },
  { "ColorbarForegroundPlain", I18N_NOOP("HTML status bar foreground - No HTML message") },
  { "ColorbarBackgroundHTML",  I18N_NOOP("HTML status bar background - HTML message") },
  { "ColorbarForegroundHTML",  I18N_NOOP("HTML status bar foreground - HTML message") },
};
static const int numColorNames = sizeof colorNames / sizeof *colorNames;

AppearancePageColorsTab::AppearancePageColorsTab( QWidget * parent, const char * name )
  : ConfigurationPage( parent, name )
{
  // tmp. vars:
  QVBoxLayout *vlay;

  // "use custom colors" check box
  vlay = new QVBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint() );
  mCustomColorCheck = new QCheckBox( i18n("&Use custom colors"), this );
  vlay->addWidget( mCustomColorCheck );

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

  // {en,dir}able widgets depending on the state of mCustomColorCheck:
  connect( mCustomColorCheck, SIGNAL(toggled(bool)),
	   mColorList, SLOT(setEnabled(bool)) );
  connect( mCustomColorCheck, SIGNAL(toggled(bool)),
	   mRecycleColorCheck, SLOT(setEnabled(bool)) );
}

void AppearancePage::ColorsTab::setup() {
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
    Qt::red, // new msg
    Qt::blue, // unread mgs
    QColor( 0x00, 0x7F, 0x00 ), // important msg
    QColor( 0x00, 0x80, 0xFF ), // light blue // pgp encrypted
    QColor( 0x40, 0xFF, 0x40 ), // light green // pgp ok, trusted key
    QColor( 0xA0, 0xFF, 0x40 ), // light yellow // pgp ok, untrusted key
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

void AppearancePage::ColorsTab::apply() {
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

QString AppearancePage::LayoutTab::title() {
  return i18n("&Layout");
}

QString AppearancePage::LayoutTab::helpAnchor() const {
  return QString::fromLatin1("configure-appearance-layout");
}

static const int numWindowLayouts = 5;

static const char * windowLayoutToolTips[numWindowLayouts] = {
  I18N_NOOP("<qt><h3>KMail Window Layout</h3>"
	    "<ul>"
	    "<li>Long folder list</li>"
	    "<li>MIME tree (if visible) between message list and reader pane</li>"
	    "</ul>"
	    "</qt>"),
  I18N_NOOP("<qt><h3>KMail Window Layout</h3>"
	    "<ul>"
	    "<li>Long folder list</li>"
	    "<li>MIME tree (if visible) below reader pane</li>"
	    "</ul>"
	    "</qt>"),
  I18N_NOOP("<qt><h3>KMail Window Layout</h3>"
	    "<ul>"
	    "<li>Basically long folder list</li>"
	    "<li>MIME tree (if visible) below folder list</li>"
	    "</ul>"
	    "</qt>"),
  I18N_NOOP("<qt><h3>KMail Window Layout</h3>"
	    "<ul>"
	    "<li>Medium folder list</li>"
	    "<li>MIME tree (if visible) between message list and reader pane</li>"
	    "<li>Full width reader pane</li>"
	    "</ul>"
	    "</qt>"),
  I18N_NOOP("<qt><h3>KMail Window Layout</h3>"
	    "<ul>"
	    "<li>Short folder list</li>"
	    "<li>MIME tree (if visible) between message list / folder tree "
	    "and reader pane</li>"
	    "<li>Full width reader pane and MIME tree</li>"
	    "</ul>"
	    "</qt>")
};

AppearancePageLayoutTab::AppearancePageLayoutTab( QWidget * parent, const char * name )
  : ConfigurationPage( parent, name ),
    mShowMIMETreeModeLastValue( -1 )
{
  // tmp. vars:
  QVBoxLayout * vlay;
  QPushButton * button;

  vlay = new QVBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint() );

  // "show colorbar" check box:
  mShowColorbarCheck = new QCheckBox( i18n("Show HTML status &bar"), this );
  vlay->addWidget( mShowColorbarCheck );
  vlay->addWidget( new QLabel( i18n("<qt><p>Below, you can change the "
				    "arrangement of KMail's window components "
				    "(folder list, message list, reader pane "
				    "and the optional MIME tree).</p></qt>"),
			       this ) );
  // The window layout
  mWindowLayoutBG = new QHButtonGroup( i18n("&Window Layout"), this );
  mWindowLayoutBG->layout()->setSpacing( KDialog::spacingHint() );
  mWindowLayoutBG->setExclusive( true );

  for ( int i = 0 ; i < numWindowLayouts ; ++i ) {
    button = new QPushButton( mWindowLayoutBG );
    mWindowLayoutBG->insert( button, i );
    button->setPixmap( pixmapFor( i, 2 /* never */ ) );
    button->setFixedSize( button->sizeHint() );
    button->setAutoDefault( false );
    button->setToggleButton( true );
    QToolTip::add( button, i18n( windowLayoutToolTips[i] ) );
  }

  vlay->addWidget( mWindowLayoutBG );

  // the MIME Tree Viewer
  mShowMIMETreeMode = new QVButtonGroup( i18n("Show MIME Tree"), this );
  mShowMIMETreeMode->layout()->setSpacing( KDialog::spacingHint() );

  mShowMIMETreeMode->insert(
    new QRadioButton( i18n("&Never"),  mShowMIMETreeMode ), 0 );
  mShowMIMETreeMode->insert(
    new QRadioButton( i18n("&Smart"),  mShowMIMETreeMode ), 1 );
  mShowMIMETreeMode->insert(
    new QRadioButton( i18n("Alwa&ys"), mShowMIMETreeMode ), 2 );

  vlay->addWidget( mShowMIMETreeMode );

  connect( mShowMIMETreeMode, SIGNAL(clicked(int)),
	   this, SLOT(showMIMETreeClicked(int)) );

  vlay->addStretch( 10 ); // spacer
}

QPixmap AppearancePage::LayoutTab::pixmapFor( int layout, int mode ) {
  QString suffix;
  switch( mode ) {
  case 0: // Never
    suffix = "_no_mime";
    break;
  case 1: // Smart
    suffix = "_smart_mime";
    break;
  default: ;
  }

  // the icon files are numbered 1..5 !
  return UserIcon( QString("kmailwindowlayout%1").arg( layout+1 ) + suffix );
}

void AppearancePage::LayoutTab::showMIMETreeClicked( int mode )
{
  if ( mShowMIMETreeModeLastValue == mode ) return;

  mShowMIMETreeModeLastValue = mode;
  for ( int i = 0 ; i < numWindowLayouts ; ++i )
    mWindowLayoutBG->find( i )->setPixmap( pixmapFor( i, mode ) );
}

void AppearancePage::LayoutTab::setup() {
  KConfigGroup reader( KMKernel::config(), "Reader" );
  KConfigGroup geometry( KMKernel::config(), "Geometry" );

  mShowColorbarCheck->setChecked( reader.readBoolEntry( "showColorbar", false ) );

  int windowLayout = geometry.readNumEntry( "windowLayout", 1 );
  if( windowLayout < 0 || windowLayout > 4 )
      windowLayout = 1;
  mWindowLayoutBG->setButton( windowLayout );

  int num = geometry.readNumEntry( "showMIME", 1 );
  if ( num < 0 || num > 2 ) num = 1;
  mShowMIMETreeMode->setButton( num );
  showMIMETreeClicked( num );
}

void AppearancePage::LayoutTab::installProfile( KConfig * profile ) {
  KConfigGroup reader( profile, "Reader" );
  KConfigGroup geometry( profile, "Geometry" );

  if ( reader.hasKey( "showColorbar" ) )
    mShowColorbarCheck->setChecked( reader.readBoolEntry( "showColorbar" ) );

  if( geometry.hasKey( "windowLayout" ) ) {
      int windowLayout = geometry.readNumEntry( "windowLayout", 0 );
      if( windowLayout < 0 || windowLayout > 4 )
          windowLayout = 0;
      mWindowLayoutBG->setButton( windowLayout );
  }

  if( geometry.hasKey( "showMIME" ) ) {
    int num = geometry.readNumEntry( "showMIME" );
    if ( num < 0 || num > 2 ) num = 1;
    mShowMIMETreeMode->setButton( num );
  }
}

void AppearancePage::LayoutTab::apply() {
  KConfigGroup reader( KMKernel::config(), "Reader" );
  KConfigGroup geometry( KMKernel::config(), "Geometry" );

  reader.writeEntry( "showColorbar", mShowColorbarCheck->isChecked() );

  geometry.writeEntry( "windowLayout",
                       mWindowLayoutBG->id( mWindowLayoutBG->selected() ) );
  geometry.writeEntry( "showMIME",
                       mShowMIMETreeMode->id( mShowMIMETreeMode->selected()));
}

QString AppearancePage::HeadersTab::title() {
  return i18n("H&eaders");
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
  { I18N_NOOP("Fanc&y format (%1)"), KMime::DateFormatter::Fancy },
  { I18N_NOOP("C&ustom (Shift+F1 for help)"), KMime::DateFormatter::Custom }
};
static const int numDateDisplayConfig =
  sizeof dateDisplayConfig / sizeof *dateDisplayConfig;

AppearancePageHeadersTab::AppearancePageHeadersTab( QWidget * parent, const char * name )
  : ConfigurationPage( parent, name ),
    mCustomDateFormatEdit( 0 )
{
  // tmp. vars:
  QButtonGroup * group;
  QRadioButton * radio;
  QString msg;

  QVBoxLayout * vlay = new QVBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint() );

  // "General Options" group:
  group = new QVButtonGroup( i18n( "General Options" ), this );
  group->layout()->setSpacing( KDialog::spacingHint() );

  mMessageSizeCheck = new QCheckBox( i18n("&Display message sizes"), group );

  mCryptoIconsCheck = new QCheckBox( i18n( "Show crypto &icons" ), group );

  mNestedMessagesCheck =
    new QCheckBox( i18n("&Thread list of message headers"), group );

  vlay->addWidget( group );

  // "Message Header Threading Options" group:
  mNestingPolicy =
    new QVButtonGroup( i18n("Message Header Threading Options"), this );
  mNestingPolicy->layout()->setSpacing( KDialog::spacingHint() );

  mNestingPolicy->insert(
    new QRadioButton( i18n("Always &keep threads open"),
		      mNestingPolicy ), 0 );
  mNestingPolicy->insert(
    new QRadioButton( i18n("Threads default to op&en"),
		      mNestingPolicy ), 1 );
  mNestingPolicy->insert(
    new QRadioButton( i18n("Threads default to clo&sed"),
		      mNestingPolicy ), 2 );
  mNestingPolicy->insert(
    new QRadioButton( i18n("Open threads that contain new, unread "
			   "or important &messages and open watched threads."),
                      mNestingPolicy ), 3 );

  vlay->addWidget( mNestingPolicy );

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
      msg = i18n( "<qt><p><strong>These expressions may be used for the date:"
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
      QWhatsThis::add( mCustomDateFormatEdit, msg );
      QWhatsThis::add( radio, msg );
    }
  } // end for loop populating mDateDisplay

  vlay->addWidget( mDateDisplay );

  vlay->addStretch( 10 ); // spacer
}

void AppearancePage::HeadersTab::setup() {
  KConfigGroup general( KMKernel::config(), "General" );
  KConfigGroup geometry( KMKernel::config(), "Geometry" );

  // "General Options":
  mNestedMessagesCheck->setChecked( geometry.readBoolEntry( "nestedMessages", false ) );
  mMessageSizeCheck->setChecked( general.readBoolEntry( "showMessageSize", false ) );
  mCryptoIconsCheck->setChecked( general.readBoolEntry( "showCryptoIcons", false ) );

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

  if ( geometry.hasKey( "nestingPolicy" ) ) {
    int num = geometry.readNumEntry( "nestingPolicy" );
    if ( num < 0 || num > 3 ) num = 3;
    mNestingPolicy->setButton( num );
  }

  if ( general.hasKey( "dateFormat" ) )
    setDateDisplay( general.readNumEntry( "dateFormat" ),
		   general.readEntry( "customDateFormat" ) );
}

void AppearancePage::HeadersTab::apply() {
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

  int dateDisplayID = mDateDisplay->id( mDateDisplay->selected() );
  // check bounds:
  assert( dateDisplayID >= 0 ); assert( dateDisplayID < numDateDisplayConfig );
  general.writeEntry( "dateFormat",
		      dateDisplayConfig[ dateDisplayID ].dateDisplay );
  general.writeEntry( "customDateFormat", mCustomDateFormatEdit->text() );
}


QString AppearancePage::ProfileTab::title() {
  return i18n("&Profiles");
}

QString AppearancePage::ProfileTab::helpAnchor() const {
  return QString::fromLatin1("configure-appearance-profiles");
}

AppearancePageProfileTab::AppearancePageProfileTab( QWidget * parent, const char * name )
  : ConfigurationPage( parent, name )
{
  // tmp. vars:
  QVBoxLayout *vlay;

  vlay = new QVBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint() );

  mListView = new KListView( this, "mListView" );
  mListView->addColumn( i18n("Available Profiles") );
  mListView->addColumn( i18n("Description") );
  mListView->setFullWidth();
  mListView->setAllColumnsShowFocus( true );
  mListView->setFrameStyle( QFrame::WinPanel + QFrame::Sunken );
  mListView->setSorting( -1 );

  vlay->addWidget( new QLabel( mListView,
			       i18n("&Select a profile and click 'Apply' to take "
				    "on its settings:"), this ) );
  vlay->addWidget( mListView, 1 );

  /* not implemented (yet?)
  hlay = new QHBoxLayout( vlay );
  QPushButton *pushButton = new QPushButton(i18n("&New"), page4 );
  pushButton->setAutoDefault( false );
  hlay->addWidget( pushButton );
  mAppearance.profileDeleteButton = new QPushButton(i18n("Dele&te"), page4 );
  mAppearance.profileDeleteButton->setAutoDefault( false );
  hlay->addWidget( mAppearance.profileDeleteButton );
  hlay->addStretch(10);
  */
}

void AppearancePage::ProfileTab::setup() {
  mListView->clear();
  // find all profiles (config files named "profile-xyz-rc"):
  QString profileFilenameFilter = QString::fromLatin1("profile-*-rc");
  mProfileList = KGlobal::dirs()->findAllResources( "data", "kmail/"+
						    profileFilenameFilter );

  kdDebug(5006) << "Profile manager: found " << mProfileList.count()
		<< " profiles:" << endl;

  // build the list and populate the list view:
  QListViewItem * listItem = 0;
  for ( QStringList::Iterator it = mProfileList.begin() ;
	it != mProfileList.end() ; ++it ) {
    KConfig profile( (*it), true /* read-only */, false /* no KDE global */ );
    profile.setGroup("KMail Profile");
    QString name = profile.readEntry( "Name" );
    if ( name.isEmpty() ) {
      kdWarning(5006) << "File \"" << (*it)
		      << "\" doesn't provide a profile name!" << endl;
      name = i18n("Unnamed");
    }
    QString desc = profile.readEntry( "Comment" );
    if ( desc.isEmpty() ) {
      kdWarning(5006) << "File \"" << (*it)
		      << "\" doesn't provide a description!" << endl;
      desc = i18n("Not available");
    }
    listItem = new QListViewItem( mListView, listItem, name, desc );
  }
}


void AppearancePage::ProfileTab::apply() {
  if ( !this->isVisible() ) return; // don't apply when not currently shown

  int index = mListView->itemIndex( mListView->selectedItem() );
  if ( index < 0 ) return; // non selected

  assert( index < (int)mProfileList.count() );

  KConfig profile( *mProfileList.at(index), true, false );
  emit profileSelected( &profile );
}


// *************************************************************
// *                                                           *
// *                      ComposerPage                         *
// *                                                           *
// *************************************************************



QString ComposerPage::iconLabel() {
  return i18n("Composer");
}

const char * ComposerPage::iconName() {
  return "edit";
}

QString ComposerPage::title() {
  return i18n("Phrases & General Behavior");
}

QString ComposerPage::helpAnchor() const {
  return QString::fromLatin1("configure-composer");
}

ComposerPage::ComposerPage( QWidget * parent, const char * name )
  : TabbedConfigurationPage( parent, name )
{
  //
  // "General" tab:
  //
  mGeneralTab = new GeneralTab();
  addTab( mGeneralTab, mGeneralTab->title() );

  //
  // "Phrases" tab:
  //
  mPhrasesTab = new PhrasesTab();
  addTab( mPhrasesTab, mPhrasesTab->title() );

  //
  // "Subject" tab:
  //
  mSubjectTab = new SubjectTab();
  addTab( mSubjectTab, mSubjectTab->title() );

  //
  // "Charset" tab:
  //
  mCharsetTab = new CharsetTab();
  addTab( mCharsetTab, mCharsetTab->title() );

  //
  // "Headers" tab:
  //
  mHeadersTab = new HeadersTab();
  addTab( mHeadersTab, mHeadersTab->title() );
}

QString ComposerPage::GeneralTab::title() {
  return i18n("&General");
}

QString ComposerPage::GeneralTab::helpAnchor() const {
  return QString::fromLatin1("configure-composer-general");
}


ComposerPageGeneralTab::ComposerPageGeneralTab( QWidget * parent, const char * name )
  : ConfigurationPage( parent, name )
{
  // tmp. vars:
  QVBoxLayout *vlay;
  QHBoxLayout *hlay;
  QGroupBox   *group;
  QLabel      *label;
  QHBox       *hbox;

  vlay = new QVBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint() );

  // some check buttons...
  mAutoAppSignFileCheck =
    new QCheckBox( i18n("A&utomatically append signature"), this );
  vlay->addWidget( mAutoAppSignFileCheck );

  mSmartQuoteCheck = new QCheckBox( i18n("Use smart &quoting"), this );
  vlay->addWidget( mSmartQuoteCheck );

  mAutoRequestMDNCheck = new QCheckBox( i18n("Automatically request message disposition notifications"), this );
  vlay->addWidget( mAutoRequestMDNCheck );

  // a checkbutton for "word wrap" and a spinbox for the column in
  // which to wrap:
  hlay = new QHBoxLayout( vlay ); // inherits spacing
  mWordWrapCheck = new QCheckBox( i18n("Word &wrap at column:"), this );
  hlay->addWidget( mWordWrapCheck );
  mWrapColumnSpin = new KIntSpinBox( 30/*min*/, 78/*max*/, 1/*step*/,
				     78/*init*/, 10 /*base*/, this );
  mWrapColumnSpin->setEnabled( false ); // since !mWordWrapCheck->isChecked()
  hlay->addWidget( mWrapColumnSpin );
  hlay->addStretch( 1 );
  // only enable the spinbox if the checkbox is checked:
  connect( mWordWrapCheck, SIGNAL(toggled(bool)),
	   mWrapColumnSpin, SLOT(setEnabled(bool)) );

  // The "exteral editor" group:
  group = new QVGroupBox( i18n("External Editor"), this );
  group->layout()->setSpacing( KDialog::spacingHint() );

  mExternalEditorCheck =
    new QCheckBox( i18n("Use e&xternal editor instead of composer"), group );

  hbox = new QHBox( group );
  label = new QLabel( i18n("Specify e&ditor:"), hbox );
  mEditorRequester = new KURLRequester( hbox );
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

  label = new QLabel( i18n("\"%f\" will be replaced with the "
			   "filename to edit."), group );
  label->setEnabled( false ); // see above
  connect( mExternalEditorCheck, SIGNAL(toggled(bool)),
	   label, SLOT(setEnabled(bool)) );

  vlay->addWidget( group );
  vlay->addStretch( 100 );

  QString msg = i18n("<qt><p>Enable this option if you want KMail to request "
		     "Message Disposition Notifications (MDNs) for each of your "
		     "outgoing messages.</p>"
		     "<p>This option only affects the default; "
		     "you can still enable or disable MDN requesting on a "
		     "per-message basis in the composer, menu item "
		     "<em>Options</em>->&gt;"
		     "<em>Request Disposition Notification</em>.</p></qt>");
  QWhatsThis::add( mAutoRequestMDNCheck, msg );
}

void ComposerPage::GeneralTab::setup() {
  KConfigGroup composer( KMKernel::config(), "Composer" );
  KConfigGroup general( KMKernel::config(), "General" );

  // various check boxes:
  bool state = ( composer.readEntry("signature").lower() != "manual" );
  mAutoAppSignFileCheck->setChecked( state );
  mSmartQuoteCheck->setChecked( composer.readBoolEntry( "smart-quote", true ) );
  mAutoRequestMDNCheck->setChecked( composer.readBoolEntry( "request-mdn", false ) );
  mWordWrapCheck->setChecked( composer.readBoolEntry( "word-wrap", true ) );
  mWrapColumnSpin->setValue( composer.readNumEntry( "break-at", 78 ) );

  // editor group:
  mExternalEditorCheck->setChecked( general.readBoolEntry( "use-external-editor", false ) );
  mEditorRequester->setURL( general.readPathEntry( "external-editor" ) );
}

void ComposerPage::GeneralTab::installProfile( KConfig * profile ) {
  KConfigGroup composer( profile, "Composer" );
  KConfigGroup general( profile, "General" );

  if ( composer.hasKey( "signature" ) ) {
    bool state = ( composer.readEntry("signature").lower() == "auto" );
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

  if ( general.hasKey( "use-external-editor" )
       && general.hasKey( "external-editor" ) ) {
    mExternalEditorCheck->setChecked( general.readBoolEntry( "use-external-editor" ) );
    mEditorRequester->setURL( general.readPathEntry( "external-editor" ) );
  }
}

void ComposerPage::GeneralTab::apply() {
  KConfigGroup general( KMKernel::config(), "General" );
  KConfigGroup composer( KMKernel::config(), "Composer" );

  general.writeEntry( "use-external-editor", mExternalEditorCheck->isChecked()
		                         && !mEditorRequester->url().isEmpty() );
  general.writePathEntry( "external-editor", mEditorRequester->url() );

  bool autoSignature = mAutoAppSignFileCheck->isChecked();
  composer.writeEntry( "signature", autoSignature ? "auto" : "manual" );
  composer.writeEntry( "smart-quote", mSmartQuoteCheck->isChecked() );
  composer.writeEntry( "request-mdn", mAutoRequestMDNCheck->isChecked() );
  composer.writeEntry( "word-wrap", mWordWrapCheck->isChecked() );
  composer.writeEntry( "break-at", mWrapColumnSpin->value() );
}



QString ComposerPage::PhrasesTab::title() {
  return i18n("&Phrases");
}

QString ComposerPage::PhrasesTab::helpAnchor() const {
  return QString::fromLatin1("configure-composer-phrases");
}

ComposerPagePhrasesTab::ComposerPagePhrasesTab( QWidget * parent, const char * name )
  : ConfigurationPage( parent, name )
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
  glay->addMultiCellWidget( new QLabel( i18n("The following placeholders are "
					     "supported in the reply phrases:\n"
					     "%D=date, %S=subject, \n"
                                             "%e=sender's address, %F=sender's name, %f=sender's initials,\n"
                                             "%T=recipient's name, %t=recipient's name and address\n"
                                             "%C=carbon copy names, %c=carbon copy names and addresses\n"
					     "%%=percent sign, %_=space, "
					     "%L=linebreak"), this ),
			    0, 0, 0, 2 ); // row 0; cols 0..2

  // row 1: label and language combo box:
  mPhraseLanguageCombo = new LanguageComboBox( false, this );
  glay->addWidget( new QLabel( mPhraseLanguageCombo,
			       i18n("&Language:"), this ), 1, 0 );
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
  glay->addWidget( new QLabel( mPhraseReplyEdit,
			       i18n("Repl&y to sender:"), this ), 3, 0 );
  glay->addMultiCellWidget( mPhraseReplyEdit, 3, 3, 1, 2 ); // cols 1..2

  // row 4: "reply to all" line edit and label:
  mPhraseReplyAllEdit = new KLineEdit( this );
  glay->addWidget( new QLabel( mPhraseReplyAllEdit,
			       i18n("Reply &to all:"), this ), 4, 0 );
  glay->addMultiCellWidget( mPhraseReplyAllEdit, 4, 4, 1, 2 ); // cols 1..2

  // row 5: "forward" line edit and label:
  mPhraseForwardEdit = new KLineEdit( this );
  glay->addWidget( new QLabel( mPhraseForwardEdit,
			       i18n("&Forward:"), this ), 5, 0 );
  glay->addMultiCellWidget( mPhraseForwardEdit, 5, 5, 1, 2 ); // cols 1..2

  // row 6: "quote indicator" line edit and label:
  mPhraseIndentPrefixEdit = new KLineEdit( this );
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
  QWidget* w = dynamic_cast<QWidget*>(parent());

  if ( !w ) return;

  NewLanguageDialog dialog( mLanguageList, w , "New", true );
  int result = dialog.exec();
  if ( result == QDialog::Accepted ) slotAddNewLanguage( dialog.language() );
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
}

void ComposerPage::PhrasesTab::slotLanguageChanged( const QString& )
{
  int index = mPhraseLanguageCombo->currentItem();
  assert( index < (int)mLanguageList.count() );
  saveActiveLanguageItem();
  mActiveLanguageItem = index;
  setLanguageItemInformation( index );
}


void ComposerPage::PhrasesTab::setup() {
  KConfigGroup general( KMKernel::config(), "General" );

  mLanguageList.clear();
  mPhraseLanguageCombo->clear();
  mActiveLanguageItem = -1;

  int num = general.readNumEntry( "reply-languages", 0 );
  int currentNr = general.readNumEntry( "reply-current-language" ,0 );

  // build mLanguageList and mPhraseLanguageCombo:
  for ( int i = 0 ; i < num ; i++ ) {
    KConfigGroup config( KMKernel::config(),
			 QCString("KMMessage #") + QCString().setNum(i) );
    QString lang = config.readEntry( "language" );
    mLanguageList.append(
         LanguageItem( lang,
		       config.readEntry( "phrase-reply" ),
		       config.readEntry( "phrase-reply-all" ),
		       config.readEntry( "phrase-forward" ),
		       config.readEntry( "indent-prefix" ) ) );
    mPhraseLanguageCombo->insertLanguage( lang );
  }

  if ( num == 0 )
    slotAddNewLanguage( KGlobal::locale()->language() );

  if ( currentNr >= num || currentNr < 0 )
    currentNr = 0;

  mPhraseLanguageCombo->setCurrentItem( currentNr );
  mActiveLanguageItem = currentNr;
  setLanguageItemInformation( currentNr );
  mRemoveButton->setEnabled( mLanguageList.count() > 1 );
}

void ComposerPage::PhrasesTab::apply() {
  KConfigGroup general( KMKernel::config(), "General" );

  general.writeEntry( "reply-languages", mLanguageList.count() );
  general.writeEntry( "reply-current-language", mPhraseLanguageCombo->currentItem() );

  saveActiveLanguageItem();
  LanguageItemList::Iterator it = mLanguageList.begin();
  for ( int i = 0 ; it != mLanguageList.end() ; ++it, ++i ) {
    KConfigGroup config( KMKernel::config(),
			 QCString("KMMessage #") + QCString().setNum(i) );
    config.writeEntry( "language", (*it).mLanguage );
    config.writeEntry( "phrase-reply", (*it).mReply );
    config.writeEntry( "phrase-reply-all", (*it).mReplyAll );
    config.writeEntry( "phrase-forward", (*it).mForward );
    config.writeEntry( "indent-prefix", (*it).mIndentPrefix );
  }
}



QString ComposerPage::SubjectTab::title() {
  return i18n("&Subject");
}

QString ComposerPage::SubjectTab::helpAnchor() const {
  return QString::fromLatin1("configure-composer-subject");
}

ComposerPageSubjectTab::ComposerPageSubjectTab( QWidget * parent, const char * name )
  : ConfigurationPage( parent, name )
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
    static_cast<SimpleStringListEditor::ButtonCode>( SimpleStringListEditor::Add|SimpleStringListEditor::Remove );
  mReplyListEditor =
    new SimpleStringListEditor( group, 0, buttonCode,
				i18n("A&dd..."), i18n("Re&move"),
				QString::null,
				i18n("Enter new reply prefix:") );

  // row 2: "replace [...]" check box:
  mReplaceReplyPrefixCheck =
     new QCheckBox( i18n("Replace recognized prefi&x with \"Re:\""), group );

  vlay->addWidget( group );


  group = new QVGroupBox( i18n("Forward Subject Prefixes"), this );
  group->layout()->setSpacing( KDialog::marginHint() );

  // row 0: help text:
  label= new QLabel( i18n("Recognize any sequence of the following prefixes\n"
			  "(entries are case-insensitive regular expressions):"), group );
  label->setAlignment( AlignLeft|WordBreak );

  // row 1: string list editor
  mForwardListEditor =
    new SimpleStringListEditor( group, 0, buttonCode,
				i18n("Add..."),
				i18n("Remo&ve"), QString::null,
				i18n("Enter new forward prefix:") );

  // row 3: "replace [...]" check box:
  mReplaceForwardPrefixCheck =
     new QCheckBox( i18n("Replace recognized prefix with \"&Fwd:\""), group );

  vlay->addWidget( group );
}

void ComposerPage::SubjectTab::setup() {
  KConfigGroup composer( KMKernel::config(), "Composer" );

  QStringList prefixList = composer.readListEntry( "reply-prefixes", ',' );
  if ( prefixList.isEmpty() )
    prefixList << QString::fromLatin1("Re\\s*:")
	       << QString::fromLatin1("Re\\[\\d+\\]:")
	       << QString::fromLatin1("Re\\d+:");
  mReplyListEditor->setStringList( prefixList );

  mReplaceReplyPrefixCheck->setChecked( composer.readBoolEntry("replace-reply-prefix", true ) );

  prefixList = composer.readListEntry( "forward-prefixes", ',' );
  if ( prefixList.isEmpty() )
    prefixList << QString::fromLatin1("Fwd:")
              << QString::fromLatin1("FW:");
  mForwardListEditor->setStringList( prefixList );

  mReplaceForwardPrefixCheck->setChecked( composer.readBoolEntry( "replace-forward-prefix", true ) );
}

void ComposerPage::SubjectTab::apply() {
  KConfigGroup composer( KMKernel::config(), "Composer" );


  composer.writeEntry( "reply-prefixes", mReplyListEditor->stringList() );
  composer.writeEntry( "forward-prefixes", mForwardListEditor->stringList() );
  composer.writeEntry( "replace-reply-prefix",
		       mReplaceReplyPrefixCheck->isChecked() );
  composer.writeEntry( "replace-forward-prefix",
		       mReplaceForwardPrefixCheck->isChecked() );
}



QString ComposerPage::CharsetTab::title() {
  return i18n("Cha&rset");
}

QString ComposerPage::CharsetTab::helpAnchor() const {
  return QString::fromLatin1("configure-composer-charset");
}

ComposerPageCharsetTab::ComposerPageCharsetTab( QWidget * parent, const char * name )
  : ConfigurationPage( parent, name )
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
				i18n("&Modify"), i18n("Enter charset:") );
  vlay->addWidget( mCharsetListEditor, 1 );

  mKeepReplyCharsetCheck = new QCheckBox( i18n("&Keep original charset when "
						"replying or forwarding (if "
						"possible)."), this );
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
      .arg( QCString( kernel->networkCodec()->mimeName() ).lower() );
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

void ComposerPage::CharsetTab::setup() {
  KConfigGroup composer( KMKernel::config(), "Composer" );

  QStringList charsets = composer.readListEntry( "pref-charsets" );
  for ( QStringList::Iterator it = charsets.begin() ;
	it != charsets.end() ; ++it )
      if ( (*it) == QString::fromLatin1("locale") )
	(*it) = QString("%1 (locale)")
	  .arg( QCString( kernel->networkCodec()->mimeName() ).lower() );

  mCharsetListEditor->setStringList( charsets );
  mKeepReplyCharsetCheck->setChecked( !composer.readBoolEntry( "force-reply-charset", false ) );
}

void ComposerPage::CharsetTab::apply() {
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


QString ComposerPage::HeadersTab::title() {
  return i18n("H&eaders");
}

QString ComposerPage::HeadersTab::helpAnchor() const {
  return QString::fromLatin1("configure-composer-headers");
}

ComposerPageHeadersTab::ComposerPageHeadersTab( QWidget * parent, const char * name )
  : ConfigurationPage( parent, name )
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
}


void ComposerPage::HeadersTab::slotMimeHeaderValueChanged( const QString & text ) {
  // is called on ::setup(), when clearing the line edits. So be
  // prepared to not find a selection:
  QListViewItem * item = mTagList->selectedItem();
  if ( item )
    item->setText( 1, text );
}


void ComposerPage::HeadersTab::slotNewMimeHeader()
{
  QListViewItem *listItem = new QListViewItem( mTagList );
  mTagList->setCurrentItem( listItem );
  mTagList->setSelected( listItem, true );
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
}

void ComposerPage::HeadersTab::setup() {
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

void ComposerPage::HeadersTab::apply() {
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


// *************************************************************
// *                                                           *
// *                      SecurityPage                         *
// *                                                           *
// *************************************************************




QString SecurityPage::iconLabel() {
  return i18n("Security");
}

const char * SecurityPage::iconName() {
  return "encrypted";
}

QString SecurityPage::title() {
  return i18n("Security & Privacy Settings");
}

QString SecurityPage::helpAnchor() const {
  return QString::fromLatin1("configure-security");
}

SecurityPage::SecurityPage( QWidget * parent, const char * name )
  : TabbedConfigurationPage( parent, name )
{
  //
  // "General" tab:
  //
  mGeneralTab = new GeneralTab();
  addTab( mGeneralTab, mGeneralTab->title() );

  //
  // "PGP" tab:
  //
  mOpenPgpTab = new OpenPgpTab();
  addTab( mOpenPgpTab, mOpenPgpTab->title() );

  //
  // "CryptPlug" tab:
  //
  mCryptPlugTab = new CryptPlugTab();
  addTab( mCryptPlugTab, mCryptPlugTab->title() );
}

void SecurityPage::setup() {
  mGeneralTab->setup();
  mOpenPgpTab->setup();
  mCryptPlugTab->setup();
}

void SecurityPage::installProfile( KConfig * profile ) {
  mGeneralTab->installProfile( profile );
  mOpenPgpTab->installProfile( profile );
}

void SecurityPage::apply() {
  mGeneralTab->apply();
  mOpenPgpTab->apply();
  mCryptPlugTab->apply();
}

QString SecurityPage::GeneralTab::title() {
  return i18n("&General");
}

QString SecurityPage::GeneralTab::helpAnchor() const {
  return QString::fromLatin1("configure-security-general");
}

SecurityPageGeneralTab::SecurityPageGeneralTab( QWidget * parent, const char * name )
  : ConfigurationPage ( parent, name )
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

  QString confirmationWhatsThis = i18n( "<qt><p>This option enables the "
              "<em>unconditional</em> sending of delivery- and read confirmations "
	      "(&quot;receipts&quot;).</p>"
	      "<p>Returning these confirmations (so-called <em>receipts</em>) "
	      "makes it easy for the sender to track whether and - more "
	      "importantly - <em>when</em> you read his/her message.</p>"
	      "<p>You can return <em>delivery</em> confirmations in a "
	      "fine-grained manner using the &quot;confirm delivery&quot; filter "
	      "action. We advise against issuing <em>read</em> confirmations "
	      "at all.</p></qt>");

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
  mExternalReferences = new QCheckBox( i18n("Allow messages to load e&xternal "
					    "references from the Internet" ), group );
  QWhatsThis::add( mExternalReferences, externalWhatsThis );
  label = new KActiveLabel( i18n("<b>WARNING:</b> Allowing HTML in email may "
			   "increase the risk that your system will be "
			   "compromised by present and anticipated security "
			   "exploits. <a href=\"whatsthis:%1\">More about "
			   "HTML mails...</a> <a href=\"whatsthis:%2\">More "
			   "about external references...</a>")
			   .arg(htmlWhatsThis).arg(externalWhatsThis),
			   group );

  vlay->addWidget( group );

  // "Delivery Confirmations" group box:
  group = new QVGroupBox( i18n( "Delivery Confirmations" ), this );
  group->layout()->setSpacing( KDialog::spacingHint() );

  mSendReceivedReceiptCheck = new QCheckBox( i18n("Automatically &send delivery confirmations"), group );
  QWhatsThis::add( mSendReceivedReceiptCheck, confirmationWhatsThis );

  label = new KActiveLabel( i18n( "<b>WARNING:</b> Unconditionally returning "
			    "confirmations undermines your privacy. "
			    "<a href=\"whatsthis:%1\">More...</a>")
			      .arg(confirmationWhatsThis),
			    group );

  vlay->addWidget( group );

  // "Message Disposition Notification" groupbox:
  group = new QVGroupBox( i18n("Message Disposition Notifications"), this );
  group->layout()->setSpacing( KDialog::spacingHint() );


  // "ignore", "ask", "deny", "always send" radiobutton line:
  mMDNGroup = new QButtonGroup( group );
  mMDNGroup->hide();

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

  // Warning label:
  label = new KActiveLabel( i18n("<b>WARNING:</b> Unconditionally returning "
			   "confirmations undermines your privacy. "
			   "<a href=\"whatsthis:%1\">More...</a>")
			     .arg(receiptWhatsThis),
			   group );

  vlay->addWidget( group );
  vlay->addStretch( 10 ); // spacer
}

void SecurityPage::GeneralTab::setup() {
  KConfigGroup general( KMKernel::config(), "General" );
  KConfigGroup reader( KMKernel::config(), "Reader" );

  KConfigGroup mdn( KMKernel::config(), "MDN" );

  mHtmlMailCheck->setChecked( reader.readBoolEntry( "htmlMail", false ) );
  mExternalReferences->setChecked( reader.readBoolEntry( "htmlLoadExternal", false ) );
  mSendReceivedReceiptCheck->setChecked( general.readBoolEntry( "send-receipts", false ) );
  int num = mdn.readNumEntry( "default-policy", 0 );
  if ( num < 0 || num >= mMDNGroup->count() ) num = 0;
  mMDNGroup->setButton( num );
  num = mdn.readNumEntry( "quote-message", 0 );
  if ( num < 0 || num >= mOrigQuoteGroup->count() ) num = 0;
  mOrigQuoteGroup->setButton( num );
}

void SecurityPage::GeneralTab::installProfile( KConfig * profile ) {
  KConfigGroup general( profile, "General" );
  KConfigGroup reader( profile, "Reader" );
  KConfigGroup mdn( profile, "MDN" );

  if ( reader.hasKey( "htmlMail" ) )
    mHtmlMailCheck->setChecked( reader.readBoolEntry( "htmlMail" ) );
  if ( reader.hasKey( "htmlLoadExternal" ) )
    mExternalReferences->setChecked( reader.readBoolEntry( "htmlLoadExternal" ) );
  if ( general.hasKey( "send-receipts" ) )
      mSendReceivedReceiptCheck->setChecked( general.readBoolEntry( "send-receipts" ) );
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
}

void SecurityPage::GeneralTab::apply() {
  KConfigGroup general( KMKernel::config(), "General" );
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
      kernel->folderMgr()->createFolderList(&names, &folders);
      kernel->imapFolderMgr()->createFolderList(&names, &folders);
      kernel->searchFolderMgr()->createFolderList(&names, &folders);
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
  general.writeEntry( "send-receipts", mSendReceivedReceiptCheck->isChecked() );
  mdn.writeEntry( "default-policy", mMDNGroup->id( mMDNGroup->selected() ) );
  mdn.writeEntry( "quote-message", mOrigQuoteGroup->id( mOrigQuoteGroup->selected() ) );
}


QString SecurityPage::OpenPgpTab::title() {
  return i18n("Open&PGP");
}

QString SecurityPage::OpenPgpTab::helpAnchor() const {
  return QString::fromLatin1("configure-security-pgp");
}

SecurityPageOpenPgpTab::SecurityPageOpenPgpTab( QWidget * parent, const char * name )
  : ConfigurationPage ( parent, name )
{
  // tmp. vars:
  QVBoxLayout *vlay;
  QGroupBox   *group;
  QString     msg;

  vlay = new QVBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint() );

  // Generic OpenPGP configuration
  mPgpConfig = new Kpgp::Config( this );
  group = mPgpConfig->optionsGroupBox();

  // Add some custom OpenPGP options to the Options group box of Kpgp::Config
  mPgpAutoSignatureCheck =
    new QCheckBox( i18n("Automatically s&ign messages using OpenPGP"),
                   group );

  mPgpAutoEncryptCheck =
    new QCheckBox( i18n("Automatically encrypt messages &whenever possible"),
                   group );

  vlay->addWidget( mPgpConfig );

  vlay->addStretch( 10 ); // spacer

  // and now: adding QWhat'sThis all over the place:
  msg = i18n( "<qt><p>When this option is enabled, all messages you send "
              "will be signed by default. Of course it's still possible to "
              "disable signing for each message individually.</p></qt>" );
  QWhatsThis::add( mPgpAutoSignatureCheck, msg );

  msg = i18n( "<qt><p>When this option is enabled, every message you send "
              "will be encrypted whenever encryption is possible and desired. "
              "Of course it's still possible to disable the automatic "
              "encryption for each message individually.</p></qt>" );
  QWhatsThis::add( mPgpAutoEncryptCheck, msg );
}

void SecurityPage::OpenPgpTab::setup() {
  KConfigGroup composer( KMKernel::config(), "Composer" );

  mPgpConfig->setValues();
  mPgpAutoSignatureCheck->setChecked( composer.readBoolEntry( "pgp-auto-sign", false ) );
  mPgpAutoEncryptCheck->setChecked( composer.readBoolEntry( "pgp-auto-encrypt", false ) );
}

void SecurityPage::OpenPgpTab::installProfile( KConfig * profile ) {
  KConfigGroup composer( profile, "Composer" );

  if ( composer.hasKey( "pgp-auto-sign" ) )
    mPgpAutoSignatureCheck->setChecked( composer.readBoolEntry( "pgp-auto-sign" ) );
  if ( composer.hasKey( "pgp-auto-encrypt" ) )
    mPgpAutoEncryptCheck->setChecked( composer.readBoolEntry( "pgp-auto-encrypt" ) );
}

void SecurityPage::OpenPgpTab::apply() {
  KConfigGroup composer( KMKernel::config(), "Composer" );

  mPgpConfig->applySettings();
  composer.writeEntry( "pgp-auto-sign", mPgpAutoSignatureCheck->isChecked() );
  composer.writeEntry( "pgp-auto-encrypt", mPgpAutoEncryptCheck->isChecked() );
}


// *************************************************************
// *                                                           *
// *                       FolderPage                          *
// *                                                           *
// *************************************************************


QString FolderPage::iconLabel() {
  return i18n("Folders");
}

const char * FolderPage::iconName() {
  return "folder";
}

QString FolderPage::title() {
  return i18n("Settings for Folders");
}

QString FolderPage::helpAnchor() const {
  return QString::fromLatin1("configure-misc-folders");
}

FolderPage::FolderPage( QWidget * parent, const char * name )
  : ConfigurationPage( parent, name )
{
  // temp. vars:
  QVBoxLayout *vlay;
  QHBoxLayout *hlay;
  QGroupBox   *group;
  QLabel      *label;

  vlay = new QVBoxLayout( this, 0, KDialog::spacingHint() );

  // "confirm before emptying folder" check box: stretch 0
  mEmptyFolderConfirmCheck =
    new QCheckBox( i18n("Corresponds to Folder->Move All Messages to Trash",
                        "Ask for co&nfirmation before moving all messages to "
                        "trash"),
                   this );
  vlay->addWidget( mEmptyFolderConfirmCheck );
  mWarnBeforeExpire =
    new QCheckBox( i18n("&Warn before expiring messages"), this );
  vlay->addWidget( mWarnBeforeExpire );
  // "when trying to find unread messages" combo + label: stretch 0
  hlay = new QHBoxLayout( vlay ); // inherits spacing
  mLoopOnGotoUnread = new QComboBox( false, this );
  label = new QLabel( mLoopOnGotoUnread,
           i18n("to be continued with \"don't loop\", \"loop in current folder\", "
                "and \"loop in all folders\".",
                "When trying to find unread messages:"), this );
  mLoopOnGotoUnread->insertStringList( QStringList()
      << i18n("continuation of \"When trying to find unread messages:\"",
              "Don't loop")
      << i18n("continuation of \"When trying to find unread messages:\"",
              "Loop in current folder")
      << i18n("continuation of \"When trying to find unread messages:\"",
              "Loop in all folders"));
  hlay->addWidget( label );
  hlay->addWidget( mLoopOnGotoUnread, 1 );
  mJumpToUnread =
    new QCheckBox( i18n("&Jump to first unread message when entering a "
			"folder"), this );
  vlay->addWidget( mJumpToUnread );

  hlay = new QHBoxLayout( vlay ); // inherits spacing
  mDelayedMarkAsRead = new QCheckBox( i18n("Mar&k selected message as read after"), this );
  hlay->addWidget( mDelayedMarkAsRead );
  mDelayedMarkTime = new KIntSpinBox( 0 /*min*/, 60 /*max*/, 1/*step*/,
				      0 /*init*/, 10 /*base*/, this);
  mDelayedMarkTime->setSuffix( i18n(" sec") );
  mDelayedMarkTime->setEnabled( false ); // since mDelayedMarkAsREad is off
  hlay->addWidget( mDelayedMarkTime );
  hlay->addStretch( 1 );

  connect(mDelayedMarkAsRead, SIGNAL(toggled(bool)), mDelayedMarkTime,
  	SLOT(setEnabled(bool)));

  // "show popup after Drag'n'Drop" checkbox: stretch 0
  mShowPopupAfterDnD =
    new QCheckBox( i18n("Ask for action after &dragging messages to another folder"), this );
  vlay->addWidget( mShowPopupAfterDnD );

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

  // "On startup..." option:
  hlay = new QHBoxLayout( vlay ); // inherits spacing
  mOnStartupOpenFolder = new KMFolderComboBox( this );
  label = new QLabel( mOnStartupOpenFolder,
                      i18n("Open this folder on startup:"), this );
  hlay->addWidget( label );
  hlay->addWidget( mOnStartupOpenFolder, 1 );

  // "On exit..." groupbox:
  group = new QVGroupBox( i18n("On Program Exit, "
			       "Perform Following Tasks"), this );
  group->layout()->setSpacing( KDialog::spacingHint() );
  mCompactOnExitCheck = new QCheckBox( i18n("Com&pact all folders"), group );
  mEmptyTrashCheck = new QCheckBox( i18n("Empty &trash"), group );
  mExpireAtExit = new QCheckBox( i18n("&Expire old messages"), group );

  vlay->addWidget( group );
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

  msg = i18n( "what's this help",
	    "<qt><p>When jumping to the next unread message, it may occur "
	    "that no more unread messages are below the current message.</p>"
	    "<p><b>Don't loop:</b> The search will stop at the last message in "
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

void FolderPage::setup() {
  KConfigGroup general( KMKernel::config(), "General" );
  KConfigGroup behaviour( KMKernel::config(), "Behaviour" );

  mEmptyTrashCheck->setChecked( general.readBoolEntry( "empty-trash-on-exit", false ) );
  mExpireAtExit->setChecked( general.readNumEntry( "when-to-expire", 0 ) ); // set if non-zero
  mWarnBeforeExpire->setChecked( general.readBoolEntry( "warn-before-expire", true ) );
  mOnStartupOpenFolder->setFolder( general.readEntry( "startupFolder",
						  kernel->inboxFolder()->idString() ) );
  mCompactOnExitCheck->setChecked( general.readBoolEntry( "compact-all-on-exit", true ) );
  mEmptyFolderConfirmCheck->setChecked( general.readBoolEntry( "confirm-before-empty", true ) );
  // default = "Loop in current folder"
  mLoopOnGotoUnread->setCurrentItem( behaviour.readNumEntry( "LoopOnGotoUnread", 1 ) );
  mJumpToUnread->setChecked( behaviour.readBoolEntry( "JumpToUnread", false ) );
  mDelayedMarkAsRead->setChecked( behaviour.readBoolEntry( "DelayedMarkAsRead", true ) );
  mDelayedMarkTime->setValue( behaviour.readNumEntry( "DelayedMarkTime", 0 ) );
  mShowPopupAfterDnD->setChecked( behaviour.readBoolEntry( "ShowPopupAfterDnD", true ) );

  int num = general.readNumEntry("default-mailbox-format", 1 );
  if ( num < 0 || num > 1 ) num = 1;
  mMailboxPrefCombo->setCurrentItem( num );
}

void FolderPage::apply() {
  KConfigGroup general( KMKernel::config(), "General" );
  KConfigGroup behaviour( KMKernel::config(), "Behaviour" );

  general.writeEntry( "empty-trash-on-exit", mEmptyTrashCheck->isChecked() );
  general.writeEntry( "compact-all-on-exit", mCompactOnExitCheck->isChecked() );
  general.writeEntry( "confirm-before-empty", mEmptyFolderConfirmCheck->isChecked() );
  general.writeEntry( "default-mailbox-format", mMailboxPrefCombo->currentItem() );
  general.writeEntry( "warn-before-expire", mWarnBeforeExpire->isChecked() );
  general.writeEntry( "startupFolder", mOnStartupOpenFolder->getFolder() ?
				  mOnStartupOpenFolder->getFolder()->idString() : QString::null );
  behaviour.writeEntry( "LoopOnGotoUnread", mLoopOnGotoUnread->currentItem() );
  behaviour.writeEntry( "JumpToUnread", mJumpToUnread->isChecked() );
  behaviour.writeEntry( "DelayedMarkAsRead", mDelayedMarkAsRead->isChecked() );
  behaviour.writeEntry( "DelayedMarkTime", mDelayedMarkTime->value() );
  behaviour.writeEntry( "ShowPopupAfterDnD", mShowPopupAfterDnD->isChecked() );

  if ( mExpireAtExit->isChecked() )
    general.writeEntry( "when-to-expire", expireAtExit );
  else
    general.writeEntry( "when-to-expire", expireManual );
}

QString SecurityPage::CryptPlugTab::title() {
  return i18n("Crypto Plugi&ns");
}

QString SecurityPage::CryptPlugTab::helpAnchor() const {
  return QString::null;
}

SecurityPageCryptPlugTab::SecurityPageCryptPlugTab( QWidget * parent, const char * name )
  : ConfigurationPage( parent, name )
{
  QHBoxLayout * hlay = new QHBoxLayout( this, KDialog::marginHint(),
					KDialog::spacingHint() );

  //
  // 1. column: listbox, edit fields:
  //
  int row = -1;
  QGridLayout * glay = new QGridLayout( hlay, 4, 2 ); // inherits spacing
  glay->setRowStretch( 0, 1 );
  glay->setColStretch( 1, 1 );

  // listview:
  ++row;
  mPlugList = new KListView( this, "mPlugList" );
  mPlugList->addColumn( i18n("Name") );
  mPlugList->addColumn( i18n("Location") );
  mPlugList->addColumn( i18n("Update URL") );
  mPlugList->addColumn( i18n("Active") );
  mPlugList->addColumn( i18n("Initialized" ) );
  mPlugList->setAllColumnsShowFocus( true );
  mPlugList->setSorting( -1 ); // disabled
  mPlugList->header()->setClickEnabled( false );
  mPlugList->setFullWidth( true );
  glay->addMultiCellWidget( mPlugList, row, row, 0, 1 );

  connect( mPlugList, SIGNAL(selectionChanged()),
           SLOT(slotPlugSelectionChanged()) );

  // "name" line edit and label:
  ++row;
  mNameEdit = new KLineEdit( this );
  mNameEdit->setEnabled( false ); // since no item is selected in mPlugList
  glay->addWidget( new QLabel( mNameEdit, i18n("Na&me:"), this ), row, 0 );
  glay->addWidget( mNameEdit, row, 1 );

  connect( mNameEdit, SIGNAL(textChanged(const QString&)),
	   SLOT(slotPlugNameChanged(const QString&)) );

  // "Location" line edit and label:
  ++row;
  mLocationRequester = new KURLRequester( this );
  mLocationRequester->setEnabled( false ); // since no item sel'd in mPlugList
  glay->addWidget( new QLabel( mLocationRequester, i18n("&Location:"), this ), row, 0 );
  glay->addWidget( mLocationRequester, row, 1 );

  connect( mLocationRequester, SIGNAL(textChanged(const QString&)),
	   SLOT(slotPlugLocationChanged(const QString&)) );

  // "Update URL" line edit and label:
  ++row;
  mUpdateURLEdit = new KLineEdit( this );
  mUpdateURLEdit->setEnabled( false ); // since no item is sel'd in mPlugList
  glay->addWidget( new QLabel( mUpdateURLEdit, i18n("&Update URL:"), this ), row, 0 );
  glay->addWidget( mUpdateURLEdit, row, 1 );

  connect( mUpdateURLEdit, SIGNAL(textChanged(const QString&)),
	   SLOT(slotPlugUpdateURLChanged(const QString&)) );

  //
  // 2. column: action buttons
  //
  QVBoxLayout * vlay = new QVBoxLayout( hlay ); // inherits spacing

  // "New" button:
  mNewButton = new QPushButton( i18n("&New"), this );
  mNewButton->setAutoDefault( false );
  vlay->addWidget( mNewButton );

  connect( mNewButton, SIGNAL(clicked()),
	   SLOT(slotNewPlugIn()) );

  // "Remove' button:
  mRemoveButton = new QPushButton( i18n("&Remove"), this );
  mRemoveButton->setAutoDefault( false );
  vlay->addWidget( mRemoveButton );

  connect( mRemoveButton, SIGNAL(clicked()),
	   SLOT(slotDeletePlugIn()) );

  // "Activete" / "Deactivate" button:
  mActivateButton = new QPushButton( i18n("Ac&tivate"), this );
  mActivateButton->setAutoDefault( false );
  vlay->addWidget( mActivateButton );

  connect( mActivateButton, SIGNAL(clicked()),
	   SLOT(slotActivatePlugIn()) );

  // "Configure..." button:
  mConfigureButton = new QPushButton( i18n("Confi&gure..."), this );
  mConfigureButton->setAutoDefault( false );
  vlay->addWidget( mConfigureButton );

  connect( mConfigureButton, SIGNAL(clicked()),
	   SLOT(slotConfigurePlugIn()) );

  vlay->addStretch( 1 );
}

void SecurityPage::CryptPlugTab::setup()
{
  kdDebug(5006) << "CryptPlugTab::setup(): found "
	    << kernel->cryptPlugList()->count()
	    << " CryptPlugWrappers." << endl;
  mPlugList->clear();

  // populate the plugin list:
  int i = 0;
  QListViewItem * top = 0;
  for ( CryptPlugWrapperListIterator it( *(kernel->cryptPlugList()) ) ;
	it.current() ; ++it, ++i ) {
    kdDebug(5006) << "processing { \"" << (*it)->displayName()
	      << "\", \"" << (*it)->libName()
	      << "\", " << (*it)->active() << " }" << endl;
    if( !(*it)->displayName().isEmpty() ) {
      top = new QListViewItem( mPlugList, top,
			       (*it)->displayName(),
			       (*it)->libName(),
			       (*it)->updateURL(),
			       (*it)->active() ? "*" : "",
			       ( (*it)->initStatus( 0 ) == CryptPlugWrapper::InitStatus_Ok ) ? "*" : "" );
    }
  }

  if( mPlugList->childCount() > 0 ) {
    mPlugList->setCurrentItem( mPlugList->firstChild() );
    mPlugList->setSelected( mPlugList->firstChild(), true );
  }

  slotPlugSelectionChanged();
}

void SecurityPage::CryptPlugTab::slotPlugSelectionChanged() {
  QListViewItem * item = mPlugList->selectedItem();

  // enable/disable action buttons...
  mRemoveButton->setEnabled( item );
  mActivateButton->setEnabled( item );
  mConfigureButton->setEnabled( item );
  // ...and line edits:
  mNameEdit->setEnabled( item );
  mLocationRequester->setEnabled( item );
  mUpdateURLEdit->setEnabled( item );

  // set text of activate button:
  mActivateButton->setText( item && item->text( 3 ).isEmpty() ?
				    i18n("Ac&tivate") : i18n("Deac&tivate") );

  // fill/clear edit fields
  if ( item ) { // fill
    mNameEdit->setText( item->text( 0 ) );
    mLocationRequester->setURL( item->text( 1 ) );
    mUpdateURLEdit->setText( item->text( 2 ) );
  } else { // clear
    mNameEdit->clear();
    mLocationRequester->clear();
    mUpdateURLEdit->clear();
  }
}

void SecurityPage::CryptPlugTab::apply() {
  KConfigGroup general( KMKernel::config(), "General" );

  CryptPlugWrapperList * cpl = kernel->cryptPlugList();

  uint cryptPlugCount = 0;
  for ( QListViewItemIterator it( mPlugList ) ; it.current() ; ++it ) {
    if ( it.current()->text( 0 ).isEmpty() )
      continue;
    KConfigGroup config( KMKernel::config(),
                         QString("CryptPlug #%1").arg( cryptPlugCount ) );
    config.writeEntry( "name", it.current()->text( 0 ) );
    config.writePathEntry( "location", it.current()->text( 1 ) );
    config.writeEntry( "updates", it.current()->text( 2 ) );
    config.writeEntry( "active", !it.current()->text( 3 ).isEmpty() );

    CryptPlugWrapper * wrapper = cpl->at( cryptPlugCount );
    if ( wrapper ) {
      wrapper->setDisplayName( it.current()->text( 0 ) );
      if ( wrapper->libName() != it.current()->text( 1 ) ) {
	wrapper->deinitialize();
	wrapper->setLibName( it.current()->text( 1 ) );
	CryptPlugWrapper::InitStatus status;
	QString errorMsg;
	wrapper->initialize( &status, &errorMsg );
	if( CryptPlugWrapper::InitStatus_Ok != status )
	  it.current()->setText( 4, QString::null );
	else
	  it.current()->setText( 4, "*" );
      }
      wrapper->setUpdateURL( it.current()->text( 2 ) );
      wrapper->setActive( !it.current()->text( 3 ).isEmpty() );
    }
    ++cryptPlugCount; // only counts _taken_ plugins!
  }

  // remove surplus items from the crypt plug list
  while( cpl->count() > cryptPlugCount ) {
    CryptPlugWrapper* wrapper = cpl->take( cryptPlugCount );
    if( wrapper ) {
      QString dummy;
      if ( wrapper->initStatus( &dummy ) == CryptPlugWrapper::InitStatus_Ok ) {
	wrapper->deinitialize();
      }
    }
    delete wrapper;
    wrapper = 0;
  }

  general.writeEntry("crypt-plug-count", cryptPlugCount );
}

void SecurityPage::CryptPlugTab::slotPlugNameChanged( const QString & text )
{
  QListViewItem * item = mPlugList->selectedItem();
  if ( !item ) return;

  item->setText( 0, text );
}

void SecurityPage::CryptPlugTab::slotPlugLocationChanged( const QString & text )
{
  QListViewItem * item = mPlugList->selectedItem();
  if ( !item ) return;

  item->setText( 1, text );
}

void SecurityPage::CryptPlugTab::slotPlugUpdateURLChanged( const QString & text )
{
  QListViewItem * item = mPlugList->selectedItem();
  if ( !item ) return;

  item->setText( 2, text );
}

void SecurityPage::CryptPlugTab::slotNewPlugIn()
{
  CryptPlugWrapper * newWrapper = new CryptPlugWrapper( this, "", "", "" );
  kernel->cryptPlugList()->append( newWrapper );

  QListViewItem * item = new QListViewItem( mPlugList, mPlugList->lastItem() );
  mPlugList->setCurrentItem( item );
  mPlugList->setSelected( item, true );
  mNameEdit->setText( i18n("Unnamed") );
  mNameEdit->selectAll();
  mNameEdit->setFocus();

  //slotPlugSelectionChanged();// ### ???
}

void SecurityPage::CryptPlugTab::slotDeletePlugIn()
{
  QListViewItem * item = mPlugList->selectedItem();
  if ( !item ) return;

  delete item;
  mPlugList->setSelected( mPlugList->currentItem(), true );
}

void SecurityPage::CryptPlugTab::slotConfigurePlugIn() {
  CryptPlugWrapperList * cpl = kernel->cryptPlugList();
  int i = 0;
  for ( QListViewItemIterator it( mPlugList ) ; it.current() ; ++it, ++i ) {
    if ( it.current()->isSelected() ) {
      CryptPlugWrapper * wrapper = cpl->at( i );
      if ( wrapper ) {
	CryptPlugConfigDialog dialog( wrapper, i, i18n("Configure %1 Plugin").arg( it.current()->text( 0 ) ) );
	dialog.exec();
      }
      break;
    }
  }
}

void SecurityPage::CryptPlugTab::slotActivatePlugIn()
{
  QListViewItem * item = mPlugList->selectedItem();
  if ( !item ) return;

  CryptPlugWrapperList * cpl = kernel->cryptPlugList();

  // find out whether the plug-in is to be activated or de-activated
  bool activate = ( item->text( 3 ).isEmpty() );
  // (De)activate this plug-in
  // and deactivate all other plugins if necessarry
  int pos = 0;
  for ( QListViewItemIterator it( mPlugList ) ; it.current() ; ++it, ++pos ) {
    CryptPlugWrapper * plug = cpl->at( pos );
    if( plug ) {
      if( it.current()->isSelected() ) {
	// This is the one the user wants to (de)activate
	plug->setActive( activate );
	it.current()->setText( 3, activate ? "*" : "" );
      } else {
	// This is one of the other entries
	plug->setActive( false );
	it.current()->setText( 3, QString::null );
      }
    }
  }
  if( activate )
    mActivateButton->setText( i18n("Deac&tivate")  );
  else
    mActivateButton->setText( i18n("Ac&tivate") );
}


// *************************************************************
// *                                                           *
// *                      GroupwarePage                        *
// *                                                           *
// *************************************************************

QString GroupwarePage::iconLabel() {
  return i18n("Groupware");
}

QString GroupwarePage::title() {
  return i18n("Configure Groupware");
}

const char * GroupwarePage::iconName() {
  return "groupware";
}

QString GroupwarePage::helpAnchor() const {
  return QString::fromLatin1("configure-groupware");
}

GroupwarePage::GroupwarePage( QWidget * parent, const char * name )
    : ConfigurationPage( parent, name )
{
  QBoxLayout* vlay = new QVBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint() );

  mEnableGwCB = new QCheckBox( i18n("&Enable groupware functionality"), this );
  vlay->addWidget( mEnableGwCB );

  mEnableImapResCB = new QCheckBox( i18n("&Enable IMAP Resource functionality"), this );
  vlay->addWidget( mEnableImapResCB );

  mBox = new QVGroupBox( i18n("&IMAP Resource Folder Options"), this );
  vlay->addWidget(mBox);

  connect( mEnableImapResCB, SIGNAL( toggled(bool) ),
	   mBox, SLOT( setEnabled(bool) ) );

  QLabel* languageLA = new QLabel( i18n("&Language for groupware folders:"), mBox );

  mLanguageCombo = new QComboBox( false, mBox );
  languageLA->setBuddy( mLanguageCombo );

  QStringList lst;
  lst << i18n("English") << i18n("German") << i18n("French") << i18n("Dutch");
  mLanguageCombo->insertStringList( lst );

  QLabel* subfolderLA = new QLabel( i18n("Resource folders are &subfolders of:"), mBox );

  mFolderCombo = new KMFolderComboBox( mBox );
  subfolderLA->setBuddy( mFolderCombo );

  QVGroupBox* resourceVGB = new QVGroupBox( i18n( "Automatic &Resource Management" ), this );
  vlay->addWidget( resourceVGB );
  resourceVGB->setEnabled( false ); // since !mEnableGwCB->isChecked()
  connect( mEnableGwCB, SIGNAL( toggled(bool) ),
           resourceVGB, SLOT( setEnabled(bool) ) );

  mAutoResCB = new QCheckBox( i18n( "&Automatically accept resource requests" ), resourceVGB );
  mAutoDeclConflCB = new QCheckBox( i18n( "A&utomatically decline conflicting requests" ), resourceVGB );
  mAutoDeclConflCB->setEnabled( false );
  connect( mAutoResCB, SIGNAL( toggled( bool ) ),
           mAutoDeclConflCB, SLOT( setEnabled( bool ) ) );

  mLegacyMangleFromTo = new QCheckBox( i18n( "Legac&y mode: Mangle From:/To: headers in replies to invitations" ), mBox );
  QToolTip::add( mLegacyMangleFromTo, i18n( "Turn this option on in order to make Outlook(tm) understand your answers to invitations" ) );

  QLabel* dummy = new QLabel( this );
  vlay->addWidget( dummy, 2 );
}

void GroupwarePage::setup()
{
  // Read the groupware config
  KConfigGroup options( KMKernel::config(), "Groupware" );
  mEnableGwCB->setChecked( options.readBoolEntry( "Enabled", true ) );
  mAutoResCB->setChecked( options.readBoolEntry( "AutoAccept", false ) );
  mAutoDeclConflCB->setChecked( options.readBoolEntry( "AutoDeclConflict", false ) );
  mLegacyMangleFromTo->setChecked( options.readBoolEntry( "LegacyMangleFromToHeaders", false ) );

  // Read the IMAP resource config
  KConfigGroup irOptions( KMKernel::config(), "IMAP Resource" );
  mEnableImapResCB->setChecked( irOptions.readBoolEntry( "Enabled", false ) );
  mBox->setEnabled( mEnableImapResCB->isChecked() );

  int i = irOptions.readNumEntry( "Folder Language", 0 );
  mLanguageCombo->setCurrentItem(i);

  QString folderId = irOptions.readEntry( "Folder Parent" );
  if( !folderId.isNull() ) {
    mFolderCombo->setFolder( folderId );
  }
}

void GroupwarePage::installProfile( KConfig * /*profile*/ )
{
  kdDebug(5006) << "NYI: void GroupwarePage::installProfile( KConfig * /*profile*/ )" << endl;
}

void GroupwarePage::apply()
{
  // Write the groupware config
  KConfigGroup options( KMKernel::config(), "Groupware" );
  options.writeEntry( "Enabled", mEnableGwCB->isChecked() );
  if ( mEnableGwCB->isChecked() ) {
    options.writeEntry( "AutoAccept", mAutoResCB->isChecked() );
    options.writeEntry( "AutoDeclConflict", mAutoDeclConflCB->isChecked() );
    options.writeEntry( "LegacyMangleFromToHeaders", mLegacyMangleFromTo->isChecked() );
  }

  // Write the IMAP resource config
  KConfigGroup irOptions( KMKernel::config(), "IMAP Resource" );
  irOptions.writeEntry( "Enabled", mEnableImapResCB->isChecked() );
  if ( mEnableImapResCB->isChecked() ) {
    options.writeEntry( "Folder Language", mLanguageCombo->currentItem() );
    options.writeEntry( "Folder Parent", mFolderCombo->getFolder()->idString() );
  }

  // Make the groupware options read the config settings
  kernel->groupware().readConfig();
}



//----------------------------
#include "configuredialog.moc"
