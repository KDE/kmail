
#include <qlabel.h>
#include <qvbox.h>
#include <qhbox.h>
#include <qvbuttongroup.h>
#include <qradiobutton.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qtextbrowser.h>
#include <qlayout.h>
#include <qwhatsthis.h>
#include <qregexp.h>
#include <qvalidator.h>
#include <qcheckbox.h>

#include <kdebug.h>
#include <klocale.h>
#include <knuminput.h>
#include <kapplication.h>

#include "kmfoldercombobox.h"
#include "kmfolderdir.h"
#include "kmfolder.h"
#include "kmmessage.h"
#include "configuredialog.h"
#include "configuredialog_p.h"
#include "kmacctmgr.h"
#include "kmfoldermgr.h"
#include "kmacctcachedimap.h"
#include "kmfoldercachedimap.h"
#include "kmidentity.h"
#include "identitymanager.h"
#include "kmtransport.h"
#include "kmsender.h"
#include "kmmainwin.h"
#include "kmfoldertree.h"
#include "kmgroupware.h"

#include "kmgroupwarewizard.h"


WizardIdentityPage::WizardIdentityPage( QWidget * parent, const char * name )
  : QWidget( parent, name )
{
  // First either get the default identity or make a new one
  IdentityManager *im = kernel->identityManager();
  if( im->identities().count() > 0 )
    mIdentity = im->defaultIdentity().uoid();
  else {
    mIdentity = im->newFromScratch( "Kolab Identity" ).uoid();
    im->setAsDefault( mIdentity );
  }

  KMIdentity & ident = im->identityForUoid( mIdentity );

  QGridLayout *grid = new QGridLayout( this, 3, 2, KDialog::marginHint(),
				       KDialog::spacingHint() );
  grid->addColSpacing( 1, fontMetrics().maxWidth()*15 );
  grid->setRowStretch( 15, 10 );
  grid->setColStretch( 1, 10 );

  QLabel *label = new QLabel( i18n("&Your name:"), this );
  QWhatsThis::add( label, i18n("Write your name here.") );
  grid->addWidget( label, 0, 0 );
  nameEdit = new QLineEdit( ident.fullName(), this );
  nameEdit->setFocus();
  label->setBuddy( nameEdit );
  grid->addWidget( nameEdit, 0, 1 );

  label = new QLabel( i18n("Organi&zation:"), this );
  QWhatsThis::add( label, i18n("You can write the company or organization you work for.") );
  grid->addWidget( label, 1, 0 );
  orgEdit = new QLineEdit( ident.organization(), this );
  label->setBuddy( orgEdit );
  grid->addWidget( orgEdit, 1, 1 );

  label = new QLabel( i18n("&Email address:"), this );
  grid->addWidget( label, 2, 0 );
  emailEdit = new QLineEdit( ident.emailAddr(), this );
  label->setBuddy( emailEdit );
  grid->addWidget( emailEdit, 2, 1 );
}

void WizardIdentityPage::apply() const {
  // Save the identity settings
  KMIdentity & ident = identity();
  ident.setFullName( nameEdit->text().stripWhiteSpace() );
  ident.setOrganization( orgEdit->text().stripWhiteSpace() );
  ident.setEmailAddr( emailEdit->text().stripWhiteSpace() );

  kdDebug() << "Writing identity: " << ident.identityName() << ", full name: "
	    << ident.fullName() << ", uoid: " << ident.uoid() << ", org:"
	    << ident.organization() << ", email: " << ident.emailAddr() << endl;

  kdDebug() << ", Identitylist: " << kernel->identityManager()->identities().count()
	    << ", " << kernel->identityManager()->identities()[0] << endl;

  kernel->identityManager()->sort();
  kernel->identityManager()->commit();
}

KMIdentity & WizardIdentityPage::identity() const {
  return kernel->identityManager()->identityForUoid( mIdentity );
}

