/**
 * kmtransport.cpp
 *
 * Copyright (c) 2001-2002 Michael Haeckel <haeckel@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2
 *  as published by the Free Software Foundation
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <config.h>
#include <assert.h>

#include "kmtransport.h"

#include <q3buttongroup.h>
#include <QCheckBox>
#include <QLayout>
//Added by qt3to4:
#include <QGridLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <klineedit.h>
#include <QRadioButton>
#include <QTabWidget>
#include <QValidator>
#include <QLabel>
#include <QPushButton>

#include <libkdepim/servertest.h>

#include <kfiledialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kseparator.h>
#include <kdebug.h>
#include <kconfiggroup.h>
#include <kwallet.h>
using KWallet::Wallet;
#include <kprotocolinfo.h>

#include "kmkernel.h"
#include "kmaccount.h"
#include "protocols.h"
#include "transportmanager.h"
using namespace KMail;

KMTransportInfo::KMTransportInfo() : mPasswdDirty( false ),
  mStorePasswd( false ), mStorePasswdInConfig( false ), mId( 0 )
{
  name = i18n("Unnamed");
  port = "25";
  auth = false;
  specifyHostname = false;
}


KMTransportInfo::~KMTransportInfo()
{
}


void KMTransportInfo::readConfig(int id)
{
  KConfigGroup config( KMKernel::config(), "Transport " + QString::number(id) );
  mId = config.readEntry( "id", QVariant( (uint) 0 ) ).toUInt();
  type = config.readEntry("type", "smtp");
  name = config.readEntry("name", i18n("Unnamed"));
  host = config.readEntry("host", "localhost");
  port = config.readEntry("port", "25");
  user = config.readEntry("user");
  mPasswd = KMAccount::decryptStr(config.readEntry("pass"));
  precommand = config.readPathEntry("precommand");
  encryption = config.readEntry("encryption");
  authType = config.readEntry("authtype");
  auth = config.readEntry("auth", false );
  mStorePasswd = config.readEntry("storepass", false );
  specifyHostname = config.readEntry("specifyHostname", false );
  localHostname = config.readEntry("localHostname", false );

  if ( !storePasswd() )
    return;

  if ( !mPasswd.isEmpty() ) {
    // migration to kwallet if available
    if ( Wallet::isEnabled() ) {
      config.deleteEntry( "pass" );
      mPasswdDirty = true;
      mStorePasswdInConfig = false;
      writeConfig( id );
    } else
      mStorePasswdInConfig = true;
  } else {
    // read password if wallet is open, defer otherwise
    if ( Wallet::isOpen( Wallet::NetworkWallet() ) )
      readPassword();
  }
}


void KMTransportInfo::writeConfig(int id)
{
  KConfigGroup config( KMKernel::config(), "Transport " + QString::number(id) );
  if (!mId)
    mId = TransportManager::createId();
  config.writeEntry("id", mId);
  config.writeEntry("type", type);
  config.writeEntry("name", name);
  config.writeEntry("host", host);
  config.writeEntry("port", port);
  config.writeEntry("user", user);
  config.writePathEntry("precommand", precommand);
  config.writeEntry("encryption", encryption);
  config.writeEntry("authtype", authType);
  config.writeEntry("auth", auth);
  config.writeEntry("storepass", storePasswd());
  config.writeEntry("specifyHostname", specifyHostname);
  config.writeEntry("localHostname", localHostname);

  if ( storePasswd() ) {
    // write password into the wallet if possible and necessary
    bool passwdStored = false;
    Wallet *wallet = kmkernel->wallet();
    if ( mPasswdDirty ) {
      if ( wallet && wallet->writePassword( "transport-" + QString::number(mId), passwd() ) == 0 ) {
        passwdStored = true;
        mPasswdDirty = false;
        mStorePasswdInConfig = false;
      }
    } else {
      passwdStored = wallet ? !mStorePasswdInConfig /*already in the wallet*/ : config.hasKey("pass");
    }
    // wallet not available, ask the user if we should use the config file instead
    if ( !passwdStored && ( mStorePasswdInConfig ||  KMessageBox::warningYesNo( 0,
         i18n("KWallet is not available. It is strongly recommended to use "
              "KWallet for managing your passwords.\n"
              "However, KMail can store the password in its configuration "
              "file instead. The password is stored in an obfuscated format, "
              "but should not be considered secure from decryption efforts "
              "if access to the configuration file is obtained.\n"
              "Do you want to store the password for account '%1' in the "
              "configuration file?", name ),
         i18n("KWallet Not Available"),
         KGuiItem( i18n("Store Password") ),
         KGuiItem( i18n("Do Not Store Password") ) )
         == KMessageBox::Yes ) ) {
      config.writeEntry( "pass", KMAccount::encryptStr( passwd() ) );
      mStorePasswdInConfig = true;
    }
  }

  // delete already stored password if password storage is disabled
  if ( !storePasswd() ) {
    if ( !Wallet::keyDoesNotExist(
          Wallet::NetworkWallet(), "kmail", "transport-" + QString::number(mId) ) ) {
      Wallet *wallet = kmkernel->wallet();
      if ( wallet )
        wallet->removeEntry( "transport-" + QString::number(mId) );
    }
    config.deleteEntry( "pass" );
  }
}


