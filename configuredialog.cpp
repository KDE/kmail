/*
 *   kmail: KDE mail client
 *   This file: Copyright (C) 2000 Espen Sand, espen@kde.org
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
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

// Add header files alphabetically

// This must be first
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <signal.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <qcheckbox.h>
#include <qtabwidget.h>
#include <qvbox.h>
#include <qwhatsthis.h>
#include <qwidgetstack.h>
#include <qvgroupbox.h>

#include <kapplication.h>
#include <kcharsets.h>
#include <kdebug.h>
#include <kemailsettings.h>
#include <kfiledialog.h>
#include <kfontdialog.h>
#include <klineeditdlg.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpgp.h>
#include <kpgpui.h>
#include <kstandarddirs.h>
#include <kurlrequester.h>
#include <kscoring.h>
#include <kseparator.h>

#include "accountdialog.h"
#include "colorlistbox.h"
#include "configuredialog.h"
#include "kbusyptr.h"
#include "kmacctmgr.h"
#include "kmacctseldlg.h"
#include "kmfolder.h"
#include "kmfoldermgr.h"
#include "kmheaders.h"
#include "kmidentity.h"
#include "kmsender.h"
#include "kmtopwidget.h"
#include "kmtransport.h"
#include "kmfoldercombobox.h"
#ifdef SCORING
#include "kmscoring.h"
#endif

#include "configuredialog.moc"

#include <stdlib.h>
#include <kiconloader.h>

#ifndef _PATH_SENDMAIL
#define _PATH_SENDMAIL  "/usr/sbin/sendmail"
#endif

ConfigureDialog::ApplicationLaunch::ApplicationLaunch( const QString &cmd )
{
  mCmdline = cmd;
}

void ConfigureDialog::ApplicationLaunch::doIt( void )
{
  // This isn't used anywhere else so
  // it should be safe to do this here.
  // I dont' see how we can cleanly wait
  // on all possible childs in this app so
  // I use this hack instead.  Another
  // alternative is to fork() twice, recursively,
  // but that is slower.
  signal(SIGCHLD, SIG_IGN);
  // FIXME use KShellProcess instead
  system(mCmdline.latin1());
}

void ConfigureDialog::ApplicationLaunch::run( void )
{
  signal(SIGCHLD, SIG_IGN);  // see comment above.
  if( fork() == 0 )
  {
    doIt();
    exit(0);
  }
}


ConfigureDialog::ListView::ListView( QWidget *parent, const char *name,
				     int visibleItem )
  : KListView( parent, name )
{
  setVisibleItem(visibleItem);
}


void ConfigureDialog::ListView::resizeEvent( QResizeEvent *e )
{
  KListView::resizeEvent(e);
  resizeColums();
}


void ConfigureDialog::ListView::showEvent( QShowEvent *e )
{
  KListView::showEvent(e);
  resizeColums();
}


void ConfigureDialog::ListView::resizeColums( void )
{
  int c = columns();
  if( c == 0 )
  {
    return;
  }

  int w1 = viewport()->width();
  int w2 = w1 / c;
  int w3 = w1 - (c-1)*w2;

  for( int i=0; i<c-1; i++ )
  {
    setColumnWidth( i, w2 );
  }
  setColumnWidth( c-1, w3 );
}


void ConfigureDialog::ListView::setVisibleItem( int visibleItem,
						bool updateSize )
{
  mVisibleItem = QMAX( 1, visibleItem );
  if( updateSize == true )
  {
    QSize s = sizeHint();
    setMinimumSize( s.width() + verticalScrollBar()->sizeHint().width() +
		    lineWidth() * 2, s.height() );
  }
}


QSize ConfigureDialog::ListView::sizeHint( void ) const
{
  QSize s = QListView::sizeHint();

  int h = fontMetrics().height() + 2*itemMargin();
  if( h % 2 > 0 ) { h++; }

  s.setHeight( h*mVisibleItem + lineWidth()*2 + header()->sizeHint().height());
  return( s );
}



NewLanguageDialog::NewLanguageDialog( QWidget *parent, const char *name,
                                      bool modal, LanguageItem *langList )
  :KDialogBase( parent, name, modal, i18n("New Language"), Ok|Cancel|Help, Ok,
                true )
{
  QFrame *page = makeMainWidget();
  QHBoxLayout *hlay = new QHBoxLayout( page, 0, spacingHint() );
  QLabel *label = new QLabel( i18n("Language:"), page );
  hlay->addWidget( label );
  mComboBox = new QComboBox( false, page );
  hlay->addWidget( mComboBox, 1 );
  QStringList langlist = KGlobal::dirs()->findAllResources( "locale",
                               "*/entry.desktop" );
  LanguageItem *l;
  for ( QStringList::ConstIterator it = langlist.begin();
    it != langlist.end(); ++it )
  {
    KSimpleConfig entry( *it );
    entry.setGroup( "KCM Locale" );
    QString name = entry.readEntry( "Name" );
    QString path = *it;
    int index = path.findRev('/');
    path = path.left(index);
    index = path.findRev('/');
    path = path.mid(index+1);
    l = langList;
    while (l && l->mLanguage != path) l = l->next;
    if (!l)
    {
      QString output = name + " (" + path + ")";
      QPixmap flag( locate("locale", path + "/flag.png") );
      mComboBox->insertItem( flag, output );
    }
  }
  if (mComboBox->count() == 0)
  {
    mComboBox->insertItem( i18n("No more languages available") );
    enableButtonOK( false );
  } else mComboBox->listBox()->sort();
}

QString NewLanguageDialog::language( void ) const
{
  QString s = QString( mComboBox->currentText() );
  int i = s.findRev( "(" );
  return( s.mid( i + 1, s.length() - i - 2 ) );
}

LanguageComboBox::LanguageComboBox( bool rw, QWidget *parent, const char *name )
  :QComboBox( rw, parent, name )
{
}

int LanguageComboBox::insertLanguage( const QString & language )
{
  KSimpleConfig entry( locate("locale", language + "/entry.desktop") );
  entry.setGroup( "KCM Locale" );
  QString name = entry.readEntry( "Name" );
  QString output = name + " (" + language + ")";
  insertItem( QPixmap( locate("locale", language + "/flag.png") ),
    output );
  listBox()->sort();
  return listBox()->index( listBox()->findItem(output) );
}

QString LanguageComboBox::language( void ) const
{
  QString s = QString( currentText() );
  int i = s.findRev( "(" );
  return( s.mid( i + 1, s.length() - i - 2 ) );
}

void LanguageComboBox::setLanguage( const QString & language )
{
  for (int i = 0; i < count(); i++)
    if (text(i).find(QString("(%1)").arg(language)) >= 0) setCurrentItem(i);
}

LanguageItem::LanguageItem( const QString& language, const QString& reply,
  const QString& replyAll, const QString& forward, const QString& indentPrefix )
{
  mLanguage = language;
  mReply = reply;
  mReplyAll = replyAll;
  mForward = forward;
  mIndentPrefix = indentPrefix;
}


ConfigureDialog::ConfigureDialog( QWidget *parent, const char *name,
				  bool modal )

  /* deactivated Default Button as it is not used
  :KDialogBase( IconList, i18n("Configure"), Help|Default|Apply|Ok|Cancel,
		Ok, parent, name, modal, true )
  */

  : KDialogBase( IconList, i18n("Configure"), Help|Apply|Ok|Cancel,
		 Ok, parent, name, modal, true )
{
  setHelp( "configure" ); // The documentation anchor of the configure dialog's help
  setIconListAllVisible( true );

  /* Button not used
  enableButton( Default, false );
  */
  connect( this, SIGNAL( cancelClicked() ), this, SLOT( slotCancelOrClose() ));
  connect( this, SIGNAL( closeClicked() ), this, SLOT( slotCancelOrClose() ));

  makeIdentityPage();
  makeNetworkPage();
  makeAppearancePage();
  makeComposerPage();
  makeSecurityPage();
  makeMiscPage();
}


ConfigureDialog::~ConfigureDialog( void )
{
}


void ConfigureDialog::show( void )
{
  if( isVisible() == false )
  {
    setup();
    //showPage(0); Perhaps best to remember the page?
  }
  KDialogBase::show();
}


void ConfigureDialog::makeIdentityPage( void )
{
  // temp. vars:
  QGridLayout *glay;
  QWidget     *page;
  QVBoxLayout *vlay;
  QHBoxLayout *hlay;

  QFrame *topLevel = addPage( i18n("Identity"), i18n("Personal information"),
    KGlobal::instance()->iconLoader()->loadIcon( "identity", KIcon::NoGroup,
    KIcon::SizeMedium ));
  mIdentity.pageIndex = pageIndex( topLevel );
  //
  // the identity selector:
  //
  glay = new QGridLayout( topLevel, 3, 2, spacingHint() );
  glay->setColStretch( 1, 1 );
  glay->setRowStretch( 2, 1 );

  // "Identity" combobox with label:
  mIdentity.identityCombo = new QComboBox( false, topLevel );
  glay->addWidget( new QLabel( mIdentity.identityCombo,
			       i18n("&Identity:"), topLevel ), 0, 0 );
  glay->addWidget( mIdentity.identityCombo, 0, 1 );
  connect( mIdentity.identityCombo, SIGNAL(activated(int)),
	   this, SLOT(slotIdentitySelectorChanged()) );

  // "new...", "rename...", "remove" buttons:
  hlay = new QHBoxLayout(); // inherits spacing from parent layout
  glay->addLayout( hlay, 1, 1 );

  QPushButton *newButton = new QPushButton( i18n("&New..."), topLevel );
  mIdentity.renameIdentityButton =
    new QPushButton( i18n("&Rename..."), topLevel);
  mIdentity.removeIdentityButton =
    new QPushButton( i18n("Re&move"), topLevel );
  newButton->setAutoDefault( false );
  mIdentity.renameIdentityButton->setAutoDefault( false );
  mIdentity.removeIdentityButton->setAutoDefault( false );
  connect( newButton, SIGNAL(clicked()),
	   this, SLOT(slotNewIdentity()) );
  connect( mIdentity.renameIdentityButton, SIGNAL(clicked()),
	   this, SLOT(slotRenameIdentity()) );
  connect( mIdentity.removeIdentityButton, SIGNAL(clicked()),
	   this, SLOT(slotRemoveIdentity()) );
  hlay->addWidget( newButton );
  hlay->addWidget( mIdentity.renameIdentityButton );
  hlay->addWidget( mIdentity.removeIdentityButton );
  
  //
  // Tab Widget: General
  //
  QTabWidget *tabWidget = new QTabWidget( topLevel, "config-identity-tab" );
  glay->addMultiCellWidget( tabWidget, 2, 2, 0, 1 );
  page = new QWidget( tabWidget );
  tabWidget->addTab( page, i18n("&General") );
  glay = new QGridLayout( page, 4, 2, spacingHint() );
  glay->setRowStretch( 3, 1 );
  glay->setColStretch( 1, 1 );

  // row 0: "Name" line edit and label:
  mIdentity.nameEdit = new QLineEdit( page );
  glay->addWidget( mIdentity.nameEdit, 0, 1 );
  glay->addWidget( new QLabel( mIdentity.nameEdit, i18n("&Name:"), page ), 0, 0 );

  // row 1: "Organization" line edit and label:
  mIdentity.organizationEdit = new QLineEdit( page );
  glay->addWidget( mIdentity.organizationEdit, 1, 1 );
  glay->addWidget( new QLabel( mIdentity.organizationEdit,
			       i18n("&Organization:"), page ), 1, 0 );

  // row 2: "Email Address" line edit and label:
  // (row 3: spacer)
  mIdentity.emailEdit = new QLineEdit( page );
  glay->addWidget( mIdentity.emailEdit, 2, 1 );
  glay->addWidget( new QLabel( mIdentity.emailEdit,
			       i18n("&Email Address:"), page ), 2, 0 );

  //
  // Tab Widget: Advanced
  //
  page = new QWidget( tabWidget );
  tabWidget->addTab( page, i18n("&Advanced") );
  glay = new QGridLayout( page, 5, 3, spacingHint() );
  glay->setRowStretch( 4, 1 );
  glay->setColStretch( 1, 1 );

  // row 0: "Reply-To Address" line edit and label:
  mIdentity.replytoEdit = new QLineEdit( page );
  glay->addMultiCellWidget( mIdentity.replytoEdit, 0, 0, 1, 2 );
  glay->addWidget( new QLabel( mIdentity.replytoEdit,
			       i18n("&Reply-To Address:"), page ), 0, 0 );

  // row 1: "PGP KeyID" requester and label:
  QPushButton *changeKey = new QPushButton( i18n("Change..."), page );
  changeKey->setAutoDefault( false );
  glay->addWidget( changeKey, 1, 2 );
  glay->addWidget( new QLabel( changeKey, i18n("OpenPGP &Key:"), page ), 1, 0 );
  mIdentity.pgpIdentityLabel = new QLabel( page );
  mIdentity.pgpIdentityLabel->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  glay->addWidget( mIdentity.pgpIdentityLabel, 1, 1 );
  connect( changeKey, SIGNAL(clicked()), 
           this, SLOT(slotChangeDefaultPGPKey()) );
  QWhatsThis::add(mIdentity.pgpIdentityLabel,
                  i18n("<qt><p>The PGP key you choose here will be used "
                       "to sign messages and to encrypt messages to "
                       "yourself.</p></qt>"));

  // row 2: "Sent-mail Folder" combo box and label:
  mIdentity.fccCombo = new KMFolderComboBox( page );
  mIdentity.fccCombo->showOutboxFolder( FALSE );
  glay->addMultiCellWidget( mIdentity.fccCombo, 2, 2, 1, 2 );
  glay->addWidget( new QLabel( mIdentity.fccCombo,
			       i18n("Sen&t-mail Folder:"), page ), 2, 0 );

  // row 3: "Special transport" combobox and label:
  // (row 4: spacer)
  mIdentity.transportCheck = new QCheckBox( i18n("Spe&cial transport:"), page );
  glay->addWidget( mIdentity.transportCheck, 3, 0 );
  mIdentity.transportCombo = new QComboBox( true, page );
  mIdentity.transportCombo->setEnabled( false );
  glay->addMultiCellWidget( mIdentity.transportCombo, 3, 3, 1, 2 );
  connect( mIdentity.transportCheck, SIGNAL(toggled(bool)),
	   mIdentity.transportCombo, SLOT(setEnabled(bool)) );

  //
  // Tab Widget: Signature
  //
  page = new QWidget( tabWidget );
  tabWidget->addTab( page, i18n("&Signature") );
  vlay = new QVBoxLayout( page, spacingHint() );

  // "enable signatue" checkbox:
  mIdentity.signatureEnabled = new QCheckBox( i18n("&Enable Signature"), page );
  vlay->addWidget( mIdentity.signatureEnabled );

  // "obtain signature text from" combo and label:
  hlay = new QHBoxLayout( vlay ); // inherits spacing
  mIdentity.signatureSourceCombo = new QComboBox( false, page );
  mIdentity.signatureSourceCombo->setEnabled( false );
  mIdentity.signatureSourceCombo->insertStringList(
         QStringList() << i18n("file")
		       << i18n("output of command")
		       << i18n("input field below") );
  QLabel* label = new QLabel( mIdentity.signatureSourceCombo,
			      i18n("&Obtain signature text from"), page );
  label->setEnabled( false );
  hlay->addWidget( label );
  hlay->addWidget( mIdentity.signatureSourceCombo, 1 );

  // widget stack that is controlled by the source combo:
  QWidgetStack *widgetStack = new QWidgetStack( page );
  widgetStack->setEnabled( false );
  vlay->addWidget( widgetStack, 1 );
  connect( mIdentity.signatureSourceCombo, SIGNAL(highlighted(int)),
	   widgetStack, SLOT(raiseWidget(int)) );
  // connects for the enabling of the widgets depending on
  // signatureEnabled:
  connect( mIdentity.signatureEnabled, SIGNAL(toggled(bool)),
	   mIdentity.signatureSourceCombo, SLOT(setEnabled(bool)) );
  connect( mIdentity.signatureEnabled, SIGNAL(toggled(bool)),
	   widgetStack, SLOT(setEnabled(bool)) );
  connect( mIdentity.signatureEnabled, SIGNAL(toggled(bool)),
	   label, SLOT(setEnabled(bool)) );

  // page 1: "signature file" requester, label, "edit file" button:
  page = new QWidget( widgetStack );
  widgetStack->addWidget( page, 0 ); // force sequential numbers (play safe)
  vlay = new QVBoxLayout( page, spacingHint() );
  hlay = new QHBoxLayout( vlay ); // inherits spacing
  mIdentity.signatureFileRequester = new KURLRequester( page );
  hlay->addWidget( new QLabel( mIdentity.signatureFileRequester,
			       i18n("&Specify file:"), page ) );
  hlay->addWidget( mIdentity.signatureFileRequester, 1 );
  mIdentity.signatureFileRequester->button()->setAutoDefault( false );
  connect( mIdentity.signatureFileRequester,
	   SIGNAL(textChanged(const QString &)),
	   this, SLOT(slotEnableSignatureEditButton(const QString &)) );
  mIdentity.signatureEditButton = new QPushButton( i18n("Ed&it File"), page );
  connect( mIdentity.signatureEditButton, SIGNAL(clicked()),
	   this, SLOT(slotSignatureEdit()) );
  mIdentity.signatureEditButton->setAutoDefault( false );
  hlay->addWidget( mIdentity.signatureEditButton, 0 );
  vlay->addStretch( 1 ); // spacer

  // page 2: "signature command" requester and label:
  page = new QWidget( widgetStack );
  widgetStack->addWidget( page, 1 ); // force sequential numbers (play safe)
  vlay = new QVBoxLayout( page, spacingHint() );
  hlay = new QHBoxLayout( vlay ); // inherits spacing
  mIdentity.signatureCommandRequester = new KURLRequester( page );
  hlay->addWidget( new QLabel( mIdentity.signatureCommandRequester,
			       i18n("&Specify command:"), page ) );
  hlay->addWidget( mIdentity.signatureCommandRequester, 1 );
  mIdentity.signatureCommandRequester->button()->setAutoDefault( false );
  vlay->addStretch( 1 ); // spacer

  // page 3: input field for direct entering:
  mIdentity.signatureTextEdit = new QMultiLineEdit( widgetStack );
  widgetStack->addWidget( mIdentity.signatureTextEdit, 2 );
  widgetStack->raiseWidget( 0 );
}



