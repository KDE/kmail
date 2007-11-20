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
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#include <config.h>

#include "accountdialog.h"

#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qtabwidget.h>
#include <qradiobutton.h>
#include <qvalidator.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qwhatsthis.h>
#include <qhbox.h>
#include <qcombobox.h>
#include <qheader.h>
#include <qtoolbutton.h>
#include <qgrid.h>

#include <kfiledialog.h>
#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <knuminput.h>
#include <kseparator.h>
#include <kapplication.h>
#include <kmessagebox.h>
#include <kprotocolinfo.h>
#include <kiconloader.h>
#include <kpopupmenu.h>

#include <netdb.h>
#include <netinet/in.h>

#include "sieveconfig.h"
#include "kmacctmaildir.h"
#include "kmacctlocal.h"
#include "accountmanager.h"
#include "popaccount.h"
#include "kmacctimap.h"
#include "kmacctcachedimap.h"
#include "kmfoldermgr.h"
#include "kmservertest.h"
#include "protocols.h"
#include "folderrequester.h"
#include "kmmainwidget.h"
#include "kmfolder.h"
#include <libkpimidentities/identitymanager.h>
#include <libkpimidentities/identitycombo.h>
#include <libkpimidentities/identity.h>
#include "globalsettings.h"

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
  ProcmailRCParser(QString fileName = QString::null);
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
  QAsciiDict<QString> mVars;
};

ProcmailRCParser::ProcmailRCParser(QString fname)
  : mProcmailrc(fname),
    mStream(new QTextStream(&mProcmailrc))
{
  mVars.setAutoDelete(true);

  // predefined
  mVars.insert( "HOME", new QString( QDir::homeDirPath() ) );

  if( !fname || fname.isEmpty() ) {
    fname = QDir::homeDirPath() + "/.procmailrc";
    mProcmailrc.setName(fname);
  }

  QRegExp lockFileGlobal("^LOCKFILE=", true);
  QRegExp lockFileLocal("^:0", true);

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

      if(  lockFileGlobal.search(s) != -1 ) {
        processGlobalLock(s);
      } else if( lockFileLocal.search(s) != -1 ) {
        processLocalLock(s);
      } else if( int i = s.find('=') ) {
        processVariableSetting(s,i);
      }
    }

  }
  QString default_Location = getenv("MAIL");

  if (default_Location.isNull()) {
    default_Location = _PATH_MAILDIR;
    default_Location += '/';
    default_Location += getenv("USER");
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
    varValue = expandVars(s.mid(eqPos + 1).stripWhiteSpace());

  mVars.insert(varName.latin1(), new QString(varValue));
}

QString
ProcmailRCParser::expandVars(const QString &s)
{
  if( s.isEmpty()) return s;

  QString expS = s;

  QAsciiDictIterator<QString> it( mVars ); // iterator for dict

  while ( it.current() ) {
    expS.replace(QString::fromLatin1("$") + it.currentKey(), *it.current());
    ++it;
  }

  return expS;
}



AccountDialog::AccountDialog( const QString & caption, KMAccount *account,
			      QWidget *parent, const char *name, bool modal )
  : KDialogBase( parent, name, modal, caption, Ok|Cancel|Help, Ok, true ),
    mAccount( account ),
    mServerTest( 0 ),
    mCurCapa( AllCapa ),
    mCapaNormal( AllCapa ),
    mCapaSSL( AllCapa ),
    mCapaTLS( AllCapa ),
    mSieveConfigEditor( 0 )
{
  mValidator = new QRegExpValidator( QRegExp( "[A-Za-z0-9-_:.]*" ), 0 );
  setHelp("receiving-mail");

  QString accountType = mAccount->type();

  if( accountType == "local" )
  {
    makeLocalAccountPage();
  }
  else if( accountType == "maildir" )
  {
    makeMaildirAccountPage();
  }
  else if( accountType == "pop" )
  {
    makePopAccountPage();
  }
  else if( accountType == "imap" )
  {
    makeImapAccountPage();
  }
  else if( accountType == "cachedimap" )
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
  ProcmailRCParser procmailrcParser;
  QFrame *page = makeMainWidget();
  QGridLayout *topLayout = new QGridLayout( page, 12, 3, 0, spacingHint() );
  topLayout->addColSpacing( 1, fontMetrics().maxWidth()*15 );
  topLayout->setRowStretch( 11, 10 );
  topLayout->setColStretch( 1, 10 );

  mLocal.titleLabel = new QLabel( i18n("Account Type: Local Account"), page );
  topLayout->addMultiCellWidget( mLocal.titleLabel, 0, 0, 0, 2 );
  QFont titleFont( mLocal.titleLabel->font() );
  titleFont.setBold( true );
  mLocal.titleLabel->setFont( titleFont );
  KSeparator *hline = new KSeparator( KSeparator::HLine, page);
  topLayout->addMultiCellWidget( hline, 1, 1, 0, 2 );

  QLabel *label = new QLabel( i18n("Account &name:"), page );
  topLayout->addWidget( label, 2, 0 );
  mLocal.nameEdit = new KLineEdit( page );
  label->setBuddy( mLocal.nameEdit );
  topLayout->addWidget( mLocal.nameEdit, 2, 1 );

  label = new QLabel( i18n("File &location:"), page );
  topLayout->addWidget( label, 3, 0 );
  mLocal.locationEdit = new QComboBox( true, page );
  label->setBuddy( mLocal.locationEdit );
  topLayout->addWidget( mLocal.locationEdit, 3, 1 );
  mLocal.locationEdit->insertStringList(procmailrcParser.getSpoolFilesList());

  QPushButton *choose = new QPushButton( i18n("Choo&se..."), page );
  choose->setAutoDefault( false );
  connect( choose, SIGNAL(clicked()), this, SLOT(slotLocationChooser()) );
  topLayout->addWidget( choose, 3, 2 );

  QButtonGroup *group = new QButtonGroup(i18n("Locking Method"), page );
  group->setColumnLayout(0, Qt::Horizontal);
  group->layout()->setSpacing( 0 );
  group->layout()->setMargin( 0 );
  QGridLayout *groupLayout = new QGridLayout( group->layout() );
  groupLayout->setAlignment( Qt::AlignTop );
  groupLayout->setSpacing( 6 );
  groupLayout->setMargin( 11 );

  mLocal.lockProcmail = new QRadioButton( i18n("Procmail loc&kfile:"), group);
  groupLayout->addWidget(mLocal.lockProcmail, 0, 0);

  mLocal.procmailLockFileName = new QComboBox( true, group );
  groupLayout->addWidget(mLocal.procmailLockFileName, 0, 1);
  mLocal.procmailLockFileName->insertStringList(procmailrcParser.getLockFilesList());
  mLocal.procmailLockFileName->setEnabled(false);

  QObject::connect(mLocal.lockProcmail, SIGNAL(toggled(bool)),
                   mLocal.procmailLockFileName, SLOT(setEnabled(bool)));

  mLocal.lockMutt = new QRadioButton(
    i18n("&Mutt dotlock"), group);
  groupLayout->addWidget(mLocal.lockMutt, 1, 0);

  mLocal.lockMuttPriv = new QRadioButton(
    i18n("M&utt dotlock privileged"), group);
  groupLayout->addWidget(mLocal.lockMuttPriv, 2, 0);

  mLocal.lockFcntl = new QRadioButton(
    i18n("&FCNTL"), group);
  groupLayout->addWidget(mLocal.lockFcntl, 3, 0);

  mLocal.lockNone = new QRadioButton(
    i18n("Non&e (use with care)"), group);
  groupLayout->addWidget(mLocal.lockNone, 4, 0);

  topLayout->addMultiCellWidget( group, 4, 4, 0, 2 );

#if 0
  QHBox* resourceHB = new QHBox( page );
  resourceHB->setSpacing( 11 );
  mLocal.resourceCheck =
      new QCheckBox( i18n( "Account for semiautomatic resource handling" ), resourceHB );
  mLocal.resourceClearButton =
      new QPushButton( i18n( "Clear" ), resourceHB );
  QWhatsThis::add( mLocal.resourceClearButton,
                   i18n( "Delete all allocations for the resource represented by this account." ) );
  mLocal.resourceClearButton->setEnabled( false );
  connect( mLocal.resourceCheck, SIGNAL( toggled(bool) ),
           mLocal.resourceClearButton, SLOT( setEnabled(bool) ) );
  connect( mLocal.resourceClearButton, SIGNAL( clicked() ),
           this, SLOT( slotClearResourceAllocations() ) );
  mLocal.resourceClearPastButton =
      new QPushButton( i18n( "Clear Past" ), resourceHB );
  mLocal.resourceClearPastButton->setEnabled( false );
  connect( mLocal.resourceCheck, SIGNAL( toggled(bool) ),
           mLocal.resourceClearPastButton, SLOT( setEnabled(bool) ) );
  QWhatsThis::add( mLocal.resourceClearPastButton,
                   i18n( "Delete all outdated allocations for the resource represented by this account." ) );
  connect( mLocal.resourceClearPastButton, SIGNAL( clicked() ),
           this, SLOT( slotClearPastResourceAllocations() ) );
  topLayout->addMultiCellWidget( resourceHB, 5, 5, 0, 2 );
#endif

  mLocal.includeInCheck =
    new QCheckBox( i18n("Include in m&anual mail check"),
                   page );
  topLayout->addMultiCellWidget( mLocal.includeInCheck, 5, 5, 0, 2 );

  mLocal.intervalCheck =
    new QCheckBox( i18n("Enable &interval mail checking"), page );
  topLayout->addMultiCellWidget( mLocal.intervalCheck, 6, 6, 0, 2 );
  connect( mLocal.intervalCheck, SIGNAL(toggled(bool)),
	   this, SLOT(slotEnableLocalInterval(bool)) );
  mLocal.intervalLabel = new QLabel( i18n("Check inter&val:"), page );
  topLayout->addWidget( mLocal.intervalLabel, 7, 0 );
  mLocal.intervalSpin = new KIntNumInput( page );
  mLocal.intervalLabel->setBuddy( mLocal.intervalSpin );
  mLocal.intervalSpin->setRange( GlobalSettings::self()->minimumCheckInterval(), 10000, 1, false );
  mLocal.intervalSpin->setSuffix( i18n(" min") );
  mLocal.intervalSpin->setValue( defaultmailcheckintervalmin );
  topLayout->addWidget( mLocal.intervalSpin, 7, 1 );

  label = new QLabel( i18n("&Destination folder:"), page );
  topLayout->addWidget( label, 8, 0 );
  mLocal.folderCombo = new QComboBox( false, page );
  label->setBuddy( mLocal.folderCombo );
  topLayout->addWidget( mLocal.folderCombo, 8, 1 );

  label = new QLabel( i18n("&Pre-command:"), page );
  topLayout->addWidget( label, 9, 0 );
  mLocal.precommand = new KLineEdit( page );
  label->setBuddy( mLocal.precommand );
  topLayout->addWidget( mLocal.precommand, 9, 1 );

  mLocal.identityLabel = new QLabel( i18n("Identity:"), page );
  topLayout->addWidget( mLocal.identityLabel, 10, 0 );
  mLocal.identityCombo = new KPIM::IdentityCombo(kmkernel->identityManager(), page );
  mLocal.identityLabel->setBuddy( mLocal.identityCombo );
  topLayout->addWidget( mLocal.identityCombo, 10, 1 );

  connect(kapp,SIGNAL(kdisplayFontChanged()),SLOT(slotFontChanged()));
}