WizardKolabPage::WizardKolabPage( QWidget * parent, const char * name )
  : QWidget( parent, name ), mFolder(0), mAccount(0)
{
  QGridLayout *grid = new QGridLayout( this, 7, 2, KDialog::marginHint(),
				       KDialog::spacingHint() );
  grid->addColSpacing( 1, fontMetrics().maxWidth()*15 );
  grid->setRowStretch( 15, 10 );
  grid->setColStretch( 1, 10 );

  QLabel *label = new QLabel( i18n("&Login:"), this );
  QWhatsThis::add( label, i18n("Your Internet Service Provider gave you a <em>user name</em> which is used to authenticate you with their servers. It usually is the first part of your email address (the part before <em>@</em>).") );
  grid->addWidget( label, 0, 0 );
  loginEdit = new QLineEdit( this );
  label->setBuddy( loginEdit );
  grid->addWidget( loginEdit, 0, 1 );

  label = new QLabel( i18n("P&assword:"), this );
  grid->addWidget( label, 1, 0 );
  passwordEdit = new QLineEdit( this );
  passwordEdit->setEchoMode( QLineEdit::Password );
  label->setBuddy( passwordEdit );
  grid->addWidget( passwordEdit, 1, 1 );

  label = new QLabel( i18n("Ho&st:"), this );
  grid->addWidget( label, 2, 0 );
  hostEdit = new QLineEdit( this );
  // only letters, digits, '-', '.', ':' (IPv6) and '_' (for Windows
  // compatibility) are allowed
  hostEdit->setValidator(new QRegExpValidator( QRegExp( "[A-Za-z0-9-_:.]*" ), 0 ) );
  label->setBuddy( hostEdit );
  grid->addWidget( hostEdit, 2, 1 );

  storePasswordCheck =
    new QCheckBox( i18n("Sto&re IMAP password in configuration file"), this );
  storePasswordCheck->setChecked( true );
  grid->addMultiCellWidget( storePasswordCheck, 3, 3, 0, 1 );

  excludeCheck = new QCheckBox( i18n("E&xclude from \"Check Mail\""), this );
  grid->addMultiCellWidget( excludeCheck, 4, 4, 0, 1 );

  intervalCheck = new QCheckBox( i18n("Enable &interval mail checking"), this );
  intervalCheck->setChecked( true );
  grid->addMultiCellWidget( intervalCheck, 5, 5, 0, 2 );
  intervalLabel = new QLabel( i18n("Check inter&val:"), this );
  grid->addWidget( intervalLabel, 6, 0 );
  intervalSpin = new KIntNumInput( this );
  intervalSpin->setRange( 1, 60, 1, FALSE );
  intervalSpin->setValue( 1 );
  intervalSpin->setSuffix( i18n( " min" ) );
  intervalLabel->setBuddy( intervalSpin );
  connect( intervalCheck, SIGNAL(toggled(bool)), intervalSpin, SLOT(setEnabled(bool)) );
  grid->addWidget( intervalSpin, 6, 1 );
}

void WizardKolabPage::init( const QString &email )
{
  static bool first = true;
  if( !first ) return;
  first = false;

  // Workaround since Qt can't have default focus on more than one page
  loginEdit->setFocus();

  int at = email.find('@');
  if( at > 1 && email.length() > (uint)at ) {
    // Set reasonable login and host defaults
    loginEdit->setText( email.left( at ) );
    hostEdit->setText( email.mid( at + 1 ) );
  }
}

