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
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
#include <config.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <klineedit.h>
#include <qlayout.h>
#include <qtabwidget.h>
#include <qradiobutton.h>
#include <qvalidator.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qwhatsthis.h>
#include <qhbox.h>

#include <kfiledialog.h>
#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <knuminput.h>
#include <kseparator.h>
#include <kapplication.h>

#include <netdb.h>
#include <netinet/in.h>

#include "accountdialog.h"
#include "sieveconfig.h"
using KMail::SieveConfig;
using KMail::SieveConfigEditor;
#include "kmacctmaildir.h"
#include "kmacctlocal.h"
#include "kmacctmgr.h"
#include "kmacctexppop.h"
#include "kmacctimap.h"
#include "kmacctcachedimap.h"
#include "kmfoldermgr.h"
#include "kmservertest.h"

#include <cassert>
#include <stdlib.h>

#ifdef HAVE_PATHS_H
#include <paths.h>	/* defines _PATH_MAILDIR */
#endif

#ifndef _PATH_MAILDIR
#define _PATH_MAILDIR "/var/spool/mail"
#endif
#undef None

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
    mAccount(account), mSieveConfigEditor( 0 )
{
  mValidator = new QRegExpValidator( QRegExp( "[A-Za-z0-9-_:.]*" ), 0 );
  mServerTest = 0;
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
    QString msg = i18n( "Account type is not supported" );
    KMessageBox::information( topLevelWidget(),msg,i18n("Configure Account") );
    return;
  }

  setupSettings();
}

AccountDialog::~AccountDialog()
{
    delete mValidator;
    mValidator = 0L;
    delete mServerTest;
    mServerTest = 0L;
}

void AccountDialog::makeLocalAccountPage()
{
  ProcmailRCParser procmailrcParser;
  QFrame *page = makeMainWidget();
  QGridLayout *topLayout = new QGridLayout( page, 12, 3, 0, spacingHint() );
  topLayout->addColSpacing( 1, fontMetrics().maxWidth()*15 );
  topLayout->setRowStretch( 11, 10 );
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
  mLocal.nameEdit = new KLineEdit( page );
  label->setBuddy( mLocal.nameEdit );
  topLayout->addWidget( mLocal.nameEdit, 2, 1 );

  label = new QLabel( i18n("&Location:"), page );
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

  mLocal.lockProcmail = new QRadioButton( i18n("Procmail loc&kfile"), group);
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

  mLocal.excludeCheck =
    new QCheckBox( i18n("E&xclude from \"Check Mail\""), page );
  topLayout->addMultiCellWidget( mLocal.excludeCheck, 5, 5, 0, 2 );

  mLocal.intervalCheck =
    new QCheckBox( i18n("Enable &interval mail checking"), page );
  topLayout->addMultiCellWidget( mLocal.intervalCheck, 6, 6, 0, 2 );
  connect( mLocal.intervalCheck, SIGNAL(toggled(bool)),
	   this, SLOT(slotEnableLocalInterval(bool)) );
  mLocal.intervalLabel = new QLabel( i18n("Check inter&val:"), page );
  topLayout->addWidget( mLocal.intervalLabel, 7, 0 );
  mLocal.intervalSpin = new KIntNumInput( page );
  mLocal.intervalLabel->setBuddy( mLocal.intervalSpin );
  mLocal.intervalSpin->setRange( 1, 10000, 1, FALSE );
  mLocal.intervalSpin->setSuffix( i18n(" min") );
  mLocal.intervalSpin->setValue( 1 );
  topLayout->addWidget( mLocal.intervalSpin, 7, 1 );

  label = new QLabel( i18n("&Destination folder:"), page );
  topLayout->addWidget( label, 8, 0 );
  mLocal.folderCombo = new QComboBox( false, page );
  label->setBuddy( mLocal.folderCombo );
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

  label = new QLabel( i18n("&Pre-command:"), page );
  topLayout->addWidget( label, 9, 0 );
  mLocal.precommand = new KLineEdit( page );
  label->setBuddy( mLocal.precommand );
  topLayout->addWidget( mLocal.precommand, 9, 1 );

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

  mMaildir.titleLabel = new QLabel( i18n("Account type: Maildir account"), page );
  topLayout->addMultiCellWidget( mMaildir.titleLabel, 0, 0, 0, 2 );
  QFont titleFont( mMaildir.titleLabel->font() );
  titleFont.setBold( true );
  mMaildir.titleLabel->setFont( titleFont );
  QFrame *hline = new QFrame( page );
  hline->setFrameStyle( QFrame::Sunken | QFrame::HLine );
  topLayout->addMultiCellWidget( hline, 1, 1, 0, 2 );

  mMaildir.nameEdit = new KLineEdit( page );
  topLayout->addWidget( mMaildir.nameEdit, 2, 1 );
  QLabel *label = new QLabel( mMaildir.nameEdit, i18n("&Name:"), page );
  topLayout->addWidget( label, 2, 0 );

  mMaildir.locationEdit = new QComboBox( true, page );
  topLayout->addWidget( mMaildir.locationEdit, 3, 1 );
  mMaildir.locationEdit->insertStringList(procmailrcParser.getSpoolFilesList());
  label = new QLabel( mMaildir.locationEdit, i18n("&Location:"), page );
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

  mMaildir.excludeCheck =
    new QCheckBox( i18n("E&xclude from \"Check Mail\""), page );
  topLayout->addMultiCellWidget( mMaildir.excludeCheck, 4, 4, 0, 2 );

  mMaildir.intervalCheck =
    new QCheckBox( i18n("Enable &interval mail checking"), page );
  topLayout->addMultiCellWidget( mMaildir.intervalCheck, 5, 5, 0, 2 );
  connect( mMaildir.intervalCheck, SIGNAL(toggled(bool)),
	   this, SLOT(slotEnableMaildirInterval(bool)) );
  mMaildir.intervalLabel = new QLabel( i18n("Check inter&val:"), page );
  topLayout->addWidget( mMaildir.intervalLabel, 6, 0 );
  mMaildir.intervalSpin = new KIntNumInput( page );
  mMaildir.intervalSpin->setRange( 1, 10000, 1, FALSE );
  mMaildir.intervalSpin->setSuffix( i18n(" min") );
  mMaildir.intervalSpin->setValue( 1 );
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

  connect(kapp,SIGNAL(kdisplayFontChanged()),SLOT(slotFontChanged()));
}