void AccountDialog::makeMaildirAccountPage()
{
  ProcmailRCParser procmailrcParser;

  QFrame *page = makeMainWidget();
  QGridLayout *topLayout = new QGridLayout( page, 11, 3, 0, spacingHint() );
  topLayout->addColSpacing( 1, fontMetrics().maxWidth()*15 );
  topLayout->setRowStretch( 11, 10 );
  topLayout->setColStretch( 1, 10 );

  mMaildir.titleLabel = new QLabel( i18n("Account Type: Maildir Account"), page );
  topLayout->addMultiCellWidget( mMaildir.titleLabel, 0, 0, 0, 2 );
  QFont titleFont( mMaildir.titleLabel->font() );
  titleFont.setBold( true );
  mMaildir.titleLabel->setFont( titleFont );
  QFrame *hline = new QFrame( page );
  hline->setFrameStyle( QFrame::Sunken | QFrame::HLine );
  topLayout->addMultiCellWidget( hline, 1, 1, 0, 2 );

  mMaildir.nameEdit = new KLineEdit( page );
  topLayout->addWidget( mMaildir.nameEdit, 2, 1 );
  QLabel *label = new QLabel( mMaildir.nameEdit, i18n("Account &name:"), page );
  topLayout->addWidget( label, 2, 0 );

  mMaildir.locationEdit = new QComboBox( true, page );
  topLayout->addWidget( mMaildir.locationEdit, 3, 1 );
  mMaildir.locationEdit->insertStringList(procmailrcParser.getSpoolFilesList());
  label = new QLabel( mMaildir.locationEdit, i18n("Folder &location:"), page );
  topLayout->addWidget( label, 3, 0 );

  QPushButton *choose = new QPushButton( i18n("Choo&se..."), page );
  choose->setAutoDefault( false );
  connect( choose, SIGNAL(clicked()), this, SLOT(slotMaildirChooser()) );
  topLayout->addWidget( choose, 3, 2 );

#if 0
  QHBox* resourceHB = new QHBox( page );
  resourceHB->setSpacing( 11 );
  mMaildir.resourceCheck =
      new QCheckBox( i18n( "Account for semiautomatic resource handling" ), resourceHB );
  mMaildir.resourceClearButton =
      new QPushButton( i18n( "Clear" ), resourceHB );
  mMaildir.resourceClearButton->setEnabled( false );
  connect( mMaildir.resourceCheck, SIGNAL( toggled(bool) ),
           mMaildir.resourceClearButton, SLOT( setEnabled(bool) ) );
  QWhatsThis::add( mMaildir.resourceClearButton,
                   i18n( "Delete all allocations for the resource represented by this account." ) );
  connect( mMaildir.resourceClearButton, SIGNAL( clicked() ),
           this, SLOT( slotClearResourceAllocations() ) );
  mMaildir.resourceClearPastButton =
      new QPushButton( i18n( "Clear Past" ), resourceHB );
  mMaildir.resourceClearPastButton->setEnabled( false );
  connect( mMaildir.resourceCheck, SIGNAL( toggled(bool) ),
           mMaildir.resourceClearPastButton, SLOT( setEnabled(bool) ) );
  QWhatsThis::add( mMaildir.resourceClearPastButton,
                   i18n( "Delete all outdated allocations for the resource represented by this account." ) );
  connect( mMaildir.resourceClearPastButton, SIGNAL( clicked() ),
           this, SLOT( slotClearPastResourceAllocations() ) );
  topLayout->addMultiCellWidget( resourceHB, 4, 4, 0, 2 );
#endif

  mMaildir.includeInCheck =
    new QCheckBox( i18n("Include in &manual mail check"), page );
  topLayout->addMultiCellWidget( mMaildir.includeInCheck, 4, 4, 0, 2 );

  mMaildir.intervalCheck =
    new QCheckBox( i18n("Enable &interval mail checking"), page );
  topLayout->addMultiCellWidget( mMaildir.intervalCheck, 5, 5, 0, 2 );
  connect( mMaildir.intervalCheck, SIGNAL(toggled(bool)),
	   this, SLOT(slotEnableMaildirInterval(bool)) );
  mMaildir.intervalLabel = new QLabel( i18n("Check inter&val:"), page );
  topLayout->addWidget( mMaildir.intervalLabel, 6, 0 );
  mMaildir.intervalSpin = new KIntNumInput( page );
  mMaildir.intervalSpin->setRange( GlobalSettings::self()->minimumCheckInterval(), 10000, 1, false );
  mMaildir.intervalSpin->setSuffix( i18n(" min") );
  mMaildir.intervalSpin->setValue( defaultmailcheckintervalmin );
  mMaildir.intervalLabel->setBuddy( mMaildir.intervalSpin );
  topLayout->addWidget( mMaildir.intervalSpin, 6, 1 );

  mMaildir.folderCombo = new QComboBox( false, page );
  topLayout->addWidget( mMaildir.folderCombo, 7, 1 );
  label = new QLabel( mMaildir.folderCombo,
		      i18n("&Destination folder:"), page );
  topLayout->addWidget( label, 7, 0 );

  mMaildir.precommand = new KLineEdit( page );
  topLayout->addWidget( mMaildir.precommand, 8, 1 );
  label = new QLabel( mMaildir.precommand, i18n("&Pre-command:"), page );
  topLayout->addWidget( label, 8, 0 );


  mMaildir.identityLabel = new QLabel( i18n("Identity:"), page );
  topLayout->addWidget( mMaildir.identityLabel, 9, 0 );
  mMaildir.identityCombo = new KPIM::IdentityCombo(kmkernel->identityManager(), page );
  mMaildir.identityLabel->setBuddy( mMaildir.identityCombo );
  topLayout->addWidget( mMaildir.identityCombo, 9, 1 );

  connect(kapp,SIGNAL(kdisplayFontChanged()),SLOT(slotFontChanged()));
}


void AccountDialog::makePopAccountPage()
{
  QFrame *page = makeMainWidget();
  QVBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );

  mPop.titleLabel = new QLabel( page );
  mPop.titleLabel->setText( i18n("Account Type: POP Account") );
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

  QGridLayout *grid = new QGridLayout( page1, 16, 2, marginHint(), spacingHint() );
  grid->addColSpacing( 1, fontMetrics().maxWidth()*15 );
  grid->setRowStretch( 15, 10 );
  grid->setColStretch( 1, 10 );

  QLabel *label = new QLabel( i18n("Account &name:"), page1 );
  grid->addWidget( label, 0, 0 );
  mPop.nameEdit = new KLineEdit( page1 );
  label->setBuddy( mPop.nameEdit );
  grid->addWidget( mPop.nameEdit, 0, 1 );

  label = new QLabel( i18n("&Login:"), page1 );
  QWhatsThis::add( label, i18n("Your Internet Service Provider gave you a <em>user name</em> which is used to authenticate you with their servers. It usually is the first part of your email address (the part before <em>@</em>).") );
  grid->addWidget( label, 1, 0 );
  mPop.loginEdit = new KLineEdit( page1 );
  label->setBuddy( mPop.loginEdit );
  grid->addWidget( mPop.loginEdit, 1, 1 );

  label = new QLabel( i18n("P&assword:"), page1 );
  grid->addWidget( label, 2, 0 );
  mPop.passwordEdit = new KLineEdit( page1 );
  mPop.passwordEdit->setEchoMode( QLineEdit::Password );
  label->setBuddy( mPop.passwordEdit );
  grid->addWidget( mPop.passwordEdit, 2, 1 );

  label = new QLabel( i18n("Ho&st:"), page1 );
  grid->addWidget( label, 3, 0 );
  mPop.hostEdit = new KLineEdit( page1 );
  // only letters, digits, '-', '.', ':' (IPv6) and '_' (for Windows
  // compatibility) are allowed
  mPop.hostEdit->setValidator(mValidator);
  label->setBuddy( mPop.hostEdit );
  grid->addWidget( mPop.hostEdit, 3, 1 );

  label = new QLabel( i18n("&Port:"), page1 );
  grid->addWidget( label, 4, 0 );
  mPop.portEdit = new KLineEdit( page1 );
  mPop.portEdit->setValidator( new QIntValidator(this) );
  label->setBuddy( mPop.portEdit );
  grid->addWidget( mPop.portEdit, 4, 1 );

  mPop.storePasswordCheck =
    new QCheckBox( i18n("Sto&re POP password"), page1 );
  QWhatsThis::add( mPop.storePasswordCheck,
                   i18n("Check this option to have KMail store "
                   "the password.\nIf KWallet is available "
                   "the password will be stored there which is considered "
                   "safe.\nHowever, if KWallet is not available, "
                   "the password will be stored in KMail's configuration "
                   "file. The password is stored in an "
                   "obfuscated format, but should not be "
                   "considered secure from decryption efforts "
                   "if access to the configuration file is obtained.") );
  grid->addMultiCellWidget( mPop.storePasswordCheck, 5, 5, 0, 1 );

  mPop.leaveOnServerCheck =
    new QCheckBox( i18n("Lea&ve fetched messages on the server"), page1 );
  connect( mPop.leaveOnServerCheck, SIGNAL( clicked() ),
           this, SLOT( slotLeaveOnServerClicked() ) );
  grid->addMultiCellWidget( mPop.leaveOnServerCheck, 6, 6, 0, 1 );
  QHBox *afterDaysBox = new QHBox( page1 );
  afterDaysBox->setSpacing( KDialog::spacingHint() );
  mPop.leaveOnServerDaysCheck =
    new QCheckBox( i18n("Leave messages on the server for"), afterDaysBox );
  connect( mPop.leaveOnServerDaysCheck, SIGNAL( toggled(bool) ),
           this, SLOT( slotEnableLeaveOnServerDays(bool)) );
  mPop.leaveOnServerDaysSpin = new KIntNumInput( afterDaysBox );
  mPop.leaveOnServerDaysSpin->setRange( 1, 365, 1, false );
  connect( mPop.leaveOnServerDaysSpin, SIGNAL(valueChanged(int)),
           SLOT(slotLeaveOnServerDaysChanged(int)));
  mPop.leaveOnServerDaysSpin->setValue( 1 );
  afterDaysBox->setStretchFactor( mPop.leaveOnServerDaysSpin, 1 );
  grid->addMultiCellWidget( afterDaysBox, 7, 7, 0, 1 );
  QHBox *leaveOnServerCountBox = new QHBox( page1 );
  leaveOnServerCountBox->setSpacing( KDialog::spacingHint() );
  mPop.leaveOnServerCountCheck =
    new QCheckBox( i18n("Keep only the last"), leaveOnServerCountBox );
  connect( mPop.leaveOnServerCountCheck, SIGNAL( toggled(bool) ),
           this, SLOT( slotEnableLeaveOnServerCount(bool)) );
  mPop.leaveOnServerCountSpin = new KIntNumInput( leaveOnServerCountBox );
  mPop.leaveOnServerCountSpin->setRange( 1, 999999, 1, false );
  connect( mPop.leaveOnServerCountSpin, SIGNAL(valueChanged(int)),
           SLOT(slotLeaveOnServerCountChanged(int)));
  mPop.leaveOnServerCountSpin->setValue( 100 );
  grid->addMultiCellWidget( leaveOnServerCountBox, 8, 8, 0, 1 );
  QHBox *leaveOnServerSizeBox = new QHBox( page1 );
  leaveOnServerSizeBox->setSpacing( KDialog::spacingHint() );
  mPop.leaveOnServerSizeCheck =
    new QCheckBox( i18n("Keep only the last"), leaveOnServerSizeBox );
  connect( mPop.leaveOnServerSizeCheck, SIGNAL( toggled(bool) ),
           this, SLOT( slotEnableLeaveOnServerSize(bool)) );
  mPop.leaveOnServerSizeSpin = new KIntNumInput( leaveOnServerSizeBox );
  mPop.leaveOnServerSizeSpin->setRange( 1, 999999, 1, false );
  mPop.leaveOnServerSizeSpin->setSuffix( i18n(" MB") );
  mPop.leaveOnServerSizeSpin->setValue( 10 );
  grid->addMultiCellWidget( leaveOnServerSizeBox, 9, 9, 0, 1 );
