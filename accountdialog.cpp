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
  ProcmailRCParser procmailrcParser;
  QWidget *page = new QWidget( this );
  setMainWidget( page );
  QGridLayout *topLayout = new QGridLayout( page );
  topLayout->setSpacing( spacingHint() );
  topLayout->setMargin( 0 );
  topLayout->addItem( new QSpacerItem( fontMetrics().maxWidth()*15, 0 ), 0, 1 );
  topLayout->setRowStretch( 11, 10 );
  topLayout->setColumnStretch( 1, 10 );

  mLocal.titleLabel = new QLabel( i18n("Account Type: Local Account"), page );
  topLayout->addWidget( mLocal.titleLabel, 0, 0, 1, 3 );
  QFont titleFont( mLocal.titleLabel->font() );
  titleFont.setBold( true );
  mLocal.titleLabel->setFont( titleFont );
  KSeparator *hline = new KSeparator( Qt::Horizontal, page);
  topLayout->addWidget( hline, 1, 0, 1, 3 );

  QLabel *label = new QLabel( i18n("Account &name:"), page );
  topLayout->addWidget( label, 2, 0 );
  mLocal.nameEdit = new KLineEdit( page );
  label->setBuddy( mLocal.nameEdit );
  topLayout->addWidget( mLocal.nameEdit, 2, 1 );

  label = new QLabel( i18n("File &location:"), page );
  topLayout->addWidget( label, 3, 0 );
  mLocal.locationEdit = new QComboBox( page );
  mLocal.locationEdit->setEditable( true );
  label->setBuddy( mLocal.locationEdit );
  topLayout->addWidget( mLocal.locationEdit, 3, 1 );
  mLocal.locationEdit->addItems(procmailrcParser.getSpoolFilesList());

  QPushButton *choose = new QPushButton( i18n("Choo&se..."), page );
  choose->setAutoDefault( false );
  connect( choose, SIGNAL(clicked()), this, SLOT(slotLocationChooser()) );
  topLayout->addWidget( choose, 3, 2 );

  QGroupBox *group = new QGroupBox( i18n("Locking Method"), page );
  QGridLayout *groupLayout = new QGridLayout();
  group->setLayout( groupLayout );
  groupLayout->setAlignment( Qt::AlignTop );
  groupLayout->setSpacing( 6 );
  groupLayout->setMargin( 11 );

  mLocal.lockProcmail = new QRadioButton( i18n("Procmail loc&kfile:"), group);
  groupLayout->addWidget(mLocal.lockProcmail, 0, 0);

  mLocal.procmailLockFileName = new QComboBox( group );
  mLocal.procmailLockFileName->setEditable( true );
  groupLayout->addWidget(mLocal.procmailLockFileName, 0, 1);
  mLocal.procmailLockFileName->addItems(procmailrcParser.getLockFilesList());
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

  topLayout->addWidget( group, 4, 0, 1, 3 );

  mLocal.includeInCheck =
    new QCheckBox( i18n("Include in m&anual mail check"),
                   page );
  topLayout->addWidget( mLocal.includeInCheck, 5, 0, 1, 3 );

  mLocal.intervalCheck =
    new QCheckBox( i18n("Enable &interval mail checking"), page );
  topLayout->addWidget( mLocal.intervalCheck, 6, 0, 1, 3 );
  connect( mLocal.intervalCheck, SIGNAL(toggled(bool)),
           this, SLOT(slotEnableLocalInterval(bool)) );
  mLocal.intervalLabel = new QLabel( i18n("Check inter&val:"), page );
  topLayout->addWidget( mLocal.intervalLabel, 7, 0 );
  mLocal.intervalSpin = new KIntNumInput( page );
  mLocal.intervalLabel->setBuddy( mLocal.intervalSpin );
  mLocal.intervalSpin->setRange( GlobalSettings::self()->minimumCheckInterval(),
                                 10000, 1 );
  mLocal.intervalSpin->setSliderEnabled( false );
  mLocal.intervalSpin->setSuffix( i18n(" min") );
  mLocal.intervalSpin->setValue( 1 );
  topLayout->addWidget( mLocal.intervalSpin, 7, 1 );

  label = new QLabel( i18n("&Destination folder:"), page );
  topLayout->addWidget( label, 8, 0 );
  mLocal.folderCombo = new QComboBox( page );
  mLocal.folderCombo->setEditable( false );
  label->setBuddy( mLocal.folderCombo );
  topLayout->addWidget( mLocal.folderCombo, 8, 1 );

  label = new QLabel( i18n("&Pre-command:"), page );
  topLayout->addWidget( label, 9, 0 );
  mLocal.precommand = new KLineEdit( page );
  label->setBuddy( mLocal.precommand );
  topLayout->addWidget( mLocal.precommand, 9, 1 );

  connect(KGlobalSettings::self(),SIGNAL(kdisplayFontChanged()),SLOT(slotFontChanged()));
}

void AccountDialog::makeMaildirAccountPage()
{
  ProcmailRCParser procmailrcParser;

  QWidget *page = new QWidget( this );
  setMainWidget( page );
  QGridLayout *topLayout = new QGridLayout( page );
  topLayout->setSpacing( spacingHint() );
  topLayout->setMargin( 0 );
  topLayout->addItem( new QSpacerItem( fontMetrics().maxWidth()*15, 0 ), 0, 1 );
  topLayout->setRowStretch( 11, 10 );
  topLayout->setColumnStretch( 1, 10 );

  mMaildir.titleLabel = new QLabel( i18n("Account Type: Maildir Account"), page );
  topLayout->addWidget( mMaildir.titleLabel, 0, 0, 1, 3 );
  QFont titleFont( mMaildir.titleLabel->font() );
  titleFont.setBold( true );
  mMaildir.titleLabel->setFont( titleFont );
  QFrame *hline = new QFrame( page );
  hline->setFrameShape ( QFrame::HLine );
  hline->setFrameShadow( QFrame::Sunken);
  topLayout->addWidget( hline, 1, 0, 1, 3 );

  mMaildir.nameEdit = new KLineEdit( page );
  topLayout->addWidget( mMaildir.nameEdit, 2, 1 );
  QLabel *label = new QLabel( i18n("Account &name:"), page );
  label->setBuddy( mMaildir.nameEdit );
  topLayout->addWidget( label, 2, 0 );

  mMaildir.locationEdit = new QComboBox( page );
  mMaildir.locationEdit->setEditable( true );
  topLayout->addWidget( mMaildir.locationEdit, 3, 1 );
  mMaildir.locationEdit->addItems(procmailrcParser.getSpoolFilesList());
  label = new QLabel( i18n("Folder &location:"), page );
  label->setBuddy( mMaildir.locationEdit );
  topLayout->addWidget( label, 3, 0 );

  QPushButton *choose = new QPushButton( i18n("Choo&se..."), page );
  choose->setAutoDefault( false );
  connect( choose, SIGNAL(clicked()), this, SLOT(slotMaildirChooser()) );
  topLayout->addWidget( choose, 3, 2 );

  mMaildir.includeInCheck =
    new QCheckBox( i18n("Include in &manual mail check"), page );
  topLayout->addWidget( mMaildir.includeInCheck, 4, 0, 1, 3 );

  mMaildir.intervalCheck =
    new QCheckBox( i18n("Enable &interval mail checking"), page );
  topLayout->addWidget( mMaildir.intervalCheck, 5, 0, 1, 3 );
  connect( mMaildir.intervalCheck, SIGNAL(toggled(bool)),
           this, SLOT(slotEnableMaildirInterval(bool)) );
  mMaildir.intervalLabel = new QLabel( i18n("Check inter&val:"), page );
  topLayout->addWidget( mMaildir.intervalLabel, 6, 0 );
  mMaildir.intervalSpin = new KIntNumInput( page );
  mMaildir.intervalSpin->setRange( GlobalSettings::self()->minimumCheckInterval(),
                                   10000, 1 );
  mMaildir.intervalSpin->setSliderEnabled( false );
  mMaildir.intervalSpin->setSuffix( i18n(" min") );
  mMaildir.intervalSpin->setValue( 1 );
  mMaildir.intervalLabel->setBuddy( mMaildir.intervalSpin );
  topLayout->addWidget( mMaildir.intervalSpin, 6, 1 );

  mMaildir.folderCombo = new QComboBox( page );
  mMaildir.folderCombo->setEditable( false );
  topLayout->addWidget( mMaildir.folderCombo, 7, 1 );
  label = new QLabel(i18n("&Destination folder:"), page );
  label->setBuddy( mMaildir.folderCombo );
  topLayout->addWidget( label, 7, 0 );

  mMaildir.precommand = new KLineEdit( page );
  topLayout->addWidget( mMaildir.precommand, 8, 1 );
  label = new QLabel( i18n("&Pre-command:"), page );
  label->setBuddy( mMaildir.precommand );
  topLayout->addWidget( label, 8, 0 );

  connect(KGlobalSettings::self(),SIGNAL(kdisplayFontChanged()),SLOT(slotFontChanged()));
}


