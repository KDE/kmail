#undef QT_NO_ASCII_CAST
/*
 *   kmail: KDE mail client  
 *   This file: Copyright (C) 2000 Espen Sand, espen@kde.org
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

#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qlayout.h>
#include <qtabwidget.h>
#include <qtextstream.h>
#include <qradiobutton.h>
#include <qvalidator.h>

#include <kapp.h>
#include <kconfig.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <knuminput.h>
#include <kseparator.h>

#include <netdb.h>
#include <netinet/in.h>

#include "accountdialog.h"
#include "kmacctlocal.h"
#include "kmacctmgr.h"
#include "kmacctexppop.h"
#include "kmacctimap.h"
#include "kmfolder.h"
#include "kmfoldermgr.h"
#include "kmglobal.h"

#include "accountdialog.moc"
#undef None

class ProcmailRCParser
{
public:
  ProcmailRCParser(QString fileName = QString::null);
  ~ProcmailRCParser();

  QStringList getLockFilesList() { return mLockFiles; }
  QStringList getSpoolFilesList() { return mSpoolFiles; }

protected:
  void processGlobalLock(const QString&);
  void processLocalLock(const QString&);
  void processVariableSetting(const QString&, int);
  QString expandVars(const QString&);

  QFile mProcmailrc;
  QTextStream *mStream;
  QStringList mLockFiles;
  QStringList mSpoolFiles;
  QAsciiDict<QString> mVars;
};

ProcmailRCParser::ProcmailRCParser(QString fname)
  : mProcmailrc(fname),
    mStream(new QTextStream(&mProcmailrc))
{
  mVars.setAutoDelete(true);

  if( !fname || fname.isEmpty() ) {
    fname = QDir::homeDirPath() + "/.procmailrc";
    mProcmailrc.setName(fname);
  }

  static QRegExp lockFileGlobal("LOCKFILE=", true),
    lockFileLocal("^:0", true);

  if(  mProcmailrc.open(IO_ReadOnly) ) {

    QString s;

    while( !mStream->eof() ) {

      s = mStream->readLine().stripWhiteSpace();

      if(  s[0] == '#' ) continue; // skip comments

      int commentPos = -1;

      if( (commentPos = s.find('#')) > -1 ) {
        // get rid of trailing comment
        s.truncate(commentPos);
        s = s.stripWhiteSpace();
      }

      if(  lockFileGlobal.match(s) != -1 ) {
        processGlobalLock(s);
      } else if( lockFileLocal.match(s) != -1 ) {
        processLocalLock(s);
      } else if( int i = s.find('=') ) {
        processVariableSetting(s,i);
      }
    }

  }
}

ProcmailRCParser::~ProcmailRCParser()
{
  delete mStream;
}

void
ProcmailRCParser::processGlobalLock(const QString &s)
{
  QString val = expandVars(s.mid(s.find('=') + 1).stripWhiteSpace());
  if ( !mLockFiles.contains(val) )
    mLockFiles << val;
}

void
ProcmailRCParser::processLocalLock(const QString &s)
{
  QString val;
  int colonPos = s.findRev(':');

  if (colonPos > 0) { // we don't care about the leading one
    val = s.mid(colonPos + 1).stripWhiteSpace();

    if ( val.length() ) {
      // user specified a lockfile, so process it
      //
      val = expandVars(val);
      if( val[0] != '/' && mVars.find("MAILDIR") )
        val.insert(0, *(mVars["MAILDIR"]) + '/');
    } // else we'll deduce the lockfile name one we
    // get the spoolfile name
  }

  // parse until we find the spoolfile
  QString line, prevLine;
  do {
    prevLine = line;
    line = mStream->readLine().stripWhiteSpace();
  } while ( !mStream->eof() && (line[0] == '*' ||
                                prevLine[prevLine.length() - 1] == '\\' ));

  if( line[0] != '!' && line[0] != '|' &&  line[0] != '{' ) {
    // this is a filename, expand it
    //
    line =  line.stripWhiteSpace();
    line = expandVars(line);

    // prepend default MAILDIR if needed
    if( line[0] != '/' && mVars.find("MAILDIR") )
      line.insert(0, *(mVars["MAILDIR"]) + '/');

    // now we have the spoolfile name
    if ( !mSpoolFiles.contains(line) )
      mSpoolFiles << line;

    if( colonPos > 0 && (!val || val.isEmpty()) ) {
      // there is a local lockfile, but the user didn't
      // specify the name so compute it from the spoolfile's name
      val = line;

      // append lock extension
      if( mVars.find("LOCKEXT") )
        val += *(mVars["LOCKEXT"]);
      else
        val += ".lock";
    }

    if ( val && !mLockFiles.contains(val) )
      mLockFiles << val;
  }

}

void
ProcmailRCParser::processVariableSetting(const QString &s, int eqPos)
{
  if( eqPos == -1) return;

  QString varName = s.left(eqPos),
    varValue = expandVars(s.mid(eqPos + 1).stripWhiteSpace());

  mVars.insert(varName.latin1(), new QString(varValue));
}

QString
ProcmailRCParser::expandVars(const QString &s)
{
  if( !s || s.isEmpty()) return s;

  QString expS = s;

  QAsciiDictIterator<QString> it( mVars ); // iterator for dict

  while ( it.current() ) {
    QString var = "\\$"; var += it.currentKey();

    expS.replace(QRegExp(var), *it.current());

    ++it;
  }

  return expS;
}



AccountDialog::AccountDialog( KMAccount *account, const QStringList &identity,
			      QWidget *parent, const char *name, bool modal )
  :KDialogBase( parent, name, modal, i18n("Configure Account"),
		Ok|Cancel|Help, Ok, true ), mAccount(account),
  mIdentityList( identity )
{
  QString accountType = mAccount->type();

  if( accountType == "local" )
  {
    makeLocalAccountPage();
  }
  else if( accountType == "pop" )
  {
    makePopAccountPage();
  }
  else if( accountType == "imap" )
  {
    makeImapAccountPage();
  }
  else
  {
    QString msg = i18n( "Account type is not supported" );
    KMessageBox::information( topLevelWidget(),msg,i18n("Configure Account") );
    return;
  }

  setupSettings();
}


void AccountDialog::makeLocalAccountPage()
{
  ProcmailRCParser procmailrcParser;

  QFrame *page = makeMainWidget();
  QGridLayout *topLayout = new QGridLayout( page, 11, 3, 0, spacingHint() );
  topLayout->addColSpacing( 1, fontMetrics().maxWidth()*15 );
  topLayout->setRowStretch( 10, 10 );
  topLayout->setColStretch( 1, 10 );

  mLocal.titleLabel = new QLabel( i18n("Account type: Local account"), page );
  topLayout->addMultiCellWidget( mLocal.titleLabel, 0, 0, 0, 2 );
  QFont titleFont( mLocal.titleLabel->font() );
  titleFont.setBold( true );
  mLocal.titleLabel->setFont( titleFont );
  KSeparator *hline = new KSeparator( KSeparator::HLine, page);
  topLayout->addMultiCellWidget( hline, 1, 1, 0, 2 );

  QLabel *label = new QLabel( i18n("&Name:"), page );
  topLayout->addWidget( label, 2, 0 );
  mLocal.nameEdit = new QLineEdit( page );
  label->setBuddy( mLocal.nameEdit );
  topLayout->addWidget( mLocal.nameEdit, 2, 1 );

  label = new QLabel( i18n("Location:"), page );
  topLayout->addWidget( label, 3, 0 );
  mLocal.locationEdit = new QComboBox( true, page );
  topLayout->addWidget( mLocal.locationEdit, 3, 1 );
  mLocal.locationEdit->insertStringList(procmailrcParser.getSpoolFilesList());

  QPushButton *choose = new QPushButton( i18n("&Choose..."), page );
  choose->setAutoDefault( false );
  connect( choose, SIGNAL(clicked()), this, SLOT(slotLocationChooser()) );
  topLayout->addWidget( choose, 3, 2 );

  QButtonGroup *group = new QButtonGroup(i18n("Locking method"), page );
  group->setColumnLayout(0, Qt::Horizontal);
  group->layout()->setSpacing( 0 );
  group->layout()->setMargin( 0 );
  QGridLayout *groupLayout = new QGridLayout( group->layout() );
  groupLayout->setAlignment( Qt::AlignTop );
  groupLayout->setSpacing( 6 );
  groupLayout->setMargin( 11 );

  mLocal.lockProcmail = new QRadioButton(
    i18n("Procmail lockfile"), group);
  groupLayout->addWidget(mLocal.lockProcmail, 0, 0);

  mLocal.procmailLockFileName = new QComboBox( true, group );
  groupLayout->addWidget(mLocal.procmailLockFileName, 0, 1);
  mLocal.procmailLockFileName->insertStringList(procmailrcParser.getLockFilesList());
  mLocal.procmailLockFileName->setEnabled(false);

  QObject::connect(mLocal.lockProcmail, SIGNAL(toggled(bool)),
                   mLocal.procmailLockFileName, SLOT(setEnabled(bool)));

  mLocal.lockMutt = new QRadioButton(
    i18n("Mutt dotlock"), group);
  groupLayout->addWidget(mLocal.lockMutt, 1, 0);

  mLocal.lockMuttPriv = new QRadioButton(
    i18n("&Mutt dotlock privileged"), group);
  groupLayout->addWidget(mLocal.lockMuttPriv, 2, 0);

  mLocal.lockFcntl = new QRadioButton(
    i18n("&FCNTL"), group);
  groupLayout->addWidget(mLocal.lockFcntl, 3, 0);

  mLocal.lockNone = new QRadioButton(
    i18n("&none (use with care)"), group);
  groupLayout->addWidget(mLocal.lockNone, 4, 0);

  topLayout->addMultiCellWidget( group, 4, 4, 0, 2 );

  mLocal.excludeCheck =
    new QCheckBox( i18n("E&xclude from \"Check Mail\""), page );
  topLayout->addMultiCellWidget( mLocal.excludeCheck, 5, 5, 0, 2 );

  mLocal.intervalCheck =
    new QCheckBox( i18n("&Enable interval mail checking"), page );
  topLayout->addMultiCellWidget( mLocal.intervalCheck, 6, 6, 0, 2 );
  connect( mLocal.intervalCheck, SIGNAL(toggled(bool)),
	   this, SLOT(slotEnableLocalInterval(bool)) );
  mLocal.intervalLabel = new QLabel( i18n("Check interval (minutes):"), page );
  topLayout->addWidget( mLocal.intervalLabel, 7, 0 );
  mLocal.intervalSpin = new KIntNumInput( page );
  mLocal.intervalSpin->setRange( 1, 10000, 1, FALSE );
  mLocal.intervalSpin->setValue( 1 );
  topLayout->addWidget( mLocal.intervalSpin, 7, 1 );

  label = new QLabel( i18n("Destination folder:"), page );
  topLayout->addWidget( label, 8, 0 );
  mLocal.folderCombo = new QComboBox( false, page );
  topLayout->addWidget( mLocal.folderCombo, 8, 1 );

  /* -sanders Probably won't support this way, use filters insteada
  label = new QLabel( i18n("Default identity:"), page );
  topLayout->addWidget( label, 9, 0 );
  mLocal.identityCombo = new QComboBox( false, page );
  topLayout->addWidget( mLocal.identityCombo, 9, 1 );
  // GS - this was moved inside the commented block 9/30/2000
  //      (I think Don missed it?)
  label->setEnabled(false);
  */

  //mLocal.identityCombo->setEnabled(false);

  label = new QLabel( i18n("&Precommand:"), page );
  topLayout->addWidget( label, 9, 0 );
  mLocal.precommand = new QLineEdit( page );
  label->setBuddy( mLocal.precommand );
  topLayout->addWidget( mLocal.precommand, 9, 1 );

  connect(kapp,SIGNAL(kdisplayFontChanged()),SLOT(slotFontChanged()));
}