void ConfigureDialog::makeNetworkPage( void )
{
  // temp. variables:
  QWidget     *page;
  QVBoxLayout *vlay, *btn_vlay;
  QHBoxLayout *hlay;
  QGroupBox   *group;

  QVBox *vbox = addVBoxPage( i18n("Network"),
			     i18n("Setup for sending and receiving messages"),
    KGlobal::instance()->iconLoader()->loadIcon( "network", KIcon::NoGroup,
    KIcon::SizeMedium ));
  QTabWidget *tabWidget = new QTabWidget( vbox, "config-network-tab" );
  mNetwork.pageIndex = pageIndex(vbox);

  //
  // "Sending" tab:
  //
  page = new QWidget( tabWidget );
  tabWidget->addTab( page, i18n("Sending") );
  vlay = new QVBoxLayout( page, spacingHint() );
  // label: zero stretch
  vlay->addWidget( new QLabel( i18n("SMTP or sendmail"), page ) );

  // hbox layout: stretch 10, spacing inherited from vlay
  hlay = new QHBoxLayout();
  vlay->addLayout( hlay, 10 );

  // transport list: left widget in hlay; stretch 1
  mNetwork.transportList = new ListView( page, "transportList", 5 );
  mNetwork.transportList->addColumn( i18n("Name") );
  mNetwork.transportList->addColumn( i18n("Type") );
  mNetwork.transportList->setAllColumnsShowFocus( true );
  mNetwork.transportList->setFrameStyle( QFrame::WinPanel + QFrame::Sunken );
  mNetwork.transportList->setSorting( -1 );
  connect( mNetwork.transportList, SIGNAL(selectionChanged()),
           SLOT(slotTransportSelected()) );
  connect( mNetwork.transportList, SIGNAL(doubleClicked( QListViewItem *)),
           SLOT(slotModifySelectedTransport()) );
  hlay->addWidget( mNetwork.transportList, 1 );

  // a vbox layout for the buttons: zero stretch, spacing inherited from hlay
  btn_vlay = new QVBoxLayout( hlay );

  // "add..." button: stretch 0
  mNetwork.addTransportButton =
    new QPushButton( i18n("&Add..."), page );
  mNetwork.addTransportButton->setAutoDefault( false );
  connect( mNetwork.addTransportButton, SIGNAL(clicked()),
	   this, SLOT(slotAddTransport()) );
  btn_vlay->addWidget( mNetwork.addTransportButton );

  // "modify..." button: stretch 0
  mNetwork.modifyTransportButton =
    new QPushButton( i18n("&Modify..."), page );
  mNetwork.modifyTransportButton->setAutoDefault( false );
  mNetwork.modifyTransportButton->setEnabled( false );
  connect( mNetwork.modifyTransportButton, SIGNAL(clicked()),
	   this, SLOT(slotModifySelectedTransport()) );
  btn_vlay->addWidget( mNetwork.modifyTransportButton );

  // "remove..." button: stretch 0
  mNetwork.removeTransportButton
    = new QPushButton( i18n("&Remove..."), page );
  mNetwork.removeTransportButton->setAutoDefault( false );
  mNetwork.removeTransportButton->setEnabled( false );
  connect( mNetwork.removeTransportButton, SIGNAL(clicked()),
	   this, SLOT(slotRemoveSelectedTransport()) );
  btn_vlay->addWidget( mNetwork.removeTransportButton );

  // "up" button: stretch 0
  mNetwork.transportUpButton
    = new QPushButton( QString::null, page );
  mNetwork.transportUpButton->setPixmap( BarIcon( "up", KIcon::SizeSmall ) );
  mNetwork.transportUpButton->setEnabled( false );
  connect( mNetwork.transportUpButton, SIGNAL(clicked()),
           this, SLOT(slotTransportUp()) );
  btn_vlay->addWidget( mNetwork.transportUpButton );

  // "down" button: stretch 0
  mNetwork.transportDownButton
    = new QPushButton( QString::null, page );
  mNetwork.transportDownButton->setPixmap( BarIcon( "down", KIcon::SizeSmall ) );
  mNetwork.transportDownButton->setEnabled( false );
  connect( mNetwork.transportDownButton, SIGNAL(clicked()),
           this, SLOT(slotTransportDown()) );
  btn_vlay->addWidget( mNetwork.transportDownButton );
  btn_vlay->addStretch( 1 ); // spacer

  // "Common options" groupbox:
  group = new QGroupBox( 0, Qt::Vertical,
			 i18n("Common options"), page );
  vlay->addWidget(group);

  // a grid layout for the contents of the "common options" group box
  QGridLayout *glay = new QGridLayout( group->layout(), 4, 3, spacingHint() );
  glay->setColStretch( 2, 10 );

  // "default send method" combo:
  mNetwork.sendMethodCombo = new QComboBox( false, group );
  mNetwork.sendMethodCombo->insertItem(i18n("Send now"));
  mNetwork.sendMethodCombo->insertItem(i18n("Send later"));
  glay->addWidget( mNetwork.sendMethodCombo, 0, 1 );

  // "message property" combo:
  mNetwork.messagePropertyCombo = new QComboBox( false, group );
  mNetwork.messagePropertyCombo->insertItem(i18n("Allow 8-bit"));
  mNetwork.messagePropertyCombo->insertItem(
    i18n("MIME Compliant (Quoted Printable)"));
  glay->addWidget( mNetwork.messagePropertyCombo, 1, 1 );

  // labels:
  glay->addWidget( new QLabel( mNetwork.sendMethodCombo, /*buddy*/
			       i18n("&Default send method:"), group ), 0, 0 );
  glay->addWidget( new QLabel( mNetwork.messagePropertyCombo, /*buddy*/
			       i18n("Message &Property:"), group ), 1, 0 );

  // "confirm before send" check box:
  mNetwork.confirmSendCheck =
    new QCheckBox(i18n("&Confirm before send"), group );
  glay->addMultiCellWidget( mNetwork.confirmSendCheck, 2, 2, 0, 1 );

  // "send mail in outbox on check" check box:
  mNetwork.sendOutboxCheck =
    new QCheckBox(i18n("&Send mail in Outbox folder on check"), group );
  glay->addMultiCellWidget( mNetwork.sendOutboxCheck, 3, 3, 0, 1 );

  //
  // "Receiving" tab:
  //
  page = new QWidget( tabWidget );
  tabWidget->addTab( page, i18n("Receiving") );
  vlay = new QVBoxLayout( page, spacingHint() );

  // label: zero stretch
  vlay->addWidget( new QLabel( i18n("Accounts:   (add at least one account!)"), page ) );

  // hbox layout: stretch 10, spacing inherited from vlay
  hlay = new QHBoxLayout();
  vlay->addLayout( hlay, 10 );

  // account list: left widget in hlay; stretch 1
  mNetwork.accountList = new ListView( page, "accountList", 5 );
  mNetwork.accountList->addColumn( i18n("Name") );
  mNetwork.accountList->addColumn( i18n("Type") );
  mNetwork.accountList->addColumn( i18n("Folder") );
  mNetwork.accountList->setAllColumnsShowFocus( true );
  mNetwork.accountList->setFrameStyle( QFrame::WinPanel + QFrame::Sunken );
  mNetwork.accountList->setSorting( -1 );
  connect( mNetwork.accountList, SIGNAL(selectionChanged ()),
	   this, SLOT(slotAccountSelected()) );
  connect( mNetwork.accountList, SIGNAL(doubleClicked( QListViewItem *)),
	   this, SLOT(slotModifySelectedAccount()) );
  hlay->addWidget( mNetwork.accountList, 1 );

  // a vbox layout for the buttons: zero stretch, spacing inherited from hlay
  btn_vlay = new QVBoxLayout( hlay );

  // "add..." button: stretch 0
  mNetwork.addAccountButton =
    new QPushButton( i18n("&Add..."), page );
  mNetwork.addAccountButton->setAutoDefault( false );
  connect( mNetwork.addAccountButton, SIGNAL(clicked()),
	   this, SLOT(slotAddAccount()) );
  btn_vlay->addWidget( mNetwork.addAccountButton );

  // "modify..." button: stretch 0
  mNetwork.modifyAccountButton =
    new QPushButton( i18n("&Modify..."), page );
  mNetwork.modifyAccountButton->setAutoDefault( false );
  mNetwork.modifyAccountButton->setEnabled( false );
  connect( mNetwork.modifyAccountButton, SIGNAL(clicked()),
	   this, SLOT(slotModifySelectedAccount()) );
  btn_vlay->addWidget( mNetwork.modifyAccountButton );

  // "remove..." button: stretch 0
  mNetwork.removeAccountButton
    = new QPushButton( i18n("&Remove..."), page );
  mNetwork.removeAccountButton->setAutoDefault( false );
  mNetwork.removeAccountButton->setEnabled( false );
  connect( mNetwork.removeAccountButton, SIGNAL(clicked()),
	   this, SLOT(slotRemoveSelectedAccount()) );
  btn_vlay->addWidget( mNetwork.removeAccountButton );
  btn_vlay->addStretch( 1 ); // spacer

  // "New Mail Notification" group box: stretch 0
  group = new QVGroupBox( i18n("&New Mail Notification"), page );
  vlay->addWidget( group );
  group->layout()->setSpacing( spacingHint() );

  // "beep on new mail" check box:
  mNetwork.beepNewMailCheck =
    new QCheckBox(i18n("&Beep on new mail"), group );
  //  vlay->addWidget( mNetwork.beepNewMailCheck );

  // "display message box" check box:
  mNetwork.showMessageBoxCheck =
    new QCheckBox(i18n("&Display message box on new mail"), group );
  //  vlay->addWidget( mNetwork.showMessageBoxCheck );

  // "Execute command" check box:
  mNetwork.mailCommandCheck =
    new QCheckBox( i18n("E&xecute command line on new mail"), group );

  // HBox layout for the "specify command" line:
  QHBox *hbox = new QHBox( group );
  

  // command line edit (stretch 1) and label (stretch 0):
  QLabel *label =
    new QLabel( i18n("Specify command:"), hbox );
  mNetwork.mailCommandEdit = new QLineEdit( hbox );
  mNetwork.mailCommandEdit->setEnabled( false );
  label->setBuddy( mNetwork.mailCommandEdit );
  label->setEnabled( false );

  // "choose..." button (stretch 0):
  mNetwork.mailCommandChooseButton =
    new QPushButton( i18n("&Choose..."), hbox );
  mNetwork.mailCommandChooseButton->setAutoDefault( false );
  mNetwork.mailCommandChooseButton->setEnabled( false );

  connect( mNetwork.mailCommandChooseButton, SIGNAL(clicked()),
	   this, SLOT(slotMailCommandChooser()) );
  // Connections that {en,dis}able the "specify command" according to
  // the state of the "Execute command" check box:
  connect( mNetwork.mailCommandCheck, SIGNAL(toggled(bool)),
	   label, SLOT(setEnabled(bool)) );
  connect( mNetwork.mailCommandCheck, SIGNAL(toggled(bool)),
	   mNetwork.mailCommandEdit, SLOT(setEnabled(bool)) );
  connect( mNetwork.mailCommandCheck, SIGNAL(toggled(bool)),
	   mNetwork.mailCommandChooseButton, SLOT(setEnabled(bool)) );

}



void ConfigureDialog::makeAppearancePage( void )
{
  QVBox *vbox = addVBoxPage( i18n("Appearance"),
			     i18n("Customize visual appearance"),
    KGlobal::instance()->iconLoader()->loadIcon( "appearance", KIcon::NoGroup,
    KIcon::SizeMedium ));
  QTabWidget *tabWidget = new QTabWidget( vbox, "tab" );
  mAppearance.pageIndex = pageIndex(vbox);

  QWidget *page1 = new QWidget( tabWidget );
  tabWidget->addTab( page1, i18n("&Fonts") );
  QVBoxLayout *vlay = new QVBoxLayout( page1, spacingHint() );
  mAppearance.customFontCheck =
    new QCheckBox( i18n("&Use custom fonts"), page1 );
  connect( mAppearance.customFontCheck, SIGNAL(clicked() ),
	   this, SLOT(slotCustomFontSelectionChanged()) );
  vlay->addWidget( mAppearance.customFontCheck );
  KSeparator *hline = new KSeparator( KSeparator::HLine, page1);
  vlay->addWidget( hline );
  QHBoxLayout *hlay = new QHBoxLayout( vlay );
  mAppearance.fontLocationLabel = new QLabel( i18n("Apply to:"), page1 );
  hlay->addWidget( mAppearance.fontLocationLabel );
  mAppearance.fontLocationCombo = new QComboBox( page1 );
  //
  // If you add or remove entries to this list, make sure to revise
  // slotFontSelectorChanged(..) as well.
  //
  QStringList fontStringList;
  fontStringList.append( i18n("Message Body") );
  fontStringList.append( i18n("Message List") );
  fontStringList.append( i18n("Message List - Date Field") );
  fontStringList.append( i18n("Folder List") );
  fontStringList.append( i18n("Quoted text - First level") );
  fontStringList.append( i18n("Quoted text - Second level") );
  fontStringList.append( i18n("Quoted text - Third level") );
  mAppearance.fontLocationCombo->insertStringList(fontStringList);

  connect( mAppearance.fontLocationCombo, SIGNAL(activated(int) ),
	   this, SLOT(slotFontSelectorChanged(int)) );
  hlay->addWidget( mAppearance.fontLocationCombo );
  hlay->addStretch(10);
  vlay->addSpacing( spacingHint() );
  mAppearance.fontChooser =
    new KFontChooser( page1, "font", false, QStringList(), false, 4 );
  vlay->addWidget( mAppearance.fontChooser );

  QWidget *page2 = new QWidget( tabWidget );
  tabWidget->addTab( page2, i18n("&Colors") );
  vlay = new QVBoxLayout( page2, spacingHint() );
  mAppearance.customColorCheck =
    new QCheckBox( i18n("&Use custom colors"), page2 );
  connect( mAppearance.customColorCheck, SIGNAL(clicked() ),
	   this, SLOT(slotCustomColorSelectionChanged()) );
  vlay->addWidget( mAppearance.customColorCheck );
  hline = new KSeparator( KSeparator::HLine, page2);
  vlay->addWidget( hline );

  QStringList modeList;
  modeList.append( i18n("Composer Background") );
  modeList.append( i18n("Normal Text") );
  modeList.append( i18n("Quoted Text - First level") );
  modeList.append( i18n("Quoted Text - Second level") );
  modeList.append( i18n("Quoted Text - Third level") );
  modeList.append( i18n("URL Link") );
  modeList.append( i18n("Followed URL Link") );
  modeList.append( i18n("New Message") );
  modeList.append( i18n("Unread Message") );
  modeList.append( i18n("Flagged Message") );
  modeList.append( i18n("PGP Message - OK") );
  modeList.append( i18n("PGP Message - Warning") );
  modeList.append( i18n("PGP Message - Error") );
  modeList.append( i18n("Colorbar - PGP Message") );
  modeList.append( i18n("Colorbar - Plain Text Message") );
  modeList.append( i18n("Colorbar - HTML Message") );

  mAppearance.colorList = new ColorListBox( page2 );
  vlay->addWidget( mAppearance.colorList, 10 );
  for( uint i=0; i<modeList.count(); i++ )
  {
    ColorListItem *listItem = new ColorListItem( modeList[i] );
    mAppearance.colorList->insertItem( listItem );
  }

  mAppearance.recycleColorCheck =
    new QCheckBox( i18n("&Recycle colors on deep quoting"), page2 );
  vlay->addWidget( mAppearance.recycleColorCheck );


  QWidget *page3 = new QWidget( tabWidget );
  tabWidget->addTab( page3, i18n("&Layout") );
  vlay = new QVBoxLayout( page3, spacingHint() );

  mAppearance.longFolderCheck =
    new QCheckBox( i18n("&Show long folder list"), page3 );
  vlay->addWidget( mAppearance.longFolderCheck );

  mAppearance.showColorbarCheck =
    new QCheckBox( i18n("Show color &bar"), page3 );
  vlay->addWidget( mAppearance.showColorbarCheck );

  mAppearance.messageSizeCheck =
    new QCheckBox( i18n("&Display message sizes"), page3 );
  vlay->addWidget( mAppearance.messageSizeCheck );

   mAppearance.nestedMessagesCheck =
    new QCheckBox( i18n("&Thread list of message headers"), page3 );
  vlay->addWidget( mAppearance.nestedMessagesCheck );

  QButtonGroup *threadGroup = new QButtonGroup( i18n("Message header threading options"), page3 );
  vlay->addWidget( threadGroup );
  QVBoxLayout * vthread = new QVBoxLayout( threadGroup, spacingHint() );
  vthread->addSpacing( fontMetrics().lineSpacing() );
  mAppearance.rdAlwaysOpen = new QRadioButton( i18n("&Always keep threads open"), threadGroup );
  vthread->addWidget( mAppearance.rdAlwaysOpen );
  mAppearance.rdDefaultOpen = new QRadioButton( i18n("Threads default to &open"), threadGroup );
  vthread->addWidget( mAppearance.rdDefaultOpen );
  mAppearance.rdDefaultClosed = new QRadioButton( i18n("Threads default to &closed"), threadGroup );
  vthread->addWidget( mAppearance.rdDefaultClosed );
  mAppearance.rdUnreadOpen = new QRadioButton( i18n("Open threads that contain new or &unread messages"), threadGroup );
  vthread->addWidget( mAppearance.rdUnreadOpen );

  QButtonGroup *dateGroup = new QButtonGroup( i18n( "Display of Date" ), page3 );
  vlay->addWidget( dateGroup );
  vthread = new QVBoxLayout( dateGroup, spacingHint() );
  vthread->addSpacing( fontMetrics().lineSpacing() );
  time_t currentTime;
  time( &currentTime );
  mAppearance.rdDateCtime = new QRadioButton( i18n("untranslated format",
                                                   "&Standard C-Format (%1)").arg(
                                                       KMHeaders::formatDate( currentTime,
                                                                              CTime ) ),
                                              dateGroup );
  vthread->addWidget( mAppearance.rdDateCtime );
  mAppearance.rdDateLocalized = new QRadioButton( i18n("&Localized Format (%1)")
                                                  .arg( KMHeaders::formatDate( currentTime,
                                                                               Localized ) ),
                                                  dateGroup );
  vthread->addWidget( mAppearance.rdDateLocalized );
  mAppearance.rdDateFancy = new QRadioButton( i18n("&Fancy Format (%1)")
                                              .arg( KMHeaders::formatDate( currentTime,
                                                                           FancyDate ) ),
                                              dateGroup );
  vthread->addWidget( mAppearance.rdDateFancy );

  vlay->addStretch(10); // Eat unused space a bottom


  QWidget *page4 = new QWidget( tabWidget );
  tabWidget->addTab( page4, i18n("&Profiles") );
  vlay = new QVBoxLayout( page4, spacingHint() );

  QLabel *label = new QLabel( page4 );
  label->setText(i18n("&Select a GUI profile:"));
  vlay->addWidget( label );

  mAppearance.profileList = new ListView( page4, "tagList" );
  mAppearance.profileList->addColumn( i18n("Available profiles") );
  mAppearance.profileList->setAllColumnsShowFocus( true );
  mAppearance.profileList->setFrameStyle( QFrame::WinPanel + QFrame::Sunken );
  mAppearance.profileList->setSorting( -1 );
  label->setBuddy( mAppearance.profileList );
  vlay->addWidget( mAppearance.profileList, 1 );

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

  mAppearance.mListItemDefault =
    new QListViewItem( mAppearance.profileList,
    i18n("KMail - default") );
  mAppearance.mListItemDefaultHtml =
    new QListViewItem( mAppearance.profileList, mAppearance.mListItemDefault,
    i18n("KMail - with HTML preview enabled - less secure !") );
  mAppearance.mListItemContrast =
    new QListViewItem(mAppearance.profileList, mAppearance.mListItemDefaultHtml,
    i18n("High Contrast - Bigger fonts for the visually impaired user"));
  mAppearance.mListItemPurist=
    new QListViewItem(mAppearance.profileList, mAppearance.mListItemContrast,
    i18n("Purist - most features turned off, KDE global settings are used"));

  /* this suggests that the default profile is the selected one
     which is not always the case
  mAppearance.profileList->setSelected( mAppearance.mListItemDefault, true );
  */

  QWidget *page5 = new QWidget( tabWidget );
  tabWidget->addTab( page5, i18n("&Addressbook") );
  vlay = new QVBoxLayout( page5, spacingHint() );

  label = new QLabel( i18n("&Choose Addressbook:"), page5 );
  vlay->addWidget( label );

  mAppearance.addressbookCombo = new QComboBox( page5 );
  label->setBuddy( mAppearance.addressbookCombo );
  QStringList abStringList;
  abStringList.append( i18n("Traditional KMail") );
  abStringList.append( i18n("Traditional KMail interface using KAB database") );
  abStringList.append( i18n("KAB") );
  abStringList.append( i18n("KAddressbook") );
  mAppearance.addressbookCombo->insertStringList( abStringList );
  vlay->addWidget( mAppearance.addressbookCombo );
  vlay->addSpacing( spacingHint() );

  connect( mAppearance.addressbookCombo, SIGNAL(activated(int) ),
	   this, SLOT(slotAddressbookSelectorChanged(int)) );

  QGroupBox *descrbox = new QGroupBox( i18n("Description"), page5 );
  vlay->addWidget( descrbox );

  QVBoxLayout *grpLay = new QVBoxLayout( descrbox, KDialogBase::spacingHint());
  grpLay->addSpacing( fontMetrics().lineSpacing() );


  mAppearance.addressbookStrings.clear();
  mAppearance.addressbookStrings.append( i18n("The traditional KMail graphical "
    "interface using the\ntraditional KMail specific address book database") );
  mAppearance.addressbookStrings.append( i18n("The traditional KMail graphical "
    "interface using the\nstandard KDE Address Book (KAB) database"));
  mAppearance.addressbookStrings.append( i18n("The KDE Address Book "
    "graphical interface (KAB)\nusing the standard KDE Address Book (KAB) "
    "database\n\n"
    "Requires the kdeutils package to be installed."));
  mAppearance.addressbookStrings.append( i18n("The standard KDE Address Book "
    "(KAddressbook)\nusing the standard KDE Address Book (KAB) database."));

  mAppearance.addressbookLabel = new QLabel( descrbox );

  grpLay->addWidget( mAppearance.addressbookLabel );

  mAppearance.addressbookLabel->setText(*mAppearance.addressbookStrings.at(0));
  vlay->addStretch(10);

#ifdef SCORING
  // Score
  QWidget *page6 = new QWidget( tabWidget );
  tabWidget->addTab( page6, i18n("&Messages Scoring") );
  vlay = new QVBoxLayout( page6, spacingHint() );

  KScoringEditorWidget* ksc =
      new KScoringEditorWidget(KMScoringManager::globalScoringManager(),
                              page6);
  KMScoringManager::globalScoringManager()->setMainWin(parent());
  vlay->addWidget( ksc );
#endif
}



