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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <assert.h>

#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qradiobutton.h>
#include <qtabwidget.h>
#include <qvalidator.h>
#include <qlabel.h>
#include <qpushbutton.h>

#include <kapplication.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kseparator.h>

#include "kmservertest.h"
#include "kmtransport.h"
#include "kmaccount.h"


KMTransportInfo::KMTransportInfo()
{
  name = i18n("Unnamed");
  port = "25";
  auth = FALSE;
  storePass = FALSE;
}


KMTransportInfo::~KMTransportInfo()
{
}


void KMTransportInfo::readConfig(int id)
{
  KConfig *config = kapp->config();
  KConfigGroupSaver saver(config, "Transport " + QString::number(id));
  type = config->readEntry("type", "smtp");
  name = config->readEntry("name", i18n("Unnamed"));
  host = config->readEntry("host", "localhost");
  port = config->readEntry("port", "25");
  user = config->readEntry("user");
  pass = KMAccount::decryptStr(config->readEntry("pass"));
  precommand = config->readEntry("precommand");
  encryption = config->readEntry("encryption");
  authType = config->readEntry("authtype");
  auth = config->readBoolEntry("auth");
  storePass = config->readBoolEntry("storepass");
}


void KMTransportInfo::writeConfig(int id)
{
  KConfig *config = kapp->config();
  KConfigGroupSaver saver(config, "Transport " + QString::number(id));
  config->writeEntry("type", type);
  config->writeEntry("name", name);
  config->writeEntry("host", host);
  config->writeEntry("port", port);
  config->writeEntry("user", user);
  config->writeEntry("pass", (storePass) ? KMAccount::encryptStr(pass) : "");
  config->writeEntry("precommand", precommand);
  config->writeEntry("encryption", encryption);
  config->writeEntry("authtype", authType);
  config->writeEntry("auth", auth);
  config->writeEntry("storepass", storePass);
}


int KMTransportInfo::findTransport(const QString &name)
{
  KConfig *config = kapp->config();
  KConfigGroupSaver saver(config, "General");
  int numTransports = config->readNumEntry("transports", 0);
  for (int i = 1; i <= numTransports; i++)
  {
    KConfigGroupSaver saver(config, "Transport " + QString::number(i));
    if (config->readEntry("name") == name) return i;
  }
  return 0;
}


QStringList KMTransportInfo::availableTransports()
{
  QStringList result;
  KConfig *config = kapp->config();
  KConfigGroupSaver saver(config, "General");
  int numTransports = config->readNumEntry("transports", 0);
  for (int i = 1; i <= numTransports; i++)
  {
    KConfigGroupSaver saver(config, "Transport " + QString::number(i));
    result.append(config->readEntry("name"));
  }
  return result;
}


KMTransportSelDlg::KMTransportSelDlg( QWidget *parent, const char *name,
  bool modal )
  : KDialogBase( parent, name, modal, i18n("Add Transport"), Ok|Cancel, Ok )
{
  QFrame *page = makeMainWidget();
  QVBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );
 
  QButtonGroup *group = new QButtonGroup( i18n("Transport"), page );
  connect(group, SIGNAL(clicked(int)), SLOT(buttonClicked(int)) );
 
  topLayout->addWidget( group, 10 );
  QVBoxLayout *vlay = new QVBoxLayout( group, spacingHint()*2, spacingHint() );
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
				      QWidget *parent, const char *name,
				      bool modal )
  : KDialogBase( parent, name, modal, caption, Ok|Cancel, Ok, true )
{
  assert(transportInfo != NULL);
  mServerTest = NULL;
  mTransportInfo = transportInfo;
 
  if( transportInfo->type == QString::fromLatin1("sendmail") )
  {
    makeSendmailPage();
  } else {
    makeSmtpPage();
  }
 
  setupSettings();
}


KMTransportDialog::~KMTransportDialog()
{
}


