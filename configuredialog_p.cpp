#ifndef KDE_USE_FINAL
#define QT_NO_CAST_ASCII
#endif
// configuredialog_p.cpp: classes internal to ConfigureDialog
// see configuredialog.cpp for details.

// This must be first
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// my header:
#include "configuredialog_p.h"

// other KMail headers:
#include "kmidentity.h" // for IdentityList::{export,import}Data

// other kdenetwork headers: (none)

// other KDE headers:
#include <kemailsettings.h> // for IdentityEntry::fromControlCenter()
#include <kmtransport.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>

// Qt headers:
#include <qheader.h>
#include <qtabwidget.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qlabel.h>
#include <qlayout.h>

// Other headers:
#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

// used in IdentityList::{import,export}Data
static QString pipeSymbol = QString::fromLatin1("|");

//********************************************************
// Identity handling
//********************************************************

IdentityEntry IdentityEntry::fromControlCenter()
{
  KEMailSettings emailSettings;
  emailSettings.setProfile( emailSettings.defaultProfileName() );

  return // use return optimization (saves a copy)
    IdentityEntry( emailSettings.getSetting( KEMailSettings::RealName ),
		   emailSettings.getSetting( KEMailSettings::EmailAddress ),
		   emailSettings.getSetting( KEMailSettings::Organization ),
		   emailSettings.getSetting( KEMailSettings::ReplyToAddress ) );
}



QStringList IdentityList::names() const
{
  QStringList list;
  QValueListConstIterator<IdentityEntry> it = begin();
  for( ; it != end() ; ++it )
    list << (*it).identityName();
  return list;
}


IdentityEntry & IdentityList::getByName( const QString & i )
{
  QValueListIterator<IdentityEntry> it = begin();
  for( ; it != end() ; ++it )
    if( i == (*it).identityName() )
      return (*it);
  assert( 0 ); // one should throw an exception instead, but well...
}

void IdentityList::importData()
{
  clear();
  bool defaultIdentity = true;

  QStringList identities = KMIdentity::identities();
  QStringList::Iterator it = identities.begin();
  for( ; it != identities.end(); ++it )
  {
    KMIdentity ident( *it );
    ident.readConfig();

    IdentityEntry entry;
    entry.setIsDefault( defaultIdentity );
    // only the first entry is the default one:
    defaultIdentity = false;

    entry.setIdentityName( ident.identity() );
    entry.setFullName( ident.fullName() );
    entry.setOrganization( ident.organization() );
    entry.setPgpIdentity( ident.pgpIdentity() );
    entry.setEmailAddress( ident.emailAddr() );
    entry.setReplyToAddress( ident.replyToAddr() );

    // We encode the "use output of command" case with a trailing pipe
    // symbol to distinguish it from the case of a normal data file.
    QString signatureFileName = ident.signatureFile();
    if( signatureFileName.endsWith( pipeSymbol ) ) {
      // it's a command: chop off the "|".
      entry.setSignatureFileName( signatureFileName
			    .left( signatureFileName.length() - 1 ) );
      entry.setSignatureFileIsAProgram( true );
    } else {
      // it's an ordinary file:
      entry.setSignatureFileName( signatureFileName );
      entry.setSignatureFileIsAProgram( false );
    }
    
    entry.setSignatureInlineText( ident.signatureInlineText() );
    entry.setUseSignatureFile( ident.useSignatureFile() );
    entry.setTransport( ident.transport() );
    entry.setFcc( ident.fcc() );
    entry.setDrafts( ident.drafts() );

    append( entry );
  }
}


void IdentityList::exportData() const
{
  QStringList identityNames;
  QValueListConstIterator<IdentityEntry> it = begin();
  for( ; it != end() ; ++it )
  {
    KMIdentity ident( (*it).identityName() );
    ident.setFullName( (*it).fullName() );
    ident.setOrganization( (*it).organization() );
    ident.setPgpIdentity( (*it).pgpIdentity().local8Bit() );
    ident.setEmailAddr( (*it).emailAddress() );
    ident.setReplyToAddr( (*it).replyToAddress() );
    ident.setUseSignatureFile( (*it).useSignatureFile() );
    
    // We encode the "use output of command" case with a trailing pipe
    // symbol to distinguish it from the case of a normal data file.
    QString signatureFileName = (*it).signatureFileName();
    if ( (*it).signatureFileIsAProgram() ) 
      signatureFileName += pipeSymbol;
    ident.setSignatureFile( signatureFileName );

    ident.setSignatureInlineText( (*it).signatureInlineText() );
    ident.setTransport( (*it).transport() );
    ident.setFcc( (*it).fcc() );
    ident.setDrafts( (*it).drafts() );

    ident.writeConfig( false ); // saves the identity data

    identityNames << (*it).identityName();
  }

  KMIdentity::saveIdentities( identityNames, false ); // writes a list of names
}