int KMTransportInfo::findTransport(const QString &name)
{
  KConfigGroup config( KMKernel::config(), "General" );
  int numTransports = config.readEntry("transports", 0 );
  for (int i = 1; i <= numTransports; i++)
  {
    KConfigGroup config( KMKernel::config(), "Transport " + QString::number(i) );
    if (config.readEntry("name") == name) return i;
  }
  return 0;
}


QStringList KMTransportInfo::availableTransports()
{
  QStringList result;
  KConfigGroup config( KMKernel::config(), "General" );
  int numTransports = config.readEntry("transports", 0 );
  for (int i = 1; i <= numTransports; i++)
  {
    KConfigGroup config( KMKernel::config(), "Transport " + QString::number(i) );
    result.append(config.readEntry("name"));
  }
  return result;
}


QString KMTransportInfo::passwd() const
{
  if ( auth && storePasswd() && mPasswd.isEmpty() )
    readPassword();
  return mPasswd;
}


void KMTransportInfo::setPasswd( const QString &passwd )
{
  if ( passwd != mPasswd ) {
    mPasswd = passwd;
    mPasswdDirty = true;
  }
}


void KMTransportInfo::setStorePasswd( bool store )
{
  if ( mStorePasswd != store && store )
    mPasswdDirty = true;
  mStorePasswd = store;
}


void KMTransportInfo::readPassword() const
{
  if ( !storePasswd() || !auth )
    return;

  // ### workaround for broken Wallet::keyDoesNotExist() which returns wrong
  // results for new entries without closing and reopening the wallet
  if ( Wallet::isOpen( Wallet::NetworkWallet() ) ) {
    Wallet* wallet = kmkernel->wallet();
    if ( !wallet || !wallet->hasEntry( "transport-" + QString::number(mId) ) )
      return;
  } else {
    if ( Wallet::keyDoesNotExist( Wallet::NetworkWallet(), "kmail", "transport-" + QString::number(mId) ) )
      return;
  }

  if ( kmkernel->wallet() )
    kmkernel->wallet()->readPassword( "transport-" + QString::number(mId), mPasswd );
}


KMTransportSelDlg::KMTransportSelDlg( QWidget *parent )
  : KDialog( parent )
{
  setCaption( i18n("Add Transport") );
  setButtons( Ok|Cancel );
  QWidget *page = new QWidget( this );
  setMainWidget( page );
  QVBoxLayout *topLayout = new QVBoxLayout( page );
  topLayout->setSpacing( spacingHint() );
  topLayout->setMargin( 0 );

  Q3ButtonGroup *group = new Q3ButtonGroup( i18n("Transport"), page );
  connect(group, SIGNAL(clicked(int)), SLOT(buttonClicked(int)) );

  topLayout->addWidget( group, 10 );
  QVBoxLayout *vlay = new QVBoxLayout( group );
  vlay->setSpacing( spacingHint() );
  vlay->setMargin( spacingHint()*2 );
  vlay->addSpacing( fontMetrics().lineSpacing() );

  QRadioButton *radioButton1 = new QRadioButton( i18n("SM&TP"), group );
  vlay->addWidget( radioButton1 );
  QRadioButton *radioButton2 = new QRadioButton( i18n("&Sendmail"), group );
  vlay->addWidget( radioButton2 );

  vlay->addStretch( 10 );

  radioButton1->setChecked(true); // Pop is most common ?
  buttonClicked(0);
}