#if 0
  QHBox *resourceHB = new QHBox( page1 );
  resourceHB->setSpacing( 11 );
  mPop.resourceCheck =
      new QCheckBox( i18n( "Account for semiautomatic resource handling" ), resourceHB );
  mPop.resourceClearButton =
      new QPushButton( i18n( "Clear" ), resourceHB );
  mPop.resourceClearButton->setEnabled( false );
  connect( mPop.resourceCheck, SIGNAL( toggled(bool) ),
           mPop.resourceClearButton, SLOT( setEnabled(bool) ) );
  QWhatsThis::add( mPop.resourceClearButton,
                   i18n( "Delete all allocations for the resource represented by this account." ) );
  connect( mPop.resourceClearButton, SIGNAL( clicked() ),
           this, SLOT( slotClearResourceAllocations() ) );
  mPop.resourceClearPastButton =
      new QPushButton( i18n( "Clear Past" ), resourceHB );
  mPop.resourceClearPastButton->setEnabled( false );
  connect( mPop.resourceCheck, SIGNAL( toggled(bool) ),
           mPop.resourceClearPastButton, SLOT( setEnabled(bool) ) );
  QWhatsThis::add( mPop.resourceClearPastButton,
                   i18n( "Delete all outdated allocations for the resource represented by this account." ) );
  connect( mPop.resourceClearPastButton, SIGNAL( clicked() ),
           this, SLOT( slotClearPastResourceAllocations() ) );
  grid->addMultiCellWidget( resourceHB, 10, 10, 0, 2 );
#endif

  mPop.includeInCheck =
    new QCheckBox( i18n("Include in man&ual mail check"), page1 );
  grid->addMultiCellWidget( mPop.includeInCheck, 10, 10, 0, 1 );

  QHBox * hbox = new QHBox( page1 );
  hbox->setSpacing( KDialog::spacingHint() );
  mPop.filterOnServerCheck =
    new QCheckBox( i18n("&Filter messages if they are greater than"), hbox );
  mPop.filterOnServerSizeSpin = new KIntNumInput ( hbox );
  mPop.filterOnServerSizeSpin->setEnabled( false );
  hbox->setStretchFactor( mPop.filterOnServerSizeSpin, 1 );
  mPop.filterOnServerSizeSpin->setRange( 1, 10000000, 100, false );
  connect(mPop.filterOnServerSizeSpin, SIGNAL(valueChanged(int)),
          SLOT(slotFilterOnServerSizeChanged(int)));
  mPop.filterOnServerSizeSpin->setValue( 50000 );
  grid->addMultiCellWidget( hbox, 11, 11, 0, 1 );
  connect( mPop.filterOnServerCheck, SIGNAL(toggled(bool)),
	   mPop.filterOnServerSizeSpin, SLOT(setEnabled(bool)) );
  connect( mPop.filterOnServerCheck, SIGNAL( clicked() ),
           this, SLOT( slotFilterOnServerClicked() ) );
  QString msg = i18n("If you select this option, POP Filters will be used to "
		     "decide what to do with messages. You can then select "
		     "to download, delete or keep them on the server." );
  QWhatsThis::add( mPop.filterOnServerCheck, msg );
  QWhatsThis::add( mPop.filterOnServerSizeSpin, msg );

  mPop.intervalCheck =
    new QCheckBox( i18n("Enable &interval mail checking"), page1 );
  grid->addMultiCellWidget( mPop.intervalCheck, 12, 12, 0, 1 );
  connect( mPop.intervalCheck, SIGNAL(toggled(bool)),
	   this, SLOT(slotEnablePopInterval(bool)) );
  mPop.intervalLabel = new QLabel( i18n("Chec&k interval:"), page1 );
  grid->addWidget( mPop.intervalLabel, 13, 0 );
  mPop.intervalSpin = new KIntNumInput( page1 );
  mPop.intervalSpin->setRange( GlobalSettings::self()->minimumCheckInterval(), 10000, 1, false );
  mPop.intervalSpin->setSuffix( i18n(" min") );
  mPop.intervalSpin->setValue( defaultmailcheckintervalmin );
  mPop.intervalLabel->setBuddy( mPop.intervalSpin );
  grid->addWidget( mPop.intervalSpin, 13, 1 );

  label = new QLabel( i18n("Des&tination folder:"), page1 );
  grid->addWidget( label, 14, 0 );
  mPop.folderCombo = new QComboBox( false, page1 );
  label->setBuddy( mPop.folderCombo );
  grid->addWidget( mPop.folderCombo, 14, 1 );

  label = new QLabel( i18n("Pre-com&mand:"), page1 );
  grid->addWidget( label, 15, 0 );
  mPop.precommand = new KLineEdit( page1 );
  label->setBuddy(mPop.precommand);
  grid->addWidget( mPop.precommand, 15, 1 );

  mPop.identityLabel = new QLabel( i18n("Identity:"), page1 );
  grid->addWidget( mPop.identityLabel, 16, 0 );
  mPop.identityCombo = new KPIM::IdentityCombo(kmkernel->identityManager(), page1 );
  mPop.identityLabel->setBuddy( mPop.identityCombo );
  grid->addWidget( mPop.identityCombo, 16, 1 );

  QWidget *page2 = new QWidget( tabWidget );
  tabWidget->addTab( page2, i18n("&Extras") );
  QVBoxLayout *vlay = new QVBoxLayout( page2, marginHint(), spacingHint() );

  vlay->addSpacing( KDialog::spacingHint() );

  QHBoxLayout *buttonLay = new QHBoxLayout( vlay );
  mPop.checkCapabilities =
    new QPushButton( i18n("Check &What the Server Supports"), page2 );
  connect(mPop.checkCapabilities, SIGNAL(clicked()),
    SLOT(slotCheckPopCapabilities()));
  buttonLay->addStretch();
  buttonLay->addWidget( mPop.checkCapabilities );
  buttonLay->addStretch();

  vlay->addSpacing( KDialog::spacingHint() );

  mPop.encryptionGroup = new QButtonGroup( 1, Qt::Horizontal,
    i18n("Encryption"), page2 );
  mPop.encryptionNone =
    new QRadioButton( i18n("&None"), mPop.encryptionGroup );
  mPop.encryptionSSL =
    new QRadioButton( i18n("Use &SSL for secure mail download"),
    mPop.encryptionGroup );
  mPop.encryptionTLS =
    new QRadioButton( i18n("Use &TLS for secure mail download"),
    mPop.encryptionGroup );
  connect(mPop.encryptionGroup, SIGNAL(clicked(int)),
    SLOT(slotPopEncryptionChanged(int)));
  vlay->addWidget( mPop.encryptionGroup );

  mPop.authGroup = new QButtonGroup( 1, Qt::Horizontal,
    i18n("Authentication Method"), page2 );
  mPop.authUser = new QRadioButton( i18n("Clear te&xt") , mPop.authGroup,
                                    "auth clear text" );
  mPop.authLogin = new QRadioButton( i18n("Please translate this "
    "authentication method only if you have a good reason", "&LOGIN"),
    mPop.authGroup, "auth login" );
  mPop.authPlain = new QRadioButton( i18n("Please translate this "
    "authentication method only if you have a good reason", "&PLAIN"),
    mPop.authGroup, "auth plain"  );
  mPop.authCRAM_MD5 = new QRadioButton( i18n("CRAM-MD&5"), mPop.authGroup, "auth cram-md5" );
  mPop.authDigestMd5 = new QRadioButton( i18n("&DIGEST-MD5"), mPop.authGroup, "auth digest-md5" );
  mPop.authNTLM = new QRadioButton( i18n("&NTLM"), mPop.authGroup, "auth ntlm" );
  mPop.authGSSAPI = new QRadioButton( i18n("&GSSAPI"), mPop.authGroup, "auth gssapi" );
  if ( KProtocolInfo::capabilities("pop3").contains("SASL") == 0 )
  {
    mPop.authNTLM->hide();
    mPop.authGSSAPI->hide();
  }
  mPop.authAPOP = new QRadioButton( i18n("&APOP"), mPop.authGroup, "auth apop" );

  vlay->addWidget( mPop.authGroup );

  mPop.usePipeliningCheck =
    new QCheckBox( i18n("&Use pipelining for faster mail download"), page2 );
  connect(mPop.usePipeliningCheck, SIGNAL(clicked()),
    SLOT(slotPipeliningClicked()));
  vlay->addWidget( mPop.usePipeliningCheck );

  vlay->addStretch();

  connect(kapp,SIGNAL(kdisplayFontChanged()),SLOT(slotFontChanged()));
}


