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

#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qheader.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qlistbox.h>
#include <qmultilineedit.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qregexp.h>
#include <qtabwidget.h>
#include <qvalidator.h>
#include <qvbox.h>
#include <qwhatsthis.h>

#include <kapp.h>
#include <kcharsets.h>
#include <kcolorbtn.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kfontdialog.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <klineeditdlg.h>
#include <klistview.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <knuminput.h>
#include <kpgp.h>
#include <ksimpleconfig.h>
#include <kstddirs.h>
#include <kurlrequester.h>
#include <kglobalsettings.h>
#include <kscoring.h>
#include <kscoringeditor.h>
#include <kseparator.h>

#include "accountdialog.h"
#include "colorlistbox.h"
#include "configuredialog.h"
#include "kbusyptr.h"
#include "kmaccount.h"
#include "kmacctmgr.h"
#include "kmacctseldlg.h"
#include "kmfolder.h"
#include "kmglobal.h"
#include "kmheaders.h"
#include "kmidentity.h"
#include "kmmessage.h"
#include "kmsender.h"
#include "kmtopwidget.h"
#include "kmscoring.h"

#include "configuredialog.moc"

#include <stdlib.h>


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


NewIdentityDialog::NewIdentityDialog( QWidget *parent, const char *name,
				      bool modal )
  :KDialogBase( parent, name, modal, i18n("New Identity"), Ok|Cancel|Help, Ok,
		true )
{
  QFrame *page = makeMainWidget();
  QGridLayout *glay = new QGridLayout( page, 6, 2, 0, spacingHint() );
  glay->addColSpacing( 1, fontMetrics().maxWidth()*15 );
  glay->setRowStretch( 5, 10 );

  QLabel *label = new QLabel( i18n("&New Identity:"), page );
  glay->addWidget( label, 0, 0 );

  mLineEdit = new QLineEdit( page );
  label->setBuddy(mLineEdit);
  mLineEdit->setFocus();
  glay->addWidget( mLineEdit, 0, 1 );

  QButtonGroup *buttonGroup = new QButtonGroup( page );
  connect( buttonGroup, SIGNAL(clicked(int)), this, SLOT(radioClicked(int)) );
  buttonGroup->hide();

  QRadioButton *radioEmpty =
    new QRadioButton( i18n("&With empty fields"), page );
  buttonGroup->insert(radioEmpty, Empty );
  glay->addMultiCellWidget( radioEmpty, 1, 1, 0, 1 );

  QRadioButton *radioControlCenter =
    new QRadioButton( i18n("&Use Control Center settings"), page );
  buttonGroup->insert(radioControlCenter, ControlCenter );
  glay->addMultiCellWidget( radioControlCenter, 2, 2, 0, 1 );

  QRadioButton *radioDuplicate =
    new QRadioButton( i18n("&Duplicate existing identity"), page );
  buttonGroup->insert(radioDuplicate, ExistingEntry );
  glay->addMultiCellWidget( radioDuplicate, 3, 3, 0, 1 );

  mComboLabel = new QLabel( i18n("Existing identities:"), page );
  glay->addWidget( mComboLabel, 4, 0 );

  mComboBox = new QComboBox( false, page );
  glay->addWidget( mComboBox, 4, 1 );

  buttonGroup->setButton(0);
  radioClicked(0);
}


void NewIdentityDialog::slotOk( void )
{
  QString identity = identityText().stripWhiteSpace();
  if( identity.isEmpty() == true )
  {
    KMessageBox::error( this, i18n("You must specify an identity") );
    return;
  }

  for( int i=0; i<mComboBox->count(); i++ )
  {
    if( identity == mComboBox->text(i) )
    {
      KMessageBox::error( this, i18n("The identity already exist") );
      return;
    }
  }
  accept();
}



void NewIdentityDialog::radioClicked( int id )
{
  mDuplicateMode = id;

  bool state = mDuplicateMode == 2;
  mComboLabel->setEnabled( state );
  mComboBox->setEnabled( state );
}


void NewIdentityDialog::setIdentities( const QStringList &list )
{
  mComboBox->clear();
  mComboBox->insertStringList( list );
}


QString NewIdentityDialog::identityText( void )
{
  return( mLineEdit->text() );
}


QString NewIdentityDialog::duplicateText( void )
{
  return( mComboBox->isEnabled() ? mComboBox->currentText() : QString::null );
}


int NewIdentityDialog::duplicateMode( void )
{
  return( mDuplicateMode );
}



RenameIdentityDialog::RenameIdentityDialog( QWidget *parent, const char *name,
					    bool modal )
  :KDialogBase( parent, name, modal, i18n("Rename Identity"), Ok|Cancel|Help,
		Ok, true )
{
  QFrame *page = makeMainWidget();
  QGridLayout *glay = new QGridLayout( page, 4, 2, 0, spacingHint() );
  glay->addColSpacing( 1, fontMetrics().maxWidth()*20 );
  glay->setRowStretch( 3, 10 );

  QLabel *label = new QLabel( i18n("Current Name:"), page );
  glay->addWidget( label, 0, 0 );

  mCurrentNameLabel = new QLabel( page );
  glay->addWidget( mCurrentNameLabel, 0, 1 );

  QFont f( mCurrentNameLabel->font() );
  f.setBold(true);
  mCurrentNameLabel->setFont(f);

  glay->addRowSpacing( 1, spacingHint() );

  label = new QLabel( i18n("&New Name:"), page );
  glay->addWidget( label, 2, 0 );

  mLineEdit = new QLineEdit( page );
  label->setBuddy( mLineEdit );
  glay->addWidget( mLineEdit, 2, 1 );
}


void RenameIdentityDialog::showEvent( QShowEvent * )
{
  mLineEdit->setFocus();
}


void RenameIdentityDialog::slotOk( void )
{
  QString identity = identityText().stripWhiteSpace();
  if( identity.isEmpty() == true )
  {
    KMessageBox::error( this, i18n("You must specify an identity") );
    return;
  }

  QStringList::Iterator it;
  for( it = mIdentityList.begin(); it != mIdentityList.end(); ++it )
  {
    if( *it == identity )
    {
      KMessageBox::error( this, i18n("The identity already exist") );
      return;
    }
  }

  accept();
}


void RenameIdentityDialog::setIdentities( const QString &current,
					  const QStringList &list )
{
  mCurrentNameLabel->setText( current );
  mIdentityList = list;
  mLineEdit->setText( current );
  mLineEdit->setSelection( 0, current.length() );
}