void AccountDialog::makePopAccountPage()
{
  QFrame *page = makeMainWidget();
  QVBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );

  mPop.titleLabel = new QLabel( page );
  mPop.titleLabel->setText( i18n("Account type: POP Account") );
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

  QLabel *label = new QLabel( i18n("&Name:"), page1 );
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
    new QCheckBox( i18n("Sto&re POP password in configuration file"), page1 );
  grid->addMultiCellWidget( mPop.storePasswordCheck, 5, 5, 0, 1 );

  mPop.deleteMailCheck =
    new QCheckBox( i18n("&Delete message from server after fetching"), page1 );
  grid->addMultiCellWidget( mPop.deleteMailCheck, 6, 6, 0, 1 );

#if 0
  QHBox* resourceHB = new QHBox( page1 );
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
  grid->addMultiCellWidget( resourceHB, 7, 7, 0, 2 );
#endif

  mPop.excludeCheck =
    new QCheckBox( i18n("E&xclude from \"Check Mail\""), page1 );
  grid->addMultiCellWidget( mPop.excludeCheck, 7, 7, 0, 1 );

  QHBox * hbox = new QHBox( page1 );
  hbox->setSpacing( KDialog::spacingHint() );
  mPop.filterOnServerCheck =
    new QCheckBox( i18n("&Filter messages if they are greater than"), hbox );
  mPop.filterOnServerSizeSpin = new KIntNumInput ( hbox );
  mPop.filterOnServerSizeSpin->setEnabled( false );
  hbox->setStretchFactor( mPop.filterOnServerSizeSpin, 1 );
  mPop.filterOnServerSizeSpin->setRange( 1, 10000000, 100, FALSE );
  mPop.filterOnServerSizeSpin->setValue( 50000 );
  mPop.filterOnServerSizeSpin->setSuffix( i18n(" byte") );
  grid->addMultiCellWidget( hbox, 8, 8, 0, 1 );
  connect( mPop.filterOnServerCheck, SIGNAL(toggled(bool)),
	   mPop.filterOnServerSizeSpin, SLOT(setEnabled(bool)) );
  QString msg = i18n("If you select this option, POP Filters will be used to "
		     "decide what to do with messages. You can then select "
		     "to download, delete or keep them on the server." );
  QWhatsThis::add( mPop.filterOnServerCheck, msg );
  QWhatsThis::add( mPop.filterOnServerSizeSpin, msg );

  mPop.intervalCheck =
    new QCheckBox( i18n("Enable &interval mail checking"), page1 );
  grid->addMultiCellWidget( mPop.intervalCheck, 9, 9, 0, 1 );
  connect( mPop.intervalCheck, SIGNAL(toggled(bool)),
	   this, SLOT(slotEnablePopInterval(bool)) );
  mPop.intervalLabel = new QLabel( i18n("Check inter&val:"), page1 );
  grid->addWidget( mPop.intervalLabel, 10, 0 );
  mPop.intervalSpin = new KIntNumInput( page1 );
  mPop.intervalSpin->setRange( 1, 10000, 1, FALSE );
  mPop.intervalSpin->setSuffix( i18n(" min") );
  mPop.intervalSpin->setValue( 1 );
  mPop.intervalLabel->setBuddy( mPop.intervalSpin );
  grid->addWidget( mPop.intervalSpin, 10, 1 );

  label = new QLabel( i18n("Des&tination folder:"), page1 );
  grid->addWidget( label, 11, 0 );
  mPop.folderCombo = new QComboBox( false, page1 );
  label->setBuddy( mPop.folderCombo );
  grid->addWidget( mPop.folderCombo, 11, 1 );

  label = new QLabel( i18n("Precom&mand:"), page1 );
  grid->addWidget( label, 12, 0 );
  mPop.precommand = new KLineEdit( page1 );
  label->setBuddy(mPop.precommand);
  grid->addWidget( mPop.precommand, 12, 1 );

  QWidget *page2 = new QWidget( tabWidget );
  tabWidget->addTab( page2, i18n("&Extras") );
  QVBoxLayout *vlay = new QVBoxLayout( page2, marginHint(), spacingHint() );

  mPop.usePipeliningCheck =
    new QCheckBox( i18n("&Use pipelining for faster mail download"), page2 );
  connect(mPop.usePipeliningCheck, SIGNAL(clicked()),
    SLOT(slotPipeliningClicked()));
  vlay->addWidget( mPop.usePipeliningCheck );

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
  mPop.authUser = new QRadioButton( i18n("Clear te&xt") , mPop.authGroup );
  mPop.authLogin = new QRadioButton( i18n("Please translate this "
    "authentication method only if you have a good reason", "&LOGIN"),
    mPop.authGroup );
  mPop.authPlain = new QRadioButton( i18n("Please translate this "
    "authentication method only if you have a good reason", "&PLAIN"),
    mPop.authGroup  );
  mPop.authCRAM_MD5 = new QRadioButton( i18n("CRAM-MD&5"), mPop.authGroup );
  mPop.authDigestMd5 = new QRadioButton( i18n("&DIGEST-MD5"), mPop.authGroup );
  mPop.authAPOP = new QRadioButton( i18n("&APOP"), mPop.authGroup );
  vlay->addWidget( mPop.authGroup );

  vlay->addStretch();

  QHBoxLayout *buttonLay = new QHBoxLayout( vlay );
  mPop.checkCapabilities =
    new QPushButton( i18n("Check &What the Server Supports"), page2 );
  connect(mPop.checkCapabilities, SIGNAL(clicked()),
    SLOT(slotCheckPopCapabilities()));
  buttonLay->addStretch();
  buttonLay->addWidget( mPop.checkCapabilities );

  connect(kapp,SIGNAL(kdisplayFontChanged()),SLOT(slotFontChanged()));
}