void AccountDialog::makePopAccountPage()
{
  QWidget *page = new QWidget( this );
  setMainWidget( page );
  QVBoxLayout *topLayout = new QVBoxLayout( page );
  topLayout->setSpacing( spacingHint() );
  topLayout->setMargin( 0 );

  mPop.titleLabel = new QLabel( page );
  mPop.titleLabel->setText( i18n("Account Type: POP Account") );
  QFont titleFont( mPop.titleLabel->font() );
  titleFont.setBold( true );
  mPop.titleLabel->setFont( titleFont );
  topLayout->addWidget( mPop.titleLabel );
  KSeparator *hline = new KSeparator( Qt::Horizontal, page);
  topLayout->addWidget( hline );

  QTabWidget *tabWidget = new QTabWidget(page);
  topLayout->addWidget( tabWidget );

  QWidget *page1 = new QWidget( tabWidget );
  tabWidget->addTab( page1, i18n("&General") );

  QGridLayout *grid = new QGridLayout( page1 );
  grid->setSpacing( spacingHint() );
  grid->setMargin( marginHint() );
  grid->addItem( new QSpacerItem( fontMetrics().maxWidth()*15, 0 ), 0, 1 );
  grid->setRowStretch( 15, 10 );
  grid->setColumnStretch( 1, 10 );

  QLabel *label = new QLabel( i18n("Account &name:"), page1 );
  grid->addWidget( label, 0, 0 );
  mPop.nameEdit = new KLineEdit( page1 );
  label->setBuddy( mPop.nameEdit );
  grid->addWidget( mPop.nameEdit, 0, 1 );

  label = new QLabel( i18n("&Login:"), page1 );
  label->setWhatsThis( i18n("Your Internet Service Provider gave you a <em>user name</em> which is used to authenticate you with their servers. It usually is the first part of your email address (the part before <em>@</em>).") );
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
  connect( mPop.passwordEdit, SIGNAL( textEdited( const QString& ) ),
           this, SLOT( slotPopPasswordChanged( const QString& ) ) );

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
  mPop.storePasswordCheck->setWhatsThis(
                   i18n("Check this option to have KMail store "
                   "the password.\nIf KWallet is available "
                   "the password will be stored there which is considered "
                   "safe.\nHowever, if KWallet is not available, "
                   "the password will be stored in KMail's configuration "
                   "file. The password is stored in an "
                   "obfuscated format, but should not be "
                   "considered secure from decryption efforts "
                   "if access to the configuration file is obtained.") );
  grid->addWidget( mPop.storePasswordCheck, 5, 0, 1, 2 );

  mPop.leaveOnServerCheck =
    new QCheckBox( i18n("Lea&ve fetched messages on the server"), page1 );
  connect( mPop.leaveOnServerCheck, SIGNAL( clicked() ),
           this, SLOT( slotLeaveOnServerClicked() ) );
  grid->addWidget( mPop.leaveOnServerCheck, 6, 0, 1, 2 );
  KHBox *afterDaysBox = new KHBox( page1 );
  afterDaysBox->setSpacing( KDialog::spacingHint() );
  mPop.leaveOnServerDaysCheck =
    new QCheckBox( i18n("Leave messages on the server for"), afterDaysBox );
  connect( mPop.leaveOnServerDaysCheck, SIGNAL( toggled(bool) ),
           this, SLOT( slotEnableLeaveOnServerDays(bool)) );
  mPop.leaveOnServerDaysSpin = new KIntNumInput( afterDaysBox );
  mPop.leaveOnServerDaysSpin->setRange( 1, 365, 1 );
  mPop.leaveOnServerDaysSpin->setSliderEnabled( false );
  connect( mPop.leaveOnServerDaysSpin, SIGNAL(valueChanged(int)),
           SLOT(slotLeaveOnServerDaysChanged(int)));
  mPop.leaveOnServerDaysSpin->setValue( 1 );
  afterDaysBox->setStretchFactor( mPop.leaveOnServerDaysSpin, 1 );
  grid->addWidget( afterDaysBox, 7, 0, 1, 2 );
  KHBox *leaveOnServerCountBox = new KHBox( page1 );
  leaveOnServerCountBox->setSpacing( KDialog::spacingHint() );
  mPop.leaveOnServerCountCheck =
    new QCheckBox( i18n("Keep only the last"), leaveOnServerCountBox );
  connect( mPop.leaveOnServerCountCheck, SIGNAL( toggled(bool) ),
           this, SLOT( slotEnableLeaveOnServerCount(bool)) );
  mPop.leaveOnServerCountSpin = new KIntNumInput( leaveOnServerCountBox );
  mPop.leaveOnServerCountSpin->setRange( 1, 999999, 1 );
  mPop.leaveOnServerCountSpin->setSliderEnabled( false );
  connect( mPop.leaveOnServerCountSpin, SIGNAL(valueChanged(int)),
           SLOT(slotLeaveOnServerCountChanged(int)));
  mPop.leaveOnServerCountSpin->setValue( 100 );
  grid->addWidget( leaveOnServerCountBox, 8, 0, 1, 2 );
  KHBox *leaveOnServerSizeBox = new KHBox( page1 );
  leaveOnServerSizeBox->setSpacing( KDialog::spacingHint() );
  mPop.leaveOnServerSizeCheck =
    new QCheckBox( i18n("Keep only the last"), leaveOnServerSizeBox );
  connect( mPop.leaveOnServerSizeCheck, SIGNAL( toggled(bool) ),
           this, SLOT( slotEnableLeaveOnServerSize(bool)) );
  mPop.leaveOnServerSizeSpin = new KIntNumInput( leaveOnServerSizeBox );
  mPop.leaveOnServerSizeSpin->setRange( 1, 999999, 1 );
  mPop.leaveOnServerSizeSpin->setSliderEnabled( false );
  mPop.leaveOnServerSizeSpin->setSuffix( i18n(" MB") );
  mPop.leaveOnServerSizeSpin->setValue( 10 );
  grid->addWidget( leaveOnServerSizeBox, 9, 0, 1, 2 );

  mPop.includeInCheck =
    new QCheckBox( i18n("Include in man&ual mail check"), page1 );
  grid->addWidget( mPop.includeInCheck, 10, 0, 1, 2 );

  KHBox * hbox = new KHBox( page1 );
  hbox->setSpacing( KDialog::spacingHint() );
  mPop.filterOnServerCheck =
    new QCheckBox( i18n("&Filter messages if they are greater than"), hbox );
  mPop.filterOnServerSizeSpin = new KIntNumInput ( hbox );
  mPop.filterOnServerSizeSpin->setEnabled( false );
  hbox->setStretchFactor( mPop.filterOnServerSizeSpin, 1 );
  mPop.filterOnServerSizeSpin->setRange( 1, 10000000, 100 );
  mPop.filterOnServerSizeSpin->setSliderEnabled( false );
  connect(mPop.filterOnServerSizeSpin, SIGNAL(valueChanged(int)),
          SLOT(slotFilterOnServerSizeChanged(int)));
  mPop.filterOnServerSizeSpin->setValue( 50000 );
  grid->addWidget( hbox, 11, 0, 1, 2 );
  connect( mPop.filterOnServerCheck, SIGNAL(toggled(bool)),
           mPop.filterOnServerSizeSpin, SLOT(setEnabled(bool)) );
  connect( mPop.filterOnServerCheck, SIGNAL( clicked() ),
           this, SLOT( slotFilterOnServerClicked() ) );
  QString msg = i18n("If you select this option, POP Filters will be used to "
                     "decide what to do with messages. You can then select "
                     "to download, delete or keep them on the server." );
  mPop.filterOnServerCheck->setWhatsThis( msg );
  mPop.filterOnServerSizeSpin->setWhatsThis( msg );

  mPop.intervalCheck =
    new QCheckBox( i18n("Enable &interval mail checking"), page1 );
  grid->addWidget( mPop.intervalCheck, 12, 0, 1, 2 );
  connect( mPop.intervalCheck, SIGNAL(toggled(bool)),
           this, SLOT(slotEnablePopInterval(bool)) );
  mPop.intervalLabel = new QLabel( i18n("Chec&k interval:"), page1 );
  grid->addWidget( mPop.intervalLabel, 13, 0 );
  mPop.intervalSpin = new KIntNumInput( page1 );
  mPop.intervalSpin->setRange( GlobalSettings::self()->minimumCheckInterval(),
                               10000, 1 );
  mPop.intervalSpin->setSliderEnabled( false );
  mPop.intervalSpin->setSuffix( i18n(" min") );
  mPop.intervalSpin->setValue( 1 );
  mPop.intervalLabel->setBuddy( mPop.intervalSpin );
  grid->addWidget( mPop.intervalSpin, 13, 1 );

  label = new QLabel( i18n("Des&tination folder:"), page1 );
  grid->addWidget( label, 14, 0 );
  mPop.folderCombo = new QComboBox( page1 );
  label->setBuddy( mPop.folderCombo );
  grid->addWidget( mPop.folderCombo, 14, 1 );

  label = new QLabel( i18n("Pre-com&mand:"), page1 );
  grid->addWidget( label, 15, 0 );
  mPop.precommand = new KLineEdit( page1 );
  label->setBuddy(mPop.precommand);
  grid->addWidget( mPop.precommand, 15, 1 );

  QWidget *page2 = new QWidget( tabWidget );
  tabWidget->addTab( page2, i18n("&Extras") );
  QVBoxLayout *vlay = new QVBoxLayout( page2 );
  vlay->setSpacing( spacingHint() );
  vlay->setMargin( marginHint() );

  vlay->addSpacing( KDialog::spacingHint() );

  QHBoxLayout *buttonLay = new QHBoxLayout();
  vlay->addLayout( buttonLay );
  mPop.checkCapabilities =
    new QPushButton( i18n("Check &What the Server Supports"), page2 );
  connect( mPop.checkCapabilities, SIGNAL(clicked()),
           SLOT(slotCheckPopCapabilities()) );
  buttonLay->addStretch();
  buttonLay->addWidget( mPop.checkCapabilities );
  buttonLay->addStretch();

  vlay->addSpacing( KDialog::spacingHint() );

  mPop.encryptionGroup = new QGroupBox( i18n("Encryption"), page2 );
  mPop.encryptionGroup->setLayout( new QVBoxLayout() );
  mPop.encryptionNone =
    new QRadioButton( i18n("&None"), mPop.encryptionGroup );
  mPop.encryptionSSL =
    new QRadioButton( i18n("Use &SSL for secure mail download"),
    mPop.encryptionGroup );
  mPop.encryptionTLS =
    new QRadioButton( i18n("Use &TLS for secure mail download"),
    mPop.encryptionGroup );
  mPop.encryptionGroup->layout()->addWidget( mPop.encryptionNone );
  mPop.encryptionGroup->layout()->addWidget( mPop.encryptionSSL );
  mPop.encryptionGroup->layout()->addWidget( mPop.encryptionTLS );

  mPop.encryptionButtonGroup = new QButtonGroup();
  mPop.encryptionButtonGroup->addButton( mPop.encryptionNone,
                                         Transport::EnumEncryption::None );
  mPop.encryptionButtonGroup->addButton( mPop.encryptionSSL,
                                         Transport::EnumEncryption::SSL );
  mPop.encryptionButtonGroup->addButton( mPop.encryptionTLS,
                                         Transport::EnumEncryption::TLS );

  connect( mPop.encryptionButtonGroup, SIGNAL(buttonClicked(int)),
           SLOT(slotPopEncryptionChanged(int)) );
  vlay->addWidget( mPop.encryptionGroup );

  mPop.authGroup = new QGroupBox( i18n("Authentication Method"), page2 );
  mPop.authGroup->setLayout( new QVBoxLayout() );
  mPop.authUser = new QRadioButton( i18n("Clear te&xt") , mPop.authGroup );
  mPop.authUser->setObjectName( "auth clear text" );
  mPop.authLogin = new QRadioButton( i18nc("Please translate this "
    "authentication method only if you have a good reason", "&LOGIN"),
    mPop.authGroup );
  mPop.authLogin->setObjectName( "auth login" );
  mPop.authPlain = new QRadioButton( i18nc("Please translate this "
    "authentication method only if you have a good reason", "&PLAIN"),
    mPop.authGroup  );
  mPop.authPlain->setObjectName( "auth plain" );
  mPop.authCRAM_MD5 = new QRadioButton( i18n("CRAM-MD&5"), mPop.authGroup );
  mPop.authCRAM_MD5->setObjectName( "auth cram-md5" );
  mPop.authDigestMd5 = new QRadioButton( i18n("&DIGEST-MD5"), mPop.authGroup );
  mPop.authDigestMd5->setObjectName( "auth digest-md5" );
  mPop.authNTLM = new QRadioButton( i18n("&NTLM"), mPop.authGroup );
  mPop.authNTLM->setObjectName( "auth ntlm" );
  mPop.authGSSAPI = new QRadioButton( i18n("&GSSAPI"), mPop.authGroup );
  mPop.authGSSAPI->setObjectName( "auth gssapi" );
  if ( KProtocolInfo::capabilities("pop3").contains("SASL") == 0 )
  {
    mPop.authNTLM->hide();
    mPop.authGSSAPI->hide();
  }
  mPop.authAPOP = new QRadioButton( i18n("&APOP"), mPop.authGroup );
  mPop.authAPOP->setObjectName( "auth apop" );

  mPop.authGroup->layout()->addWidget( mPop.authUser );
  mPop.authGroup->layout()->addWidget( mPop.authLogin );
  mPop.authGroup->layout()->addWidget( mPop.authPlain );
  mPop.authGroup->layout()->addWidget( mPop.authCRAM_MD5 );
  mPop.authGroup->layout()->addWidget( mPop.authDigestMd5 );
  mPop.authGroup->layout()->addWidget( mPop.authNTLM );
  mPop.authGroup->layout()->addWidget( mPop.authGSSAPI );
  mPop.authGroup->layout()->addWidget( mPop.authAPOP );

  mPop.authButtonGroup = new QButtonGroup();
  mPop.authButtonGroup->addButton( mPop.authUser );
  mPop.authButtonGroup->addButton( mPop.authLogin );
  mPop.authButtonGroup->addButton( mPop.authPlain );
  mPop.authButtonGroup->addButton( mPop.authCRAM_MD5 );
  mPop.authButtonGroup->addButton( mPop.authDigestMd5 );
  mPop.authButtonGroup->addButton( mPop.authNTLM );
  mPop.authButtonGroup->addButton( mPop.authGSSAPI );
  mPop.authButtonGroup->addButton( mPop.authAPOP );

  vlay->addWidget( mPop.authGroup );

  mPop.usePipeliningCheck =
    new QCheckBox( i18n("&Use pipelining for faster mail download"), page2 );
  connect( mPop.usePipeliningCheck, SIGNAL(clicked()),
           SLOT(slotPipeliningClicked()) );
  vlay->addWidget( mPop.usePipeliningCheck );

  vlay->addStretch();

  connect(KGlobalSettings::self(),SIGNAL(kdisplayFontChanged()),SLOT(slotFontChanged()));
}


