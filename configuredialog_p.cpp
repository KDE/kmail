#define QT_NO_CAST_ASCII
#define QT_NO_COMPAT
// configuredialog_p.cpp: classes internal to ConfigureDialog
// see configuredialog.cpp for details.

// my header:
#include "configuredialog_p.h"

// other KMail headers:
#include "kmidentity.h" // for IdentityList::{export,import}Data

// other kdenetwork headers: (none)
// other KDE headers:
#include <kemailsettings.h> // for IdentityEntry::fromControlCenter()
#include <kmessagebox.h>
#include <klocale.h> // for i18n

// Qt headers:
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qlabel.h>
#include <qframe.h>
#include <qwidget.h>
#include <qlayout.h>
#include <assert.h>

// Other headers:
#include <assert.h>

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
    entry.setTransport(ident.transport());
    entry.setFcc(ident.fcc());
    entry.setDrafts(ident.drafts());

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




// FIXME: Add the help button again if there actually _is_ a
// help text for this dialog!
NewIdentityDialog::NewIdentityDialog( const QStringList & identities,
				      QWidget *parent, const char *name,
				      bool modal )
  : KDialogBase( parent, name, modal, i18n("New Identity"),
		 Ok|Cancel/*|Help*/, Ok, true ), mDuplicateMode( Empty )
{
  QFrame *page = makeMainWidget();
  QVBoxLayout *vlay = new QVBoxLayout( page, marginHint(), spacingHint() );
  QGridLayout *glay = new QGridLayout( vlay, 5, 2 );
  vlay->addStretch( 1 );

  // row 0: line edit with label
  mLineEdit = new QLineEdit( page );
  mLineEdit->setFocus();
  glay->addWidget( new QLabel( mLineEdit, i18n("&New Identity:"), page ), 0, 0 );
  glay->addWidget( mLineEdit, 0, 1 );

  QButtonGroup *buttonGroup = new QButtonGroup( page );
  buttonGroup->hide();
  connect( buttonGroup, SIGNAL(clicked(int)),
	   this, SLOT(slotRadioClicked(int)) ); // keep track of DuplicateMode

  // row 1: radio button
  QRadioButton *radio = new QRadioButton( i18n("&With empty fields"), page );
  radio->setChecked( true );
  buttonGroup->insert( radio, Empty );
  glay->addMultiCellWidget( radio, 1, 1, 0, 1 );

  // row 2: radio button
  radio = new QRadioButton( i18n("&Use Control Center settings"), page );
  buttonGroup->insert( radio, ControlCenter );
  glay->addMultiCellWidget( radio, 2, 2, 0, 1 );

  // row 3: radio button
  radio = new QRadioButton( i18n("&Duplicate existing identity"), page );
  buttonGroup->insert( radio, ExistingEntry );
  glay->addMultiCellWidget( radio, 3, 3, 0, 1 );

  // row 4: combobox with existing identities and label
  mComboBox = new QComboBox( false, page );
  mComboBox->insertStringList( identities );
  mComboBox->setEnabled( false );
  QLabel *label = new QLabel( mComboBox, i18n("&Existing identities:"), page );
  label->setEnabled( false );
  glay->addWidget( label, 4, 0 );
  glay->addWidget( mComboBox, 4, 1 );
  // enable/disable combobox and label depending on the third radio
  // button's state:
  connect( radio, SIGNAL(toggled(bool)),
	   label, SLOT(setEnabled(bool)) );
  connect( radio, SIGNAL(toggled(bool)),
	   mComboBox, SLOT(setEnabled(bool)) );
}


void NewIdentityDialog::slotOk( void )
{
  QString identity = identityName().stripWhiteSpace();

  if( identity.isEmpty() ) {
    KMessageBox::error( this, i18n("You must specify an identity name!") );
    return; // don't leave the dialog
  }

  for( int i=0 ; i < mComboBox->count() ; i++ )
    if( identity == mComboBox->text( i ) ) {
      QString message = i18n("An identity named \"%1\" already exists!\n"
			     "Please choose another name.").arg(identity);
      KMessageBox::error( this, message );
      return; // don't leave the dialog
    }

  accept();
}


void NewIdentityDialog::slotRadioClicked( int which )
{
  assert( which == (int)Empty ||
	  which == (int)ControlCenter ||
	  which == (int)ExistingEntry );
  mDuplicateMode = static_cast<DuplicateMode>( which );
}


#include "configuredialog_p.moc"