void AccountDialog::makePopAccountPage()
{
  QFrame *page = makeMainWidget();
  QVBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );

  mPop.titleLabel = new QLabel( page );
  mPop.titleLabel->setText( i18n("Account type: Pop Account") );
  QFont titleFont( mPop.titleLabel->font() );
  titleFont.setBold( true );
  mPop.titleLabel->setFont( titleFont );
  topLayout->addWidget( mPop.titleLabel );
  KSeparator *hline = new KSeparator( KSeparator::HLine, page);
  topLayout->addWidget( hline );

  QTabWidget *tabWidget = new QTabWidget(page);
  topLayout->addWidget( tabWidget );

  QWidget *page1 = new QWidget( tabWidget );
  tabWidget->addTab( page1, i18n("&General") );

  QGridLayout *grid = new QGridLayout( page1, 13, 2, spacingHint() );
  grid->addColSpacing( 1, fontMetrics().maxWidth()*15 );
  grid->setRowStretch( 12, 10 );
  grid->setColStretch( 1, 10 );

  QLabel *label = new QLabel( i18n("&Name:"), page1 );
  grid->addWidget( label, 0, 0 );
  mPop.nameEdit = new QLineEdit( page1 );
  label->setBuddy( mPop.nameEdit );
  grid->addWidget( mPop.nameEdit, 0, 1 );

  label = new QLabel( i18n("&Login:"), page1 );
  grid->addWidget( label, 1, 0 );
  mPop.loginEdit = new QLineEdit( page1 );
  label->setBuddy( mPop.loginEdit );
  grid->addWidget( mPop.loginEdit, 1, 1 );

  label = new QLabel( i18n("P&assword:"), page1 );
  grid->addWidget( label, 2, 0 );
  mPop.passwordEdit = new QLineEdit( page1 );
  mPop.passwordEdit->setEchoMode( QLineEdit::Password );
  label->setBuddy( mPop.passwordEdit );
  grid->addWidget( mPop.passwordEdit, 2, 1 );

  label = new QLabel( i18n("&Host:"), page1 );
  grid->addWidget( label, 3, 0 );
  mPop.hostEdit = new QLineEdit( page1 );
  label->setBuddy( mPop.hostEdit );
  grid->addWidget( mPop.hostEdit, 3, 1 );

  label = new QLabel( i18n("&Port:"), page1 );
  grid->addWidget( label, 4, 0 );
  mPop.portEdit = new QLineEdit( page1 );
  mPop.portEdit->setValidator( new QIntValidator(this) );
  label->setBuddy( mPop.portEdit );
  grid->addWidget( mPop.portEdit, 4, 1 );

  mPop.storePasswordCheck =
    new QCheckBox( i18n("Store POP password in configuration file"), page1 );
  grid->addMultiCellWidget( mPop.storePasswordCheck, 5, 5, 0, 1 );

  mPop.deleteMailCheck =
    new QCheckBox( i18n("Delete mail from server"), page1 );
  grid->addMultiCellWidget( mPop.deleteMailCheck, 6, 6, 0, 1 );

  mPop.excludeCheck =
    new QCheckBox( i18n("Exclude from \"Check Mail\""), page1 );
  grid->addMultiCellWidget( mPop.excludeCheck, 7, 7, 0, 1 );

  mPop.intervalCheck =
    new QCheckBox( i18n("Enable interval mail checking"), page1 );
  grid->addMultiCellWidget( mPop.intervalCheck, 8, 8, 0, 1 );
  connect( mPop.intervalCheck, SIGNAL(toggled(bool)),
	   this, SLOT(slotEnablePopInterval(bool)) );
  mPop.intervalLabel = new QLabel( i18n("Check interval (minutes):"), page1 );
  grid->addWidget( mPop.intervalLabel, 9, 0 );
  mPop.intervalSpin = new KIntNumInput( page1 );
  mPop.intervalSpin->setRange( 1, 10000, 1, FALSE );
  mPop.intervalSpin->setValue( 1 );
  grid->addWidget( mPop.intervalSpin, 9, 1 );

  label = new QLabel( i18n("Destination folder:"), page1 );
  grid->addWidget( label, 10, 0 );
  mPop.folderCombo = new QComboBox( false, page1 );
  grid->addWidget( mPop.folderCombo, 10, 1 );

  label = new QLabel( i18n("Pre&command:"), page1 );
  grid->addWidget( label, 11, 0 );
  mPop.precommand = new QLineEdit( page1 );
  label->setBuddy(mPop.precommand);
  grid->addWidget( mPop.precommand, 11, 1 );

  QWidget *page2 = new QWidget( tabWidget );
  tabWidget->addTab( page2, i18n("S&ecurity") );
  QVBoxLayout *vlay = new QVBoxLayout( page2, spacingHint() );
  QButtonGroup *group = new QButtonGroup( 1, Qt::Horizontal,
    i18n("Encryption"), page2 );
  mPop.useSSLCheck =
    new QCheckBox( i18n("Use &SSL for secure mail download"), group );
  connect(mPop.useSSLCheck, SIGNAL(clicked()), this, SLOT(slotSSLChanged()));
  mPop.useTLSCheck =
    new QCheckBox( i18n("Use &TLS for secure mail download"), group );
  connect(mPop.useTLSCheck, SIGNAL(clicked()), this, SLOT(slotTLSChanged()));
  vlay->addWidget( group );

  group = new QButtonGroup( 1, Qt::Horizontal,
    i18n("Authentication method"), page2 );
  mPop.authAuto = new QRadioButton( i18n("Most secure method supported"),
    group );
  mPop.authUser = new QRadioButton( i18n("Clear text") , group );
  mPop.authPlain = new QRadioButton( i18n("Please translate this "
  "authentification method only, if you have a good reason", "PLAIN"), group );
  mPop.authCRAM_MD5 = new QRadioButton( i18n("CRAM-MD5"), group );
  mPop.authAPOP = new QRadioButton( i18n("APOP"), group );
  vlay->addWidget( group );
  vlay->addStretch();

  connect(kapp,SIGNAL(kdisplayFontChanged()),SLOT(slotFontChanged()));
}


