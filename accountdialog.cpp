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

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qlayout.h>
#include <qspinbox.h>
#include <qvalidator.h> 

#include <kapp.h>
#include <kconfig.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "accountdialog.h"
#include "kmacctlocal.h"
#include "kmacctmgr.h"
#include "kmacctpop.h"
#include "kmacctexppop.h"
#include "kmfolder.h"
#include "kmfoldermgr.h"
#include "kmglobal.h"

#include "accountdialog.moc"

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
  else if( accountType == "pop" || accountType == "experimental pop" )
  {
    makePopAccountPage();
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
  QFrame *page = makeMainWidget();
  QGridLayout *topLayout = new QGridLayout( page, 10, 3, 0, spacingHint() );
  topLayout->addColSpacing( 1, fontMetrics().maxWidth()*15 );
  topLayout->setRowStretch( 9, 10 );
  topLayout->setColStretch( 1, 10 );

  mLocal.titleLabel = new QLabel( i18n("Account type: Local account"), page );
  topLayout->addMultiCellWidget( mLocal.titleLabel, 0, 0, 0, 2 );
  QFont titleFont( mLocal.titleLabel->font() );
  titleFont.setBold( true );
  mLocal.titleLabel->setFont( titleFont );
  QFrame *hline = new QFrame( page );
  hline->setFrameStyle( QFrame::Sunken | QFrame::HLine );
  topLayout->addMultiCellWidget( hline, 1, 1, 0, 2 );

  QLabel *label = new QLabel( i18n("Name:"), page );
  topLayout->addWidget( label, 2, 0 );
  mLocal.nameEdit = new QLineEdit( page );
  topLayout->addWidget( mLocal.nameEdit, 2, 1 );

  label = new QLabel( i18n("Location:"), page );
  topLayout->addWidget( label, 3, 0 );
  mLocal.locationEdit = new QLineEdit( page );
  topLayout->addWidget( mLocal.locationEdit, 3, 1 );
  QPushButton *choose = new QPushButton( i18n("Choose..."), page );
  choose->setAutoDefault( false );
  connect( choose, SIGNAL(clicked()), this, SLOT(slotLocationChooser()) );
  topLayout->addWidget( choose, 3, 2 );
 
  mLocal.excludeCheck = 
    new QCheckBox( i18n("Exclude from \"Check Mail\""), page );
  topLayout->addMultiCellWidget( mLocal.excludeCheck, 4, 4, 0, 2 );

  mLocal.intervalCheck = 
    new QCheckBox( i18n("Enable interval mail checking"), page );
  topLayout->addMultiCellWidget( mLocal.intervalCheck, 5, 5, 0, 2 );
  connect( mLocal.intervalCheck, SIGNAL(toggled(bool)),
	   this, SLOT(slotEnableLocalInterval(bool)) );
  mLocal.intervalLabel = new QLabel( i18n("Check interval (minutes):"), page );
  topLayout->addWidget( mLocal.intervalLabel, 6, 0 );
  mLocal.intervalSpin = new QSpinBox( page );
  mLocal.intervalSpin->setRange( 1, 10000 );
  mLocal.intervalSpin->setValue( 1 );
  topLayout->addWidget( mLocal.intervalSpin, 6, 1 );

  label = new QLabel( i18n("Destination folder:"), page );
  topLayout->addWidget( label, 7, 0 );
  mLocal.folderCombo = new QComboBox( false, page );
  topLayout->addWidget( mLocal.folderCombo, 7, 1 );

  label = new QLabel( i18n("Default identity:"), page );
  topLayout->addWidget( label, 8, 0 );
  mLocal.identityCombo = new QComboBox( false, page );
  topLayout->addWidget( mLocal.identityCombo, 8, 1 );

  label->setEnabled(false);
  mLocal.identityCombo->setEnabled(false);

  connect(kapp,SIGNAL(kdisplayFontChanged()),SLOT(slotFontChanged()));
}



