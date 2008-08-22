/******************************************************************************
**
** Filename   : accountwizard.cpp
** Created on : 07 February, 2005
** Copyright  : (c) 2005 Tobias Koenig
** Email      : tokoe@kde.org
**
******************************************************************************/

/******************************************************************************
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
**   In addition, as a special exception, the copyright holders give
**   permission to link the code of this program with any edition of
**   the Qt library by Trolltech AS, Norway (or with modified versions
**   of Qt that use the same license as Qt), and distribute linked
**   combinations including the two.  You must obey the GNU General
**   Public License in all respects for all of the code used other than
**   Qt.  If you modify this file, you may extend this exception to
**   your version of the file, but you are not obligated to do so.  If
**   you do not wish to do so, delete this exception statement from
**   your version.
******************************************************************************/

#include "accountwizard.h"

#include "kmacctlocal.h"
#include "kmkernel.h"
#include "popaccount.h"
#include "kmacctimap.h"
#include "kmacctcachedimap.h"
#include "kmacctmaildir.h"
#include "accountmanager.h"
using KMail::AccountManager;

#include "globalsettings.h"
#include "kpimidentities/identity.h"
#include "kpimidentities/identitymanager.h"
#include "protocols.h"

#include <mailtransport/transport.h>
#include <mailtransport/transportmanager.h>
#include <mailtransport/servertest.h>
using namespace MailTransport;

#include "identitylistview.h"
using KMail::IdentityListView;
using KMail::IdentityListViewItem;

#include <kdialog.h>
#include <kfiledialog.h>
#include <klineedit.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kconfiggroup.h>
#include <kvbox.h>
#include <kmaccount.h>

#include <QCheckBox>
#include <QDir>
#include <QLabel>
#include <QLayout>
#include <QListWidget>
#include <QPushButton>
#include <QGridLayout>
#include <QRadioButton>
#include <QButtonGroup>
#include <QTreeWidget>

class AccountTypeBox : public QListWidget
{
  public:
    enum Type {
      Local,
      POP3,
      IMAP,
      dIMAP,
      Maildir
    };

    AccountTypeBox( QWidget *parent )
      : QListWidget( parent )
    {
      mTypeList << KAccount::displayNameForType( KAccount::Local );
      mTypeList << KAccount::displayNameForType( KAccount::Pop );
      mTypeList << KAccount::displayNameForType( KAccount::Imap );
      mTypeList << KAccount::displayNameForType( KAccount::DImap );
      mTypeList << KAccount::displayNameForType( KAccount::Maildir );

      setSelectionBehavior( QAbstractItemView::SelectRows );
      addItems( mTypeList );
    }

    void setType( Type type )
    {
      setCurrentRow( static_cast<int>( type ) );
    }

    Type type() const
    {
      return static_cast<Type>( currentRow() );
    }

  private:
    QStringList mTypeList;
};

AccountWizard::AccountWizard( KMKernel *kernel, QWidget *parent )
  : KAssistantDialog( parent ), mKernel( kernel ),
    mAccount( 0 ), mTransport( 0 ), mServerTest( 0 )
{
  showButton( Help, false );

  setupWelcomePage();
  setupAccountTypePage();
  setupAccountInformationPage();
  setupLoginInformationPage();
  setupServerInformationPage();

  connect( this, SIGNAL( currentPageChanged( KPageWidgetItem *,
                                             KPageWidgetItem * ) ),
           this, SLOT( slotCurrentPageChanged( KPageWidgetItem * ) ) );
}

AccountWizard::~AccountWizard()
{
  delete mServerTest;
  mServerTest = 0;
}

void AccountWizard::start( KMKernel *kernel, QWidget *parent )
{
  KConfigGroup wizardConfig( KMKernel::config(), "AccountWizard" );

  AccountWizard wizard( kernel, parent );
  int result = wizard.exec();
  if ( result == QDialog::Accepted ) {
    wizardConfig.writeEntry( "ShowOnStartup", false );
    kernel->slotConfigChanged();
  }
}