void AccountDialog::makeImapAccountPage( bool connected )
{
  QWidget *page = new QWidget( this );
  setMainWidget( page );
  QVBoxLayout *topLayout = new QVBoxLayout( page );
  topLayout->setSpacing( spacingHint() );
  topLayout->setMargin( 0 );

  mImap.titleLabel = new QLabel( page );
  if( connected )
    mImap.titleLabel->setText( i18n("Account Type: Disconnected IMAP Account") );
  else
    mImap.titleLabel->setText( i18n("Account Type: IMAP Account") );
  QFont titleFont( mImap.titleLabel->font() );
  titleFont.setBold( true );
  mImap.titleLabel->setFont( titleFont );
  topLayout->addWidget( mImap.titleLabel );
  KSeparator *hline = new KSeparator( Qt::Horizontal, page);
  topLayout->addWidget( hline );

  QTabWidget *tabWidget = new QTabWidget(page);
  topLayout->addWidget( tabWidget );

  QWidget *page1 = new QWidget( tabWidget );
  tabWidget->addTab( page1, i18n("&General") );

  int row = -1;
  QGridLayout *grid = new QGridLayout( page1 );
  grid->setSpacing( spacingHint() );
  grid->setMargin( marginHint() );
  grid->addItem( new QSpacerItem( fontMetrics().maxWidth()*16, 0 ), 0, 1 );

  ++row;
  QLabel *label = new QLabel( i18n("Account &name:"), page1 );
  grid->addWidget( label, row, 0 );
  mImap.nameEdit = new KLineEdit( page1 );
  label->setBuddy( mImap.nameEdit );
  grid->addWidget( mImap.nameEdit, row, 1 );

  ++row;
  label = new QLabel( i18n("&Login:"), page1 );
  label->setWhatsThis( i18n("Your Internet Service Provider gave you a <em>user name</em> which is used to authenticate you with their servers. It usually is the first part of your email address (the part before <em>@</em>).") );
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
  KHBox* box = new KHBox( page1 );
  QLabel* namespaceLabel = new QLabel( i18n("Namespaces:"), box );
  namespaceLabel->setWhatsThis( i18n( "Here you see the different namespaces that your IMAP server supports."
        "Each namespace represents a prefix that separates groups of folders."
        "Namespaces allow KMail for example to display your personal folders and shared folders in one account." ) );
  // button to reload
  QToolButton* button = new QToolButton( box );
  button->setAutoRaise(true);
  button->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );
  button->setFixedSize( 22, 22 );
  button->setIcon( KIcon("view-refresh") );
  connect( button, SIGNAL(clicked()), this, SLOT(slotReloadNamespaces()) );
  button->setWhatsThis(
      i18n("Reload the namespaces from the server. This overwrites any changes.") );
  grid->addWidget( box, row, 0 );

  // grid with label, namespace list and edit button
  QWidget* listbox = new QWidget( page1 );
  label = new QLabel( i18n("Personal"), listbox );
  label->setWhatsThis( i18n( "Personal namespaces include your personal folders." ) );
  mImap.personalNS = new KLineEdit( listbox );
  mImap.personalNS->setReadOnly( true );
  mImap.editPNS = new QToolButton( listbox );
  mImap.editPNS->setIcon( KIcon("document-properties") );
  mImap.editPNS->setAutoRaise( true );
  mImap.editPNS->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );
  mImap.editPNS->setFixedSize( 22, 22 );
  connect( mImap.editPNS, SIGNAL(clicked()), this, SLOT(slotEditPersonalNamespace()) );

  QGridLayout* listboxLayout = new QGridLayout;
  listboxLayout->setMargin( 0 );
  listboxLayout->setSpacing( KDialog::spacingHint() );
  listboxLayout->addWidget( label, 0, 0 );
  listboxLayout->addWidget( mImap.personalNS, 0, 1 );
  listboxLayout->addWidget( mImap.editPNS, 0, 2 );

  label = new QLabel( i18n("Other Users"), listbox );
  label->setWhatsThis( i18n( "These namespaces include the folders of other users." ) );
  mImap.otherUsersNS = new KLineEdit( listbox );
  mImap.otherUsersNS->setReadOnly( true );
  mImap.editONS = new QToolButton( listbox );
  mImap.editONS->setIcon( KIcon("document-properties") );
  mImap.editONS->setAutoRaise( true );
  mImap.editONS->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );
  mImap.editONS->setFixedSize( 22, 22 );
  connect( mImap.editONS, SIGNAL(clicked()), this, SLOT(slotEditOtherUsersNamespace()) );

  listboxLayout->addWidget( label, 1, 0 );
  listboxLayout->addWidget( mImap.otherUsersNS, 1, 1 );
  listboxLayout->addWidget( mImap.editONS, 1, 2 );

  label = new QLabel( i18n("Shared"), listbox );
  label->setWhatsThis( i18n( "These namespaces include the shared folders." ) );
  mImap.sharedNS = new KLineEdit( listbox );
  mImap.sharedNS->setReadOnly( true );
  mImap.editSNS = new QToolButton( listbox );
  mImap.editSNS->setIcon( KIcon("document-properties") );
  mImap.editSNS->setAutoRaise( true );
  mImap.editSNS->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );
  mImap.editSNS->setFixedSize( 22, 22 );
  connect( mImap.editSNS, SIGNAL(clicked()), this, SLOT(slotEditSharedNamespace()) );

  listboxLayout->addWidget( label, 2, 0 );
  listboxLayout->addWidget( mImap.sharedNS, 2, 1 );
  listboxLayout->addWidget( mImap.editSNS, 2, 2 );
  listbox->setLayout( listboxLayout );

  namespaceLabel->setBuddy( listbox );
  grid->addWidget( listbox, row, 1 );

  ++row;
  mImap.storePasswordCheck =
    new QCheckBox( i18n("Sto&re IMAP password"), page1 );
  mImap.storePasswordCheck->setWhatsThis(
                   i18n("Check this option to have KMail store "
                   "the password.\nIf KWallet is available "
                   "the password will be stored there which is considered "
                   "safe.\nHowever, if KWallet is not available, "
                   "the password will be stored in KMail's configuration "
                   "file. The password is stored in an "
                   "obfuscated format, but should not be "
                   "considered secure from decryption efforts "
                   "if access to the configuration file is obtained.") );
  grid->addWidget( mImap.storePasswordCheck, row, 0, 1, 2 );

  if( !connected ) {
    ++row;
    mImap.autoExpungeCheck =
      new QCheckBox( i18n("Automaticall&y compact folders (expunges deleted messages)"), page1);
    grid->addWidget( mImap.autoExpungeCheck, row, 0, 1, 2 );
  }

  ++row;
  mImap.hiddenFoldersCheck = new QCheckBox( i18n("Sho&w hidden folders"), page1);
  grid->addWidget( mImap.hiddenFoldersCheck, row, 0, 1, 2 );


  ++row;
  mImap.subscribedFoldersCheck = new QCheckBox(
    i18n("Show only s&ubscribed folders"), page1);
  grid->addWidget( mImap.subscribedFoldersCheck, row, 0, 1, 2 );

  ++row;
  mImap.locallySubscribedFoldersCheck = new QCheckBox(
    i18n("Show only &locally subscribed folders"), page1);
  grid->addWidget( mImap.locallySubscribedFoldersCheck, row, 0, 1, 2 );

  if ( !connected ) {
    // not implemented for disconnected yet
    ++row;
    mImap.loadOnDemandCheck = new QCheckBox(
        i18n("Load attach&ments on demand"), page1);
    mImap.loadOnDemandCheck->setWhatsThis(
        i18n("Activate this to load attachments not automatically when you select the email but only when you click on the attachment. This way also big emails are shown instantly.") );
    grid->addWidget( mImap.loadOnDemandCheck, row, 0, 1, 2 );
  }

  if ( !connected ) {
    // not implemented for disconnected yet
    ++row;
    mImap.listOnlyOpenCheck = new QCheckBox(
        i18n("List only open folders"), page1);
    mImap.listOnlyOpenCheck->setWhatsThis(
        i18n("Only folders that are open (expanded) in the folder tree are checked for subfolders. Use this if there are many folders on the server.") );
    grid->addWidget( mImap.listOnlyOpenCheck, row, 0, 1, 2 );
  }

  ++row;
  mImap.includeInCheck =
    new QCheckBox( i18n("Include in manual mail chec&k"), page1 );
  grid->addWidget( mImap.includeInCheck, row, 0, 1, 2 );

  ++row;
  mImap.intervalCheck =
    new QCheckBox( i18n("Enable &interval mail checking"), page1 );
  grid->addWidget( mImap.intervalCheck, row, 0, 1, 3 );
  connect( mImap.intervalCheck, SIGNAL(toggled(bool)),
           this, SLOT(slotEnableImapInterval(bool)) );
  ++row;
  mImap.intervalLabel = new QLabel( i18n("Check inter&val:"), page1 );
  grid->addWidget( mImap.intervalLabel, row, 0 );
  mImap.intervalSpin = new KIntNumInput( page1 );
  mImap.intervalSpin->setRange( GlobalSettings::minimumCheckInterval(), 60, 1 );
  mImap.intervalSpin->setSliderEnabled( false );
  mImap.intervalSpin->setValue( 1 );
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
  mImap.useDefaultIdentityCheck = new QCheckBox( i18n("Use Default Identity"), page );
  connect( mImap.useDefaultIdentityCheck, SIGNAL( toggled(bool) ),
           this, SLOT( slotIdentityCheckboxChanged() ) );
  grid->addWidget( mImap.useDefaultIdentityCheck, row, 0 );

  ++row;
  mImap.identityLabel = new QLabel( i18n("Identity:"), page1 );
  grid->addWidget( mImap.identityLabel, row, 0 );
  mImap.identityCombo = new KPIMIdentities::IdentityCombo(kmkernel->identityManager(), page1 );
  mImap.identityLabel->setBuddy( mImap.identityCombo );
  grid->addWidget( mImap.identityCombo, row, 1 );

  QWidget *page2 = new QWidget( tabWidget );
  tabWidget->addTab( page2, i18n("S&ecurity") );
  QVBoxLayout *vlay = new QVBoxLayout( page2 );
  vlay->setSpacing( spacingHint() );
  vlay->setMargin( marginHint() );

  vlay->addSpacing( KDialog::spacingHint() );

  QHBoxLayout *buttonLay = new QHBoxLayout();
  vlay->addLayout( buttonLay );
  mImap.checkCapabilities =
    new QPushButton( i18n("Check &What the Server Supports"), page2 );
  connect(mImap.checkCapabilities, SIGNAL(clicked()),
    SLOT(slotCheckImapCapabilities()));
  buttonLay->addStretch();
  buttonLay->addWidget( mImap.checkCapabilities );
  buttonLay->addStretch();

  vlay->addSpacing( KDialog::spacingHint() );

  mImap.encryptionGroup = new QGroupBox( i18n("Encryption"), page2 );
  mImap.encryptionGroup->setLayout( new QVBoxLayout() );
  mImap.encryptionNone =
    new QRadioButton( i18n("&None"), mImap.encryptionGroup );
  mImap.encryptionSSL =
    new QRadioButton( i18n("Use &SSL for secure mail download"),
    mImap.encryptionGroup );
  mImap.encryptionTLS =
    new QRadioButton( i18n("Use &TLS for secure mail download"),
    mImap.encryptionGroup );
  mImap.encryptionGroup->layout()->addWidget( mImap.encryptionNone );
  mImap.encryptionGroup->layout()->addWidget( mImap.encryptionSSL );
  mImap.encryptionGroup->layout()->addWidget( mImap.encryptionTLS );

  mImap.encryptionButtonGroup = new QButtonGroup();
  mImap.encryptionButtonGroup->addButton( mImap.encryptionNone,
                                          Transport::EnumEncryption::None );
  mImap.encryptionButtonGroup->addButton( mImap.encryptionSSL,
                                          Transport::EnumEncryption::SSL );
  mImap.encryptionButtonGroup->addButton( mImap.encryptionTLS,
                                          Transport::EnumEncryption::TLS );

  connect( mImap.encryptionButtonGroup, SIGNAL(buttonClicked(int)),
           SLOT(slotImapEncryptionChanged(int)) );

  vlay->addWidget( mImap.encryptionGroup );

  mImap.authGroup = new QGroupBox( i18n("Authentication Method"), page2 );
  mImap.authGroup->setLayout( new QVBoxLayout() );
  mImap.authUser = new QRadioButton( i18n("Clear te&xt"), mImap.authGroup );
  mImap.authLogin = new QRadioButton( i18nc("Please translate this "
    "authentication method only if you have a good reason", "&LOGIN"),
    mImap.authGroup );
  mImap.authPlain = new QRadioButton( i18nc("Please translate this "
    "authentication method only if you have a good reason", "&PLAIN"),
     mImap.authGroup );
  mImap.authCramMd5 = new QRadioButton( i18n("CRAM-MD&5"), mImap.authGroup );
  mImap.authDigestMd5 = new QRadioButton( i18n("&DIGEST-MD5"), mImap.authGroup );
  mImap.authNTLM = new QRadioButton( i18n("&NTLM"), mImap.authGroup );
  mImap.authGSSAPI = new QRadioButton( i18n("&GSSAPI"), mImap.authGroup );
  mImap.authAnonymous = new QRadioButton( i18n("&Anonymous"), mImap.authGroup );

  mImap.authGroup->layout()->addWidget( mImap.authUser );
  mImap.authGroup->layout()->addWidget( mImap.authLogin );
  mImap.authGroup->layout()->addWidget( mImap.authPlain );
  mImap.authGroup->layout()->addWidget( mImap.authCramMd5 );
  mImap.authGroup->layout()->addWidget( mImap.authDigestMd5 );
  mImap.authGroup->layout()->addWidget( mImap.authNTLM );
  mImap.authGroup->layout()->addWidget( mImap.authGSSAPI );
  mImap.authGroup->layout()->addWidget( mImap.authAnonymous );

  mImap.authButtonGroup = new QButtonGroup();
  mImap.authButtonGroup->addButton( mImap.authUser );
  mImap.authButtonGroup->addButton( mImap.authLogin );
  mImap.authButtonGroup->addButton( mImap.authPlain );
  mImap.authButtonGroup->addButton( mImap.authCramMd5 );
  mImap.authButtonGroup->addButton( mImap.authDigestMd5 );
  mImap.authButtonGroup->addButton( mImap.authNTLM );
  mImap.authButtonGroup->addButton( mImap.authGSSAPI );
  mImap.authButtonGroup->addButton( mImap.authAnonymous );

  vlay->addWidget( mImap.authGroup );

  vlay->addStretch();

  // TODO (marc/bo): Test this
  mSieveConfigEditor = new SieveConfigEditor( tabWidget );
  mSieveConfigEditor->layout()->setMargin( KDialog::marginHint() );
  tabWidget->addTab( mSieveConfigEditor, i18n("&Filtering") );

  connect(KGlobalSettings::self(),SIGNAL(kdisplayFontChanged()),SLOT(slotFontChanged()));
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
        mLocal.locationEdit->addItem( acctLocal->location() );

    if ( acctLocal->procmailLockFileName().isEmpty() )
        acctLocal->setProcmailLockFileName( procmailrcParser.getLockFilesList().first() );
    else
        mLocal.procmailLockFileName->addItem( acctLocal->procmailLockFileName() );

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

    mLocal.intervalSpin->setValue( qMax(1, interval) );
    mLocal.intervalCheck->setChecked( interval >= 1 );
    mLocal.includeInCheck->setChecked( !mAccount->checkExclude() );
    mLocal.precommand->setText( mAccount->precommand() );

    slotEnableLocalInterval( interval >= 1 );
    folderCombo = mLocal.folderCombo;
  }
  else if( accountType == KAccount::Pop )
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
    mPop.intervalSpin->setValue( qMax(1, interval) );
    mPop.includeInCheck->setChecked( !mAccount->checkExclude() );
    mPop.precommand->setText( ap.precommand() );
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
  else if( accountType == KAccount::Imap )
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
    mImap.intervalCheck->setChecked( interval >= 1 );
    mImap.intervalSpin->setValue( qMax(1, interval) );
    mImap.includeInCheck->setChecked( !ai.checkExclude() );
    mImap.intervalCheck->setChecked( interval >= 1 );
    mImap.intervalSpin->setValue( qMax(1, interval) );
    QString trashfolder = ai.trash();
    if (trashfolder.isEmpty())
      trashfolder = kmkernel->trashFolder()->idString();
    mImap.trashCombo->setFolder( trashfolder );
    slotEnableImapInterval( interval >= 1 );
    mImap.identityCombo->setCurrentIdentity( mAccount->identityId() );
    mImap.useDefaultIdentityCheck->setChecked( mAccount->useDefaultIdentity() );
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
  else if( accountType == KAccount::DImap )
  {
    KMAcctCachedImap &ai = *(KMAcctCachedImap*)mAccount;
    mImap.nameEdit->setText( mAccount->name() );
    mImap.nameEdit->setFocus();
    mImap.loginEdit->setText( ai.login() );
    mImap.passwordEdit->setText( ai.passwd());
    mImap.hostEdit->setText( ai.host() );
    mImap.portEdit->setText( QString("%1").arg( ai.port() ) );
    mImap.hiddenFoldersCheck->setChecked( ai.hiddenFolders() );
    mImap.subscribedFoldersCheck->setChecked( ai.onlySubscribedFolders() );
    mImap.locallySubscribedFoldersCheck->setChecked( ai.onlyLocallySubscribedFolders() );
    mImap.storePasswordCheck->setChecked( ai.storePasswd() );
    mImap.intervalCheck->setChecked( interval >= 1 );
    mImap.intervalSpin->setValue( qMax(1, interval) );
    mImap.includeInCheck->setChecked( !ai.checkExclude() );
    mImap.intervalCheck->setChecked( interval >= 1 );
    mImap.intervalSpin->setValue( qMax(1, interval) );
    QString trashfolder = ai.trash();
    if (trashfolder.isEmpty())
      trashfolder = kmkernel->trashFolder()->idString();
    mImap.trashCombo->setFolder( trashfolder );
    slotEnableImapInterval( interval >= 1 );
    mImap.identityCombo->setCurrentIdentity( mAccount->identityId() );
    mImap.useDefaultIdentityCheck->setChecked( mAccount->useDefaultIdentity() );
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
  else if( accountType == KAccount::Maildir )
  {
    KMAcctMaildir *acctMaildir = dynamic_cast<KMAcctMaildir*>(mAccount);

    mMaildir.nameEdit->setText( mAccount->name() );
    mMaildir.nameEdit->setFocus();
    mMaildir.locationEdit->setEditText( acctMaildir->location() );

    mMaildir.intervalSpin->setValue( qMax(1, interval) );
    mMaildir.intervalCheck->setChecked( interval >= 1 );
    mMaildir.includeInCheck->setChecked( !mAccount->checkExclude() );
    mMaildir.precommand->setText( mAccount->precommand() );
    slotEnableMaildirInterval( interval >= 1 );
    folderCombo = mMaildir.folderCombo;
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
  if ( mServerTest && !mServerTest->capabilities().contains( ServerTest::UIDL ) &&
       mPop.leaveOnServerCheck->isChecked() ) {
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
       mPop.filterOnServerCheck->isChecked() ) {
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
      "of mail and downloading it."), QString(),
      "pipelining");
}