void AccountDialog::makePopAccountPage()
{
  QFrame *page = makeMainWidget();
  QGridLayout *topLayout = new QGridLayout( page, 16, 2, 0, spacingHint() );
  topLayout->addColSpacing( 1, fontMetrics().maxWidth()*15 );
  topLayout->setRowStretch( 16, 10 );
  topLayout->setColStretch( 1, 10 );
  
  mPop.titleLabel = new QLabel( page );
  if( QString(mAccount->type()) == "pop" )
  {
    mPop.titleLabel->setText( i18n("Account type: Pop Account") );
  }
  else
  {
    mPop.titleLabel->setText( i18n("Account type: Advanced Pop Account") );
  }
  QFont titleFont( mPop.titleLabel->font() );
  titleFont.setBold( true );
  mPop.titleLabel->setFont( titleFont );
  topLayout->addMultiCellWidget( mPop.titleLabel, 0, 0, 0, 1 );
  QFrame *hline = new QFrame( page );
  hline->setFrameStyle( QFrame::Sunken | QFrame::HLine );
  topLayout->addMultiCellWidget( hline, 1, 1, 0, 1 );

  QLabel *label = new QLabel( i18n("Name:"), page );
  topLayout->addWidget( label, 2, 0 );
  mPop.nameEdit = new QLineEdit( page );
  topLayout->addWidget( mPop.nameEdit, 2, 1 );

  label = new QLabel( i18n("Login:"), page );
  topLayout->addWidget( label, 3, 0 );
  mPop.loginEdit = new QLineEdit( page );
  topLayout->addWidget( mPop.loginEdit, 3, 1 );

  label = new QLabel( i18n("Password:"), page );
  topLayout->addWidget( label, 4, 0 );
  mPop.passwordEdit = new QLineEdit( page );
  mPop.passwordEdit->setEchoMode( QLineEdit::Password );
  topLayout->addWidget( mPop.passwordEdit, 4, 1 );  

  label = new QLabel( i18n("Host:"), page );
  topLayout->addWidget( label, 5, 0 );
  mPop.hostEdit = new QLineEdit( page );
  topLayout->addWidget( mPop.hostEdit, 5, 1 );  

  label = new QLabel( i18n("Port:"), page );
  topLayout->addWidget( label, 6, 0 );
  mPop.portEdit = new QLineEdit( page );
  mPop.portEdit->setValidator( new QIntValidator(this) );
  topLayout->addWidget( mPop.portEdit, 6, 1 ); 

  mPop.storePasswordCheck = 
    new QCheckBox( i18n("Store POP password in configuration file"), page );
  topLayout->addMultiCellWidget( mPop.storePasswordCheck, 7, 7, 0, 1 );
  
  mPop.deleteMailCheck = 
    new QCheckBox( i18n("Delete mail from server"), page );
  topLayout->addMultiCellWidget( mPop.deleteMailCheck, 8, 8, 0, 1 );

  mPop.retriveAllCheck = 
    new QCheckBox( i18n("Retrive all mail from server"), page );
  topLayout->addMultiCellWidget( mPop.retriveAllCheck, 9, 9, 0, 1 );  

  mPop.excludeCheck = 
    new QCheckBox( i18n("Exclude from \"Check Mail\""), page );
  topLayout->addMultiCellWidget( mPop.excludeCheck, 10, 10, 0, 1 );

  mPop.intervalCheck = 
    new QCheckBox( i18n("Enable interval mail checking"), page );
  topLayout->addMultiCellWidget( mPop.intervalCheck, 11, 11, 0, 1 );
  connect( mPop.intervalCheck, SIGNAL(toggled(bool)),
	   this, SLOT(slotEnablePopInterval(bool)) );
  mPop.intervalLabel = new QLabel( i18n("Check interval (minutes):"), page );
  topLayout->addWidget( mPop.intervalLabel, 12, 0 );
  mPop.intervalSpin = new QSpinBox( page );
  mPop.intervalSpin->setRange( 1, 10000 );
  mPop.intervalSpin->setValue( 1 );
  topLayout->addWidget( mPop.intervalSpin, 12, 1 );

  label = new QLabel( i18n("Destination folder:"), page );
  topLayout->addWidget( label, 13, 0 );
  mPop.folderCombo = new QComboBox( false, page );
  topLayout->addWidget( mPop.folderCombo, 13, 1 );

  label = new QLabel( i18n("Default identity:"), page );
  topLayout->addWidget( label, 14, 0 );
  mPop.identityCombo = new QComboBox( false, page );
  topLayout->addWidget( mPop.identityCombo, 14, 1 );

  label->setEnabled(false);
  mPop.identityCombo->setEnabled(false);

  connect(kapp,SIGNAL(kdisplayFontChanged()),SLOT(slotFontChanged()));
}