void KMTransportSelDlg::buttonClicked( int id )
{
  mSelectedButton = id;
}


int KMTransportSelDlg::selected( void ) const
{
  return mSelectedButton;
}


KMTransportDialog::KMTransportDialog( const QString & caption,
				      KMTransportInfo *transportInfo,
				      QWidget *parent )
  : KDialog( parent ),
    mServerTest( 0 ),
    mTransportInfo( transportInfo ),
    mAuthNone( AllAuth ), mAuthSSL( AllAuth ), mAuthTLS( AllAuth )
{
  setCaption( caption );
  setButtons( Ok|Cancel );
  assert(transportInfo != 0);

  if( transportInfo->type == QString::fromLatin1("sendmail") )
  {
    makeSendmailPage();
  } else {
    makeSmtpPage();
  }

  setupSettings();
  connect(this,SIGNAL(okClicked()),this,SLOT(slotOk()));
}


KMTransportDialog::~KMTransportDialog()
{
}


void KMTransportDialog::makeSendmailPage()
{
  QWidget *page = new QWidget( this );
  setMainWidget( page );
  QVBoxLayout *topLayout = new QVBoxLayout( page );
  topLayout->setSpacing( spacingHint() );
  topLayout->setMargin( 0 );

  mSendmail.titleLabel = new QLabel( page );
  mSendmail.titleLabel->setText( i18n("Transport: Sendmail") );
  QFont titleFont( mSendmail.titleLabel->font() );
  titleFont.setBold( true );
  mSendmail.titleLabel->setFont( titleFont );
  topLayout->addWidget( mSendmail.titleLabel );
  KSeparator *hline = new KSeparator( Qt::Horizontal, page);
  topLayout->addWidget( hline );

  QGridLayout *grid = new QGridLayout();
  topLayout->addItem( grid );
  grid->setSpacing( spacingHint() );
  grid->addItem( new QSpacerItem( fontMetrics().maxWidth()*15, 0 ), 0, 1 );
  grid->setRowStretch( 2, 10 );
  grid->setColumnStretch( 1, 10 );

  QLabel *label = new QLabel( i18n("&Name:"), page );
  grid->addWidget( label, 0, 0 );
  mSendmail.nameEdit = new KLineEdit( page );
  label->setBuddy( mSendmail.nameEdit );
  grid->addWidget( mSendmail.nameEdit, 0, 1 );

  label = new QLabel( i18n("&Location:"), page );
  grid->addWidget( label, 1, 0 );
  mSendmail.locationEdit = new KLineEdit( page );
  label->setBuddy(mSendmail.locationEdit);
  grid->addWidget( mSendmail.locationEdit, 1, 1 );
  mSendmail.chooseButton =
    new QPushButton( i18n("Choos&e..."), page );
  connect( mSendmail.chooseButton, SIGNAL(clicked()),
           this, SLOT(slotSendmailChooser()) );

  connect( mSendmail.locationEdit, SIGNAL(textChanged ( const QString & )),
           this, SLOT(slotSendmailEditPath(const QString &)) );

  mSendmail.chooseButton->setAutoDefault( false );
  grid->addWidget( mSendmail.chooseButton, 1, 2 );
  slotSendmailEditPath(mSendmail.locationEdit->text());
}

void KMTransportDialog::slotSendmailEditPath(const QString & _text)
{
  enableButtonOk( !_text.isEmpty() );
}

