
#include "expirypropertiesdialog.h"
#include "folderrequester.h"
#include "kmfolder.h"
#include "mainfolderview.h"

#include <QVariant>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QRadioButton>
#include <QLayout>
#include <QGroupBox>
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
ExpiryPropertiesDialog::ExpiryPropertiesDialog( MainFolderView* tree, KMFolder* folder )
    : KDialog( tree ),
      mFolder( folder )
{
  setCaption( i18n( "Mail Expiry Properties" ) );
  setButtons( Ok|Cancel );
  setModal( false );
  setObjectName( "expiry_properties" );
  setAttribute( Qt::WA_DeleteOnClose );

  QWidget* privateLayoutWidget = new QWidget;
  privateLayoutWidget->setObjectName( "privateLayoutWidget" );
  setMainWidget( privateLayoutWidget );

  QVBoxLayout *globalVBox = new QVBoxLayout;
  globalVBox->setMargin( marginHint() );
  globalVBox->setObjectName( "globalVBox" );
  globalVBox->setSpacing( spacingHint() );
  privateLayoutWidget->setLayout( globalVBox );

  QGridLayout *daysBox = new QGridLayout;

  expireReadMailCB = new QCheckBox;
  expireReadMailCB->setObjectName( "expireReadMailCB" );
  expireReadMailCB->setText( i18n( "Expire read messages after" ) );
  connect( expireReadMailCB, SIGNAL( toggled( bool )),
           this, SLOT( slotUpdateControls() ) );
  daysBox->addWidget( expireReadMailCB, 0, 0, Qt::AlignLeft );

  expireReadMailSB = new QSpinBox;
  expireReadMailSB->setObjectName( "expireReadMailSB" );
  expireReadMailSB->setMaximum( 999999 );
  expireReadMailSB->setValue( 30 );
  daysBox->addWidget( expireReadMailSB, 0, 1 );

  expireUnreadMailCB = new QCheckBox;
  expireUnreadMailCB->setObjectName( "expireUnreadMailCB" );
  expireUnreadMailCB->setText( i18n( "Expire unread messages after" ) );
  connect( expireUnreadMailCB, SIGNAL( toggled( bool )),
           this, SLOT( slotUpdateControls() ) );
  daysBox->addWidget( expireUnreadMailCB, 1, 0, Qt::AlignLeft );

  expireUnreadMailSB = new QSpinBox;
  expireUnreadMailSB->setObjectName( "expireUnreadMailSB" );
  expireUnreadMailSB->setMaximum( 99999 );
  expireUnreadMailSB->setValue( 30 );
  updateSpinBoxSuffix();
  connect ( expireReadMailSB, SIGNAL( valueChanged( int )),
           this, SLOT( updateSpinBoxSuffix() ) );
  connect ( expireUnreadMailSB, SIGNAL( valueChanged( int )),
           this, SLOT( updateSpinBoxSuffix() ) );
  daysBox->addWidget( expireUnreadMailSB, 1, 1 );

  daysBox->setColumnStretch( 3, 1 );
  globalVBox->addLayout( daysBox );

  globalVBox->addSpacing( 30 );

  QGroupBox *actionsGroup = new QGroupBox;
  actionsGroup->hide(); // for mutual exclusion of the radio buttons

  QHBoxLayout *moveToHBox = new QHBoxLayout();
  moveToHBox->setMargin( 0 );
  moveToHBox->setSpacing( 6 );

  moveToRB = new QRadioButton( actionsGroup );
  moveToRB->setObjectName( "moveToRB" );
  moveToRB->setText( i18n( "Move expired messages to:" ) );
  connect( moveToRB, SIGNAL( toggled( bool ) ), this, SLOT( slotUpdateControls() ) );
  moveToHBox->addWidget( moveToRB );

  folderSelector = new KMail::FolderRequester( privateLayoutWidget );
  folderSelector->setFolderTree( tree );
  folderSelector->setMustBeReadWrite( true );
  moveToHBox->addWidget( folderSelector );
  globalVBox->addLayout( moveToHBox );

  deletePermanentlyRB = new QRadioButton( actionsGroup );
  deletePermanentlyRB->setObjectName( "deletePermanentlyRB" );
  deletePermanentlyRB->setText( i18n( "Delete expired messages permanently" ) );
  globalVBox->addWidget( deletePermanentlyRB );

  globalVBox->addSpacing( 30 );

  QLabel *note = new QLabel;
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
  setAttribute(Qt::WA_WState_Polished);
}

/*
 *  Destroys the object and frees any allocated resources
 */
ExpiryPropertiesDialog::~ExpiryPropertiesDialog()
{
    // no need to delete child widgets, Qt does it all for us
}

void ExpiryPropertiesDialog::accept()
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

  KDialog::accept();
}


void ExpiryPropertiesDialog::slotUpdateControls()
{
  bool showExpiryActions = expireReadMailCB->isChecked() || expireUnreadMailCB->isChecked();
  moveToRB->setEnabled( showExpiryActions );
  folderSelector->setEnabled( showExpiryActions && moveToRB->isChecked() );
  deletePermanentlyRB->setEnabled( showExpiryActions );

  expireReadMailSB->setEnabled( expireReadMailCB->isChecked() );
  expireUnreadMailSB->setEnabled( expireUnreadMailCB->isChecked() );
}

void ExpiryPropertiesDialog::updateSpinBoxSuffix()
{
  expireReadMailSB->setSuffix( QString(" ") + i18np("day", "days", expireReadMailSB->value() ) );
  expireUnreadMailSB->setSuffix( QString(" ") + i18np("day", "days", expireUnreadMailSB->value() ) );
}


#include "expirypropertiesdialog.moc"
