/*
    This file is part of KMail.

    Copyright (c) 2003 - 2004 Bo Thorsen <bo@klaralvdalens-datakonsult.se>
    Copyright (c) 2003 Steffen Hansen <steffen@klaralvdalens-datakonsult.se>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "startupwizard.h"

#include "kmfoldercombobox.h"
#include "configuredialog_p.h"
#include "kmacctmgr.h"
#include "kmcomposewin.h"
#include "kmfoldermgr.h"
#include "kmacctcachedimap.h"
#include "kmfoldercachedimap.h"
#include <libkpimidentities/identity.h>
#include <libkpimidentities/identitymanager.h>
#include "kmtransport.h"
#include "kmsender.h"
#include "kmgroupware.h"
#include "kmkernel.h"
#include "kmailicalifaceimpl.h"
#include "globalsettings.h"

#include <kdebug.h>
#include <klocale.h>
#include <knuminput.h>
#include <kapplication.h>
#include <kconfig.h>

#include <qvbox.h>
#include <qvbuttongroup.h>
#include <qtextbrowser.h>
#include <qwhatsthis.h>
#include <qvalidator.h>

WizardIdentityPage::WizardIdentityPage( QWidget * parent, const char * name )
  : QWidget( parent, name )
{
  // First either get the default identity or make a new one
  KPIM::IdentityManager *im = kmkernel->identityManager();
  if( im->identities().count() > 0 )
    mIdentity = im->defaultIdentity().uoid();
  else {
    mIdentity = im->newFromScratch( "Kolab Identity" ).uoid();
    im->setAsDefault( mIdentity );
  }

  KPIM::Identity & ident = im->identityForUoid( mIdentity );

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
  KPIM::Identity & ident = identity();
  ident.setFullName( nameEdit->text().stripWhiteSpace() );
  ident.setOrganization( orgEdit->text().stripWhiteSpace() );
  ident.setEmailAddr( emailEdit->text().stripWhiteSpace() );
  kmkernel->identityManager()->sort();
  kmkernel->identityManager()->commit();
}

KPIM::Identity & WizardIdentityPage::identity() const {
  return kmkernel->identityManager()->identityForUoid( mIdentity );
}

WizardKolabPage::WizardKolabPage( QWidget * parent, const char * name )
  : QWidget( parent, name ), mFolder(0), mAccount(0), mTransport( 0 )
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
    loginEdit->setText( email );
    hostEdit->setText( email.mid( at + 1 ) );
  }
}

void WizardKolabPage::apply()
{
  // Handle the account
  if( mAccount == 0 ) {
    // Create the account
    mAccount = static_cast<KMAcctCachedImap*>
      ( kmkernel->acctMgr()->create( QString("cachedimap"), "Kolab" ) );
    mAccount->init(); // fill the account fields with good default values
    kmkernel->acctMgr()->add(mAccount);

    // Set all default settings
    // TODO: read these from a system wide settings file
    mAccount->setAuth( "PLAIN" );
    mAccount->setPrefix( "/" );
    mAccount->setUseSSL( false );
    mAccount->setUseTLS( true );
    mAccount->setSieveConfig( KMail::SieveConfig( true ) );
    kmkernel->cleanupImapFolders();
    assert( mAccount->folder() );

    // This Must Be False!!
    mAccount->setAutoExpunge( false );
  }

  mAccount->setLogin( loginEdit->text().stripWhiteSpace() );
  mAccount->setPasswd( passwordEdit->text() );
  mAccount->setHost( hostEdit->text().stripWhiteSpace() );
  mAccount->setStorePasswd( storePasswordCheck->isChecked() );
  mAccount->setCheckExclude( excludeCheck->isChecked() );

  kmkernel->acctMgr()->writeConfig( false );

  // Sync new IMAP account ASAP
  kdDebug(5006) << mAccount->folder()->name() << endl;

  if( mFolder == 0 ) {
    KMFolderDir *child = mAccount->folder()->child();
    if( child == 0 )
      child = mAccount->folder()->createChildFolder();

    mFolder = kmkernel->dimapFolderMgr()->
      createFolder( "INBOX", false, KMFolderTypeCachedImap, child );
    static_cast<KMFolderCachedImap*>(mFolder)->setSilentUpload( true );
  }
  if ( !mAccount->checkingMail() ) {
    mAccount->setCheckingMail( true );
    mAccount->processNewMail( false );
  }

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
  kmkernel->msgSender()->setSendImmediate( true );
}


StartupWizard::StartupWizard( QWidget* parent, const char* name, bool modal )
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

int StartupWizard::language() const
{
  return mLanguageCombo->currentItem();
}

KMFolder* StartupWizard::folder() const
{
  if( groupwareEnabled() && useDefaultKolabSettings() )
    return mKolabWidget->folder();
  else
    return mFolderCombo->getFolder();
}

void StartupWizard::setAppropriatePages()
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

void StartupWizard::slotGroupwareEnabled( int i )
{
  mGroupwareEnabled = (i == 0);
  serverSettings->setEnabled( mGroupwareEnabled );
}

void StartupWizard::slotServerSettings( int i )
{
  mUseDefaultKolabSettings = (i == 0);
}

QWidget* StartupWizard::createIntroPage()
{
  QWidget* page = new QWidget(this, "intro_page");
  QBoxLayout* top = new QHBoxLayout( page );
  QTextBrowser* text = new QTextBrowser( page );
  text->setText( i18n("<b>You do not seem to have any groupware folders "
		    "configured in KMail.</b><br>"
		    "This is probably because you are running KMail for the first time, or "
		    "because you have enabled the groupware functionality for the first time.<br>"
		    "You now have the choice between disabling the groupware functionality, "
		    "or leaving it enabled and going through this wizard.<br>"
		    "If you disable the groupware functionality for now, you can always enable it again from "
		    "the KMail configure dialog."));
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

QWidget* StartupWizard::createIdentityPage()
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

QWidget* StartupWizard::createKolabPage()
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

QWidget* StartupWizard::createAccountPage()
{
  QWidget* page = new QWidget( this, "account_page");
  QBoxLayout* top = new QHBoxLayout( page );
  QTextBrowser* text = new QTextBrowser( page );
  text->setText(i18n("If you want, you can create new accounts before going on with"
		     " groupware configuration."));
  top->addWidget( text );

  mAccountWidget = new AccountsPage( page, "account_page" );
  mAccountWidget->setup();
  top->addWidget( mAccountWidget, 1 );
  setHelpEnabled( page, false );
  return page;
}

QWidget* StartupWizard::createLanguagePage()
{
  QWidget* page = new QWidget(this, "language_page");
  QBoxLayout* top = new QHBoxLayout( page );
  QTextBrowser* text = new QTextBrowser(  page );
  text->setText( i18n("If you want to make your groupware folders work with other "
		    "applications, you might want to select a different language "
		    "than English.<br>"
		    "If this is not an issue, leave the language as is."));
  top->addWidget( text );

  QVBox* rightSide = new QVBox( page );
  top->addWidget( rightSide, 1 );

  mLanguageLabel = new QLabel( rightSide );

  mLanguageCombo = new QComboBox( false, rightSide );

  QStringList lst;
  lst << i18n("English") << i18n("German") << i18n("French") << i18n("Dutch");
  mLanguageCombo->insertStringList( lst );

  setLanguage( 0, false );
  setHelpEnabled( page, false );
  return page;
}

QWidget* StartupWizard::createFolderSelectionPage()
{
  QWidget* page = new QWidget(this, "foldersel_page");
  QBoxLayout* top = new QHBoxLayout( page );
  QTextBrowser* text = new QTextBrowser( page );
  text->setText(i18n("The groupware functionality needs some special folders to store "
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

void StartupWizard::slotUpdateParentFolderName()
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

void StartupWizard::setLanguage( int language, bool guessed )
{
  mLanguageCombo->setCurrentItem( language );
  if( guessed ) {
    mLanguageLabel->setText( i18n("The folders present indicates that you want to use the selected folder language"));
  } else {
    mLanguageLabel->setText( i18n("The folder language cannot be guessed, please select a language:"));
  }
}

QWidget* StartupWizard::createFolderCreationPage()
{
  QHBox* page = new QHBox(this, "foldercre_page");
  mFolderCreationText = new QTextBrowser( page );
  slotUpdateParentFolderName();
  setFinishEnabled( page, true );
  setNextEnabled( page, false);
  setHelpEnabled( page, false );
  return page;
}

QWidget* StartupWizard::createOutroPage()
{
  QHBox* page = new QHBox(this, "outtro_page");
  QTextBrowser* text = new QTextBrowser( page );
  text->setText( i18n("The groupware functionality has been disabled.") );
  setFinishEnabled( page, true );
  setNextEnabled( page, false);
  setHelpEnabled( page, false );
  return page;
}

void StartupWizard::back()
{
  QWizard::back();
}

void StartupWizard::next()
{
  if( currentPage() == mAccountPage ) {
    kdDebug(5006) << "AccountPage appropriate: " << appropriate(mAccountPage) << endl;
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
    mFolderCreationText->setText( i18n("You have chosen to use standard kolab settings.\nPress Finish to proceed.") );

  } else if( currentPage() == mIdentityPage ) {
    mIdentityWidget->apply();
    mKolabWidget->init( userIdentity().emailAddr() );
  }

  // Set which ones apply, given the present state of answers
  setAppropriatePages();

  QWizard::next();
}

static bool checkSubfolders( KMFolderDir* dir, int language )
{
  KMailICalIfaceImpl& ical = kmkernel->iCalIface();
  return dir->hasNamedFolder( ical.folderName( KFolderTreeItem::Inbox, language ) ) &&
    dir->hasNamedFolder( ical.folderName( KFolderTreeItem::Calendar, language ) ) &&
    dir->hasNamedFolder( ical.folderName( KFolderTreeItem::Contacts, language ) ) &&
    dir->hasNamedFolder( ical.folderName( KFolderTreeItem::Notes, language ) ) &&
    dir->hasNamedFolder( ical.folderName( KFolderTreeItem::Tasks, language ) );
}

void StartupWizard::guessExistingFolderLanguage()
{
  KMFolderDir* dir = folder()->child();

  if(  checkSubfolders( dir, 0 ) ) {
    // Check English
    setLanguage( 0, true );
  } else if( checkSubfolders( dir, 1 ) ) {
    // Check German
    setLanguage( 1, true );
  } else {
    setLanguage( 0, false );
  }
}

KPIM::Identity &StartupWizard::userIdentity()
{
  return mIdentityWidget->identity();
}

const KPIM::Identity &StartupWizard::userIdentity() const
{
  return mIdentityWidget->identity();
}

QString StartupWizard::name() const
{
  return userIdentity().fullName();
}

QString StartupWizard::login() const
{
  return mKolabWidget->loginEdit->text().stripWhiteSpace();
}

QString StartupWizard::host() const
{
  return mKolabWidget->hostEdit->text().stripWhiteSpace();
}

QString StartupWizard::email() const
{
  return userIdentity().emailAddr();
}

QString StartupWizard::passwd() const
{
  return KMAccount::encryptStr( mKolabWidget->passwordEdit->text() );
}

bool StartupWizard::storePasswd() const
{
  return mKolabWidget->storePasswordCheck->isChecked();
}


void StartupWizard::run()
{
  /* 
   *
   * FIXME the below is no longer up to date. If you, dear reader are here
   * because you want to fix it, know this:
   *
   * 1 ) I applaud your efforts : )
   * 2 ) please look in kmail.kcfg and use KConfigXT
   * 
   */
  KConfigGroup options( KMKernel::config(), "Groupware" );

  // Check if this wizard was previously run
  if( options.readEntry( "Enabled", "notset" ) != "notset" )
    return;

  StartupWizard wiz(0, "groupware wizard", TRUE );
  int rc = wiz.exec();

  options.writeEntry( "Enabled", rc == QDialog::Accepted && wiz.groupwareEnabled() );
  if( rc == QDialog::Accepted ) {
    options.writeEntry( "FolderLanguage", wiz.language() );
    options.writeEntry( "GroupwareFolder", wiz.folder()->idString() );

    kmkernel->groupware().readConfig();

    if( wiz.groupwareEnabled() && wiz.useDefaultKolabSettings() ) {
      // Write the apps configs
      writeKOrganizerConfig( wiz );
      writeKAbcConfig();
      writeKAddressbookConfig( wiz );
    }
  }
}