void KMTransportDialog::makeSmtpPage()
{
  QWidget *page = new QWidget( this );
  setMainWidget( page );
  QVBoxLayout *topLayout = new QVBoxLayout( page );
  topLayout->setSpacing( spacingHint() );
  topLayout->setMargin( 0 );

  mSmtp.titleLabel = new QLabel( page );
  mSmtp.titleLabel->setText( i18n("Transport: SMTP") );
  QFont titleFont( mSmtp.titleLabel->font() );
  titleFont.setBold( true );
  mSmtp.titleLabel->setFont( titleFont );
  topLayout->addWidget( mSmtp.titleLabel );
  KSeparator *hline = new KSeparator( Qt::Horizontal, page);
  topLayout->addWidget( hline );

  QTabWidget *tabWidget = new QTabWidget(page);
  topLayout->addWidget( tabWidget );

  QWidget *page1 = new QWidget( tabWidget );
  tabWidget->addTab( page1, i18n("&General") );

  QGridLayout *grid = new QGridLayout( page1 );
  grid->setSpacing( spacingHint() );
  grid->addItem( new QSpacerItem( fontMetrics().maxWidth()*15, 0 ), 0, 1 );
  grid->setRowStretch( 13, 10 );
  grid->setColumnStretch( 1, 10 );

  QLabel *label = new QLabel( i18n("&Name:"), page1 );
  grid->addWidget( label, 0, 0 );
  mSmtp.nameEdit = new KLineEdit( page1 );
  mSmtp.nameEdit->setWhatsThis(
                  i18n("The name that KMail will use when "
                       "referring to this server."));
  label->setBuddy( mSmtp.nameEdit );
  grid->addWidget( mSmtp.nameEdit, 0, 1 );

  label = new QLabel( i18n("&Host:"), page1 );
  grid->addWidget( label, 3, 0 );
  mSmtp.hostEdit = new KLineEdit( page1 );
  mSmtp.hostEdit->setWhatsThis(
                  i18n("The domain name or numerical address "
                       "of the SMTP server."));
  label->setBuddy( mSmtp.hostEdit );
  grid->addWidget( mSmtp.hostEdit, 3, 1 );

  label = new QLabel( i18n("&Port:"), page1 );
  grid->addWidget( label, 4, 0 );
  mSmtp.portEdit = new KLineEdit( page1 );
  mSmtp.portEdit->setValidator( new QIntValidator(this) );
  mSmtp.portEdit->setWhatsThis(
                  i18n("The port number that the SMTP server "
                       "is listening on. The default port is 25."));
  label->setBuddy( mSmtp.portEdit );
  grid->addWidget( mSmtp.portEdit, 4, 1 );

  label = new QLabel( i18n("Preco&mmand:"), page1 );
  grid->addWidget( label, 5, 0 );
  mSmtp.precommand = new KLineEdit( page1 );
  mSmtp.precommand->setWhatsThis(
                  i18n("A command to run locally, prior "
                       "to sending email. This can be used "
                       "to set up ssh tunnels, for example. "
                       "Leave it empty if no command should be run."));
  label->setBuddy(mSmtp.precommand);
  grid->addWidget( mSmtp.precommand, 5, 1 );

  QFrame* line = new QFrame( page1 );
  line->setFrameStyle( QFrame::HLine | QFrame::Plain );
  grid->addWidget( line, 6, 0, 1, 2 );

  mSmtp.authCheck =
    new QCheckBox( i18n("Server &requires authentication"), page1 );
  mSmtp.authCheck->setWhatsThis(
                  i18n("Check this option if your SMTP server "
                       "requires authentication before accepting "
                       "mail. This is known as "
                       "'Authenticated SMTP' or simply ASMTP."));
  connect(mSmtp.authCheck, SIGNAL(clicked()),
          SLOT(slotRequiresAuthClicked()));
  grid->addWidget( mSmtp.authCheck, 7, 0, 1, 2 );

  mSmtp.loginLabel = new QLabel( i18n("&Login:"), page1 );
  grid->addWidget( mSmtp.loginLabel, 8, 0 );
  mSmtp.loginEdit = new KLineEdit( page1 );
  mSmtp.loginLabel->setBuddy( mSmtp.loginEdit );
  mSmtp.loginEdit->setWhatsThis(
                  i18n("The user name to send to the server "
                       "for authorization"));
  grid->addWidget( mSmtp.loginEdit, 8, 1 );

  mSmtp.passwordLabel = new QLabel( i18n("P&assword:"), page1 );
  grid->addWidget( mSmtp.passwordLabel, 9, 0 );
  mSmtp.passwordEdit = new KLineEdit( page1 );
  mSmtp.passwordEdit->setEchoMode( QLineEdit::Password );
  mSmtp.passwordLabel->setBuddy( mSmtp.passwordEdit );
  mSmtp.passwordEdit->setWhatsThis(
                  i18n("The password to send to the server "
                       "for authorization"));
  grid->addWidget( mSmtp.passwordEdit, 9, 1 );

  mSmtp.storePasswordCheck =
    new QCheckBox( i18n("&Store SMTP password"), page1 );
  mSmtp.storePasswordCheck->setWhatsThis(
                  i18n("Check this option to have KMail store "
                  "the password.\nIf KWallet is available "
                  "the password will be stored there which is considered "
                  "safe.\nHowever, if KWallet is not available, "
                  "the password will be stored in KMail's configuration "
                  "file. The password is stored in an "
                  "obfuscated format, but should not be "
                  "considered secure from decryption efforts "
                  "if access to the configuration file is obtained."));
  grid->addWidget( mSmtp.storePasswordCheck, 10, 0, 1, 2 );

  line = new QFrame( page1 );
  line->setFrameStyle( QFrame::HLine | QFrame::Plain );
  grid->addWidget( line, 11, 0, 1, 2 );

  mSmtp.specifyHostnameCheck =
    new QCheckBox( i18n("Sen&d custom hostname to server"), page1 );
  grid->addWidget( mSmtp.specifyHostnameCheck, 12, 0, 1, 2 );
  mSmtp.specifyHostnameCheck->setWhatsThis(
                  i18n("Check this option to have KMail use "
                       "a custom hostname when identifying itself "
                       "to the mail server."
                       "<p>This is useful when your system's hostname "
                       "may not be set correctly or to mask your "
                       "system's true hostname."));

  mSmtp.localHostnameLabel = new QLabel( i18n("Hos&tname:"), page1 );
  grid->addWidget( mSmtp.localHostnameLabel, 13, 0);
  mSmtp.localHostnameEdit = new KLineEdit( page1 );
  mSmtp.localHostnameEdit->setWhatsThis(
                  i18n("Enter the hostname KMail should use when "
                       "identifying itself to the server."));
  mSmtp.localHostnameLabel->setBuddy( mSmtp.localHostnameEdit );
  grid->addWidget( mSmtp.localHostnameEdit, 13, 1 );
  connect( mSmtp.specifyHostnameCheck, SIGNAL(toggled(bool)),
           mSmtp.localHostnameEdit, SLOT(setEnabled(bool)));
  connect( mSmtp.specifyHostnameCheck, SIGNAL(toggled(bool)),
           mSmtp.localHostnameLabel, SLOT(setEnabled(bool)));

  QWidget *page2 = new QWidget( tabWidget );
  tabWidget->addTab( page2, i18n("S&ecurity") );
  QVBoxLayout *vlay = new QVBoxLayout( page2 );
  vlay->setSpacing( spacingHint() );
  mSmtp.encryptionGroup = new Q3ButtonGroup( 1, Qt::Horizontal,
    i18n("Encryption"), page2 );
  mSmtp.encryptionNone =
    new QRadioButton( i18n("&None"), mSmtp.encryptionGroup );
  mSmtp.encryptionSSL =
    new QRadioButton( i18n("&SSL"), mSmtp.encryptionGroup );
  mSmtp.encryptionTLS =
    new QRadioButton( i18n("&TLS"), mSmtp.encryptionGroup );
  connect(mSmtp.encryptionGroup, SIGNAL(clicked(int)),
    SLOT(slotSmtpEncryptionChanged(int)));
  vlay->addWidget( mSmtp.encryptionGroup );

  mSmtp.authGroup = new Q3ButtonGroup( 1, Qt::Horizontal,
    i18n("Authentication Method"), page2 );
  mSmtp.authLogin = new QRadioButton( i18nc("Please translate this "
    "authentication method only if you have a good reason", "&LOGIN"),
    mSmtp.authGroup );
  mSmtp.authPlain = new QRadioButton( i18nc("Please translate this "
    "authentication method only if you have a good reason", "&PLAIN"),
    mSmtp.authGroup  );
  mSmtp.authCramMd5 = new QRadioButton( i18n("CRAM-MD&5"), mSmtp.authGroup );
  mSmtp.authDigestMd5 = new QRadioButton( i18n("&DIGEST-MD5"), mSmtp.authGroup );
  mSmtp.authNTLM = new QRadioButton( i18n("&NTLM"), mSmtp.authGroup );
  mSmtp.authGSSAPI = new QRadioButton( i18n("&GSSAPI"), mSmtp.authGroup );
  if ( KProtocolInfo::capabilities("smtp").contains("SASL") == 0 ) {
    mSmtp.authNTLM->hide();
    mSmtp.authGSSAPI->hide();
  }
  vlay->addWidget( mSmtp.authGroup );

  vlay->addStretch();

  QHBoxLayout *buttonLay = new QHBoxLayout();
  vlay->addLayout( buttonLay );
  mSmtp.checkCapabilities =
    new QPushButton( i18n("Check &What the Server Supports"), page2 );
  connect(mSmtp.checkCapabilities, SIGNAL(clicked()),
    SLOT(slotCheckSmtpCapabilities()));
  buttonLay->addStretch();
  buttonLay->addWidget( mSmtp.checkCapabilities );
}