QString RenameIdentityDialog::identityText( void )
{
  return( mLineEdit->text() );
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
  mComboBox = new QComboBox( page );
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

  :KDialogBase( IconList, i18n("Configure"), Help|Apply|Ok|Cancel,
		Ok, parent, name, modal, true )
{
  setHelp( "kmail/kmail.html", QString::null );
  setIconListAllVisible( true );

  /* Button not used
  enableButton( Default, false );
  */
  secondIdentity = false;
  connect( this, SIGNAL( cancelClicked() ), this, SLOT( slotCancelOrClose() ));
  connect( this, SIGNAL( closeClicked() ), this, SLOT( slotCancelOrClose() ));

  makeIdentityPage();
  makeNetworkPage();
  makeAppearancePage();
  makeComposerPage();
  makeMimePage();
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
  QFrame *page = addPage( i18n("Identity"), i18n("Personal information"),
    KGlobal::instance()->iconLoader()->loadIcon( "identity", KIcon::NoGroup,
    KIcon::SizeMedium ));
  QVBoxLayout *topLevel = new QVBoxLayout( page, 0, spacingHint() );
  mIdentity.pageIndex = pageIndex(page);

  QGridLayout *glay = new QGridLayout( topLevel, 12, 3 );
  glay->addColSpacing( 1, fontMetrics().maxWidth()*15 );
  //glay->addRowSpacing( 6, spacingHint() );
  glay->setRowStretch( 11, 10 );
  glay->setColStretch( 1, 10 );

  /*
  QLabel *label = new QLabel( i18n("Identity:"), page );
  glay->addWidget( label, 0, 0 );
  QWidget *helper = new QWidget( page );
  glay->addMultiCellWidget( helper, 0, 0, 1, 2 );
  QHBoxLayout *hlay = new QHBoxLayout( helper, 0, spacingHint() );
  mIdentity.identityCombo = new QComboBox( false, helper );
  connect( mIdentity.identityCombo, SIGNAL(activated(int)),
	   this, SLOT(slotIdentitySelectorChanged()) );
  hlay->addWidget( mIdentity.identityCombo, 10 );
  QPushButton *newButton = new QPushButton( i18n("New..."), helper );
  connect( newButton, SIGNAL(clicked()),
	   this, SLOT(slotNewIdentity()) );
  newButton->setAutoDefault( false );
  hlay->addWidget( newButton );
  mIdentity.removeIdentityButton = new QPushButton( i18n("Remove"), helper );
  connect( mIdentity.removeIdentityButton, SIGNAL(clicked()),
	   this, SLOT(slotRemoveIdentity()) );
  mIdentity.removeIdentityButton->setAutoDefault( false );
  hlay->addWidget( mIdentity.removeIdentityButton );
  */

  QLabel *label = new QLabel( i18n("&Identity:"), page );
  glay->addWidget( label, 0, 0 );
  mIdentity.identityCombo = new QComboBox( false, page );
  label->setBuddy( mIdentity.identityCombo );
  connect( mIdentity.identityCombo, SIGNAL(activated(int)),
	   this, SLOT(slotIdentitySelectorChanged()) );
  glay->addMultiCellWidget( mIdentity.identityCombo, 0, 0, 1, 2 );

  QWidget *helper = new QWidget( page );
  glay->addMultiCellWidget( helper, 1, 1, 1, 2 );
  QHBoxLayout *hlay = new QHBoxLayout( helper, 0, spacingHint() );
  QPushButton *newButton = new QPushButton( i18n("&New..."), helper );
  mIdentity.renameIdentityButton = new QPushButton( i18n("&Rename..."), helper);
  mIdentity.removeIdentityButton = new QPushButton( i18n("Re&move"), helper );
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


  label = new QLabel( i18n("&Name:"), page );
  glay->addWidget( label, 2, 0 );
  mIdentity.nameEdit = new QLineEdit( page );
  label->setBuddy(mIdentity.nameEdit);
  glay->addMultiCellWidget( mIdentity.nameEdit, 2, 2, 1, 2 );

  label = new QLabel( i18n("&Organization:"), page );
  glay->addWidget( label, 3, 0 );
  mIdentity.organizationEdit = new QLineEdit( page );
  label->setBuddy(mIdentity.organizationEdit);
  glay->addMultiCellWidget( mIdentity.organizationEdit, 3, 3, 1, 2 );

  label = new QLabel( i18n("&Email Address:"), page );
  glay->addWidget( label, 4, 0 );
  mIdentity.emailEdit = new QLineEdit( page );
  label->setBuddy(mIdentity.emailEdit);
  glay->addMultiCellWidget( mIdentity.emailEdit, 4, 4, 1, 2 );

  label = new QLabel( i18n("&Reply-To Address:"), page );
  glay->addWidget( label, 5, 0 );
  mIdentity.replytoEdit = new QLineEdit( page );
  label->setBuddy(mIdentity.replytoEdit);
  glay->addMultiCellWidget( mIdentity.replytoEdit, 5, 5, 1, 2 );

  label = new QLabel( i18n("PGP &User Identity:"),page );
  glay->addWidget( label, 6, 0 );
  mIdentity.pgpIdentityEdit = new QLineEdit( page );
  label->setBuddy(mIdentity.pgpIdentityEdit);
  glay->addMultiCellWidget( mIdentity.pgpIdentityEdit, 6, 6, 1, 2 );

  QButtonGroup *buttonGroup = new QButtonGroup( page );
  connect( buttonGroup, SIGNAL(clicked(int)),
	   this, SLOT(slotSignatureType(int)) );
  buttonGroup->hide();
  mIdentity.signatureFileRadio =
    new QRadioButton( i18n("&Use a signature from file"), page );
  buttonGroup->insert( mIdentity.signatureFileRadio );
  glay->addMultiCellWidget( mIdentity.signatureFileRadio, 7, 7, 0, 1 );

  mIdentity.transportButton = new QPushButton(i18n("&Add Transport..."),
                                              page);
  connect(mIdentity.transportButton, SIGNAL(clicked()), this,
          SLOT(slotIdentityTransport()));
  glay->addWidget(mIdentity.transportButton, 7, 2);
  mIdentity.transportButton->setAutoDefault( false );
  if (mIdentity.identityCombo->currentItem() == 0) {
    mIdentity.transportButton->setEnabled(false);
    mIdentity.transportButton->hide();
  }
  else {
    mIdentity.transportButton->setEnabled(true);
    mIdentity.transportButton->show();
  }

  mIdentity.signatureFileLabel = new QLabel( i18n("&Signature File:"), page );
  glay->addWidget( mIdentity.signatureFileLabel, 8, 0 );
  mIdentity.signatureFileEdit = new KURLRequester( page );
  mIdentity.signatureFileLabel->setBuddy(mIdentity.signatureFileEdit);
  QPushButton *button = mIdentity.signatureFileEdit->button();
  button->setText( i18n("&Choose...") );
  button->setAutoDefault( false );
  connect( mIdentity.signatureFileEdit, SIGNAL(textChanged(const QString &)),
	   this, SLOT( slotSignatureFile(const QString &)) );
  glay->addMultiCellWidget( mIdentity.signatureFileEdit, 8, 8, 1, 2 );

  mIdentity.signatureExecCheck =
    new QCheckBox( i18n("&The file is a program"), page );
  glay->addWidget( mIdentity.signatureExecCheck, 9, 1 );
  mIdentity.signatureEditButton = new QPushButton( i18n("Ed&it File"), page );
  connect( mIdentity.signatureEditButton, SIGNAL(clicked()),
	   this, SLOT(slotSignatureEdit()) );
  mIdentity.signatureEditButton->setAutoDefault( false );
  glay->addWidget( mIdentity.signatureEditButton, 9, 2 );
  button->setMinimumSize( mIdentity.signatureEditButton->sizeHint() );

  mIdentity.signatureTextRadio =
    new QRadioButton( i18n("&Specify signature below"), page );
  buttonGroup->insert( mIdentity.signatureTextRadio );
  glay->addMultiCellWidget( mIdentity.signatureTextRadio, 10, 10, 0, 2 );

  mIdentity.signatureTextEdit = new QMultiLineEdit( page );
  mIdentity.signatureTextEdit->setText("Does not work yet");
  glay->addMultiCellWidget( mIdentity.signatureTextEdit, 11, 11, 0, 2 );
}



void ConfigureDialog::makeNetworkPage( void )
{
  QFrame *page = addPage( i18n("Network"),
			  i18n("Setup for sending and receiving messages"),
    KGlobal::instance()->iconLoader()->loadIcon( "network", KIcon::NoGroup,
    KIcon::SizeMedium ));
  QVBoxLayout *topLevel = new QVBoxLayout( page, 0, spacingHint() );
  mNetwork.pageIndex = pageIndex(page);

  QTabWidget *tabWidget = new QTabWidget( page, "tab" );
  topLevel->addWidget( tabWidget );

  QWidget *page1 = new QWidget( tabWidget );
  tabWidget->addTab( page1, i18n("Se&nding Mail") );

  QButtonGroup *buttonGroup = new QButtonGroup( page1 );
  buttonGroup->hide();
  connect( buttonGroup, SIGNAL(clicked(int)),
	   this, SLOT(slotSendmailType(int)) );

  QGridLayout *glay = new QGridLayout( page1, 5, 4, spacingHint() );
  glay->addColSpacing( 2, fontMetrics().maxWidth()*15 );

  mNetwork.sendmailRadio = new QRadioButton( i18n("&Sendmail"), page1 );
  buttonGroup->insert(mNetwork.sendmailRadio);
  glay->addMultiCellWidget(mNetwork.sendmailRadio, 0, 0, 0, 3);
  QLabel *label = new QLabel( i18n("&Location:"), page1 );
  glay->addWidget( label, 1, 1 );
  mNetwork.sendmailLocationEdit = new QLineEdit( page1 );
  label->setBuddy(mNetwork.sendmailLocationEdit);
  glay->addWidget( mNetwork.sendmailLocationEdit, 1, 2 );
  mNetwork.sendmailChooseButton =
    new QPushButton( i18n("&Choose..."), page1 );
  connect( mNetwork.sendmailChooseButton, SIGNAL(clicked()),
	   this, SLOT(slotSendmailChooser()) );
  mNetwork.sendmailChooseButton->setAutoDefault( false );
  glay->addWidget( mNetwork.sendmailChooseButton, 1, 3 );

  mNetwork.smtpRadio = new QRadioButton( i18n("SM&TP"), page1 );
  buttonGroup->insert(mNetwork.smtpRadio);
  glay->addMultiCellWidget(mNetwork.smtpRadio, 2, 2, 0, 3);
  label = new QLabel( i18n("Ser&ver:"), page1 );
  glay->addWidget( label, 3, 1 );
  mNetwork.smtpServerEdit = new QLineEdit( page1 );
  label->setBuddy(mNetwork.smtpServerEdit);
  glay->addWidget( mNetwork.smtpServerEdit, 3, 2 );
  label = new QLabel( i18n("&Port:"), page1 );
  glay->addWidget( label, 4, 1 );
  mNetwork.smtpPortEdit = new QLineEdit( page1 );
  label->setBuddy(mNetwork.smtpPortEdit);
  mNetwork.smtpPortEdit->setValidator( new QIntValidator(page1) );
  glay->addWidget( mNetwork.smtpPortEdit, 4, 2 );

  QWidget *page2 = new QWidget( tabWidget );
  tabWidget->addTab( page2, i18n("&Properties") );

  glay = new QGridLayout( page2, 4, 3, spacingHint() );
  glay->setColStretch( 2, 10 );

  label = new QLabel( i18n("&Default send method:"), page2 );
  glay->addWidget( label, 0, 0 );
  mNetwork.sendMethodCombo = new QComboBox( page2 );
  label->setBuddy( mNetwork.sendMethodCombo );
  mNetwork.sendMethodCombo->insertItem(i18n("Send now"));
  mNetwork.sendMethodCombo->insertItem(i18n("Send later"));
  glay->addWidget( mNetwork.sendMethodCombo, 0, 1 );

  label = new QLabel( i18n("Message &Property:"), page2 );
  glay->addWidget( label, 1, 0 );
  mNetwork.messagePropertyCombo = new QComboBox( page2 );
  label->setBuddy( mNetwork.messagePropertyCombo );
  mNetwork.messagePropertyCombo->insertItem(i18n("Allow 8-bit"));
  mNetwork.messagePropertyCombo->insertItem(
    i18n("MIME Compliant (Quoted Printable)"));
  glay->addWidget( mNetwork.messagePropertyCombo, 1, 1 );

 label = new QLabel( i18n("Pr&ecommand:"), page2 );
  glay->addWidget( label, 2, 0 );
  mNetwork.precommandEdit = new QLineEdit( page2 );
  label->setBuddy(mNetwork.precommandEdit);
  glay->addWidget( mNetwork.precommandEdit, 2, 1 );

  mNetwork.confirmSendCheck =
    new QCheckBox(i18n("&Confirm before send"), page2 );
  glay->addMultiCellWidget( mNetwork.confirmSendCheck, 3, 3, 0, 1 );



  buttonGroup = new QButtonGroup(i18n("Incoming Mail"), page );
  topLevel->addWidget(buttonGroup, 10 );

  glay = new QGridLayout( buttonGroup, 6, 2, spacingHint() );
  glay->addColSpacing( 0, fontMetrics().maxWidth()*15 );
  glay->addRowSpacing( 0, fontMetrics().lineSpacing() );
  glay->setColStretch( 0, 10 );
  glay->setRowStretch( 5, 100 );

  label = new QLabel( buttonGroup );
  label->setText(i18n("Accounts:   (add at least one account!)"));
  glay->addMultiCellWidget(label, 1, 1, 0, 1);
  mNetwork.accountList = new ListView( buttonGroup, "accountList", 5 );
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
  glay->addMultiCellWidget( mNetwork.accountList, 2, 5, 0, 0 );

  mNetwork.addAccountButton =
    new QPushButton( i18n("&Add..."), buttonGroup );
  mNetwork.addAccountButton->setAutoDefault( false );
  connect( mNetwork.addAccountButton, SIGNAL(clicked()),
	   this, SLOT(slotAddAccount()) );
  glay->addWidget( mNetwork.addAccountButton, 2, 1 );

  mNetwork.modifyAccountButton =
    new QPushButton( i18n("&Modify..."), buttonGroup );
  mNetwork.modifyAccountButton->setAutoDefault( false );
  mNetwork.modifyAccountButton->setEnabled( false );
  connect( mNetwork.modifyAccountButton, SIGNAL(clicked()),
	   this, SLOT(slotModifySelectedAccount()) );
  glay->addWidget( mNetwork.modifyAccountButton, 3, 1 );

  mNetwork.removeAccountButton
    = new QPushButton( i18n("&Remove..."), buttonGroup );
  mNetwork.removeAccountButton->setAutoDefault( false );
  mNetwork.removeAccountButton->setEnabled( false );
  connect( mNetwork.removeAccountButton, SIGNAL(clicked()),
	   this, SLOT(slotRemoveSelectedAccount()) );
  glay->addWidget( mNetwork.removeAccountButton, 4, 1 );
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
  mAppearance.unicodeFontCheck =
    new QCheckBox( i18n("Use un&icode font for messages"), page1 );
  vlay->addWidget( mAppearance.unicodeFontCheck );
  KSeparator *hline = new KSeparator( KSeparator::HLine, page1);
  vlay->addWidget( hline );
  QHBoxLayout *hlay = new QHBoxLayout( vlay );
  mAppearance.fontLocationLabel = new QLabel( i18n("Location:"), page1 );
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

  // Score
  QWidget *page6 = new QWidget( tabWidget );
  tabWidget->addTab( page6, i18n("&Messages Scoring") );
  vlay = new QVBoxLayout( page6, spacingHint() );

  KScoringEditorWidget* ksc =
      new KScoringEditorWidget(KMScoringManager::globalScoringManager(),
                              page6);
  KMScoringManager::globalScoringManager()->setMainWin(parent());
  vlay->addWidget( ksc );

}



void ConfigureDialog::makeComposerPage( void )
{
   QVBox *vbox = addVBoxPage( i18n("Composer"),
			  i18n("Phrases and general behavior"),
      KGlobal::instance()->iconLoader()->loadIcon( "edit", KIcon::NoGroup,
      KIcon::SizeMedium ));
  QTabWidget *tabWidget = new QTabWidget( vbox, "tab" );
  mComposer.pageIndex = pageIndex(vbox);

  QWidget *page = new QWidget( tabWidget );
  tabWidget->addTab( page, i18n("&General") );
  QVBoxLayout *topLevel = new QVBoxLayout( page, spacingHint() );
  QGroupBox *group = new QGroupBox(i18n("Phrases"), page );
  topLevel->addWidget( group );

  QGridLayout *glay = new QGridLayout( group, 8, 2, spacingHint() );
  glay->addRowSpacing( 0, fontMetrics().lineSpacing() );
  glay->setColStretch( 1, 10 );

  QLabel *label = new QLabel( group );
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

  mComposer.autoAppSignFileCheck =
    new QCheckBox( i18n("&Automatically append signature"), page );
  topLevel->addWidget( mComposer.autoAppSignFileCheck );

  mComposer.smartQuoteCheck =
    new QCheckBox( i18n("&Use smart quoting"), page );
  topLevel->addWidget( mComposer.smartQuoteCheck );

  mComposer.pgpAutoSignatureCheck =
    new QCheckBox( i18n("Automatically &sign messages using PGP"), page );
  topLevel->addWidget( mComposer.pgpAutoSignatureCheck );

  QHBoxLayout *hlay = new QHBoxLayout( topLevel );
  mComposer.wordWrapCheck =
    new QCheckBox( i18n("&Word wrap at column:"), page );
  connect( mComposer.wordWrapCheck, SIGNAL(clicked() ),
	   this, SLOT(slotWordWrapSelectionChanged()) );
  hlay->addWidget( mComposer.wordWrapCheck );
  mComposer.wrapColumnSpin = new KIntNumInput( page );
  mComposer.wrapColumnSpin->setRange( 1, 10000, 1, FALSE );
  hlay->addWidget( mComposer.wrapColumnSpin, 0, AlignLeft );
  hlay->addStretch(10);

  topLevel->addStretch(10);

  // ----- editor page
  page = new QWidget( tabWidget );
  QGroupBox * editorGroup = new QGroupBox( i18n("External Editor"), page );
  tabWidget->addTab( page, i18n("&Editor") );
  QBoxLayout * elay = new QVBoxLayout( page, spacingHint() );
  elay->addWidget( editorGroup );
  elay->addStretch( 10 );
  QBoxLayout * vlay = new QVBoxLayout( editorGroup, spacingHint() );
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
  QGroupBox *charsetsGroup = new QGroupBox( i18n("Available charsets"),
    charsetPage );
  QGridLayout *charsetsGridLay = new QGridLayout( charsetsGroup, 6, 2,
    spacingHint() );
  charsetsGridLay->addRowSpacing( 0, fontMetrics().lineSpacing() );
  charsetsGridLay->setRowStretch( 1, 10 );
  mComposer.charsetListBox = new QListBox( charsetsGroup );
  charsetsGridLay->addMultiCellWidget( mComposer.charsetListBox, 1, 5, 0, 0 );
  mComposer.addCharsetButton = new QPushButton( i18n("&Add..."), charsetsGroup );
  charsetsGridLay->addWidget( mComposer.addCharsetButton, 2, 1 );
  mComposer.removeCharsetButton = new QPushButton( i18n("&Remove"),
    charsetsGroup );
  charsetsGridLay->addWidget( mComposer.removeCharsetButton, 3, 1 );
  mComposer.charsetUpButton = new QPushButton( i18n("&Up"), charsetsGroup );
  mComposer.charsetUpButton->setAutoRepeat( TRUE );
  charsetsGridLay->addWidget( mComposer.charsetUpButton, 4, 1 );
  mComposer.charsetDownButton = new QPushButton( i18n("&Down"), charsetsGroup );
  mComposer.charsetDownButton->setAutoRepeat( TRUE );
  charsetsGridLay->addWidget( mComposer.charsetDownButton, 5, 1 );
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

  //default charset
  QGroupBox *defaultCharsetGroup = new QGroupBox( i18n("Default charset"),
    charsetPage );
  QVBoxLayout *charsetVLay = new QVBoxLayout( defaultCharsetGroup,
    spacingHint() );
  charsetVLay->addSpacing( fontMetrics().lineSpacing() );
  mComposer.defaultCharsetCombo = new QComboBox( defaultCharsetGroup );
  charsetVLay->addWidget( mComposer.defaultCharsetCombo );
  mComposer.forceReplyCharsetCheck =
    new QCheckBox( i18n("&Use own default charset when replying or forwarding"),
    defaultCharsetGroup );
  charsetVLay->addWidget( mComposer.forceReplyCharsetCheck );
  topLevel3->addWidget( defaultCharsetGroup );
  topLevel3->addSpacing( spacingHint() );
}



void ConfigureDialog::makeMimePage( void )
{
  QFrame *page = addPage( i18n("Mime Headers"),
    i18n("Custom header tags for outgoing emails"),
    KGlobal::instance()->iconLoader()->loadIcon( "readme", KIcon::NoGroup,
    KIcon::SizeMedium ));
  QVBoxLayout *topLevel = new QVBoxLayout( page, 0, spacingHint() );
  mMime.pageIndex = pageIndex(page);

  mMime.createOwnMessageIdCheck =
    new QCheckBox( i18n("&Create own Message-Id headers"), page );
  topLevel->addWidget( mMime.createOwnMessageIdCheck );

  QGridLayout *glay0 = new QGridLayout( topLevel, 1, 2 );
  glay0->setColStretch( 1, 10 );

  int indent = 20;
  QString lblTxt = i18n("&Use this Message-Id suffix:");
  mMime.messageIdSuffixLabel = new QLabel( lblTxt, page );
  mMime.messageIdSuffixLabel->setAlignment( AlignLeft );
  mMime.messageIdSuffixLabel->setFixedHeight( mMime.messageIdSuffixLabel->sizeHint().height() );
  mMime.messageIdSuffixLabel->setIndent( indent );
  glay0->addWidget( mMime.messageIdSuffixLabel, 0, 0 );

  mMime.messageIdSuffixEdit = new QLineEdit( page );
  mMime.messageIdSuffixLabel->setBuddy( mMime.messageIdSuffixEdit );
  mMime.messageIdSuffixEdit->setFocus();
  glay0->addWidget( mMime.messageIdSuffixEdit,  0, 1 );

  lblTxt = i18n("(Name must be unique, you may use a domain name\n"
    "that you are the owner of.)");
  mMime.messageIdSuffixHintLabel = new QLabel( lblTxt, page );
  mMime.messageIdSuffixHintLabel->setAlignment( AlignLeft );
  mMime.messageIdSuffixHintLabel->setFixedHeight( mMime.messageIdSuffixHintLabel->sizeHint().height() );
  mMime.messageIdSuffixHintLabel->setIndent( indent );
  topLevel->addWidget( mMime.messageIdSuffixHintLabel );

  connect( mMime.createOwnMessageIdCheck, SIGNAL(clicked() ),
	   this, SLOT(slotCreateOwnMessageIdChanged()) );
  slotCreateOwnMessageIdChanged();

  KSeparator *hline = new KSeparator( KSeparator::HLine, page);
  topLevel->addWidget( hline );

  QLabel *label = new QLabel( page );
  label->setText(i18n("Define custom mime header tags:"));
  topLevel->addWidget( label );

  mMime.tagList = new ListView( page, "tagList" );
  mMime.tagList->addColumn( i18n("Name") );
  mMime.tagList->addColumn( i18n("Value") );
  mMime.tagList->setAllColumnsShowFocus( true );
  mMime.tagList->setFrameStyle( QFrame::WinPanel + QFrame::Sunken );
  mMime.tagList->setSorting( -1 );
  mMime.tagList->setMinimumSize( 0, 0 );
  connect( mMime.tagList, SIGNAL(selectionChanged()),
	   this, SLOT(slotMimeHeaderSelectionChanged()) );
  topLevel->addWidget( mMime.tagList );

  QGridLayout *glay = new QGridLayout( topLevel, 3, 2 );
  glay->setColStretch( 1, 10 );

  mMime.tagNameLabel = new QLabel(i18n("N&ame:"), page );
  mMime.tagNameLabel->setEnabled(false);
  glay->addWidget( mMime.tagNameLabel, 0, 0 );
  mMime.tagNameEdit = new QLineEdit(page);
  mMime.tagNameLabel->setBuddy(mMime.tagNameEdit);
  mMime.tagNameEdit->setEnabled(false);
  connect( mMime.tagNameEdit, SIGNAL(textChanged(const QString&)),
	   this, SLOT(slotMimeHeaderNameChanged(const QString&)) );
  glay->addWidget( mMime.tagNameEdit, 0, 1 );

  mMime.tagValueLabel = new QLabel(i18n("&Value:"), page );
  mMime.tagValueLabel->setEnabled(false);
  glay->addWidget( mMime.tagValueLabel, 1, 0 );
  mMime.tagValueEdit = new QLineEdit(page);
  mMime.tagValueLabel->setBuddy(mMime.tagValueEdit);
  mMime.tagValueEdit->setEnabled(false);
  connect( mMime.tagValueEdit, SIGNAL(textChanged(const QString&)),
	   this, SLOT(slotMimeHeaderValueChanged(const QString&)) );
  glay->addWidget( mMime.tagValueEdit, 1, 1 );

  QWidget *helper = new QWidget( page );
  glay->addWidget( helper, 2, 1 );
  QHBoxLayout *hlay = new QHBoxLayout( helper, 0, spacingHint() );
  QPushButton *pushButton = new QPushButton(i18n("&New"), helper );
  connect( pushButton, SIGNAL(clicked()), this, SLOT(slotNewMimeHeader()) );
  pushButton->setAutoDefault( false );
  hlay->addWidget( pushButton );
  pushButton = new QPushButton(i18n("&Delete"), helper );
  connect( pushButton, SIGNAL(clicked()), this, SLOT(slotDeleteMimeHeader()));
  pushButton->setAutoDefault( false );
  hlay->addWidget( pushButton );
  hlay->addStretch(10);

  topLevel->addSpacing( spacingHint()*2 );
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
    
    mSecurity.pgpConfig = new KpgpConfig(page);
    vlay->addWidget( mSecurity.pgpConfig );
    vlay->addStretch(10);
}

#include <kinstance.h>
#include <kglobal.h>


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
  connect( mMisc.emptyTrashCheck, SIGNAL(stateChanged(int)),
		  this, SLOT(slotEmptyTrashState(int)) );
  tvlay->addWidget( mMisc.emptyTrashCheck );
  QHBoxLayout *stlay = new QHBoxLayout( spacingHint() ,"hly1");
  stlay->setMargin(0);
  tvlay->addLayout( stlay );
  mMisc.keepSmallTrashCheck =
    new QCheckBox(i18n("&Keep trash size below "), tgroup );
  stlay->addWidget( mMisc.keepSmallTrashCheck );
  mMisc.smallTrashSizeSpin = new KIntNumInput( tgroup );
  //mFolder.smallTrashSizeSpin->setMinValue(1);
  stlay->addWidget( mMisc.smallTrashSizeSpin );
  stlay->addWidget( new QLabel( "MB", tgroup ) );
  stlay->addStretch( 100 );

  QHBoxLayout *rmvlay = new QHBoxLayout( spacingHint(),"hly2" );
  rmvlay->setMargin(0);
  tvlay->addLayout( rmvlay );
  mMisc.removeOldMailCheck =
    new QCheckBox(i18n("&In trash, on exit, remove messages older than"), tgroup );
  rmvlay->addWidget( mMisc.removeOldMailCheck );
  mMisc.oldMailAgeSpin = new KIntNumInput( tgroup );
  mMisc.oldMailAgeSpin->setValue(1);
  //mFolder.oldMailAgeSpin->setMinValue(1);
  rmvlay->addWidget( mMisc.oldMailAgeSpin );
  mMisc.timeUnitCombo = new QComboBox( tgroup );
  mMisc.timeUnitCombo->insertItem(i18n("month(s)"));
  mMisc.timeUnitCombo->insertItem(i18n("week(s)"));
  mMisc.timeUnitCombo->insertItem(i18n("day(s)"));
  rmvlay->addWidget( mMisc.timeUnitCombo );
  rmvlay->addStretch( 100 );


  //---------- group: folders

  QGroupBox *group = new QGroupBox( i18n("&Folders"), page );
  topLevel->addWidget( group );
  QVBoxLayout *vlay = new QVBoxLayout( group, spacingHint() );
  vlay->addSpacing( fontMetrics().lineSpacing() );
  mMisc.sendOutboxCheck =
    new QCheckBox(i18n("&Send Mail in outbox Folder on Check"), group );
  vlay->addWidget( mMisc.sendOutboxCheck );
  mMisc.compactOnExitCheck =
    new QCheckBox(i18n("C&ompact all folders on exit"), group );
  vlay->addWidget( mMisc.compactOnExitCheck );
  mMisc.emptyFolderConfirmCheck =
    new QCheckBox(i18n("Conf&irm before emptying folders"), group );
  vlay->addWidget( mMisc.emptyFolderConfirmCheck );

  //---------- group: New Mail Notification
  group = new QGroupBox( i18n("&New Mail Notification"), page );
  topLevel->addWidget( group );
  vlay = new QVBoxLayout( group, spacingHint() );
  vlay->addSpacing( fontMetrics().lineSpacing() );
  mMisc.beepNewMailCheck =
    new QCheckBox(i18n("&Beep on new mail"), group );
  vlay->addWidget( mMisc.beepNewMailCheck );
  mMisc.showMessageBoxCheck =
    new QCheckBox(i18n("&Display message box on new mail"), group );
  vlay->addWidget( mMisc.showMessageBoxCheck );
  mMisc.mailCommandCheck =
    new QCheckBox( i18n("E&xecute command line on new mail"), group );
  vlay->addWidget( mMisc.mailCommandCheck );
  connect( mMisc.mailCommandCheck, SIGNAL(clicked() ),
	   this, SLOT(slotMailCommandSelectionChanged()) );
  QHBoxLayout *hlay = new QHBoxLayout( vlay );
  mMisc.mailCommandLabel = new QLabel( i18n("Specify command:"), group );
  hlay->addWidget( mMisc.mailCommandLabel );
  mMisc.mailCommandEdit = new QLineEdit( group );
  mMisc.mailCommandLabel->setBuddy(mMisc.mailCommandEdit);
  hlay->addWidget( mMisc.mailCommandEdit );
  mMisc.mailCommandChooseButton =
    new QPushButton( i18n("&Choose..."), group );
  connect( mMisc.mailCommandChooseButton, SIGNAL(clicked()),
	   this, SLOT(slotMailCommandChooser()) );
  mMisc.mailCommandChooseButton->setAutoDefault( false );
  hlay->addWidget( mMisc.mailCommandChooseButton );

  topLevel->addStretch( 10 );
}