void AccountDialog::makeImapAccountPage()
{
  QFrame *page = makeMainWidget();
  QVBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );

  mImap.titleLabel = new QLabel( page );
  mImap.titleLabel->setText( i18n("Account type: Imap Account") );
  QFont titleFont( mImap.titleLabel->font() );
  titleFont.setBold( true );
  mImap.titleLabel->setFont( titleFont );
  topLayout->addWidget( mImap.titleLabel );
  KSeparator *hline = new KSeparator( KSeparator::HLine, page);
  topLayout->addWidget( hline );

  QTabWidget *tabWidget = new QTabWidget(page);
  topLayout->addWidget( tabWidget );

  QWidget *page1 = new QWidget( tabWidget );
  tabWidget->addTab( page1, i18n("&General") );

  QGridLayout *grid = new QGridLayout( page1, 10, 2, spacingHint() );
  grid->addColSpacing( 1, fontMetrics().maxWidth()*15 );
  grid->setRowStretch( 10, 10 );
  grid->setColStretch( 1, 10 );

  QLabel *label = new QLabel( i18n("&Name:"), page1 );
  grid->addWidget( label, 0, 0 );
  mImap.nameEdit = new QLineEdit( page1 );
  label->setBuddy( mImap.nameEdit );
  grid->addWidget( mImap.nameEdit, 0, 1 );

  label = new QLabel( i18n("&Login:"), page1 );
  grid->addWidget( label, 1, 0 );
  mImap.loginEdit = new QLineEdit( page1 );
  label->setBuddy( mImap.loginEdit );
  grid->addWidget( mImap.loginEdit, 1, 1 );

  label = new QLabel( i18n("P&assword:"), page1 );
  grid->addWidget( label, 2, 0 );
  mImap.passwordEdit = new QLineEdit( page1 );
  mImap.passwordEdit->setEchoMode( QLineEdit::Password );
  label->setBuddy( mImap.passwordEdit );
  grid->addWidget( mImap.passwordEdit, 2, 1 );

  label = new QLabel( i18n("&Host:"), page1 );
  grid->addWidget( label, 3, 0 );
  mImap.hostEdit = new QLineEdit( page1 );
  label->setBuddy( mImap.hostEdit );
  grid->addWidget( mImap.hostEdit, 3, 1 );

  label = new QLabel( i18n("&Port:"), page1 );
  grid->addWidget( label, 4, 0 );
  mImap.portEdit = new QLineEdit( page1 );
  mImap.portEdit->setValidator( new QIntValidator(this) );
  label->setBuddy( mImap.portEdit );
  grid->addWidget( mImap.portEdit, 4, 1 );

  label = new QLabel( i18n("Pre&fix to folders:"), page1 );
  grid->addWidget( label, 5, 0 );
  mImap.prefixEdit = new QLineEdit( page1 );
  label->setBuddy( mImap.prefixEdit );
  grid->addWidget( mImap.prefixEdit, 5, 1 );

  mImap.autoExpungeCheck =
    new QCheckBox( i18n("&Automatically expunge deleted messages"), page1);
  grid->addMultiCellWidget( mImap.autoExpungeCheck, 6, 6, 0, 1 );

  mImap.hiddenFoldersCheck = new QCheckBox( i18n("&Show hidden folders"), page1);
  grid->addMultiCellWidget( mImap.hiddenFoldersCheck, 7, 7, 0, 1 );

  mImap.storePasswordCheck =
    new QCheckBox( i18n("Sto&re IMAP password in configuration file"), page1 );
  grid->addMultiCellWidget( mImap.storePasswordCheck, 7, 7, 0, 1 );

  QWidget *page2 = new QWidget( tabWidget );
  tabWidget->addTab( page2, i18n("S&ecurity") );
  QVBoxLayout *vlay = new QVBoxLayout( page2, spacingHint() );

  QButtonGroup *group = new QButtonGroup( 1, Qt::Horizontal,
    i18n("Encryption"), page2 );
  mImap.useSSLCheck =
    new QCheckBox( i18n("Use &SSL for secure mail download"), group );
  connect(mImap.useSSLCheck, SIGNAL(clicked()), this, SLOT(slotImapSSLChanged()));
  mImap.useTLSCheck =
    new QCheckBox( i18n("Use &TLS for secure mail download"), group );
  connect(mImap.useTLSCheck, SIGNAL(clicked()), this, SLOT(slotImapTLSChanged()));
  vlay->addWidget( group );

  group = new QButtonGroup( 1, Qt::Horizontal,
    i18n("Authentication method"), page2 );
  mImap.authAuto = new QRadioButton( i18n("Clear text"), group );
  mImap.authPlain = new QRadioButton( i18n("Please translate this "
  "authentification method only, if you have a good reason", "PLAIN"), group );
  mImap.authLogin = new QRadioButton( i18n("Please translate this "
  "authentification method only, if you have a good reason", "LOGIN"), group );
  mImap.authCramMd5 = new QRadioButton( i18n("CRAM-MD5"), group );
  mImap.authAnonymous = new QRadioButton( i18n("Anonymous"), group );
  vlay->addWidget( group );
  vlay->addStretch();

  connect(kapp,SIGNAL(kdisplayFontChanged()),SLOT(slotFontChanged()));
}