void ConfigureDialog::makeComposerPage( void )
{
  // temp. vars:
  QWidget     *page;
  QHBoxLayout *hlay;
  QVBoxLayout *vlay;
  QGridLayout *glay;
  QLabel      *label;
  QPushButton *button;

  QVBox *vbox = addVBoxPage( i18n("Composer"),
			  i18n("Phrases and general behavior"),
      KGlobal::instance()->iconLoader()->loadIcon( "edit", KIcon::NoGroup,
      KIcon::SizeMedium ));
  QTabWidget *tabWidget = new QTabWidget( vbox, "tab" );
  mComposer.pageIndex = pageIndex(vbox);

  page = new QWidget( tabWidget );
  tabWidget->addTab( page, i18n("&General") );
  QVBoxLayout *topLevel = new QVBoxLayout( page, spacingHint() );

  mComposer.autoAppSignFileCheck =
    new QCheckBox( i18n("&Automatically append signature"), page );
  topLevel->addWidget( mComposer.autoAppSignFileCheck );

  mComposer.smartQuoteCheck =
    new QCheckBox( i18n("&Use smart quoting"), page );
  topLevel->addWidget( mComposer.smartQuoteCheck );

  mComposer.pgpAutoSignatureCheck =
    new QCheckBox( i18n("Automatically &sign messages using PGP"), page );
  topLevel->addWidget( mComposer.pgpAutoSignatureCheck );

  mComposer.pgpAutoEncryptCheck =
    new QCheckBox( i18n("Automatically encrypt messages if possible"), page );
  topLevel->addWidget( mComposer.pgpAutoEncryptCheck );

  hlay = new QHBoxLayout( topLevel );
  mComposer.wordWrapCheck =
    new QCheckBox( i18n("&Word wrap at column:"), page );
  connect( mComposer.wordWrapCheck, SIGNAL(clicked() ),
	   this, SLOT(slotWordWrapSelectionChanged()) );
  hlay->addWidget( mComposer.wordWrapCheck );
  mComposer.wrapColumnSpin = new KIntNumInput( page );
  mComposer.wrapColumnSpin->setRange( 1, 10000, 1, FALSE );
  hlay->addWidget( mComposer.wrapColumnSpin, 0, AlignLeft );
  hlay->addStretch(10);

  QGroupBox * editorGroup = new QGroupBox( i18n("External Editor"), page );
  topLevel->addWidget(editorGroup);
  vlay = new QVBoxLayout( editorGroup, spacingHint() );
  vlay->addSpacing( fontMetrics().lineSpacing() );
  mComposer.externalEditorCheck =
    new QCheckBox(i18n("&Use external editor instead of composer"),
    editorGroup );
  connect( mComposer.externalEditorCheck, SIGNAL(clicked() ),
	   this, SLOT(slotExternalEditorSelectionChanged()) );
  vlay->addWidget( mComposer.externalEditorCheck );
  QHBoxLayout *edhlay = new QHBoxLayout( vlay );
  mComposer.externalEditorLabel = new QLabel( i18n("&Specify editor:"),
    editorGroup );
  edhlay->addWidget( mComposer.externalEditorLabel );
  mComposer.externalEditorEdit = new QLineEdit( editorGroup );
  mComposer.externalEditorLabel->setBuddy( mComposer.externalEditorEdit );
  edhlay->addWidget( mComposer.externalEditorEdit );
  mComposer.externalEditorChooseButton =
    new QPushButton( i18n("&Choose..."), editorGroup );
  connect( mComposer.externalEditorChooseButton, SIGNAL(clicked()),
	   this, SLOT(slotExternalEditorChooser()) );
  mComposer.externalEditorChooseButton->setAutoDefault( false );
  edhlay->addWidget( mComposer.externalEditorChooseButton );
  mComposer.externalEditorHelp = new QLabel( editorGroup );
  mComposer.externalEditorHelp->setText(
    i18n("\"%f\" will be replaced with the filename to edit."));
  vlay->addWidget( mComposer.externalEditorHelp );

  topLevel->addStretch(10);

  // ----- reply phrases page

  page = new QWidget( tabWidget );
  tabWidget->addTab( page, i18n("P&hrases") );
  topLevel = new QVBoxLayout( page, spacingHint() );
  QGroupBox *group = new QGroupBox(i18n("Phrases"), page );
  topLevel->addWidget( group );
  topLevel->addStretch( 10 );

  glay = new QGridLayout( group, 8, 2, spacingHint() );
  glay->addRowSpacing( 0, fontMetrics().lineSpacing() );
  glay->setColStretch( 1, 10 );

  label = new QLabel( group );
  label->setText(
     i18n( "The following placeholders are supported in the reply phrases:\n"
	   "%D=date, %S=subject, %F=sender, %%=percent sign, %_=space, %L=linebreak"));
  label->setAlignment( WordBreak );
  glay->addMultiCellWidget( label, 1, 1, 0, 1 );
  label = new QLabel( i18n("&Language:"), group );
  glay->addWidget( label, 2, 0 );
  mComposer.phraseLanguageCombo = new LanguageComboBox( false, group );
  label->setBuddy( mComposer.phraseLanguageCombo );
  glay->addWidget( mComposer.phraseLanguageCombo, 2, 1 );
  mComposer.LanguageList = NULL;
  QHBoxLayout *languageHlay = new QHBoxLayout( 0, 0, spacingHint() );
  glay->addLayout( languageHlay, 3, 1 );
  QPushButton *newButton = new QPushButton( i18n("&Add..."), group );
  mComposer.removeButton = new QPushButton( i18n("&Remove"), group );
  newButton->setAutoDefault( false );
  mComposer.removeButton->setAutoDefault( false );
  languageHlay->addWidget( newButton );
  languageHlay->addWidget( mComposer.removeButton );
  connect( newButton, SIGNAL(clicked()),
           this, SLOT(slotNewLanguage()) );
  connect( mComposer.removeButton, SIGNAL(clicked()),
           this, SLOT(slotRemoveLanguage()) );
  connect( mComposer.phraseLanguageCombo, SIGNAL(activated( const QString& )),
           this, SLOT(slotLanguageChanged( const QString& )) );
  label = new QLabel( i18n("&Reply to sender:"), group );
  glay->addWidget( label, 4, 0 );
  mComposer.phraseReplyEdit = new QLineEdit( group );
  label->setBuddy(mComposer.phraseReplyEdit);
  glay->addWidget( mComposer.phraseReplyEdit, 4, 1 );
  label = new QLabel( i18n("Reply to &all:"), group );
  glay->addWidget( label, 5, 0 );
  mComposer.phraseReplyAllEdit = new QLineEdit( group );
  label->setBuddy(mComposer.phraseReplyAllEdit);
  glay->addWidget( mComposer.phraseReplyAllEdit, 5, 1 );
  label = new QLabel( i18n("&Forward:"), group );
  glay->addWidget( label, 6, 0 );
  mComposer.phraseForwardEdit = new QLineEdit( group );
  label->setBuddy(mComposer.phraseForwardEdit);
  glay->addWidget( mComposer.phraseForwardEdit, 6, 1 );
  label = new QLabel( i18n("&Indentation:"), group );
  glay->addWidget( label, 7, 0 );
  mComposer.phraseindentPrefixEdit = new QLineEdit( group );
  label->setBuddy(mComposer.phraseindentPrefixEdit);
  glay->addWidget( mComposer.phraseindentPrefixEdit, 7, 1 );

  // ----- subject page
  QWidget *subjectPage = new QWidget( tabWidget );
  tabWidget->addTab( subjectPage, i18n("&Subject") );
  QVBoxLayout *topLevel2 = new QVBoxLayout( subjectPage, spacingHint() );

  QGroupBox *replyGroup = new QGroupBox(i18n("Reply subject prefixes"), subjectPage );
  topLevel2->addWidget( replyGroup );

  QGridLayout *glay2 = new QGridLayout( replyGroup, 6, 3, spacingHint() );
  glay2->addRowSpacing( 0, fontMetrics().lineSpacing() );
  glay2->setColStretch( 2, 10 );
  glay2->setRowStretch( 4, 10 );

  label = new QLabel( replyGroup );
  label->setText(i18n( "Recognize the following prefixes (matching is case-insensitive)"));
  glay2->addMultiCellWidget( label, 1, 1, 0, 2 );

  mComposer.replyListBox = new QListBox( replyGroup, "prefixList" );
  glay2->addMultiCellWidget(mComposer.replyListBox, 2, 4, 0, 0);
  connect( mComposer.replyListBox, SIGNAL(selectionChanged ()),
	   this, SLOT(slotReplyPrefixSelected()) );

  mComposer.addReplyPrefixButton =
     new QPushButton( i18n("&Add..."), replyGroup );
  mComposer.addReplyPrefixButton->setAutoDefault( false );
  connect( mComposer.addReplyPrefixButton, SIGNAL(clicked()),
  	   this, SLOT(slotAddReplyPrefix()) );
  glay2->addWidget( mComposer.addReplyPrefixButton, 2, 1 );

  mComposer.removeReplyPrefixButton =
     new QPushButton( i18n("&Remove"), replyGroup );
  mComposer.removeReplyPrefixButton->setAutoDefault( false );
  mComposer.removeReplyPrefixButton->setEnabled( false );
  connect( mComposer.removeReplyPrefixButton, SIGNAL(clicked()),
  	   this, SLOT(slotRemoveSelReplyPrefix()) );
  glay2->addWidget( mComposer.removeReplyPrefixButton, 3, 1 );

  mComposer.replaceReplyPrefixCheck =
     new QCheckBox( i18n("Re&place recognized prefix with Re:"), replyGroup );
  glay2->addMultiCellWidget( mComposer.replaceReplyPrefixCheck, 5, 5, 0, 2);

  //forward group
  QGroupBox *forwardGroup = new QGroupBox(i18n("Forward subject prefixes"), subjectPage );
  topLevel2->addWidget( forwardGroup );

  QGridLayout *glay3 = new QGridLayout( forwardGroup, 6, 3, spacingHint() );
  glay3->addRowSpacing( 0, fontMetrics().lineSpacing() );
  glay3->setColStretch( 2, 10 );
  glay3->setRowStretch( 4, 10 );

  label = new QLabel( forwardGroup );
  label->setText(i18n( "Recognize the following prefixes (matching is case-insensitive)"));
  glay3->addMultiCellWidget( label, 1, 1, 0, 2 );

  mComposer.forwardListBox = new QListBox( forwardGroup, "prefixList" );
  glay3->addMultiCellWidget(mComposer.forwardListBox, 2, 4, 0, 0);
  connect( mComposer.forwardListBox, SIGNAL(selectionChanged ()),
	   this, SLOT(slotForwardPrefixSelected()) );

  mComposer.addForwardPrefixButton =
     new QPushButton( i18n("A&dd..."), forwardGroup );
  mComposer.addForwardPrefixButton->setAutoDefault( false );
  connect( mComposer.addForwardPrefixButton, SIGNAL(clicked()),
  	   this, SLOT(slotAddForwardPrefix()) );
  glay3->addWidget( mComposer.addForwardPrefixButton, 2, 1 );

  mComposer.removeForwardPrefixButton =
     new QPushButton( i18n("Re&move"), forwardGroup );
  mComposer.removeForwardPrefixButton->setAutoDefault( false );
  mComposer.removeForwardPrefixButton->setEnabled( false );
  connect( mComposer.removeForwardPrefixButton, SIGNAL(clicked()),
  	   this, SLOT(slotRemoveSelForwardPrefix()) );
  glay3->addWidget( mComposer.removeForwardPrefixButton, 3, 1 );

  mComposer.replaceForwardPrefixCheck =
     new QCheckBox( i18n("&Replace recognized prefix with Fwd:"), forwardGroup );
  glay3->addMultiCellWidget( mComposer.replaceForwardPrefixCheck, 5, 5, 0, 2);

  // ----- charset page
  QWidget *charsetPage = new QWidget( tabWidget );
  tabWidget->addTab( charsetPage, i18n("&Charset") );
  QVBoxLayout *topLevel3 = new QVBoxLayout( charsetPage, spacingHint() );

  //list of charsets
  QGroupBox *charsetsGroup = new QGroupBox( i18n("Preferred charsets"),
    charsetPage );
  QGridLayout *charsetsGridLay = new QGridLayout( charsetsGroup, 8, 2,
    spacingHint() );
  charsetsGridLay->addRowSpacing( 0, fontMetrics().lineSpacing() );
  charsetsGridLay->setRowStretch( 1, 10 );
  label = new QLabel( charsetsGroup );
  label->setAlignment( WordBreak);
  label->setText( i18n("This list is checked for every outgoing mail from the "
    "top to the bottom for a charset that contains all required characters.") );
  charsetsGridLay->addWidget( label, 1, 0 );
  mComposer.charsetListBox = new QListBox( charsetsGroup );
  charsetsGridLay->addMultiCellWidget( mComposer.charsetListBox, 2, 6, 0, 0 );
  mComposer.addCharsetButton = new QPushButton( i18n("&Add..."), charsetsGroup );
  charsetsGridLay->addWidget( mComposer.addCharsetButton, 3, 1 );
  mComposer.removeCharsetButton = new QPushButton( i18n("&Remove"),
    charsetsGroup );
  charsetsGridLay->addWidget( mComposer.removeCharsetButton, 4, 1 );
  mComposer.charsetUpButton = new QPushButton( i18n("&Up"), charsetsGroup );
  mComposer.charsetUpButton->setAutoRepeat( TRUE );
  charsetsGridLay->addWidget( mComposer.charsetUpButton, 5, 1 );
  mComposer.charsetDownButton = new QPushButton( i18n("&Down"), charsetsGroup );
  mComposer.charsetDownButton->setAutoRepeat( TRUE );
  charsetsGridLay->addWidget( mComposer.charsetDownButton, 6, 1 );
  mComposer.forceReplyCharsetCheck =
    new QCheckBox( i18n("&Keep original charset when replying or forwarding (if possible)."),
    charsetsGroup );
  charsetsGridLay->addMultiCellWidget( mComposer.forceReplyCharsetCheck, 7, 7, 0, 1 );
  topLevel3->addWidget( charsetsGroup );
  connect( mComposer.addCharsetButton, SIGNAL(clicked()),
  	   this, SLOT(slotAddCharset()) );
  connect( mComposer.removeCharsetButton, SIGNAL(clicked()),
  	   this, SLOT(slotRemoveSelCharset()) );
  connect( mComposer.charsetUpButton, SIGNAL(clicked()),
           this, SLOT(slotCharsetUp()) );
  connect( mComposer.charsetDownButton, SIGNAL(clicked()),
           this, SLOT(slotCharsetDown()) );
  connect( mComposer.charsetListBox, SIGNAL(selectionChanged()),
           this, SLOT(slotCharsetSelectionChanged()) );

  topLevel3->addSpacing( spacingHint() );
  topLevel3->addStretch();

  //
  // Tab Widget: Headers
  //
  page = new QWidget( tabWidget );
  tabWidget->addTab( page, i18n("&Headers") );
  vlay = new QVBoxLayout( page, marginHint(), spacingHint() );

  // "create own message id"
  mComposer.createOwnMessageIdCheck =
    new QCheckBox( i18n("&Create own Message-Id headers "
			"(see \"What's This\" (Shift-F1) help!)"), page );
  vlay->addWidget( mComposer.createOwnMessageIdCheck );
  QWhatsThis::add( mComposer.createOwnMessageIdCheck,
		   i18n("<qt><p>Check this option if your mail server doesn't "
			"add a <i>Message-ID</i> header to your mails. "
			"<i>Message-ID</i> headers are used to control the "
			"grouping of mails that belong together "
			"(\"threading\").</p>"
			"<p>You can find out if your mail server adds a "
			"<i>Message-ID</i> header to your mails by sending a "
			"message to yourself and then choosing <i>View|All "
			"headers</i> to see if the header includes such a "
			"field.</p>"
			"<p>If it doesn't, you should add a custom suffix in "
			"the lineedit below. It must be unique and it must "
			"consist of only latin letters, arabic numbers and "
			"\".\".</p>"
			"<p>Recommended values include the hostname of your "
			"mail server or a domainname you control.</p></qt>") );

  // "msg-id suffix" line edit and label:
  hlay = new QHBoxLayout( vlay ); // inherits spacing
  mComposer.messageIdSuffixEdit = new QLineEdit( page );
  label = new QLabel( mComposer.messageIdSuffixEdit,
		      i18n("&Use this Message-Id suffix:"), page );
  label->setEnabled( false );
  mComposer.messageIdSuffixEdit->setEnabled( false );
  hlay->addWidget( label );
  hlay->addWidget( mComposer.messageIdSuffixEdit, 1 );
  connect( mComposer.createOwnMessageIdCheck, SIGNAL(toggled(bool) ),
	   label, SLOT(setEnabled(bool)) );
  connect( mComposer.createOwnMessageIdCheck, SIGNAL(toggled(bool) ),
	   mComposer.messageIdSuffixEdit, SLOT(setEnabled(bool)) );

  // horizontal rule and "custom header fields" label:
  vlay->addWidget( new KSeparator( KSeparator::HLine, page ) );
  vlay->addWidget( new QLabel( i18n("Define custom mime header fields:"), page ) );

  // "custom header fields" listbox:
  glay = new QGridLayout( vlay, 5, 3 ); // inherits spacing
  glay->setRowStretch( 2, 1 );
  glay->setColStretch( 1, 1 );
  mComposer.tagList = new ListView( page, "tagList" );
  mComposer.tagList->addColumn( i18n("Name") );
  mComposer.tagList->addColumn( i18n("Value") );
  mComposer.tagList->setAllColumnsShowFocus( true );
  mComposer.tagList->setFrameStyle( QFrame::WinPanel | QFrame::Sunken );
  mComposer.tagList->setSorting( -1 );
  connect( mComposer.tagList, SIGNAL(selectionChanged()),
	   this, SLOT(slotMimeHeaderSelectionChanged()) );
  glay->addMultiCellWidget( mComposer.tagList, 0, 2, 0, 1 );

  // "new" and "remove" buttons:
  button = new QPushButton( i18n("&New"), page );
  connect( button, SIGNAL(clicked()), this, SLOT(slotNewMimeHeader()) );
  button->setAutoDefault( false );
  glay->addWidget( button, 0, 2 );
  button = new QPushButton(i18n("&Remove"), page );
  connect( button, SIGNAL(clicked()), this, SLOT(slotDeleteMimeHeader()) );
  button->setAutoDefault( false );
  glay->addWidget( button, 1, 2 );

  // "name" and "value" line edits and labels:
  mComposer.tagNameEdit = new QLineEdit( page );
  mComposer.tagNameEdit->setEnabled( false );
  mComposer.tagNameLabel = new QLabel( mComposer.tagNameEdit, i18n("N&ame:"), page );
  mComposer.tagNameLabel->setEnabled( false );
  glay->addWidget( mComposer.tagNameLabel, 3, 0 );
  glay->addWidget( mComposer.tagNameEdit, 3, 1 );
  connect( mComposer.tagNameEdit, SIGNAL(textChanged(const QString&)),
	   this, SLOT(slotMimeHeaderNameChanged(const QString&)) );

  mComposer.tagValueEdit = new QLineEdit( page );
  mComposer.tagValueEdit->setEnabled( false );
  mComposer.tagValueLabel = new QLabel( mComposer.tagValueEdit, i18n("&Value:"), page );
  mComposer.tagValueLabel->setEnabled(false);
  glay->addWidget( mComposer.tagValueLabel, 4, 0 );
  glay->addWidget( mComposer.tagValueEdit, 4, 1 );
  connect( mComposer.tagValueEdit, SIGNAL(textChanged(const QString&)),
	   this, SLOT(slotMimeHeaderValueChanged(const QString&)) );
}