void ConfigureDialog::setup( void )
{
  setupIdentityPage();
  setupNetworkPage();
  setupAppearancePage();
  setupComposerPage();
  setupMimePage();
  setupSecurityPage();
  setupMiscPage();
}



void ConfigureDialog::setupIdentityPage( void )
{
  mIdentityList.importData();
  mIdentity.identityCombo->clear();
  mIdentity.identityCombo->insertStringList( mIdentityList.identities() );
  mIdentity.mActiveIdentity = "";
  slotIdentitySelectorChanged(); // This will trigger an update
}


void ConfigureDialog::setupNetworkPage( void )
{
  if( kernel->msgSender()->method() == KMSender::smMail )
  {
    mNetwork.sendmailRadio->setChecked(true);
    slotSendmailType(0);
  }
  else if( kernel->msgSender()->method() == KMSender::smSMTP )
  {
    mNetwork.smtpRadio->setChecked(true);
    slotSendmailType(1);
  }

  mNetwork.sendmailLocationEdit->setText( kernel->msgSender()->mailer() );
  mNetwork.smtpServerEdit->setText( kernel->msgSender()->smtpHost() );
  mNetwork.smtpPortEdit->setText(
    QString().setNum(kernel->msgSender()->smtpPort()) );
  mNetwork.precommandEdit->setText( kernel->msgSender()->precommand() );

  KConfig *config = kapp->config();
  KConfigGroupSaver saver(config, "Composer");

  mNetwork.sendMethodCombo->setCurrentItem(
    kernel->msgSender()->sendImmediate() ? 0 : 1 );
  mNetwork.messagePropertyCombo->setCurrentItem(
    kernel->msgSender()->sendQuotedPrintable() ? 1 : 0 );
  mNetwork.confirmSendCheck->setChecked(
    config->readBoolEntry( "confirm-before-send", false ) );

  mNetwork.accountList->clear();
  QListViewItem *top = 0;
  for( KMAccount *a = kernel->acctMgr()->first(); a!=0;
       a = kernel->acctMgr()->next() )
  {
    QListViewItem *listItem =
      new QListViewItem( mNetwork.accountList, top, a->name(), a->type() );
    if( a->folder() )
      listItem->setText( 2, a->folder()->label() );
    top = listItem;
  }

  QListViewItem *listItem = mNetwork.accountList->firstChild();
  if( listItem != 0 )
  {
    mNetwork.accountList->setSelected( listItem, true );
  }
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
    mAppearance.unicodeFontCheck->setChecked( config->readBoolEntry(
      "unicodeFont", false ) );
    slotCustomFontSelectionChanged();
    updateFontSelector();
    slotFontSelectorChanged( mAppearance.fontLocationCombo->currentItem() );
  }

  {
    KConfigGroupSaver saver(config, "Reader");

    QColor defaultColor = QColor(kapp->palette().normal().base());
    mAppearance.colorList->setColor(
      0, config->readColorEntry("BackgroundColor",&defaultColor ) );

    defaultColor = QColor(kapp->palette().normal().text());
    mAppearance.colorList->setColor(
      1, config->readColorEntry("ForegroundColor",&defaultColor ) );

    defaultColor = QColor(kapp->palette().normal().text());
    mAppearance.colorList->setColor(
      2, config->readColorEntry("QuoutedText1",&defaultColor ) );

    defaultColor = QColor(kapp->palette().normal().text());
    mAppearance.colorList->setColor(
      3, config->readColorEntry("QuoutedText2",&defaultColor ) );

    defaultColor = QColor(kapp->palette().normal().text());
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

    state = config->readBoolEntry( "word-wrap", true );
    mComposer.wordWrapCheck->setChecked( state );

    int value = config->readEntry("break-at","78" ).toInt();
    mComposer.wrapColumnSpin->setValue( value );
    slotWordWrapSelectionChanged();

    //charsets
    QStringList charsets = config->readListEntry("charsets");
    mComposer.charsetListBox->clear();
    mComposer.charsetListBox->insertStringList( charsets );
    mComposer.charsetListBox->setCurrentItem( 0 );

    charsets.prepend( i18n("Use language encoding") );
    mComposer.defaultCharsetCombo->clear();
    mComposer.defaultCharsetCombo->insertStringList(charsets);
    QString str = config->readEntry( "charset", "" );
    if (str.isNull() || str.isEmpty() || str == "default")
      mComposer.defaultCharsetCombo->setCurrentItem( 0 );
    else
    {
      bool found = false;
      for (int j = 1; !found && (j < mComposer.defaultCharsetCombo->count()); j++ )
	if (mComposer.defaultCharsetCombo->text( j ) == str)
	{
	  mComposer.defaultCharsetCombo->setCurrentItem( j );
	  found = true;
	  break;
	}
      if (!found)
	mComposer.defaultCharsetCombo->setCurrentItem(0);
    }
    state = config->readBoolEntry( "force-reply-charset", false );
    mComposer.forceReplyCharsetCheck->setChecked( state );
  }
}

