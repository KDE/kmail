/*
 *   kmail: KDE mail client
 *   Copyright (C) 2000 Espen Sand, espen@kde.org
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
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#include "accountdialog.h"
#include "sieveconfig.h"
#include "kmacctmaildir.h"
#include "kmacctlocal.h"
#include "accountmanager.h"
#include "popaccount.h"
#include "kmacctimap.h"
#include "kmacctcachedimap.h"
#include "kmfoldermgr.h"
#include "protocols.h"
#include "folderrequester.h"
#include "kmmainwidget.h"
#include "kmfolder.h"
#include <kpimidentities/identitymanager.h>
#include <kpimidentities/identitycombo.h>
#include <kpimidentities/identity.h>
#include <mailtransport/servertest.h>
using namespace MailTransport;
#include "globalsettings.h"

#include <KGlobalSettings>
#include <KFileDialog>
#include <KLocale>
#include <KDebug>
#include <KMessageBox>
#include <KNumInput>
#include <KSeparator>
#include <KProtocolInfo>
#include <KIconLoader>
#include <KMenu>

#include <QButtonGroup>
#include <QCheckBox>
#include <QLayout>
#include <QTabWidget>
#include <QRadioButton>
#include <QValidator>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QToolButton>
#include <QGroupBox>
#include <QGridLayout>
#include <QTextStream>
#include <QList>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHash>

#include <netdb.h>
#include <netinet/in.h>

#include <cassert>
#include <stdlib.h>

#ifdef HAVE_PATHS_H
#include <paths.h>	/* defines _PATH_MAILDIR */
#endif

#ifndef _PATH_MAILDIR
#define _PATH_MAILDIR "/var/spool/mail"
#endif

namespace KMail {

class ProcmailRCParser
{
public:
  ProcmailRCParser(const QString &fileName = QString());
  ~ProcmailRCParser();

  QStringList getLockFilesList() const { return mLockFiles; }
  QStringList getSpoolFilesList() const { return mSpoolFiles; }

protected:
  void processGlobalLock(const QString&);
  void processLocalLock(const QString&);
  void processVariableSetting(const QString&, int);
  QString expandVars(const QString&);

  QFile mProcmailrc;
  QTextStream *mStream;
  QStringList mLockFiles;
  QStringList mSpoolFiles;
  QHash<QByteArray, QString> mVars;
};

ProcmailRCParser::ProcmailRCParser(const QString &filename)
  : mProcmailrc(filename),
    mStream(new QTextStream(&mProcmailrc))
{
  // predefined
  mVars.insert( "HOME", QDir::homePath() );

  if( filename.isEmpty() ) {
    mProcmailrc.setFileName(QDir::homePath() + "/.procmailrc");
  }

  QRegExp lockFileGlobal("^LOCKFILE=", Qt::CaseSensitive);
  QRegExp lockFileLocal("^:0", Qt::CaseSensitive);

  if(  mProcmailrc.open(QIODevice::ReadOnly) ) {

    QString s;

    while( !mStream->atEnd() ) {

      s = mStream->readLine().trimmed();

      if(  s[0] == '#' ) continue; // skip comments

      int commentPos = -1;

      if( (commentPos = s.indexOf('#')) > -1 ) {
        // get rid of trailing comment
        s.truncate(commentPos);
        s = s.trimmed();
      }

      if(  lockFileGlobal.indexIn(s) != -1 ) {
        processGlobalLock(s);
      } else if( lockFileLocal.indexIn(s) != -1 ) {
        processLocalLock(s);
      } else if( int i = s.indexOf('=') ) {
        processVariableSetting(s,i);
      }
    }

  }
  QString default_Location = qgetenv("MAIL");

  if (default_Location.isNull()) {
    default_Location = _PATH_MAILDIR;
    default_Location += '/';
    default_Location += qgetenv("USER");
  }
  if ( !mSpoolFiles.contains(default_Location) )
    mSpoolFiles << default_Location;

  default_Location = default_Location + ".lock";
  if ( !mLockFiles.contains(default_Location) )
    mLockFiles << default_Location;
}

ProcmailRCParser::~ProcmailRCParser()
{
  delete mStream;
}

void
ProcmailRCParser::processGlobalLock(const QString &s)
{
  QString val = expandVars(s.mid(s.indexOf('=') + 1).trimmed());
  if ( !mLockFiles.contains(val) )
    mLockFiles << val;
}

void
ProcmailRCParser::processLocalLock(const QString &s)
{
  QString val;
  int colonPos = s.lastIndexOf(':');

  if (colonPos > 0) { // we don't care about the leading one
    val = s.mid(colonPos + 1).trimmed();

    if ( val.length() ) {
      // user specified a lockfile, so process it
      //
      val = expandVars(val);
      if ( val[0] != '/' && mVars.contains("MAILDIR") )
        val.insert(0, mVars["MAILDIR"] + '/');
    } // else we'll deduce the lockfile name one we
    // get the spoolfile name
  }

  // parse until we find the spoolfile
  QString line, prevLine;
  do {
    prevLine = line;
    line = mStream->readLine().trimmed();
  } while ( !mStream->atEnd() && (line[0] == '*' ||
                                prevLine[prevLine.length() - 1] == '\\' ));

  if( line[0] != '!' && line[0] != '|' &&  line[0] != '{' ) {
    // this is a filename, expand it
    //
    line =  line.trimmed();
    line = expandVars(line);

    // prepend default MAILDIR if needed
    if( line[0] != '/' && mVars.contains("MAILDIR") )
      line.insert(0, mVars["MAILDIR"] + '/');

    // now we have the spoolfile name
    if ( !mSpoolFiles.contains(line) )
      mSpoolFiles << line;

    if( colonPos > 0 && val.isEmpty() ) {
      // there is a local lockfile, but the user didn't
      // specify the name so compute it from the spoolfile's name
      val = line;

      // append lock extension
      if( mVars.contains("LOCKEXT") )
        val += mVars["LOCKEXT"];
      else
        val += ".lock";
    }

    if ( !val.isNull() && !mLockFiles.contains(val) ) {
      mLockFiles << val;
    }
  }

}

void
ProcmailRCParser::processVariableSetting(const QString &s, int eqPos)
{
  if( eqPos == -1) return;

  QString varName = s.left(eqPos),
    varValue = expandVars(s.mid(eqPos + 1).trimmed());

  mVars.insert( varName.toLatin1(), varValue );
}

QString
ProcmailRCParser::expandVars(const QString &s)
{
  if( s.isEmpty()) return s;

  QString expS = s;

  for ( QHash<QByteArray, QString>::const_iterator it = mVars.begin(); it != mVars.end(); ++it ) {
    expS.replace( QString::fromLatin1("$") + it.key(), it.value() );
  }

  return expS;
}



AccountDialog::AccountDialog( const QString & caption, KMAccount *account,
                              QWidget *parent )
  : KDialog( parent ),
    mAccount( account ),
    mServerTest( 0 ),
    mSieveConfigEditor( 0 )
{
  setCaption( caption );
  setButtons( Ok|Cancel|Help );
  mValidator = new QRegExpValidator( QRegExp( "[A-Za-z0-9-_:.]*" ), 0 );
  setHelp("receiving-mail");

  KAccount::Type accountType = mAccount->type();

  if( accountType == KAccount::Local )
  {
    makeLocalAccountPage();
  }
  else if( accountType == KAccount::Maildir )
  {
    makeMaildirAccountPage();
  }
  else if( accountType == KAccount::Pop )
  {
    makePopAccountPage();
  }
  else if( accountType == KAccount::Imap )
  {
    makeImapAccountPage();
  }
  else if( accountType == KAccount::DImap )
  {
    makeImapAccountPage(true);
  }
  else
  {
    QString msg = i18n( "Account type is not supported." );
    KMessageBox::information( topLevelWidget(),msg,i18n("Configure Account") );
    return;
  }

  setupSettings();
  connect(this,SIGNAL(okClicked()),this,SLOT(slotOk()));
}

AccountDialog::~AccountDialog()
{
  delete mValidator;
  mValidator = 0;
  delete mServerTest;
  mServerTest = 0;
}

void AccountDialog::makeLocalAccountPage()
{
  QWidget *page = new QWidget( this );
  mLocal.ui.setupUi( page );
  setMainWidget( page );

  ProcmailRCParser procmailrcParser;
  mLocal.ui.locationEdit->addItems( procmailrcParser.getSpoolFilesList() );
  mLocal.ui.choose->setAutoDefault( false );
  mLocal.ui.procmailLockFileName->addItems( procmailrcParser.getLockFilesList() );
  mLocal.ui.intervalSpin->setRange( GlobalSettings::self()->minimumCheckInterval(), 10000, 1 );

  connect( mLocal.ui.choose, SIGNAL(clicked()), this, SLOT(slotLocationChooser()) );
  connect( mLocal.ui.lockProcmail, SIGNAL(toggled(bool)),
           mLocal.ui.procmailLockFileName, SLOT(setEnabled(bool)) );
  connect( mLocal.ui.intervalCheck, SIGNAL(toggled(bool)),
           this, SLOT(slotEnableLocalInterval(bool)) );
  connect( KGlobalSettings::self(),SIGNAL(kdisplayFontChanged()),
           SLOT(slotFontChanged()) );
}

void AccountDialog::makeMaildirAccountPage()
{
  QWidget *page = new QWidget( this );
  mMaildir.ui.setupUi( page );
  setMainWidget( page );
  ProcmailRCParser procmailrcParser;

  mMaildir.ui.locationEdit->addItems( procmailrcParser.getSpoolFilesList() );
  mMaildir.ui.choose->setAutoDefault( false );
  mMaildir.ui.intervalSpin->setRange( GlobalSettings::self()->minimumCheckInterval(), 10000, 1 );

  connect( mMaildir.ui.choose, SIGNAL(clicked()),
           this, SLOT(slotMaildirChooser()) );
  connect( mMaildir.ui.intervalCheck, SIGNAL(toggled(bool)),
           this, SLOT(slotEnableMaildirInterval(bool)) );
  connect( KGlobalSettings::self(),SIGNAL(kdisplayFontChanged()),
           SLOT(slotFontChanged()) );
}


void AccountDialog::makePopAccountPage()
{
  QWidget *page = new QWidget( this );
  mPop.ui.setupUi( page );
  setMainWidget( page );

  connect( mPop.ui.passwordEdit, SIGNAL( textEdited( const QString& ) ),
           this, SLOT( slotPopPasswordChanged( const QString& ) ) );

  // only letters, digits, '-', '.', ':' (IPv6) and '_' (for Windows
  // compatibility) are allowed
  mPop.ui.hostEdit->setValidator( mValidator );
  mPop.ui.portEdit->setValidator( new QIntValidator(this) );

  connect( mPop.ui.leaveOnServerCheck, SIGNAL( clicked() ),
           this, SLOT( slotLeaveOnServerClicked() ) );
  connect( mPop.ui.leaveOnServerDaysCheck, SIGNAL( toggled(bool) ),
           this, SLOT( slotEnableLeaveOnServerDays(bool)) );
  connect( mPop.ui.leaveOnServerDaysSpin, SIGNAL(valueChanged(int)),
           SLOT(slotLeaveOnServerDaysChanged(int)));
  connect( mPop.ui.leaveOnServerCountCheck, SIGNAL( toggled(bool) ),
           this, SLOT( slotEnableLeaveOnServerCount(bool)) );
  connect( mPop.ui.leaveOnServerCountSpin, SIGNAL(valueChanged(int)),
           SLOT(slotLeaveOnServerCountChanged(int)));
  connect( mPop.ui.leaveOnServerSizeCheck, SIGNAL( toggled(bool) ),
           this, SLOT( slotEnableLeaveOnServerSize(bool)) );

  connect(mPop.ui.filterOnServerSizeSpin, SIGNAL(valueChanged(int)),
          SLOT(slotFilterOnServerSizeChanged(int)));
  connect( mPop.ui.filterOnServerCheck, SIGNAL(toggled(bool)),
           mPop.ui.filterOnServerSizeSpin, SLOT(setEnabled(bool)) );
  connect( mPop.ui.filterOnServerCheck, SIGNAL( clicked() ),
           this, SLOT( slotFilterOnServerClicked() ) );

  connect( mPop.ui.intervalCheck, SIGNAL(toggled(bool)),
           this, SLOT(slotEnablePopInterval(bool)) );
  mPop.ui.intervalSpin->setRange( GlobalSettings::self()->minimumCheckInterval(),
                                  10000, 1 );

//  Page 2
  connect( mPop.ui.checkCapabilities, SIGNAL(clicked()),
           SLOT(slotCheckPopCapabilities()) );
  mPop.encryptionButtonGroup = new QButtonGroup();
  mPop.encryptionButtonGroup->addButton( mPop.ui.encryptionNone,
                                         Transport::EnumEncryption::None );
  mPop.encryptionButtonGroup->addButton( mPop.ui.encryptionSSL,
                                         Transport::EnumEncryption::SSL );
  mPop.encryptionButtonGroup->addButton( mPop.ui.encryptionTLS,
                                         Transport::EnumEncryption::TLS );

  connect( mPop.encryptionButtonGroup, SIGNAL(buttonClicked(int)),
           SLOT(slotPopEncryptionChanged(int)) );

  if ( KProtocolInfo::capabilities("pop3").contains("SASL") == 0 )
  {
    mPop.ui.authNTLM->hide();
    mPop.ui.authGSSAPI->hide();
  }
  mPop.authButtonGroup = new QButtonGroup();
  mPop.authButtonGroup->addButton( mPop.ui.authUser );
  mPop.authButtonGroup->addButton( mPop.ui.authLogin );
  mPop.authButtonGroup->addButton( mPop.ui.authPlain );
  mPop.authButtonGroup->addButton( mPop.ui.authCRAM_MD5 );
  mPop.authButtonGroup->addButton( mPop.ui.authDigestMd5 );
  mPop.authButtonGroup->addButton( mPop.ui.authNTLM );
  mPop.authButtonGroup->addButton( mPop.ui.authGSSAPI );
  mPop.authButtonGroup->addButton( mPop.ui.authAPOP );

  connect( mPop.ui.usePipeliningCheck, SIGNAL(clicked()),
           SLOT(slotPipeliningClicked()) );

  connect(KGlobalSettings::self(),SIGNAL(kdisplayFontChanged()),
          SLOT(slotFontChanged()));
}


void AccountDialog::makeImapAccountPage( bool connected )
{
  QWidget *page = new QWidget( this );
  mImap.ui.setupUi( page );
  setMainWidget( page );
  if( connected )
    mImap.ui.titleLabel->setText( i18n("Account Type: Disconnected IMAP Account") );
  else
    mImap.ui.titleLabel->setText( i18n("Account Type: IMAP Account") );

  // only letters, digits, '-', '.', ':' (IPv6) and '_' (for Windows
  // compatibility) are allowed
  mImap.ui.hostEdit->setValidator( mValidator );
  mImap.ui.portEdit->setValidator( new QIntValidator(this) );

  mImap.ui.button->setAutoRaise( true );
  mImap.ui.button->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );
  mImap.ui.button->setFixedSize( 22, 22 );
  mImap.ui.button->setIcon( KIcon("view-refresh") );
  mImap.ui.editPNS->setIcon( KIcon("document-properties") );
  mImap.ui.editPNS->setFixedSize( 22, 22 );
  mImap.ui.editONS->setIcon( KIcon("document-properties") );
  mImap.ui.editONS->setFixedSize( 22, 22 );
  mImap.ui.editSNS->setIcon( KIcon("document-properties") );
  mImap.ui.editSNS->setFixedSize( 22, 22 );