void AccountWizard::slotCurrentPageChanged( KPageWidgetItem *current )
{
  if ( current == mWelcomePage ) {
    // do nothing
  } else if ( current == mAccountTypePage ) {
    if ( !mTypeBox->currentItem() ) {
      mTypeBox->setType( AccountTypeBox::POP3 );
    }
  } else if ( current == mLoginInformationPage ) {
    if ( mLoginName->text().isEmpty() ) {
      // try to extract login from email address
      QString email = mEMailAddress->text();
      int pos = email.indexOf( '@' );
      if ( pos != -1 ) {
        mLoginName->setText( email.left( pos ) );
      }

      // take the whole email as login otherwise?!?
    }
  } else if ( current == mServerInformationPage ) {
    if ( mTypeBox->type() == AccountTypeBox::Local ||
         mTypeBox->type() == AccountTypeBox::Maildir ) {
      mIncomingServer->hide();
      mIncomingLocationWdg->show();
      mIncomingLabel->setText( i18n( "Location:" ) );

      if ( mTypeBox->type() == AccountTypeBox::Local ) {
        mIncomingLocation->setText( QDir::homePath() + "/inbox" );
      } else {
        mIncomingLocation->setText( QDir::homePath() + "/Mail/" );
      }
    } else {
      mIncomingServer->show();
      mIncomingLocationWdg->hide();
      mIncomingLabel->setText( i18n( "Incoming server:" ) );
    }
  }
}

void AccountWizard::slotIdentityStateChanged( int mode )
{
  if ( mode == Qt::Checked) {
    setAppropriate( mAccountInformationPage, true );
  } else {
    setAppropriate( mAccountInformationPage, false );
  }
}

void AccountWizard::setupWelcomePage()
{
  KVBox *box = new KVBox( this );
  box->setSpacing( KDialog::spacingHint() );

  QLabel *label = new QLabel( i18n( "Welcome to KMail's account wizard" ), box );
  QFont font = label->font();
  font.setBold( true );
  label->setFont( font );

  QLabel *message;
  if ( kmkernel->firstStart() ) {
    message = new QLabel( i18n( "<qt>It seems you have started KMail for the first time.<br>"
                      "You can use this wizard to setup your mail accounts. "
                      "Just enter the connection data that you received from your email provider "
                      "into the following pages.</qt>" ), box );
  } else {
    message = new QLabel( i18n( "<qt>You can use this wizard to setup your mail accounts.<br>"
                     "Just enter the connection data that you received from your email provider "
                      "into the following pages.</qt>" ), box );
  }
  message->setWordWrap( true );

  mCreateNewIdentity = new QCheckBox( i18n( "Create a new identity" ), box );
  QString helpText( i18n( "An identity is your email address, "
                                        "name, organization and so on.<br>"
                                        "Do not uncheck this if you don't know what "
                                        "you are doing<br>as some servers refuses to send mail "
                                        "if the sending identity<br>does not match the one belonging "
                                        "to that account.") );
  mCreateNewIdentity->setToolTip( helpText );
  mCreateNewIdentity->setWhatsThis( helpText );
  mCreateNewIdentity->setChecked( true );
  if ( onlyDefaultIdentity() ) {
    mCreateNewIdentity->setVisible( false );
  }
  connect( mCreateNewIdentity, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotIdentityStateChanged( int ) ) );

  mWelcomePage = new KPageWidgetItem( box, i18n("Account Wizard") );
  addPage( mWelcomePage );
}

void AccountWizard::setupAccountTypePage()
{
  KVBox *box = new KVBox( this );
  box->setSpacing( KDialog::spacingHint() );

  new QLabel( i18n( "Select what kind of account you would like to create" ), box );

  mTypeBox = new AccountTypeBox( box );

  mAccountTypePage = new KPageWidgetItem( box, i18n("Account Type") );
  addPage( mAccountTypePage );
}