void ConfigureDialog::makeSecurityPage( void )
{
    QVBox *vbox = addVBoxPage( i18n("Security"),
                               i18n("Security and Privacy Settings"),
                               KGlobal::instance()->iconLoader()->
                               loadIcon( "encrypted", KIcon::NoGroup,
                                         KIcon::SizeMedium ));
    mSecurity.pageIndex = pageIndex(vbox);

    QTabWidget *tabWidget = new QTabWidget( vbox, "tab" );
    QWidget *page = new QWidget( tabWidget );

    tabWidget->addTab( page, i18n("&General") );
    QVBoxLayout *vlay = new QVBoxLayout( page, spacingHint() );

    QGroupBox *gb = new QGroupBox( i18n( "HTML Mails" ), page );
    vlay->addWidget( gb );
    QVBoxLayout *glay = new QVBoxLayout( gb, KDialog::spacingHint() );
    glay->addSpacing( fontMetrics().lineSpacing() );

    mSecurity.htmlMailCheck =
      new QCheckBox( i18n("&Prefer HTML to plain text"), gb );
    glay->addWidget( mSecurity.htmlMailCheck );
    mSecurity.externalReferences =
      new QCheckBox( i18n( "&Load external references from the net" ), gb );
    glay->addWidget( mSecurity.externalReferences );

    QLabel *label = new QLabel( gb );
    label->setAlignment( WordBreak);
    label->setTextFormat( RichText );
    label->setText(i18n(
      "<b>WARNING:</b> Allowing HTML in EMail may increase the risk "
      "that your system will be compromised by present and anticipated "
      "security exploits. Use \"What's this\" help (Shift-F1) for detailed "
      "information on each option.") );
    glay->addWidget( label );

    gb = new QGroupBox( i18n( "Delivery and Read Confirmations" ), page );
    vlay->addWidget( gb );
    glay = new QVBoxLayout( gb, KDialog::spacingHint() );
    glay->addSpacing( fontMetrics().lineSpacing() );

    mSecurity.sendReceiptCheck = new QCheckBox(
	   i18n("&Automatically send receive- and read confirmations"), gb );
    glay->addWidget( mSecurity.sendReceiptCheck );
    label = new QLabel( gb );
    label->setAlignment( WordBreak);
    label->setTextFormat( RichText );
    label->setText( i18n(
      "<p><b>WARNING:</b> Unconditionally returning receipts undermines your privacy. "
      "See \"What's this\" help (Shift-F1) for more." ) );
    glay->addWidget( label );

    vlay->addStretch(10);

    QWhatsThis::add( mSecurity.htmlMailCheck,
		     i18n( "<qt><p>EMails sometimes come in both formats. This options "
			   "controls whether you want the HTML part or the plain text "
			   "part to be displayed.</p>"
			   "<p>Displaying the HTML part makes the message look better, "
			   "but at the same time increases the risk of security holes "
			   "being exploited.</p>"
			   "<p>Displaying the plain text part loses much of the message's "
			   "formatting, but makes it <em>impossible</em> "
			   "to expolit security holes in the HTML renderer (Konqueror).</p>"
			   "<p>The option below guards against one common misuse of HTML "
			   "mails. But it cannot guard against security issues that were not "
			   "known at the time this version of KMail was written.</p>"
			   "<p>It is therefore advisable to <em>not</em> prefer HTML to "
			   "plain text.</p></qt>" ) );
    QWhatsThis::add( mSecurity.externalReferences,
                     i18n( "<qt><p>Some mail advertisements are in HTML "
                           "and contain references to images that these "
                           "advertisements use to find out you've read "
                           "their mail (\"web bugs\").</p>"
			   "<p>There's no valid reason to load images off "
			   "the net like this, since the sender can always "
			   "attach the needed images directly.</p>"
                           "<p>To guard from such a misuse of the HTML "
                           "displaying feature of kmail, this option is "
                           "<em>disabled</em> by default.</p>"
                           "<p>If you nonetheless wish to e.g. view images in "
			   "HTML mails that were not attached to it, you can "
			   "enable this option, but you should be aware of the "
			   "possible problem.</p></qt>" ) );
    QWhatsThis::add( mSecurity.sendReceiptCheck, i18n(
		     "<qt><p>This options enables the <em>unconditional</em> sending "
		     "of delivery- and read confirmations (\"receipts\").</p>"
		     "<p>Returning receipts makes it easy for the sender to track "
		     "whether and - more importantly - <em>when</em> you read his/her "
		     "mail.</p>"
		     "<p>You can return <em>delivery</em> receipts in a fine-grained "
		     "way using the \"confirm delivery\" filter action. We advise "
		     "against issuing <em>read</em> confirmations at all.</p></qt>") );

    // ---------- PGP tab
    page = new QWidget( tabWidget );
    tabWidget->addTab( page, i18n("&PGP") );
    vlay = new QVBoxLayout( page, spacingHint() );

    mSecurity.pgpConfig = new Kpgp::Config(page);
    vlay->addWidget( mSecurity.pgpConfig );
    vlay->addStretch(10);
}



void ConfigureDialog::makeMiscPage( void )
{
  //KIconLoader *loader = instace->iconLoader();
  ///KGlobal::instance()->iconLoader()

  QFrame *page = addPage( i18n("Miscellaneous"), i18n("Various settings"),
    KGlobal::instance()->iconLoader()->loadIcon( "misc", KIcon::NoGroup,
    KIcon::SizeMedium ));
  QVBoxLayout *topLevel = new QVBoxLayout( page, 0, spacingHint() );
  mMisc.pageIndex = pageIndex(page);

  //---------- group: trash folder
  QGroupBox *tgroup = new QGroupBox( i18n("&Trash folder"), page );
  topLevel->addWidget( tgroup );
  QVBoxLayout *tvlay = new QVBoxLayout( tgroup, spacingHint() );

  tvlay->addSpacing( fontMetrics().lineSpacing() );
  mMisc.emptyTrashCheck =
    new QCheckBox(i18n("&Empty trash on exit"), tgroup );
  tvlay->addWidget( mMisc.emptyTrashCheck );
  QHBoxLayout *stlay = new QHBoxLayout( spacingHint() ,"hly1");
  stlay->setMargin(0);
  tvlay->addLayout( stlay );

  //---------- group: Housekeeping
  // In this group is verything to do with general housekeeping,
  // such as message expiry.
  QGroupBox *hkGroup = new QGroupBox(i18n("Automatic message expiry"), page);
  topLevel->addWidget(hkGroup);
  QVBoxLayout *hklay = new QVBoxLayout(hkGroup, spacingHint());
  hklay->addSpacing(fontMetrics().lineSpacing());

  mMisc.expBGroup = new QButtonGroup(page);
  mMisc.expBGroup->hide();
  mMisc.manualExpiry = new QRadioButton(i18n("Manual expiry"), hkGroup);
  mMisc.expBGroup->insert(mMisc.manualExpiry, expireManual);
  hklay->addWidget(mMisc.manualExpiry);
  mMisc.expireAtExit = new QRadioButton(i18n("Expire old messages on exit"), hkGroup);
  mMisc.expBGroup->insert(mMisc.expireAtExit, expireAtExit);
  hklay->addWidget(mMisc.expireAtExit);
  //mMisc.expireAtStart = new QRadioButton(i18n("Expire old messages on startup"), hkGroup);
  //mMisc.expBGroup->insert(mMisc.expireAtStart, expireAtStart);
  //hklay->addWidget(mMisc.expireAtStart);
//  mMisc.expireDaily = new QRadioButton(i18n("Expire old messages daily (at midnight)"),
//				       hkGroup);
//  mMisc.expBGroup->insert(mMisc.expireDaily, expireDaily);
//  hklay->addWidget(mMisc.expireDaily);
//  mMisc.expireDaily->setEnabled(false); // Not currently implemented!

  mMisc.warnBeforeExpire = new QCheckBox(i18n("Warn before expiring messages"), hkGroup);
  hklay->addWidget(mMisc.warnBeforeExpire);



  //---------- group: folders

  QGroupBox *group = new QGroupBox( i18n("&Folders"), page );
  topLevel->addWidget( group );
  QVBoxLayout *vlay = new QVBoxLayout( group, spacingHint() );
  vlay->addSpacing( fontMetrics().lineSpacing() );
  mMisc.compactOnExitCheck =
    new QCheckBox(i18n("C&ompact all folders on exit"), group );
  vlay->addWidget( mMisc.compactOnExitCheck );
  mMisc.emptyFolderConfirmCheck =
    new QCheckBox(i18n("Conf&irm before emptying folders"), group );
  vlay->addWidget( mMisc.emptyFolderConfirmCheck );

  QHBoxLayout *hlay = new QHBoxLayout();
  vlay->addLayout( hlay, 10 );
  hlay->addWidget( new QLabel( i18n("Default Mailbox Format:"), group ) );
  mMisc.mailboxPrefCombo = new QComboBox( false, group );
  mMisc.mailboxPrefCombo->insertItem( "mbox" );
  mMisc.mailboxPrefCombo->insertItem( "maildir" );
  QWhatsThis::add( mMisc.mailboxPrefCombo,
                   i18n("<qt>This selects which mailbox format will be "
                        "the default.<p><b>mbox:</b> Common but less "
                        "reliable.<p><b>maildir:</b> More reliable, but "
                        "less commonly used</qt>") );
  hlay->addWidget( mMisc.mailboxPrefCombo );
  hlay->addStretch( 1 );

  topLevel->addStretch( 10 );
}

void ConfigureDialog::setup( void )
{
  setupIdentityPage();
  setupNetworkPage();
  setupAppearancePage();
  setupComposerPage();
  setupSecurityPage();
  setupMiscPage();
}

void ConfigureDialog::setupIdentityPage( void )
{
  mIdentity.identities.importData();
  mIdentity.identityCombo->clear();
  mIdentity.identityCombo->insertStringList( mIdentity.identities.names() );
  mIdentity.activeIdentity = QString::null;
  slotIdentitySelectorChanged(); // This will trigger an update
}