  if( connected ) {
    // not implemented for disconnected yet
    mImap.ui.autoExpungeCheck->hide();
    mImap.ui.loadOnDemandCheck->hide();
    mImap.ui.listOnlyOpenCheck->hide();
  }


  mImap.trashCombo = new FolderRequester( page,
                      kmkernel->getKMMainWidget()->folderTree() );
  mImap.trashCombo->setShowOutbox( false );
  mImap.ui.trashLabel->setBuddy( mImap.trashCombo );
  mImap.ui.trashLayout->addWidget( mImap.trashCombo, 1 );
  mImap.ui.trashLabel->setBuddy( mImap.trashCombo );

  mImap.identityCombo = new KPIMIdentities::IdentityCombo( kmkernel->identityManager(), page );
  mImap.ui.identityLabel->setBuddy( mImap.identityCombo );
  mImap.ui.identityLayout->addWidget( mImap.identityCombo, 1 );
  mImap.ui.identityLabel->setBuddy( mImap.identityCombo );

  mImap.encryptionButtonGroup = new QButtonGroup();
  mImap.encryptionButtonGroup->addButton( mImap.ui.encryptionNone,
                                          Transport::EnumEncryption::None );
  mImap.encryptionButtonGroup->addButton( mImap.ui.encryptionSSL,
                                          Transport::EnumEncryption::SSL );
  mImap.encryptionButtonGroup->addButton( mImap.ui.encryptionTLS,
                                          Transport::EnumEncryption::TLS );

  mImap.authButtonGroup = new QButtonGroup();
  mImap.authButtonGroup->addButton( mImap.ui.authUser );
  mImap.authButtonGroup->addButton( mImap.ui.authLogin );
  mImap.authButtonGroup->addButton( mImap.ui.authPlain );
  mImap.authButtonGroup->addButton( mImap.ui.authCramMd5 );
  mImap.authButtonGroup->addButton( mImap.ui.authDigestMd5 );
  mImap.authButtonGroup->addButton( mImap.ui.authNTLM );
  mImap.authButtonGroup->addButton( mImap.ui.authGSSAPI );
  mImap.authButtonGroup->addButton( mImap.ui.authAnonymous );