NewIdentityDialog::NewIdentityDialog( const QStringList & identities,
				      QWidget *parent, const char *name,
				      bool modal )
  : KDialogBase( parent, name, modal, i18n("New Identity"),
		 Ok|Cancel|Help, Ok, true )
{
  setHelp( QString::fromLatin1("configure-identity-newidentitydialog") );
  QWidget * page = makeMainWidget();
  QVBoxLayout * vlay = new QVBoxLayout( page, 0, spacingHint() );

  // row 0: line edit with label
  QHBoxLayout * hlay = new QHBoxLayout( vlay ); // inherits spacing
  mLineEdit = new QLineEdit( page );
  mLineEdit->setFocus();
  hlay->addWidget( new QLabel( mLineEdit, i18n("&New Identity:"), page ) );
  hlay->addWidget( mLineEdit, 1 );
  connect( mLineEdit, SIGNAL(textChanged(const QString&)),
	   this, SLOT(slotEnableOK(const QString&)) );

  mButtonGroup = new QButtonGroup( page );
  mButtonGroup->hide();

  // row 1: radio button
  QRadioButton *radio = new QRadioButton( i18n("&With empty fields"), page );
  radio->setChecked( true );
  mButtonGroup->insert( radio, Empty );
  vlay->addWidget( radio );

  // row 2: radio button
  radio = new QRadioButton( i18n("&Use Control Center settings"), page );
  mButtonGroup->insert( radio, ControlCenter );
  vlay->addWidget( radio );

  // row 3: radio button
  radio = new QRadioButton( i18n("&Duplicate existing identity"), page );
  mButtonGroup->insert( radio, ExistingEntry );
  vlay->addWidget( radio );

  // row 4: combobox with existing identities and label
  hlay = new QHBoxLayout( vlay ); // inherits spacing
  mComboBox = new QComboBox( false, page );
  mComboBox->insertStringList( identities );
  mComboBox->setEnabled( false );
  QLabel *label = new QLabel( mComboBox, i18n("&Existing identities:"), page );
  label->setEnabled( false );
  hlay->addWidget( label );
  hlay->addWidget( mComboBox, 1 );

  vlay->addStretch( 1 ); // spacer

  // enable/disable combobox and label depending on the third radio
  // button's state:
  connect( radio, SIGNAL(toggled(bool)),
	   label, SLOT(setEnabled(bool)) );
  connect( radio, SIGNAL(toggled(bool)),
	   mComboBox, SLOT(setEnabled(bool)) );

  enableButtonOK( false ); // since line edit is empty
}

NewIdentityDialog::DuplicateMode NewIdentityDialog::duplicateMode() const {
  int id = mButtonGroup->id( mButtonGroup->selected() );
  assert( id == (int)Empty
	  || id == (int)ControlCenter
	  || id == (int)ExistingEntry );
  return static_cast<DuplicateMode>( id );
}

void NewIdentityDialog::slotEnableOK( const QString & proposedIdentityName ) {
  // OK button is disabled if
  QString name = proposedIdentityName.stripWhiteSpace();
  // name isn't empty
  if ( name.isEmpty() ) {
    enableButtonOK( false );
    return;
  }
  // or name doesn't yet exist.
  for ( int i = 0 ; i < mComboBox->count() ; i++ )
    if ( mComboBox->text(i) == name ) {
      enableButtonOK( false );
      return;
    }
  enableButtonOK( true );
}

void ApplicationLaunch::doIt()
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
  system(mCmdLine.latin1());
}

void ApplicationLaunch::run()
{
  signal(SIGCHLD, SIG_IGN);  // see comment above.
  if( fork() == 0 )
  {
    doIt();
    exit(0);
  }
}


ListView::ListView( QWidget *parent, const char *name,
				     int visibleItem )
  : KListView( parent, name )
{
  setVisibleItem(visibleItem);
}


void ListView::resizeEvent( QResizeEvent *e )
{
  KListView::resizeEvent(e);
  resizeColums();
}


void ListView::showEvent( QShowEvent *e )
{
  KListView::showEvent(e);
  resizeColums();
}


void ListView::resizeColums()
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


void ListView::setVisibleItem( int visibleItem, bool updateSize )
{
  mVisibleItem = QMAX( 1, visibleItem );
  if( updateSize == true )
  {
    QSize s = sizeHint();
    setMinimumSize( s.width() + verticalScrollBar()->sizeHint().width() +
		    lineWidth() * 2, s.height() );
  }
}