void AccountDialog::makeImapAccountPage( bool connected )
{
  QFrame *page = makeMainWidget();
  QVBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );

  mImap.titleLabel = new QLabel( page );
  if( connected )
    mImap.titleLabel->setText( i18n("Account Type: Disconnected IMAP Account") );
  else
    mImap.titleLabel->setText( i18n("Account Type: IMAP Account") );
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

  int row = -1;
  QGridLayout *grid = new QGridLayout( page1, 16, 2, marginHint(), spacingHint() );
  grid->addColSpacing( 1, fontMetrics().maxWidth()*16 );

  ++row;
  QLabel *label = new QLabel( i18n("Account &name:"), page1 );
  grid->addWidget( label, row, 0 );
  mImap.nameEdit = new KLineEdit( page1 );
  label->setBuddy( mImap.nameEdit );
  grid->addWidget( mImap.nameEdit, row, 1 );

  ++row;
  label = new QLabel( i18n("&Login:"), page1 );
  QWhatsThis::add( label, i18n("Your Internet Service Provider gave you a <em>user name</em> which is used to authenticate you with their servers. It usually is the first part of your email address (the part before <em>@</em>).") );
  grid->addWidget( label, row, 0 );
  mImap.loginEdit = new KLineEdit( page1 );
  label->setBuddy( mImap.loginEdit );
  grid->addWidget( mImap.loginEdit, row, 1 );

  ++row;
  label = new QLabel( i18n("P&assword:"), page1 );
  grid->addWidget( label, row, 0 );
  mImap.passwordEdit = new KLineEdit( page1 );
  mImap.passwordEdit->setEchoMode( QLineEdit::Password );
  label->setBuddy( mImap.passwordEdit );
  grid->addWidget( mImap.passwordEdit, row, 1 );

  ++row;
  label = new QLabel( i18n("Ho&st:"), page1 );
  grid->addWidget( label, row, 0 );
  mImap.hostEdit = new KLineEdit( page1 );
  // only letters, digits, '-', '.', ':' (IPv6) and '_' (for Windows
  // compatibility) are allowed
  mImap.hostEdit->setValidator(mValidator);
  label->setBuddy( mImap.hostEdit );
  grid->addWidget( mImap.hostEdit, row, 1 );

  ++row;
  label = new QLabel( i18n("&Port:"), page1 );
  grid->addWidget( label, row, 0 );
  mImap.portEdit = new KLineEdit( page1 );
  mImap.portEdit->setValidator( new QIntValidator(this) );
  label->setBuddy( mImap.portEdit );
  grid->addWidget( mImap.portEdit, row, 1 );

  // namespace list
  ++row;
  QHBox* box = new QHBox( page1 );
  label = new QLabel( i18n("Namespaces:"), box );
  QWhatsThis::add( label, i18n( "Here you see the different namespaces that your IMAP server supports."
        "Each namespace represents a prefix that separates groups of folders."
        "Namespaces allow KMail for example to display your personal folders and shared folders in one account." ) );
  // button to reload
  QToolButton* button = new QToolButton( box );
  button->setAutoRaise(true);
  button->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );
  button->setFixedSize( 22, 22 );
  button->setIconSet(
      KGlobal::iconLoader()->loadIconSet( "reload", KIcon::Small, 0 ) );
  connect( button, SIGNAL(clicked()), this, SLOT(slotReloadNamespaces()) );
  QWhatsThis::add( button,
      i18n("Reload the namespaces from the server. This overwrites any changes.") );
  grid->addWidget( box, row, 0 );

  // grid with label, namespace list and edit button
  QGrid* listbox = new QGrid( 3, page1 );
  label = new QLabel( i18n("Personal"), listbox );
  QWhatsThis::add( label, i18n( "Personal namespaces include your personal folders." ) );
  mImap.personalNS = new KLineEdit( listbox );
  mImap.personalNS->setReadOnly( true );
  mImap.editPNS = new QToolButton( listbox );
  mImap.editPNS->setIconSet(
      KGlobal::iconLoader()->loadIconSet( "edit", KIcon::Small, 0 ) );
  mImap.editPNS->setAutoRaise( true );
  mImap.editPNS->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );
  mImap.editPNS->setFixedSize( 22, 22 );
  connect( mImap.editPNS, SIGNAL(clicked()), this, SLOT(slotEditPersonalNamespace()) );

  label = new QLabel( i18n("Other Users"), listbox );
  QWhatsThis::add( label, i18n( "These namespaces include the folders of other users." ) );
  mImap.otherUsersNS = new KLineEdit( listbox );
  mImap.otherUsersNS->setReadOnly( true );
  mImap.editONS = new QToolButton( listbox );
  mImap.editONS->setIconSet(
      KGlobal::iconLoader()->loadIconSet( "edit", KIcon::Small, 0 ) );
  mImap.editONS->setAutoRaise( true );
  mImap.editONS->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );
  mImap.editONS->setFixedSize( 22, 22 );
  connect( mImap.editONS, SIGNAL(clicked()), this, SLOT(slotEditOtherUsersNamespace()) );

  label = new QLabel( i18n("Shared"), listbox );
  QWhatsThis::add( label, i18n( "These namespaces include the shared folders." ) );
  mImap.sharedNS = new KLineEdit( listbox );
  mImap.sharedNS->setReadOnly( true );
  mImap.editSNS = new QToolButton( listbox );
  mImap.editSNS->setIconSet(
      KGlobal::iconLoader()->loadIconSet( "edit", KIcon::Small, 0 ) );
  mImap.editSNS->setAutoRaise( true );
  mImap.editSNS->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );
  mImap.editSNS->setFixedSize( 22, 22 );
  connect( mImap.editSNS, SIGNAL(clicked()), this, SLOT(slotEditSharedNamespace()) );

  label->setBuddy( listbox );
  grid->addWidget( listbox, row, 1 );

  ++row;
  mImap.storePasswordCheck =
    new QCheckBox( i18n("Sto&re IMAP password"), page1 );
  QWhatsThis::add( mImap.storePasswordCheck,
                   i18n("Check this option to have KMail store "
                   "the password.\nIf KWallet is available "
                   "the password will be stored there which is considered "
                   "safe.\nHowever, if KWallet is not available, "
                   "the password will be stored in KMail's configuration "
                   "file. The password is stored in an "
                   "obfuscated format, but should not be "
                   "considered secure from decryption efforts "
                   "if access to the configuration file is obtained.") );
  grid->addMultiCellWidget( mImap.storePasswordCheck, row, row, 0, 1 );

  if( !connected ) {
    ++row;
    mImap.autoExpungeCheck =
      new QCheckBox( i18n("Automaticall&y compact folders (expunges deleted messages)"), page1);
    grid->addMultiCellWidget( mImap.autoExpungeCheck, row, row, 0, 1 );
  }

  ++row;
  mImap.hiddenFoldersCheck = new QCheckBox( i18n("Sho&w hidden folders"), page1);
  grid->addMultiCellWidget( mImap.hiddenFoldersCheck, row, row, 0, 1 );


  ++row;
  mImap.subscribedFoldersCheck = new QCheckBox(
    i18n("Show only s&ubscribed folders"), page1);
  grid->addMultiCellWidget( mImap.subscribedFoldersCheck, row, row, 0, 1 );

  ++row;
  mImap.locallySubscribedFoldersCheck = new QCheckBox(
    i18n("Show only &locally subscribed folders"), page1);
  grid->addMultiCellWidget( mImap.locallySubscribedFoldersCheck, row, row, 0, 1 );

  if ( !connected ) {
    // not implemented for disconnected yet
    ++row;
    mImap.loadOnDemandCheck = new QCheckBox(
        i18n("Load attach&ments on demand"), page1);
    QWhatsThis::add( mImap.loadOnDemandCheck,
        i18n("Activate this to load attachments not automatically when you select the email but only when you click on the attachment. This way also big emails are shown instantly.") );
    grid->addMultiCellWidget( mImap.loadOnDemandCheck, row, row, 0, 1 );
  }

  if ( !connected ) {
    // not implemented for disconnected yet
    ++row;
    mImap.listOnlyOpenCheck = new QCheckBox(
        i18n("List only open folders"), page1);
    QWhatsThis::add( mImap.listOnlyOpenCheck,
        i18n("Only folders that are open (expanded) in the folder tree are checked for subfolders. Use this if there are many folders on the server.") );
    grid->addMultiCellWidget( mImap.listOnlyOpenCheck, row, row, 0, 1 );
  }

#if 0
  ++row;
  QHBox* resourceHB = new QHBox( page1 );
  resourceHB->setSpacing( 11 );
  mImap.resourceCheck =
      new QCheckBox( i18n( "Account for semiautomatic resource handling" ), resourceHB );
  mImap.resourceClearButton =
      new QPushButton( i18n( "Clear" ), resourceHB );
  mImap.resourceClearButton->setEnabled( false );
  connect( mImap.resourceCheck, SIGNAL( toggled(bool) ),
           mImap.resourceClearButton, SLOT( setEnabled(bool) ) );
  QWhatsThis::add( mImap.resourceClearButton,
                   i18n( "Delete all allocations for the resource represented by this account." ) );
  connect( mImap.resourceClearButton, SIGNAL( clicked() ),
           this, SLOT( slotClearResourceAllocations() ) );
  mImap.resourceClearPastButton =
      new QPushButton( i18n( "Clear Past" ), resourceHB );
  mImap.resourceClearPastButton->setEnabled( false );
  connect( mImap.resourceCheck, SIGNAL( toggled(bool) ),
           mImap.resourceClearPastButton, SLOT( setEnabled(bool) ) );
  QWhatsThis::add( mImap.resourceClearPastButton,
                   i18n( "Delete all outdated allocations for the resource represented by this account." ) );
  connect( mImap.resourceClearPastButton, SIGNAL( clicked() ),
           this, SLOT( slotClearPastResourceAllocations() ) );
  grid->addMultiCellWidget( resourceHB, row, row, 0, 2 );
