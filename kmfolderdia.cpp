// -*- mode: C++; c-file-style: "gnu" -*-
// kmfolderdia.cpp
#include <config.h>

#include "kmacctfolder.h"
#include "kmfoldermgr.h"
#include "identitycombo.h"
#include "kmfolderimap.h"
#include "kmfolder.h"
#include "kmkernel.h"

#include <klineedit.h>
#include <klocale.h>
#include <knuminput.h>
#include <kmessagebox.h>
#include <kicondialog.h>
#include <kconfig.h>
#include <kdebug.h>

#include <qcheckbox.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qregexp.h>
#include <qlabel.h>

#include <assert.h>

#include "kmfolderdia.h" //has to be later because of KMFolder* fdcls
#include "kmfolderdia.moc"


//-----------------------------------------------------------------------------
KMFolderDialog::KMFolderDialog(KMFolder *aFolder, KMFolderDir *aFolderDir,
			       QWidget *aParent, const QString& aCap,
			       const QString& aName):
  KDialogBase( KDialogBase::Plain,
               aCap, KDialogBase::Ok|KDialogBase::Cancel,
               KDialogBase::Ok, aParent, "KMFolderDialog", TRUE ),
  mFolder( aFolder ),
  mFolderDir( aFolderDir )
{
  kdDebug(5006)<<"KMFolderDialog::KMFolderDialog()" << endl;

  QFrame *page = plainPage();

  QVBoxLayout *topLayout =  new QVBoxLayout( page, 0, spacingHint(),
                                             "topLayout" );

  QGroupBox *fpGroup = new QGroupBox( i18n("Folder Position"), page, "fpGroup" );
  fpGroup->setColumnLayout( 0,  Qt::Vertical );

  topLayout->addWidget( fpGroup );

  QHBoxLayout *hl = new QHBoxLayout( fpGroup->layout() );
  hl->setSpacing( 6 );

  QLabel *label = new QLabel( i18n("&Name:"), fpGroup );
  hl->addWidget( label );

  mNameEdit = new KLineEdit( fpGroup );
  if( !mFolder )
    mNameEdit->setFocus();
  mNameEdit->setText( mFolder ? mFolder->label() : i18n("unnamed") );
  if (!aName.isEmpty())
    mNameEdit->setText(aName);
  mNameEdit->setMinimumSize(mNameEdit->sizeHint());
  label->setBuddy( mNameEdit );
  hl->addWidget( mNameEdit );
  connect( mNameEdit, SIGNAL( textChanged( const QString & ) ),
           this, SLOT( slotFolderNameChanged( const QString & ) ) );

  QLabel* belongsToLabel = new QLabel( i18n("&Belongs to:" ), fpGroup );
  hl->addWidget( belongsToLabel );

  mBelongsToComboBox = new QComboBox(fpGroup);
  hl->addWidget( mBelongsToComboBox );
  belongsToLabel->setBuddy( mBelongsToComboBox );

  //start icons group
  QGroupBox *iconGroup = new QGroupBox( i18n("Folder Icons"), page, "iconGroup" );
  iconGroup->setColumnLayout( 0,  Qt::Vertical );

  topLayout->addWidget( iconGroup );

  QVBoxLayout *ivl = new QVBoxLayout( iconGroup->layout() );
  ivl->setSpacing( 6 );

  QHBoxLayout *ihl = new QHBoxLayout( ivl );
  mIconsCheckBox = new QCheckBox( i18n("Use custom &icons"), iconGroup );
  mIconsCheckBox->setChecked( false );
  ihl->addWidget( mIconsCheckBox );

  ihl = new QHBoxLayout( ivl );
  mNormalIconLabel = new QLabel( i18n("&Normal:"), iconGroup );
  mNormalIconLabel->setEnabled( false );
  ihl->addWidget( mNormalIconLabel );

  mNormalIconButton = new KIconButton( iconGroup );
  mNormalIconLabel->setBuddy( mNormalIconButton );
  mNormalIconButton->setIconType( KIcon::NoGroup , KIcon::Any, true );
  mNormalIconButton->setIconSize( 16 );
  mNormalIconButton->setStrictIconSize( true );
  mNormalIconButton->setFixedSize( 28, 28 );
  mNormalIconButton->setIcon( QString("folder") );
  mNormalIconButton->setEnabled( false );
  ihl->addWidget( mNormalIconButton );
  ihl->addStretch( 1 );

  mUnreadIconLabel = new QLabel( i18n("&Unread:"), iconGroup );
  mUnreadIconLabel->setEnabled( false );
  ihl->addWidget( mUnreadIconLabel );

  mUnreadIconButton = new KIconButton( iconGroup );
  mUnreadIconLabel->setBuddy( mUnreadIconButton );
  mUnreadIconButton->setIconType( KIcon::NoGroup, KIcon::Any, true );
  mUnreadIconButton->setIconSize( 16 );
  mUnreadIconButton->setStrictIconSize( true );
  mUnreadIconButton->setFixedSize( 28, 28 );
  mUnreadIconButton->setIcon( QString("folder_open") );
  mUnreadIconButton->setEnabled( false );
  ihl->addWidget( mUnreadIconButton );
  ihl->addStretch( 1 );

  connect( mIconsCheckBox, SIGNAL(toggled(bool)),
	   mNormalIconButton, SLOT(setEnabled(bool)) );
  connect( mIconsCheckBox, SIGNAL(toggled(bool)),
	   mUnreadIconButton, SLOT(setEnabled(bool)) );
  connect( mIconsCheckBox, SIGNAL(toggled(bool)),
	   mNormalIconLabel, SLOT(setEnabled(bool)) );
  connect( mIconsCheckBox, SIGNAL(toggled(bool)),
	   mUnreadIconLabel, SLOT(setEnabled(bool)) );

  connect( mNormalIconButton, SIGNAL(iconChanged(QString)),
	   this, SLOT(slotChangeIcon(QString)) );

  //end icons group

  mMailboxTypeGroupBox = new QGroupBox( i18n("Folder Type"), page, "mMailboxTypeGroupBox" );
  mMailboxTypeGroupBox->setColumnLayout( 0,  Qt::Vertical );

  topLayout->addWidget( mMailboxTypeGroupBox );

  QHBoxLayout *ml = new QHBoxLayout( mMailboxTypeGroupBox->layout() );
  ml->setSpacing( 6 );

  QLabel *label_type = new QLabel( i18n("&Mailbox format:" ), mMailboxTypeGroupBox );
  ml->addWidget( label_type );
  mMailboxTypeComboBox = new QComboBox(mMailboxTypeGroupBox);
  label_type->setBuddy( mMailboxTypeComboBox );
  mMailboxTypeComboBox->insertItem("mbox", 0);
  mMailboxTypeComboBox->insertItem("maildir", 1);
  mMailboxTypeComboBox->insertItem("search", 2);
  {
    KConfig *config = KMKernel::config();
    KConfigGroupSaver saver(config, "General");
    int type = config->readNumEntry("default-mailbox-format", 1);
    if ( type < 0 || type > 1 ) type = 1;
    mMailboxTypeComboBox->setCurrentItem( type );
  }
  mMailboxTypeComboBox->setEnabled( !mFolder );
  ml->addWidget( mMailboxTypeComboBox );
  ml->addStretch( 1 );

  QStringList str;
  if( !mFolder ) {
    // new folder can be subfolder of any other folder
    kmkernel->folderMgr()->createFolderList( &str, &mFolders  );
    kmkernel->imapFolderMgr()->createI18nFolderList( &str, &mFolders );
  }
  else if( mFolder->folderType() != KMFolderTypeImap
           && mFolder->folderType() != KMFolderTypeCachedImap ) {
    // already existant local folder can only be moved locally
    kmkernel->folderMgr()->createFolderList( &str, &mFolders  );
  }
  else {
    // already existant IMAP folder can't be moved, but we add all
    // IMAP folders so that the correct parent folder can be shown
    kmkernel->imapFolderMgr()->createI18nFolderList( &str, &mFolders );
  }

  // remove the local system folders from the list of parent folders because
  // they can't have child folders
  if( !mFolder || ( mFolder->folderType() != KMFolderTypeImap
                    && mFolder->folderType() != KMFolderTypeCachedImap ) ) {
    QGuardedPtr<KMFolder> curFolder;
    QValueListIterator<QGuardedPtr<KMFolder> > folderIt = mFolders.begin();
    QStringList::Iterator strIt = str.begin();
    while( folderIt != mFolders.end() ) {
      curFolder = *folderIt;
      kdDebug(5006) << "Looking at folder '" << curFolder->label() << "'"
                    << " and corresponding string '" << (*strIt) << "'"
                    << endl;
      if( curFolder->isSystemFolder()
          && curFolder->folderType() != KMFolderTypeImap
          && curFolder->folderType() != KMFolderTypeCachedImap ) {
        kdDebug(5006) << "Removing folder '" << curFolder->label() << "'"
                      << endl;
        folderIt = mFolders.remove( folderIt );
        kdDebug(5006) << "Removing string '" << (*strIt) << "'"
                      << endl;
        strIt = str.remove( strIt );
      }
      else {
        ++folderIt;
        ++strIt;
      }
    }
  }

  str.prepend( i18n( "Top Level" ) );

  mBelongsToComboBox->insertStringList( str );
  // we want to know if the activated changes
  connect( mBelongsToComboBox, SIGNAL(activated(int)), SLOT(slotUpdateItems(int)) );

  // Mailing-list data tab
  //
  QGroupBox *mlGroup = new QGroupBox(  i18n("Associated Mailing List" ), page );
  mlGroup->setColumnLayout( 0,  Qt::Vertical );
  QGridLayout *mlLayout = new QGridLayout(mlGroup->layout());
  mlLayout->setSpacing( 6 );
  mHoldsMailingListCheckBox = new QCheckBox( i18n("&Folder holds a mailing list"), mlGroup);
  topLayout->addWidget( mlGroup );
  mlLayout->addMultiCellWidget(mHoldsMailingListCheckBox, 0, 0, 0, 1);

  mlLayout->setColStretch(0, 1);
  mlLayout->setColStretch(1, 100);

  label = new QLabel( i18n("&Post address:"), mlGroup );
  label->setEnabled(false);
  QObject::connect( mHoldsMailingListCheckBox, SIGNAL(toggled(bool)),
		    label, SLOT(setEnabled(bool)) );
  mlLayout->addWidget( label, 1, 0 );
  mMailingListPostAddressEdit = new KLineEdit( mlGroup );
  label->setBuddy( mMailingListPostAddressEdit );
  mlLayout->addWidget( mMailingListPostAddressEdit, 1, 1 );
  mMailingListPostAddressEdit->setEnabled(false);

  connect( mHoldsMailingListCheckBox, SIGNAL(toggled(bool)),
	   mMailingListPostAddressEdit, SLOT(setEnabled(bool)) );

  //
  // Expiry data.
  //
  mExpireGroupBox = new QGroupBox(i18n("Old Message Expiry"), page);
  mExpireGroupBox->setColumnLayout(0, Qt::Vertical);
  QGridLayout *expLayout = new QGridLayout(mExpireGroupBox->layout());
  expLayout->setSpacing(6);

  // Checkbox for setting whether expiry is enabled on this folder.
  mExpireFolderCheckBox = new QCheckBox(i18n("E&xpire old messages in this folder"), mExpireGroupBox);
  QObject::connect(mExpireFolderCheckBox, SIGNAL(toggled(bool)), SLOT(slotExpireFolder(bool)));
  topLayout->addWidget(mExpireGroupBox);
  expLayout->addMultiCellWidget(mExpireFolderCheckBox, 0, 0, 0, 1);

  // Expiry time for read documents.
  label = new QLabel(i18n("Expire &read email after:"), mExpireGroupBox);
  label->setEnabled(false);
  QObject::connect( mExpireFolderCheckBox, SIGNAL(toggled(bool)),
		    label, SLOT(setEnabled(bool)) );
  expLayout->addWidget(label, 1, 0);
  mReadExpiryTimeNumInput = new KIntNumInput(mExpireGroupBox);
  mReadExpiryTimeNumInput->setRange(1, 500, 1, false);
  mReadExpiryTimeNumInput->setEnabled(false);
  mReadExpiryTimeNumInput->setValue(7);
  label->setBuddy(mReadExpiryTimeNumInput);
  expLayout->addWidget(mReadExpiryTimeNumInput, 1, 1);

  mReadExpiryUnitsComboBox = new QComboBox(mExpireGroupBox);
  mReadExpiryUnitsComboBox->insertItem(i18n("Never"));
  mReadExpiryUnitsComboBox->insertItem(i18n("Day(s)"));
  mReadExpiryUnitsComboBox->insertItem(i18n("Week(s)"));
  mReadExpiryUnitsComboBox->insertItem(i18n("Month(s)"));
  mReadExpiryUnitsComboBox->setEnabled(false);
  expLayout->addWidget(mReadExpiryUnitsComboBox, 1, 2);
  connect( mReadExpiryUnitsComboBox, SIGNAL( activated( int ) ),
           this, SLOT( slotReadExpiryUnitChanged( int ) ) );

  // Expiry time for unread documents.
  label = new QLabel(i18n("Expire unr&ead email after:"), mExpireGroupBox);
  label->setEnabled(false);
  QObject::connect( mExpireFolderCheckBox, SIGNAL(toggled(bool)),
		    label, SLOT(setEnabled(bool)) );
  expLayout->addWidget(label, 2, 0);
  mUnreadExpiryTimeNumInput = new KIntNumInput(mExpireGroupBox);
  mUnreadExpiryTimeNumInput->setRange(1, 500, 1, false);
  mUnreadExpiryTimeNumInput->setEnabled(false);
  mUnreadExpiryTimeNumInput->setValue(28);
  label->setBuddy(mUnreadExpiryTimeNumInput);
  expLayout->addWidget(mUnreadExpiryTimeNumInput, 2, 1);

  mUnreadExpiryUnitsComboBox = new QComboBox(mExpireGroupBox);
  mUnreadExpiryUnitsComboBox->insertItem(i18n("Never"));
  mUnreadExpiryUnitsComboBox->insertItem(i18n("Day(s)"));
  mUnreadExpiryUnitsComboBox->insertItem(i18n("Week(s)"));
  mUnreadExpiryUnitsComboBox->insertItem(i18n("Month(s)"));
  mUnreadExpiryUnitsComboBox->setEnabled(false);
  expLayout->addWidget(mUnreadExpiryUnitsComboBox, 2, 2);
  connect( mUnreadExpiryUnitsComboBox, SIGNAL( activated( int ) ),
           this, SLOT( slotUnreadExpiryUnitChanged( int ) ) );

  expLayout->setColStretch(0, 3);
  expLayout->setColStretch(0, 100);


  QGroupBox *idGroup = new QGroupBox(  i18n("Identity" ), page );
  idGroup->setColumnLayout( 0, Qt::Vertical );
  QHBoxLayout *idLayout = new QHBoxLayout(idGroup->layout());
  idLayout->setSpacing( 6 );
  topLayout->addWidget( idGroup );

  label = new QLabel( i18n("&Sender:"), idGroup );
  idLayout->addWidget( label );
  mIdentityComboBox = new IdentityCombo( idGroup );
  label->setBuddy( mIdentityComboBox );
  idLayout->addWidget( mIdentityComboBox, 3 );

  QGroupBox* senderGroup = new QGroupBox( i18n("Show Sender/Receiver"), page, "senderGroup" );
  senderGroup->setColumnLayout( 0,  Qt::Vertical );

  topLayout->addWidget( senderGroup );

  QHBoxLayout *sl = new QHBoxLayout( senderGroup->layout() );
  sl->setSpacing( 6 );

  QLabel *sender_label = new QLabel( i18n("Sho&w:" ), senderGroup );
  sl->addWidget( sender_label );
  mShowSenderReceiverComboBox = new QComboBox(senderGroup);
  sender_label->setBuddy(mShowSenderReceiverComboBox);
  mShowSenderReceiverComboBox->insertItem(i18n("Default"), 0);
  mShowSenderReceiverComboBox->insertItem(i18n("Sender"), 1);
  mShowSenderReceiverComboBox->insertItem(i18n("Receiver"), 2);

  QString whoField;
  if (mFolder) whoField = mFolder->userWhoField();
  if (whoField.isEmpty()) mShowSenderReceiverComboBox->setCurrentItem(0);
  if (whoField == "From") mShowSenderReceiverComboBox->setCurrentItem(1);
  if (whoField == "To") mShowSenderReceiverComboBox->setCurrentItem(2);

  sl->addWidget( mShowSenderReceiverComboBox );
  sl->addStretch( 1 );


  if ( ((!mFolder) && mFolderDir->type() == KMImapDir) ||
       (mFolder && (mFolder->folderType() == KMFolderTypeImap)) )
  {
    KMFolderImap* imapFolder = 0;
    if (mFolder) imapFolder = static_cast<KMFolderImap*>((KMFolder*)mFolder);
    bool checked = (imapFolder) ? imapFolder->includeInMailCheck() : true;
    // should this folder be included in new-mail-checks?
    QGroupBox* newmailGroup = new QGroupBox( i18n("Check for New Mail"), page, "newmailGroup" );
    newmailGroup->setColumnLayout( 0,  Qt::Vertical );
    topLayout->addWidget( newmailGroup );

    QHBoxLayout *nml = new QHBoxLayout( newmailGroup->layout() );
    nml->setSpacing( 6 );
    QLabel *newmailLabel = new QLabel( i18n("Include in check:" ), newmailGroup );
    nml->addWidget( newmailLabel );
    mNewMailCheckBox = new QCheckBox(newmailGroup);
    mNewMailCheckBox->setChecked(checked);
    newmailLabel->setBuddy(mNewMailCheckBox);
    nml->addWidget( mNewMailCheckBox );
    nml->addStretch( 1 );
  }

  KMFolder* parentFolder = 0;

  if( aFolderDir ) {
    // search the parent folder of the folder
    kdDebug(5006) << "search the parent folder of the folder" << endl;
    QValueListConstIterator<QGuardedPtr<KMFolder> > it;
    int i = 1;
    for( it = mFolders.begin(); it != mFolders.end(); ++it, ++i ) {
      kdDebug(5006) << "checking folder '" << (*it)->label() << "'" << endl;
      if( (*it)->child() == aFolderDir ) {
        parentFolder = *it;
        mBelongsToComboBox->setCurrentItem( i );
        slotUpdateItems( i );
        break;
      }
    }
  }

  if( mFolder ) {
    initializeWithValuesFromFolder( mFolder );

    // mailbox folder type
    switch ( mFolder->folderType() ) {
      case KMFolderTypeSearch:
        mMailboxTypeComboBox->setCurrentItem( 2 );
        belongsToLabel->hide();
        mBelongsToComboBox->hide();
        break;
      case KMFolderTypeMaildir:
        mMailboxTypeComboBox->setCurrentItem( 1 );
        break;
      case KMFolderTypeMbox:
        mMailboxTypeComboBox->setCurrentItem( 0 );
        break;
      case KMFolderTypeImap:
      case KMFolderTypeCachedImap:
        belongsToLabel->setEnabled( false );
        mBelongsToComboBox->setEnabled( false );
        break;
      default: ;
    }
  }
  else if ( parentFolder ) {
    initializeWithValuesFromFolder( parentFolder );
  }

  // Musn't be able to edit details for a system folder.
  // Make sure we don't bomb out if there isn't a folder
  // object yet (i.e. just about to create new folder).

  if ( mFolder ) {
    if ( mFolder->folderType() == KMFolderTypeImap
         || mFolder->folderType() == KMFolderTypeCachedImap ) {
      mExpireGroupBox->hide();
      mMailboxTypeGroupBox->hide();
    }
    else if ( mFolder->isSystemFolder() ) {
      fpGroup->hide();
      iconGroup->hide();
      mMailboxTypeGroupBox->hide();
      mlGroup->hide();
      idGroup->hide();
    }
  }

  kdDebug(5006)<<"Exiting KMFolderDialog::KMFolderDialog()\n";
}