void ConfigureDialog::setupMimePage( void )
{
  KConfig *config = kapp->config();
  KConfigGroupSaver saver(config, "General");

  mMime.tagList->clear();
  mMime.currentTagItem = 0;
  mMime.tagNameEdit->clear();
  mMime.tagValueEdit->clear();
  mMime.tagNameEdit->setEnabled(false);
  mMime.tagValueEdit->setEnabled(false);
  mMime.tagNameLabel->setEnabled(false);
  mMime.tagValueLabel->setEnabled(false);

  QString str = config->readEntry( "myMessageIdSuffix", "" );
  mMime.messageIdSuffixEdit->setText( str );
  bool state = (str.isNull() || str.isEmpty())
             ? false
             : config->readBoolEntry("createOwnMessageIdHeaders", false );
  mMime.createOwnMessageIdCheck->setChecked(  state );
  mMime.messageIdSuffixLabel->setEnabled(     state );
  mMime.messageIdSuffixEdit->setEnabled(      state );
  mMime.messageIdSuffixHintLabel->setEnabled( state );


  QListViewItem *top = 0;

  int count = config->readNumEntry( "mime-header-count", 0 );
  mMime.tagList->clear();
  for(int i = 0; i < count; i++)
  {
    KConfigGroupSaver saver(config, QString("Mime #%1").arg(i) );
    QString name  = config->readEntry("name", "");
    QString value = config->readEntry("value", "");
    if( name.length() > 0 )
    {
      QListViewItem *listItem =
	new QListViewItem( mMime.tagList, top, name, value );
      top = listItem;
    }
  }
  if (mMime.tagList->childCount() > 0)
  {
    mMime.tagList->setCurrentItem(mMime.tagList->firstChild());
    mMime.tagList->setSelected(mMime.tagList->firstChild(), TRUE);
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

  bool state = config->readBoolEntry("empty-trash-on-exit",false);
  mMisc.emptyTrashCheck->setChecked( state );
  state = config->readBoolEntry("keep-small-trash", true);
  mMisc.keepSmallTrashCheck->setChecked( state );
  int num = config->readNumEntry("small-trash-size", 1);
  mMisc.smallTrashSizeSpin->setValue( num );
  state = config->readBoolEntry("remove-old-mail-from-trash", true);
  mMisc.removeOldMailCheck->setChecked( state );
  num = config->readNumEntry("old-mail-age", 1);
  mMisc.oldMailAgeSpin->setValue( num );
  num = config->readNumEntry("old-mail-age-unit", 1);
  mMisc.timeUnitCombo->setCurrentItem( num );
  state = config->readBoolEntry("sendOnCheck", false);
  mMisc.sendOutboxCheck->setChecked( state );
  state = config->readBoolEntry("compact-all-on-exit", true );
  mMisc.compactOnExitCheck->setChecked( state );
  state = config->readBoolEntry("confirm-before-empty", true );
  mMisc.emptyFolderConfirmCheck->setChecked( state );

  state = config->readBoolEntry("beep-on-mail", false );
  mMisc.beepNewMailCheck->setChecked( state );
  state = config->readBoolEntry("msgbox-on-mail", false);
  mMisc.showMessageBoxCheck->setChecked( state );
  state = config->readBoolEntry("exec-on-mail", false);
  mMisc.mailCommandCheck->setChecked( state );
  mMisc.mailCommandEdit->setText( config->readEntry("exec-on-mail-cmd", ""));
  slotExternalEditorSelectionChanged();
  slotMailCommandSelectionChanged();
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

    mAppearance.colorList->setColor( 0, kapp->palette().normal().base() );
    mAppearance.colorList->setColor( 1, kapp->palette().normal().text() );
    mAppearance.colorList->setColor( 2, red );
    mAppearance.colorList->setColor( 3, darkGreen );
    mAppearance.colorList->setColor( 4, darkMagenta );
    mAppearance.colorList->setColor( 5, KGlobalSettings::linkColor() );
    mAppearance.colorList->setColor( 6, KGlobalSettings::visitedLinkColor() );
    mAppearance.colorList->setColor( 7, blue );
    mAppearance.colorList->setColor( 8, red );
    mAppearance.customColorCheck->setChecked( true );

    mAppearance.longFolderCheck->setChecked( true );
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

    mAppearance.colorList->setColor( 0, kapp->palette().normal().base() );
    mAppearance.colorList->setColor( 1, kapp->palette().normal().text() );
    mAppearance.colorList->setColor( 2, red );
    mAppearance.colorList->setColor( 3, darkGreen );
    mAppearance.colorList->setColor( 4, darkMagenta );
    mAppearance.colorList->setColor( 5, blue );
    mAppearance.colorList->setColor( 6, red );
    mAppearance.colorList->setColor( 7, blue );
    mAppearance.colorList->setColor( 8, red );
    mAppearance.customColorCheck->setChecked( true );

    mAppearance.longFolderCheck->setChecked( true );
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
    mIdentityList.exportData();
    if( secondIdentity ) {
	KConfigGroupSaver saver(config, "Composer");
	long mShowHeaders = config->readNumEntry("headers", HDR_STANDARD);
	mShowHeaders |= HDR_IDENTITY;
	config->writeEntry("headers", mShowHeaders);
    }
  }
  if( activePage == mNetwork.pageIndex || everything )
  {
    // Sending mail
    if( mNetwork.sendmailRadio->isChecked() )
    {
      kernel->msgSender()->setMethod( KMSender::smMail );
    }
    else
    {
      kernel->msgSender()->setMethod( KMSender::smSMTP );
    }
    kernel->msgSender()->setMailer(mNetwork.sendmailLocationEdit->text() );
    kernel->msgSender()->setSmtpHost( mNetwork.smtpServerEdit->text() );
    kernel->msgSender()->setSmtpPort( mNetwork.smtpPortEdit->text().toInt() );
    kernel->msgSender()->setPrecommand( mNetwork.precommandEdit->text() );

    bool sendNow = mNetwork.sendMethodCombo->currentItem() == 0;
    kernel->msgSender()->setSendImmediate( sendNow );
    bool quotedPrintable = mNetwork.messagePropertyCombo->currentItem() == 1;
    kernel->msgSender()->setSendQuotedPrintable( quotedPrintable );
    kernel->msgSender()->writeConfig(FALSE);
    // Moved from composer page !
    KConfigGroupSaver saver(config, "Composer");
    bool confirmBeforeSend = mNetwork.confirmSendCheck->isChecked();
    config->writeEntry("confirm-before-send", confirmBeforeSend );

    // Add accounts marked as new
    QValueList< QGuardedPtr<KMAccount> >::Iterator it;
    for (it = mNewAccounts.begin(); it != mNewAccounts.end(); ++it )
      kernel->acctMgr()->add( *it );
    mNewAccounts.clear();

    // Update accounts that have been modified
    QValueList<mModifiedAccountsType*>::Iterator j;
    for (j = mModifiedAccounts.begin(); j != mModifiedAccounts.end(); ++j )
      (*j)->oldAccount->pseudoAssign( (*j)->newAccount );
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
      config->writeEntry("unicodeFont", mAppearance.unicodeFontCheck->
			 isChecked());
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
      bool defaultColors = !mAppearance.customColorCheck->isChecked();
      config->writeEntry("defaultColors", defaultColors );
      if (!defaultColors)
      {
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
      }
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
	charsetList.append( mComposer.charsetListBox->item( j )->text() );
      config->writeEntry("charsets", charsetList);

      bool autoSignature = mComposer.autoAppSignFileCheck->isChecked();
      config->writeEntry("signature", autoSignature ? "auto" : "manual" );
      config->writeEntry("smart-quote", mComposer.smartQuoteCheck->isChecked() );
      config->writeEntry("pgp-auto-sign",
			 mComposer.pgpAutoSignatureCheck->isChecked() );
      config->writeEntry("word-wrap", mComposer.wordWrapCheck->isChecked() );
      config->writeEntry("break-at", mComposer.wrapColumnSpin->value() );
      // charset settings
      if ( mComposer.defaultCharsetCombo->currentItem() == 0 )
	config->writeEntry("charset", "default");
      else
	config->writeEntry("charset", mComposer.defaultCharsetCombo->
			   currentText());
      config->writeEntry("force-reply-charset",
			 mComposer.forceReplyCharsetCheck->isChecked() );
    }
  }
  if( activePage == mMime.pageIndex || everything )
  {
    KConfigGroupSaver saver(config, "General");
    config->writeEntry( "createOwnMessageIdHeaders",
                       mMime.createOwnMessageIdCheck->isChecked() );
    config->writeEntry( "myMessageIdSuffix",
                       mMime.messageIdSuffixEdit->text() );

    int numValidEntry = 0;
    int numEntry = mMime.tagList->childCount();
    QListViewItem *item = mMime.tagList->firstChild();
    for (int i = 0; i < numEntry; i++)
    {
      KConfigGroupSaver saver(config, QString("Mime #%1").arg(i));
      if( item->text(0).length() > 0 )
      {
	config->writeEntry( "name",  item->text(0) );
	config->writeEntry( "value", item->text(1) );
	numValidEntry += 1;
      }
      item = item->nextSibling();
    }
    config->writeEntry("mime-header-count", numValidEntry );
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
    config->writeEntry( "keep-small-trash",
			mMisc.keepSmallTrashCheck->isChecked() );
    config->writeEntry( "small-trash-size",
			mMisc.smallTrashSizeSpin->value() );
    config->writeEntry( "remove-old-mail-from-trash",
			mMisc.removeOldMailCheck->isChecked() );
    config->writeEntry( "old-mail-age",
			mMisc.oldMailAgeSpin->value() );
    config->writeEntry( "old-mail-age-unit",
			mMisc.timeUnitCombo->currentItem() );
    config->writeEntry( "sendOnCheck",
			mMisc.sendOutboxCheck->isChecked() );
    config->writeEntry( "compact-all-on-exit",
			mMisc.compactOnExitCheck->isChecked() );
    config->writeEntry( "confirm-before-empty",
			mMisc.emptyFolderConfirmCheck->isChecked() );

    config->writeEntry( "beep-on-mail",
			mMisc.beepNewMailCheck->isChecked() );
    config->writeEntry( "msgbox-on-mail",
			mMisc.showMessageBoxCheck->isChecked() );
    config->writeEntry( "exec-on-mail",
			mMisc.mailCommandCheck->isChecked() );
    config->writeEntry( "exec-on-mail-cmd",
			mMisc.mailCommandEdit->text() );
  }

  kdDebug(5006) << "KMScoringManager::globalScoringManager()->save();" << endl;
  KMScoringManager::globalScoringManager()->save();

  //
  // Always
  //
  KConfigGroupSaver saver(config, "General");
  config->writeEntry("first-start", false);
  config->sync();

  //
  // Make other components read the new settings
  //
  KMMessage::readConfig();
  kernel->kbp()->busy(); // this can take some time when a large folder is open
  QListIterator<KMainWindow> it(*KMainWindow::memberList);
  for( it.toFirst(); it.current(); ++it )
  {
    if (it.current()->inherits("KMTopLevelWidget"))
    {
      ((KMTopLevelWidget*)it.current())->readConfig();
    }
  }
  kernel->kbp()->idle();
}