  // Connect all slots.
  connect( mImap.ui.button, SIGNAL(clicked()), this, SLOT(slotReloadNamespaces()) );
  connect( mImap.ui.editPNS, SIGNAL(clicked()), this, SLOT(slotEditPersonalNamespace()) );
  connect( mImap.ui.editONS, SIGNAL(clicked()), this, SLOT(slotEditOtherUsersNamespace()) );
  connect( mImap.ui.editSNS, SIGNAL(clicked()), this, SLOT(slotEditSharedNamespace()) );
  connect( mImap.ui.intervalCheck, SIGNAL(toggled(bool)), this, SLOT(slotEnableImapInterval(bool)) );
  connect( mImap.ui.useDefaultIdentityCheck, SIGNAL( toggled(bool) ), this, SLOT( slotIdentityCheckboxChanged() ) );
  connect( mImap.ui.checkCapabilities, SIGNAL(clicked()), SLOT(slotCheckImapCapabilities()));
  connect( mImap.encryptionButtonGroup, SIGNAL(buttonClicked(int)), SLOT(slotImapEncryptionChanged(int)) );

  // TODO (marc/bo): Test this
  mSieveConfigEditor = new SieveConfigEditor( mImap.ui.tabWidget );
  mSieveConfigEditor->layout()->setMargin( KDialog::marginHint() );
  mImap.ui.tabWidget->addTab( mSieveConfigEditor, i18n("&Filtering") );

  connect( KGlobalSettings::self(),SIGNAL(kdisplayFontChanged()),SLOT(slotFontChanged()) );
}