#endif

  ++row;
  mImap.includeInCheck =
    new QCheckBox( i18n("Include in manual mail chec&k"), page1 );
  grid->addMultiCellWidget( mImap.includeInCheck, row, row, 0, 1 );

  ++row;
  mImap.intervalCheck =
    new QCheckBox( i18n("Enable &interval mail checking"), page1 );
  grid->addMultiCellWidget( mImap.intervalCheck, row, row, 0, 2 );
  connect( mImap.intervalCheck, SIGNAL(toggled(bool)),
	   this, SLOT(slotEnableImapInterval(bool)) );
  ++row;
  mImap.intervalLabel = new QLabel( i18n("Check inter&val:"), page1 );
  grid->addWidget( mImap.intervalLabel, row, 0 );
  mImap.intervalSpin = new KIntNumInput( page1 );
  mImap.intervalSpin->setRange( GlobalSettings::minimumCheckInterval(), 60, 1, false );
  mImap.intervalSpin->setValue( defaultmailcheckintervalmin );
  mImap.intervalSpin->setSuffix( i18n( " min" ) );
  mImap.intervalLabel->setBuddy( mImap.intervalSpin );
  grid->addWidget( mImap.intervalSpin, row, 1 );

  ++row;
  label = new QLabel( i18n("&Trash folder:"), page1 );
  grid->addWidget( label, row, 0 );
  mImap.trashCombo = new FolderRequester( page1,
      kmkernel->getKMMainWidget()->folderTree() );
  mImap.trashCombo->setShowOutbox( false );
  label->setBuddy( mImap.trashCombo );
  grid->addWidget( mImap.trashCombo, row, 1 );

  ++row;
  mImap.identityLabel = new QLabel( i18n("Identity:"), page1 );
  grid->addWidget( mImap.identityLabel, row, 0 );
  mImap.identityCombo = new KPIM::IdentityCombo(kmkernel->identityManager(), page1 );
  mImap.identityLabel->setBuddy( mImap.identityCombo );
  grid->addWidget( mImap.identityCombo, row, 1 );

  QWidget *page2 = new QWidget( tabWidget );
  tabWidget->addTab( page2, i18n("S&ecurity") );
  QVBoxLayout *vlay = new QVBoxLayout( page2, marginHint(), spacingHint() );

  vlay->addSpacing( KDialog::spacingHint() );

  QHBoxLayout *buttonLay = new QHBoxLayout( vlay );
  mImap.checkCapabilities =
    new QPushButton( i18n("Check &What the Server Supports"), page2 );
  connect(mImap.checkCapabilities, SIGNAL(clicked()),
    SLOT(slotCheckImapCapabilities()));
  buttonLay->addStretch();
  buttonLay->addWidget( mImap.checkCapabilities );
  buttonLay->addStretch();

  vlay->addSpacing( KDialog::spacingHint() );

  mImap.encryptionGroup = new QButtonGroup( 1, Qt::Horizontal,
    i18n("Encryption"), page2 );
  mImap.encryptionNone =
    new QRadioButton( i18n("&None"), mImap.encryptionGroup );
  mImap.encryptionSSL =
    new QRadioButton( i18n("Use &SSL for secure mail download"),
    mImap.encryptionGroup );
  mImap.encryptionTLS =
    new QRadioButton( i18n("Use &TLS for secure mail download"),
    mImap.encryptionGroup );
  connect(mImap.encryptionGroup, SIGNAL(clicked(int)),
    SLOT(slotImapEncryptionChanged(int)));
  vlay->addWidget( mImap.encryptionGroup );

  mImap.authGroup = new QButtonGroup( 1, Qt::Horizontal,
    i18n("Authentication Method"), page2 );
  mImap.authUser = new QRadioButton( i18n("Clear te&xt"), mImap.authGroup );
  mImap.authLogin = new QRadioButton( i18n("Please translate this "
    "authentication method only if you have a good reason", "&LOGIN"),
    mImap.authGroup );
  mImap.authPlain = new QRadioButton( i18n("Please translate this "
    "authentication method only if you have a good reason", "&PLAIN"),
     mImap.authGroup );
  mImap.authCramMd5 = new QRadioButton( i18n("CRAM-MD&5"), mImap.authGroup );
  mImap.authDigestMd5 = new QRadioButton( i18n("&DIGEST-MD5"), mImap.authGroup );
  mImap.authNTLM = new QRadioButton( i18n("&NTLM"), mImap.authGroup );
  mImap.authGSSAPI = new QRadioButton( i18n("&GSSAPI"), mImap.authGroup );
  mImap.authAnonymous = new QRadioButton( i18n("&Anonymous"), mImap.authGroup );
  vlay->addWidget( mImap.authGroup );

  vlay->addStretch();

  // TODO (marc/bo): Test this
  mSieveConfigEditor = new SieveConfigEditor( tabWidget );
  mSieveConfigEditor->layout()->setMargin( KDialog::marginHint() );
  tabWidget->addTab( mSieveConfigEditor, i18n("&Filtering") );

  connect(kapp,SIGNAL(kdisplayFontChanged()),SLOT(slotFontChanged()));
}


void AccountDialog::setupSettings()
{
  QComboBox *folderCombo = 0;
  int interval = mAccount->checkInterval();

  QString accountType = mAccount->type();
  if( accountType == "local" )
  {
    ProcmailRCParser procmailrcParser;
    KMAcctLocal *acctLocal = dynamic_cast<KMAcctLocal*>(mAccount);

    if ( acctLocal->location().isEmpty() )
        acctLocal->setLocation( procmailrcParser.getSpoolFilesList().first() );
    else
        mLocal.locationEdit->insertItem( acctLocal->location() );

    if ( acctLocal->procmailLockFileName().isEmpty() )
        acctLocal->setProcmailLockFileName( procmailrcParser.getLockFilesList().first() );
    else
        mLocal.procmailLockFileName->insertItem( acctLocal->procmailLockFileName() );

    mLocal.nameEdit->setText( mAccount->name() );
    mLocal.nameEdit->setFocus();
    mLocal.locationEdit->setEditText( acctLocal->location() );
    if (acctLocal->lockType() == mutt_dotlock)
      mLocal.lockMutt->setChecked(true);
    else if (acctLocal->lockType() == mutt_dotlock_privileged)
      mLocal.lockMuttPriv->setChecked(true);
    else if (acctLocal->lockType() == procmail_lockfile) {
      mLocal.lockProcmail->setChecked(true);
      mLocal.procmailLockFileName->setEditText(acctLocal->procmailLockFileName());
    } else if (acctLocal->lockType() == FCNTL)
      mLocal.lockFcntl->setChecked(true);
    else if (acctLocal->lockType() == lock_none)
      mLocal.lockNone->setChecked(true);

    if ( interval <= 0 ) mLocal.intervalSpin->setValue( defaultmailcheckintervalmin );
    else mLocal.intervalSpin->setValue( interval );
    mLocal.intervalCheck->setChecked( interval >= 1 );
#if 0
    mLocal.resourceCheck->setChecked( mAccount->resource() );
#endif
    mLocal.includeInCheck->setChecked( !mAccount->checkExclude() );
    mLocal.precommand->setText( mAccount->precommand() );

    slotEnableLocalInterval( interval >= 1 );
    folderCombo = mLocal.folderCombo;
    mLocal.identityCombo-> setCurrentIdentity( mAccount->identityId() );
  }
  else if( accountType == "pop" )
  {
    PopAccount &ap = *(PopAccount*)mAccount;
    mPop.nameEdit->setText( mAccount->name() );
    mPop.nameEdit->setFocus();
    mPop.loginEdit->setText( ap.login() );
    mPop.passwordEdit->setText( ap.passwd());
    mPop.hostEdit->setText( ap.host() );
    mPop.portEdit->setText( QString("%1").arg( ap.port() ) );
    mPop.usePipeliningCheck->setChecked( ap.usePipelining() );
    mPop.storePasswordCheck->setChecked( ap.storePasswd() );
    mPop.leaveOnServerCheck->setChecked( ap.leaveOnServer() );
    mPop.leaveOnServerDaysCheck->setEnabled( ap.leaveOnServer() );
    mPop.leaveOnServerDaysCheck->setChecked( ap.leaveOnServerDays() >= 1 );
    mPop.leaveOnServerDaysSpin->setValue( ap.leaveOnServerDays() >= 1 ?
                                            ap.leaveOnServerDays() : 7 );
    mPop.leaveOnServerCountCheck->setEnabled( ap.leaveOnServer() );
    mPop.leaveOnServerCountCheck->setChecked( ap.leaveOnServerCount() >= 1 );
    mPop.leaveOnServerCountSpin->setValue( ap.leaveOnServerCount() >= 1 ?
                                            ap.leaveOnServerCount() : 100 );
    mPop.leaveOnServerSizeCheck->setEnabled( ap.leaveOnServer() );
    mPop.leaveOnServerSizeCheck->setChecked( ap.leaveOnServerSize() >= 1 );
    mPop.leaveOnServerSizeSpin->setValue( ap.leaveOnServerSize() >= 1 ?
                                            ap.leaveOnServerSize() : 10 );
    mPop.filterOnServerCheck->setChecked( ap.filterOnServer() );
    mPop.filterOnServerSizeSpin->setValue( ap.filterOnServerCheckSize() );
    mPop.intervalCheck->setChecked( interval >= 1 );
    if ( interval <= 0 ) mPop.intervalSpin->setValue( defaultmailcheckintervalmin );
    else mPop.intervalSpin->setValue( interval );
#if 0
    mPop.resourceCheck->setChecked( mAccount->resource() );
#endif
    mPop.includeInCheck->setChecked( !mAccount->checkExclude() );
    mPop.precommand->setText( ap.precommand() );
    mPop.identityCombo-> setCurrentIdentity( mAccount->identityId() );
    if (ap.useSSL())
      mPop.encryptionSSL->setChecked( true );
    else if (ap.useTLS())
      mPop.encryptionTLS->setChecked( true );
    else mPop.encryptionNone->setChecked( true );
    if (ap.auth() == "LOGIN")
      mPop.authLogin->setChecked( true );
    else if (ap.auth() == "PLAIN")
      mPop.authPlain->setChecked( true );
    else if (ap.auth() == "CRAM-MD5")
      mPop.authCRAM_MD5->setChecked( true );
    else if (ap.auth() == "DIGEST-MD5")
      mPop.authDigestMd5->setChecked( true );
    else if (ap.auth() == "NTLM")
      mPop.authNTLM->setChecked( true );
    else if (ap.auth() == "GSSAPI")
      mPop.authGSSAPI->setChecked( true );
    else if (ap.auth() == "APOP")
      mPop.authAPOP->setChecked( true );
    else mPop.authUser->setChecked( true );

    slotEnableLeaveOnServerDays( mPop.leaveOnServerDaysCheck->isEnabled() ?
                                   ap.leaveOnServerDays() >= 1 : 0);
    slotEnableLeaveOnServerCount( mPop.leaveOnServerCountCheck->isEnabled() ?
                                    ap.leaveOnServerCount() >= 1 : 0);
    slotEnableLeaveOnServerSize( mPop.leaveOnServerSizeCheck->isEnabled() ?
                                    ap.leaveOnServerSize() >= 1 : 0);
    slotEnablePopInterval( interval >= 1 );
    folderCombo = mPop.folderCombo;
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
    mImap.autoExpungeCheck->setChecked( ai.autoExpunge() );
    mImap.hiddenFoldersCheck->setChecked( ai.hiddenFolders() );
    mImap.subscribedFoldersCheck->setChecked( ai.onlySubscribedFolders() );
    mImap.locallySubscribedFoldersCheck->setChecked( ai.onlyLocallySubscribedFolders() );
    mImap.loadOnDemandCheck->setChecked( ai.loadOnDemand() );
    mImap.listOnlyOpenCheck->setChecked( ai.listOnlyOpenFolders() );
    mImap.storePasswordCheck->setChecked( ai.storePasswd() );
#if 0
    mImap.resourceCheck->setChecked( ai.resource() );
#endif
    mImap.includeInCheck->setChecked( !ai.checkExclude() );
    mImap.intervalCheck->setChecked( interval >= 1 );
    if ( interval <= 0 ) mImap.intervalSpin->setValue( defaultmailcheckintervalmin );
    else mImap.intervalSpin->setValue( interval );
    QString trashfolder = ai.trash();
    if (trashfolder.isEmpty())
      trashfolder = kmkernel->trashFolder()->idString();
    mImap.trashCombo->setFolder( trashfolder );
    slotEnableImapInterval( interval >= 1 );
    mImap.identityCombo-> setCurrentIdentity( mAccount->identityId() );
    //mImap.identityCombo->insertStringList( kmkernel->identityManager()->shadowIdentities() );
    if (ai.useSSL())
      mImap.encryptionSSL->setChecked( true );
    else if (ai.useTLS())
      mImap.encryptionTLS->setChecked( true );
    else mImap.encryptionNone->setChecked( true );
    if (ai.auth() == "CRAM-MD5")
      mImap.authCramMd5->setChecked( true );
    else if (ai.auth() == "DIGEST-MD5")
      mImap.authDigestMd5->setChecked( true );
    else if (ai.auth() == "NTLM")
      mImap.authNTLM->setChecked( true );
    else if (ai.auth() == "GSSAPI")
      mImap.authGSSAPI->setChecked( true );
    else if (ai.auth() == "ANONYMOUS")
      mImap.authAnonymous->setChecked( true );
    else if (ai.auth() == "PLAIN")
      mImap.authPlain->setChecked( true );
    else if (ai.auth() == "LOGIN")
      mImap.authLogin->setChecked( true );
    else mImap.authUser->setChecked( true );
    if ( mSieveConfigEditor )
      mSieveConfigEditor->setConfig( ai.sieveConfig() );
  }
  else if( accountType == "cachedimap" )
  {
    KMAcctCachedImap &ai = *(KMAcctCachedImap*)mAccount;
    mImap.nameEdit->setText( mAccount->name() );
    mImap.nameEdit->setFocus();
    mImap.loginEdit->setText( ai.login() );
    mImap.passwordEdit->setText( ai.passwd());
    mImap.hostEdit->setText( ai.host() );
    mImap.portEdit->setText( QString("%1").arg( ai.port() ) );
#if 0
    mImap.resourceCheck->setChecked( ai.resource() );
#endif
    mImap.hiddenFoldersCheck->setChecked( ai.hiddenFolders() );
    mImap.subscribedFoldersCheck->setChecked( ai.onlySubscribedFolders() );
    mImap.locallySubscribedFoldersCheck->setChecked( ai.onlyLocallySubscribedFolders() );
    mImap.storePasswordCheck->setChecked( ai.storePasswd() );
    mImap.intervalCheck->setChecked( interval >= 1 );
    if ( interval <= 0 ) mImap.intervalSpin->setValue( defaultmailcheckintervalmin );
    else mImap.intervalSpin->setValue( interval );
    mImap.includeInCheck->setChecked( !ai.checkExclude() );
    QString trashfolder = ai.trash();
    if (trashfolder.isEmpty())
      trashfolder = kmkernel->trashFolder()->idString();
    mImap.trashCombo->setFolder( trashfolder );
    slotEnableImapInterval( interval >= 1 );
    mImap.identityCombo-> setCurrentIdentity( mAccount->identityId() );
    //mImap.identityCombo->insertStringList( kmkernel->identityManager()->shadowIdentities() );
    if (ai.useSSL())
      mImap.encryptionSSL->setChecked( true );
    else if (ai.useTLS())
      mImap.encryptionTLS->setChecked( true );
    else mImap.encryptionNone->setChecked( true );
    if (ai.auth() == "CRAM-MD5")
      mImap.authCramMd5->setChecked( true );
    else if (ai.auth() == "DIGEST-MD5")
      mImap.authDigestMd5->setChecked( true );
    else if (ai.auth() == "GSSAPI")
      mImap.authGSSAPI->setChecked( true );
    else if (ai.auth() == "NTLM")
      mImap.authNTLM->setChecked( true );
    else if (ai.auth() == "ANONYMOUS")
      mImap.authAnonymous->setChecked( true );
    else if (ai.auth() == "PLAIN")
      mImap.authPlain->setChecked( true );
    else if (ai.auth() == "LOGIN")
      mImap.authLogin->setChecked( true );
    else mImap.authUser->setChecked( true );
    if ( mSieveConfigEditor )
      mSieveConfigEditor->setConfig( ai.sieveConfig() );
  }
  else if( accountType == "maildir" )
  {
    KMAcctMaildir *acctMaildir = dynamic_cast<KMAcctMaildir*>(mAccount);

    mMaildir.nameEdit->setText( mAccount->name() );
    mMaildir.nameEdit->setFocus();
    mMaildir.locationEdit->setEditText( acctMaildir->location() );

    if ( interval <= 0 ) mMaildir.intervalSpin->setValue( defaultmailcheckintervalmin );
    else mMaildir.intervalSpin->setValue( interval );
    mMaildir.intervalCheck->setChecked( interval >= 1 );
#if 0
    mMaildir.resourceCheck->setChecked( mAccount->resource() );
#endif
    mMaildir.includeInCheck->setChecked( !mAccount->checkExclude() );
    mMaildir.precommand->setText( mAccount->precommand() );
    mMaildir.identityCombo-> setCurrentIdentity( mAccount->identityId() );
    slotEnableMaildirInterval( interval >= 1 );
    folderCombo = mMaildir.folderCombo;
  }
  else // Unknown account type
    return;

  if ( accountType == "imap" || accountType == "cachedimap" )
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
    folderCombo->insertItem( i18n("<none>") );
  }
  else
  {
    uint i = 0;
    int curIndex = -1;
    kmkernel->folderMgr()->createI18nFolderList(&mFolderNames, &mFolderList);
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
    mFolderList.prepend(kmkernel->inboxFolder());
    folderCombo->insertStringList(mFolderNames);
    folderCombo->setCurrentItem(curIndex + 1);

    // -sanders hack for startup users. Must investigate this properly
    if (folderCombo->count() == 0)
      folderCombo->insertItem( i18n("inbox") );
  }
}