void KMTransportDialog::setupSettings()
{
  if (mTransportInfo->type == "sendmail")
  {
    mSendmail.nameEdit->setText(mTransportInfo->name);
    mSendmail.locationEdit->setText(mTransportInfo->host);
  } else {
    mSmtp.nameEdit->setText(mTransportInfo->name);
    mSmtp.hostEdit->setText(mTransportInfo->host);
    mSmtp.portEdit->setText(mTransportInfo->port);
    mSmtp.authCheck->setChecked(mTransportInfo->auth);
    mSmtp.loginEdit->setText(mTransportInfo->user);
    mSmtp.passwordEdit->setText(mTransportInfo->passwd());
    mSmtp.storePasswordCheck->setChecked(mTransportInfo->storePasswd());
    mSmtp.precommand->setText(mTransportInfo->precommand);
    mSmtp.specifyHostnameCheck->setChecked(mTransportInfo->specifyHostname);
    mSmtp.localHostnameEdit->setText(mTransportInfo->localHostname);

    if (mTransportInfo->encryption == "TLS")
      mSmtp.encryptionTLS->setChecked(true);
    else if (mTransportInfo->encryption == "SSL")
      mSmtp.encryptionSSL->setChecked(true);
    else mSmtp.encryptionNone->setChecked(true);

    if (mTransportInfo->authType == "LOGIN")
      mSmtp.authLogin->setChecked(true);
    else if (mTransportInfo->authType == "CRAM-MD5")
      mSmtp.authCramMd5->setChecked(true);
    else if (mTransportInfo->authType == "DIGEST-MD5")
      mSmtp.authDigestMd5->setChecked(true);
    else if (mTransportInfo->authType == "NTLM")
      mSmtp.authNTLM->setChecked(true);
    else if (mTransportInfo->authType == "GSSAPI")
      mSmtp.authGSSAPI->setChecked(true);
    else mSmtp.authPlain->setChecked(true);

    slotRequiresAuthClicked();
    mSmtp.localHostnameEdit->setEnabled(mTransportInfo->specifyHostname);
    mSmtp.localHostnameLabel->setEnabled(mTransportInfo->specifyHostname);
  }
}