void AccountDialog::setupSettings()
{
  QComboBox *folderCombo = 0;
  int interval = mAccount->checkInterval();

  KAccount::Type accountType = mAccount->type();
  if( accountType == KAccount::Local )
  {
    ProcmailRCParser procmailrcParser;
    KMAcctLocal *acctLocal = static_cast<KMAcctLocal*>(mAccount);

    if ( acctLocal->location().isEmpty() )
      acctLocal->setLocation( procmailrcParser.getSpoolFilesList().first() );
    else
      mLocal.ui.locationEdit->addItem( acctLocal->location() );

    if ( acctLocal->procmailLockFileName().isEmpty() )
      acctLocal->setProcmailLockFileName( procmailrcParser.getLockFilesList().first() );
    else
      mLocal.ui.procmailLockFileName->addItem( acctLocal->procmailLockFileName() );

    mLocal.ui.nameEdit->setText( mAccount->name() );
    mLocal.ui.nameEdit->setFocus();
    mLocal.ui.locationEdit->setEditText( acctLocal->location() );
    if (acctLocal->lockType() == mutt_dotlock)
      mLocal.ui.lockMutt->setChecked(true);
    else if (acctLocal->lockType() == mutt_dotlock_privileged)
      mLocal.ui.lockMuttPriv->setChecked(true);
    else if (acctLocal->lockType() == procmail_lockfile) {
      mLocal.ui.lockProcmail->setChecked(true);
      mLocal.ui.procmailLockFileName->setEditText(acctLocal->procmailLockFileName());
    } else if (acctLocal->lockType() == FCNTL)
      mLocal.ui.lockFcntl->setChecked(true);
    else if (acctLocal->lockType() == lock_none)
      mLocal.ui.lockNone->setChecked(true);

    mLocal.ui.intervalSpin->setValue( qMax(1, interval) );
    mLocal.ui.intervalCheck->setChecked( interval >= 1 );
    mLocal.ui.includeInCheck->setChecked( !mAccount->checkExclude() );
    mLocal.ui.precommand->setText( mAccount->precommand() );

    slotEnableLocalInterval( interval >= 1 );
    folderCombo = mLocal.ui.folderCombo;
  }
  else if( accountType == KAccount::Pop )
  {
    PopAccount &ap = *(PopAccount*)mAccount;
    mPop.ui.nameEdit->setText( mAccount->name() );
    mPop.ui.nameEdit->setFocus();
    mPop.ui.loginEdit->setText( ap.login() );
    mPop.ui.passwordEdit->setText( ap.passwd());
    mPop.ui.hostEdit->setText( ap.host() );
    mPop.ui.portEdit->setText( QString("%1").arg( ap.port() ) );
    mPop.ui.usePipeliningCheck->setChecked( ap.usePipelining() );
    mPop.ui.storePasswordCheck->setChecked( ap.storePasswd() );
    mPop.ui.leaveOnServerCheck->setChecked( ap.leaveOnServer() );
    mPop.ui.leaveOnServerDaysCheck->setEnabled( ap.leaveOnServer() );
    mPop.ui.leaveOnServerDaysCheck->setChecked( ap.leaveOnServerDays() >= 1 );
    mPop.ui.leaveOnServerDaysSpin->setValue( ap.leaveOnServerDays() >= 1 ?
                                            ap.leaveOnServerDays() : 7 );
    mPop.ui.leaveOnServerCountCheck->setEnabled( ap.leaveOnServer() );
    mPop.ui.leaveOnServerCountCheck->setChecked( ap.leaveOnServerCount() >= 1 );
    mPop.ui.leaveOnServerCountSpin->setValue( ap.leaveOnServerCount() >= 1 ?
                                            ap.leaveOnServerCount() : 100 );
    mPop.ui.leaveOnServerSizeCheck->setEnabled( ap.leaveOnServer() );
    mPop.ui.leaveOnServerSizeCheck->setChecked( ap.leaveOnServerSize() >= 1 );
    mPop.ui.leaveOnServerSizeSpin->setValue( ap.leaveOnServerSize() >= 1 ?
                                            ap.leaveOnServerSize() : 10 );
    mPop.ui.filterOnServerCheck->setChecked( ap.filterOnServer() );
    mPop.ui.filterOnServerSizeSpin->setValue( ap.filterOnServerCheckSize() );
    mPop.ui.intervalCheck->setChecked( interval >= 1 );
    mPop.ui.intervalSpin->setValue( qMax(1, interval) );
    mPop.ui.includeInCheck->setChecked( !mAccount->checkExclude() );
    mPop.ui.precommand->setText( ap.precommand() );
    if (ap.useSSL())
      mPop.ui.encryptionSSL->setChecked( true );
    else if (ap.useTLS())
      mPop.ui.encryptionTLS->setChecked( true );
    else mPop.ui.encryptionNone->setChecked( true );
    if (ap.auth() == "LOGIN")
      mPop.ui.authLogin->setChecked( true );
    else if (ap.auth() == "PLAIN")
      mPop.ui.authPlain->setChecked( true );
    else if (ap.auth() == "CRAM-MD5")
      mPop.ui.authCRAM_MD5->setChecked( true );
    else if (ap.auth() == "DIGEST-MD5")
      mPop.ui.authDigestMd5->setChecked( true );
    else if (ap.auth() == "NTLM")
      mPop.ui.authNTLM->setChecked( true );
    else if (ap.auth() == "GSSAPI")
      mPop.ui.authGSSAPI->setChecked( true );
    else if (ap.auth() == "APOP")
      mPop.ui.authAPOP->setChecked( true );
    else mPop.ui.authUser->setChecked( true );

    slotEnableLeaveOnServerDays( mPop.ui.leaveOnServerDaysCheck->isEnabled() ?
                                   ap.leaveOnServerDays() >= 1 : 0);
    slotEnableLeaveOnServerCount( mPop.ui.leaveOnServerCountCheck->isEnabled() ?
                                    ap.leaveOnServerCount() >= 1 : 0);
    slotEnableLeaveOnServerSize( mPop.ui.leaveOnServerSizeCheck->isEnabled() ?
                                    ap.leaveOnServerSize() >= 1 : 0);
    slotEnablePopInterval( interval >= 1 );
    folderCombo = mPop.ui.folderCombo;
  }
  else if( accountType == KAccount::Imap )
  {
    KMAcctImap &ai = *(KMAcctImap*)mAccount;
    mImap.ui.nameEdit->setText( mAccount->name() );
    mImap.ui.nameEdit->setFocus();
    mImap.ui.loginEdit->setText( ai.login() );
    mImap.ui.passwordEdit->setText( ai.passwd());
    mImap.ui.hostEdit->setText( ai.host() );
    mImap.ui.portEdit->setText( QString("%1").arg( ai.port() ) );
    mImap.ui.autoExpungeCheck->setChecked( ai.autoExpunge() );
    mImap.ui.hiddenFoldersCheck->setChecked( ai.hiddenFolders() );
    mImap.ui.subscribedFoldersCheck->setChecked( ai.onlySubscribedFolders() );
    mImap.ui.locallySubscribedFoldersCheck->setChecked( ai.onlyLocallySubscribedFolders() );
    mImap.ui.loadOnDemandCheck->setChecked( ai.loadOnDemand() );
    mImap.ui.listOnlyOpenCheck->setChecked( ai.listOnlyOpenFolders() );
    mImap.ui.storePasswordCheck->setChecked( ai.storePasswd() );
    mImap.ui.intervalCheck->setChecked( interval >= 1 );
    mImap.ui.intervalSpin->setValue( qMax(1, interval) );
    mImap.ui.includeInCheck->setChecked( !ai.checkExclude() );
    QString trashfolder = ai.trash();
    if (trashfolder.isEmpty())
      trashfolder = kmkernel->trashFolder()->idString();
    mImap.trashCombo->setFolder( trashfolder );
    slotEnableImapInterval( interval >= 1 );
    mImap.identityCombo->setCurrentIdentity( mAccount->identityId() );
    mImap.ui.useDefaultIdentityCheck->setChecked( mAccount->useDefaultIdentity() );
    //mImap.identityCombo->insertStringList( kmkernel->identityManager()->shadowIdentities() );
    if (ai.useSSL())
      mImap.ui.encryptionSSL->setChecked( true );
    else if (ai.useTLS())
      mImap.ui.encryptionTLS->setChecked( true );
    else mImap.ui.encryptionNone->setChecked( true );
    if (ai.auth() == "CRAM-MD5")
      mImap.ui.authCramMd5->setChecked( true );
    else if (ai.auth() == "DIGEST-MD5")
      mImap.ui.authDigestMd5->setChecked( true );
    else if (ai.auth() == "NTLM")
      mImap.ui.authNTLM->setChecked( true );
    else if (ai.auth() == "GSSAPI")
      mImap.ui.authGSSAPI->setChecked( true );
    else if (ai.auth() == "ANONYMOUS")
      mImap.ui.authAnonymous->setChecked( true );
    else if (ai.auth() == "PLAIN")
      mImap.ui.authPlain->setChecked( true );
    else if (ai.auth() == "LOGIN")
      mImap.ui.authLogin->setChecked( true );
    else mImap.ui.authUser->setChecked( true );
    if ( mSieveConfigEditor )
      mSieveConfigEditor->setConfig( ai.sieveConfig() );
  }
  else if( accountType == KAccount::DImap )
  {
    KMAcctCachedImap &ai = *(KMAcctCachedImap*)mAccount;
    mImap.ui.nameEdit->setText( mAccount->name() );
    mImap.ui.nameEdit->setFocus();
    mImap.ui.loginEdit->setText( ai.login() );
    mImap.ui.passwordEdit->setText( ai.passwd());
    mImap.ui.hostEdit->setText( ai.host() );
    mImap.ui.portEdit->setText( QString("%1").arg( ai.port() ) );
    mImap.ui.hiddenFoldersCheck->setChecked( ai.hiddenFolders() );
    mImap.ui.subscribedFoldersCheck->setChecked( ai.onlySubscribedFolders() );
    mImap.ui.locallySubscribedFoldersCheck->setChecked( ai.onlyLocallySubscribedFolders() );
    mImap.ui.storePasswordCheck->setChecked( ai.storePasswd() );
    mImap.ui.intervalCheck->setChecked( interval >= 1 );
    mImap.ui.intervalSpin->setValue( qMax(1, interval) );
    mImap.ui.includeInCheck->setChecked( !ai.checkExclude() );
    mImap.ui.intervalCheck->setChecked( interval >= 1 );
    mImap.ui.intervalSpin->setValue( qMax(1, interval) );
    QString trashfolder = ai.trash();
    if (trashfolder.isEmpty())
      trashfolder = kmkernel->trashFolder()->idString();
    mImap.trashCombo->setFolder( trashfolder );
    slotEnableImapInterval( interval >= 1 );
    mImap.identityCombo->setCurrentIdentity( mAccount->identityId() );
    mImap.ui.useDefaultIdentityCheck->setChecked( mAccount->useDefaultIdentity() );
    //mImap.identityCombo->insertStringList( kmkernel->identityManager()->shadowIdentities() );
    if (ai.useSSL())
      mImap.ui.encryptionSSL->setChecked( true );
    else if (ai.useTLS())
      mImap.ui.encryptionTLS->setChecked( true );
    else mImap.ui.encryptionNone->setChecked( true );
    if (ai.auth() == "CRAM-MD5")
      mImap.ui.authCramMd5->setChecked( true );
    else if (ai.auth() == "DIGEST-MD5")
      mImap.ui.authDigestMd5->setChecked( true );
    else if (ai.auth() == "GSSAPI")
      mImap.ui.authGSSAPI->setChecked( true );
    else if (ai.auth() == "NTLM")
      mImap.ui.authNTLM->setChecked( true );
    else if (ai.auth() == "ANONYMOUS")
      mImap.ui.authAnonymous->setChecked( true );
    else if (ai.auth() == "PLAIN")
      mImap.ui.authPlain->setChecked( true );
    else if (ai.auth() == "LOGIN")
      mImap.ui.authLogin->setChecked( true );
    else mImap.ui.authUser->setChecked( true );
    if ( mSieveConfigEditor )
      mSieveConfigEditor->setConfig( ai.sieveConfig() );
  }
  else if( accountType == KAccount::Maildir )
  {
    KMAcctMaildir *acctMaildir = dynamic_cast<KMAcctMaildir*>(mAccount);

    mMaildir.ui.nameEdit->setText( mAccount->name() );
    mMaildir.ui.nameEdit->setFocus();
    mMaildir.ui.locationEdit->setEditText( acctMaildir->location() );

    mMaildir.ui.intervalSpin->setValue( qMax(1, interval) );
    mMaildir.ui.intervalCheck->setChecked( interval >= 1 );
    mMaildir.ui.includeInCheck->setChecked( !mAccount->checkExclude() );
    mMaildir.ui.precommand->setText( mAccount->precommand() );
    slotEnableMaildirInterval( interval >= 1 );
    folderCombo = mMaildir.ui.folderCombo;
  }
  else // Unknown account type
    return;

  if ( accountType == KAccount::Imap ||
       accountType == KAccount::DImap )
  {
    // settings for imap in general
    ImapAccountBase &ai = *(ImapAccountBase*)mAccount;
    // namespaces
    if ( ( ai.namespaces().isEmpty() || ai.namespaceToDelimiter().isEmpty() ) &&
         !ai.login().isEmpty() && !ai.passwd().isEmpty() && !ai.host().isEmpty() )
    {
      slotReloadNamespaces();
    } else {
      slotSetupNamespaces( ai.namespacesWithDelimiter() );
    }
  }

  if (!folderCombo) return;

  KMFolderDir *fdir = (KMFolderDir*)&kmkernel->folderMgr()->dir();
  KMFolder *acctFolder = mAccount->folder();
  if( acctFolder == 0 )
  {
    acctFolder = (KMFolder*)fdir->first();
  }
  if( acctFolder == 0 )
  {
    folderCombo->addItem( i18n("<none>") );
  }
  else
  {
    int i = 0;
    int curIndex = -1;
    kmkernel->folderMgr()->createI18nFolderList(&mFolderNames, &mFolderList);
    while (i < mFolderNames.count())
    {
      //QList<QPointer<KMFolder> >::Iterator it = mFolderList.at(i);
      KMFolder *folder = mFolderList.at(i);
      if (folder->isSystemFolder())
      {
        mFolderList.removeAll(folder);
        mFolderNames.removeAt(i);
      } else {
        if (folder == acctFolder) curIndex = i;
        i++;
      }
    }
    mFolderNames.prepend(i18n("inbox"));
    mFolderList.prepend(kmkernel->inboxFolder());
    folderCombo->addItems(mFolderNames);
    folderCombo->setCurrentIndex(curIndex + 1);

    // -sanders hack for startup users. Must investigate this properly
    if (folderCombo->count() == 0)
      folderCombo->addItem( i18n("inbox") );
  }
}

void AccountDialog::slotLeaveOnServerClicked()
{
  bool state = mPop.ui.leaveOnServerCheck->isChecked();
  mPop.ui.leaveOnServerDaysCheck->setEnabled( state );
  mPop.ui.leaveOnServerCountCheck->setEnabled( state );
  mPop.ui.leaveOnServerSizeCheck->setEnabled( state );
  if ( state ) {
    if ( mPop.ui.leaveOnServerDaysCheck->isChecked() ) {
      slotEnableLeaveOnServerDays( state );
    }
    if ( mPop.ui.leaveOnServerCountCheck->isChecked() ) {
      slotEnableLeaveOnServerCount( state );
    }
    if ( mPop.ui.leaveOnServerSizeCheck->isChecked() ) {
      slotEnableLeaveOnServerSize( state );
    }
  } else {
    slotEnableLeaveOnServerDays( state );
    slotEnableLeaveOnServerCount( state );
    slotEnableLeaveOnServerSize( state );
  }
  if ( mServerTest && !mServerTest->capabilities().contains( ServerTest::UIDL ) &&
       mPop.ui.leaveOnServerCheck->isChecked() ) {
    KMessageBox::information( topLevelWidget(),
                              i18n("The server does not seem to support unique "
                                   "message numbers, but this is a "
                                   "requirement for leaving messages on the "
                                   "server.\n"
                                   "Since some servers do not correctly "
                                   "announce their capabilities you still "
                                   "have the possibility to turn leaving "
                                   "fetched messages on the server on.") );
  }
}