void ConfigureDialog::saveActiveIdentity( void )
{
  IdentityEntry *entry = mIdentityList.get(mIdentity.mActiveIdentity);
  if( entry != 0 )
  {
    entry->setFullName( mIdentity.nameEdit->text() );
    entry->setOrganization( mIdentity.organizationEdit->text() );
    entry->setPgpIdentity( mIdentity.pgpIdentityEdit->text() );
    entry->setEmailAddress( mIdentity.emailEdit->text() );
    entry->setReplyToAddress( mIdentity.replytoEdit->text() );
    entry->setSignatureFileName( mIdentity.signatureFileEdit->url() );
    entry->setSignatureInlineText( mIdentity.signatureTextEdit->text() );
    entry->setSignatureFileIsAProgram(
      mIdentity.signatureExecCheck->isChecked() );
    entry->setUseSignatureFile( mIdentity.signatureFileRadio->isChecked() );
  }
}


void ConfigureDialog::setIdentityInformation( const QString &identity )
{
  if( mIdentity.mActiveIdentity == identity )
  {
    return;
  }

  //
  // 1. Save current settings to the list
  //
  saveActiveIdentity();

  mIdentity.mActiveIdentity = identity;

  //
  // 2. Display the new settings
  //
  bool useSignatureFile;
  IdentityEntry *entry = mIdentityList.get( mIdentity.mActiveIdentity );
  if( entry == 0 )
  {
    mIdentity.nameEdit->clear();
    mIdentity.organizationEdit->clear();
    mIdentity.pgpIdentityEdit->clear();
    mIdentity.emailEdit->clear();
    mIdentity.replytoEdit->clear();
    mIdentity.signatureFileEdit->clear();
    mIdentity.signatureExecCheck->setChecked( false );
    mIdentity.signatureTextEdit->clear();
    useSignatureFile = true;
  }
  else
  {
    mIdentity.nameEdit->setText( entry->fullName() );
    mIdentity.organizationEdit->setText( entry->organization() );
    mIdentity.pgpIdentityEdit->setText( entry->pgpIdentity() );
    mIdentity.emailEdit->setText( entry->emailAddress() );
    mIdentity.replytoEdit->setText( entry->replyToAddress() );
    mIdentity.signatureFileEdit->setURL( entry->signatureFileName() );
    mIdentity.signatureExecCheck->setChecked(entry->signatureFileIsAProgram());
    mIdentity.signatureTextEdit->setText( entry->signatureInlineText() );
    useSignatureFile = entry->useSignatureFile();
    if (entry->transport().isNull() || entry->transport().isEmpty())
      mIdentity.transportButton->setText(i18n("Add Transport..."));
    else
      mIdentity.transportButton->setText(i18n("Edit Transport..."));
  }

  if( useSignatureFile == true )
  {
    mIdentity.signatureFileRadio->setChecked(true);
    slotSignatureType(0);
  }
  else
  {
    mIdentity.signatureTextRadio->setChecked(true);
    slotSignatureType(1);
  }
}