void AccountDialog::setupSettings()
{
  QComboBox *folderCombo = 0;
  int interval = mAccount->checkInterval();

  QString accountType = mAccount->type();
  if( accountType == "local" )
  {
    KMAcctLocal *acctLocal = dynamic_cast<KMAcctLocal*>(mAccount);

    mLocal.nameEdit->setText( mAccount->name() );
    mLocal.nameEdit->setFocus();
    mLocal.locationEdit->setEditText( acctLocal->location() );
    if (acctLocal->mLock == mutt_dotlock)
      mLocal.lockMutt->setChecked(true);
    else if (acctLocal->mLock == mutt_dotlock_privileged)
      mLocal.lockMuttPriv->setChecked(true);
    else if (acctLocal->mLock == procmail_lockfile) {
      mLocal.lockProcmail->setChecked(true);
      mLocal.procmailLockFileName->setEditText(acctLocal->procmailLockFileName());
    } else if (acctLocal->mLock == FCNTL)
      mLocal.lockFcntl->setChecked(true);
    else if (acctLocal->mLock == None)
      mLocal.lockNone->setChecked(true);

    mLocal.intervalSpin->setValue( QMAX(1, interval) );
    mLocal.intervalCheck->setChecked( interval >= 1 );
    mLocal.excludeCheck->setChecked( mAccount->checkExclude() );
    mLocal.precommand->setText( mAccount->precommand() );

    slotEnableLocalInterval( interval >= 1 );
    folderCombo = mLocal.folderCombo;
    //    mLocal.identityCombo->insertStringList( mIdentityList );
  }
  else if( accountType == "pop" )
  {
    KMAcctExpPop &ap = *(KMAcctExpPop*)mAccount;
    mPop.nameEdit->setText( mAccount->name() );
    mPop.nameEdit->setFocus();
    mPop.loginEdit->setText( ap.login() );
    mPop.passwordEdit->setText( ap.passwd());
    mPop.hostEdit->setText( ap.host() );
    mPop.portEdit->setText( QString("%1").arg( ap.port() ) );
    mPop.useSSLCheck->setChecked( ap.useSSL() );
    mPop.useTLSCheck->setChecked( ap.useTLS() );
    mPop.storePasswordCheck->setChecked( ap.storePasswd() );
    mPop.deleteMailCheck->setChecked( !ap.leaveOnServer() );
    mPop.intervalCheck->setChecked( interval >= 1 );
    mPop.intervalSpin->setValue( QMAX(1, interval) );
    mPop.excludeCheck->setChecked( mAccount->checkExclude() );
    mPop.precommand->setText( ap.precommand() );
    if (ap.auth() == "USER")
      mPop.authUser->setChecked( TRUE );
    else if (ap.auth() == "PLAIN")
      mPop.authPlain->setChecked( TRUE );
    else if (ap.auth() == "CRAM-MD5")
      mPop.authCRAM_MD5->setChecked( TRUE );
    else if (ap.auth() == "APOP")
      mPop.authAPOP->setChecked( TRUE );
    else mPop.authAuto->setChecked( TRUE );

    slotEnablePopInterval( interval >= 1 );
    folderCombo = mPop.folderCombo;
    //    mPop.identityCombo->insertStringList( mIdentityList );
  }
  else if( accountType == "imap" )
  {
    KMAcctImap &ai = *(KMAcctImap*)mAccount;
    mImap.nameEdit->setText( mAccount->name() );
    mImap.nameEdit->setFocus();
    mImap.loginEdit->setText( ai.login() );
    mImap.passwordEdit->setText( ai.passwd());
    mImap.hostEdit->setText( ai.host() );
    mImap.portEdit->setText( QString("%1").arg( ai.port() ) );
    mImap.prefixEdit->setText( ai.prefix() );
    mImap.autoExpungeCheck->setChecked( ai.autoExpunge() );
    mImap.hiddenFoldersCheck->setChecked( ai.hiddenFolders() );
    mImap.storePasswordCheck->setChecked( ai.storePasswd() );
    mImap.useSSLCheck->setChecked( ai.useSSL() );
    mImap.useTLSCheck->setChecked( ai.useTLS() );
    if (ai.auth() == "CRAM-MD5")
      mImap.authCramMd5->setChecked( TRUE );
    else if (ai.auth() == "ANONYMOUS")
      mImap.authAnonymous->setChecked( TRUE );
    else if (ai.auth() == "PLAIN")
      mImap.authPlain->setChecked( TRUE );
    else if (ai.auth() == "LOGIN")
      mImap.authLogin->setChecked( TRUE );
    else mImap.authAuto->setChecked( TRUE );
  }
  else // Unknown account type
    return;

  if (!folderCombo) return;

  KMFolderDir *fdir = (KMFolderDir*)&kernel->folderMgr()->dir();
  KMFolder *acctFolder = mAccount->folder();
  if( acctFolder == 0 )
  {
    acctFolder = (KMFolder*)fdir->first();
  }
  if( acctFolder == 0 )
  {
    folderCombo->insertItem( i18n("<none>") );
  }
  else
  {
    uint i = 0;
    int curIndex = -1;
    kernel->folderMgr()->createI18nFolderList(&mFolderNames, &mFolderList);
    while (i < mFolderNames.count())
    {
      QValueList<QGuardedPtr<KMFolder> >::Iterator it = mFolderList.at(i);
      KMFolder *folder = *it;
      if (folder->isSystemFolder())
      {
        mFolderList.remove(it);
        mFolderNames.remove(mFolderNames.at(i));
      } else {
        if (folder == acctFolder) curIndex = i;
        i++;
      }
    }
    mFolderNames.prepend(i18n("inbox"));
    mFolderList.prepend(kernel->inboxFolder());
    folderCombo->insertStringList(mFolderNames);
    folderCombo->setCurrentItem(curIndex + 1);

    // -sanders hack for startup users. Must investigate this properly
    if (folderCombo->count() == 0)
      folderCombo->insertItem( i18n("inbox") );
  }
}