void ConfigureDialog::setupNetworkPage( void )
{
  KConfig *config = kapp->config();
  KConfigGroupSaver saver(config, "General");
  int numTransports = config->readNumEntry("transports", 1);

  QListViewItem *top = NULL;
  mTransportList.setAutoDelete(TRUE);
  mTransportList.clear();
  mNetwork.transportList->clear();
  for (int i = 1; i <= numTransports; i++)
  {
    KMTransportInfo *ti = new KMTransportInfo();
    ti->readConfig(i);
    mTransportList.append(ti);
    QListViewItem *listItem =
      new QListViewItem( mNetwork.transportList, top, ti->name, ti->type );
    top = listItem;
  }

  mNetwork.sendMethodCombo->setCurrentItem(
    kernel->msgSender()->sendImmediate() ? 0 : 1 );
  mNetwork.messagePropertyCombo->setCurrentItem(
    kernel->msgSender()->sendQuotedPrintable() ? 1 : 0 );
  {
    KConfigGroupSaver saver(config, "Composer");

    mNetwork.confirmSendCheck->setChecked(
      config->readBoolEntry( "confirm-before-send", false ) );
  }
  mNetwork.sendOutboxCheck->setChecked( config->readBoolEntry("sendOnCheck", false) );


  QListViewItem *listItem = mNetwork.transportList->firstChild();
  if (listItem)
  {
    listItem->setText(1, listItem->text(1) + " " + i18n("(Default)"));
    mNetwork.transportList->setCurrentItem( listItem );
    mNetwork.transportList->setSelected( listItem, true );
  }
  slotUpdateTransportCombo();

  mNetwork.accountList->clear();
  top = NULL;
  for( KMAccount *a = kernel->acctMgr()->first(); a!=0;
       a = kernel->acctMgr()->next() )
  {
    QListViewItem *listItem =
      new QListViewItem( mNetwork.accountList, top, a->name(), a->type() );
    if( a->folder() )
      listItem->setText( 2, a->folder()->label() );
    top = listItem;
  }

  listItem = mNetwork.accountList->firstChild();
  if (listItem)
  {
    mNetwork.accountList->setCurrentItem( listItem );
    mNetwork.accountList->setSelected( listItem, true );
  }

  mNetwork.beepNewMailCheck->setChecked( config->readBoolEntry("beep-on-mail", false ) );
  mNetwork.showMessageBoxCheck->setChecked( config->readBoolEntry("msgbox-on-mail", false) );
  mNetwork.mailCommandCheck->setChecked( config->readBoolEntry("exec-on-mail", false) );
  mNetwork.mailCommandEdit->setText( config->readEntry("exec-on-mail-cmd", ""));
}

void ConfigureDialog::setupAppearancePage( void )
{
  KConfig *config = kapp->config();
  bool state;

  { //area for config group "Fonts"
    KConfigGroupSaver saver(config, "Fonts");
    mAppearance.font[0] = QFont("helvetica");
    mAppearance.font[0] =
      config->readFontEntry("body-font", &mAppearance.font[0]);
    mAppearance.font[1] =
      config->readFontEntry("list-font", &mAppearance.font[0]);
    mAppearance.font[2] =
      config->readFontEntry("list-date-font", &mAppearance.font[0]);
    mAppearance.font[3] =
      config->readFontEntry("folder-font", &mAppearance.font[0]);
    mAppearance.font[4] =
      config->readFontEntry("quote1-font", &mAppearance.font[0]);
    mAppearance.font[5] =
      config->readFontEntry("quote2-font", &mAppearance.font[0]);
    mAppearance.font[6] =
      config->readFontEntry("quote3-font", &mAppearance.font[0]);

    state = config->readBoolEntry("defaultFonts", TRUE );
    mAppearance.customFontCheck->setChecked( state == false ? true : false );
    slotCustomFontSelectionChanged();
    updateFontSelector();
    slotFontSelectorChanged( mAppearance.fontLocationCombo->currentItem() );
  }

  {
    KConfigGroupSaver saver(config, "Reader");

    mAppearance.showColorbarCheck->setChecked(
      config->readBoolEntry( "showColorbar", false ) );

    QColor defaultColor = QColor(kapp->palette().active().base());
    mAppearance.colorList->setColor(
      0, config->readColorEntry("BackgroundColor",&defaultColor ) );

    defaultColor = QColor(kapp->palette().active().text());
    mAppearance.colorList->setColor(
      1, config->readColorEntry("ForegroundColor",&defaultColor ) );

    defaultColor = QColor(kapp->palette().active().text());
    mAppearance.colorList->setColor(
      2, config->readColorEntry("QuoutedText1",&defaultColor ) );

    defaultColor = QColor(kapp->palette().active().text());
    mAppearance.colorList->setColor(
      3, config->readColorEntry("QuoutedText2",&defaultColor ) );

    defaultColor = QColor(kapp->palette().active().text());
    mAppearance.colorList->setColor(
      4, config->readColorEntry("QuoutedText3",&defaultColor ) );

    defaultColor = KGlobalSettings::linkColor();
    mAppearance.colorList->setColor(
      5, config->readColorEntry("LinkColor",&defaultColor ) );

    defaultColor = KGlobalSettings::visitedLinkColor();
    mAppearance.colorList->setColor(
      6, config->readColorEntry("FollowedColor",&defaultColor ) );

    defaultColor = QColor("red");
    mAppearance.colorList->setColor(
      7, config->readColorEntry("NewMessage",&defaultColor ) );

    defaultColor = QColor("blue");
    mAppearance.colorList->setColor(
      8, config->readColorEntry("UnreadMessage",&defaultColor ) );

    defaultColor = QColor(0,0x7F,0);
    mAppearance.colorList->setColor(
      9, config->readColorEntry("FlagMessage",&defaultColor ) );

    defaultColor = QColor( 0x40, 0xFF, 0x40 ); // light green
    mAppearance.colorList->setColor(
      10, config->readColorEntry( "PGPMessageOK",&defaultColor ) );

    defaultColor = QColor( 0xFF, 0xFF, 0x40 ); // light yellow
    mAppearance.colorList->setColor(
      11, config->readColorEntry( "PGPMessageWarn",&defaultColor ) );

    defaultColor = QColor( 0xFF, 0x00, 0x00 ); // red
    mAppearance.colorList->setColor(
      12, config->readColorEntry( "PGPMessageErr",&defaultColor ) );

    defaultColor = QColor( 0x80, 0xFF, 0x80 ); // very light green
    mAppearance.colorList->setColor(
      13, config->readColorEntry( "ColorbarPGP",&defaultColor ) );

    defaultColor = QColor( 0xFF, 0xFF, 0x80 ); // very light yellow
    mAppearance.colorList->setColor(
      14, config->readColorEntry( "ColorbarPlain",&defaultColor ) );

    defaultColor = QColor( 0xFF, 0x40, 0x40 ); // light red
    mAppearance.colorList->setColor(
      15, config->readColorEntry( "ColorbarHTML",&defaultColor ) );

    state = config->readBoolEntry("defaultColors", true );
    mAppearance.customColorCheck->setChecked( state == false ? true : false );
    slotCustomColorSelectionChanged();

    state = config->readBoolEntry( "RecycleQuoteColors", false );
    mAppearance.recycleColorCheck->setChecked( state );
  }

  {
    KConfigGroupSaver saver(config, "Geometry");
    state = config->readBoolEntry( "longFolderList", true );
    mAppearance.longFolderCheck->setChecked( state );

    state = config->readBoolEntry( "nestedMessages", false );
    mAppearance.nestedMessagesCheck->setChecked( state );

    switch( config->readNumEntry( "nestingPolicy", 3 ) )
    {
    case 0:
      mAppearance.rdAlwaysOpen->setChecked( true );
      break;
    case 1:
      mAppearance.rdDefaultOpen->setChecked( true );
      break;
    case 2:
      mAppearance.rdDefaultClosed->setChecked( true );
      break;
    case 3:
      mAppearance.rdUnreadOpen->setChecked( true );
      break;
    default:
     mAppearance.rdUnreadOpen->setChecked( true );
     break;
    }
  }

  {
    KConfigGroupSaver saver(config, "General");
    state = config->readBoolEntry( "showMessageSize", false );
    mAppearance.messageSizeCheck->setChecked( state );
    mAppearance.addressbookCombo->setCurrentItem( config->readNumEntry( "addressbook", 3 )) ;
    mAppearance.addressbookLabel->setText( *mAppearance.addressbookStrings.at( config->readNumEntry( "addressbook", 3 )) );

    QString dateDisplay = config->readEntry( "dateDisplay", "fancyDate" );
    if ( dateDisplay == "ctime" )
      mAppearance.rdDateCtime->setChecked( true );
    else if ( dateDisplay == "localized" )
      mAppearance.rdDateLocalized->setChecked( true );
    else
      mAppearance.rdDateFancy->setChecked( true );
  }
}


void ConfigureDialog::setupComposerPage( void )
{
  KConfig *config = kapp->config();
  mComposer.CurrentLanguage = NULL;
  LanguageItem *l = mComposer.LanguageList;
  while (mComposer.LanguageList)
  {
    l = mComposer.LanguageList;
    mComposer.LanguageList = l->next;
    delete l;
  }
  mComposer.phraseLanguageCombo->clear();

  KConfigGroupSaver saver(config, "General");
  int num = config->readNumEntry("reply-languages",0);
  int currentNr = config->readNumEntry("reply-current-language",0);
  QString itemStr;
  int nr;

  for (int i = num - 1; i >= 0; i--)
  {
    KConfigGroupSaver saver(config, QString("KMMessage #%1").arg(i));
    l = new LanguageItem( config->readEntry("language"),
                          config->readEntry("phrase-reply"),
                          config->readEntry("phrase-reply-all"),
                          config->readEntry("phrase-forward"),
                          config->readEntry("indent-prefix") );
    l->next = mComposer.LanguageList;
    mComposer.LanguageList = l;
    nr = mComposer.phraseLanguageCombo->insertLanguage( l->mLanguage );
    if (currentNr == i) itemStr = mComposer.phraseLanguageCombo->listBox()->
      text(nr);
  }
  mComposer.phraseLanguageCombo->setCurrentItem(mComposer.phraseLanguageCombo->
    listBox()->index(mComposer.phraseLanguageCombo->
    listBox()->findItem(itemStr)));
  mComposer.phraseLanguageCombo->listBox()->setCurrentItem(
    mComposer.phraseLanguageCombo->currentItem() );
  if (num == 0) slotAddNewLanguage( KGlobal::locale()->language() );
  slotLanguageChanged( NULL );

  // editor
    bool state = config->readBoolEntry( "use-external-editor", false );
  mComposer.externalEditorCheck->setChecked( state );
  mComposer.externalEditorEdit->setText( config->readEntry("external-editor", "") );

  {
    KConfigGroupSaver saver(config, "Composer");

    // prefixes
    QStringList prefixList = config->readListEntry("reply-prefixes", ',');
    if (prefixList.count() == 0)
      prefixList.append("Re:");
    mComposer.replyListBox->clear();
    mComposer.replyListBox->insertStringList(prefixList);
    state = config->readBoolEntry("replace-reply-prefix", true );
    mComposer.replaceReplyPrefixCheck->setChecked( state );

    prefixList = config->readListEntry("forward-prefixes", ',');
    if (prefixList.count() == 0)
      prefixList.append("Fwd:");
    mComposer.forwardListBox->clear();
    mComposer.forwardListBox->insertStringList(prefixList);
    state = config->readBoolEntry("replace-forward-prefix", true);
    mComposer.replaceForwardPrefixCheck->setChecked( state );

    state = ( config->readEntry("signature").lower() == "auto" );
    mComposer.autoAppSignFileCheck->setChecked( state );

    state = config->readBoolEntry( "smart-quote", true );
    mComposer.smartQuoteCheck->setChecked(state);

    state = config->readBoolEntry( "pgp-auto-sign", false );
    mComposer.pgpAutoSignatureCheck->setChecked(state);

    state = config->readBoolEntry( "pgp-auto-encrypt", false );
    mComposer.pgpAutoEncryptCheck->setChecked(state);

    state = config->readBoolEntry( "word-wrap", true );
    mComposer.wordWrapCheck->setChecked( state );

    int value = config->readEntry("break-at","78" ).toInt();
    mComposer.wrapColumnSpin->setValue( value );
    slotWordWrapSelectionChanged();

    //charsets
    QStringList charsets = config->readListEntry("pref-charsets");
    for(QStringList::Iterator it = charsets.begin(); it!= charsets.end(); ++it)
    {
       if (*it == "locale")
         *it = QString("%1 (locale)").arg(QCString(KGlobal::locale()->codecForEncoding()->mimeName()).lower());
    }  
    mComposer.charsetListBox->clear();
    mComposer.charsetListBox->insertStringList( charsets );
    mComposer.charsetListBox->setCurrentItem( 0 );

    state = config->readBoolEntry( "force-reply-charset", false );
    mComposer.forceReplyCharsetCheck->setChecked( !state );
  }

  // custom headers
  mComposer.tagList->clear();
  mComposer.currentTagItem = 0;
  mComposer.tagNameEdit->clear();
  mComposer.tagValueEdit->clear();
  mComposer.tagNameEdit->setEnabled(false);
  mComposer.tagValueEdit->setEnabled(false);
  mComposer.tagNameLabel->setEnabled(false);
  mComposer.tagValueLabel->setEnabled(false);

  QString str = config->readEntry( "myMessageIdSuffix", "" );
  mComposer.messageIdSuffixEdit->setText( str );
  state = ( !str.isEmpty() &&
	    config->readBoolEntry("createOwnMessageIdHeaders", false ) );
  mComposer.createOwnMessageIdCheck->setChecked( state );

  QListViewItem *top = 0;

  int count = config->readNumEntry( "mime-header-count", 0 );
  mComposer.tagList->clear();
  for(int i = 0; i < count; i++)
  {
    KConfigGroupSaver saver(config, QString("Mime #%1").arg(i) );
    QString name  = config->readEntry("name", "");
    QString value = config->readEntry("value", "");
    if( name.length() > 0 )
      top = new QListViewItem( mComposer.tagList, top, name, value );
  }
  if (mComposer.tagList->childCount() > 0)
  {
    mComposer.tagList->setCurrentItem(mComposer.tagList->firstChild());
    mComposer.tagList->setSelected(mComposer.tagList->firstChild(), TRUE);
  }
}

void ConfigureDialog::setupSecurityPage( void )
{
  bool state;

  mSecurity.pgpConfig->setValues();

  KConfig *config = kapp->config();
  KConfigGroupSaver saver(config, "General");
  {
    KConfigGroupSaver saver(config, "Reader");
    state = config->readBoolEntry( "htmlMail", false );
    mSecurity.htmlMailCheck->setChecked( state );
    state = config->readBoolEntry( "htmlLoadExternal", false );
    mSecurity.externalReferences->setChecked( state );
  }
  state = config->readBoolEntry("send-receipts", false );
  mSecurity.sendReceiptCheck->setChecked( state );
}


void ConfigureDialog::setupMiscPage( void )
{
  KConfig *config = kapp->config();
  KConfigGroupSaver saver(config, "General");
  int num = 0;

  bool state = config->readBoolEntry("empty-trash-on-exit",false);
  mMisc.emptyTrashCheck->setChecked( state );

  num = config->readNumEntry("when-to-expire", 0);
  mMisc.expBGroup->setButton(num);
  state = config->readBoolEntry("warn-before-expire", true);
  mMisc.warnBeforeExpire->setChecked(state);

  state = config->readBoolEntry("compact-all-on-exit", true );
  mMisc.compactOnExitCheck->setChecked( state );
  state = config->readBoolEntry("confirm-before-empty", true );
  mMisc.emptyFolderConfirmCheck->setChecked( state );

  mMisc.mailboxPrefCombo->setCurrentItem( config->readNumEntry("default-mailbox-format", 0 ) );

  slotExternalEditorSelectionChanged();
}


void ConfigureDialog::installProfile( void )
{
  QListViewItem *item = mAppearance.profileList->selectedItem();
  if( item == 0 )
  {
    return;
  }

  if( item == mAppearance.mListItemDefault )
  {
    mAppearance.font[0] = QFont("helvetica");
    mAppearance.font[1] = QFont("helvetica");
    mAppearance.font[2] = QFont("helvetica");
    mAppearance.font[3] = QFont("helvetica");
    mAppearance.font[4] = QFont("helvetica");
    mAppearance.font[5] = QFont("helvetica");
    mAppearance.customFontCheck->setChecked( true );

    mAppearance.colorList->setColor( 0, kapp->palette().active().base() );
    mAppearance.colorList->setColor( 1, kapp->palette().active().text() );
    mAppearance.colorList->setColor( 2, red );
    mAppearance.colorList->setColor( 3, darkGreen );
    mAppearance.colorList->setColor( 4, darkMagenta );
    mAppearance.colorList->setColor( 5, KGlobalSettings::linkColor() );
    mAppearance.colorList->setColor( 6, KGlobalSettings::visitedLinkColor() );
    mAppearance.colorList->setColor( 7, blue );
    mAppearance.colorList->setColor( 8, red );
    mAppearance.customColorCheck->setChecked( true );

    mAppearance.longFolderCheck->setChecked( true );
    mAppearance.showColorbarCheck->setChecked( false );
    mAppearance.messageSizeCheck->setChecked( true );
    mAppearance.nestedMessagesCheck->setChecked( true );
    mAppearance.rdDateFancy->setChecked( true );
    mSecurity.htmlMailCheck->setChecked( false );
  }
  else if( item == mAppearance.mListItemDefaultHtml )
  {
    mAppearance.font[0] = QFont("helvetica");
    mAppearance.font[1] = QFont("helvetica");
    mAppearance.font[2] = QFont("helvetica");
    mAppearance.font[3] = QFont("helvetica");
    mAppearance.font[4] = QFont("helvetica");
    mAppearance.font[5] = QFont("helvetica");
    mAppearance.customFontCheck->setChecked( true );

    mAppearance.colorList->setColor( 0, kapp->palette().active().base() );
    mAppearance.colorList->setColor( 1, kapp->palette().active().text() );
    mAppearance.colorList->setColor( 2, red );
    mAppearance.colorList->setColor( 3, darkGreen );
    mAppearance.colorList->setColor( 4, darkMagenta );
    mAppearance.colorList->setColor( 5, blue );
    mAppearance.colorList->setColor( 6, red );
    mAppearance.colorList->setColor( 7, blue );
    mAppearance.colorList->setColor( 8, red );
    mAppearance.customColorCheck->setChecked( true );

    mAppearance.longFolderCheck->setChecked( true );
    mAppearance.showColorbarCheck->setChecked( false );
    mAppearance.messageSizeCheck->setChecked( true );
    mAppearance.nestedMessagesCheck->setChecked( true );
    mAppearance.rdDateFancy->setChecked( true );
    mSecurity.htmlMailCheck->setChecked( true );
  }
  else if( item == mAppearance.mListItemContrast )
  {
    mAppearance.font[0] = QFont("helvetica", 14, QFont::Bold);
    mAppearance.font[1] = QFont("helvetica", 14, QFont::Bold);
    mAppearance.font[2] = QFont("helvetica", 14, QFont::Bold);
    mAppearance.font[3] = QFont("helvetica", 14, QFont::Bold);
    mAppearance.font[4] = QFont("helvetica", 14, QFont::Bold);
    mAppearance.font[5] = QFont("helvetica", 14, QFont::Bold);
    mAppearance.customFontCheck->setChecked( true );
    mAppearance.colorList->setColor( 0, QColor("#FAEBD7") );
    mAppearance.colorList->setColor( 1, black );
    mAppearance.colorList->setColor( 2, red );
    mAppearance.colorList->setColor( 3, darkGreen );
    mAppearance.colorList->setColor( 4, darkMagenta );
    mAppearance.colorList->setColor( 5, blue );
    mAppearance.colorList->setColor( 6, red );
    mAppearance.colorList->setColor( 7, blue );
    mAppearance.colorList->setColor( 8, red );
    mAppearance.customColorCheck->setChecked( true );

    mAppearance.longFolderCheck->setChecked( true );
    mAppearance.showColorbarCheck->setChecked( false );
    mAppearance.messageSizeCheck->setChecked( true );
    mAppearance.nestedMessagesCheck->setChecked( true );
    mAppearance.rdDateLocalized->setChecked( true );
    mSecurity.htmlMailCheck->setChecked( false );
  }
  else if( item == mAppearance.mListItemPurist)
  {
    mAppearance.customFontCheck->setChecked( false );

    mAppearance.customColorCheck->setChecked( false );

    mAppearance.longFolderCheck->setChecked( true );
    mAppearance.showColorbarCheck->setChecked( false );
    mAppearance.messageSizeCheck->setChecked( false );
    mAppearance.nestedMessagesCheck->setChecked( false );
    mAppearance.rdDateCtime->setChecked( true );
    mSecurity.htmlMailCheck->setChecked( false );
  }
  else
  {
  }

  slotCustomFontSelectionChanged();
  updateFontSelector();
  slotCustomColorSelectionChanged();
}