void AccountWizard::setupAccountInformationPage()
{
  QWidget *page = new QWidget( this );
  QGridLayout *layout = new QGridLayout( page );
  layout->setSpacing( KDialog::spacingHint() );
  layout->setMargin( KDialog::marginHint() );

  QLabel *label = new QLabel( i18n( "Real name:" ), page );
  mRealName = new KLineEdit( page );
  label->setBuddy( mRealName );

  layout->addWidget( label, 0, 0 );
  layout->addWidget( mRealName, 0, 1 );

  label = new QLabel( i18n( "E-mail address:" ), page );
  mEMailAddress = new KLineEdit( page );
  label->setBuddy( mEMailAddress );

  layout->addWidget( label, 1, 0 );
  layout->addWidget( mEMailAddress, 1, 1 );

  label = new QLabel( i18n( "Organization:" ), page );
  mOrganization = new KLineEdit( page );
  label->setBuddy( mOrganization );

  layout->addWidget( label, 2, 0 );
  layout->addWidget( mOrganization, 2, 1 );

  mAccountInformationPage= new KPageWidgetItem( page, i18n("Account Information") );
  addPage( mAccountInformationPage );
}

void AccountWizard::setupLoginInformationPage()
{
  QWidget *page = new QWidget( this );
  QGridLayout *layout = new QGridLayout( page );
  layout->setSpacing( KDialog::spacingHint() );
  layout->setMargin( KDialog::marginHint() );

  QLabel *label = new QLabel( i18n( "Login name:" ), page );
  mLoginName = new KLineEdit( page );
  label->setBuddy( mLoginName );

  layout->addWidget( label, 0, 0 );
  layout->addWidget( mLoginName, 0, 1 );

  label = new QLabel( i18n( "Password:" ), page );
  mPassword = new KLineEdit( page );
  mPassword->setEchoMode( QLineEdit::Password );
  label->setBuddy( mPassword );

  layout->addWidget( label, 1, 0 );
  layout->addWidget( mPassword, 1, 1 );

  mLoginInformationPage = new KPageWidgetItem( page, i18n("Login Information") );
  addPage( mLoginInformationPage );
}

void AccountWizard::setupServerInformationPage()
{
  QWidget *page = new QWidget( this );
  QGridLayout *layout = new QGridLayout( page );
  layout->setSpacing( KDialog::spacingHint() );
  layout->setMargin( KDialog::marginHint() );

  // Incoming server
  mIncomingLabel = new QLabel( page );
  /* mIncomingLabel text is set in slotCurrentPageChanged() */
  mIncomingServer = new KLineEdit( page );
  mIncomingLabel->setBuddy( mIncomingServer );

  layout->addWidget( mIncomingLabel, 0, 0 );
  layout->addWidget( mIncomingServer, 0, 1 );

  mIncomingLocationWdg = new KHBox( page );
  mIncomingLocation = new KLineEdit( mIncomingLocationWdg );
  layout->addWidget( mIncomingLocationWdg, 0, 1 );

  mChooseLocation = new QPushButton( i18n( "Choose..." ), mIncomingLocationWdg );
  connect( mChooseLocation, SIGNAL( clicked() ),
           this, SLOT( chooseLocation() ) );

  // Outgoing server
  QLabel *label = new QLabel( i18n( "Outgoing server:" ), page );
  mOutgoingServer = new KLineEdit( page );
  label->setBuddy( mOutgoingServer );

  layout->addWidget( label, 2, 0 );
  layout->addWidget( mOutgoingServer, 2, 1 );

  // Local delivery
  mLocalDelivery = new QCheckBox( i18n( "Use local delivery" ), page );
  mLocalDelivery->setWhatsThis( i18n( "If your local host acts as a sending mail server (SMTP), you may activate this." ) );
  layout->addWidget( mLocalDelivery, 4, 0 );

  connect( mLocalDelivery, SIGNAL( toggled( bool ) ),
           mOutgoingServer, SLOT( setDisabled( bool ) ) );

  mServerInformationPage = new KPageWidgetItem( page, i18n("Server Information") );
  addPage( mServerInformationPage );
}