void AccountDialog::makeImapAccountPage( bool connected )
{
  QFrame *page = makeMainWidget();
  QVBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );

  mImap.titleLabel = new QLabel( page );
  if( connected )
    mImap.titleLabel->setText( i18n("Account type: Disconnected Imap Account") );
  else
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

  int row = -1;
  QGridLayout *grid = new QGridLayout( page1, 15, 2, marginHint(), spacingHint() );
  grid->addColSpacing( 1, fontMetrics().maxWidth()*15 );
  grid->setRowStretch( 15, 10 );
  grid->setColStretch( 1, 10 );

  ++row;
  QLabel *label = new QLabel( i18n("&Name:"), page1 );
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

  ++row;
  label = new QLabel( i18n("Prefix to fol&ders:"), page1 );
  grid->addWidget( label, row, 0 );
  mImap.prefixEdit = new KLineEdit( page1 );
  label->setBuddy( mImap.prefixEdit );
  grid->addWidget( mImap.prefixEdit, row, 1 );

  ++row;
  mImap.storePasswordCheck =
    new QCheckBox( i18n("Sto&re IMAP password in configuration file"), page1 );
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

  if( connected ) {
    ++row;
    mImap.progressDialogCheck = new QCheckBox( i18n("Show &progress window"), page1);
    grid->addMultiCellWidget( mImap.progressDialogCheck, row, row, 0, 1 );
  }

  ++row;
  mImap.subscribedFoldersCheck = new QCheckBox(
    i18n("Show only s&ubscribed folders"), page1);
  grid->addMultiCellWidget( mImap.subscribedFoldersCheck, row, row, 0, 1 );

  if ( !connected ) {
    // not implemented for disconnected yet
    ++row;
    mImap.loadOnDemandCheck = new QCheckBox(
        i18n("Load attach&ments on demand"), page1);
    grid->addMultiCellWidget( mImap.loadOnDemandCheck, row, row, 0, 1 );
  }

  ++row;