//-----------------------------------------------------------------------------
void KMFolderDialog::initializeWithValuesFromFolder( KMFolder* folder ) {
  if ( !folder )
    return;

  // folder icons
  mIconsCheckBox->setChecked( folder->useCustomIcons() );
  mNormalIconLabel->setEnabled( folder->useCustomIcons() );
  mNormalIconButton->setEnabled( folder->useCustomIcons() );
  mUnreadIconLabel->setEnabled( folder->useCustomIcons() );
  mUnreadIconButton->setEnabled( folder->useCustomIcons() );
  QString iconPath = folder->normalIconPath();
  if ( !iconPath.isEmpty() )
    mNormalIconButton->setIcon( iconPath );
  iconPath = folder->unreadIconPath();
  if ( !iconPath.isEmpty() )
    mUnreadIconButton->setIcon( iconPath );


  // mailing-list related settings
  mMailingListPostAddressEdit->setText( folder->mailingListPostAddress() );
  mMailingListPostAddressEdit->setEnabled( folder->isMailingList() );
  mHoldsMailingListCheckBox->setChecked( folder->isMailingList() );

  // folder identity
  mIdentityComboBox->setCurrentIdentity( folder->identity() );

  // settings for automatic deletion of old messages
  mExpireFolderCheckBox->setChecked( folder->isAutoExpire() );
  // Legal values for units are 0=never, 1=days, 2=weeks, 3=months.
  // Should really do something better than hardcoding this everywhere.
  if( folder->getReadExpireUnits() >= 0
      && folder->getReadExpireUnits() < expireMaxUnits) {
    mReadExpiryUnitsComboBox->setCurrentItem( folder->getReadExpireUnits() );
  }
  if( folder->getUnreadExpireUnits() >= 0
      && folder->getUnreadExpireUnits() < expireMaxUnits ) {
    mUnreadExpiryUnitsComboBox->setCurrentItem( folder->getUnreadExpireUnits() );
  }
  int age = folder->getReadExpireAge();
  if ( age >= 1 && age <= 500 ) {
    mReadExpiryTimeNumInput->setValue( age );
  } else {
    mReadExpiryTimeNumInput->setValue( 7 );
  }
  age = folder->getUnreadExpireAge();
  if ( age >= 1 && age <= 500 ) {
    mUnreadExpiryTimeNumInput->setValue( age );
  } else {
    mUnreadExpiryTimeNumInput->setValue( 28 );
  }
  if( !folder->isAutoExpire() ) {
    mReadExpiryTimeNumInput->setEnabled( false );
    mReadExpiryUnitsComboBox->setEnabled( false );
    mUnreadExpiryTimeNumInput->setEnabled( false );
    mUnreadExpiryUnitsComboBox->setEnabled( false );
  }
  else {
    // disable the number fields if "Never" is selected
    mReadExpiryTimeNumInput->setEnabled( mReadExpiryUnitsComboBox->currentItem() != 0 );
    mUnreadExpiryTimeNumInput->setEnabled( mUnreadExpiryUnitsComboBox->currentItem() != 0 );
  }
}