void AccountDialog::slotPopEncryptionChanged( int id )
{
  kDebug(5006) << "ID:" << id;
  // adjust port
  if ( id == Transport::EnumEncryption::SSL || mPop.portEdit->text() == "995" )
    mPop.portEdit->setText( ( id == Transport::EnumEncryption::SSL ) ? "995" : "110" );

  enablePopFeatures();
  const QAbstractButton *old = mPop.authButtonGroup->checkedButton();
  if ( old && !old->isEnabled() )
    checkHighest( mPop.authButtonGroup );
}

void AccountDialog::slotPopPasswordChanged(const QString& text)
{
  if ( text.isEmpty() )
    mPop.storePasswordCheck->setCheckState( Qt::Unchecked );
  else
    mPop.storePasswordCheck->setCheckState( Qt::Checked );
}

void AccountDialog::slotImapEncryptionChanged( int id )
{
  kDebug(5006) << id;
  // adjust port
  if ( id == Transport::EnumEncryption::SSL || mImap.portEdit->text() == "993" )
    mImap.portEdit->setText( ( id == Transport::EnumEncryption::SSL ) ? "993" : "143" );

  enableImapAuthMethods();
  QAbstractButton *old = mImap.authButtonGroup->checkedButton();
  if ( !old->isEnabled() )
    checkHighest( mImap.authButtonGroup );
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
  mServerTest = new ServerTest( this );
  Transport::EnumEncryption::type encryptionType;
  if ( mPop.encryptionSSL->isChecked() )
    encryptionType = Transport::EnumEncryption::SSL;
  else
    encryptionType = Transport::EnumEncryption::None;
  mServerTest->setPort( encryptionType, mPop.portEdit->text().toInt() );
  mServerTest->setServer( mPop.hostEdit->text() );
  mServerTest->setProtocol( "pop" );
  connect( mServerTest, SIGNAL( finished(QList<int>) ),
           this, SLOT( slotPopCapabilities(QList<int>) ) );
  mServerTest->start();
  mServerTestFailed = false;
  mPop.checkCapabilities->setEnabled( false );
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
  mServerTest = new ServerTest( this );
  Transport::EnumEncryption::type encryptionType;
  if ( mImap.encryptionSSL->isChecked() )
    encryptionType = Transport::EnumEncryption::SSL;
  else
    encryptionType = Transport::EnumEncryption::None;
  mServerTest->setPort( encryptionType, mImap.portEdit->text().toInt() );
  mServerTest->setServer( mImap.hostEdit->text() );
  mServerTest->setProtocol( "imap" );
  connect( mServerTest, SIGNAL( finished(QList<int>) ),
           this, SLOT( slotImapCapabilities(QList<int>) ) );
  mServerTest->start();
  mServerTestFailed = false;
  mImap.checkCapabilities->setEnabled(false);
}