void AccountWizard::chooseLocation()
{
  QString location;

  if ( mTypeBox->type() == AccountTypeBox::Local ) {
    location = KFileDialog::getSaveFileName( QString(), QString(), this );
  } else if ( mTypeBox->type() == AccountTypeBox::Maildir ) {
    location = KFileDialog::getExistingDirectory( QString(), this );
  }

  if ( !location.isEmpty() ) {
    mIncomingLocation->setText( location );
  }
}

void AccountWizard::clearAccountInfo()
{
  mRealName->clear();
  mEMailAddress->clear();
  mOrganization->clear();
}

QString AccountWizard::identityName() const
{
  // create identity name
  QString name( i18nc( "Default name for new email accounts/identities.", "Unnamed" ) );

  QString idName = mEMailAddress->text();
  int pos = idName.indexOf( '@' );
  if ( pos != -1 ) {
    name = idName.mid( 0, pos );
  }

  // Make the name a bit more human friendly
  name.replace( '.', ' ' );
  pos = name.indexOf( ' ' );
  if ( pos != 0 ) {
    name[ pos + 1 ] = name[ pos + 1 ].toUpper();
  }
  name[ 0 ] = name[ 0 ].toUpper();

  KPIMIdentities::IdentityManager *manager = mKernel->identityManager();
  if ( !manager->isUnique( name ) ) {
    name = manager->makeUnique( name );
  }
  return name;
}

QString AccountWizard::accountName() const
{
  // create account name
  // Use the domain part of the incoming server
  QString name( i18nc( "Default name for new email accounts/identities.", "Unnamed" ) );

  QString server = mIncomingServer->text();
  int pos = server.indexOf( '.' );
  if ( pos != -1 ) {
    name = server.mid( pos + 1 );
    name[ 0 ] = name[ 0 ].toUpper();
  }

  AccountManager *manager = mKernel->acctMgr();
  if ( !manager->isUnique( name ) ) {
    name = manager->makeUnique( name );
  }
  return name;
}

QLabel *AccountWizard::createInfoLabel( const QString &msg )
{
  QLabel *label = new QLabel( msg, this );
  label->setFrameStyle( QFrame::Panel | QFrame::Raised );
  label->setLineWidth( 3 );
  label->resize( fontMetrics().width( msg ) + 20, label->height() * 2 );
  label->move( width() / 2 - label->width() / 2, height() / 2 - label->height() / 2 );
  label->setAutoFillBackground( true );
  label->show();

  return label;
}

bool AccountWizard::onlyDefaultIdentity() const
{
  KPIMIdentities::IdentityManager* manager = kmkernel->identityManager();
  if ( manager->identities().count() == 1 && !manager->defaultIdentity().mailingAllowed() ) {
    return true;
  }
  return false;
}

void AccountWizard::accept()
{
  //Disable finish button to keep user from pressing it repeatedly
  //when dialog is waiting for server checks
  enableButton( KDialog::User1, false );

  //TODO: Add mProgress to the widget in some way...
  QTimer::singleShot( 0, this, SLOT( createTransport() ) );
}

void AccountWizard::createIdentity()
{
  // store identity information
  KPIMIdentities::IdentityManager *manager = mKernel->identityManager();
  KPIMIdentities::Identity *identity = 0;
  if ( onlyDefaultIdentity() ) {
    identity = &manager->modifyIdentityForUoid( manager->defaultIdentity().uoid() );
  } else {
    identity = &manager->newFromScratch( identityName() );
  }
  Q_ASSERT( identity != 0 );
  identity->setFullName( mRealName->text() );
  identity->setEmailAddr( mEMailAddress->text() );
  identity->setOrganization( mOrganization->text() );
  identity->setTransport( mTransport->name() );
  manager->commit();
  QTimer::singleShot( 0, this, SLOT( createAccount() ) );
}