//
// Refresh the font selector with the active font string. The current
// font selector setting is ignored.
//
void ConfigureDialog::updateFontSelector( void )
{
  mAppearance.activeFontIndex = mAppearance.fontLocationCombo->currentItem();
  if( mAppearance.activeFontIndex < 0 ) mAppearance.activeFontIndex = 0;

  int i=mAppearance.activeFontIndex;
  mAppearance.fontChooser->setFont( mAppearance.font[i] );
}



void ConfigureDialog::slotDefault( void )
{
  KMessageBox::sorry( this, i18n( "This feature is not working yet." ) );
}

void ConfigureDialog::slotCancelOrClose( void )
{
  QValueList< QGuardedPtr<KMAccount> >::Iterator it;
  for (it = mNewAccounts.begin(); it != mNewAccounts.end(); ++it )
    delete *it;
  QValueList<mModifiedAccountsType*>::Iterator j;
  for (j = mModifiedAccounts.begin(); j != mModifiedAccounts.end(); ++j ) {
    delete (*j)->newAccount;
    delete (*j);
  }

  mAccountsToDelete.clear();
  mNewAccounts.clear();
  mModifiedAccounts.clear();
}

void ConfigureDialog::slotOk( void )
{
  slotDoApply(true);
  mModifiedAccounts.clear();
  mAccountsToDelete.clear();
  mNewAccounts.clear();
  accept();
}


void ConfigureDialog::slotApply( void )
{
  slotDoApply(false);
}


void ConfigureDialog::slotDoApply( bool everything )
{
  KConfig *config = kapp->config();

  int activePage = activePageIndex();
  if( activePage == mIdentity.pageIndex || everything )
  {
    saveActiveIdentity(); // Copy from textfields into list
    mIdentity.identities.exportData();
    if( mIdentity.secondIdentity ) {
	KConfigGroupSaver saver(config, "Composer");
	long mShowHeaders = config->readNumEntry("headers", HDR_STANDARD);
	mShowHeaders |= HDR_IDENTITY;
	config->writeEntry("headers", mShowHeaders);
    }
  }
  if( activePage == mNetwork.pageIndex || everything )
  {
    // Save transports
    KConfigGroupSaver saver(config, "General");
    config->writeEntry("transports", mTransportList.count());
    for (uint i = 1; i <= mTransportList.count(); i++)
      mTransportList.at(i-1)->writeConfig(i);

    // Save common options
    bool sendNow = mNetwork.sendMethodCombo->currentItem() == 0;
    kernel->msgSender()->setSendImmediate( sendNow );
    bool quotedPrintable = mNetwork.messagePropertyCombo->currentItem() == 1;
    kernel->msgSender()->setSendQuotedPrintable( quotedPrintable );
    kernel->msgSender()->writeConfig(FALSE);
    {
      KConfigGroupSaver(config, "Composer");
      bool confirmBeforeSend = mNetwork.confirmSendCheck->isChecked();
      config->writeEntry("confirm-before-send", confirmBeforeSend );
    }
    config->writeEntry( "sendOnCheck",
			mNetwork.sendOutboxCheck->isChecked() );

    // Add accounts marked as new
    QValueList< QGuardedPtr<KMAccount> >::Iterator it;
    for (it = mNewAccounts.begin(); it != mNewAccounts.end(); ++it )
      kernel->acctMgr()->add( *it );
    mNewAccounts.clear();

    // Update accounts that have been modified
    QValueList<mModifiedAccountsType*>::Iterator j;
    for (j = mModifiedAccounts.begin(); j != mModifiedAccounts.end(); ++j )
    {
      (*j)->oldAccount->pseudoAssign( (*j)->newAccount );
      delete (*j)->newAccount;
      delete (*j);
    }
    mModifiedAccounts.clear();

    // Delete accounts marked for deletion
    for (it = mAccountsToDelete.begin(); it != mAccountsToDelete.end(); ++it ) {
      // The old entries will never really disappear, so better at least clear the password:
      (*it)->clearPasswd();
      kernel->acctMgr()->writeConfig();
      if ((it == 0) || (!kernel->acctMgr()->remove(*it)))
        KMessageBox::sorry( this,
			    i18n("Unable to locate account %1").arg((*it)->name()) );
    }
    mAccountsToDelete.clear();

    // Incoming mail
    kernel->acctMgr()->writeConfig(FALSE);
    kernel->cleanupImapFolders();

    // Save Mail notification settings
    config->writeEntry( "beep-on-mail",
			mNetwork.beepNewMailCheck->isChecked() );
    config->writeEntry( "msgbox-on-mail",
			mNetwork.showMessageBoxCheck->isChecked() );
    config->writeEntry( "exec-on-mail",
			mNetwork.mailCommandCheck->isChecked() );
    config->writeEntry( "exec-on-mail-cmd",
			mNetwork.mailCommandEdit->text() );
  }
  if( activePage == mAppearance.pageIndex || everything )
  {
    //
    // Fake a selector change. It will save the current selector setting
    // into the font string with index "mAppearance.activeFontIndex"
    //
    slotFontSelectorChanged( mAppearance.activeFontIndex );

    //
    // If the profile tab page is visible, then install the selected
    // entry. It will the be written to disk below.
    //
    if( mAppearance.profileList->isVisible() )
    {
      installProfile();
    }

    {
      KConfigGroupSaver saver(config, "Fonts");
      bool defaultFonts = !mAppearance.customFontCheck->isChecked();
      config->writeEntry("defaultFonts", defaultFonts );
      config->writeEntry( "body-font",   mAppearance.font[0] );
      config->writeEntry( "list-font",   mAppearance.font[1] );
      config->writeEntry( "list-date-font", mAppearance.font[2] );
      config->writeEntry( "folder-font", mAppearance.font[3] );
      config->writeEntry( "quote1-font", mAppearance.font[4] );
      config->writeEntry( "quote2-font", mAppearance.font[5] );
      config->writeEntry( "quote3-font", mAppearance.font[6] );
//  GS - should this be here?
//    printf("WRITE: %s\n", mAppearance.fontString[3].latin1() );
    }

    {
      KConfigGroupSaver saver(config, "Reader");
      config->writeEntry( "showColorbar",
                          mAppearance.showColorbarCheck->isChecked() );
      bool defaultColors = !mAppearance.customColorCheck->isChecked();
      config->writeEntry("defaultColors", defaultColors );
      //if (!defaultColors)
      //{
	// Don't write color info when we use default colors.
	config->writeEntry("BackgroundColor", mAppearance.colorList->color(0) );
	config->writeEntry("ForegroundColor", mAppearance.colorList->color(1) );
	config->writeEntry("QuoutedText1",    mAppearance.colorList->color(2) );
	config->writeEntry("QuoutedText2",    mAppearance.colorList->color(3) );
	config->writeEntry("QuoutedText3",    mAppearance.colorList->color(4) );
	config->writeEntry("LinkColor",       mAppearance.colorList->color(5) );
	config->writeEntry("FollowedColor",   mAppearance.colorList->color(6) );
	config->writeEntry("NewMessage",      mAppearance.colorList->color(7) );
	config->writeEntry("UnreadMessage",   mAppearance.colorList->color(8) );
	config->writeEntry("FlagMessage",     mAppearance.colorList->color(9) );
	config->writeEntry("PGPMessageOK",    mAppearance.colorList->color(10) );
	config->writeEntry("PGPMessageWarn",  mAppearance.colorList->color(11) );
	config->writeEntry("PGPMessageErr",   mAppearance.colorList->color(12) );
	config->writeEntry("ColorbarPGP",     mAppearance.colorList->color(13) );
	config->writeEntry("ColorbarPlain",   mAppearance.colorList->color(14) );
	config->writeEntry("ColorbarHTML",    mAppearance.colorList->color(15) );
      //}
      bool recycleColors = mAppearance.recycleColorCheck->isChecked();
      config->writeEntry("RecycleQuoteColors", recycleColors );
    }

    {
      KConfigGroupSaver saver(config, "Geometry");
      bool longFolderList = mAppearance.longFolderCheck->isChecked();
      config->writeEntry( "longFolderList", longFolderList );

      bool nestedMessages = mAppearance.nestedMessagesCheck->isChecked();
      config->writeEntry( "nestedMessages", nestedMessages );

      int threadPolicy = 3;
      if( mAppearance.rdAlwaysOpen->isChecked() )
	threadPolicy = 0;
      else if( mAppearance.rdDefaultOpen->isChecked() )
	threadPolicy = 1;
      else if( mAppearance.rdDefaultClosed->isChecked() )
       threadPolicy = 2;

      config->writeEntry( "nestingPolicy", threadPolicy );
    }

    {
      KConfigGroupSaver saver(config, "General");
      bool messageSize = mAppearance.messageSizeCheck->isChecked();
      config->writeEntry( "showMessageSize", messageSize );
      config->writeEntry( "addressbook", mAppearance.addressbookCombo->currentItem() );

      if ( mAppearance.rdDateCtime->isChecked() )
        config->writeEntry( "dateDisplay", "ctime" );
      else if ( mAppearance.rdDateLocalized->isChecked() )
        config->writeEntry( "dateDisplay", "localized" );
      else if ( mAppearance.rdDateFancy->isChecked() )
        config->writeEntry( "dateDisplay", "fancyDate" );
    }
  }
  if( activePage == mComposer.pageIndex || everything )
  {
    slotSaveOldPhrases();
    LanguageItem *l = mComposer.LanguageList;
    int languageCount = 0, currentNr = 0;
    while (l)
    {
      if (l == mComposer.CurrentLanguage) currentNr = languageCount;
      KConfigGroupSaver saver(config, QString("KMMessage #%1").arg(languageCount));
      config->writeEntry( "language", l->mLanguage );
      config->writeEntry( "phrase-reply", l->mReply );
      config->writeEntry( "phrase-reply-all", l->mReplyAll );
      config->writeEntry( "phrase-forward", l->mForward );
      config->writeEntry( "indent-prefix", l->mIndentPrefix );
      l = l->next;
      languageCount++;
    }

    {
      KConfigGroupSaver saver(config, "General");
      config->writeEntry("reply-languages", languageCount);
      config->writeEntry("reply-current-language", currentNr);

      config->writeEntry( "use-external-editor",
			  mComposer.externalEditorCheck->isChecked() );
      config->writeEntry( "external-editor",
			  mComposer.externalEditorEdit->text() );
    }

    {
      KConfigGroupSaver saver(config, "Composer");

      int prefixCount = mComposer.replyListBox->count();
      QStringList prefixList;
      int j;
      for (j = 0; j < prefixCount; j++)
	prefixList.append( mComposer.replyListBox->item( j )->text() );
      config->writeEntry("reply-prefixes", prefixList);
      config->writeEntry("replace-reply-prefix",
			 mComposer.replaceReplyPrefixCheck->isChecked() );
      prefixList.clear();
      prefixCount = mComposer.forwardListBox->count();
      for (j = 0; j < prefixCount; j++)
	prefixList.append( mComposer.forwardListBox->item( j )->text() );
      config->writeEntry("forward-prefixes", prefixList);
      config->writeEntry("replace-forward-prefix",
			 mComposer.replaceForwardPrefixCheck->isChecked() );

      QStringList charsetList;
      int charsetCount = mComposer.charsetListBox->count();
      for (j = 0; j < charsetCount; j++)
      {
        QString charset = mComposer.charsetListBox->item( j )->text();
        if (charset.endsWith("(locale)"))
           charset = "locale";
	charsetList.append( charset );
      }
      config->writeEntry("pref-charsets", charsetList);

      bool autoSignature = mComposer.autoAppSignFileCheck->isChecked();
      config->writeEntry("signature", autoSignature ? "auto" : "manual" );
      config->writeEntry("smart-quote", mComposer.smartQuoteCheck->isChecked() );
      config->writeEntry("pgp-auto-sign",
			 mComposer.pgpAutoSignatureCheck->isChecked() );
      config->writeEntry("pgp-auto-encrypt",
			 mComposer.pgpAutoEncryptCheck->isChecked() );
      config->writeEntry("word-wrap", mComposer.wordWrapCheck->isChecked() );
      config->writeEntry("break-at", mComposer.wrapColumnSpin->value() );

      config->writeEntry("force-reply-charset",
			 !mComposer.forceReplyCharsetCheck->isChecked() );
    }
    
    { // "headers" tab:
      KConfigGroupSaver saver(config, "General");
      config->writeEntry( "createOwnMessageIdHeaders",
			  mComposer.createOwnMessageIdCheck->isChecked() );
      config->writeEntry( "myMessageIdSuffix",
			  mComposer.messageIdSuffixEdit->text() );
      
      int numValidEntry = 0;
      int numEntry = mComposer.tagList->childCount();
      QListViewItem *item = mComposer.tagList->firstChild();
      for (int i = 0; i < numEntry; i++) {
	KConfigGroupSaver saver(config, QString("Mime #%1").arg(i));
	if( item->text(0).length() > 0 ) {
	  config->writeEntry( "name",  item->text(0) );
	  config->writeEntry( "value", item->text(1) );
	  numValidEntry++;
	}
	item = item->nextSibling();
      }
      config->writeEntry("mime-header-count", numValidEntry );
    }
  }
  if( activePage == mSecurity.pageIndex || everything )
  {
    mSecurity.pgpConfig->applySettings();
    {
      KConfigGroupSaver saver(config, "Reader");
      bool htmlMail = mSecurity.htmlMailCheck->isChecked();
      config->writeEntry( "htmlMail", htmlMail );
      config->writeEntry( "htmlLoadExternal", mSecurity.
                          externalReferences->isChecked() );
    }
    KConfigGroupSaver saver(config, "General");
    config->writeEntry( "send-receipts",
			mSecurity.sendReceiptCheck->isChecked() );
  }
  if( activePage == mMisc.pageIndex || everything )
  {
    KConfigGroupSaver saver(config, "General");
    config->writeEntry( "empty-trash-on-exit",
			mMisc.emptyTrashCheck->isChecked() );
    config->writeEntry( "compact-all-on-exit",
			mMisc.compactOnExitCheck->isChecked() );
    config->writeEntry( "confirm-before-empty",
			mMisc.emptyFolderConfirmCheck->isChecked() );

    config->writeEntry( "default-mailbox-format",
                        mMisc.mailboxPrefCombo->currentItem() );


    // There's got to be a better way to handle radio buttons...
    if (mMisc.manualExpiry->isChecked()) {
      config->writeEntry("when-to-expire", expireManual);
    } else if (mMisc.expireAtExit->isChecked()) {
      config->writeEntry("when-to-expire", expireAtExit);
    } else if (mMisc.expireAtStart->isChecked()) {
      config->writeEntry("when-to-expire", expireAtStart);
    } else if (mMisc.expireDaily->isChecked()) {
      config->writeEntry("when-to-expire", expireDaily);
    } else if (mMisc.expireWeekly->isChecked()) {
      config->writeEntry("when-to-expire", expireWeekly);
    } else {
      // If we don't know, make expiry manual (just in case).
      config->writeEntry("when-to-expire", expireManual);
    }
    config->writeEntry("warn-before-expire", mMisc.warnBeforeExpire->isChecked());

  }

#ifdef SCORING
  kdDebug(5006) << "KMScoringManager::globalScoringManager()->save();" << endl;
  KMScoringManager::globalScoringManager()->save();
#endif

  //
  // Make other components read the new settings
  //
  KMMessage::readConfig();
  kernel->kbp()->busy(); // this can take some time when a large folder is open
  QPtrListIterator<KMainWindow> it(*KMainWindow::memberList);
  for( it.toFirst(); it.current(); ++it )
  {
    if (it.current()->inherits("KMTopLevelWidget"))
    {
      ((KMTopLevelWidget*)it.current())->readConfig();
    }
  }
  kernel->kbp()->idle();
}






