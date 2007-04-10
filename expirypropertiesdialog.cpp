#include "config.h"

#include "expirypropertiesdialog.h"
#include "folderrequester.h"
#include "kmfolder.h"
#include "kmfoldertree.h"

#include <QVariant>
#include <QPushButton>
#include <q3groupbox.h>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QRadioButton>
#include <q3buttongroup.h>
#include <QComboBox>
#include <QLayout>
#include <QToolTip>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <klocale.h>
#include <kmessagebox.h>

using namespace KMail;

/*
 *  Constructs a ExpiryPropertiesDialog as a child of 'parent', with the
 *  name 'name'.
 *
 */
ExpiryPropertiesDialog::ExpiryPropertiesDialog( KMFolderTree* tree, KMFolder* folder )
    : KDialog( tree ),
      mFolder( folder )
{
  setCaption( i18n( "Mail Expiry Properties" ) );
  setButtons( Ok|Cancel );
  setModal( false );
  setObjectName( "expiry_properties" );
  setAttribute( Qt::WA_DeleteOnClose );

  connect( this, SIGNAL( okClicked() ), SLOT( slotOk() ) );

  QWidget* privateLayoutWidget = new QWidget( this );
  privateLayoutWidget->setObjectName( "globalVBox" );
  setMainWidget( privateLayoutWidget );
  privateLayoutWidget->setGeometry( QRect( 10, 20, 270, 138 ) );
  globalVBox = new QVBoxLayout( privateLayoutWidget );
  globalVBox->setMargin( 11 );
  globalVBox->setObjectName( "globalVBox" );
  globalVBox->setSpacing( 20 );

  readHBox = new QHBoxLayout();
  readHBox->setMargin( 0 );
  readHBox->setSpacing( 6 );
  readHBox->setObjectName( "readHBox" );

  expireReadMailCB = new QCheckBox( privateLayoutWidget );
  expireReadMailCB->setObjectName( "expireReadMailCB" );
  expireReadMailCB->setText( i18n( "Expire read mails after" ) );
  connect( expireReadMailCB, SIGNAL( toggled( bool )),
           this, SLOT( slotUpdateControls() ) );
  readHBox->addWidget( expireReadMailCB );

  expireReadMailSB = new QSpinBox( privateLayoutWidget );
  expireReadMailSB->setObjectName( "expireReadMailSB" );
  expireReadMailSB->setMaximum( 999999 );
  expireReadMailSB->setValue( 30 );
  readHBox->addWidget( expireReadMailSB );

  labelDays = new QLabel( privateLayoutWidget );
  labelDays->setObjectName( "labelDays" );
  labelDays->setText( i18n( "days" ) );
  readHBox->addWidget( labelDays );
  globalVBox->addLayout( readHBox );

  unreadHBox = new QHBoxLayout();
  unreadHBox->setMargin( 0 );
  unreadHBox->setSpacing( 6 );
  unreadHBox->setObjectName( "unreadHBox" );

  expireUnreadMailCB = new QCheckBox( privateLayoutWidget );
  expireUnreadMailCB->setObjectName( "expireUnreadMailCB" );
  expireUnreadMailCB->setText( i18n( "Expire unread mails after" ) );
  connect( expireUnreadMailCB, SIGNAL( toggled( bool )),
           this, SLOT( slotUpdateControls() ) );
  unreadHBox->addWidget( expireUnreadMailCB );

  expireUnreadMailSB = new QSpinBox( privateLayoutWidget );
  expireUnreadMailSB->setObjectName( "expireUnreadMailSB" );
  expireUnreadMailSB->setMaximum( 99999 );
  expireUnreadMailSB->setValue( 30 );
  unreadHBox->addWidget( expireUnreadMailSB );

  labelDays2 = new QLabel( privateLayoutWidget );
  labelDays2->setObjectName( "labelDays2" );
  labelDays2->setText( i18n( "days" ) );
  labelDays2->setAlignment( Qt::AlignTop );
  unreadHBox->addWidget( labelDays2 );
  globalVBox->addLayout( unreadHBox );

  expiryActionHBox = new QHBoxLayout();
  expiryActionHBox->setMargin( 0 );
  expiryActionHBox->setSpacing( 6 );
  expiryActionHBox->setObjectName( "expiryActionHBox" );

  expiryActionLabel = new QLabel( privateLayoutWidget );
  expiryActionLabel->setObjectName( "expiryActionLabel" );
  expiryActionLabel->setText( i18n( "Expiry action:" ) );
  expiryActionLabel->setAlignment( Qt::AlignVCenter );
  expiryActionHBox->addWidget( expiryActionLabel );

  actionsHBox = new QVBoxLayout();
  actionsHBox->setMargin( 0 );
  actionsHBox->setSpacing( 6 );
  actionsHBox->setObjectName( "actionsHBox" );
  actionsGroup = new Q3ButtonGroup( this );
  actionsGroup->hide(); // for mutual exclusion of the radio buttons

  moveToHBox = new QHBoxLayout();
  moveToHBox->setMargin( 0 );
  moveToHBox->setSpacing( 6 );
  moveToHBox->setObjectName( "moveToHBox" );

  moveToRB = new QRadioButton( privateLayoutWidget );
  moveToRB->setObjectName( "moveToRB" );
  actionsGroup->insert( moveToRB );
  connect( moveToRB, SIGNAL( toggled( bool ) ),
           this, SLOT( slotUpdateControls() ) );
  moveToRB->setText( i18n( "Move to:" ) );
  moveToHBox->addWidget( moveToRB );

  folderSelector = new KMail::FolderRequester( privateLayoutWidget, tree );
  folderSelector->setMustBeReadWrite( true );
  moveToHBox->addWidget( folderSelector );
  actionsHBox->addLayout( moveToHBox );

  deletePermanentlyRB = new QRadioButton( privateLayoutWidget );
  deletePermanentlyRB->setObjectName( "deletePermanentlyRB" );
  actionsGroup->insert( deletePermanentlyRB );
  deletePermanentlyRB->setText( i18n( "Delete permanently" ) );
  actionsHBox->addWidget( deletePermanentlyRB );
  expiryActionHBox->addLayout( actionsHBox );
  globalVBox->addLayout( expiryActionHBox );

  note = new QLabel( privateLayoutWidget );
  note->setObjectName( "note" );
  note->setText( i18n( "Note: Expiry action will be applied immediately after confirming settings." ) );
  note->setAlignment( Qt::AlignVCenter );
  note->setWordWrap( true );

  globalVBox->addWidget( note );

  // Load the values from the folder
  bool expiryGloballyOn = mFolder->isAutoExpire();
  int daysToExpireRead, daysToExpireUnread;
  mFolder->daysToExpire( daysToExpireUnread, daysToExpireRead);

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
#ifdef __GNUC__
#warning Port me!
#endif
//  clearWState( WState_Polished );
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
}

void ExpiryPropertiesDialog::slotUpdateControls()
{
  bool showExpiryActions = expireReadMailCB->isChecked() || expireUnreadMailCB->isChecked();
  moveToRB->setEnabled( showExpiryActions );
  folderSelector->setEnabled( showExpiryActions && moveToRB->isChecked() );
  deletePermanentlyRB->setEnabled( showExpiryActions );
}


#include "expirypropertiesdialog.moc"