//-----------------------------------------------------------------------------
void KMFolderDialog::slotFolderNameChanged( const QString& str )
{
  enableButtonOK( !str.isEmpty() );
}

//-----------------------------------------------------------------------------
void KMFolderDialog::slotUpdateItems ( int current )
{
  KMFolder* selectedFolder = 0;
  // check if the index is valid (the top level has no entrance in the mFolders)
  if (current > 0) selectedFolder = *mFolders.at(current - 1);
  if (selectedFolder && (selectedFolder->folderType() == KMFolderTypeImap ||
			 selectedFolder->folderType() == KMFolderTypeCachedImap))
  {
    // deactivate stuff that is not available for imap
    mMailboxTypeGroupBox->setEnabled( false );
    mExpireGroupBox->setEnabled( false );
  } else {
    // activate it
    mMailboxTypeGroupBox->setEnabled( true );
    mExpireGroupBox->setEnabled( true );
  }
}

//-----------------------------------------------------------------------------
void KMFolderDialog::slotOk()
{
  bool bIsNewFolder = ( !mFolder );
  // moving of IMAP folders is not yet supported
  if ( bIsNewFolder || !mFolder->isSystemFolder() )
  {
    QString acctName;
    QString fldName, oldFldName;
    KMFolderDir *selectedFolderDir = &(kmkernel->folderMgr()->dir());
    KMFolder *selectedFolder = 0;
    int curFolder = mBelongsToComboBox->currentItem();

    if( !bIsNewFolder ) oldFldName = mFolder->name();
    if (!mNameEdit->text().isEmpty()) fldName = mNameEdit->text();
    else fldName = oldFldName;
    fldName.replace("/", "");
    fldName.replace(QRegExp("^\\.*"), "");
    if (fldName.isEmpty()) fldName = i18n("unnamed");

    if (mMailboxTypeComboBox->currentItem() == 2) {
      selectedFolderDir = &(kmkernel->searchFolderMgr()->dir());
    }
    else if (curFolder != 0)
    {
      selectedFolder = *mFolders.at(curFolder - 1);
      selectedFolderDir = selectedFolder->createChildFolder();
    }

    QString message = i18n( "<qt>Failed to create folder <b>%1</b>, folder already exists.</qt>" ).arg(fldName);
    if( selectedFolderDir->hasNamedFolder( fldName )
        && ( !( mFolder
                && ( selectedFolderDir == mFolder->parent() )
                && ( mFolder->name() == fldName ) ) ) )
    {
      KMessageBox::error( this, message );
      return;
    }

    message = i18n( "<qt>Cannot move folder <b>%1</b> into a subfolder below itself.</qt>" ).arg(fldName);
    KMFolderDir* folderDir = selectedFolderDir;


    // Buggy?
    if( mFolder && mFolder->child() )
      while( ( folderDir != &kmkernel->folderMgr()->dir() )
             && ( folderDir != mFolder->parent() ) ) {
        if( folderDir->findRef( mFolder ) != -1 ) {
          KMessageBox::error( this, message );
          return;
        }
        folderDir = folderDir->parent();
      }
    // End buggy?


    if( mFolder && mFolder->child() && selectedFolderDir &&
        ( selectedFolderDir->path().find( mFolder->child()->path() + "/" ) == 0 ) ) {
      KMessageBox::error( this, message );
      return;
    }

    if( mFolder && mFolder->child()
        && ( selectedFolderDir == mFolder->child() ) ) {
      KMessageBox::error( this, message );
      return;
    }

    if( bIsNewFolder ) {
      if (selectedFolder && selectedFolder->folderType() == KMFolderTypeImap)
      {
        mFolder = kmkernel->imapFolderMgr()->createFolder( fldName, FALSE, KMFolderTypeImap, selectedFolderDir );
        static_cast<KMFolderImap*>(selectedFolder)->createFolder(fldName);
      } else if (selectedFolder && selectedFolder->folderType() == KMFolderTypeCachedImap){
        mFolder = kmkernel->imapFolderMgr()->createFolder( fldName, FALSE, KMFolderTypeCachedImap, selectedFolderDir );
      } else if (mMailboxTypeComboBox->currentItem() == 2) {
        mFolder = kmkernel->searchFolderMgr()->createFolder(fldName, FALSE, KMFolderTypeSearch, &kmkernel->searchFolderMgr()->dir() );
      } else if (mMailboxTypeComboBox->currentItem() == 1) {
        mFolder = kmkernel->folderMgr()->createFolder(fldName, FALSE, KMFolderTypeMaildir, selectedFolderDir );
      } else {
        mFolder = kmkernel->folderMgr()->createFolder(fldName, FALSE, KMFolderTypeMbox, selectedFolderDir );
      }
    }
    else if( ( oldFldName != fldName )
             || ( mFolder->parent() != selectedFolderDir ) )
    {
      if( mFolder->parent() != selectedFolderDir ) {
        if( mFolder->folderType() == KMFolderTypeCachedImap ) {
          QString message = i18n("Moving IMAP folders is not supported");
          KMessageBox::error( this, message );
        } else
          mFolder->rename(fldName, selectedFolderDir );
      } else
        mFolder->rename(fldName);

      kmkernel->folderMgr()->contentsChanged();
    }
  }

  if( mFolder )
  {
		// settings for mailingList
    mFolder->setMailingList( mHoldsMailingListCheckBox->isChecked() );
    mFolder->setMailingListPostAddress( mMailingListPostAddressEdit->text() );
    mFolder->setMailingListAdminAddress( QString::null );

    mFolder->setIdentity( mIdentityComboBox->currentIdentity() );

    // Settings for auto expiry of old email messages.
    mFolder->setAutoExpire(mExpireFolderCheckBox->isChecked());
    mFolder->setUnreadExpireAge(mUnreadExpiryTimeNumInput->value());
    mFolder->setReadExpireAge(mReadExpiryTimeNumInput->value());
    mFolder->setUnreadExpireUnits((ExpireUnits)mUnreadExpiryUnitsComboBox->currentItem());
    mFolder->setReadExpireUnits((ExpireUnits)mReadExpiryUnitsComboBox->currentItem());
    // Update the tree iff new icon paths are different and not empty or if
    // useCustomIcons changed.
    if ( mFolder->useCustomIcons() != mIconsCheckBox->isChecked() ) {
      mFolder->setUseCustomIcons( mIconsCheckBox->isChecked() );
      // Reset icons, useCustomIcons was turned off.
      if ( !mFolder->useCustomIcons() ) {
        mFolder->setIconPaths( "", "" );
      }
    }
    if ( mFolder->useCustomIcons() &&
      (( mNormalIconButton->icon() != mFolder->normalIconPath() ) &&
      ( !mNormalIconButton->icon().isEmpty())) ||
      (( mUnreadIconButton->icon() != mFolder->unreadIconPath() ) &&
      ( !mUnreadIconButton->icon().isEmpty())) ) {
      mFolder->setIconPaths( mNormalIconButton->icon(), mUnreadIconButton->icon() );
    }
        // set whoField
    if (mShowSenderReceiverComboBox->currentItem() == 1)
      mFolder->setUserWhoField("From");
    else if (mShowSenderReceiverComboBox->currentItem() == 2)
      mFolder->setUserWhoField("To");
    else
      mFolder->setUserWhoField(QString::null);

    if( bIsNewFolder )
      mFolder->close();

    if( mFolder->folderType() == KMFolderTypeImap )
    {
      KMFolderImap* imapFolder = static_cast<KMFolderImap*>( (KMFolder*) mFolder );
      imapFolder->setIncludeInMailCheck(
          mNewMailCheckBox->isChecked() );
    }
  }

  KDialogBase::slotOk();
}