void AccountDialog::slotFilterOnServerClicked()
{
  if ( mServerTest && !mServerTest->capabilities().contains( ServerTest::Top ) &&
       mPop.ui.filterOnServerCheck->isChecked() ) {
    KMessageBox::information( topLevelWidget(),
                              i18n("The server does not seem to support "
                                   "fetching message headers, but this is a "
                                   "requirement for filtering messages on the "
                                   "server.\n"
                                   "Since some servers do not correctly "
                                   "announce their capabilities you still "
                                   "have the possibility to turn filtering "
                                   "messages on the server on.") );
  }
}

void AccountDialog::slotPipeliningClicked()
{
  if (mPop.ui.usePipeliningCheck->isChecked())
    KMessageBox::information( topLevelWidget(),
      i18n("Please note that this feature can cause some POP3 servers "
      "that do not support pipelining to send corrupted mail;\n"
      "this is configurable, though, because some servers support pipelining "
      "but do not announce their capabilities. To check whether your POP3 server "
      "announces pipelining support use the \"Check What the Server "
      "Supports\" button at the bottom of the dialog;\n"
      "if your server does not announce it, but you want more speed, then "
      "you should do some testing first by sending yourself a batch "
      "of mail and downloading it."), QString(),
      "pipelining");
}


void AccountDialog::slotPopEncryptionChanged( int id )
{
  kDebug(5006) << "ID:" << id;
  // adjust port
  if ( id == Transport::EnumEncryption::SSL || mPop.ui.portEdit->text() == "995" )
    mPop.ui.portEdit->setText( ( id == Transport::EnumEncryption::SSL ) ? "995" : "110" );

  enablePopFeatures();
  const QAbstractButton *old = mPop.authButtonGroup->checkedButton();
  if ( old && !old->isEnabled() )
    checkHighest( mPop.authButtonGroup );
}

void AccountDialog::slotPopPasswordChanged(const QString& text)
{
  if ( text.isEmpty() )
    mPop.ui.storePasswordCheck->setCheckState( Qt::Unchecked );
  else
    mPop.ui.storePasswordCheck->setCheckState( Qt::Checked );
}

void AccountDialog::slotImapEncryptionChanged( int id )
{
  kDebug(5006) << id;
  // adjust port
  if ( id == Transport::EnumEncryption::SSL || mImap.ui.portEdit->text() == "993" )
    mImap.ui.portEdit->setText( ( id == Transport::EnumEncryption::SSL ) ? "993" : "143" );

  enableImapAuthMethods();
  QAbstractButton *old = mImap.authButtonGroup->checkedButton();
  if ( !old->isEnabled() )
    checkHighest( mImap.authButtonGroup );
}


void AccountDialog::slotCheckPopCapabilities()
{
  if ( mPop.ui.hostEdit->text().isEmpty() || mPop.ui.portEdit->text().isEmpty() )
  {
     KMessageBox::sorry( this, i18n( "Please specify a server and port on "
                                     "the General tab first." ) );
     return;
  }
  delete mServerTest;
  mServerTest = new ServerTest( this );
  Transport::EnumEncryption::type encryptionType;
  if ( mPop.ui.encryptionSSL->isChecked() )
    encryptionType = Transport::EnumEncryption::SSL;
  else
    encryptionType = Transport::EnumEncryption::None;
  mServerTest->setPort( encryptionType, mPop.ui.portEdit->text().toInt() );
  mServerTest->setServer( mPop.ui.hostEdit->text() );
  mServerTest->setProtocol( "pop" );
  connect( mServerTest, SIGNAL( finished(QList<int>) ),
           this, SLOT( slotPopCapabilities(QList<int>) ) );
  mServerTest->start();
  mServerTestFailed = false;
  mPop.ui.checkCapabilities->setEnabled( false );
}


void AccountDialog::slotCheckImapCapabilities()
{
  if ( mImap.ui.hostEdit->text().isEmpty() || mImap.ui.portEdit->text().isEmpty() )
  {
     KMessageBox::sorry( this, i18n( "Please specify a server and port on "
              "the General tab first." ) );
     return;
  }
  delete mServerTest;
  mServerTest = new ServerTest( this );
  Transport::EnumEncryption::type encryptionType;
  if ( mImap.ui.encryptionSSL->isChecked() )
    encryptionType = Transport::EnumEncryption::SSL;
  else
    encryptionType = Transport::EnumEncryption::None;
  mServerTest->setPort( encryptionType, mImap.ui.portEdit->text().toInt() );
  mServerTest->setServer( mImap.ui.hostEdit->text() );
  mServerTest->setProtocol( "imap" );
  connect( mServerTest, SIGNAL( finished(QList<int>) ),
           this, SLOT( slotImapCapabilities(QList<int>) ) );
  mServerTest->start();
  mServerTestFailed = false;
  mImap.ui.checkCapabilities->setEnabled(false);
}

void AccountDialog::slotPopCapabilities( QList<int> encryptionTypes )
{
  mPop.ui.checkCapabilities->setEnabled( true );

  // If the servertest did not find any useable authentication modes, assume the
  // connection failed and don't disable any of the radioboxes.
  if ( encryptionTypes.isEmpty() ) {
    mServerTestFailed = true;
    return;
  }

  mPop.ui.encryptionNone->setEnabled( encryptionTypes.contains( Transport::EnumEncryption::None ) );
  mPop.ui.encryptionSSL->setEnabled( encryptionTypes.contains( Transport::EnumEncryption::SSL ) );
  mPop.ui.encryptionTLS->setEnabled(  encryptionTypes.contains( Transport::EnumEncryption::TLS )  );
  checkHighest( mPop.encryptionButtonGroup );
}


void AccountDialog::enablePopFeatures()
{
  kDebug(5006);
  if ( !mServerTest || mServerTestFailed )
    return;

  QList<int> supportedAuths;
  if ( mPop.encryptionButtonGroup->checkedId() == Transport::EnumEncryption::None )
    supportedAuths = mServerTest->normalProtocols();
  if ( mPop.encryptionButtonGroup->checkedId() == Transport::EnumEncryption::SSL )
    supportedAuths = mServerTest->secureProtocols();
  if ( mPop.encryptionButtonGroup->checkedId() == Transport::EnumEncryption::TLS )
    supportedAuths = mServerTest->tlsProtocols();

  mPop.ui.authPlain->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::PLAIN ) );
  mPop.ui.authLogin->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::LOGIN ) );
  mPop.ui.authCRAM_MD5->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::CRAM_MD5 ) );
  mPop.ui.authDigestMd5->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::DIGEST_MD5 ) );
  mPop.ui.authNTLM->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::NTLM ) );
  mPop.ui.authGSSAPI->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::GSSAPI ) );
  mPop.ui.authAPOP->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::APOP ) );

  if ( mServerTest && !mServerTest->capabilities().contains( ServerTest::Pipelining ) &&
       mPop.ui.usePipeliningCheck->isChecked() ) {
    mPop.ui.usePipeliningCheck->setChecked( false );
    KMessageBox::information( topLevelWidget(),
                              i18n("The server does not seem to support "
                                   "pipelining; therefore, this option has "
                                   "been disabled.\n"
                                   "Since some servers do not correctly "
                                   "announce their capabilities you still "
                                   "have the possibility to turn pipelining "
                                   "on. But please note that this feature can "
                                   "cause some POP servers that do not "
                                   "support pipelining to send corrupt "
                                   "messages. So before using this feature "
                                   "with important mail you should first "
                                   "test it by sending yourself a larger "
                                   "number of test messages which you all "
                                   "download in one go from the POP "
                                   "server.") );
  }

  if ( mServerTest && !mServerTest->capabilities().contains( ServerTest::UIDL ) &&
       mPop.ui.leaveOnServerCheck->isChecked() ) {
    mPop.ui.leaveOnServerCheck->setChecked( false );
    KMessageBox::information( topLevelWidget(),
                              i18n("The server does not seem to support unique "
                                   "message numbers, but this is a "
                                   "requirement for leaving messages on the "
                                   "server; therefore, this option has been "
                                   "disabled.\n"
                                   "Since some servers do not correctly "
                                   "announce their capabilities you still "
                                   "have the possibility to turn leaving "
                                   "fetched messages on the server on.") );
  }

  if ( mServerTest && !mServerTest->capabilities().contains( ServerTest::Top ) &&
       mPop.ui.filterOnServerCheck->isChecked() ) {
    mPop.ui.filterOnServerCheck->setChecked( false );
    KMessageBox::information( topLevelWidget(),
                              i18n("The server does not seem to support "
                                   "fetching message headers, but this is a "
                                   "requirement for filtering messages on the "
                                   "server; therefore, this option has been "
                                   "disabled.\n"
                                   "Since some servers do not correctly "
                                   "announce their capabilities you still "
                                   "have the possibility to turn filtering "
                                   "messages on the server on.") );
  }
}