void AccountDialog::slotImapSSLChanged()
{
  if (mImap.useSSLCheck->isChecked()) {
    struct servent *serv = getservbyname("imaps", "tcp");
    if (serv) {
      QString x;
      x.sprintf("%u", ntohs(serv->s_port));
      mImap.portEdit->setText(x);
    } else {
      mImap.portEdit->setText("993");
    }
    mImap.useTLSCheck->setChecked( FALSE );
  } else {
    struct servent *serv = getservbyname("imap", "tcp");
    if (serv) {
      QString x;
      x.sprintf("%u", ntohs(serv->s_port));
      mImap.portEdit->setText(x);
    } else {
      mImap.portEdit->setText("110");
    }
  }
}


void AccountDialog::slotImapTLSChanged()
{
  mImap.useSSLCheck->setChecked( FALSE );
  slotImapSSLChanged();
}


void AccountDialog::slotSSLChanged()
{
  if (mPop.useSSLCheck->isChecked()) {
    struct servent *serv = getservbyname("pop3s", "tcp");
    if (serv) {
      QString x;
      x.sprintf("%u", ntohs(serv->s_port));
      mPop.portEdit->setText(x);
    } else {
      mPop.portEdit->setText("995");
    }
    mPop.useTLSCheck->setChecked( FALSE );
  } else {
    struct servent *serv = getservbyname("pop-3", "tcp");
    if (serv) {
      QString x;
      x.sprintf("%u", ntohs(serv->s_port));
      mPop.portEdit->setText(x);
    } else {
      mPop.portEdit->setText("110");
    }
  }
}