// Write the KOrganizer settings
void StartupWizard::writeKOrganizerConfig( const StartupWizard& wiz ) {
  KConfig config( "korganizerrc" );

  KConfigGroup optionsKOrgGeneral( &config, "Personal Settings" );
  optionsKOrgGeneral.writeEntry( "user_name", wiz.name() );
  optionsKOrgGeneral.writeEntry( "user_email", wiz.email() );

  KConfigGroup optionsKOrgGroupware( &config, "Groupware" );
  optionsKOrgGroupware.writeEntry( "Publish FreeBusy lists", true );
  optionsKOrgGroupware.writeEntry( "Publish FreeBusy days", 60 );
  optionsKOrgGroupware.writeEntry( "Publish to Kolab server", true );
  optionsKOrgGroupware.writeEntry( "Publish to Kolab server name", wiz.host() );
  optionsKOrgGroupware.writeEntry( "Publish user name", wiz.login() );
  optionsKOrgGroupware.writeEntry( "Remember publish password", wiz.storePasswd() );
  if( wiz.storePasswd() ) {
    optionsKOrgGroupware.writeEntry( "Publish Server Password", wiz.passwd() );
    optionsKOrgGroupware.writeEntry( "Retrieve Server Password", wiz.passwd() );
  }
  optionsKOrgGroupware.writeEntry( "Retrieve FreeBusy lists", true );
  optionsKOrgGroupware.writeEntry( "Retrieve from Kolab server", true );
  optionsKOrgGroupware.writeEntry( "Retrieve from Kolab server name", wiz.host() );
  optionsKOrgGroupware.writeEntry( "Retrieve user name", wiz.login() );
  optionsKOrgGroupware.writeEntry( "Remember retrieve password", wiz.storePasswd() );

  config.sync();
}