QSize ListView::sizeHint() const
{
  QSize s = QListView::sizeHint();

  int h = fontMetrics().height() + 2*itemMargin();
  if( h % 2 > 0 ) { h++; }

  s.setHeight( h*mVisibleItem + lineWidth()*2 + header()->sizeHint().height());
  return s;
}


static QString flagPng = QString::fromLatin1("/flag.png");

NewLanguageDialog::NewLanguageDialog( LanguageItemList & suppressedLangs,
				      QWidget *parent, const char *name,
				      bool modal )
  : KDialogBase( parent, name, modal, i18n("New Language"), Ok|Cancel, Ok, true )
{
  // layout the page (a combobox with label):
  QWidget *page = makeMainWidget();
  QHBoxLayout *hlay = new QHBoxLayout( page, 0, spacingHint() );
  mComboBox = new QComboBox( false, page );
  hlay->addWidget( new QLabel( mComboBox, i18n("Choose &language:"), page ) );
  hlay->addWidget( mComboBox, 1 );

  QStringList pathList = KGlobal::dirs()->findAllResources( "locale",
                               QString::fromLatin1("*/entry.desktop") );
  // extract a list of language tags that should not be included:
  QStringList suppressedAcronyms;
  for ( LanguageItemList::Iterator lit = suppressedLangs.begin();
	lit != suppressedLangs.end(); ++lit )
    suppressedAcronyms << (*lit).mLanguage;

  // populate the combo box:
  for ( QStringList::ConstIterator it = pathList.begin();
	it != pathList.end(); ++it )
  {
    KSimpleConfig entry( *it );
    entry.setGroup( "KCM Locale" );
    // full name:
    QString name = entry.readEntry( "Name" );
    // {2,3}-letter abbreviation:
    // we extract it from the path: "/prefix/de/entry.desktop" -> "de"
    QString acronym = (*it).section( '/', -2, -2 );

    if ( suppressedAcronyms.find( acronym ) == suppressedAcronyms.end() ) {
      // not found:
      QString displayname = QString::fromLatin1("%1 (%2)")
	.arg( name ).arg( acronym );
      QPixmap flag( locate("locale", acronym + flagPng ) );
      mComboBox->insertItem( flag, displayname );
    }
  }
  if ( !mComboBox->count() ) {
    mComboBox->insertItem( i18n("No more languages available") );
    enableButtonOK( false );
  } else mComboBox->listBox()->sort();
}

QString NewLanguageDialog::language() const
{
  QString s = mComboBox->currentText();
  int i = s.findRev( '(' );
  return s.mid( i + 1, s.length() - i - 2 );
}


LanguageComboBox::LanguageComboBox( bool rw, QWidget *parent, const char *name )
  : QComboBox( rw, parent, name )
{
}

int LanguageComboBox::insertLanguage( const QString & language )
{
  static QString entryDesktop = QString::fromLatin1("/entry.desktop");
  KSimpleConfig entry( locate("locale", language + entryDesktop) );
  entry.setGroup( "KCM Locale" );
  QString name = entry.readEntry( "Name" );
  QString output = QString::fromLatin1("%1 (%2)").arg( name ).arg( language );
  insertItem( QPixmap( locate("locale", language + flagPng ) ), output );
  return listBox()->index( listBox()->findItem(output) );
}

QString LanguageComboBox::language() const
{
  QString s = currentText();
  int i = s.findRev( '(' );
  return s.mid( i + 1, s.length() - i - 2 );
}

void LanguageComboBox::setLanguage( const QString & language )
{
  QString parenthizedLanguage = QString::fromLatin1("(%1)").arg( language );
  for (int i = 0; i < count(); i++)
    // ### FIXME: use .endWith():
    if ( text(i).find( parenthizedLanguage ) >= 0 ) {
      setCurrentItem(i);
      return;
    }
}

/********************************************************************
 *
 *     *ConfigurationPage classes
 *
 ********************************************************************/

TabbedConfigurationPage::TabbedConfigurationPage( QWidget * parent,
						  const char * name )
  : ConfigurationPage( parent, name )
{
  QVBoxLayout *vlay = new QVBoxLayout( this, 0, KDialog::spacingHint() );
  mTabWidget = new QTabWidget( this );
  vlay->addWidget( mTabWidget );
}

void TabbedConfigurationPage::addTab( QWidget * tab, const QString & title ) {
  mTabWidget->addTab( tab, title );
}



#include "configuredialog_p.moc"