void ConfigureDialog::saveActiveIdentity()
{
  if ( mIdentity.activeIdentity.isEmpty() ) return;

  kdDebug() << "mIdentity.activeIdentity == \""
	    << mIdentity.activeIdentity << "\"" << endl;
  // hits an assert if mIdentity.activeIdentity isn't in the list:
  IdentityEntry & entry
    = mIdentity.identities.getByName( mIdentity.activeIdentity );

  // "General" tab:
  entry.setFullName( mIdentity.nameEdit->text() );
  entry.setOrganization( mIdentity.organizationEdit->text() );
  entry.setEmailAddress( mIdentity.emailEdit->text() );
  // "Advanced" tab:
  entry.setPgpIdentity( mIdentity.pgpIdentityLabel->text() );
  entry.setReplyToAddress( mIdentity.replytoEdit->text() );
  entry.setTransport( ( mIdentity.transportCheck->isChecked() ) ?
	        mIdentity.transportCombo->currentText() : QString::null );
  entry.setFcc( mIdentity.fccCombo->getFolder()->idString() );
  // "Signature" tab:
  if ( mIdentity.signatureEnabled->isChecked() ) {
    switch ( mIdentity.signatureSourceCombo->currentItem() ) {
    case 0: // signature from file
      entry.setSignatureFileName( mIdentity.signatureFileRequester->url() );
      entry.setUseSignatureFile( true );
      entry.setSignatureFileIsAProgram( false );
      break;
    case 1: // signature from command
      entry.setSignatureFileName( mIdentity.signatureCommandRequester->url() );
      entry.setUseSignatureFile( true );
      entry.setSignatureFileIsAProgram( true );
      break;
    case 2: // inline specified
      entry.setSignatureFileName( QString::null );
      entry.setUseSignatureFile( false );
      entry.setSignatureFileIsAProgram( false );
      break;
    default:
      kdFatal(5006) << "mIdentity.signatureSourceCombo->currentItem() > 2"
		    << endl;
    }
  } else {
    // not enabled - fake empty data file:
    entry.setSignatureFileName( QString::null );
    entry.setUseSignatureFile( true );
    entry.setSignatureFileIsAProgram( false );
  }
  entry.setSignatureInlineText( mIdentity.signatureTextEdit->text() );
}


void ConfigureDialog::setIdentityInformation( const QString &identity )
{
  if( mIdentity.activeIdentity == identity ) return;

  //
  // 1. Save current settings to the list
  //
  saveActiveIdentity();

  mIdentity.activeIdentity = identity;

  //
  // 2. Display the new settings
  //
  if( mIdentity.identities.names().findIndex( mIdentity.activeIdentity ) < 0 ) {
    // new entry: clear the widgets
    // "General" tab:
    mIdentity.nameEdit->clear();
    mIdentity.organizationEdit->clear();
    mIdentity.emailEdit->clear();
    // "Advanced" tab:
    mIdentity.replytoEdit->clear();
    mIdentity.pgpIdentityLabel->clear();
    mIdentity.transportCheck->setChecked( false );
    mIdentity.transportCombo->clearEdit();
    mIdentity.fccCombo->setFolder( kernel->sentFolder() );
    // "Signature" tab:
    mIdentity.signatureEnabled->setChecked( false );
    mIdentity.signatureSourceCombo->setCurrentItem( 0 );
    mIdentity.signatureFileRequester->clear();
    mIdentity.signatureCommandRequester->clear();
    mIdentity.signatureTextEdit->clear();
  } else {
    // existing entry: set from saved values
    const IdentityEntry & entry =
      mIdentity.identities.getByName( mIdentity.activeIdentity );
    // "General" tab:
    mIdentity.nameEdit->setText( entry.fullName() );
    mIdentity.organizationEdit->setText( entry.organization() );
    mIdentity.emailEdit->setText( entry.emailAddress() );
    // "Advanced" tab:
    mIdentity.pgpIdentityLabel->setText( entry.pgpIdentity() );
    mIdentity.replytoEdit->setText( entry.replyToAddress() );
    mIdentity.transportCheck->setChecked( !entry.transport().isEmpty() );
    mIdentity.transportCombo->setEditText( entry.transport() );
    mIdentity.transportCombo->setEnabled( !entry.transport().isEmpty() );
    if ( entry.fcc().isEmpty() )
      mIdentity.fccCombo->setFolder( kernel->sentFolder() );
    else
      mIdentity.fccCombo->setFolder( entry.fcc() );
    // "Signature" tab:
    if ( entry.useSignatureFile() ) {
      if ( entry.signatureFileName().stripWhiteSpace().isEmpty() ) {
	// disable signatures:
	mIdentity.signatureEnabled->setChecked( false );
	mIdentity.signatureFileRequester->clear();
	mIdentity.signatureCommandRequester->clear();
	mIdentity.signatureSourceCombo->setCurrentItem( 0 );
      } else {
	mIdentity.signatureEnabled->setChecked( true );
	if ( entry.signatureFileIsAProgram() ) {
	  // use file && file is a program
	  mIdentity.signatureFileRequester->clear();
	  mIdentity.signatureCommandRequester->setURL( entry.signatureFileName() );
	  mIdentity.signatureSourceCombo->setCurrentItem( 1 );
	} else {
	  // use file && file is data file
	  mIdentity.signatureFileRequester->setURL( entry.signatureFileName() );
	  mIdentity.signatureCommandRequester->clear();
	  mIdentity.signatureSourceCombo->setCurrentItem( 0 );
	}
      }
    } else {
      mIdentity.signatureEnabled->setChecked(
               !entry.signatureInlineText().isEmpty() );
      // !use file, specify inline
      mIdentity.signatureFileRequester->clear();
      mIdentity.signatureCommandRequester->clear();
      mIdentity.signatureSourceCombo->setCurrentItem( 2 );
    }
    mIdentity.signatureTextEdit->setText( entry.signatureInlineText() );
  }
}


void ConfigureDialog::slotNewIdentity()
{
  //
  // First: Save current setting to the list. In the dialog box we
  // can choose to copy from the list so it must be synced.
  //
  saveActiveIdentity();

  //
  // Make and open the dialog
  //
  NewIdentityDialog dialog( mIdentity.identities.names(), this, "new", true );

  if( dialog.exec() == QDialog::Accepted ) {
    QString identityName = dialog.identityName().stripWhiteSpace();
    if( !identityName.isEmpty() )
    {
      if ( mIdentity.identities.count() == 1 )
	mIdentity.secondIdentity = true;

      //
      // Construct a new IdentityEntry:
      //
      IdentityEntry entry;
      switch ( dialog.duplicateMode() ) {
      case NewIdentityDialog::ExistingEntry:
	// let's hope this uses operator=
	entry = mIdentity.identities[ dialog.duplicateIdentity() ];
	break;
      case NewIdentityDialog::ControlCenter:
	entry = IdentityEntry::fromControlCenter();
	break;
      case NewIdentityDialog::Empty:
      default: ;
      }
      entry.setIdentityName( identityName );
      // add the new entry and sort the list:
      mIdentity.identities << entry;
      mIdentity.identities.sort();

      //
      // Set the modified identity list as the valid list in the
      // identity combo and make the new identity the current item.
      //
      mIdentity.identityCombo->clear();
      mIdentity.identityCombo->insertStringList( mIdentity.identities.names() );
      mIdentity.identityCombo->setCurrentItem(
		 mIdentity.identities.names().findIndex( identityName ) );

      slotIdentitySelectorChanged(); // ### needed?
    }
  }
}


void ConfigureDialog::slotRenameIdentity()
{
  bool ok;
  QString oldName = mIdentity.identityCombo->currentText();
  QString message = i18n("Rename identity \"%1\" to").arg( oldName );
  do {
    QString newName = KLineEditDlg::getText( i18n("Rename identity"),
			     message, oldName, &ok, this ).stripWhiteSpace();
    
    if ( ok && newName != oldName ) {
      if ( newName.isEmpty() ) {
	KMessageBox::error( this, i18n("You must specify an identity name!") );
	continue;
      }
      if ( mIdentity.identities.names().contains( newName ) ) {
	QString errorMessage =
	  i18n("An identity named \"%1\" already exists!\n"
	       "Please choose another name.").arg( newName );
	KMessageBox::error( this, errorMessage );
	continue;
      }

      // change the name
      int index = mIdentity.identityCombo->currentItem();
      assert( index < mIdentity.identities.count() );
      IdentityEntry & entry = mIdentity.identities[ index ];
      assert( entry.identityName() == oldName );
      entry.setIdentityName( newName );
      
      // resort the list:
      mIdentity.identities.sort();

      // and update the view:
      mIdentity.activeIdentity = newName;
      QStringList identityNames = mIdentity.identities.names();
      mIdentity.identityCombo->clear();
      mIdentity.identityCombo->insertStringList( identityNames );
      mIdentity.identityCombo->setCurrentItem( identityNames.findIndex( newName ) );
      slotIdentitySelectorChanged(); // ### needed?
    }
  } while ( false );
}


void ConfigureDialog::slotRemoveIdentity()
{
  int currentItem = mIdentity.identityCombo->currentItem();
  if( currentItem > 0 ) { // Item 0 is the default and cannot be removed.
    QString msg = i18n("<qt>Do you really want to remove identity\n"
		       "<b>%1</b>?</qt>")
      .arg( mIdentity.identityCombo->currentText() );

    if( KMessageBox::warningYesNo( this, msg ) == KMessageBox::Yes ) {
      mIdentity.identities.remove( // hasn't a ::remove(int)
         mIdentity.identities.getByName( mIdentity.identityCombo->currentText() ) );
      mIdentity.identityCombo->removeItem( currentItem );
      mIdentity.identityCombo->setCurrentItem( currentItem-1 );
      // prevent attempt to save removed identity:
      mIdentity.activeIdentity = QString::null;
      slotIdentitySelectorChanged(); // ### needed?
    }
  }
}


void ConfigureDialog::slotIdentitySelectorChanged()
{
  int currentItem = mIdentity.identityCombo->currentItem();
  mIdentity.removeIdentityButton->setEnabled( currentItem != 0 );
  mIdentity.renameIdentityButton->setEnabled( currentItem != 0 );
  setIdentityInformation( mIdentity.identityCombo->currentText() );
}


void ConfigureDialog::slotChangeDefaultPGPKey( void )
{
  Kpgp::Module *pgp;
  
  if ( !(pgp = Kpgp::Module::getKpgp()) )
    return;

  QCString keyID = pgp->selectDefaultKey();

  if ( !keyID.isEmpty() )
    mIdentity.pgpIdentityLabel->setText( keyID ); 
}


void ConfigureDialog::slotEnableSignatureEditButton( const QString &filename )
{
  mIdentity.signatureEditButton->setDisabled( filename.stripWhiteSpace().isEmpty() );
}


void ConfigureDialog::slotSignatureEdit( void )
{
  QString fileName = mIdentity.signatureFileRequester->url().stripWhiteSpace();
  if( fileName.isEmpty() )
  {
    KMessageBox::error( this, i18n("You must specify a filename") );
    return;
  }

  QFileInfo fileInfo( fileName );
  if( fileInfo.isDir() )
  {
    QString msg = i18n("You have specified a directory\n\n%1").arg(fileName);
    KMessageBox::error( this, msg );
    return;
  }

  if( !fileInfo.exists() )
  {
    // Create the file first
    QFile file( fileName );
    if( !file.open( IO_ReadWrite ) )
    {
      QString msg = i18n("Unable to create new file at\n\n%1").arg(fileName);
      KMessageBox::error( this, msg );
      return;
    }
  }

  QString cmdline = QString::fromLatin1(DEFAULT_EDITOR_STR);

  QString argument = "\"" + fileName + "\"";
  ApplicationLaunch kl(cmdline.replace(QRegExp("\\%f"), argument ));
  kl.run();
}


//
// Network page
//

void ConfigureDialog::slotTransportSelected()
{
  QListViewItem *cur = mNetwork.transportList->currentItem();
  mNetwork.modifyTransportButton->setEnabled( cur );
  mNetwork.removeTransportButton->setEnabled( cur );
  mNetwork.transportDownButton->setEnabled( cur && cur->itemBelow() );
  mNetwork.transportUpButton->setEnabled( cur && cur->itemAbove() );
}


void ConfigureDialog::slotUpdateTransportCombo()
{
  QString content = mIdentity.transportCombo->currentText();
  mIdentity.transportCombo->clear();
  for (KMTransportInfo *ti = mTransportList.first(); ti;
    ti = mTransportList.next())
      mIdentity.transportCombo->insertItem(ti->name);
  mIdentity.transportCombo->setEditText( content );
}


void ConfigureDialog::slotAddTransport()
{
  KMTransportSelDlg transportSelectorDialog( this );
  if( transportSelectorDialog.exec() != QDialog::Accepted )
  {
    return;
  }

  KMTransportInfo *transportInfo = new KMTransportInfo();
  transportInfo->type
    = (transportSelectorDialog.selected() == 1) ? "sendmail" : "smtp";

  if (transportInfo->type == "sendmail")
  {
    transportInfo->name = i18n("Sendmail");
    transportInfo->host = _PATH_SENDMAIL;
  }

  KMTransportDialog *dialog = new KMTransportDialog( transportInfo, this );
  dialog->setCaption( i18n("Add transport") );

  QStringList transportNames;
  for (KMTransportInfo *ti = mTransportList.first(); ti;
    ti = mTransportList.next())
      transportNames.append(ti->name);

  if( dialog->exec() == QDialog::Accepted )
  {
    QString transportName = transportInfo->name;
    int suffix = 1;
    while (transportNames.find( transportInfo->name ) != transportNames.end())
    {
      transportInfo->name = QString( "%1 %2" ).arg( transportName )
      .arg( suffix );
      suffix++;
    }
    mTransportList.append(transportInfo);
    QListViewItem *lastItem = mNetwork.transportList->firstChild();
    if (lastItem)
      while (lastItem->nextSibling()) lastItem = lastItem->nextSibling();
    new QListViewItem( mNetwork.transportList, lastItem, transportInfo->name,
      (lastItem) ? transportInfo->type :
      transportInfo->type + " " + i18n("(Default)") );
  } else {
    delete transportInfo;
  }

  delete dialog;
  slotUpdateTransportCombo();
}


void ConfigureDialog::slotModifySelectedTransport()
{
  QListViewItem *item = mNetwork.transportList->currentItem();
  if (!item) return;
  KMTransportInfo *ti;
  for (ti = mTransportList.first(); ti; ti = mTransportList.next())
    if (ti->name == item->text(0)) break;
  if (!ti) return;

  KMTransportDialog *dialog = new KMTransportDialog( ti, this );
  dialog->setCaption( i18n("Modify transport") );

  if (dialog->exec() == QDialog::Accepted )
  {
    QStringList transportNames;
    for (KMTransportInfo *ti2 = mTransportList.first(); ti2;
      ti2 = mTransportList.next())
        if (ti2 != ti) transportNames.append(ti2->name);

    QString transportName = ti->name;
    int suffix = 1;
    while (transportNames.find( ti->name ) != transportNames.end())
    {
      ti->name = QString( "%1 %2" ).arg( transportName ).arg( suffix );
      suffix++;
    }
    item->setText(0, ti->name);
  }

  delete dialog;
  slotUpdateTransportCombo();
}


void ConfigureDialog::slotRemoveSelectedTransport()
{
  QListViewItem *item = mNetwork.transportList->currentItem();
  if (!item) return;
  KMTransportInfo *ti;
  for (ti = mTransportList.first(); ti; ti = mTransportList.next())
    if (ti->name == item->text(0)) break;
  if (!ti) return;

  QListViewItem *item2 = item->itemBelow();
  if (!item2) item2 = item->itemAbove();
  if (item2) mNetwork.transportList->setCurrentItem(item2);
  mNetwork.transportList->setSelected(item2, TRUE);
  mNetwork.transportList->removeItem(item);
  mTransportList.remove(ti);
  slotUpdateTransportCombo();
}


void ConfigureDialog::slotTransportUp()
{
  QListViewItem *item = mNetwork.transportList->currentItem();
  if (!item) return;
  QListViewItem *above = item->itemAbove();
  if (!above) return;

  KMTransportInfo *ti, *ti2 = NULL;
  int i = 0;
  for (ti = mTransportList.first(); ti;
    ti2 = ti, ti = mTransportList.next(), i++)
      if (ti->name == item->text(0)) break;
  if (!ti || !ti2) return;
  ti = mTransportList.take(i);
  mTransportList.insert(i-1, ti);

  above->setText(0, ti->name);
  above->setText(1, ti->type + ((above->itemAbove()) ? "" :
    (" " + i18n("(Default)"))));
  item->setText(0, ti2->name);
  item->setText(1, ti2->type);

  mNetwork.transportList->setCurrentItem(above);
  mNetwork.transportList->setSelected(above, TRUE);
}


void ConfigureDialog::slotTransportDown()
{
  QListViewItem *item = mNetwork.transportList->currentItem();
  if (!item) return;
  QListViewItem *below = item->itemBelow();
  if (!below) return;

  KMTransportInfo *ti, *ti2 = NULL;
  int i = 0;
  for (ti = mTransportList.first(); ti; ti = mTransportList.next(), i++)
    if (ti->name == item->text(0)) break;
  ti2 = mTransportList.next();
  if (!ti || !ti2) return;
  ti = mTransportList.take(i);
  mTransportList.insert(i+1, ti);

  item->setText(0, ti2->name);
  item->setText(1, ti2->type + ((item->itemAbove()) ? "" :
    (" " + i18n("(Default)"))));
  below->setText(0, ti->name);
  below->setText(1, ti->type);

  mNetwork.transportList->setCurrentItem(below);
  mNetwork.transportList->setSelected(below, TRUE);
}


void ConfigureDialog::slotAccountSelected( void )
{
  mNetwork.modifyAccountButton->setEnabled( true );
  mNetwork.removeAccountButton->setEnabled( true );
}

QStringList ConfigureDialog::occupiedNames( void )
{
  QStringList accountNames = kernel->acctMgr()->getAccounts();

  QValueList<mModifiedAccountsType*>::Iterator k;
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

  QValueList<mModifiedAccountsType*>::Iterator j;
  for (j = mModifiedAccounts.begin(); j != mModifiedAccounts.end(); ++j )
    accountNames += (*j)->newAccount->name();

  return accountNames;
}