QStringList ConfigureDialog::identityStrings( void )
{
  QStringList list;
  for( int i=0; i< mIdentity.identityCombo->count(); i++ )
  {
    list += mIdentity.identityCombo->text(i);
  }
  return( list );
}



void ConfigureDialog::slotNewIdentity( void )
{
  //
  // First. Save current setting to the list. In the dialog box we
  // can choose to copy from the list so it must be synced.
  //
  saveActiveIdentity();

  //
  // Make and open the dialog
  //
  NewIdentityDialog *dialog = new NewIdentityDialog( this, "new", true );
  QStringList list = identityStrings();
  dialog->setIdentities( list );

  int result = dialog->exec();
  if( result == QDialog::Accepted )
  {
    QString identityText = dialog->identityText().stripWhiteSpace();
    if( identityText.isEmpty() == false )
    {
      if (list.count() == 1)
	  secondIdentity = true;

      //
      // Add the new identity. Make sure the default identity is
      // first in the otherwise sorted list
      //
      QString defaultIdentity = list.first();
      list.remove( defaultIdentity );
      list += identityText;
      list.sort();
      list.prepend( defaultIdentity );

      //
      // Set the modifiled list as the valid list in the combo and
      // make the new identity the current item.
      //
      mIdentity.identityCombo->clear();
      mIdentity.identityCombo->insertStringList(list);
      mIdentity.identityCombo->setCurrentItem( list.findIndex(identityText) );

      if( dialog->duplicateMode() == NewIdentityDialog::ControlCenter )
      {
	mIdentityList.add( identityText, this, true );
      }
      else if( dialog->duplicateMode() == NewIdentityDialog::ExistingEntry )
      {
	mIdentityList.add( identityText, dialog->duplicateText() );
      }
      else
      {
	mIdentityList.add( identityText, this, false );
      }
      slotIdentitySelectorChanged();
    }
  }
  delete dialog;
}


void ConfigureDialog::slotRenameIdentity( void )
{
  RenameIdentityDialog *dialog = new RenameIdentityDialog( this, "new", true );

  QStringList list = identityStrings();
  dialog->setIdentities( mIdentity.identityCombo->currentText(), list );

  int result = dialog->exec();
  if( result == QDialog::Accepted )
  {
    int index = mIdentity.identityCombo->currentItem();
    IdentityEntry *entry = mIdentityList.get( index );
    if( entry != 0 )
    {
      entry->setIdentity( dialog->identityText() );
      mIdentity.mActiveIdentity = entry->identity();
      mIdentity.identityCombo->clear();
      mIdentity.identityCombo->insertStringList( mIdentityList.identities() );
      mIdentity.identityCombo->setCurrentItem( index );
    }
  }

  delete dialog;
}


void ConfigureDialog::slotRemoveIdentity( void )
{
  int currentItem = mIdentity.identityCombo->currentItem();
  if( currentItem > 0 ) // Item 0 is the default and can not be removed.
  {
    QString msg = i18n(
     "Do you really want to remove the identity\n"
     "named \"%1\" ?").arg(mIdentity.identityCombo->currentText());
    int result = KMessageBox::warningYesNo( this, msg );
    if( result == KMessageBox::Yes )
    {
      mIdentityList.remove( mIdentity.identityCombo->currentText() );
      mIdentity.identityCombo->removeItem( currentItem );
      mIdentity.identityCombo->setCurrentItem( currentItem-1 );
      slotIdentitySelectorChanged();
    }
  }
}


void ConfigureDialog::slotIdentitySelectorChanged( void )
{
  int currentItem = mIdentity.identityCombo->currentItem();
  mIdentity.removeIdentityButton->setEnabled( currentItem != 0 );
  mIdentity.renameIdentityButton->setEnabled( currentItem != 0 );
  setIdentityInformation( mIdentity.identityCombo->currentText() );
  if (currentItem == 0) {
    mIdentity.transportButton->setEnabled(false);
    mIdentity.transportButton->hide();
  }
  else {
    mIdentity.transportButton->setEnabled(true);
    mIdentity.transportButton->show();
  }
}


void ConfigureDialog::slotSignatureType( int id )
{
  bool flag;
  if( id == 0 )
  {
    flag = true;
  }
  else if( id == 1 )
  {
    flag = false;
  }
  else
  {
    return;
  }

  mIdentity.signatureFileLabel->setEnabled( flag );
  mIdentity.signatureFileEdit->setEnabled( flag );
  mIdentity.signatureExecCheck->setEnabled( flag );
  if( flag==true )
  {
    mIdentity.signatureEditButton->setEnabled(
      !mIdentity.signatureExecCheck->isChecked() );
  }
  else
  {
    mIdentity.signatureEditButton->setEnabled( false );
  }
  mIdentity.signatureTextEdit->setEnabled( !flag );
}