void KMTransportDialog::saveSettings()
{
  if (mTransportInfo->type == "sendmail")
  {
    mTransportInfo->name = mSendmail.nameEdit->text().trimmed();
    mTransportInfo->host = mSendmail.locationEdit->text().trimmed();
  } else {
    mTransportInfo->name = mSmtp.nameEdit->text();
    mTransportInfo->host = mSmtp.hostEdit->text().trimmed();
    mTransportInfo->port = mSmtp.portEdit->text().trimmed();
    mTransportInfo->auth = mSmtp.authCheck->isChecked();
    mTransportInfo->user = mSmtp.loginEdit->text().trimmed();
    mTransportInfo->setPasswd( mSmtp.passwordEdit->text() );
    mTransportInfo->setStorePasswd( mSmtp.storePasswordCheck->isChecked() );
    mTransportInfo->precommand = mSmtp.precommand->text().trimmed();
    mTransportInfo->specifyHostname = mSmtp.specifyHostnameCheck->isChecked();
    mTransportInfo->localHostname = mSmtp.localHostnameEdit->text().trimmed();

    mTransportInfo->encryption = (mSmtp.encryptionTLS->isChecked()) ? "TLS" :
    (mSmtp.encryptionSSL->isChecked()) ? "SSL" : "NONE";

    mTransportInfo->authType = (mSmtp.authLogin->isChecked()) ? "LOGIN" :
    (mSmtp.authCramMd5->isChecked()) ? "CRAM-MD5" :
    (mSmtp.authDigestMd5->isChecked()) ? "DIGEST-MD5" :
    (mSmtp.authNTLM->isChecked()) ? "NTLM" :
    (mSmtp.authGSSAPI->isChecked()) ? "GSSAPI" : "PLAIN";
  }
}