void AccountDialog::slotTLSChanged()
{
  mPop.useSSLCheck->setChecked( FALSE );
  slotSSLChanged();
}


void AccountDialog::slotOk()
{
  saveSettings();
  accept();
}


void AccountDialog::saveSettings()
{
  QString accountType = mAccount->type();
  if( accountType == "local" )
  {
    KMAcctLocal *acctLocal = dynamic_cast<KMAcctLocal*>(mAccount);

    if (acctLocal) {
      mAccount->setName( mLocal.nameEdit->text() );
      acctLocal->setLocation( mLocal.locationEdit->currentText() );
      if (mLocal.lockMutt->isChecked())
        acctLocal->setLockType(mutt_dotlock);
      else if (mLocal.lockMuttPriv->isChecked())
        acctLocal->setLockType(mutt_dotlock_privileged);
      else if (mLocal.lockProcmail->isChecked()) {
        acctLocal->setLockType(procmail_lockfile);
        acctLocal->setProcmailLockFileName(mLocal.procmailLockFileName->currentText());
      }
      else if (mLocal.lockNone->isChecked())
        acctLocal->setLockType(None);
      else acctLocal->setLockType(FCNTL);
    }

    mAccount->setCheckInterval( mLocal.intervalCheck->isChecked() ?
			     mLocal.intervalSpin->value() : 0 );
    mAccount->setCheckExclude( mLocal.excludeCheck->isChecked() );

    mAccount->setPrecommand( mLocal.precommand->text() );

    mAccount->setFolder( *mFolderList.at(mLocal.folderCombo->currentItem()) );

  }
  else if( accountType == "pop" )
  {
    mAccount->setName( mPop.nameEdit->text() );
    mAccount->setCheckInterval( mPop.intervalCheck->isChecked() ?
			     mPop.intervalSpin->value() : 0 );
    mAccount->setCheckExclude( mPop.excludeCheck->isChecked() );

    mAccount->setFolder( *mFolderList.at(mPop.folderCombo->currentItem()) );

    KMAcctExpPop &epa = *(KMAcctExpPop*)mAccount;
    epa.setHost( mPop.hostEdit->text() );
    epa.setPort( mPop.portEdit->text().toInt() );
    epa.setLogin( mPop.loginEdit->text() );
    epa.setPasswd( mPop.passwordEdit->text(), true );
    epa.setUseSSL( mPop.useSSLCheck->isChecked() );
    epa.setUseTLS( mPop.useTLSCheck->isChecked() );
    epa.setStorePasswd( mPop.storePasswordCheck->isChecked() );
    epa.setPasswd( mPop.passwordEdit->text(), epa.storePasswd() );
    epa.setLeaveOnServer( !mPop.deleteMailCheck->isChecked() );
    epa.setPrecommand( mPop.precommand->text() );
    if (mPop.authUser->isChecked())
      epa.setAuth("USER");
    else if (mPop.authPlain->isChecked())
      epa.setAuth("PLAIN");
    else if (mPop.authCRAM_MD5->isChecked())
      epa.setAuth("CRAM-MD5");
    else if (mPop.authAPOP->isChecked())
      epa.setAuth("APOP");
    else epa.setAuth("AUTO");
  }
  else if( accountType == "imap" )
  {
    mAccount->setName( mImap.nameEdit->text() );
    mAccount->setCheckInterval( 0 );
    mAccount->setCheckExclude( TRUE );

    KMAcctImap &epa = *(KMAcctImap*)mAccount;
    epa.setHost( mImap.hostEdit->text() );
    epa.setPort( mImap.portEdit->text().toInt() );
    epa.setPrefix( mImap.prefixEdit->text() );
    epa.setLogin( mImap.loginEdit->text() );
    epa.setAutoExpunge( mImap.autoExpungeCheck->isChecked() );
    epa.setHiddenFolders( mImap.hiddenFoldersCheck->isChecked() );
    epa.setUseSSL( mImap.useSSLCheck->isChecked() );
    epa.setUseTLS( mImap.useTLSCheck->isChecked() );
    epa.setStorePasswd( mImap.storePasswordCheck->isChecked() );
    epa.setPasswd( mImap.passwordEdit->text(), epa.storePasswd() );

    if (mImap.authCramMd5->isChecked())
      epa.setAuth("CRAM-MD5");
    else if (mImap.authAnonymous->isChecked())
      epa.setAuth("ANONYMOUS");
    else if (mImap.authPlain->isChecked())
      epa.setAuth("PLAIN");
    else if (mImap.authLogin->isChecked())
      epa.setAuth("LOGIN");
    else epa.setAuth("*");
  }
  kernel->acctMgr()->writeConfig(TRUE);
}