void ConfigureDialog::slotSignatureChooser( KURLRequester *req )
{
  if ( req->url().isEmpty() )
    req->fileDialog()->setURL( QDir::homeDirPath() );

  req->fileDialog()->setCaption(i18n("Choose Signature File"));
}


void ConfigureDialog::slotSignatureFile( const QString &filename )
{
  QString path = filename.stripWhiteSpace();
  if( mIdentity.signatureFileRadio->isChecked() == true )
  {
    bool state = path.isEmpty() == false ? true : false;
    mIdentity.signatureEditButton->setEnabled( state );
    mIdentity.signatureExecCheck->setEnabled( state );
  }
}


void ConfigureDialog::slotSignatureEdit( void )
{
  QString fileName = mIdentity.signatureFileEdit->url().stripWhiteSpace();
  if( fileName.isEmpty() == true )
  {
    KMessageBox::error( this, i18n("You must specify a filename") );
    return;
  }

  QFileInfo fileInfo( fileName );
  if( fileInfo.isDir() == true )
  {
    QString msg = i18n("You have specified a directory\n\n%1").arg(fileName);
    KMessageBox::error( this, msg );
    return;
  }

  if( fileInfo.exists() == false )
  {
    // Create the file first
    QFile file( fileName );
    if( file.open( IO_ReadWrite ) == false )
    {
      QString msg = i18n("Unable to create new file at\n\n%1").arg(fileName);
      KMessageBox::error( this, msg );
      return;
    }
  }

  QString cmdline = QString(DEFAULT_EDITOR_STR);

  QString argument = "\"" + fileName + "\"";
  ApplicationLaunch kl(cmdline.replace(QRegExp("\\%f"), argument ));
  kl.run();
}


void ConfigureDialog::slotSignatureExecMode( bool state )
{
  mIdentity.signatureEditButton->setEnabled( !state );
}

//
// Network page
//

void ConfigureDialog::slotSendmailChooser( void )
{
  KFileDialog dialog("/", QString::null, this, 0, true );
  dialog.setCaption(i18n("Choose Sendmail Location") );

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

    mNetwork.sendmailLocationEdit->setText( url.path() );
  }
}