void KMTransportDialog::makeSendmailPage()
{
  QFrame *page = makeMainWidget();
  QVBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );
 
  mSendmail.titleLabel = new QLabel( page );
  mSendmail.titleLabel->setText( i18n("Transport: Sendmail") );
  QFont titleFont( mSendmail.titleLabel->font() );
  titleFont.setBold( true );
  mSendmail.titleLabel->setFont( titleFont );
  topLayout->addWidget( mSendmail.titleLabel );
  KSeparator *hline = new KSeparator( KSeparator::HLine, page);
  topLayout->addWidget( hline );
 
  QGridLayout *grid = new QGridLayout( topLayout, 3, 3, spacingHint() );
  grid->addColSpacing( 1, fontMetrics().maxWidth()*15 );
  grid->setRowStretch( 2, 10 );
  grid->setColStretch( 1, 10 );

  QLabel *label = new QLabel( i18n("&Name:"), page );
  grid->addWidget( label, 0, 0 );
  mSendmail.nameEdit = new QLineEdit( page );
  label->setBuddy( mSendmail.nameEdit );
  grid->addWidget( mSendmail.nameEdit, 0, 1 );

  label = new QLabel( i18n("&Location:"), page );
  grid->addWidget( label, 1, 0 );
  mSendmail.locationEdit = new QLineEdit( page );
  label->setBuddy(mSendmail.locationEdit);
  grid->addWidget( mSendmail.locationEdit, 1, 1 );
  mSendmail.chooseButton =
    new QPushButton( i18n("&Choose..."), page );
  connect( mSendmail.chooseButton, SIGNAL(clicked()),
           this, SLOT(slotSendmailChooser()) );
  mSendmail.chooseButton->setAutoDefault( false );
  grid->addWidget( mSendmail.chooseButton, 1, 2 );
}