void AccountDialog::setupSettings()
{
  QComboBox *folderCombo;
  int interval = mAccount->checkInterval();

  QString accountType = mAccount->type();
  if( accountType == "local" )
  {
    mLocal.nameEdit->setText( mAccount->name() );
    mLocal.locationEdit->setText( ((KMAcctLocal*)mAccount)->location() );
    mLocal.intervalSpin->setValue( QMAX(1, interval) ); 
    mLocal.intervalCheck->setChecked( interval >= 1 );
    mLocal.excludeCheck->setChecked( mAccount->checkExclude() );

    slotEnableLocalInterval( interval >= 1 );
    folderCombo = mLocal.folderCombo;
    mLocal.identityCombo->insertStringList( mIdentityList );
  }
  else
  {
    KMAcctPop &ap = *(KMAcctPop*)mAccount;
    mPop.nameEdit->setText( mAccount->name() );
    mPop.loginEdit->setText( ap.login() );
    mPop.passwordEdit->setText( ap.passwd());
    mPop.hostEdit->setText( ap.host() );
    mPop.portEdit->setText( QString("%1").arg( ap.port() ) ); 
    mPop.storePasswordCheck->setChecked( ap.storePasswd() );
    mPop.deleteMailCheck->setChecked( !ap.leaveOnServer() );
    mPop.retriveAllCheck->setChecked( ap.retrieveAll() );
    mPop.intervalCheck->setChecked( interval >= 1 );
    mPop.intervalSpin->setValue( QMAX(1, interval) );
    mPop.excludeCheck->setChecked( mAccount->checkExclude() );
    
    slotEnablePopInterval( interval >= 1 );
    folderCombo = mPop.folderCombo;
    mPop.identityCombo->insertStringList( mIdentityList );
  }

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
    uint i=0;
    for( KMFolder *folder = (KMFolder*)fdir->first(); folder != 0;
	 folder = (KMFolder*)fdir->next() )
    {
      if( folder->isDir() || folder->isSystemFolder() ) 
      {
	continue;
      }
      folderCombo->insertItem( folder->name() );
      if( folder == acctFolder )
      {
	folderCombo->setCurrentItem(i);
      }
      i++;
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
    mAccount->setName( mLocal.nameEdit->text() );
    ((KMAcctLocal*)mAccount)->setLocation( mLocal.locationEdit->text() );
    mAccount->setCheckInterval( mLocal.intervalCheck->isChecked() ? 
			     mLocal.intervalSpin->value() : 0 );
    mAccount->setCheckExclude( mLocal.excludeCheck->isChecked() );
 
    KMFolder *folder 
      = kernel->folderMgr()->find( mLocal.folderCombo->currentText() );
    mAccount->setFolder( folder );

  }
  else if( accountType == "pop" || accountType == "experimental pop" )
  {
    mAccount->setName( mPop.nameEdit->text() );
    mAccount->setCheckInterval( mPop.intervalCheck->isChecked() ? 
			     mPop.intervalSpin->value() : 0 );
    mAccount->setCheckExclude( mPop.excludeCheck->isChecked() );

    KMFolder *folder 
      = kernel->folderMgr()->find( mPop.folderCombo->currentText() );
    mAccount->setFolder( folder );
    
    if( accountType == "pop" )
    {
      KMAcctPop &pa = *(KMAcctPop*)mAccount;
      pa.setHost( mPop.hostEdit->text() );
      pa.setPort( mPop.portEdit->text().toInt() );
      pa.setLogin( mPop.loginEdit->text() );
      pa.setPasswd( mPop.passwordEdit->text(), true );
      pa.setStorePasswd( mPop.storePasswordCheck->isChecked() );
      pa.setPasswd( mPop.passwordEdit->text(), pa.storePasswd() );
      pa.setLeaveOnServer( !mPop.deleteMailCheck->isChecked() );
      pa.setRetrieveAll( mPop.retriveAllCheck->isChecked() );
    }
    else
    {
      KMAcctExpPop &epa = *(KMAcctExpPop*)mAccount;
      epa.setHost( mPop.hostEdit->text() );
      epa.setPort( mPop.portEdit->text().toInt() );
      epa.setLogin( mPop.loginEdit->text() );
      epa.setPasswd( mPop.passwordEdit->text(), true );
      epa.setStorePasswd( mPop.storePasswordCheck->isChecked() );
      epa.setPasswd( mPop.passwordEdit->text(), epa.storePasswd() );
      epa.setLeaveOnServer( !mPop.deleteMailCheck->isChecked() );
      epa.setRetrieveAll( mPop.retriveAllCheck->isChecked() );
    }
  }
  kernel->acctMgr()->writeConfig(TRUE);
}


void AccountDialog::slotLocationChooser()
{
  static QString directory( "/" );
  
  KFileDialog dialog( directory, "*", this, 0, true );
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
  
  mLocal.locationEdit->setText( url.path() );
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
  else
  {
    QFont titleFont( mPop.titleLabel->font() );
    titleFont.setBold( true );
    mPop.titleLabel->setFont(titleFont);
  }
}