void AccountDialog::slotImapCapabilities( QList<int> encryptionTypes )
{
  mImap.ui.checkCapabilities->setEnabled( true );

  // If the servertest did not find any useable authentication modes, assume the
  // connection failed and don't disable any of the radioboxes.
  if ( encryptionTypes.isEmpty() ) {
    mServerTestFailed = true;
    return;
  }

  mImap.ui.encryptionNone->setEnabled( encryptionTypes.contains( Transport::EnumEncryption::None ) );
  mImap.ui.encryptionSSL->setEnabled( encryptionTypes.contains( Transport::EnumEncryption::SSL ) );
  mImap.ui.encryptionTLS->setEnabled(  encryptionTypes.contains( Transport::EnumEncryption::TLS )  );
  checkHighest( mImap.encryptionButtonGroup );
}

void AccountDialog::slotLeaveOnServerDaysChanged ( int value )
{
  mPop.ui.leaveOnServerDaysSpin->setSuffix( i18np(" day", " days", value) );
}


void AccountDialog::slotLeaveOnServerCountChanged ( int value )
{
  mPop.ui.leaveOnServerCountSpin->setSuffix( i18np(" message", " messages", value) );
}


void AccountDialog::slotFilterOnServerSizeChanged ( int value )
{
  mPop.ui.filterOnServerSizeSpin->setSuffix( i18np(" byte", " bytes", value) );
}

void AccountDialog::slotIdentityCheckboxChanged()
{
  if ( mAccount->type() == KAccount::Imap ||
       mAccount->type() == KAccount::DImap  ) {
     mImap.identityCombo->setEnabled( !mImap.ui.useDefaultIdentityCheck->isChecked() );
   }
   else
     assert( false );
}

void AccountDialog::enableImapAuthMethods()
{
  kDebug(5006);
  if ( !mServerTest || mServerTestFailed )
    return;

  QList<int> supportedAuths;
  if ( mImap.encryptionButtonGroup->checkedId() == Transport::EnumEncryption::None )
    supportedAuths = mServerTest->normalProtocols();
  if ( mImap.encryptionButtonGroup->checkedId() == Transport::EnumEncryption::SSL )
    supportedAuths = mServerTest->secureProtocols();
  if ( mImap.encryptionButtonGroup->checkedId() == Transport::EnumEncryption::TLS )
    supportedAuths = mServerTest->tlsProtocols();

  mImap.ui.authPlain->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::PLAIN ) );
  mImap.ui.authLogin->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::LOGIN ) );
  mImap.ui.authCramMd5->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::CRAM_MD5 ) );
  mImap.ui.authDigestMd5->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::DIGEST_MD5 ) );
  mImap.ui.authNTLM->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::NTLM ) );
  mImap.ui.authGSSAPI->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::GSSAPI ) );
  mImap.ui.authAnonymous->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::ANONYMOUS ) );
}


void AccountDialog::checkHighest( QButtonGroup *btnGroup )
{
  kDebug(5006) << btnGroup;
  QAbstractButton *btn;
  foreach (btn, btnGroup->buttons()) {
    if (btn && btn->isEnabled()) {
      btn->animateClick();
      return;
    }
  }
}


void AccountDialog::slotOk()
{
  saveSettings();
  accept();
}


void AccountDialog::saveSettings()
{
  KAccount::Type accountType = mAccount->type();
  if( accountType == KAccount::Local )
  {
    KMAcctLocal *acctLocal = dynamic_cast<KMAcctLocal*>(mAccount);

    if (acctLocal) {
      mAccount->setName( mLocal.ui.nameEdit->text() );
      acctLocal->setLocation( mLocal.ui.locationEdit->currentText() );
      if (mLocal.ui.lockMutt->isChecked())
        acctLocal->setLockType(mutt_dotlock);
      else if (mLocal.ui.lockMuttPriv->isChecked())
        acctLocal->setLockType(mutt_dotlock_privileged);
      else if (mLocal.ui.lockProcmail->isChecked()) {
        acctLocal->setLockType(procmail_lockfile);
        acctLocal->setProcmailLockFileName(mLocal.ui.procmailLockFileName->currentText());
      }
      else if (mLocal.ui.lockNone->isChecked())
        acctLocal->setLockType(lock_none);
      else acctLocal->setLockType(FCNTL);
    }

    mAccount->setCheckInterval( mLocal.ui.intervalCheck->isChecked() ?
                                mLocal.ui.intervalSpin->value() : 0 );
    mAccount->setCheckExclude( !mLocal.ui.includeInCheck->isChecked() );

    mAccount->setPrecommand( mLocal.ui.precommand->text() );

    mAccount->setFolder( mFolderList.at(mLocal.ui.folderCombo->currentIndex()) );
  }
  else if( accountType == KAccount::Pop )
  {
    mAccount->setName( mPop.ui.nameEdit->text() );
    mAccount->setCheckInterval( mPop.ui.intervalCheck->isChecked() ?
                                mPop.ui.intervalSpin->value() : 0 );
    mAccount->setCheckExclude( !mPop.ui.includeInCheck->isChecked() );

    mAccount->setFolder( mFolderList.at(mPop.ui.folderCombo->currentIndex()) );

    initAccountForConnect();
    PopAccount &epa = *(PopAccount*)mAccount;
    epa.setUsePipelining( mPop.ui.usePipeliningCheck->isChecked() );
    epa.setLeaveOnServer( mPop.ui.leaveOnServerCheck->isChecked() );
    epa.setLeaveOnServerDays( mPop.ui.leaveOnServerCheck->isChecked() ?
                              ( mPop.ui.leaveOnServerDaysCheck->isChecked() ?
                                mPop.ui.leaveOnServerDaysSpin->value() : -1 ) : 0);
    epa.setLeaveOnServerCount( mPop.ui.leaveOnServerCheck->isChecked() ?
                               ( mPop.ui.leaveOnServerCountCheck->isChecked() ?
                                 mPop.ui.leaveOnServerCountSpin->value() : -1 ) : 0 );
    epa.setLeaveOnServerSize( mPop.ui.leaveOnServerCheck->isChecked() ?
                              ( mPop.ui.leaveOnServerSizeCheck->isChecked() ?
                                mPop.ui.leaveOnServerSizeSpin->value() : -1 ) : 0 );
    epa.setFilterOnServer( mPop.ui.filterOnServerCheck->isChecked() );
    epa.setFilterOnServerCheckSize (mPop.ui.filterOnServerSizeSpin->value() );
    epa.setPrecommand( mPop.ui.precommand->text() );

  }
  else if( accountType == KAccount::Imap )
  {
    mAccount->setName( mImap.ui.nameEdit->text() );
    mAccount->setCheckInterval( mImap.ui.intervalCheck->isChecked() ?
                                mImap.ui.intervalSpin->value() : 0 );
    mAccount->setIdentityId( mImap.identityCombo->currentIdentity() );
    mAccount->setUseDefaultIdentity( mImap.ui.useDefaultIdentityCheck->isChecked() );
    mAccount->setCheckExclude( !mImap.ui.includeInCheck->isChecked() );
    mAccount->setFolder( kmkernel->imapFolderMgr()->findById(mAccount->id()) );

    initAccountForConnect();
    KMAcctImap &epa = *(KMAcctImap*)mAccount;
    epa.setAutoExpunge( mImap.ui.autoExpungeCheck->isChecked() );
    epa.setHiddenFolders( mImap.ui.hiddenFoldersCheck->isChecked() );
    epa.setOnlySubscribedFolders( mImap.ui.subscribedFoldersCheck->isChecked() );
    epa.setOnlyLocallySubscribedFolders( mImap.ui.locallySubscribedFoldersCheck->isChecked() );
    epa.setLoadOnDemand( mImap.ui.loadOnDemandCheck->isChecked() );
    epa.setListOnlyOpenFolders( mImap.ui.listOnlyOpenCheck->isChecked() );
    KMFolder *t = mImap.trashCombo->folder();
    if ( t )
      epa.setTrash( mImap.trashCombo->folder()->idString() );
    else
      epa.setTrash( kmkernel->trashFolder()->idString() );
    epa.setCheckExclude( !mImap.ui.includeInCheck->isChecked() );
    if ( mSieveConfigEditor )
      epa.setSieveConfig( mSieveConfigEditor->config() );
  }
  else if( accountType == KAccount::DImap )
  {
    mAccount->setName( mImap.ui.nameEdit->text() );
    mAccount->setCheckInterval( mImap.ui.intervalCheck->isChecked() ?
                                mImap.ui.intervalSpin->value() : 0 );
    mAccount->setIdentityId( mImap.identityCombo->currentIdentity() );
    mAccount->setUseDefaultIdentity( mImap.ui.useDefaultIdentityCheck->isChecked() );
    mAccount->setCheckExclude( !mImap.ui.includeInCheck->isChecked() );
    //mAccount->setFolder( NULL );
    mAccount->setFolder( kmkernel->dimapFolderMgr()->findById(mAccount->id()) );
    //kDebug(5006) <<"account for folder" << mAccount->folder()->name();

    initAccountForConnect();
    KMAcctCachedImap &epa = *(KMAcctCachedImap*)mAccount;
    epa.setHiddenFolders( mImap.ui.hiddenFoldersCheck->isChecked() );
    epa.setOnlySubscribedFolders( mImap.ui.subscribedFoldersCheck->isChecked() );
    epa.setOnlyLocallySubscribedFolders( mImap.ui.locallySubscribedFoldersCheck->isChecked() );
    epa.setStorePasswd( mImap.ui.storePasswordCheck->isChecked() );
    epa.setPasswd( mImap.ui.passwordEdit->text(), epa.storePasswd() );
    KMFolder *t = mImap.trashCombo->folder();
    if ( t )
      epa.setTrash( mImap.trashCombo->folder()->idString() );
    else
      epa.setTrash( kmkernel->trashFolder()->idString() );
    epa.setCheckExclude( !mImap.ui.includeInCheck->isChecked() );
    if ( mSieveConfigEditor )
      epa.setSieveConfig( mSieveConfigEditor->config() );
  }
  else if( accountType == KAccount::Maildir )
  {
    KMAcctMaildir *acctMaildir = dynamic_cast<KMAcctMaildir*>(mAccount);

    if (acctMaildir) {
        mAccount->setName( mMaildir.ui.nameEdit->text() );
        acctMaildir->setLocation( mMaildir.ui.locationEdit->currentText() );

        KMFolder *targetFolder = mFolderList.at(mMaildir.ui.folderCombo->currentIndex());
        if ( targetFolder->location()  == acctMaildir->location() ) {
            /*
               Prevent data loss if the user sets the destination folder to be the same as the
               source account maildir folder by setting the target folder to the inbox.
               ### FIXME post 3.2: show dialog and let the user chose another target folder
            */
            targetFolder = kmkernel->inboxFolder();
        }
        mAccount->setFolder( targetFolder );
    }
    mAccount->setCheckInterval( mMaildir.ui.intervalCheck->isChecked() ?
                                mMaildir.ui.intervalSpin->value() : 0 );
    mAccount->setCheckExclude( !mMaildir.ui.includeInCheck->isChecked() );

    mAccount->setPrecommand( mMaildir.ui.precommand->text() );
  }

  if ( accountType == KAccount::Imap ||
       accountType == KAccount::DImap )
  {
    // settings for imap in general
    ImapAccountBase &ai = *(ImapAccountBase*)mAccount;
    // namespace
    ImapAccountBase::nsMap map;
    ImapAccountBase::namespaceDelim delimMap;
    ImapAccountBase::nsDelimMap::Iterator it;
    ImapAccountBase::namespaceDelim::Iterator it2;
    for ( it = mImap.nsMap.begin(); it != mImap.nsMap.end(); ++it ) {
      QStringList list;
      for ( it2 = it.value().begin(); it2 != it.value().end(); ++it2 ) {
        list << it2.key();
        delimMap[it2.key()] = it2.value();
      }
      map[it.key()] = list;
    }
    ai.setNamespaces( map );
    ai.setNamespaceToDelimiter( delimMap );
  }

  kmkernel->acctMgr()->writeConfig(true);
  // get the new account and register the new destination folder
  // this is the target folder for local or pop accounts and the root folder
  // of the account for (d)imap
  KMAccount* newAcct = kmkernel->acctMgr()->find(mAccount->id());
  if (newAcct)
  {
    if( accountType == KAccount::Local ) {
      newAcct->setFolder( mFolderList.at(mLocal.ui.folderCombo->currentIndex()), true );
    } else if ( accountType == KAccount::Pop ) {
      newAcct->setFolder( mFolderList.at(mPop.ui.folderCombo->currentIndex()), true );
    } else if ( accountType == KAccount::Maildir ) {
      newAcct->setFolder( mFolderList.at(mMaildir.ui.folderCombo->currentIndex()), true );
    } else if ( accountType == KAccount::Imap ) {
      newAcct->setFolder( kmkernel->imapFolderMgr()->findById(mAccount->id()), true );
    } else if ( accountType == KAccount::DImap ) {
      newAcct->setFolder( kmkernel->dimapFolderMgr()->findById(mAccount->id()), true );
    }
  }
}