void WizardKolabPage::apply()
{
  // Handle the account
  if( mAccount == 0 ) {
    // Create the account
    mAccount = static_cast<KMAcctCachedImap*>
      ( kernel->acctMgr()->create( QString("cachedimap"), "Kolab" ) );
    mAccount->init(); // fill the account fields with good default values
    kernel->acctMgr()->add(mAccount);

    // Set all default settings
    // TODO: read these from a system wide settings file
    mAccount->setAuth( "PLAIN" );
    mAccount->setPrefix( "/" );
    mAccount->setUseSSL( false );
    mAccount->setUseTLS( true );
    mAccount->setSieveConfig( KMail::SieveConfig( true ) );
    kernel->cleanupImapFolders();
    assert( mAccount->folder() );

    // This Must Be False!!
    mAccount->setAutoExpunge( false );
  }

  mAccount->setLogin( loginEdit->text().stripWhiteSpace() );
  mAccount->setPasswd( passwordEdit->text() );
  mAccount->setHost( hostEdit->text().stripWhiteSpace() );
  mAccount->setStorePasswd( storePasswordCheck->isChecked() );
  mAccount->setCheckExclude( excludeCheck->isChecked() );

  kernel->acctMgr()->writeConfig( false );

  // Sync new IMAP account ASAP
  kdDebug() << mAccount->folder()->name() << endl;

  if( mFolder == 0 ) {
    KMFolderDir *child = mAccount->folder()->child();
    if( child == 0 )
      child = mAccount->folder()->createChildFolder();

    mFolder = kernel->imapFolderMgr()->
      createFolder( "INBOX", true, KMFolderTypeCachedImap, child );
    static_cast<KMFolderCachedImap*>(mFolder)->setSilentUpload( true );
  }
  mAccount->processNewMail(false);

  // Handle SMTP transport
  if( mTransport == 0 ) {
    mTransport = new KMTransportInfo();
    mTransport->type = "smtp";
    mTransport->name = "Kolab";
    mTransport->port = "25";
    mTransport->encryption = "TLS";
    mTransport->authType = "PLAIN";
    mTransport->auth = true;
    mTransport->precommand = "";
  }

  mTransport->host = hostEdit->text().stripWhiteSpace();
  mTransport->user = loginEdit->text().stripWhiteSpace();
  mTransport->pass = passwordEdit->text();
  mTransport->storePass = storePasswordCheck->isChecked();

  // Save transports:
  KConfigGroup general( KMKernel::config(), "General" );
  KConfigGroup composer( KMKernel::config(), "Composer" );
  // TODO: Set more transports
  general.writeEntry( "transports", 1/*mTransportInfoList.count()*/ );
//   QPtrListIterator<KMTransportInfo> it( mTransportInfoList );
//   for ( int i = 1 ; it.current() ; ++it, ++i )
//     (*it)->writeConfig(i);
  mTransport->writeConfig(1);

  // Save common options:
  general.writeEntry( "sendOnCheck", false );
  kernel->msgSender()->setSendImmediate( true );

  //
  // Make other components read the new settings
  //
  KMMessage::readConfig();
  // kernel->kbp()->busy(); // this can take some time when a large folder is open
  if ( KMainWindow::memberList ) {
    QPtrListIterator<KMainWindow> it( *KMainWindow::memberList );
    for ( it.toFirst() ; it.current() ; ++it )
      // ### FIXME: use dynamic_cast.
      if(      (*it)->isA("KMMainWin") )
	((KMMainWin*)(*it))->readConfig();
  }
  // kernel->kbp()->idle();
}


KMGroupwareWizard::KMGroupwareWizard( QWidget* parent, const char* name, bool modal )
  : QWizard( parent, name, modal ), mGroupwareEnabled(true)
{
  addPage( mIntroPage = createIntroPage(), i18n("Groupware Functionality for KMail") );
  addPage( mIdentityPage = createIdentityPage(), i18n("Your Identity") );
  addPage( mKolabPage = createKolabPage(), i18n("Kolab Groupware Settings") );
  addPage( mAccountPage = createAccountPage(), i18n("Accounts") );
  addPage( mFolderSelectionPage = createFolderSelectionPage(), i18n("Folder Selection") );
  addPage( mLanguagePage = createLanguagePage(), i18n("Folder Language") );
  addPage( mFolderCreationPage = createFolderCreationPage(), i18n("Folder Creation") );
  addPage( mOutroPage = createOutroPage(), i18n("Done") );
}

int KMGroupwareWizard::language() const
{
  return mLanguageCombo->currentItem();
}

KMFolder* KMGroupwareWizard::folder() const
{
  if( groupwareEnabled() && useDefaultKolabSettings() )
    return mKolabWidget->folder();
  else
    return mFolderCombo->getFolder();
}

void KMGroupwareWizard::setAppropriatePages()
{
  setAppropriate( mKolabPage, groupwareEnabled() && useDefaultKolabSettings() );
  setAppropriate( mAccountPage, !groupwareEnabled() || !useDefaultKolabSettings() );
  setAppropriate( mLanguagePage, groupwareEnabled() );
  setAppropriate( mFolderSelectionPage, groupwareEnabled() && !useDefaultKolabSettings() );
  setAppropriate( mFolderCreationPage, groupwareEnabled() );
  setAppropriate( mOutroPage, !groupwareEnabled() );
  setNextEnabled( mOutroPage, false);
  setFinishEnabled( mOutroPage, true );
  setFinishEnabled( mFolderCreationPage, true );
}

void KMGroupwareWizard::slotGroupwareEnabled( int i )
{
  mGroupwareEnabled = (i == 0);
  serverSettings->setEnabled( mGroupwareEnabled );
}

void KMGroupwareWizard::slotServerSettings( int i )
{
  mUseDefaultKolabSettings = (i == 0);
}