void ConfigureDialog::slotSendmailType( int id )
{
  bool useSendmail;
  if( id == 0 )
  {
    useSendmail = true;
  }
  else if( id == 1 )
  {
    useSendmail = false;
  }
  else
  {
    return;
  }

  mNetwork.sendmailLocationEdit->setEnabled( useSendmail );
  mNetwork.sendmailChooseButton->setEnabled( useSendmail );
  mNetwork.smtpServerEdit->setEnabled( !useSendmail );
  mNetwork.smtpPortEdit->setEnabled( !useSendmail );
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

  AccountDialog *dialog = new AccountDialog( account, identityStrings(), this);
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
  AccountDialog *dialog = new AccountDialog( account, identityStrings(), this);
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


void ConfigureDialog::slotCreateOwnMessageIdChanged( void )
{
  bool flag = mMime.createOwnMessageIdCheck->isChecked();
  mMime.messageIdSuffixLabel->setEnabled( flag );
  mMime.messageIdSuffixEdit->setEnabled( flag );
  mMime.messageIdSuffixHintLabel->setEnabled( flag );
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
    mComposer.replyListBox->insertItem( linedlg->text() );
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
    mComposer.forwardListBox->insertItem( linedlg->text() );
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
    if (linedlg->text().lower() == "us-ascii" ||
      KMMsgBase::codecForName( linedlg->text() ))
    {
      mComposer.charsetListBox->insertItem( linedlg->text(),
        mComposer.charsetListBox->currentItem() + 1 );
      mComposer.charsetListBox->setSelected( mComposer.charsetListBox->
        currentItem() + 1, TRUE );
      mComposer.defaultCharsetCombo->insertItem( linedlg->text() );
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
    for (int i = 0; i < mComposer.defaultCharsetCombo->count(); i++)
    {
      if (mComposer.defaultCharsetCombo->text( i ) ==
          mComposer.charsetListBox->currentText())
      {
        mComposer.defaultCharsetCombo->removeItem( i );
        break;
      }
    }
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
  mMime.currentTagItem = mMime.tagList->selectedItem();
  if( mMime.currentTagItem != 0 )
  {
    mMime.tagNameEdit->setText( mMime.currentTagItem->text(0) );
    mMime.tagValueEdit->setText( mMime.currentTagItem->text(1) );
    mMime.tagNameEdit->setEnabled(true);
    mMime.tagValueEdit->setEnabled(true);
    mMime.tagNameLabel->setEnabled(true);
    mMime.tagValueLabel->setEnabled(true);
  }
}


void ConfigureDialog::slotMimeHeaderNameChanged( const QString &text )
{
  if( mMime.currentTagItem != 0 )
  {
    mMime.currentTagItem->setText(0, text );
  }
}


void ConfigureDialog::slotMimeHeaderValueChanged( const QString &text )
{
  if( mMime.currentTagItem != 0 )
  {
    mMime.currentTagItem->setText(1, text );
  }
}


void ConfigureDialog::slotNewMimeHeader( void )
{
  QListViewItem *listItem = new QListViewItem( mMime.tagList, "", "" );
  mMime.tagList->setCurrentItem( listItem );
  mMime.tagList->setSelected( listItem, true );

  mMime.currentTagItem = mMime.tagList->selectedItem();
  if( mMime.currentTagItem != 0 )
  {
    mMime.tagNameEdit->setEnabled(true);
    mMime.tagValueEdit->setEnabled(true);
    mMime.tagNameLabel->setEnabled(true);
    mMime.tagValueLabel->setEnabled(true);
    mMime.tagNameEdit->setFocus();
  }
}


void ConfigureDialog::slotDeleteMimeHeader( void )
{
  if( mMime.currentTagItem != 0 )
  {
    QListViewItem *next = mMime.currentTagItem->itemAbove();
    if( next == 0 )
    {
      next = mMime.currentTagItem->itemBelow();
    }

    mMime.tagNameEdit->clear();
    mMime.tagValueEdit->clear();
    mMime.tagNameEdit->setEnabled(false);
    mMime.tagValueEdit->setEnabled(false);
    mMime.tagNameLabel->setEnabled(false);
    mMime.tagValueLabel->setEnabled(false);

    mMime.tagList->takeItem( mMime.currentTagItem );
    mMime.currentTagItem = 0;

    if( next != 0 )
    {
      mMime.tagList->setSelected( next, true );
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


void ConfigureDialog::slotMailCommandSelectionChanged( void )
{
  bool flag = mMisc.mailCommandCheck->isChecked();
  mMisc.mailCommandEdit->setEnabled( flag );
  mMisc.mailCommandChooseButton->setEnabled( flag );
  mMisc.mailCommandLabel->setEnabled( flag );
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

    mMisc.mailCommandEdit->setText( url.path() );
  }
}

void ConfigureDialog::slotEmptyTrashState( int state)
{
  bool on = ( state == 0 ); // button not checked
  mMisc.removeOldMailCheck->setEnabled( on );
  mMisc.oldMailAgeSpin->setEnabled( on );
  mMisc.timeUnitCombo->setEnabled( on );
  mMisc.keepSmallTrashCheck->setEnabled( on );
  mMisc.smallTrashSizeSpin->setEnabled( on );
}

IdentityEntry::IdentityEntry( void )
{
  mSignatureFileIsAProgram = false;
  mUseSignatureFile = true;
}


QString IdentityEntry::identity() const
{
  return( mIdentity );
}

QString IdentityEntry::fullName() const
{
  return( mFullName );
}

QString IdentityEntry::organization() const
{
  return( mOrganization );
}

QString IdentityEntry::pgpIdentity() const
{
  return( mPgpIdentity );
}

QString IdentityEntry::emailAddress() const
{
  return( mEmailAddress );
}

QString IdentityEntry::replyToAddress() const
{
  return( mReplytoAddress );
}

QString IdentityEntry::signatureFileName( bool exportIdentity ) const
{
  if( exportIdentity == true && mSignatureFileIsAProgram == true )
  {
    printf("exportIdentity=%d\n", exportIdentity );
    printf("mSignatureFileIsAProgram=%d\n", mSignatureFileIsAProgram );

    return( mSignatureFileName + "|" );
  }
  else
  {
    return( mSignatureFileName );
  }
}

QString IdentityEntry::signatureInlineText() const
{
  return( mSignatureInlineText );
}

bool IdentityEntry::signatureFileIsAProgram() const
{
  return( mSignatureFileIsAProgram );
}

bool IdentityEntry::useSignatureFile() const
{
  return( mUseSignatureFile );
}

QString IdentityEntry::transport() const
{
  return ( mTransport );
}


void IdentityEntry::setIdentity( const QString &identity )
{
  mIdentity = identity;
}

void IdentityEntry::setFullName( const QString &fullName )
{
  mFullName = fullName;
}

void IdentityEntry::setOrganization( const QString &organization )
{
  mOrganization = organization;
}

void IdentityEntry::setPgpIdentity( const QString &pgpIdentity )
{
  mPgpIdentity = pgpIdentity;
}

void IdentityEntry::setEmailAddress( const QString &emailAddress )
{
  mEmailAddress = emailAddress;
}

void IdentityEntry::setReplyToAddress( const QString &replytoAddress )
{
  mReplytoAddress = replytoAddress;
}

void IdentityEntry::setSignatureFileName( const QString &signatureFileName,
					  bool importIdentity )
{
  if( importIdentity == true )
  {
    if( signatureFileName.right(1) == "|" )
    {
      mSignatureFileName=signatureFileName.left(signatureFileName.length()-1);
      setSignatureFileIsAProgram(true);
    }
    else
    {
      mSignatureFileName=signatureFileName;
      setSignatureFileIsAProgram(false);
    }
  }
  else
  {
    mSignatureFileName=signatureFileName;
  }
}

void IdentityEntry::setSignatureInlineText( const QString &signatureInlineText)
{
  mSignatureInlineText = signatureInlineText;
}

void IdentityEntry::setSignatureFileIsAProgram( bool signatureFileIsAProgram )
{
  mSignatureFileIsAProgram = signatureFileIsAProgram;
}

void IdentityEntry::setUseSignatureFile( bool useSignatureFile )
{
  mUseSignatureFile = useSignatureFile;
}

void IdentityEntry::setTransport( const QString &transport )
{
  mTransport = transport;
}



IdentityList::IdentityList()
{
  mList.setAutoDelete(true);
}


QStringList IdentityList::identities()
{
  QStringList list;
  for( IdentityEntry *e = mList.first(); e != 0; e = mList.next() )
  {
    list += e->identity();
  }
  return( list );
}


IdentityEntry *IdentityList::get( const QString &identity )
{
  for( IdentityEntry *e = mList.first(); e != 0; e = mList.next() )
  {
    if( identity == e->identity() )
    {
      return( e );
    }
  }
  return( 0 );
}


IdentityEntry *IdentityList::get( uint index )
{
  return( mList.at(index) );
}


void IdentityList::remove( const QString &identity )
{
  IdentityEntry *e = get(identity);
  if( e != 0 )
  {
    mList.remove(e);
  }
}



void IdentityList::importData()
{
  mList.clear();
  IdentityEntry entry;
  QStringList identities = KMIdentity::identities();
  QStringList::Iterator it;
  for( it = identities.begin(); it != identities.end(); ++it )
  {
    KMIdentity ident( *it );
    ident.readConfig();
    entry.setIdentity( ident.identity() );
    entry.setFullName( ident.fullName() );
    entry.setOrganization( ident.organization() );
    entry.setPgpIdentity( ident.pgpIdentity() );
    entry.setEmailAddress( ident.emailAddr() );
    entry.setReplyToAddress( ident.replyToAddr() );
    entry.setSignatureFileName( ident.signatureFile(), true );
    entry.setSignatureInlineText( ident.signatureInlineText() );
    entry.setUseSignatureFile( ident.useSignatureFile() );
    entry.setTransport(ident.transport());
    add( entry );
  }
}


void IdentityList::exportData()
{
  QStringList ids;
  for( IdentityEntry *e = mList.first(); e != 0; e = mList.next() )
  {
    KMIdentity ident( e->identity() );
    ident.setFullName( e->fullName() );
    ident.setOrganization( e->organization() );
    ident.setPgpIdentity( e->pgpIdentity() );
    ident.setEmailAddr( e->emailAddress() );
    ident.setReplyToAddr( e->replyToAddress() );
    ident.setUseSignatureFile( e->useSignatureFile() );
    ident.setSignatureFile( e->signatureFileName(true) );
    ident.setSignatureInlineText( e->signatureInlineText() );
    ident.setTransport( e->transport() );
    ident.writeConfig(false);
    ids.append( e->identity() );
  }

  KMIdentity::saveIdentities( ids, false );
}



void IdentityList::add( const IdentityEntry &entry )
{
  if( get( entry.identity() ) != 0 )
  {
    return; // We can not have duplicates.
  }

  mList.append( new IdentityEntry(entry) );
}


void IdentityList::add( const QString &identity, const QString &copyFrom )
{
  if( get( identity ) != 0 )
  {
    return; // We can not have duplicates.
  }

  IdentityEntry newEntry;

  IdentityEntry *src = get( copyFrom );
  if( src != 0 )
  {
    newEntry = *src;
  }

  newEntry.setIdentity( identity );
  add( newEntry );
}


void IdentityList::add( const QString &identity, QWidget *parent,
			bool useControlCenter )
{
  if( get( identity ) != 0 )
  {
    return; // We can not have duplicates.
  }

  IdentityEntry newEntry;

  newEntry.setIdentity( identity );
  if( useControlCenter == true )
  {
    //
    // The returned filename is empty if the file exists but
    // is not readable so we only have to test if the file exists.
    //
    QString configFileName = locate( "config", "emaildefaults" );
    QFileInfo fileInfo(configFileName);
    if( fileInfo.exists() == false )
    {
      QString msg = i18n(""
        "The email configuration file could not be located.\n"
	"You can create one in Control Center.\n\n"
	"(Search for \"email\" in Control Center)");
      KMessageBox::error( parent, msg );
    }
    else
    {
      KSimpleConfig config( configFileName, false );
      config.setGroup("UserInfo");

      newEntry.setFullName( config.readEntry( "FullName", "" ) );
      newEntry.setEmailAddress( config.readEntry( "EmailAddress", "" ) );
      newEntry.setOrganization( config.readEntry( "Organization", "" ) );
      newEntry.setReplyToAddress( config.readEntry( "ReplyAddr", "" ) );
    }
  }
  add( newEntry );
}


void IdentityList::update( const IdentityEntry &entry )
{
  for( IdentityEntry *e = mList.first(); e != 0; e = mList.next() )
  {
    if( entry.identity() == e->identity() )
    {
      *e = entry;
      return;
    }
  }
}


// ConfigureTransportDialog stuff below here.

ConfigureTransportDialog::ConfigureTransportDialog(QWidget *parent, const char *name, bool modal, const QString &transport) : KDialogBase( parent, name, modal, i18n("Configure Transport"), Ok|Cancel, Ok, true )
{
  QFrame *frame = makeMainWidget();
  QButtonGroup *buttonGroup = new QButtonGroup( frame );
  buttonGroup->hide();
  connect( buttonGroup, SIGNAL(clicked(int)),
	   this, SLOT(slotSendmailType(int)) );

  QGridLayout *glay = new QGridLayout( frame, 6, 4, spacingHint() );
  glay->addColSpacing( 2, fontMetrics().maxWidth()*15 );

  this->sendmailRadio = new QRadioButton( i18n("Sendmail"), frame );
  buttonGroup->insert(this->sendmailRadio);
  glay->addMultiCellWidget(this->sendmailRadio, 0, 0, 0, 3);
  QLabel *label = new QLabel( i18n("Location:"), frame );
  glay->addWidget( label, 1, 1 );
  this->sendmailLocationEdit = new QLineEdit( frame );
  glay->addWidget( this->sendmailLocationEdit, 1, 2 );
  this->sendmailChooseButton =
    new QPushButton( i18n("Choose..."), frame );
  connect( this->sendmailChooseButton, SIGNAL(clicked()),
	   this, SLOT(slotSendmailChooser()) );
  this->sendmailChooseButton->setAutoDefault( false );
  glay->addWidget( this->sendmailChooseButton, 1, 3 );

  this->smtpRadio = new QRadioButton( i18n("SMTP"), frame );
  buttonGroup->insert(this->smtpRadio);
  glay->addMultiCellWidget(this->smtpRadio, 2, 2, 0, 3);
  label = new QLabel( i18n("Server:"), frame );
  glay->addWidget( label, 3, 1 );
  this->smtpServerEdit = new QLineEdit( frame );
  glay->addWidget( this->smtpServerEdit, 3, 2 );
  label = new QLabel( i18n("Port:"), frame );
  glay->addWidget( label, 4, 1 );
  this->smtpPortEdit = new QLineEdit( frame );
  this->smtpPortEdit->setValidator( new QIntValidator(frame) );
  glay->addWidget( this->smtpPortEdit, 4, 2 );

  this->deleteRadio = new QRadioButton(i18n("Remove this transport."), frame);
  buttonGroup->insert(this->deleteRadio);
  glay->addMultiCellWidget(this->deleteRadio, 5, 5, 0, 3);

  // Set widgets to default values:
  sendmailLocationEdit->setText(kernel->msgSender()->mailer());
  // Reset the widgets based on passed in values.
  mTransport = transport;
  if (!mTransport.isEmpty()) {
    if (mTransport.left(5) == "smtp:") {
      int lastColon = mTransport.findRev(':');
      smtpRadio->setChecked(true);
      smtpServerEdit->setText(mTransport.mid(7,lastColon - 7));
      smtpPortEdit->setText(mTransport.mid(lastColon + 1, mTransport.length()
                                           - (lastColon + 1)));
      this->slotSendmailType(1);
    }
    else {
      sendmailRadio->setChecked(true);
      sendmailLocationEdit->setText(mTransport);
      this->slotSendmailType(0);
    }
  }
}

QString ConfigureTransportDialog::getTransport(void)
{
  return mTransport;
}

void ConfigureTransportDialog::slotOk( void )
{
  if (smtpRadio->isChecked()) {
    if (!smtpServerEdit->text().isEmpty()) {
      mTransport = "smtp://" + smtpServerEdit->text() + ":";
      if (smtpPortEdit->text().isNull()) mTransport += "25";
      else mTransport += smtpPortEdit->text();
    }
  }
  else if (sendmailRadio->isChecked()) {
    if (!sendmailLocationEdit->text().isEmpty())
      mTransport = sendmailLocationEdit->text();
  }
  else if (deleteRadio->isChecked()) {
    mTransport = "";
  }
  accept();
}

void ConfigureTransportDialog::slotCancel( void )
{
  reject();
}

void ConfigureTransportDialog::slotSendmailType( int id )
{
  bool useSendmail;
  if( id == 0 )
  {
    useSendmail = true;
  }
  else if( id == 1 )
  {
    useSendmail = false;
  }
  else
  {
    this->sendmailLocationEdit->setEnabled(false);
    this->sendmailChooseButton->setEnabled(false);
    this->smtpServerEdit->setEnabled(false);
    this->smtpPortEdit->setEnabled(false);
    return;
  }

  this->sendmailLocationEdit->setEnabled( useSendmail );
  this->sendmailChooseButton->setEnabled( useSendmail );
  this->smtpServerEdit->setEnabled( !useSendmail );
  this->smtpPortEdit->setEnabled( !useSendmail );
}

void ConfigureTransportDialog::slotSendmailChooser( void )
{
  KFileDialog dialog("/", QString::null, this, 0, true );
  dialog.setCaption(i18n("Choose Sendmail Location") );

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

    this->sendmailLocationEdit->setText( url.path() );
  }
}

// This is the ConfigureDialog slot for the button that opens the
// ConfigureTransportDialog. I put it down here, so I can easily
// find it while working on it.
void ConfigureDialog::slotIdentityTransport(void)
{
  IdentityEntry *id;
  id = mIdentityList.get(mIdentity.mActiveIdentity);
  // Create the dialog.
  ConfigureTransportDialog ctd(this, 0, true, id->transport());
  ctd.setCaption(i18n("Configure Transport"));
  if (ctd.exec() == QDialog::Accepted) {
    id->setTransport(ctd.getTransport());
    if (ctd.getTransport().isEmpty())
      mIdentity.transportButton->setText("Add Transport...");
    else
      mIdentity.transportButton->setText("Edit Transport...");
  }
}