/**
 * Called when the 'auto expire' toggle is clicked.
 * Enables/disables all widgets related to this.
 */
void KMFolderDialog::slotExpireFolder(bool expire)
{
  if (expire) {
    // disable the number field if "Never" is selected
    mReadExpiryTimeNumInput->setEnabled( mReadExpiryUnitsComboBox->currentItem() != 0 );
    mReadExpiryUnitsComboBox->setEnabled(true);
    // disable the number field if "Never" is selected
    mUnreadExpiryTimeNumInput->setEnabled( mUnreadExpiryUnitsComboBox->currentItem() != 0 );
    mUnreadExpiryUnitsComboBox->setEnabled(true);
  } else {
    mReadExpiryTimeNumInput->setEnabled(false);
    mReadExpiryUnitsComboBox->setEnabled(false);
    mUnreadExpiryTimeNumInput->setEnabled(false);
    mUnreadExpiryUnitsComboBox->setEnabled(false);
  }
}


/**
 * Enable/disable the number field if appropriate
 */
void
KMFolderDialog::slotReadExpiryUnitChanged( int value )
{
  // disable the number field if "Never" is selected
  mReadExpiryTimeNumInput->setEnabled( value != 0 );
}


/**
 * Enable/disable the number field if appropriate
 */
void
KMFolderDialog::slotUnreadExpiryUnitChanged( int value )
{
  // disable the number field if "Never" is selected
  mUnreadExpiryTimeNumInput->setEnabled( value != 0 );
}


void
KMFolderDialog::slotChangeIcon( QString icon ) // can't use a const-ref here, due to KIconButton's signal
{
    mUnreadIconButton->setIcon( icon );
}