#if 0
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
  mImap.excludeCheck =
    new QCheckBox( i18n("E&xclude from \"Check Mail\""), page1 );
  grid->addMultiCellWidget( mImap.excludeCheck, row, row, 0, 1 );

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
  mImap.intervalSpin->setRange( 1, 60, 1, FALSE );
  mImap.intervalSpin->setValue( 1 );
  mImap.intervalSpin->setSuffix( i18n( " min" ) );
  mImap.intervalLabel->setBuddy( mImap.intervalSpin );
  grid->addWidget( mImap.intervalSpin, row, 1 );

  ++row;
  mImap.trashCombo = new KMFolderComboBox( page1 );
  mImap.trashCombo->showOutboxFolder( FALSE );
  grid->addWidget( mImap.trashCombo, row, 1 );
  grid->addWidget( new QLabel( mImap.trashCombo, i18n("&Trash folder:"), page1 ), row, 0 );

  QWidget *page2 = new QWidget( tabWidget );
  tabWidget->addTab( page2, i18n("S&ecurity") );
  QVBoxLayout *vlay = new QVBoxLayout( page2, marginHint(), spacingHint() );

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
  mImap.authAnonymous = new QRadioButton( i18n("&Anonymous"), mImap.authGroup );
  vlay->addWidget( mImap.authGroup );

  vlay->addStretch();

  QHBoxLayout *buttonLay = new QHBoxLayout( vlay );
  mImap.checkCapabilities =
    new QPushButton( i18n("Check &What the Server Supports"), page2 );
  connect(mImap.checkCapabilities, SIGNAL(clicked()),
    SLOT(slotCheckImapCapabilities()));
  buttonLay->addStretch();
  buttonLay->addWidget( mImap.checkCapabilities );

#if 0 // ### (marc) this isn't ready for prime-time yet... Reactivate post-3.2.
  mSieveConfigEditor = new SieveConfigEditor( tabWidget );
  mSieveConfigEditor->layout()->setMargin( KDialog::marginHint() );
  tabWidget->addTab( mSieveConfigEditor, i18n("&Filtering") );
#endif

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
#if 0
    mLocal.resourceCheck->setChecked( mAccount->resource() );
#endif
    mLocal.excludeCheck->setChecked( mAccount->checkExclude() );
    mLocal.precommand->setText( mAccount->precommand() );

    slotEnableLocalInterval( interval >= 1 );
    folderCombo = mLocal.folderCombo;
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
    mPop.usePipeliningCheck->setChecked( ap.usePipelining() );
    mPop.storePasswordCheck->setChecked( ap.storePasswd() );
    mPop.deleteMailCheck->setChecked( !ap.leaveOnServer() );
    mPop.filterOnServerCheck->setChecked( ap.filterOnServer() );
    mPop.filterOnServerSizeSpin->setValue( ap.filterOnServerCheckSize() );
    mPop.intervalCheck->setChecked( interval >= 1 );
    mPop.intervalSpin->setValue( QMAX(1, interval) );
#if 0
    mPop.resourceCheck->setChecked( mAccount->resource() );
#endif
    mPop.excludeCheck->setChecked( mAccount->checkExclude() );
    mPop.precommand->setText( ap.precommand() );
    if (ap.useSSL())
      mPop.encryptionSSL->setChecked( TRUE );
    else if (ap.useTLS())
      mPop.encryptionTLS->setChecked( TRUE );
    else mPop.encryptionNone->setChecked( TRUE );
    if (ap.auth() == "LOGIN")
      mPop.authLogin->setChecked( TRUE );
    else if (ap.auth() == "PLAIN")
      mPop.authPlain->setChecked( TRUE );
    else if (ap.auth() == "CRAM-MD5")
      mPop.authCRAM_MD5->setChecked( TRUE );
    else if (ap.auth() == "DIGEST-MD5")
      mPop.authDigestMd5->setChecked( TRUE );
    else if (ap.auth() == "APOP")
      mPop.authAPOP->setChecked( TRUE );
    else mPop.authUser->setChecked( TRUE );

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
    QString prefix = ai.prefix();
    if (!prefix.isEmpty() && prefix[0] == '/') prefix = prefix.mid(1);
    if (!prefix.isEmpty() && prefix[prefix.length() - 1] == '/')
      prefix = prefix.left(prefix.length() - 1);
    mImap.prefixEdit->setText( prefix );
    mImap.autoExpungeCheck->setChecked( ai.autoExpunge() );
    mImap.hiddenFoldersCheck->setChecked( ai.hiddenFolders() );
    mImap.subscribedFoldersCheck->setChecked( ai.onlySubscribedFolders() );
    mImap.loadOnDemandCheck->setChecked( ai.loadOnDemand() );
    mImap.storePasswordCheck->setChecked( ai.storePasswd() );
    mImap.intervalCheck->setChecked( interval >= 1 );
    mImap.intervalSpin->setValue( QMAX(1, interval) );