void ConfigureDialog::slotAddAccount( void )
{
  KMAcctSelDlg accountSelectorDialog( this );
  if( accountSelectorDialog.exec() != QDialog::Accepted )
  {
    return;
  }

  const char *accountType = 0;
  switch( accountSelectorDialog.selected() )
  {
    case 0:
      accountType = "local";
    break;

    case 1:
      accountType = "pop";
    break;

    case 2:
      accountType = "imap";
    break;

    case 3:
      accountType = "maildir";
    break;

    default:
      KMessageBox::sorry( this, i18n("Unknown account type selected") );
      return;
    break;
  }

  KMAccount *account = kernel->acctMgr()->create(accountType,i18n("Unnamed"));
  if( account == 0 )
  {
    KMessageBox::sorry( this, i18n("Unable to create account") );
    return;
  }

  account->init(); // fill the account fields with good default values

  AccountDialog *dialog =
    new AccountDialog( account, mIdentity.identities.names(), this);
  dialog->setCaption( i18n("Add account") );

  QStringList accountNames = occupiedNames();

  if( dialog->exec() == QDialog::Accepted )
  {
    QString accountName = account->name();
    int suffix = 1;
    while (accountNames.find( account->name() ) != accountNames.end()) {
      account->setName( QString( "%1 %2" ).arg( accountName ).arg( suffix ));
      ++suffix;
    }

    QListViewItem *after = mNetwork.accountList->firstChild();
    while (after && after->nextSibling())
      after = after->nextSibling();

    QListViewItem *listItem =
      new QListViewItem(mNetwork.accountList, after,
			account->name(), account->type());
    if( account->folder() )
      listItem->setText( 2, account->folder()->label() );

    mNewAccounts.append( account );
  }
  else
  {
    delete account;
  }
  delete dialog;
}



void ConfigureDialog::slotModifySelectedAccount( void )
{
  QListViewItem *listItem = mNetwork.accountList->selectedItem();
  if( listItem == 0 )
  {
    return;
  }

  KMAccount *account = 0;
  QValueList<mModifiedAccountsType*>::Iterator j;
  for (j = mModifiedAccounts.begin(); j != mModifiedAccounts.end(); ++j )
    if ((*j)->newAccount->name() == listItem->text(0))
    {
      account = (*j)->newAccount;
      break;
    }

  if (!account) {
    QValueList< QGuardedPtr<KMAccount> >::Iterator it;
    for (it = mNewAccounts.begin(); it != mNewAccounts.end(); ++it )
      if ((*it)->name() == listItem->text(0))
      {
	account = *it;
	break;
      }

    if (!account) {
      account = kernel->acctMgr()->find( listItem->text(0) );
      if( account == 0 )
      {
        KMessageBox::sorry( this, i18n("Unable to locate account") );
        return;
      }

      mModifiedAccountsType *mod = new mModifiedAccountsType;
      mod->oldAccount = account;
      mod->newAccount = kernel->acctMgr()->create(account->type(),account->name());
      mod->newAccount->pseudoAssign(account);
      mModifiedAccounts.append( mod );
      account = mod->newAccount;
    }

    if( account == 0 )
    {
      KMessageBox::sorry( this, i18n("Unable to locate account") );
      return;
    }
  }

  QStringList accountNames = occupiedNames();
  accountNames.remove( account->name() );
  AccountDialog *dialog =
    new AccountDialog( account, mIdentity.identities.names(), this);
  dialog->setCaption( i18n("Modify account") );
  if( dialog->exec() == QDialog::Accepted )
  {
    QString accountName = account->name();
    int suffix = 1;
    while (accountNames.find( account->name() ) != accountNames.end()) {
      account->setName( QString( "%1 %2" ).arg( accountName ).arg( suffix ));
      ++suffix;
    }

    listItem->setText( 0, account->name() );
    listItem->setText( 1, account->type() );
    if( account->folder() )
      listItem->setText( 2, account->folder()->label() );
  }
  delete dialog;
}



void ConfigureDialog::slotRemoveSelectedAccount( void )
{
  QListViewItem *listItem = mNetwork.accountList->selectedItem();
  if( listItem == 0 )
  {
    return;
  }

  KMAccount *acct = 0;
  QValueList<mModifiedAccountsType*>::Iterator j;
  for (j = mModifiedAccounts.begin(); j != mModifiedAccounts.end(); ++j )
    if ((*j)->newAccount->name() == listItem->text(0)) {
      acct = (*j)->oldAccount;
      mAccountsToDelete.append( acct );
      mModifiedAccounts.remove( j );
      break;
    }
  QValueList< QGuardedPtr<KMAccount> >::Iterator it;
  if (!acct) {
    for (it = mNewAccounts.begin(); it != mNewAccounts.end(); ++it )
      if ((*it)->name() == listItem->text(0)) {
	acct = *it;
	mNewAccounts.remove( it );
	break;
      }
  }
  if (!acct) {
    acct = kernel->acctMgr()->find( listItem->text(0) );
    if (acct)
      mAccountsToDelete.append( acct );
  }
  if ( acct == 0 )
  {
    KMessageBox::sorry( this,  i18n("Unable to locate account %1").arg(listItem->text(0)) );
    return;
  }

  mNetwork.accountList->takeItem( listItem );
  if( mNetwork.accountList->childCount() == 0 )
  {
    mNetwork.modifyAccountButton->setEnabled( false );
    mNetwork.removeAccountButton->setEnabled( false );
  }
  else
  {
    mNetwork.accountList->setSelected(mNetwork.accountList->firstChild(),true);
  }
}


void ConfigureDialog::slotCustomFontSelectionChanged( void )
{
  bool flag = mAppearance.customFontCheck->isChecked();
  mAppearance.fontLocationLabel->setEnabled( flag );
  mAppearance.fontLocationCombo->setEnabled( flag );
  mAppearance.fontChooser->setEnabled( flag );
}


void ConfigureDialog::slotFontSelectorChanged( int index )
{
  if( index < 0 || index >= mAppearance.fontLocationCombo->count() )
  {
    return; // Should never happen, but it is better to check.
  }

  //
  // Save current fontselector setting before we install the new
  //
  if( mAppearance.activeFontIndex >= 0 )
  {
    mAppearance.font[mAppearance.activeFontIndex] =
      mAppearance.fontChooser->font();
  }
  mAppearance.activeFontIndex = index;

  //
  // Display the new setting
  //
  mAppearance.fontChooser->setFont(mAppearance.font[index]);

  //
  // Disable Family and Size list if we have selected a qoute font
  //
  bool enable = index != 4 && index != 5 && index != 6;
  mAppearance.fontChooser->enableColumn(
    KFontChooser::FamilyList|KFontChooser::SizeList, enable );
  enable = enable && index != 0;
  mAppearance.fontChooser->enableColumn( KFontChooser::CharsetList, enable );
}

void ConfigureDialog::slotAddressbookSelectorChanged( int index )
{
  mAppearance.addressbookLabel->setText(*mAppearance.addressbookStrings.at(index));
}

void ConfigureDialog::slotCustomColorSelectionChanged( void )
{
  bool state = mAppearance.customColorCheck->isChecked();
  mAppearance.colorList->setEnabled( state );
  if (state && (mAppearance.colorList->currentItem() < 0))
     mAppearance.colorList->setCurrentItem(0);
  mAppearance.recycleColorCheck->setEnabled( state );
}

void ConfigureDialog::slotNewLanguage( void )
{
  NewLanguageDialog *dialog = new NewLanguageDialog( this, "new", true,
    mComposer.LanguageList );
  int result = dialog->exec();
  if ( result == QDialog::Accepted ) slotAddNewLanguage( dialog->language() );
  delete dialog;
}

void ConfigureDialog::slotAddNewLanguage( const QString& lang )
{
  mComposer.phraseLanguageCombo->setCurrentItem( mComposer.
    phraseLanguageCombo->insertLanguage(lang) );
  KLocale locale("kmail");
  locale.setLanguage( lang );
  LanguageItem *l = new LanguageItem( lang,
    locale.translate("On %D, you wrote:"),
    locale.translate("On %D, %F wrote:"),
    locale.translate("Forwarded Message"),
    locale.translate(">%_") );
  l->next = mComposer.LanguageList;
  mComposer.LanguageList = l;
  slotLanguageChanged( NULL );
}

void ConfigureDialog::slotRemoveLanguage( void )
{
  LanguageItem *l = mComposer.LanguageList, *l2 = NULL;
  while (l && l->mLanguage != mComposer.phraseLanguageCombo->language())
  { l2 = l; l = l->next; }
  if (l)
  {
    if (l2 == NULL) {
      mComposer.LanguageList = l->next;
    } else l2->next = l->next;
    delete l;
    mComposer.CurrentLanguage = NULL;
  }
  mComposer.phraseLanguageCombo->removeItem( mComposer.phraseLanguageCombo->
    currentItem() );
  slotLanguageChanged( NULL );
}

void ConfigureDialog::slotSaveOldPhrases( void )
{
  LanguageItem *l = mComposer.CurrentLanguage;
  if (l)
  {
    l->mReply = mComposer.phraseReplyEdit->text();
    l->mReplyAll = mComposer.phraseReplyAllEdit->text();
    l->mForward  = mComposer.phraseForwardEdit->text();
    l->mIndentPrefix = mComposer.phraseindentPrefixEdit->text();
  }
}

void ConfigureDialog::slotLanguageChanged( const QString& )
{
  QString s = mComposer.phraseLanguageCombo->language();
  slotSaveOldPhrases();
  LanguageItem *l = mComposer.LanguageList;
  while (l && l->mLanguage != s) l = l->next;
  if (l)
  {
    mComposer.phraseReplyEdit->setText( l->mReply );
    mComposer.phraseReplyAllEdit->setText( l->mReplyAll );
    mComposer.phraseForwardEdit->setText( l->mForward );
    mComposer.phraseindentPrefixEdit->setText( l->mIndentPrefix );
  }
  mComposer.CurrentLanguage = l;
  mComposer.removeButton->setEnabled( mComposer.phraseLanguageCombo->
    count() > 1 );
}

void ConfigureDialog::slotWordWrapSelectionChanged( void )
{
  mComposer.wrapColumnSpin->setEnabled(mComposer.wordWrapCheck->isChecked());
}

//
// Composer page
//

void ConfigureDialog::slotAddReplyPrefix( void )
{
  KLineEditDlg *linedlg = new KLineEditDlg(i18n("Enter new reply prefix"), "", this);
  if( linedlg->exec() == QDialog::Accepted )
  {
      QString repString=linedlg->text();
      if(!repString.isEmpty())
          mComposer.replyListBox->insertItem( repString );
  }
  delete linedlg;
}

void ConfigureDialog::slotRemoveSelReplyPrefix( void )
{
  int crItem = mComposer.replyListBox->currentItem();
  if( crItem != -1 )
    mComposer.replyListBox->removeItem( crItem );
}

void ConfigureDialog::slotReplyPrefixSelected( void )
{
  mComposer.removeReplyPrefixButton->setEnabled( true );
}

void ConfigureDialog::slotAddForwardPrefix( void )
{
  KLineEditDlg *linedlg = new KLineEditDlg(i18n("Enter new forward prefix"), "", this);
  if( linedlg->exec() == QDialog::Accepted )
  {
      QString forwString=linedlg->text();
      if(!forwString.isEmpty())
          mComposer.forwardListBox->insertItem( forwString );
  }
  delete linedlg;
}

void ConfigureDialog::slotRemoveSelForwardPrefix( void )
{
  int crItem = mComposer.forwardListBox->currentItem();
  if( crItem != -1 )
    mComposer.forwardListBox->removeItem( crItem );
}

void ConfigureDialog::slotForwardPrefixSelected( void )
{
  mComposer.removeForwardPrefixButton->setEnabled( true );
}

void ConfigureDialog::slotAddCharset( void )
{
  KLineEditDlg * linedlg = new KLineEditDlg(i18n("Enter charset to add"), "",
    this);
  if ( linedlg->exec() == QDialog::Accepted )
  {
      QString charsetText=linedlg->text();
      if (!charsetText.isEmpty() && 
          ((charsetText.lower() == "us-ascii") ||
           (charsetText.lower() == "locale") ||
           KMMsgBase::codecForName( charsetText.latin1() )))
    {
      if (charsetText.lower() == "locale")
         charsetText = QString("%1 (locale)").arg(QCString(KGlobal::locale()->codecForEncoding()->mimeName()).lower());
      mComposer.charsetListBox->insertItem( charsetText,
        mComposer.charsetListBox->currentItem() + 1 );
      mComposer.charsetListBox->setSelected( mComposer.charsetListBox->
        currentItem() + 1, TRUE );
    } else {
      KMessageBox::sorry( this, i18n("This charset is not supported.") );
    }
  }
}

void ConfigureDialog::slotRemoveSelCharset( void )
{
  int crItem = mComposer.charsetListBox->currentItem();
  if( crItem != -1 )
  {
    mComposer.charsetListBox->removeItem( crItem );
    if (crItem - mComposer.charsetListBox->count() <= 0) crItem--;
    mComposer.charsetListBox->setSelected( crItem, TRUE );
  }
}

void ConfigureDialog::slotCharsetUp( void )
{
  int crItem = mComposer.charsetListBox->currentItem();
  QString text = mComposer.charsetListBox->text( crItem );
  mComposer.charsetListBox->removeItem( crItem );
  mComposer.charsetListBox->insertItem( text, crItem - 1 );
  mComposer.charsetListBox->setSelected( crItem - 1, TRUE );
}

void ConfigureDialog::slotCharsetDown( void )
{
  int crItem = mComposer.charsetListBox->currentItem();
  QString text = mComposer.charsetListBox->text( crItem );
  mComposer.charsetListBox->removeItem( crItem );
  mComposer.charsetListBox->insertItem( text, crItem + 1 );
  mComposer.charsetListBox->setSelected( crItem + 1, TRUE );
}

void ConfigureDialog::slotCharsetSelectionChanged( void )
{
  mComposer.charsetUpButton->setEnabled( mComposer.charsetListBox->
    currentItem() > 0 );
  mComposer.charsetDownButton->setEnabled( mComposer.charsetListBox->
    count() - mComposer.charsetListBox->currentItem() > 1 );
  mComposer.removeCharsetButton->setEnabled( mComposer.charsetListBox->
    count() != 0 );
}

void ConfigureDialog::slotMimeHeaderSelectionChanged( void )
{
  mComposer.currentTagItem = mComposer.tagList->selectedItem();
  if( mComposer.currentTagItem != 0 )
  {
    mComposer.tagNameEdit->setText( mComposer.currentTagItem->text(0) );
    mComposer.tagValueEdit->setText( mComposer.currentTagItem->text(1) );
    mComposer.tagNameEdit->setEnabled(true);
    mComposer.tagValueEdit->setEnabled(true);
    mComposer.tagNameLabel->setEnabled(true);
    mComposer.tagValueLabel->setEnabled(true);
  }
}


void ConfigureDialog::slotMimeHeaderNameChanged( const QString &text )
{
  if( mComposer.currentTagItem != 0 )
  {
    mComposer.currentTagItem->setText(0, text );
  }
}


void ConfigureDialog::slotMimeHeaderValueChanged( const QString &text )
{
  if( mComposer.currentTagItem != 0 )
  {
    mComposer.currentTagItem->setText(1, text );
  }
}


void ConfigureDialog::slotNewMimeHeader( void )
{
  QListViewItem *listItem = new QListViewItem( mComposer.tagList, "", "" );
  mComposer.tagList->setCurrentItem( listItem );
  mComposer.tagList->setSelected( listItem, true );

  mComposer.currentTagItem = mComposer.tagList->selectedItem();
  if( mComposer.currentTagItem != 0 )
  {
    mComposer.tagNameEdit->setEnabled(true);
    mComposer.tagValueEdit->setEnabled(true);
    mComposer.tagNameLabel->setEnabled(true);
    mComposer.tagValueLabel->setEnabled(true);
    mComposer.tagNameEdit->setFocus();
  }
}


void ConfigureDialog::slotDeleteMimeHeader( void )
{
  if( mComposer.currentTagItem != 0 )
  {
    QListViewItem *next = mComposer.currentTagItem->itemAbove();
    if( next == 0 )
    {
      next = mComposer.currentTagItem->itemBelow();
    }

    mComposer.tagNameEdit->clear();
    mComposer.tagValueEdit->clear();
    mComposer.tagNameEdit->setEnabled(false);
    mComposer.tagValueEdit->setEnabled(false);
    mComposer.tagNameLabel->setEnabled(false);
    mComposer.tagValueLabel->setEnabled(false);

    mComposer.tagList->takeItem( mComposer.currentTagItem );
    mComposer.currentTagItem = 0;

    if( next != 0 )
    {
      mComposer.tagList->setSelected( next, true );
    }
  }
}


void ConfigureDialog::slotExternalEditorSelectionChanged( void )
{
  bool flag = mComposer.externalEditorCheck->isChecked();
  mComposer.externalEditorEdit->setEnabled( flag );
  mComposer.externalEditorChooseButton->setEnabled( flag );
  mComposer.externalEditorLabel->setEnabled( flag );
  mComposer.externalEditorHelp->setEnabled( flag );
}


void ConfigureDialog::slotExternalEditorChooser( void )
{
  KFileDialog dialog("/", QString::null, this, 0, true );
  dialog.setCaption(i18n("Choose External Editor") );

  if( dialog.exec() == QDialog::Accepted )
  {
    KURL url = dialog.selectedURL();
    if( url.isEmpty() == true )
    {
      return;
    }

    if( url.isLocalFile() == false )
    {
      KMessageBox::sorry( 0L, i18n( "Only local files allowed." ) );
      return;
    }

    mComposer.externalEditorEdit->setText( url.path() );
  }
}


void ConfigureDialog::slotMailCommandChooser( void )
{
  KFileDialog dialog("/", QString::null, this, 0, true );
  dialog.setCaption(i18n("Choose External Command") );

  if( dialog.exec() == QDialog::Accepted )
  {
    KURL url = dialog.selectedURL();
    if( url.isEmpty() == true )
    {
      return;
    }

    if( url.isLocalFile() == false )
    {
      KMessageBox::sorry( 0L, i18n( "Only local files allowed." ) );
      return;
    }

    mNetwork.mailCommandEdit->setText( url.path() );
  }
}

