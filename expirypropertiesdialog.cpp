
#include "expirypropertiesdialog.h"
#include "folderrequester.h"
#include "kmfoldertree.h"
#include "kmglobal.h"

#include <qvariant.h>
#include <qpushbutton.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qspinbox.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

#include <klocale.h>
#include <kmessagebox.h>

using namespace KMail;

/*
 *  Constructs a ExpiryPropertiesDialog as a child of 'parent', with the
 *  name 'name'.
 *
 */
ExpiryPropertiesDialog::ExpiryPropertiesDialog( KMFolderTree* tree, KMFolder* folder )
    : KDialogBase( tree, "expiry_properties", false, i18n( "Mail Expiry Properties" ), 
                   KDialogBase::Ok|KDialogBase::Cancel, 
                   KDialogBase::Ok, true ),
      mFolder( folder )
{
  setWFlags( getWFlags() | WDestructiveClose );
  QWidget* privateLayoutWidget = new QWidget( this, "globalVBox" );
  setMainWidget( privateLayoutWidget );
  privateLayoutWidget->setGeometry( QRect( 10, 20, 270, 138 ) );
  globalVBox = new QVBoxLayout( privateLayoutWidget, 11, 6, "globalVBox"); 
  globalVBox->setSpacing( 20 );

  readHBox = new QHBoxLayout( 0, 0, 6, "readHBox"); 

  expireReadMailCB = new QCheckBox( privateLayoutWidget, "expireReadMailCB" );
  expireReadMailCB->setText( i18n( "Expire read mails after" ) );
  connect( expireReadMailCB, SIGNAL( toggled( bool )),
           this, SLOT( slotUpdateControls() ) );
  readHBox->addWidget( expireReadMailCB );

  expireReadMailSB = new QSpinBox( privateLayoutWidget, "expireReadMailSB" );
  expireReadMailSB->setMaxValue( 999999 );
  expireReadMailSB->setValue( 30 );
  readHBox->addWidget( expireReadMailSB );

  labelDays = new QLabel( privateLayoutWidget, "labelDays" );
  labelDays->setText( i18n( "days" ) );
  readHBox->addWidget( labelDays );
  globalVBox->addLayout( readHBox );

  unreadHBox = new QHBoxLayout( 0, 0, 6, "unreadHBox"); 

  expireUnreadMailCB = new QCheckBox( privateLayoutWidget, "expireUnreadMailCB" );
  expireUnreadMailCB->setText( i18n( "Expire unread mails after" ) );
  connect( expireUnreadMailCB, SIGNAL( toggled( bool )),
           this, SLOT( slotUpdateControls() ) );
  unreadHBox->addWidget( expireUnreadMailCB );

  expireUnreadMailSB = new QSpinBox( privateLayoutWidget, "expireUnreadMailSB" );
  expireUnreadMailSB->setMaxValue( 99999 );
  expireUnreadMailSB->setValue( 30 );
  unreadHBox->addWidget( expireUnreadMailSB );

  labelDays2 = new QLabel( privateLayoutWidget, "labelDays2" );
  labelDays2->setText( i18n( "days" ) );
  labelDays2->setAlignment( int( QLabel::AlignTop ) );
  unreadHBox->addWidget( labelDays2 );
  globalVBox->addLayout( unreadHBox );

  expiryActionHBox = new QHBoxLayout( 0, 0, 6, "expiryActionHBox"); 

  expiryActionLabel = new QLabel( privateLayoutWidget, "expiryActionLabel" );
  expiryActionLabel->setText( i18n( "Expiry action:" ) );
  expiryActionLabel->setAlignment( int( QLabel::AlignVCenter ) );
  expiryActionHBox->addWidget( expiryActionLabel );

  actionsHBox = new QVBoxLayout( 0, 0, 6, "actionsHBox"); 
  actionsGroup = new QButtonGroup( this );
  actionsGroup->hide(); // for mutual exclusion of the radio buttons

  moveToHBox = new QHBoxLayout( 0, 0, 6, "moveToHBox"); 

  moveToRB = new QRadioButton( privateLayoutWidget, "moveToRB" );
  actionsGroup->insert( moveToRB );
  connect( moveToRB, SIGNAL( toggled( bool ) ),
           this, SLOT( slotUpdateControls() ) );
  moveToRB->setText( i18n( "Move to:" ) );
  moveToHBox->addWidget( moveToRB );

  folderSelector = new KMail::FolderRequester( privateLayoutWidget, tree );
  folderSelector->setMustBeReadWrite( true );
  moveToHBox->addWidget( folderSelector );
  actionsHBox->addLayout( moveToHBox );

  deletePermanentlyRB = new QRadioButton( privateLayoutWidget, "deletePermanentlyRB" );
  actionsGroup->insert( deletePermanentlyRB );
  deletePermanentlyRB->setText( i18n( "Delete permanently" ) );
  actionsHBox->addWidget( deletePermanentlyRB );
  expiryActionHBox->addLayout( actionsHBox );
  globalVBox->addLayout( expiryActionHBox );

  note = new QLabel( privateLayoutWidget, "note" );
  note->setText( i18n( "Note: Expiry action will be applied immediately after confirming settings." ) );
  note->setAlignment( int( QLabel::WordBreak | QLabel::AlignVCenter ) );
  globalVBox->addWidget( note );

  // Load the values from the folder
  bool expiryGloballyOn = mFolder->isAutoExpire();
  int daysToExpireRead, daysToExpireUnread;
  mFolder->daysToExpire( daysToExpireRead, daysToExpireUnread );

  if ( expiryGloballyOn 
      && mFolder->getReadExpireUnits() != expireNever 
      && daysToExpireRead >= 0 ) {
    expireReadMailCB->setChecked( true );
    expireReadMailSB->setValue( daysToExpireRead );
  }
  if ( expiryGloballyOn
      && mFolder->getUnreadExpireUnits() != expireNever 
      && daysToExpireUnread >= 0 ) {
    expireUnreadMailCB->setChecked( true );
    expireUnreadMailSB->setValue( daysToExpireUnread );
  }

  if ( mFolder->expireAction() == KMFolder::ExpireDelete )
    deletePermanentlyRB->setChecked( true );
  else
    moveToRB->setChecked( true );

  QString destFolderID = mFolder->expireToFolderId();
  if ( !destFolderID.isEmpty() ) {
    KMFolder* destFolder = kmkernel->findFolderById( destFolderID );
    if ( destFolder )
      folderSelector->setFolder( destFolder );
  }
  slotUpdateControls();
  resize( QSize(295, 204).expandedTo(minimumSizeHint()) );
  clearWState( WState_Polished );
}