void KMTransportDialog::makeSmtpPage()
{
  QFrame *page = makeMainWidget();
  QVBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );
 
  mSmtp.titleLabel = new QLabel( page );
  mSmtp.titleLabel->setText( i18n("Transport: SMTP") );
  QFont titleFont( mSmtp.titleLabel->font() );
  titleFont.setBold( true );
  mSmtp.titleLabel->setFont( titleFont );
  topLayout->addWidget( mSmtp.titleLabel );
  KSeparator *hline = new KSeparator( KSeparator::HLine, page);
  topLayout->addWidget( hline );
 
  QTabWidget *tabWidget = new QTabWidget(page);
  topLayout->addWidget( tabWidget );
 
  QWidget *page1 = new QWidget( tabWidget );
  tabWidget->addTab( page1, i18n("&General") );
 
  QGridLayout *grid = new QGridLayout( page1, 14, 2, spacingHint() );
  grid->addColSpacing( 1, fontMetrics().maxWidth()*15 );
  grid->setRowStretch( 13, 10 );
  grid->setColStretch( 1, 10 );
 
  QLabel *label = new QLabel( i18n("&Name:"), page1 );
  grid->addWidget( label, 0, 0 );
  mSmtp.nameEdit = new QLineEdit( page1 );
  label->setBuddy( mSmtp.nameEdit );
  grid->addWidget( mSmtp.nameEdit, 0, 1 );
 
  label = new QLabel( i18n("&Host:"), page1 );
  grid->addWidget( label, 3, 0 );
  mSmtp.hostEdit = new QLineEdit( page1 );
  label->setBuddy( mSmtp.hostEdit );
  grid->addWidget( mSmtp.hostEdit, 3, 1 );

  label = new QLabel( i18n("&Port:"), page1 );
  grid->addWidget( label, 4, 0 );
  mSmtp.portEdit = new QLineEdit( page1 );
  mSmtp.portEdit->setValidator( new QIntValidator(this) );
  label->setBuddy( mSmtp.portEdit );
  grid->addWidget( mSmtp.portEdit, 4, 1 );

  mSmtp.authCheck =
    new QCheckBox( i18n("Server &requires authentication"), page1 );
  connect(mSmtp.authCheck, SIGNAL(clicked()),
          SLOT(slotRequiresAuthClicked()));
  grid->addMultiCellWidget( mSmtp.authCheck, 5, 5, 0, 1 );
 
  mSmtp.loginLabel = new QLabel( i18n("&Login:"), page1 );
  grid->addWidget( mSmtp.loginLabel, 6, 0 );
  mSmtp.loginEdit = new QLineEdit( page1 );
  mSmtp.loginLabel->setBuddy( mSmtp.loginEdit );
  grid->addWidget( mSmtp.loginEdit, 6, 1 );
 
  mSmtp.passwordLabel = new QLabel( i18n("P&assword:"), page1 );
  grid->addWidget( mSmtp.passwordLabel, 7, 0 );
  mSmtp.passwordEdit = new QLineEdit( page1 );
  mSmtp.passwordEdit->setEchoMode( QLineEdit::Password );
  mSmtp.passwordLabel->setBuddy( mSmtp.passwordEdit );
  grid->addWidget( mSmtp.passwordEdit, 7, 1 );
 
  mSmtp.storePasswordCheck =
    new QCheckBox( i18n("&Store SMTP password in configuration file"), page1 );
  grid->addMultiCellWidget( mSmtp.storePasswordCheck, 8, 8, 0, 1 );
 
  label = new QLabel( i18n("Pre&command:"), page1 );
  grid->addWidget( label, 9, 0 );
  mSmtp.precommand = new QLineEdit( page1 );
  label->setBuddy(mSmtp.precommand);
  grid->addWidget( mSmtp.precommand, 9, 1 );

  QWidget *page2 = new QWidget( tabWidget );
  tabWidget->addTab( page2, i18n("S&ecurity") );
  QVBoxLayout *vlay = new QVBoxLayout( page2, spacingHint() );
  mSmtp.encryptionGroup = new QButtonGroup( 1, Qt::Horizontal,
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
 
  mSmtp.authGroup = new QButtonGroup( 1, Qt::Horizontal,
    i18n("Authentication method"), page2 );
  mSmtp.authPlain = new QRadioButton( i18n("Please translate this "
    "authentication method only if you have a good reason", "PLAIN"),
    mSmtp.authGroup  );
  mSmtp.authLogin = new QRadioButton( i18n("Please translate this "
    "authentication method only if you have a good reason", "LOGIN"),
    mSmtp.authGroup );
  mSmtp.authCramMd5 = new QRadioButton( i18n("CRAM-MD5"), mSmtp.authGroup );
  mSmtp.authDigestMd5 = new QRadioButton( i18n("DIGEST-MD5"), mSmtp.authGroup );
  vlay->addWidget( mSmtp.authGroup );
 
  QHBoxLayout *buttonLay = new QHBoxLayout( vlay );
  mSmtp.checkCapabilities =
    new QPushButton( i18n("Check what the server supports"), page2 );
  connect(mSmtp.checkCapabilities, SIGNAL(clicked()),
    SLOT(slotCheckSmtpCapabilities()));
  buttonLay->addWidget( mSmtp.checkCapabilities );
  buttonLay->addStretch();
  vlay->addStretch();
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
    mSmtp.passwordEdit->setText(mTransportInfo->pass);
    mSmtp.storePasswordCheck->setChecked(mTransportInfo->storePass);
    mSmtp.precommand->setText(mTransportInfo->precommand);

    if (mTransportInfo->encryption == "TLS")
      mSmtp.encryptionTLS->setChecked(TRUE);
    else if (mTransportInfo->encryption == "SSL")
      mSmtp.encryptionSSL->setChecked(TRUE);
    else mSmtp.encryptionNone->setChecked(TRUE);

    if (mTransportInfo->authType == "LOGIN")
      mSmtp.authLogin->setChecked(TRUE);
    else if (mTransportInfo->authType == "CRAM-MD5")
      mSmtp.authCramMd5->setChecked(TRUE);
    else if (mTransportInfo->authType == "DIGEST-MD5")
      mSmtp.authDigestMd5->setChecked(TRUE);
    else mSmtp.authPlain->setChecked(TRUE);

    slotRequiresAuthClicked();
  }
}