void AccountWizard::createTransport()
{
  mTransport = TransportManager::self()->createTransport();

  if ( mLocalDelivery->isChecked() ) { // local delivery
    QString pathToSendmail = KStandardDirs::findExe( "sendmail" );
    if ( pathToSendmail.isEmpty() ) {
      pathToSendmail = KStandardDirs::findExe( "sendmail", "/usr/sbin" );
      if ( pathToSendmail.isEmpty() ) {
        kWarning(5006) <<"Could not find the sendmail binary for local delivery";
        // setting the path to the most likely location
        pathToSendmail = "/usr/sbin/sendmail";
      }
    }

    mTransport->setType( Transport::EnumType::Sendmail );
    mTransport->setName( i18n( "Sendmail" ) );
    mTransport->setHost( pathToSendmail );
    mTransport->setRequiresAuthentication( false );
    mTransport->setStorePassword( false );

    QTimer::singleShot( 0, this, SLOT( transportCreated() ) );
  } else { // delivery via SMTP
    mTransport->setType( Transport::EnumType::SMTP );
    mTransport->setName( accountName() );
    mTransport->setHost( mOutgoingServer->text() );
    mTransport->setUserName( mLoginName->text() );
    mTransport->setPassword( mPassword->text() );
    mTransport->setRequiresAuthentication( true );
    mTransport->setStorePassword( true );

    checkSmtpCapabilities( mTransport->host() );
  }
}

void AccountWizard::transportCreated()
{
  mTransport->writeConfig();
  TransportManager::self()->addTransport( mTransport );
  if ( mCreateNewIdentity->isChecked() ) {
    QTimer::singleShot( 0, this, SLOT( createIdentity() ) );
  } else {
    QTimer::singleShot( 0, this, SLOT( createAccount() ) );
  }
}

void AccountWizard::createAccount()
{
  // create incoming account
  AccountManager *acctManager = mKernel->acctMgr();

  switch ( mTypeBox->type() ) {
    case AccountTypeBox::Local:
    {
      mAccount = acctManager->create( KAccount::Local, i18n( "Local Account" ) );
      static_cast<KMAcctLocal*>( mAccount )->setLocation( mIncomingLocation->text() );
      break;
    }
    case AccountTypeBox::POP3:
    {
      mAccount = acctManager->create( KAccount::Pop, accountName() );
      KMail::PopAccount *acct = static_cast<KMail::PopAccount*>( mAccount );
      acct->setLogin( mLoginName->text() );
      acct->setPasswd( mPassword->text() );
      acct->setHost( mIncomingServer->text() );
      if ( !mPassword->text().isEmpty() )
        acct->setStorePasswd( true );
      break;
    }
    case AccountTypeBox::IMAP:
    {
      mAccount = acctManager->create( KAccount::Imap, accountName() );
      KMAcctImap *acct = static_cast<KMAcctImap*>( mAccount );
      acct->setLogin( mLoginName->text() );
      acct->setPasswd( mPassword->text() );
      acct->setHost( mIncomingServer->text() );
      if ( !mPassword->text().isEmpty() )
        acct->setStorePasswd( true );
      break;
    }
    case AccountTypeBox::dIMAP:
    {
      mAccount = acctManager->create( KAccount::DImap, accountName() );
      KMAcctCachedImap *acct = static_cast<KMAcctCachedImap*>( mAccount );
      acct->setLogin( mLoginName->text() );
      acct->setPasswd( mPassword->text() );
      acct->setHost( mIncomingServer->text() );
      if ( !mPassword->text().isEmpty() )
        acct->setStorePasswd( true );
      break;
    }
    case AccountTypeBox::Maildir:
    {
      mAccount = acctManager->create( KAccount::Maildir, i18n( "Local Account" ) );
      static_cast<KMAcctMaildir*>( mAccount )->setLocation( mIncomingLocation->text() );
      break;
    }
  }

  if ( mTypeBox->type() == AccountTypeBox::POP3 ) {
    checkPopCapabilities( mIncomingServer->text() );
  } else if ( mTypeBox->type() == AccountTypeBox::IMAP ||
              mTypeBox->type() == AccountTypeBox::dIMAP ) {
    checkImapCapabilities( mIncomingServer->text() );
  } else {
    QTimer::singleShot( 0, this, SLOT( accountCreated() ) );
  }
}