void AccountDialog::slotLocationChooser()
{
  static QString directory( "/" );

  KFileDialog dialog( directory, QString(), this );
  dialog.setCaption( i18n("Choose Location") );

  bool result = dialog.exec();
  if( result == false )
  {
    return;
  }

  KUrl url = dialog.selectedUrl();
  if( url.isEmpty() )
  {
    return;
  }
  if( url.isLocalFile() == false )
  {
    KMessageBox::sorry( 0, i18n( "Only local files are currently supported." ) );
    return;
  }

  mLocal.ui.locationEdit->setEditText( url.path() );
  directory = url.directory();
}

void AccountDialog::slotMaildirChooser()
{
  static QString directory( "/" );

  QString dir = KFileDialog::getExistingDirectory(directory, this, i18n("Choose Location"));

  if( dir.isEmpty() )
    return;

  mMaildir.ui.locationEdit->setEditText( dir );
  directory = dir;
}

void AccountDialog::slotEnableLeaveOnServerDays( bool state )
{
  if ( state && !mPop.ui.leaveOnServerDaysCheck->isEnabled()) return;
  mPop.ui.leaveOnServerDaysSpin->setEnabled( state );
}

void AccountDialog::slotEnableLeaveOnServerCount( bool state )
{
  if ( state && !mPop.ui.leaveOnServerCountCheck->isEnabled()) return;
  mPop.ui.leaveOnServerCountSpin->setEnabled( state );
  return;
}

void AccountDialog::slotEnableLeaveOnServerSize( bool state )
{
  if ( state && !mPop.ui.leaveOnServerSizeCheck->isEnabled()) return;
  mPop.ui.leaveOnServerSizeSpin->setEnabled( state );
  return;
}

void AccountDialog::slotEnablePopInterval( bool state )
{
  mPop.ui.intervalSpin->setEnabled( state );
  mPop.ui.intervalLabel->setEnabled( state );
}

void AccountDialog::slotEnableImapInterval( bool state )
{
  mImap.ui.intervalSpin->setEnabled( state );
  mImap.ui.intervalLabel->setEnabled( state );
}

void AccountDialog::slotEnableLocalInterval( bool state )
{
  mLocal.ui.intervalSpin->setEnabled( state );
  mLocal.ui.intervalLabel->setEnabled( state );
}

void AccountDialog::slotEnableMaildirInterval( bool state )
{
  mMaildir.ui.intervalSpin->setEnabled( state );
  mMaildir.ui.intervalLabel->setEnabled( state );
}

void AccountDialog::slotFontChanged( void )
{
  KAccount::Type accountType = mAccount->type();
  if( accountType == KAccount::Local )
  {
    QFont titleFont( mLocal.ui.titleLabel->font() );
    titleFont.setBold( true );
    mLocal.ui.titleLabel->setFont(titleFont);
  }
  else if( accountType == KAccount::Pop )
  {
    QFont titleFont( mPop.ui.titleLabel->font() );
    titleFont.setBold( true );
    mPop.ui.titleLabel->setFont(titleFont);
  }
  else if( accountType == KAccount::Imap )
  {
    QFont titleFont( mImap.ui.titleLabel->font() );
    titleFont.setBold( true );
    mImap.ui.titleLabel->setFont(titleFont);
  }
}

void AccountDialog::slotReloadNamespaces()
{
  if ( mAccount->type() == KAccount::Imap ||
       mAccount->type() == KAccount::DImap )
  {
    initAccountForConnect();
    mImap.ui.personalNS->setText( i18n("Fetching Namespaces...") );
    mImap.ui.otherUsersNS->setText( QString() );
    mImap.ui.sharedNS->setText( QString() );
    ImapAccountBase* ai = static_cast<ImapAccountBase*>( mAccount );
    connect( ai, SIGNAL( namespacesFetched( const ImapAccountBase::nsDelimMap& ) ),
        this, SLOT( slotSetupNamespaces( const ImapAccountBase::nsDelimMap& ) ) );
    connect( ai, SIGNAL( connectionResult(int, const QString&) ),
          this, SLOT( slotConnectionResult(int, const QString&) ) );
    ai->getNamespaces();
  }
}

void AccountDialog::slotConnectionResult( int errorCode, const QString& )
{
  if ( errorCode > 0 ) {
    ImapAccountBase* ai = static_cast<ImapAccountBase*>( mAccount );
    disconnect( ai, SIGNAL( namespacesFetched( const ImapAccountBase::nsDelimMap& ) ),
        this, SLOT( slotSetupNamespaces( const ImapAccountBase::nsDelimMap& ) ) );
    disconnect( ai, SIGNAL( connectionResult(int, const QString&) ),
          this, SLOT( slotConnectionResult(int, const QString&) ) );
    mImap.ui.personalNS->setText( QString() );
  }
}