/*
 *  Destroys the object and frees any allocated resources
 */
ExpiryPropertiesDialog::~ExpiryPropertiesDialog()
{
    // no need to delete child widgets, Qt does it all for us
}

void ExpiryPropertiesDialog::slotOk()
{
  bool enableGlobally = expireReadMailCB->isChecked() || expireUnreadMailCB->isChecked();
  if ( enableGlobally && moveToRB->isChecked() && !folderSelector->folder() ) {
    KMessageBox::error( this, i18n("Please select a folder to expire messages into."),
        i18n( "No Folder Selected" ) );
    return;
  } 
  mFolder->setAutoExpire( enableGlobally );
  // we always write out days now
  mFolder->setReadExpireAge( expireReadMailSB->value() );
  mFolder->setUnreadExpireAge( expireUnreadMailSB->value() );
  mFolder->setReadExpireUnits( expireReadMailCB->isChecked()? expireDays : expireNever );
  mFolder->setUnreadExpireUnits( expireUnreadMailCB->isChecked()? expireDays : expireNever );

  if ( deletePermanentlyRB->isChecked() )
    mFolder->setExpireAction( KMFolder::ExpireDelete );
  else
    mFolder->setExpireAction( KMFolder::ExpireMove );
  KMFolder* expireToFolder = folderSelector->folder();
  if ( expireToFolder )
    mFolder->setExpireToFolderId( expireToFolder->idString() );

  // trigger immediate expiry if there is something to do
  if ( enableGlobally )
    mFolder->expireOldMessages( true /*immediate*/);
  KDialogBase::slotOk();
}

void ExpiryPropertiesDialog::slotUpdateControls()
{
  bool showExpiryActions = expireReadMailCB->isChecked() || expireUnreadMailCB->isChecked();
  moveToRB->setEnabled( showExpiryActions );
  folderSelector->setEnabled( showExpiryActions && moveToRB->isChecked() );
  deletePermanentlyRB->setEnabled( showExpiryActions );
}


#include "expirypropertiesdialog.moc"