void KMTransportDialog::saveSettings()
{
  if (mTransportInfo->type == "sendmail")
  {
    mTransportInfo->name = mSendmail.nameEdit->text();
    mTransportInfo->host = mSendmail.locationEdit->text();
  } else {
    mTransportInfo->name = mSmtp.nameEdit->text();
    mTransportInfo->host = mSmtp.hostEdit->text();
    mTransportInfo->port = mSmtp.portEdit->text();
    mTransportInfo->auth = mSmtp.authCheck->isChecked();
    mTransportInfo->user = mSmtp.loginEdit->text();
    mTransportInfo->pass = mSmtp.passwordEdit->text();
    mTransportInfo->storePass = mSmtp.storePasswordCheck->isChecked();
    mTransportInfo->precommand = mSmtp.precommand->text();

    mTransportInfo->encryption = (mSmtp.encryptionTLS->isChecked()) ? "TLS" :
    (mSmtp.encryptionSSL->isChecked()) ? "SSL" : "NONE";

    mTransportInfo->authType = (mSmtp.authLogin->isChecked()) ? "LOGIN" :
    (mSmtp.authCramMd5->isChecked()) ? "CRAM-MD5" :
    (mSmtp.authDigestMd5->isChecked()) ? "DIGEST-MD5" : "PLAIN";
  }
}


void KMTransportDialog::slotSendmailChooser()
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
  if (id == 1 || mSmtp.portEdit->text() == "465")
    mSmtp.portEdit->setText((id == 1) ? "465" : "25");
}


void KMTransportDialog::slotCheckSmtpCapabilities()
{
  if (mServerTest) delete mServerTest;
  mServerTest = new KMServerTest("smtp", mSmtp.hostEdit->text(),
    mSmtp.portEdit->text());
  connect(mServerTest, SIGNAL(capabilities(const QStringList &)),
    SLOT(slotSmtpCapabilities(const QStringList &)));
  mSmtp.checkCapabilities->setEnabled(FALSE);
}


void KMTransportDialog::checkHighest(QButtonGroup *btnGroup)
{
  QButton *btn;
  for (int i = btnGroup->count() - 1; i >= 0; i--)
  {
    btn = btnGroup->find(i);
    if (btn && btn->isEnabled())
    {
      btn->animateClick();
      break;
    }
  }
}


void KMTransportDialog::slotSmtpCapabilities(const QStringList & list)
{
  mSmtp.checkCapabilities->setEnabled(TRUE);
  bool nc = list.findIndex("NORMAL-CONNECTION") != -1;
  mSmtp.encryptionNone->setEnabled(nc);
  mSmtp.encryptionSSL->setEnabled(list.findIndex("SSL") != -1);
  mSmtp.encryptionTLS->setEnabled(list.findIndex("STARTTLS") != -1 && nc);
  mSmtp.authPlain->setEnabled(list.findIndex("PLAIN") != -1);
  mSmtp.authLogin->setEnabled(list.findIndex("LOGIN") != -1);
  mSmtp.authCramMd5->setEnabled(list.findIndex("CRAM-MD5") != -1);
  mSmtp.authDigestMd5->setEnabled(list.findIndex("DIGEST-MD5") != -1);
  checkHighest(mSmtp.encryptionGroup);
  checkHighest(mSmtp.authGroup);
  if (mServerTest) delete mServerTest;
  mServerTest = NULL;
}


void KMTransportDialog::slotOk()
{
  saveSettings();
  accept();
}


#include "kmtransport.moc"