#if 0
    mImap.resourceCheck->setChecked( ai.resource() );
#endif
    mImap.excludeCheck->setChecked( ai.checkExclude() );
    mImap.intervalCheck->setChecked( interval >= 1 );
    mImap.intervalSpin->setValue( QMAX(1, interval) );
    QString trashfolder = ai.trash();
    if (trashfolder.isEmpty())
      trashfolder = kmkernel->trashFolder()->idString();
    mImap.trashCombo->setFolder( trashfolder );
    slotEnableImapInterval( interval >= 1 );
    if (ai.useSSL())
      mImap.encryptionSSL->setChecked( TRUE );
    else if (ai.useTLS())
      mImap.encryptionTLS->setChecked( TRUE );
    else mImap.encryptionNone->setChecked( TRUE );
    if (ai.auth() == "CRAM-MD5")
      mImap.authCramMd5->setChecked( TRUE );
    else if (ai.auth() == "DIGEST-MD5")
      mImap.authDigestMd5->setChecked( TRUE );
    else if (ai.auth() == "ANONYMOUS")
      mImap.authAnonymous->setChecked( TRUE );
    else if (ai.auth() == "PLAIN")
      mImap.authPlain->setChecked( TRUE );
    else if (ai.auth() == "LOGIN")
      mImap.authLogin->setChecked( TRUE );
    else mImap.authUser->setChecked( TRUE );
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
    QString prefix = ai.prefix();
    if (!prefix.isEmpty() && prefix[0] == '/') prefix = prefix.mid(1);
    if (!prefix.isEmpty() && prefix[prefix.length() - 1] == '/')
      prefix = prefix.left(prefix.length() - 1);
    mImap.prefixEdit->setText( prefix );
    mImap.progressDialogCheck->setChecked( ai.isProgressDialogEnabled() );
#if 0
    mImap.resourceCheck->setChecked( ai.resource() );
#endif
    mImap.hiddenFoldersCheck->setChecked( ai.hiddenFolders() );
    mImap.subscribedFoldersCheck->setChecked( ai.onlySubscribedFolders() );
    mImap.storePasswordCheck->setChecked( ai.storePasswd() );
    mImap.intervalCheck->setChecked( interval >= 1 );
    mImap.intervalSpin->setValue( QMAX(1, interval) );
    mImap.excludeCheck->setChecked( ai.checkExclude() );
    mImap.intervalCheck->setChecked( interval >= 1 );
    mImap.intervalSpin->setValue( QMAX(1, interval) );
    QString trashfolder = ai.trash();
    if (trashfolder.isEmpty())
      trashfolder = kmkernel->trashFolder()->idString();
    mImap.trashCombo->setFolder( trashfolder );
    slotEnableImapInterval( interval >= 1 );
    if (ai.useSSL())
      mImap.encryptionSSL->setChecked( TRUE );
    else if (ai.useTLS())
      mImap.encryptionTLS->setChecked( TRUE );
    else mImap.encryptionNone->setChecked( TRUE );
    if (ai.auth() == "CRAM-MD5")
      mImap.authCramMd5->setChecked( TRUE );
    else if (ai.auth() == "DIGEST-MD5")
      mImap.authDigestMd5->setChecked( TRUE );
    else if (ai.auth() == "ANONYMOUS")
      mImap.authAnonymous->setChecked( TRUE );
    else if (ai.auth() == "PLAIN")
      mImap.authPlain->setChecked( TRUE );
    else if (ai.auth() == "LOGIN")
      mImap.authLogin->setChecked( TRUE );
    else mImap.authUser->setChecked( TRUE );
    if ( mSieveConfigEditor )
      mSieveConfigEditor->setConfig( ai.sieveConfig() );
  }
  else if( accountType == "maildir" )
  {
    KMAcctMaildir *acctMaildir = dynamic_cast<KMAcctMaildir*>(mAccount);

    mMaildir.nameEdit->setText( mAccount->name() );
    mMaildir.nameEdit->setFocus();
    mMaildir.locationEdit->setEditText( acctMaildir->location() );

    mMaildir.intervalSpin->setValue( QMAX(1, interval) );
    mMaildir.intervalCheck->setChecked( interval >= 1 );
#if 0
    mMaildir.resourceCheck->setChecked( mAccount->resource() );
#endif
    mMaildir.excludeCheck->setChecked( mAccount->checkExclude() );
    mMaildir.precommand->setText( mAccount->precommand() );

    slotEnableMaildirInterval( interval >= 1 );
    folderCombo = mMaildir.folderCombo;
  }
  else // Unknown account type
    return;

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