void AccountDialog::slotPopCapabilities( QList<int> encryptionTypes )
{
  mPop.checkCapabilities->setEnabled( true );

  // If the servertest did not find any useable authentication modes, assume the
  // connection failed and don't disable any of the radioboxes.
  if ( encryptionTypes.isEmpty() ) {
    mServerTestFailed = true;
    return;
  }

  mPop.encryptionNone->setEnabled( encryptionTypes.contains( Transport::EnumEncryption::None ) );
  mPop.encryptionSSL->setEnabled( encryptionTypes.contains( Transport::EnumEncryption::SSL ) );
  mPop.encryptionTLS->setEnabled(  encryptionTypes.contains( Transport::EnumEncryption::TLS )  );
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

  mPop.authPlain->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::PLAIN ) );
  mPop.authLogin->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::LOGIN ) );
  mPop.authCRAM_MD5->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::CRAM_MD5 ) );
  mPop.authDigestMd5->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::DIGEST_MD5 ) );
  mPop.authNTLM->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::NTLM ) );
  mPop.authGSSAPI->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::GSSAPI ) );
  mPop.authAPOP->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::APOP ) );

  if ( mServerTest && !mServerTest->capabilities().contains( ServerTest::Pipelining ) &&
       mPop.usePipeliningCheck->isChecked() ) {
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

  if ( mServerTest && !mServerTest->capabilities().contains( ServerTest::UIDL ) &&
       mPop.leaveOnServerCheck->isChecked() ) {
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

  if ( mServerTest && !mServerTest->capabilities().contains( ServerTest::Top ) &&
       mPop.filterOnServerCheck->isChecked() ) {
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

void AccountDialog::slotImapCapabilities( QList<int> encryptionTypes )
{
  mImap.checkCapabilities->setEnabled( true );

  // If the servertest did not find any useable authentication modes, assume the
  // connection failed and don't disable any of the radioboxes.
  if ( encryptionTypes.isEmpty() ) {
    mServerTestFailed = true;
    return;
  }

  mImap.encryptionNone->setEnabled( encryptionTypes.contains( Transport::EnumEncryption::None ) );
  mImap.encryptionSSL->setEnabled( encryptionTypes.contains( Transport::EnumEncryption::SSL ) );
  mImap.encryptionTLS->setEnabled(  encryptionTypes.contains( Transport::EnumEncryption::TLS )  );
  checkHighest( mImap.encryptionButtonGroup );
}

void AccountDialog::slotLeaveOnServerDaysChanged ( int value )
{
  mPop.leaveOnServerDaysSpin->setSuffix( i18np(" day", " days", value) );
}


void AccountDialog::slotLeaveOnServerCountChanged ( int value )
{
  mPop.leaveOnServerCountSpin->setSuffix( i18np(" message", " messages", value) );
}


void AccountDialog::slotFilterOnServerSizeChanged ( int value )
{
  mPop.filterOnServerSizeSpin->setSuffix( i18np(" byte", " bytes", value) );
}

void AccountDialog::slotIdentityCheckboxChanged()
{
  if ( mAccount->type() == KAccount::Imap ||
       mAccount->type() == KAccount::DImap  ) {
     mImap.identityCombo->setEnabled( !mImap.useDefaultIdentityCheck->isChecked() );
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

  mImap.authPlain->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::PLAIN ) );
  mImap.authLogin->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::LOGIN ) );
  mImap.authCramMd5->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::CRAM_MD5 ) );
  mImap.authDigestMd5->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::DIGEST_MD5 ) );
  mImap.authNTLM->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::NTLM ) );
  mImap.authGSSAPI->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::GSSAPI ) );
  mImap.authAnonymous->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::ANONYMOUS ) );
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
    mAccount->setCheckExclude( !mLocal.includeInCheck->isChecked() );

    mAccount->setPrecommand( mLocal.precommand->text() );

    mAccount->setFolder( mFolderList.at(mLocal.folderCombo->currentIndex()) );
  }
  else if( accountType == KAccount::Pop )
  {
    mAccount->setName( mPop.nameEdit->text() );
    mAccount->setCheckInterval( mPop.intervalCheck->isChecked() ?
                                mPop.intervalSpin->value() : 0 );
    mAccount->setCheckExclude( !mPop.includeInCheck->isChecked() );

    mAccount->setFolder( mFolderList.at(mPop.folderCombo->currentIndex()) );

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
  else if( accountType == KAccount::Imap )
  {
    mAccount->setName( mImap.nameEdit->text() );
    mAccount->setCheckInterval( mImap.intervalCheck->isChecked() ?
                                mImap.intervalSpin->value() : 0 );
    mAccount->setIdentityId( mImap.identityCombo->currentIdentity() );
    mAccount->setUseDefaultIdentity( mImap.useDefaultIdentityCheck->isChecked() );
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
    epa.setCheckExclude( !mImap.includeInCheck->isChecked() );
    if ( mSieveConfigEditor )
      epa.setSieveConfig( mSieveConfigEditor->config() );
  }
  else if( accountType == KAccount::DImap )
  {
    mAccount->setName( mImap.nameEdit->text() );
    mAccount->setCheckInterval( mImap.intervalCheck->isChecked() ?
                                mImap.intervalSpin->value() : 0 );
    mAccount->setIdentityId( mImap.identityCombo->currentIdentity() );
    mAccount->setUseDefaultIdentity( mImap.useDefaultIdentityCheck->isChecked() );
    mAccount->setCheckExclude( !mImap.includeInCheck->isChecked() );
    //mAccount->setFolder( NULL );
    mAccount->setFolder( kmkernel->dimapFolderMgr()->findById(mAccount->id()) );
    //kDebug(5006) <<"account for folder" << mAccount->folder()->name();

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
    epa.setCheckExclude( !mImap.includeInCheck->isChecked() );
    if ( mSieveConfigEditor )
      epa.setSieveConfig( mSieveConfigEditor->config() );
  }
  else if( accountType == KAccount::Maildir )
  {
    KMAcctMaildir *acctMaildir = dynamic_cast<KMAcctMaildir*>(mAccount);

    if (acctMaildir) {
        mAccount->setName( mMaildir.nameEdit->text() );
        acctMaildir->setLocation( mMaildir.locationEdit->currentText() );

        KMFolder *targetFolder = mFolderList.at(mMaildir.folderCombo->currentIndex());
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
    mAccount->setCheckExclude( !mMaildir.includeInCheck->isChecked() );

    mAccount->setPrecommand( mMaildir.precommand->text() );
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
      newAcct->setFolder( mFolderList.at(mLocal.folderCombo->currentIndex()), true );
    } else if ( accountType == KAccount::Pop ) {
      newAcct->setFolder( mFolderList.at(mPop.folderCombo->currentIndex()), true );
    } else if ( accountType == KAccount::Maildir ) {
      newAcct->setFolder( mFolderList.at(mMaildir.folderCombo->currentIndex()), true );
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
  KAccount::Type accountType = mAccount->type();
  if( accountType == KAccount::Local )
  {
    QFont titleFont( mLocal.titleLabel->font() );
    titleFont.setBold( true );
    mLocal.titleLabel->setFont(titleFont);
  }
  else if( accountType == KAccount::Pop )
  {
    QFont titleFont( mPop.titleLabel->font() );
    titleFont.setBold( true );
    mPop.titleLabel->setFont(titleFont);
  }
  else if( accountType == KAccount::Imap )
  {
    QFont titleFont( mImap.titleLabel->font() );
    titleFont.setBold( true );
    mImap.titleLabel->setFont(titleFont);
  }
}

void AccountDialog::slotReloadNamespaces()
{
  if ( mAccount->type() == KAccount::Imap ||
       mAccount->type() == KAccount::DImap )
  {
    initAccountForConnect();
    mImap.personalNS->setText( i18n("Fetching Namespaces...") );
    mImap.otherUsersNS->setText( QString() );
    mImap.sharedNS->setText( QString() );
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
    mImap.personalNS->setText( QString() );
  }
}

void AccountDialog::slotSetupNamespaces( const ImapAccountBase::nsDelimMap& map )
{
  disconnect( this, SLOT( slotSetupNamespaces( const ImapAccountBase::nsDelimMap& ) ) );
  mImap.personalNS->setText( QString() );
  mImap.otherUsersNS->setText( QString() );
  mImap.sharedNS->setText( QString() );
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
    na.setHost( mPop.hostEdit->text().trimmed() );
    na.setPort( mPop.portEdit->text().toInt() );
    na.setLogin( mPop.loginEdit->text().trimmed() );
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
  else if ( type == KAccount::Imap ||
            type == KAccount::DImap ) {
    na.setHost( mImap.hostEdit->text().trimmed() );
    na.setPort( mImap.portEdit->text().toInt() );
    na.setLogin( mImap.loginEdit->text().trimmed() );
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