// Write the KABC settings
void StartupWizard::writeKAbcConfig() {
  KConfig config( "kabcrc" );
  KConfigGroup optionsKAbcGeneral( &config, "General" );
  QString standardKey = optionsKAbcGeneral.readEntry( "Standard" );
  QString newStandardKey;

  QStringList activeKeys = optionsKAbcGeneral.readListEntry( "ResourceKeys" );
  QStringList passiveKeys = optionsKAbcGeneral.readListEntry( "PassiveResourceKeys" );
  QStringList keys = activeKeys + passiveKeys;
  for ( QStringList::Iterator it = keys.begin(); it != keys.end(); ++it ) {
    KConfigGroup entry( &config, "Resource_" + (*it) );
    if( entry.readEntry( "ResourceType" ) == "imap" && newStandardKey.isNull() ) {
      // This is the IMAP resource that must now be the standard
      newStandardKey = *it;

      // We want to be able to write to this
      entry.writeEntry( "ResourceIsReadOnly", false );
    } else
      // Not an IMAP resource, so don't write to it anymore
      entry.writeEntry( "ResourceIsReadOnly", true );
  }

  if( newStandardKey.isNull() ) {
    // No IMAP resource was found, make one
    newStandardKey = KApplication::randomString( 10 );
    KConfigGroup entry( &config, "Resource_" + newStandardKey );
    entry.writeEntry( "ResourceName", "imap-resource" );
    entry.writeEntry( "ResourceType", "imap" );
    entry.writeEntry( "ResourceIsReadOnly", false );
    entry.writeEntry( "ResourceIsFast", true );
    activeKeys += newStandardKey;
  } else if( passiveKeys.remove( newStandardKey ) > 0 )
    // This used to be passive. Make it active
    activeKeys += newStandardKey;

  // Set the keys
  optionsKAbcGeneral.writeEntry( "ResourceKeys", activeKeys );
  optionsKAbcGeneral.writeEntry( "PassiveResourceKeys", passiveKeys );
  optionsKAbcGeneral.writeEntry( "Standard", newStandardKey );

  config.sync();
}