void AccountDialog::slotPipeliningClicked()
{
  if (mPop.usePipeliningCheck->isChecked())
    KMessageBox::information(0,
      i18n("Please note that this feature can cause some POP3 servers "
      "that don't support pipelining to send corrupted mails.\n"
      "This is configurable, because some servers support pipelining "
      "but don't announce their capabilities. To check if your POP3 server "
      "announces pipelining support, use the \"Check What the Server "
      "Supports\" button at the bottom of the dialog.\n"
      "If your server doesn't announce it, but you want more speed then "
      "you should do some testing first by sending yourself a batch "
      "of mails and downloading them."), QString::null,
      "pipelining");
}


void AccountDialog::slotPopEncryptionChanged(int id)
{
  if (id == 1 || mPop.portEdit->text() == "995")
    mPop.portEdit->setText((id == 1) ? "995" : "110");
}


void AccountDialog::slotImapEncryptionChanged(int id)
{
  if (id == 1 || mImap.portEdit->text() == "993")
    mImap.portEdit->setText((id == 1) ? "993" : "143");
}


void AccountDialog::slotCheckPopCapabilities()
{
  delete mServerTest;
  mServerTest = new KMServerTest("pop3", mPop.hostEdit->text(),
    mPop.portEdit->text().toInt());
  connect(mServerTest, SIGNAL(capabilities(const QStringList &)),
    SLOT(slotPopCapabilities(const QStringList &)));
  mPop.checkCapabilities->setEnabled(FALSE);
}


void AccountDialog::slotCheckImapCapabilities()
{
  delete mServerTest;
  mServerTest = new KMServerTest("imap", mImap.hostEdit->text(),
    mImap.portEdit->text().toInt());
  connect(mServerTest, SIGNAL(capabilities(const QStringList &)),
    SLOT(slotImapCapabilities(const QStringList &)));
  mImap.checkCapabilities->setEnabled(FALSE);
}


void AccountDialog::slotPopCapabilities(const QStringList &list)
{
  mPop.checkCapabilities->setEnabled(TRUE);
  bool nc = list.findIndex("NORMAL-CONNECTION") != -1;
  mPop.usePipeliningCheck->setChecked(list.findIndex("PIPELINING") != -1);
  mPop.encryptionNone->setEnabled(nc);
  mPop.encryptionSSL->setEnabled(list.findIndex("SSL") != -1);
  mPop.encryptionTLS->setEnabled(list.findIndex("STLS") != -1 && nc);
  mPop.authPlain->setEnabled(list.findIndex("PLAIN") != -1);
  mPop.authLogin->setEnabled(list.findIndex("LOGIN") != -1);
  mPop.authCRAM_MD5->setEnabled(list.findIndex("CRAM-MD5") != -1);
  mPop.authDigestMd5->setEnabled(list.findIndex("DIGEST-MD5") != -1);
  mPop.authAPOP->setEnabled(list.findIndex("APOP") != -1);
  checkHighest(mPop.encryptionGroup);
  checkHighest(mPop.authGroup);
  delete mServerTest;
  mServerTest = 0;
}


void AccountDialog::slotImapCapabilities(const QStringList &list)
{
  mImap.checkCapabilities->setEnabled(TRUE);
  bool nc = list.findIndex("NORMAL-CONNECTION") != -1;
  mImap.encryptionNone->setEnabled(nc);
  mImap.encryptionSSL->setEnabled(list.findIndex("SSL") != -1);
  mImap.encryptionTLS->setEnabled(list.findIndex("STARTTLS") != -1 && nc);
  mImap.authPlain->setEnabled(list.findIndex("AUTH=PLAIN") != -1);
  mImap.authLogin->setEnabled(list.findIndex("AUTH=LOGIN") != -1);
  mImap.authCramMd5->setEnabled(list.findIndex("AUTH=CRAM-MD5") != -1);
  mImap.authDigestMd5->setEnabled(list.findIndex("AUTH=DIGEST-MD5") != -1);
  mImap.authAnonymous->setEnabled(list.findIndex("AUTH=ANONYMOUS") != -1);
  checkHighest(mImap.encryptionGroup);
  checkHighest(mImap.authGroup);
  delete mServerTest;
  mServerTest = 0;
}