void KMTransportDialog::slotSendmailChooser()
{
  KFileDialog dialog(KUrl("/"), QString(), this );
  dialog.setCaption(i18n("Choose sendmail Location") );

  if( dialog.exec() == QDialog::Accepted )
  {
    KUrl url = dialog.selectedUrl();
    if( url.isEmpty() == true )
    {
      return;
    }

    if( url.isLocalFile() == false )
    {
      KMessageBox::sorry( 0, i18n( "Only local files allowed." ) );
      return;
    }

    mSendmail.locationEdit->setText( url.path() );
  }
}


void KMTransportDialog::slotRequiresAuthClicked()
{
  bool b = mSmtp.authCheck->isChecked();
  mSmtp.loginLabel->setEnabled(b);
  mSmtp.loginEdit->setEnabled(b);
  mSmtp.passwordLabel->setEnabled(b);
  mSmtp.passwordEdit->setEnabled(b);
  mSmtp.storePasswordCheck->setEnabled(b);
  mSmtp.authGroup->setEnabled(b);
}


void KMTransportDialog::slotSmtpEncryptionChanged(int id)
{
  kDebug(5006) << "KMTransportDialog::slotSmtpEncryptionChanged( " << id << " )" << endl;
  // adjust SSL port:
  if (id == SSL || mSmtp.portEdit->text() == "465")
    mSmtp.portEdit->setText((id == SSL) ? "465" : "25");

  // switch supported auth methods:
  QAbstractButton *old = mSmtp.authGroup->selected();
  int authMethods = id == TLS ? mAuthTLS : id == SSL ? mAuthSSL : mAuthNone ;
  enableAuthMethods( authMethods );
  if ( !old->isEnabled() )
    checkHighest( mSmtp.authGroup );
}

void KMTransportDialog::enableAuthMethods( unsigned int auth ) {
  kDebug(5006) << "KMTransportDialog::enableAuthMethods( " << auth << " )" << endl;
  mSmtp.authPlain->setEnabled( auth & PLAIN );
  // LOGIN doesn't offer anything over PLAIN, requires more server
  // roundtrips and is not an official SASL mechanism, but a MS-ism,
  // so only enable it if PLAIN isn't available:
  mSmtp.authLogin->setEnabled( auth & LOGIN && !(auth & PLAIN));
  mSmtp.authCramMd5->setEnabled( auth & CRAM_MD5 );
  mSmtp.authDigestMd5->setEnabled( auth & DIGEST_MD5 );
  mSmtp.authNTLM->setEnabled( auth & NTLM );
  mSmtp.authGSSAPI->setEnabled( auth & GSSAPI );
}

unsigned int KMTransportDialog::authMethodsFromString( const QString & s ) {
  unsigned int result = 0;
  QStringList sl = s.toUpper().split( '\n', QString::SkipEmptyParts );
  for ( QStringList::const_iterator it = sl.begin() ; it != sl.end() ; ++it )
    if (  *it == "SASL/LOGIN" )
      result |= LOGIN;
    else if ( *it == "SASL/PLAIN" )
      result |= PLAIN;
    else if ( *it == "SASL/CRAM-MD5" )
      result |= CRAM_MD5;
    else if ( *it == "SASL/DIGEST-MD5" )
      result |= DIGEST_MD5;
    else if ( *it == "SASL/NTLM" )
      result |= NTLM;
    else if ( *it == "SASL/GSSAPI" )
      result |= GSSAPI;
  return result;
}