void AccountDialog::slotLocationChooser()
{
  static QString directory( "/" );

  KFileDialog dialog( directory, QString::null, this, 0, true );
  dialog.setCaption( i18n("Choose Location") );

  bool result = dialog.exec();
  if( result == false )
  {
    return;
  }

  KURL url = dialog.selectedURL();
  if( url.isEmpty() )
  {
    return;
  }
  if( url.isLocalFile() == false )
  {
    KMessageBox::sorry( 0L, i18n( "Only local files supported yet." ) );
    return;
  }

  mLocal.locationEdit->setEditText( url.path() );
  directory = url.directory();
}



void AccountDialog::slotEnablePopInterval( bool state )
{
  mPop.intervalSpin->setEnabled( state );
  mPop.intervalLabel->setEnabled( state );
}


void AccountDialog::slotEnableLocalInterval( bool state )
{
  mLocal.intervalSpin->setEnabled( state );
  mLocal.intervalLabel->setEnabled( state );
}


void AccountDialog::slotFontChanged( void )
{
  QString accountType = mAccount->type();
  if( accountType == "local" )
  {
    QFont titleFont( mLocal.titleLabel->font() );
    titleFont.setBold( true );
    mLocal.titleLabel->setFont(titleFont);
  }
  else if( accountType == "pop" )
  {
    QFont titleFont( mPop.titleLabel->font() );
    titleFont.setBold( true );
    mPop.titleLabel->setFont(titleFont);
  }
  else if( accountType == "imap" )
  {
    QFont titleFont( mImap.titleLabel->font() );
    titleFont.setBold( true );
    mImap.titleLabel->setFont(titleFont);
  }
}