void AccountDialog::slotLeaveOnServerClicked()
{
  bool state = mPop.leaveOnServerCheck->isChecked();
  mPop.leaveOnServerDaysCheck->setEnabled( state );
  mPop.leaveOnServerCountCheck->setEnabled( state );
  mPop.leaveOnServerSizeCheck->setEnabled( state );
  if ( state ) {
    if ( mPop.leaveOnServerDaysCheck->isChecked() ) {
      slotEnableLeaveOnServerDays( state );
    }
    if ( mPop.leaveOnServerCountCheck->isChecked() ) {
      slotEnableLeaveOnServerCount( state );
    }
    if ( mPop.leaveOnServerSizeCheck->isChecked() ) {
      slotEnableLeaveOnServerSize( state );
    }
  } else {
    slotEnableLeaveOnServerDays( state );
    slotEnableLeaveOnServerCount( state );
    slotEnableLeaveOnServerSize( state );
  }
  if ( !( mCurCapa & UIDL ) && mPop.leaveOnServerCheck->isChecked() ) {
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
  if ( !( mCurCapa & TOP ) && mPop.filterOnServerCheck->isChecked() ) {
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
  if (mPop.usePipeliningCheck->isChecked())
    KMessageBox::information( topLevelWidget(),
      i18n("Please note that this feature can cause some POP3 servers "
      "that do not support pipelining to send corrupted mail;\n"
      "this is configurable, though, because some servers support pipelining "
      "but do not announce their capabilities. To check whether your POP3 server "
      "announces pipelining support use the \"Check What the Server "
      "Supports\" button at the bottom of the dialog;\n"
      "if your server does not announce it, but you want more speed, then "
      "you should do some testing first by sending yourself a batch "
      "of mail and downloading it."), QString::null,
      "pipelining");
}


void AccountDialog::slotPopEncryptionChanged(int id)
{
  kdDebug(5006) << "slotPopEncryptionChanged( " << id << " )" << endl;
  // adjust port
  if ( id == SSL || mPop.portEdit->text() == "995" )
    mPop.portEdit->setText( ( id == SSL ) ? "995" : "110" );

  // switch supported auth methods
  mCurCapa = ( id == TLS ) ? mCapaTLS
                           : ( id == SSL ) ? mCapaSSL
                                           : mCapaNormal;
  enablePopFeatures( mCurCapa );
  const QButton *old = mPop.authGroup->selected();
  if ( !old->isEnabled() )
    checkHighest( mPop.authGroup );
}


void AccountDialog::slotImapEncryptionChanged(int id)
{
  kdDebug(5006) << "slotImapEncryptionChanged( " << id << " )" << endl;
  // adjust port
  if ( id == SSL || mImap.portEdit->text() == "993" )
    mImap.portEdit->setText( ( id == SSL ) ? "993" : "143" );

  // switch supported auth methods
  int authMethods = ( id == TLS ) ? mCapaTLS
                                  : ( id == SSL ) ? mCapaSSL
                                                  : mCapaNormal;
  enableImapAuthMethods( authMethods );
  QButton *old = mImap.authGroup->selected();
  if ( !old->isEnabled() )
    checkHighest( mImap.authGroup );
}


void AccountDialog::slotCheckPopCapabilities()
{
  if ( mPop.hostEdit->text().isEmpty() || mPop.portEdit->text().isEmpty() )
  {
     KMessageBox::sorry( this, i18n( "Please specify a server and port on "
              "the General tab first." ) );
     return;
  }
  delete mServerTest;
  mServerTest = new KMServerTest(POP_PROTOCOL, mPop.hostEdit->text(),
    mPop.portEdit->text().toInt());
  connect( mServerTest, SIGNAL( capabilities( const QStringList &,
                                              const QStringList & ) ),
           this, SLOT( slotPopCapabilities( const QStringList &,
                                            const QStringList & ) ) );
  mPop.checkCapabilities->setEnabled(false);
}


void AccountDialog::slotCheckImapCapabilities()
{
  if ( mImap.hostEdit->text().isEmpty() || mImap.portEdit->text().isEmpty() )
  {
     KMessageBox::sorry( this, i18n( "Please specify a server and port on "
              "the General tab first." ) );
     return;
  }
  delete mServerTest;
  mServerTest = new KMServerTest(IMAP_PROTOCOL, mImap.hostEdit->text(),
    mImap.portEdit->text().toInt());
  connect( mServerTest, SIGNAL( capabilities( const QStringList &,
                                              const QStringList & ) ),
           this, SLOT( slotImapCapabilities( const QStringList &,
                                             const QStringList & ) ) );
  mImap.checkCapabilities->setEnabled(false);
}


unsigned int AccountDialog::popCapabilitiesFromStringList( const QStringList & l )
{
  unsigned int capa = 0;
  kdDebug( 5006 ) << k_funcinfo << l << endl;
  for ( QStringList::const_iterator it = l.begin() ; it != l.end() ; ++it ) {
    QString cur = (*it).upper();
    if ( cur == "PLAIN" )
      capa |= Plain;
    else if ( cur == "LOGIN" )
      capa |= Login;
    else if ( cur == "CRAM-MD5" )
      capa |= CRAM_MD5;
    else if ( cur == "DIGEST-MD5" )
      capa |= Digest_MD5;
    else if ( cur == "NTLM" )
      capa |= NTLM;
    else if ( cur == "GSSAPI" )
      capa |= GSSAPI;
    else if ( cur == "APOP" )
      capa |= APOP;
    else if ( cur == "PIPELINING" )
      capa |= Pipelining;
    else if ( cur == "TOP" )
      capa |= TOP;
    else if ( cur == "UIDL" )
      capa |= UIDL;
    else if ( cur == "STLS" )
      capa |= STLS;
  }
  return capa;
}


void AccountDialog::slotPopCapabilities( const QStringList & capaNormal,
                                         const QStringList & capaSSL )
{
  mPop.checkCapabilities->setEnabled( true );
  mCapaNormal = popCapabilitiesFromStringList( capaNormal );
  if ( mCapaNormal & STLS )
    mCapaTLS = mCapaNormal;
  else
    mCapaTLS = 0;
  mCapaSSL = popCapabilitiesFromStringList( capaSSL );
  kdDebug(5006) << "mCapaNormal = " << mCapaNormal
                << "; mCapaSSL = " << mCapaSSL
                << "; mCapaTLS = " << mCapaTLS << endl;
  mPop.encryptionNone->setEnabled( !capaNormal.isEmpty() );
  mPop.encryptionSSL->setEnabled( !capaSSL.isEmpty() );
  mPop.encryptionTLS->setEnabled( mCapaTLS != 0 );
  checkHighest( mPop.encryptionGroup );
  delete mServerTest;
  mServerTest = 0;
}


void AccountDialog::enablePopFeatures( unsigned int capa )
{
  kdDebug(5006) << "enablePopFeatures( " << capa << " )" << endl;
  mPop.authPlain->setEnabled( capa & Plain );
  mPop.authLogin->setEnabled( capa & Login );
  mPop.authCRAM_MD5->setEnabled( capa & CRAM_MD5 );
  mPop.authDigestMd5->setEnabled( capa & Digest_MD5 );
  mPop.authNTLM->setEnabled( capa & NTLM );
  mPop.authGSSAPI->setEnabled( capa & GSSAPI );
  mPop.authAPOP->setEnabled( capa & APOP );
  if ( !( capa & Pipelining ) && mPop.usePipeliningCheck->isChecked() ) {
    mPop.usePipeliningCheck->setChecked( false );
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
  if ( !( capa & UIDL ) && mPop.leaveOnServerCheck->isChecked() ) {
    mPop.leaveOnServerCheck->setChecked( false );
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
  if ( !( capa & TOP ) && mPop.filterOnServerCheck->isChecked() ) {
    mPop.filterOnServerCheck->setChecked( false );
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


unsigned int AccountDialog::imapCapabilitiesFromStringList( const QStringList & l )
{
  unsigned int capa = 0;
  for ( QStringList::const_iterator it = l.begin() ; it != l.end() ; ++it ) {
    QString cur = (*it).upper();
    if ( cur == "AUTH=PLAIN" )
      capa |= Plain;
    else if ( cur == "AUTH=LOGIN" )
      capa |= Login;
    else if ( cur == "AUTH=CRAM-MD5" )
      capa |= CRAM_MD5;
    else if ( cur == "AUTH=DIGEST-MD5" )
      capa |= Digest_MD5;
    else if ( cur == "AUTH=NTLM" )
      capa |= NTLM;
    else if ( cur == "AUTH=GSSAPI" )
      capa |= GSSAPI;
    else if ( cur == "AUTH=ANONYMOUS" )
      capa |= Anonymous;
    else if ( cur == "STARTTLS" )
      capa |= STARTTLS;
  }
  return capa;
}


void AccountDialog::slotImapCapabilities( const QStringList & capaNormal,
                                          const QStringList & capaSSL )
{
  mImap.checkCapabilities->setEnabled( true );
  mCapaNormal = imapCapabilitiesFromStringList( capaNormal );
  if ( mCapaNormal & STARTTLS )
    mCapaTLS = mCapaNormal;
  else
    mCapaTLS = 0;
  mCapaSSL = imapCapabilitiesFromStringList( capaSSL );
  kdDebug(5006) << "mCapaNormal = " << mCapaNormal
                << "; mCapaSSL = " << mCapaSSL
                << "; mCapaTLS = " << mCapaTLS << endl;
  mImap.encryptionNone->setEnabled( !capaNormal.isEmpty() );
  mImap.encryptionSSL->setEnabled( !capaSSL.isEmpty() );
  mImap.encryptionTLS->setEnabled( mCapaTLS != 0 );
  checkHighest( mImap.encryptionGroup );
  delete mServerTest;
  mServerTest = 0;
}

void AccountDialog::slotLeaveOnServerDaysChanged ( int value )
{
  mPop.leaveOnServerDaysSpin->setSuffix( i18n(" day", " days", value) );
}


void AccountDialog::slotLeaveOnServerCountChanged ( int value )
{
  mPop.leaveOnServerCountSpin->setSuffix( i18n(" message", " messages", value) );
}


void AccountDialog::slotFilterOnServerSizeChanged ( int value )
{
  mPop.filterOnServerSizeSpin->setSuffix( i18n(" byte", " bytes", value) );
}


void AccountDialog::enableImapAuthMethods( unsigned int capa )
{
  kdDebug(5006) << "enableImapAuthMethods( " << capa << " )" << endl;
  mImap.authPlain->setEnabled( capa & Plain );
  mImap.authLogin->setEnabled( capa & Login );
  mImap.authCramMd5->setEnabled( capa & CRAM_MD5 );
  mImap.authDigestMd5->setEnabled( capa & Digest_MD5 );
  mImap.authNTLM->setEnabled( capa & NTLM );
  mImap.authGSSAPI->setEnabled( capa & GSSAPI );
  mImap.authAnonymous->setEnabled( capa & Anonymous );
}


void AccountDialog::checkHighest( QButtonGroup *btnGroup )
{
  kdDebug(5006) << "checkHighest( " << btnGroup << " )" << endl;
  for ( int i = btnGroup->count() - 1; i >= 0 ; --i ) {
    QButton * btn = btnGroup->find( i );
    if ( btn && btn->isEnabled() ) {
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
        acctLocal->setLockType(lock_none);
      else acctLocal->setLockType(FCNTL);
    }

    mAccount->setCheckInterval( mLocal.intervalCheck->isChecked() ?
			     mLocal.intervalSpin->value() : 0 );
#if 0
    mAccount->setResource( mLocal.resourceCheck->isChecked() );
#endif
    mAccount->setCheckExclude( !mLocal.includeInCheck->isChecked() );

    mAccount->setPrecommand( mLocal.precommand->text() );

    mAccount->setFolder( *mFolderList.at(mLocal.folderCombo->currentItem()) );

    mAccount->setIdentityId( mLocal.identityCombo->currentIdentity() );

  }
  else if( accountType == "pop" )
  {
    mAccount->setName( mPop.nameEdit->text() );
    mAccount->setCheckInterval( mPop.intervalCheck->isChecked() ?
			     mPop.intervalSpin->value() : 0 );
#if 0
    mAccount->setResource( mPop.resourceCheck->isChecked() );
#endif
    mAccount->setCheckExclude( !mPop.includeInCheck->isChecked() );

    mAccount->setFolder( *mFolderList.at(mPop.folderCombo->currentItem()) );

    mAccount->setIdentityId( mPop.identityCombo->currentIdentity() );

    initAccountForConnect();
    PopAccount &epa = *(PopAccount*)mAccount;
    epa.setUsePipelining( mPop.usePipeliningCheck->isChecked() );
    epa.setLeaveOnServer( mPop.leaveOnServerCheck->isChecked() );
    epa.setLeaveOnServerDays( mPop.leaveOnServerCheck->isChecked() ?
                              ( mPop.leaveOnServerDaysCheck->isChecked() ?
                                mPop.leaveOnServerDaysSpin->value() : -1 ) : 0);
    epa.setLeaveOnServerCount( mPop.leaveOnServerCheck->isChecked() ?
                               ( mPop.leaveOnServerCountCheck->isChecked() ?
                                 mPop.leaveOnServerCountSpin->value() : -1 ) : 0 );
    epa.setLeaveOnServerSize( mPop.leaveOnServerCheck->isChecked() ?
                              ( mPop.leaveOnServerSizeCheck->isChecked() ?
                                mPop.leaveOnServerSizeSpin->value() : -1 ) : 0 );
    epa.setFilterOnServer( mPop.filterOnServerCheck->isChecked() );
    epa.setFilterOnServerCheckSize (mPop.filterOnServerSizeSpin->value() );
    epa.setPrecommand( mPop.precommand->text() );

  }
  else if( accountType == "imap" )
  {
    mAccount->setName( mImap.nameEdit->text() );
    mAccount->setCheckInterval( mImap.intervalCheck->isChecked() ?
                                mImap.intervalSpin->value() : 0 );
    mAccount->setIdentityId( mImap.identityCombo->currentIdentity() );

#if 0
    mAccount->setResource( mImap.resourceCheck->isChecked() );
#endif
    mAccount->setCheckExclude( !mImap.includeInCheck->isChecked() );
    mAccount->setFolder( kmkernel->imapFolderMgr()->findById(mAccount->id()) );

    initAccountForConnect();
    KMAcctImap &epa = *(KMAcctImap*)mAccount;
    epa.setAutoExpunge( mImap.autoExpungeCheck->isChecked() );
    epa.setHiddenFolders( mImap.hiddenFoldersCheck->isChecked() );
    epa.setOnlySubscribedFolders( mImap.subscribedFoldersCheck->isChecked() );
    epa.setOnlyLocallySubscribedFolders( mImap.locallySubscribedFoldersCheck->isChecked() );
    epa.setLoadOnDemand( mImap.loadOnDemandCheck->isChecked() );
    epa.setListOnlyOpenFolders( mImap.listOnlyOpenCheck->isChecked() );
    KMFolder *t = mImap.trashCombo->folder();
    if ( t )
      epa.setTrash( mImap.trashCombo->folder()->idString() );
    else
      epa.setTrash( kmkernel->trashFolder()->idString() );
#if 0
    epa.setResource( mImap.resourceCheck->isChecked() );
#endif
    epa.setCheckExclude( !mImap.includeInCheck->isChecked() );
    if ( mSieveConfigEditor )
      epa.setSieveConfig( mSieveConfigEditor->config() );
  }
  else if( accountType == "cachedimap" )
  {
    mAccount->setName( mImap.nameEdit->text() );
    mAccount->setCheckInterval( mImap.intervalCheck->isChecked() ?
                                mImap.intervalSpin->value() : 0 );
    mAccount->setIdentityId( mImap.identityCombo->currentIdentity() );

#if 0
    mAccount->setResource( mImap.resourceCheck->isChecked() );
#endif
    mAccount->setCheckExclude( !mImap.includeInCheck->isChecked() );
    //mAccount->setFolder( NULL );
    mAccount->setFolder( kmkernel->dimapFolderMgr()->findById(mAccount->id()) );
    //kdDebug(5006) << "account for folder " << mAccount->folder()->name() << endl;

    initAccountForConnect();
    KMAcctCachedImap &epa = *(KMAcctCachedImap*)mAccount;
    epa.setHiddenFolders( mImap.hiddenFoldersCheck->isChecked() );
    epa.setOnlySubscribedFolders( mImap.subscribedFoldersCheck->isChecked() );
    epa.setOnlyLocallySubscribedFolders( mImap.locallySubscribedFoldersCheck->isChecked() );
    epa.setStorePasswd( mImap.storePasswordCheck->isChecked() );
    epa.setPasswd( mImap.passwordEdit->text(), epa.storePasswd() );
    KMFolder *t = mImap.trashCombo->folder();
    if ( t )
      epa.setTrash( mImap.trashCombo->folder()->idString() );
    else
      epa.setTrash( kmkernel->trashFolder()->idString() );
#if 0
    epa.setResource( mImap.resourceCheck->isChecked() );
#endif
    epa.setCheckExclude( !mImap.includeInCheck->isChecked() );
    if ( mSieveConfigEditor )
      epa.setSieveConfig( mSieveConfigEditor->config() );
  }
  else if( accountType == "maildir" )
  {
    KMAcctMaildir *acctMaildir = dynamic_cast<KMAcctMaildir*>(mAccount);

    if (acctMaildir) {
        mAccount->setName( mMaildir.nameEdit->text() );
        acctMaildir->setLocation( mMaildir.locationEdit->currentText() );

        KMFolder *targetFolder = *mFolderList.at(mMaildir.folderCombo->currentItem());
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
    mAccount->setCheckInterval( mMaildir.intervalCheck->isChecked() ?
			     mMaildir.intervalSpin->value() : 0 );
#if 0
    mAccount->setResource( mMaildir.resourceCheck->isChecked() );
#endif
    mAccount->setCheckExclude( !mMaildir.includeInCheck->isChecked() );

    mAccount->setPrecommand( mMaildir.precommand->text() );

    mAccount->setIdentityId( mMaildir.identityCombo->currentIdentity() );
  }

  if ( accountType == "imap" || accountType == "cachedimap" )
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
      for ( it2 = it.data().begin(); it2 != it.data().end(); ++it2 ) {
        list << it2.key();
        delimMap[it2.key()] = it2.data();
      }
      map[it.key()] = list;
    }
    ai.setNamespaces( map );
    ai.setNamespaceToDelimiter( delimMap );
  }

  kmkernel->acctMgr()->writeConfig( true );
  // get the new account and register the new destination folder
  // this is the target folder for local or pop accounts and the root folder
  // of the account for (d)imap
  KMAccount* newAcct = kmkernel->acctMgr()->find(mAccount->id());
  if (newAcct)
  {
    if( accountType == "local" ) {
      newAcct->setFolder( *mFolderList.at(mLocal.folderCombo->currentItem()), true );
    } else if ( accountType == "pop" ) {
      newAcct->setFolder( *mFolderList.at(mPop.folderCombo->currentItem()), true );
    } else if ( accountType == "maildir" ) {
      newAcct->setFolder( *mFolderList.at(mMaildir.folderCombo->currentItem()), true );
    } else if ( accountType == "imap" ) {
      newAcct->setFolder( kmkernel->imapFolderMgr()->findById(mAccount->id()), true );
    } else if ( accountType == "cachedimap" ) {
      newAcct->setFolder( kmkernel->dimapFolderMgr()->findById(mAccount->id()), true );
    }
  }
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
    KMessageBox::sorry( 0, i18n( "Only local files are currently supported." ) );
    return;
  }

  mLocal.locationEdit->setEditText( url.path() );
  directory = url.directory();
}

void AccountDialog::slotMaildirChooser()
{
  static QString directory( "/" );

  QString dir = KFileDialog::getExistingDirectory(directory, this, i18n("Choose Location"));

  if( dir.isEmpty() )
    return;

  mMaildir.locationEdit->setEditText( dir );
  directory = dir;
}

void AccountDialog::slotEnableLeaveOnServerDays( bool state )
{
  if ( state && !mPop.leaveOnServerDaysCheck->isEnabled()) return;
  mPop.leaveOnServerDaysSpin->setEnabled( state );
}

void AccountDialog::slotEnableLeaveOnServerCount( bool state )
{
  if ( state && !mPop.leaveOnServerCountCheck->isEnabled()) return;
  mPop.leaveOnServerCountSpin->setEnabled( state );
  return;
}

void AccountDialog::slotEnableLeaveOnServerSize( bool state )
{
  if ( state && !mPop.leaveOnServerSizeCheck->isEnabled()) return;
  mPop.leaveOnServerSizeSpin->setEnabled( state );
  return;
}

void AccountDialog::slotEnablePopInterval( bool state )
{
  mPop.intervalSpin->setEnabled( state );
  mPop.intervalLabel->setEnabled( state );
}

void AccountDialog::slotEnableImapInterval( bool state )
{
  mImap.intervalSpin->setEnabled( state );
  mImap.intervalLabel->setEnabled( state );
}

void AccountDialog::slotEnableLocalInterval( bool state )
{
  mLocal.intervalSpin->setEnabled( state );
  mLocal.intervalLabel->setEnabled( state );
}

void AccountDialog::slotEnableMaildirInterval( bool state )
{
  mMaildir.intervalSpin->setEnabled( state );
  mMaildir.intervalLabel->setEnabled( state );
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

#if 0
void AccountDialog::slotClearResourceAllocations()
{
    mAccount->clearIntervals();
}


void AccountDialog::slotClearPastResourceAllocations()
{
    mAccount->clearOldIntervals();
}
#endif

void AccountDialog::slotReloadNamespaces()
{
  if ( mAccount->type() == "imap" || mAccount->type() == "cachedimap" )
  {
    initAccountForConnect();
    mImap.personalNS->setText( i18n("Fetching Namespaces...") );
    mImap.otherUsersNS->setText( QString::null );
    mImap.sharedNS->setText( QString::null );
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
    mImap.personalNS->setText( QString::null );
  }
}

void AccountDialog::slotSetupNamespaces( const ImapAccountBase::nsDelimMap& map )
{
  disconnect( this, SLOT( slotSetupNamespaces( const ImapAccountBase::nsDelimMap& ) ) );
  mImap.personalNS->setText( QString::null );
  mImap.otherUsersNS->setText( QString::null );
  mImap.sharedNS->setText( QString::null );
  mImap.nsMap = map;

  ImapAccountBase::namespaceDelim ns = map[ImapAccountBase::PersonalNS];
  ImapAccountBase::namespaceDelim::ConstIterator it;
  if ( !ns.isEmpty() ) {
    mImap.personalNS->setText( namespaceListToString( ns.keys() ) );
    mImap.editPNS->setEnabled( true );
  } else {
    mImap.editPNS->setEnabled( false );
  }
  ns = map[ImapAccountBase::OtherUsersNS];
  if ( !ns.isEmpty() ) {
    mImap.otherUsersNS->setText( namespaceListToString( ns.keys() ) );
    mImap.editONS->setEnabled( true );
  } else {
    mImap.editONS->setEnabled( false );
  }
  ns = map[ImapAccountBase::SharedNS];
  if ( !ns.isEmpty() ) {
    mImap.sharedNS->setText( namespaceListToString( ns.keys() ) );
    mImap.editSNS->setEnabled( true );
  } else {
    mImap.editSNS->setEnabled( false );
  }
}

const QString AccountDialog::namespaceListToString( const QStringList& list )
{
  QStringList myList = list;
  for ( QStringList::Iterator it = myList.begin(); it != myList.end(); ++it ) {
    if ( (*it).isEmpty() ) {
      (*it) = "<" + i18n("Empty") + ">";
    }
  }
  return myList.join(",");
}

void AccountDialog::initAccountForConnect()
{
  QString type = mAccount->type();
  if ( type == "local" )
    return;

  NetworkAccount &na = *(NetworkAccount*)mAccount;

  if ( type == "pop" ) {
    na.setHost( mPop.hostEdit->text().stripWhiteSpace() );
    na.setPort( mPop.portEdit->text().toInt() );
    na.setLogin( mPop.loginEdit->text().stripWhiteSpace() );
    na.setStorePasswd( mPop.storePasswordCheck->isChecked() );
    na.setPasswd( mPop.passwordEdit->text(), na.storePasswd() );
    na.setUseSSL( mPop.encryptionSSL->isChecked() );
    na.setUseTLS( mPop.encryptionTLS->isChecked() );
    if (mPop.authUser->isChecked())
      na.setAuth("USER");
    else if (mPop.authLogin->isChecked())
      na.setAuth("LOGIN");
    else if (mPop.authPlain->isChecked())
      na.setAuth("PLAIN");
    else if (mPop.authCRAM_MD5->isChecked())
      na.setAuth("CRAM-MD5");
    else if (mPop.authDigestMd5->isChecked())
      na.setAuth("DIGEST-MD5");
    else if (mPop.authNTLM->isChecked())
      na.setAuth("NTLM");
    else if (mPop.authGSSAPI->isChecked())
      na.setAuth("GSSAPI");
    else if (mPop.authAPOP->isChecked())
      na.setAuth("APOP");
    else na.setAuth("AUTO");
  }
  else if ( type == "imap" || type == "cachedimap" ) {
    na.setHost( mImap.hostEdit->text().stripWhiteSpace() );
    na.setPort( mImap.portEdit->text().toInt() );
    na.setLogin( mImap.loginEdit->text().stripWhiteSpace() );
    na.setStorePasswd( mImap.storePasswordCheck->isChecked() );
    na.setPasswd( mImap.passwordEdit->text(), na.storePasswd() );
    na.setUseSSL( mImap.encryptionSSL->isChecked() );
    na.setUseTLS( mImap.encryptionTLS->isChecked() );
    if (mImap.authCramMd5->isChecked())
      na.setAuth("CRAM-MD5");
    else if (mImap.authDigestMd5->isChecked())
      na.setAuth("DIGEST-MD5");
    else if (mImap.authNTLM->isChecked())
      na.setAuth("NTLM");
    else if (mImap.authGSSAPI->isChecked())
      na.setAuth("GSSAPI");
    else if (mImap.authAnonymous->isChecked())
      na.setAuth("ANONYMOUS");
    else if (mImap.authLogin->isChecked())
      na.setAuth("LOGIN");
    else if (mImap.authPlain->isChecked())
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
  : KDialogBase( parent, "edit_namespace", false, QString::null,
      Ok|Cancel, Ok, true ), mType( type ), mNamespaceMap( map )
{
  QVBox *page = makeVBoxMainWidget();

  QString ns;
  if ( mType == ImapAccountBase::PersonalNS ) {
    ns = i18n("Personal");
  } else if ( mType == ImapAccountBase::OtherUsersNS ) {
    ns = i18n("Other Users");
  } else {
    ns = i18n("Shared");
  }
  setCaption( i18n("Edit Namespace '%1'").arg(ns) );
  QGrid* grid = new QGrid( 2, page );

  mBg = new QButtonGroup( 0 );
  connect( mBg, SIGNAL( clicked(int) ), this, SLOT( slotRemoveEntry(int) ) );
  mDelimMap = mNamespaceMap->find( mType ).data();
  ImapAccountBase::namespaceDelim::Iterator it;
  for ( it = mDelimMap.begin(); it != mDelimMap.end(); ++it ) {
    NamespaceLineEdit* edit = new NamespaceLineEdit( grid );
    edit->setText( it.key() );
    QToolButton* button = new QToolButton( grid );
    button->setIconSet(
      KGlobal::iconLoader()->loadIconSet( "editdelete", KIcon::Small, 0 ) );
    button->setAutoRaise( true );
    button->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );
    button->setFixedSize( 22, 22 );
    mLineEditMap[ mBg->insert( button ) ] = edit;
  }
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
    delete edit;
  }
  if ( mBg->find( id ) ) {
    // delete the button
    delete mBg->find( id );
  }
  adjustSize();
}

void NamespaceEditDialog::slotOk()
{
  QMap<int, NamespaceLineEdit*>::Iterator it;
  for ( it = mLineEditMap.begin(); it != mLineEditMap.end(); ++it ) {
    NamespaceLineEdit* edit = it.data();
    if ( edit->isModified() ) {
      // register delimiter for new namespace
      mDelimMap[edit->text()] = mDelimMap[edit->lastText()];
      mDelimMap.remove( edit->lastText() );
    }
  }
  mNamespaceMap->replace( mType, mDelimMap );
  KDialogBase::slotOk();
}

} // namespace KMail

#include "accountdialog.moc"