unsigned int KMTransportDialog::authMethodsFromStringList( const QStringList & sl ) {
  unsigned int result = 0;
  for ( QStringList::const_iterator it = sl.begin() ; it != sl.end() ; ++it )
    if ( *it == "LOGIN" )
      result |= LOGIN;
    else if ( *it == "PLAIN" )
      result |= PLAIN;
    else if ( *it == "CRAM-MD5" )
      result |= CRAM_MD5;
    else if ( *it == "DIGEST-MD5" )
      result |= DIGEST_MD5;
    else if ( *it == "NTLM" )
      result |= NTLM;
    else if ( *it == "GSSAPI" )
      result |= GSSAPI;
  return result;
}

void KMTransportDialog::slotCheckSmtpCapabilities()
{
  delete mServerTest;
  mServerTest = new KPIM::ServerTest(SMTP_PROTOCOL, mSmtp.hostEdit->text(),
    mSmtp.portEdit->text().toInt());
  connect( mServerTest,
           SIGNAL( capabilities( const QStringList &, const QStringList &,
                                 const QString &, const QString &,
                                 const QString & )),
           this,
           SLOT( slotSmtpCapabilities( const QStringList &,
                                       const QStringList &, const QString &,
                                       const QString &, const QString & ) ) );
  mSmtp.checkCapabilities->setEnabled(false);
}


void KMTransportDialog::checkHighest(Q3ButtonGroup *btnGroup)
{
  for ( int i = btnGroup->count() - 1; i >= 0 ; --i )
  {
    QAbstractButton * btn = btnGroup->find(i);
    if (btn && btn->isEnabled())
    {
      btn->animateClick();
      return;
    }
  }
}


void KMTransportDialog::slotSmtpCapabilities( const QStringList & capaNormal,
                                              const QStringList & capaSSL,
                                              const QString & authNone,
                                              const QString & authSSL,
                                              const QString & authTLS )
{
  mSmtp.checkCapabilities->setEnabled( true );
  kDebug(5006) << "KMTransportDialog::slotSmtpCapabilities( ..., "
	    << authNone << ", " << authSSL << ", " << authTLS << " )" << endl;
  mSmtp.encryptionNone->setEnabled( !capaNormal.isEmpty() );
  mSmtp.encryptionSSL->setEnabled( !capaSSL.isEmpty() );
  mSmtp.encryptionTLS->setEnabled( capaNormal.indexOf("STARTTLS") != -1 );
  if ( authNone.isEmpty() && authSSL.isEmpty() && authTLS.isEmpty() ) {
    // slave doesn't seem to support "* AUTH METHODS" metadata (or server can't do AUTH)
    mAuthNone = authMethodsFromStringList( capaNormal );
    if ( mSmtp.encryptionTLS->isEnabled() )
      mAuthTLS = mAuthNone;
    else
      mAuthTLS = 0;
    mAuthSSL = authMethodsFromStringList( capaSSL );
  }
  else {
    mAuthNone = authMethodsFromString( authNone );
    mAuthSSL = authMethodsFromString( authSSL );
    mAuthTLS = authMethodsFromString( authTLS );
  }
  kDebug(5006) << "mAuthNone = " << mAuthNone
                << "; mAuthSSL = " << mAuthSSL
                << "; mAuthTLS = " << mAuthTLS << endl;
  checkHighest( mSmtp.encryptionGroup );
  delete mServerTest;
  mServerTest = 0;
}
bool KMTransportDialog::sanityCheckSmtpInput()
{
  // FIXME: add additional checks for all fields that needs it
  // this is only the beginning
  if ( mSmtp.hostEdit->text().isEmpty() ) {
    QString errorMsg = i18n("The Host field cannot be empty. Please  "
                            "enter the name or the IP address of the SMTP server.");
    KMessageBox::sorry( this, errorMsg, i18n("Invalid Hostname or Address") );
    return false;
  }
  return true;
}

void KMTransportDialog::slotOk()
{
  if (mTransportInfo->type != "sendmail") {
    if( !sanityCheckSmtpInput() ) {
      return;
    }
  }

  saveSettings();
  accept();
}


#include "kmtransport.moc"