void AccountWizard::accountCreated()
{
  if ( mAccount ) {
    mKernel->acctMgr()->add( mAccount );
    mKernel->cleanupImapFolders();
  }

  finished();
}

void AccountWizard::finished()
{
  GlobalSettings::self()->writeConfig();
  kmkernel->firstStartDone();
  KAssistantDialog::accept();
}

// ----- Security Checks --------------

void AccountWizard::checkPopCapabilities( const QString &server )
{
  delete mServerTest;
  mServerTest = new ServerTest( this );
  mServerTest->setProtocol( "pop" );
  mServerTest->setServer( server );
  connect( mServerTest, SIGNAL( finished(QList<int>) ),
           this, SLOT( popCapabilities(QList<int>) ) );

  mServerTest->start();

  mAuthInfoLabel =
    createInfoLabel( i18n( "Checking for supported security capabilities of %1...", server ) );
}

void AccountWizard::checkImapCapabilities( const QString &server )
{
  delete mServerTest;
  mServerTest = new ServerTest( this );
  mServerTest->setProtocol( "imap" );
  mServerTest->setServer( server );
  connect( mServerTest, SIGNAL( finished(QList<int>) ),
           this, SLOT( imapCapabilities(QList<int>) ) );

  mServerTest->start();

  mAuthInfoLabel =
    createInfoLabel( i18n( "Checking for supported security capabilities of %1...", server ) );
}

void AccountWizard::checkSmtpCapabilities( const QString &server )
{
  delete mServerTest;
  mServerTest = new ServerTest( this );
  mServerTest->setProtocol( "smtp" );
  mServerTest->setServer( server );
  connect( mServerTest, SIGNAL( finished(QList<int>) ),
           this, SLOT( smtpCapabilities(QList<int>) ) );

  mServerTest->start();

  mAuthInfoLabel =
    createInfoLabel( i18n( "Checking for supported security capabilities of %1...", server ) );
}

void AccountWizard::popCapabilities( QList<int> encryptionModes )
{
  KMail::NetworkAccount *account =
    static_cast<KMail::NetworkAccount*>( mAccount );

  bool useSSL = encryptionModes.contains( Transport::EnumEncryption::SSL );
  bool useTLS = encryptionModes.contains( Transport::EnumEncryption::TLS ) && !useSSL;

  account->setUseSSL( useSSL );
  account->setUseTLS( useTLS );

  QList<int> authModes;
  if ( useSSL )
    authModes = mServerTest->secureProtocols();
  else if ( useTLS )
    authModes = mServerTest->tlsProtocols();
  else
    authModes = mServerTest->normalProtocols();

  if ( authModes.contains( Transport::EnumAuthenticationType::PLAIN ) ) {
    account->setAuth( "PLAIN" );
  } else if ( authModes.contains( Transport::EnumAuthenticationType::LOGIN ) ) {
    account->setAuth( "LOGIN" );
  } else if ( authModes.contains( Transport::EnumAuthenticationType::CRAM_MD5 ) ) {
    account->setAuth( "CRAM-MD5" );
  } else if ( authModes.contains( Transport::EnumAuthenticationType::DIGEST_MD5 ) ) {
    account->setAuth( "DIGEST-MD5" );
  } else if ( authModes.contains( Transport::EnumAuthenticationType::NTLM ) ) {
    account->setAuth( "NTLM" );
  } else if ( authModes.contains( Transport::EnumAuthenticationType::GSSAPI ) ) {
    account->setAuth( "GSSAPI" );
  } else if ( authModes.contains( Transport::EnumAuthenticationType::APOP ) ) {
    account->setAuth( "APOP" );
  } else {
    account->setAuth( "USER" );
  }

  account->setPort( useSSL ? 995 : 110 );

  delete mAuthInfoLabel;
  mAuthInfoLabel = 0;

  accountCreated();
}