QWidget* KMGroupwareWizard::createIntroPage()
{
  QWidget* page = new QWidget(this, "intro_page");
  QBoxLayout* top = new QHBoxLayout( page );
  QTextBrowser* text = new QTextBrowser( page );
  text->setText( i18n("<b>You dont seem to have any groupware folders "
		    "configured in KMail.</b><br>"
		    "This is probably because you run KMail for the first time, or "
		    "because you enabled the groupware functions for the first time.<br>"
		    "You now have the choice between disabling the groupware functions, "
		    "or leaving them enabled and go through this wizard.<br>"
		    "If you disable it for now, you can always enable it again from "
		    "the KMail config dialog."));
  top->addWidget( text );

  QVBox* rightSide = new QVBox( page );
  top->addWidget( rightSide, 1 );

  QButtonGroup* bg = new QVButtonGroup( i18n("Groupware Functions"), rightSide );

  (new QRadioButton( i18n("Enable groupware functions"), bg ))->setChecked( TRUE );
  (void)new QRadioButton( i18n("Disable groupware functions"), bg );
  connect( bg, SIGNAL( clicked(int) ), this, SLOT( slotGroupwareEnabled(int) ) );

  bg = serverSettings = new QVButtonGroup( i18n("Groupware Server Setup"), rightSide );
  (new QRadioButton( i18n("Use standard groupware server settings"), bg ))->setChecked(TRUE);
  (void)new QRadioButton( i18n("Advanced server setup"), bg );
  connect( bg, SIGNAL( clicked(int) ), this, SLOT(slotServerSettings(int) ) );

  // Set the groupware setup to the right settings
  slotGroupwareEnabled( 0 );
  slotServerSettings( 0 );
  setHelpEnabled( page, false );
  setBackEnabled( page, false );
  return page;
}

QWidget* KMGroupwareWizard::createIdentityPage()
{
  QWidget* page = new QWidget( this, "identity_page" );
  QBoxLayout* top = new QHBoxLayout( page );
  QTextBrowser* text = new QTextBrowser( page );
  text->setText( i18n("Please set at least your name and email address.") );
  top->addWidget( text );

  mIdentityWidget = new WizardIdentityPage( page, "identity_page" );
  top->addWidget( mIdentityWidget, 1 );
  setHelpEnabled( page, false );
  return page;
}

QWidget* KMGroupwareWizard::createKolabPage()
{
  QWidget* page = new QWidget( this, "kolabserver_page" );
  QBoxLayout* top = new QHBoxLayout( page );
  QTextBrowser* text = new QTextBrowser( page );
  text->setText( i18n("If the groupware server is a kolab server with default"
		      " settings, you only need to set these settings.") );
  top->addWidget( text );

  mKolabWidget = new WizardKolabPage( page, "kolabserver_page" );
  top->addWidget( mKolabWidget, 1 );
  setHelpEnabled( page, false );
  return page;
}

QWidget* KMGroupwareWizard::createAccountPage()
{
  QWidget* page = new QWidget( this, "account_page");
  QBoxLayout* top = new QHBoxLayout( page );
  QTextBrowser* text = new QTextBrowser( page );
  text->setText(i18n("If you want, you can create new accounts before going on with"
		     " groupware configuration."));
  top->addWidget( text );

  mAccountWidget = new NetworkPage( page, "account_page" );
  mAccountWidget->setup();
  top->addWidget( mAccountWidget, 1 );
  setHelpEnabled( page, false );
  return page;
}

QWidget* KMGroupwareWizard::createLanguagePage()
{
  QWidget* page = new QWidget(this, "language_page");
  QBoxLayout* top = new QHBoxLayout( page );
  QTextBrowser* text = new QTextBrowser(  page );
  text->setText( i18n("If you want to make your groupware folders work with other "
		    "applications, you might want to select a different language "
		    "than english.<br>"
		    "If this is not an issue, leave the language as it is."));
  top->addWidget( text );

  QVBox* rightSide = new QVBox( page );
  top->addWidget( rightSide, 1 );  

  mLanguageLabel = new QLabel( rightSide );

  mLanguageCombo = new QComboBox( false, rightSide );

  QStringList lst;
  lst << i18n("English") << i18n("German");
  mLanguageCombo->insertStringList( lst );

  setLanguage( 0, false );
  setHelpEnabled( page, false );
  return page;
}