void AccountDialog::slotSetupNamespaces( const ImapAccountBase::nsDelimMap& map )
{
  disconnect( this, SLOT( slotSetupNamespaces( const ImapAccountBase::nsDelimMap& ) ) );
  mImap.ui.personalNS->setText( QString() );
  mImap.ui.otherUsersNS->setText( QString() );
  mImap.ui.sharedNS->setText( QString() );
  mImap.nsMap = map;

  ImapAccountBase::namespaceDelim ns = map[ImapAccountBase::PersonalNS];
  ImapAccountBase::namespaceDelim::ConstIterator it;
  if ( !ns.isEmpty() ) {
    mImap.ui.personalNS->setText( namespaceListToString( ns.keys() ) );
    mImap.ui.editPNS->setEnabled( true );
  } else {
    mImap.ui.editPNS->setEnabled( false );
  }
  ns = map[ImapAccountBase::OtherUsersNS];
  if ( !ns.isEmpty() ) {
    mImap.ui.otherUsersNS->setText( namespaceListToString( ns.keys() ) );
    mImap.ui.editONS->setEnabled( true );
  } else {
    mImap.ui.editONS->setEnabled( false );
  }
  ns = map[ImapAccountBase::SharedNS];
  if ( !ns.isEmpty() ) {
    mImap.ui.sharedNS->setText( namespaceListToString( ns.keys() ) );
    mImap.ui.editSNS->setEnabled( true );
  } else {
    mImap.ui.editSNS->setEnabled( false );
  }
}

const QString AccountDialog::namespaceListToString( const QStringList& list )
{
  QStringList myList = list;
  for ( QStringList::Iterator it = myList.begin(); it != myList.end(); ++it ) {
    if ( (*it).isEmpty() ) {
      (*it) = '<' + i18n("Empty") + '>';
    }
  }
  return myList.join(",");
}

void AccountDialog::initAccountForConnect()
{
  KAccount::Type type = mAccount->type();
  if ( type == KAccount::Local )
    return;

  NetworkAccount &na = *(NetworkAccount*)mAccount;

  if ( type == KAccount::Pop ) {
    na.setHost( mPop.ui.hostEdit->text().trimmed() );
    na.setPort( mPop.ui.portEdit->text().toInt() );
    na.setLogin( mPop.ui.loginEdit->text().trimmed() );
    na.setStorePasswd( mPop.ui.storePasswordCheck->isChecked() );
    na.setPasswd( mPop.ui.passwordEdit->text(), na.storePasswd() );
    na.setUseSSL( mPop.ui.encryptionSSL->isChecked() );
    na.setUseTLS( mPop.ui.encryptionTLS->isChecked() );
    if (mPop.ui.authUser->isChecked())
      na.setAuth("USER");
    else if (mPop.ui.authLogin->isChecked())
      na.setAuth("LOGIN");
    else if (mPop.ui.authPlain->isChecked())
      na.setAuth("PLAIN");
    else if (mPop.ui.authCRAM_MD5->isChecked())
      na.setAuth("CRAM-MD5");
    else if (mPop.ui.authDigestMd5->isChecked())
      na.setAuth("DIGEST-MD5");
    else if (mPop.ui.authNTLM->isChecked())
      na.setAuth("NTLM");
    else if (mPop.ui.authGSSAPI->isChecked())
      na.setAuth("GSSAPI");
    else if (mPop.ui.authAPOP->isChecked())
      na.setAuth("APOP");
    else na.setAuth("AUTO");
  }
  else if ( type == KAccount::Imap ||
            type == KAccount::DImap ) {
    na.setHost( mImap.ui.hostEdit->text().trimmed() );
    na.setPort( mImap.ui.portEdit->text().toInt() );
    na.setLogin( mImap.ui.loginEdit->text().trimmed() );
    na.setStorePasswd( mImap.ui.storePasswordCheck->isChecked() );
    na.setPasswd( mImap.ui.passwordEdit->text(), na.storePasswd() );
    na.setUseSSL( mImap.ui.encryptionSSL->isChecked() );
    na.setUseTLS( mImap.ui.encryptionTLS->isChecked() );
    if (mImap.ui.authCramMd5->isChecked())
      na.setAuth("CRAM-MD5");
    else if (mImap.ui.authDigestMd5->isChecked())
      na.setAuth("DIGEST-MD5");
    else if (mImap.ui.authNTLM->isChecked())
      na.setAuth("NTLM");
    else if (mImap.ui.authGSSAPI->isChecked())
      na.setAuth("GSSAPI");
    else if (mImap.ui.authAnonymous->isChecked())
      na.setAuth("ANONYMOUS");
    else if (mImap.ui.authLogin->isChecked())
      na.setAuth("LOGIN");
    else if (mImap.ui.authPlain->isChecked())
      na.setAuth("PLAIN");
    else na.setAuth("*");
  }
}

void AccountDialog::slotEditPersonalNamespace()
{
  NamespaceEditDialog dialog( this, ImapAccountBase::PersonalNS, &mImap.nsMap );
  if ( dialog.exec() == QDialog::Accepted ) {
    slotSetupNamespaces( mImap.nsMap );
  }
}

void AccountDialog::slotEditOtherUsersNamespace()
{
  NamespaceEditDialog dialog( this, ImapAccountBase::OtherUsersNS, &mImap.nsMap );
  if ( dialog.exec() == QDialog::Accepted ) {
    slotSetupNamespaces( mImap.nsMap );
  }
}

void AccountDialog::slotEditSharedNamespace()
{
  NamespaceEditDialog dialog( this, ImapAccountBase::SharedNS, &mImap.nsMap );
  if ( dialog.exec() == QDialog::Accepted ) {
    slotSetupNamespaces( mImap.nsMap );
  }
}

NamespaceLineEdit::NamespaceLineEdit( QWidget* parent )
  : KLineEdit( parent )
{
}

void NamespaceLineEdit::setText( const QString& text )
{
  mLastText = text;
  KLineEdit::setText( text );
}

NamespaceEditDialog::NamespaceEditDialog( QWidget *parent,
    ImapAccountBase::imapNamespace type, ImapAccountBase::nsDelimMap* map )
  : KDialog( parent ), mType( type ), mNamespaceMap( map )
{
  setButtons( Ok|Cancel );
  setObjectName( "edit_namespace" );
  setModal( false );
  QWidget *page = new QWidget( this );
  setMainWidget( page );

  QString ns;
  if ( mType == ImapAccountBase::PersonalNS ) {
    ns = i18n("Personal");
  } else if ( mType == ImapAccountBase::OtherUsersNS ) {
    ns = i18n("Other Users");
  } else {
    ns = i18n("Shared");
  }
  setCaption( i18n("Edit Namespace '%1'", ns) );
  QGridLayout *layout = new QGridLayout;

  mBg = new QButtonGroup( 0 );
  connect( mBg, SIGNAL( buttonClicked( int ) ), this, SLOT( slotRemoveEntry( int ) ) );
  connect( this, SIGNAL( okClicked() ), SLOT( slotOk() ) );
  mDelimMap = mNamespaceMap->find( mType ).value();
  ImapAccountBase::namespaceDelim::Iterator it;
  int row = 0;
  for ( it = mDelimMap.begin(); it != mDelimMap.end(); ++it ) {
    NamespaceLineEdit* edit = new NamespaceLineEdit( page );
    edit->setText( it.key() );
    QToolButton* button = new QToolButton( page );
    button->setIcon( KIcon("edit-delete") );
    button->setAutoRaise( true );
    button->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );
    button->setFixedSize( 22, 22 );
    mBg->addButton( button, row );
    mLineEditMap[ mBg->id( button ) ] = edit;

    layout->addWidget( edit, row, 0 );
    layout->addWidget( button, row, 1 );
    ++row;
  }

  page->setLayout( layout );
}

void NamespaceEditDialog::slotRemoveEntry( int id )
{
  if ( mLineEditMap.contains( id ) ) {
    // delete the lineedit and remove namespace from map
    NamespaceLineEdit* edit = mLineEditMap[id];
    mDelimMap.remove( edit->text() );
    if ( edit->isModified() ) {
      mDelimMap.remove( edit->lastText() );
    }
    mLineEditMap.remove( id );
    mainWidget()->layout()->removeWidget( edit );
    edit->close();
  }
  if ( mBg->button( id ) ) {
    mainWidget()->layout()->removeWidget( mBg->button( id ) );
    mBg->button( id )->close();
  }
  adjustSize();
}

void NamespaceEditDialog::slotOk()
{
  QMap<int, NamespaceLineEdit*>::Iterator it;
  for ( it = mLineEditMap.begin(); it != mLineEditMap.end(); ++it ) {
    NamespaceLineEdit* edit = it.value();
    if ( edit->isModified() ) {
      // register delimiter for new namespace
      mDelimMap[edit->text()] = mDelimMap[edit->lastText()];
      mDelimMap.remove( edit->lastText() );
    }
  }
  mNamespaceMap->insert( mType, mDelimMap );
}

} // namespace KMail

#include "accountdialog.moc"