void AccountWizard::imapCapabilities( QList<int> encryptionModes )
{
  KMail::NetworkAccount *account =
    static_cast<KMail::NetworkAccount*>( mAccount );

  bool useSSL = encryptionModes.contains( Transport::EnumEncryption::SSL );
  bool useTLS = encryptionModes.contains( Transport::EnumEncryption::TLS ) && !useSSL;

  account->setUseSSL( useSSL );
  account->setUseTLS( useTLS );

  QList<int> authModes;
  if ( useSSL )
    authModes = mServerTest->secureProtocols();
  else if ( useTLS )
    authModes = mServerTest->tlsProtocols();
  else
    authModes = mServerTest->normalProtocols();

  if ( authModes.contains( Transport::EnumAuthenticationType::CRAM_MD5 ) ) {
    account->setAuth( "CRAM-MD5" );
  } else if ( authModes.contains( Transport::EnumAuthenticationType::DIGEST_MD5 ) ) {
    account->setAuth( "DIGEST-MD5" );
  } else if ( authModes.contains( Transport::EnumAuthenticationType::NTLM ) ) {
    account->setAuth( "NTLM" );
  } else if ( authModes.contains( Transport::EnumAuthenticationType::GSSAPI ) ) {
    account->setAuth( "GSSAPI" );
  } else if ( authModes.contains( Transport::EnumAuthenticationType::ANONYMOUS ) ) {
    account->setAuth( "ANONYMOUS" );
  } else if ( authModes.contains( Transport::EnumAuthenticationType::LOGIN ) ) {
    account->setAuth( "LOGIN" );
  } else if ( authModes.contains( Transport::EnumAuthenticationType::PLAIN ) ) {
    account->setAuth( "PLAIN" );
  } else {
    account->setAuth( "*" );
  }

  account->setPort( useSSL ? 993 : 143 );

  delete mAuthInfoLabel;
  mAuthInfoLabel = 0;

  accountCreated();
}

void AccountWizard::smtpCapabilities( QList<int> encryptionModes )
{
  bool useSSL = encryptionModes.contains( Transport::EnumEncryption::SSL );
  bool useTLS = encryptionModes.contains( Transport::EnumEncryption::TLS ) && !useSSL;

  QList<int> authModes;
  if ( useSSL ) {
    authModes = mServerTest->secureProtocols();
    mTransport->setEncryption( Transport::EnumEncryption::SSL );
  }
  else if ( useTLS ) {
    authModes = mServerTest->tlsProtocols();
    mTransport->setEncryption( Transport::EnumEncryption::TLS );
  }
  else {
    authModes = mServerTest->normalProtocols();
    mTransport->setEncryption( Transport::EnumEncryption::None );
  }

  if ( authModes.contains( Transport::EnumAuthenticationType::LOGIN ) ) {
    mTransport->setAuthenticationType( Transport::EnumAuthenticationType::LOGIN );
  } else if ( authModes.contains( Transport::EnumAuthenticationType::CRAM_MD5 ) ) {
    mTransport->setAuthenticationType( Transport::EnumAuthenticationType::CRAM_MD5 );
  } else if ( authModes.contains( Transport::EnumAuthenticationType::DIGEST_MD5 ) ) {
    mTransport->setAuthenticationType( Transport::EnumAuthenticationType::DIGEST_MD5 );
  } else if ( authModes.contains( Transport::EnumAuthenticationType::NTLM ) ) {
    mTransport->setAuthenticationType( Transport::EnumAuthenticationType::NTLM );
  } else if ( authModes.contains( Transport::EnumAuthenticationType::GSSAPI ) ) {
    mTransport->setAuthenticationType( Transport::EnumAuthenticationType::GSSAPI );
  } else {
    mTransport->setAuthenticationType( Transport::EnumAuthenticationType::PLAIN );
  }

  mTransport->setPort( useSSL ? 465 : 25 );

  delete mAuthInfoLabel;
  mAuthInfoLabel = 0;

  transportCreated();
}

#include "accountwizard.moc"