void AccountDialog::checkHighest(QButtonGroup *btnGroup)
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
#if 0
    mAccount->setResource( mLocal.resourceCheck->isChecked() );
#endif
    mAccount->setCheckExclude( mLocal.excludeCheck->isChecked() );

    mAccount->setPrecommand( mLocal.precommand->text() );

    mAccount->setFolder( *mFolderList.at(mLocal.folderCombo->currentItem()) );

  }
  else if( accountType == "pop" )
  {
    mAccount->setName( mPop.nameEdit->text() );
    mAccount->setCheckInterval( mPop.intervalCheck->isChecked() ?
			     mPop.intervalSpin->value() : 0 );
#if 0
    mAccount->setResource( mPop.resourceCheck->isChecked() );
#endif
    mAccount->setCheckExclude( mPop.excludeCheck->isChecked() );

    mAccount->setFolder( *mFolderList.at(mPop.folderCombo->currentItem()) );

    KMAcctExpPop &epa = *(KMAcctExpPop*)mAccount;
    epa.setHost( mPop.hostEdit->text().stripWhiteSpace() );
    epa.setPort( mPop.portEdit->text().toInt() );
    epa.setLogin( mPop.loginEdit->text().stripWhiteSpace() );
    epa.setPasswd( mPop.passwordEdit->text(), true );
    epa.setUsePipelining( mPop.usePipeliningCheck->isChecked() );
    epa.setStorePasswd( mPop.storePasswordCheck->isChecked() );
    epa.setPasswd( mPop.passwordEdit->text(), epa.storePasswd() );
    epa.setLeaveOnServer( !mPop.deleteMailCheck->isChecked() );
    epa.setFilterOnServer( mPop.filterOnServerCheck->isChecked() );
    epa.setFilterOnServerCheckSize (mPop.filterOnServerSizeSpin->value() );
    epa.setPrecommand( mPop.precommand->text() );
    epa.setUseSSL( mPop.encryptionSSL->isChecked() );
    epa.setUseTLS( mPop.encryptionTLS->isChecked() );
    if (mPop.authUser->isChecked())
      epa.setAuth("USER");
    else if (mPop.authLogin->isChecked())
      epa.setAuth("LOGIN");
    else if (mPop.authPlain->isChecked())
      epa.setAuth("PLAIN");
    else if (mPop.authCRAM_MD5->isChecked())
      epa.setAuth("CRAM-MD5");
    else if (mPop.authDigestMd5->isChecked())
      epa.setAuth("DIGEST-MD5");
    else if (mPop.authAPOP->isChecked())
      epa.setAuth("APOP");
    else epa.setAuth("AUTO");
  }
  else if( accountType == "imap" )
  {
    mAccount->setName( mImap.nameEdit->text() );
    mAccount->setCheckInterval( mImap.intervalCheck->isChecked() ?
                                mImap.intervalSpin->value() : 0 );
#if 0
    mAccount->setResource( mImap.resourceCheck->isChecked() );
#endif
    mAccount->setCheckExclude( mImap.excludeCheck->isChecked() );
    mAccount->setFolder( 0 );

    KMAcctImap &epa = *(KMAcctImap*)mAccount;
    epa.setHost( mImap.hostEdit->text().stripWhiteSpace() );
    epa.setPort( mImap.portEdit->text().toInt() );
    QString prefix = "/" + mImap.prefixEdit->text();
    if (prefix[prefix.length() - 1] != '/') prefix += "/";
    epa.setPrefix( prefix );
    epa.setLogin( mImap.loginEdit->text().stripWhiteSpace() );
    epa.setAutoExpunge( mImap.autoExpungeCheck->isChecked() );
    epa.setHiddenFolders( mImap.hiddenFoldersCheck->isChecked() );
    epa.setOnlySubscribedFolders( mImap.subscribedFoldersCheck->isChecked() );
    epa.setLoadOnDemand( mImap.loadOnDemandCheck->isChecked() );
    epa.setStorePasswd( mImap.storePasswordCheck->isChecked() );
    epa.setPasswd( mImap.passwordEdit->text(), epa.storePasswd() );
    KMFolder *t = mImap.trashCombo->getFolder();
    if ( t )
      epa.setTrash( mImap.trashCombo->getFolder()->idString() );
    else
      epa.setTrash( kmkernel->trashFolder()->idString() );
#if 0
    epa.setResource( mImap.resourceCheck->isChecked() );
#endif
    epa.setCheckExclude( mImap.excludeCheck->isChecked() );
    epa.setUseSSL( mImap.encryptionSSL->isChecked() );
    epa.setUseTLS( mImap.encryptionTLS->isChecked() );
    if (mImap.authCramMd5->isChecked())
      epa.setAuth("CRAM-MD5");
    else if (mImap.authDigestMd5->isChecked())
      epa.setAuth("DIGEST-MD5");
    else if (mImap.authAnonymous->isChecked())
      epa.setAuth("ANONYMOUS");
    else if (mImap.authLogin->isChecked())
      epa.setAuth("LOGIN");
    else if (mImap.authPlain->isChecked())
      epa.setAuth("PLAIN");
    else epa.setAuth("*");
    if ( mSieveConfigEditor )
      epa.setSieveConfig( mSieveConfigEditor->config() );
  }
  else if( accountType == "cachedimap" )
  {
    mAccount->setName( mImap.nameEdit->text() );
    mAccount->setCheckInterval( mImap.intervalCheck->isChecked() ?
                                mImap.intervalSpin->value() : 0 );
#if 0
    mAccount->setResource( mImap.resourceCheck->isChecked() );
#endif
    mAccount->setCheckExclude( mImap.excludeCheck->isChecked() );
    //mAccount->setFolder( NULL );
    mAccount->setFolder( kmkernel->dimapFolderMgr()->find(mAccount->name()) );
    kdDebug(5006) << mAccount->name() << endl;
    //kdDebug(5006) << "account for folder " << mAccount->folder()->name() << endl;

    KMAcctCachedImap &epa = *(KMAcctCachedImap*)mAccount;
    epa.setHost( mImap.hostEdit->text().stripWhiteSpace() );
    epa.setPort( mImap.portEdit->text().toInt() );
    QString prefix = "/" + mImap.prefixEdit->text();
    if (prefix[prefix.length() - 1] != '/') prefix += "/";
    epa.setPrefix( prefix );
    epa.setLogin( mImap.loginEdit->text().stripWhiteSpace() );
    epa.setProgressDialogEnabled( mImap.progressDialogCheck->isChecked() );
    epa.setHiddenFolders( mImap.hiddenFoldersCheck->isChecked() );
    epa.setOnlySubscribedFolders( mImap.subscribedFoldersCheck->isChecked() );
    epa.setStorePasswd( mImap.storePasswordCheck->isChecked() );
    epa.setPasswd( mImap.passwordEdit->text(), epa.storePasswd() );
    KMFolder *t = mImap.trashCombo->getFolder();
    if ( t )
      epa.setTrash( mImap.trashCombo->getFolder()->idString() );
    else
      epa.setTrash( kmkernel->trashFolder()->idString() );
#if 0
    epa.setResource( mImap.resourceCheck->isChecked() );
#endif
    epa.setCheckExclude( mImap.excludeCheck->isChecked() );
    epa.setUseSSL( mImap.encryptionSSL->isChecked() );
    epa.setUseTLS( mImap.encryptionTLS->isChecked() );
    if (mImap.authCramMd5->isChecked())
      epa.setAuth("CRAM-MD5");
    else if (mImap.authDigestMd5->isChecked())
      epa.setAuth("DIGEST-MD5");
    else if (mImap.authAnonymous->isChecked())
      epa.setAuth("ANONYMOUS");
    else if (mImap.authLogin->isChecked())
      epa.setAuth("LOGIN");
    else if (mImap.authPlain->isChecked())
      epa.setAuth("PLAIN");
    else epa.setAuth("*");
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
    mAccount->setCheckExclude( mMaildir.excludeCheck->isChecked() );

    mAccount->setPrecommand( mMaildir.precommand->text() );
  }

  kmkernel->acctMgr()->writeConfig(TRUE);

  // get the new account and register the new destination folder
  KMAccount* newAcct = kmkernel->acctMgr()->find(mAccount->name());
  if (newAcct)
  {
    if( accountType == "local" ) {
      newAcct->setFolder( *mFolderList.at(mLocal.folderCombo->currentItem()), true );
    } else if ( accountType == "pop" ) {
      newAcct->setFolder( *mFolderList.at(mPop.folderCombo->currentItem()), true );
    } else if ( accountType == "maildir" ) {
      newAcct->setFolder( *mFolderList.at(mMaildir.folderCombo->currentItem()), true );
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

#include "accountdialog.moc"