QWidget* KMGroupwareWizard::createFolderSelectionPage()
{
  QWidget* page = new QWidget(this, "foldersel_page");
  QBoxLayout* top = new QHBoxLayout( page );
  QTextBrowser* text = new QTextBrowser( page );
  text->setText(i18n("The groupware functions need some special folders to store " 
		   "the contents of the calendar, contacts, tasks etc.<br>"
		   "Please select the folder that the groupware folders should "
		   "be subfolders of."));
  top->addWidget( text );
  mFolderCombo = new KMFolderComboBox( page );
  top->addWidget( mFolderCombo, 1 );
  connect( mFolderCombo, SIGNAL( activated(int) ),
	   this, SLOT( slotUpdateParentFolderName() ) );
  setHelpEnabled( page, false );
  return page;
}

void KMGroupwareWizard::slotUpdateParentFolderName()
{
  KMFolder* folder = mFolderCombo->getFolder();
  QString fldrName = i18n("<unnamed>");
  if( folder ) fldrName = folder->name();
  mFolderCreationText
    ->setText( i18n("You have chosen the folder <b>%1</b> as parent of the "
		    "groupware folders. When pressing the Finish button, "
		    "those folders will be created if "
		    "they do not already exist").arg( fldrName ));
}

void KMGroupwareWizard::setLanguage( int language, bool guessed ) 
{
  mLanguageCombo->setCurrentItem( language );
  if( guessed ) {
    mLanguageLabel->setText( i18n("The folders present indicates that you want to use the selected folder language"));
  } else {
    mLanguageLabel->setText( i18n("The folder language can't be guessed, please select a language:"));
  }
}

QWidget* KMGroupwareWizard::createFolderCreationPage()
{
  QHBox* page = new QHBox(this, "foldercre_page");
  mFolderCreationText = new QTextBrowser( page );
  slotUpdateParentFolderName();
  setFinishEnabled( page, true );
  setNextEnabled( page, false);
  setHelpEnabled( page, false );
  return page;
}

QWidget* KMGroupwareWizard::createOutroPage()
{
  QHBox* page = new QHBox(this, "outtro_page");
  QTextBrowser* text = new QTextBrowser( page );
  text->setText( i18n("The groupware functionality has been disabled.") );
  setFinishEnabled( page, true );
  setNextEnabled( page, false);
  setHelpEnabled( page, false );
  return page;
}

void KMGroupwareWizard::back()
{
  QWizard::back();
}

void KMGroupwareWizard::next()
{
  if( currentPage() == mAccountPage ) {
    kdDebug() << "AccountPage appropriate: " << appropriate(mAccountPage) << endl;
    mAccountWidget->apply();
  } else if( currentPage() == mFolderSelectionPage ) {
    /* Find the list of folders and guess the language */
    guessExistingFolderLanguage();
  } else if( currentPage() == mKolabPage ) {
    // Apply all settings to the account
    mKolabWidget->apply();
    /* Find the list of folders and guess the language */
    // TODO: guessExistingFolderLanguage();

    // Finally, just set the message at the end of the wizard
    mFolderCreationText->setText( i18n("You have chosen to use standard kolab settings") );

  } else if( currentPage() == mIdentityPage ) {
    mIdentityWidget->apply();
    mKolabWidget->init( userIdentity().emailAddr() );
  }

  // Set which ones apply, given the present state of answers
  setAppropriatePages();

  QWizard::next();
}

static bool checkSubfolders( KMFolderDir* dir, KMGroupware* gw, int language )
{
  return dir->hasNamedFolder( gw->folderName( KFolderTreeItem::Inbox, language ) ) &&
    dir->hasNamedFolder( gw->folderName( KFolderTreeItem::Calendar, language ) ) &&
    dir->hasNamedFolder( gw->folderName( KFolderTreeItem::Contacts, language ) ) &&
    dir->hasNamedFolder( gw->folderName( KFolderTreeItem::Notes, language ) ) &&
    dir->hasNamedFolder( gw->folderName( KFolderTreeItem::Tasks, language ) );
}

void KMGroupwareWizard::guessExistingFolderLanguage()
{
  KMFolderDir* dir = folder()->child();
  KMGroupware* gw = &(KMKernel::self()->groupware());

  if(  checkSubfolders( dir, gw, 0 ) ) {
    // Check English
    setLanguage( 0, true );
  } else if( checkSubfolders( dir, gw, 1 ) ) {
    // Check German
    setLanguage( 1, true );
  } else {
    setLanguage( 0, false );
  }
}

KMIdentity &KMGroupwareWizard::userIdentity() {
  return mIdentityWidget->identity();
}

#include "kmgroupwarewizard.moc"
