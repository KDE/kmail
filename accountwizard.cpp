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

#include <libkdepim/servertest.h>

#include <mailtransport/transport.h>
#include <mailtransport/transportmanager.h>
using namespace MailTransport;

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

enum Capabilities {
  Plain      =   1,
  Login      =   2,
  CRAM_MD5   =   4,
  Digest_MD5 =   8,
  Anonymous  =  16,
  APOP       =  32,
  Pipelining =  64,
  TOP        = 128,
  UIDL       = 256,
  STLS       = 512, // TLS for POP
  STARTTLS   = 512, // TLS for IMAP
  GSSAPI     = 1024,
  NTLM       = 2048,
  AllCapa    = 0xffffffff
};

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
}

void AccountWizard::start( KMKernel *kernel, QWidget *parent )
{
  KConfigGroup wizardConfig( KMKernel::config(), "AccountWizard" );

  if ( wizardConfig.readEntry( "ShowOnStartup", true ) ) {
    AccountWizard wizard( kernel, parent );
    int result = wizard.exec();
    if ( result == QDialog::Accepted ) {
      wizardConfig.writeEntry( "ShowOnStartup", false );
      kernel->slotConfigChanged();
    }
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
  } else if ( current == mAccountInformationPage ) {
    if ( mRealName->text().isEmpty() && mEMailAddress->text().isEmpty() &&
         mOrganization->text().isEmpty() ) {
      KPIMIdentities::IdentityManager *manager = mKernel->identityManager();
      const KPIMIdentities::Identity &identity = manager->defaultIdentity();

      mRealName->setText( identity.fullName() );
      mEMailAddress->setText( identity.emailAddr() );
      mOrganization->setText( identity.organization() );
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

void AccountWizard::setupWelcomePage()
{
  KVBox *box = new KVBox( this );
  box->setSpacing( KDialog::spacingHint() );

  QLabel *label = new QLabel( i18n( "Welcome to KMail" ), box );
  QFont font = label->font();
  font.setBold( true );
  label->setFont( font );

  QLabel *message = new QLabel( i18n( "<qt>It seems you have started KMail for the first time. "
                    "You can use this wizard to setup your mail accounts. Just "
                    "enter the connection data that you received from your email provider "
                    "into the following pages.</qt>" ), box );
  message->setWordWrap( true );

  mWelcomePage = new KPageWidgetItem( box, i18n("Welcome") );
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

  mIncomingUseSSL = new QCheckBox( i18n( "Use secure connection (SSL)" ), page );
  layout->addWidget( mIncomingUseSSL, 1, 1 );

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

  mOutgoingUseSSL = new QCheckBox( i18n( "Use secure connection (SSL)" ), page );
  layout->addWidget( mOutgoingUseSSL, 3, 1 );

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

QString AccountWizard::accountName() const
{
  // create account name
  QString name( i18n( "None" ) );

  QString email = mEMailAddress->text();
  int pos = email.indexOf( '@' );
  if ( pos != -1 ) {
    name = email.mid( pos + 1 );
    name[ 0 ] = name[ 0 ].toUpper();
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

void AccountWizard::accept()
{
  // store identity information
  KPIMIdentities::IdentityManager *manager = mKernel->identityManager();
  KPIMIdentities::Identity &identity =
    manager->modifyIdentityForUoid( manager->defaultIdentity().uoid() );

  identity.setFullName( mRealName->text() );
  identity.setEmailAddr( mEMailAddress->text() );
  identity.setOrganization( mOrganization->text() );

  manager->commit();

  QTimer::singleShot( 0, this, SLOT( createTransport() ) );
}

void AccountWizard::createTransport()
{
  mTransport = TransportManager::self()->createTransport();

  if ( mLocalDelivery->isChecked() ) { // local delivery

    QString pathToSendmail = KStandardDirs::findExe( "sendmail" );
    if ( pathToSendmail.isEmpty() ) {
      pathToSendmail = KStandardDirs::findExe( "sendmail", "/usr/sbin" );
      if ( pathToSendmail.isEmpty() ) {
        kWarning() << "Could not find the sendmail binary for local delivery" << endl;
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
    mTransport->setName(accountName() );
    mTransport->setHost( mOutgoingServer->text() );
    mTransport->setUserName( mLoginName->text() );
    mTransport->setPassword( mPassword->text() );

    int port = ( mOutgoingUseSSL->isChecked() ? 465 : 25 );
    checkSmtpCapabilities( mTransport->host(), port );
  }
}

void AccountWizard::transportCreated()
{
  TransportManager::self()->addTransport( mTransport );
  QTimer::singleShot( 0, this, SLOT( createAccount() ) );
}

void AccountWizard::createAccount()
{
  // create incoming account
  AccountManager *acctManager = mKernel->acctMgr();

  int port = 0;

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
      port = mIncomingUseSSL->isChecked() ? 995 : 110;
      break;
    }
    case AccountTypeBox::IMAP:
    {
      mAccount = acctManager->create( KAccount::Imap, accountName() );
      KMAcctImap *acct = static_cast<KMAcctImap*>( mAccount );
      acct->setLogin( mLoginName->text() );
      acct->setPasswd( mPassword->text() );
      acct->setHost( mIncomingServer->text() );
      port = mIncomingUseSSL->isChecked() ? 993 : 143;
      break;
    }
    case AccountTypeBox::dIMAP:
    {
      mAccount = acctManager->create( KAccount::DImap, accountName() );
      KMAcctCachedImap *acct = static_cast<KMAcctCachedImap*>( mAccount );
      acct->setLogin( mLoginName->text() );
      acct->setPasswd( mPassword->text() );
      acct->setHost( mIncomingServer->text() );
      port = mIncomingUseSSL->isChecked() ? 993 : 143;
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
    checkPopCapabilities( mIncomingServer->text(), port );
  } else if ( mTypeBox->type() == AccountTypeBox::IMAP ||
              mTypeBox->type() == AccountTypeBox::dIMAP ) {
    checkImapCapabilities( mIncomingServer->text(), port );
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

  KAssistantDialog::accept();
}

// ----- Security Checks --------------

void AccountWizard::checkPopCapabilities( const QString &server, int port )
{
  delete mServerTest;
  mServerTest = new KPIM::ServerTest( POP_PROTOCOL, server, port );

  connect( mServerTest, SIGNAL( capabilities( const QStringList &,
                                              const QStringList & ) ),
           this, SLOT( popCapabilities( const QStringList &,
                                        const QStringList & ) ) );

  mAuthInfoLabel =
    createInfoLabel( i18n( "Checking for supported security capabilities of %1...", server ) );
}

void AccountWizard::checkImapCapabilities( const QString &server, int port )
{
  delete mServerTest;
  mServerTest = new KPIM::ServerTest( IMAP_PROTOCOL, server, port );

  connect( mServerTest, SIGNAL( capabilities( const QStringList &,
                                              const QStringList & ) ),
           this, SLOT( imapCapabilities( const QStringList &,
                                         const QStringList & ) ) );

  mAuthInfoLabel =
    createInfoLabel( i18n( "Checking for supported security capabilities of %1...", server ) );
}

void AccountWizard::checkSmtpCapabilities( const QString &server, int port )
{
  delete mServerTest;
  mServerTest = new KPIM::ServerTest( SMTP_PROTOCOL, server, port );

  connect( mServerTest, SIGNAL( capabilities( const QStringList &,
                                              const QStringList &,
                                              const QString &, const QString &,
                                              const QString & ) ),
           this, SLOT( smtpCapabilities( const QStringList &,
                                         const QStringList &,
                                         const QString &,
                                         const QString &, const QString & ) ) );

  mAuthInfoLabel =
    createInfoLabel( i18n( "Checking for supported security capabilities of %1...", server ) );
}

void AccountWizard::popCapabilities( const QStringList &capaNormalList,
                                     const QStringList &capaSSLList )
{
  uint capaNormal = popCapabilitiesFromStringList( capaNormalList );
  uint capaTLS = 0;

  if ( capaNormal & STLS ) {
    capaTLS = capaNormal;
  }

  uint capaSSL = popCapabilitiesFromStringList( capaSSLList );

  KMail::NetworkAccount *account =
    static_cast<KMail::NetworkAccount*>( mAccount );

  bool useSSL = !capaSSLList.isEmpty();
  bool useTLS = capaTLS != 0;

  account->setUseSSL( useSSL );
  account->setUseTLS( useTLS );

  uint capa = ( useSSL ? capaSSL : ( useTLS ? capaTLS : capaNormal ) );

  if ( capa & Plain ) {
    account->setAuth( "PLAIN" );
  } else if ( capa & Login ) {
    account->setAuth( "LOGIN" );
  } else if ( capa & CRAM_MD5 ) {
    account->setAuth( "CRAM-MD5" );
  } else if ( capa & Digest_MD5 ) {
    account->setAuth( "DIGEST-MD5" );
  } else if ( capa & NTLM ) {
    account->setAuth( "NTLM" );
  } else if ( capa & GSSAPI ) {
    account->setAuth( "GSSAPI" );
  } else if ( capa & APOP ) {
    account->setAuth( "APOP" );
  } else {
    account->setAuth( "USER" );
  }

  account->setPort( useSSL ? 995 : 110 );

  mServerTest->deleteLater();
  mServerTest = 0;

  delete mAuthInfoLabel;
  mAuthInfoLabel = 0;

  accountCreated();
}

void AccountWizard::imapCapabilities( const QStringList &capaNormalList,
                                      const QStringList &capaSSLList )
{
  uint capaNormal = imapCapabilitiesFromStringList( capaNormalList );
  uint capaTLS = 0;
  if ( capaNormal & STARTTLS ) {
    capaTLS = capaNormal;
  }

  uint capaSSL = imapCapabilitiesFromStringList( capaSSLList );

  KMail::NetworkAccount *account =
    static_cast<KMail::NetworkAccount*>( mAccount );

  bool useSSL = !capaSSLList.isEmpty();
  bool useTLS = ( capaTLS != 0 );

  account->setUseSSL( useSSL );
  account->setUseTLS( useTLS );

  uint capa = ( useSSL ? capaSSL : ( useTLS ? capaTLS : capaNormal ) );

  if ( capa & CRAM_MD5 ) {
    account->setAuth( "CRAM-MD5" );
  } else if ( capa & Digest_MD5 ) {
    account->setAuth( "DIGEST-MD5" );
  } else if ( capa & NTLM ) {
    account->setAuth( "NTLM" );
  } else if ( capa & GSSAPI ) {
    account->setAuth( "GSSAPI" );
  } else if ( capa & Anonymous ) {
    account->setAuth( "ANONYMOUS" );
  } else if ( capa & Login ) {
    account->setAuth( "LOGIN" );
  } else if ( capa & Plain ) {
    account->setAuth( "PLAIN" );
  } else {
    account->setAuth( "*" );
  }

  account->setPort( useSSL ? 993 : 143 );

  mServerTest->deleteLater();
  mServerTest = 0;

  delete mAuthInfoLabel;
  mAuthInfoLabel = 0;

  accountCreated();
}

void AccountWizard::smtpCapabilities( const QStringList &capaNormal,
                                      const QStringList &capaSSL,
                                      const QString &authNone,
                                      const QString &authSSL,
                                      const QString &authTLS )
{
  uint authBitsNone, authBitsSSL, authBitsTLS;

  if ( authNone.isEmpty() && authSSL.isEmpty() && authTLS.isEmpty() ) {
    // slave doesn't seem to support "* AUTH METHODS" metadata (or server can't do AUTH)
    authBitsNone = authMethodsFromStringList( capaNormal );
    if ( capaNormal.indexOf( "STARTTLS" ) != -1 ) {
      authBitsTLS = authBitsNone;
    } else {
      authBitsTLS = 0;
    }
    authBitsSSL = authMethodsFromStringList( capaSSL );
  } else {
    authBitsNone = authMethodsFromString( authNone );
    authBitsSSL = authMethodsFromString( authSSL );
    authBitsTLS = authMethodsFromString( authTLS );
  }

  uint authBits = 0;
  if ( capaNormal.indexOf( "STARTTLS" ) != -1 ) {
    mTransport->setEncryption( Transport::EnumEncryption::TLS );
    authBits = authBitsTLS;
  } else if ( !capaSSL.isEmpty() ) {
    mTransport->setEncryption( Transport::EnumEncryption::SSL );
    authBits = authBitsSSL;
  } else {
    mTransport->setEncryption( Transport::EnumEncryption::None );
    authBits = authBitsNone;
  }

  if ( authBits & Login ) {
    mTransport->setAuthenticationType( Transport::EnumAuthenticationType::LOGIN );
  } else if ( authBits & CRAM_MD5 ) {
    mTransport->setAuthenticationType( Transport::EnumAuthenticationType::CRAM_MD5 );
  } else if ( authBits & Digest_MD5 ) {
    mTransport->setAuthenticationType( Transport::EnumAuthenticationType::DIGEST_MD5 );
  } else if ( authBits & NTLM ) {
    mTransport->setAuthenticationType( Transport::EnumAuthenticationType::NTLM );
  } else if ( authBits & GSSAPI ) {
    mTransport->setAuthenticationType( Transport::EnumAuthenticationType::GSSAPI );
  } else {
    mTransport->setAuthenticationType( Transport::EnumAuthenticationType::PLAIN );
  }

  mTransport->setPort( !capaSSL.isEmpty() ? 465 : 25 );

  mServerTest->deleteLater();
  mServerTest = 0;

  delete mAuthInfoLabel;
  mAuthInfoLabel = 0;

  transportCreated();
}

uint AccountWizard::popCapabilitiesFromStringList( const QStringList &l )
{
  unsigned int capa = 0;

  for ( QStringList::const_iterator it = l.begin() ; it != l.end() ; ++it ) {
    QString cur = (*it).toUpper();
    if ( cur == "PLAIN" ) {
      capa |= Plain;
    } else if ( cur == "LOGIN" ) {
      capa |= Login;
    } else if ( cur == "CRAM-MD5" ) {
      capa |= CRAM_MD5;
    } else if ( cur == "DIGEST-MD5" ) {
      capa |= Digest_MD5;
    } else if ( cur == "NTLM" ) {
      capa |= NTLM;
    } else if ( cur == "GSSAPI" ) {
      capa |= GSSAPI;
    } else if ( cur == "APOP" ) {
      capa |= APOP;
    } else if ( cur == "STLS" ) {
      capa |= STLS;
    }
  }

  return capa;
}

uint AccountWizard::imapCapabilitiesFromStringList( const QStringList &l )
{
  unsigned int capa = 0;

  for ( QStringList::const_iterator it = l.begin() ; it != l.end() ; ++it ) {
    QString cur = (*it).toUpper();
    if ( cur == "AUTH=PLAIN" ) {
      capa |= Plain;
    } else if ( cur == "AUTH=LOGIN" ) {
      capa |= Login;
    } else if ( cur == "AUTH=CRAM-MD5" ) {
      capa |= CRAM_MD5;
    } else if ( cur == "AUTH=DIGEST-MD5" ) {
      capa |= Digest_MD5;
    } else if ( cur == "AUTH=NTLM" ) {
      capa |= NTLM;
    } else if ( cur == "AUTH=GSSAPI" ) {
      capa |= GSSAPI;
    } else if ( cur == "AUTH=ANONYMOUS" ) {
      capa |= Anonymous;
    } else if ( cur == "STARTTLS" ) {
      capa |= STARTTLS;
    }
  }

  return capa;
}

uint AccountWizard::authMethodsFromString( const QString &s )
{
  unsigned int result = 0;

  QStringList sl = s.toUpper().split( '\n', QString::SkipEmptyParts );
  for ( QStringList::const_iterator it = sl.begin() ; it != sl.end() ; ++it ) {
    if ( *it == "SASL/LOGIN" ) {
      result |= Login;
    } else if ( *it == "SASL/PLAIN" ) {
      result |= Plain;
    } else if ( *it == "SASL/CRAM-MD5" ) {
      result |= CRAM_MD5;
    } else if ( *it == "SASL/DIGEST-MD5" ) {
      result |= Digest_MD5;
    } else if ( *it == "SASL/NTLM" ) {
      result |= NTLM;
    } else if ( *it == "SASL/GSSAPI" ) {
      result |= GSSAPI;
    }
  }

  return result;
}

uint AccountWizard::authMethodsFromStringList( const QStringList &l )
{
  unsigned int result = 0;

  for ( QStringList::const_iterator it = l.begin() ; it != l.end() ; ++it ) {
    if ( *it == "LOGIN" ) {
      result |= Login;
    } else if ( *it == "PLAIN" ) {
      result |= Plain;
    } else if ( *it == "CRAM-MD5" ) {
      result |= CRAM_MD5;
    } else if ( *it == "DIGEST-MD5" ) {
      result |= Digest_MD5;
    } else if ( *it == "NTLM" ) {
      result |= NTLM;
    } else if ( *it == "GSSAPI" ) {
      result |= GSSAPI;
    }
  }

  return result;
}

#include "accountwizard.moc"