// Write the KAddressbook settings
void StartupWizard::writeKAddressbookConfig( const StartupWizard& wiz ) {
  KConfig config( "kaddressbookrc" );
  KConfigGroup options( &config, "LDAP" );

  QString hostBase = QString( "dc=" ) + wiz.host();
  hostBase.replace( '.', ",dc=" );

  // Read all servers and try finding one that matches us
  uint count = options.readUnsignedNumEntry( "NumSelectedHosts");
  for ( uint i = 0; i < count; ++i ) {
    QString host = options.readEntry( QString( "SelectedHost%1").arg( i ) );
    int port = options.readUnsignedNumEntry( QString( "SelectedPort%1" ).arg( i ) );
    QString base = options.readEntry( QString( "SelectedBase%1" ).arg( i ) );

    if( host == wiz.host() && port == 389 && base == hostBase )
      // We found a match, and it's selected
      return;
  }

  // No match among the selected ones, try the unselected
  count = options.readUnsignedNumEntry( "NumHosts" );
  for ( uint i = 0; i < count; ++i ) {
    QString host = options.readEntry( QString( "SelectedHost%1").arg( i ) );
    int port = options.readUnsignedNumEntry( QString( "SelectedPort%1" ).arg( i ) );
    QString base = options.readEntry( QString( "SelectedBase%1" ).arg( i ) );

    if( host == wiz.host() && port == 389 && base == hostBase ) {
      // We found a match. Remove it from this list
      for( ++i; i < count; ++i ) {
	host = options.readEntry( QString( "Host%1" ).arg( i ) );
	port = options.readUnsignedNumEntry( QString( "Port%1" ).arg( i ) );
	base = options.readEntry( QString( "Base%1" ).arg( i ) );
	options.writeEntry( QString( "Host%1" ).arg( i-1 ), host );
	options.writeEntry( QString( "Port%1" ).arg( i-1 ), port );
	options.writeEntry( QString( "Base%1" ).arg( i-1 ), base );
      }

      // Now all the previous ones were overwritten, so remove the last one
      --count;
      options.deleteEntry( QString( "Host%1" ).arg( count ) );
      options.deleteEntry( QString( "Port%1" ).arg( count ) );
      options.deleteEntry( QString( "Base%1" ).arg( count ) );
      options.writeEntry( "NumHosts", count );
      break;
    }
  }

  // Now write the selected ldap server
  count = options.readUnsignedNumEntry( "NumSelectedHosts");
  options.writeEntry( QString( "SelectedHost%1" ).arg( count ), wiz.host() );
  options.writeEntry( QString( "SelectedPort%1" ).arg( count ), 389 );
  options.writeEntry( QString( "SelectedBase%1" ).arg( count ), hostBase );
  options.writeEntry( "NumSelectedHosts", count+1 );
}


#include "startupwizard.moc"
