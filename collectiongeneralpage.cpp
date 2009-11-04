/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2009 Montel Laurent <montel@kde.org>

  KMail is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  KMail is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "collectiongeneralpage.h"
#include <klineedit.h>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <KLocale>
#include <KComboBox>
#include <QCheckBox>
#include <KDialog>
#include <kpimidentities/identitycombo.h>
#include <akonadi/collection.h>
#include <akonadi/entitydisplayattribute.h>
#include <kmkernel.h>
#include "foldercollection.h"
using namespace Akonadi;

CollectionGeneralPage::CollectionGeneralPage(QWidget * parent) :
    CollectionPropertiesPage( parent ), mFolderCollection( 0 )
{
  setPageTitle(  i18nc("@title:tab General settings for a folder.", "General"));
  init();
}

CollectionGeneralPage::~CollectionGeneralPage()
{
  delete mFolderCollection;
}

static void addLine( QWidget *parent, QVBoxLayout* layout )
{
   QFrame *line = new QFrame( parent );
   line->setObjectName( "line" );
   line->setGeometry( QRect( 80, 150, 250, 20 ) );
   line->setFrameShape( QFrame::HLine );
   line->setFrameShadow( QFrame::Sunken );
   line->setFrameShape( QFrame::HLine );
   layout->addWidget( line );
}

void CollectionGeneralPage::init()
{
#if 0
  mIsLocalSystemFolder = mDlg->folder()->isSystemFolder();
  mIsResourceFolder = kmkernel->iCalIface().isStandardResourceFolder( mDlg->folder() );
#endif
  //TODO port it
  mIsLocalSystemFolder = false;
  mIsResourceFolder = true;
  QLabel *label;

  QVBoxLayout *topLayout = new QVBoxLayout( this );
  topLayout->setSpacing( KDialog::spacingHint() );
  topLayout->setMargin( 0 );

  // Musn't be able to edit details for a non-resource, system folder.
  if ( !mIsLocalSystemFolder || mIsResourceFolder ) {

    QHBoxLayout *hl = new QHBoxLayout();
    topLayout->addItem( hl );
    hl->setSpacing( KDialog::spacingHint() );

    label = new QLabel( i18nc("@label:textbox Name of the folder.","&Name:"), this );
    hl->addWidget( label );

    mNameEdit = new KLineEdit( this );
#if 0
    if( !mDlg->folder() )
      mNameEdit->setFocus();
    mNameEdit->setText( mDlg->folder() ? mDlg->folder()->label() : i18n("unnamed") );
    if (!aName.isEmpty())
            mNameEdit->setText(aName);
    mNameEdit->setMinimumSize(mNameEdit->sizeHint());
    // prevent renaming of IMAP inbox
    if ( mDlg->folder() && mDlg->folder()->isSystemFolder() ) {
      QString imapPath;
      if ( mDlg->folder()->folderType() == KMFolderTypeImap )
        imapPath = static_cast<KMFolderImap*>( mDlg->folder()->storage() )->imapPath();
      if ( mDlg->folder()->folderType() == KMFolderTypeCachedImap )
        imapPath = static_cast<KMFolderCachedImap*>( mDlg->folder()->storage() )->imapPath();
      if ( imapPath == "/INBOX/" )
        mNameEdit->setEnabled( false );
    }
    label->setBuddy( mNameEdit );
    hl->addWidget( mNameEdit );
#endif
    connect( mNameEdit, SIGNAL( textChanged( const QString & ) ),
                    this, SLOT( slotFolderNameChanged( const QString & ) ) );
  }


  // should new mail in this folder be ignored?
  QHBoxLayout *hbl = new QHBoxLayout();
  topLayout->addItem( hbl );
  hbl->setSpacing( KDialog::spacingHint() );
  mNotifyOnNewMailCheckBox =
    new QCheckBox( i18n("Act on new/unread mail in this folder" ), this );
  mNotifyOnNewMailCheckBox->setWhatsThis(
      i18n( "<qt><p>If this option is enabled then you will be notified about "
            "new/unread mail in this folder. Moreover, going to the "
            "next/previous folder with unread messages will stop at this "
            "folder.</p>"
            "<p>Uncheck this option if you do not want to be notified about "
            "new/unread mail in this folder and if you want this folder to "
            "be skipped when going to the next/previous folder with unread "
            "messages. This is useful for ignoring any new/unread mail in "
            "your trash and spam folder.</p></qt>" ) );
  hbl->addWidget( mNotifyOnNewMailCheckBox );
#if 0
  if ( mDlg->folder()->folderType() == KMFolderTypeImap ) {
    // should this folder be included in new-mail-checks?

    QHBoxLayout *nml = new QHBoxLayout();
    topLayout->addItem( nml );
    nml->setSpacing( KDialog::spacingHint() );
    mNewMailCheckBox = new QCheckBox( i18n("Include this folder in mail checks"), this );
    mNewMailCheckBox->setWhatsThis(
        i18n("<qt><p>If this option is enabled this folder will be included "
              "while checking new emails.</p>"
              "<p>Uncheck this option if you want to skip this folder "
              "while checking new emails.</p></qt>") );
    // default is on
    mNewMailCheckBox->setChecked(true);
    nml->addWidget( mNewMailCheckBox );
    nml->addStretch( 1 );
  }
#endif
  // should replies to mails in this folder be kept in this same folder?
  hbl = new QHBoxLayout();
  topLayout->addItem( hbl );
  hbl->setSpacing( KDialog::spacingHint() );
  mKeepRepliesInSameFolderCheckBox =
    new QCheckBox( i18n("Keep replies in this folder" ), this );
  mKeepRepliesInSameFolderCheckBox->setWhatsThis(
                   i18n( "Check this option if you want replies you write "
                         "to mails in this folder to be put in this same folder "
                         "after sending, instead of in the configured sent-mail folder." ) );
  hbl->addWidget( mKeepRepliesInSameFolderCheckBox );
  hbl->addStretch( 1 );

  // should this folder be shown in the folder selection dialog?
  hbl = new QHBoxLayout();
  topLayout->addItem( hbl );
  hbl->setSpacing( KDialog::spacingHint() );
  mHideInSelectionDialogCheckBox =
      new QCheckBox( i18n( "Hide this folder in the folder selection dialog" ), this );
  mHideInSelectionDialogCheckBox->setWhatsThis(
                  i18nc( "@info:whatsthis", "Check this option if you do not want this folder "
                         "to be shown in folder selection dialogs, such as the <interface>"
                          "Jump to Folder</interface> dialog." ) );
  hbl->addWidget( mHideInSelectionDialogCheckBox );
  hbl->addStretch( 1 );

  addLine( this, topLayout );
  // use grid layout for the following combobox settings
  QGridLayout *gl = new QGridLayout();
  topLayout->addItem( gl );
  gl->setSpacing( KDialog::spacingHint() );
  gl->setColumnStretch( 1, 100 ); // make the second column use all available space
  int row = -1;

  // sender identity
  ++row;
  mUseDefaultIdentityCheckBox = new QCheckBox( i18n("Use &default identity"),
                                               this );
  gl->addWidget( mUseDefaultIdentityCheckBox );
  connect( mUseDefaultIdentityCheckBox, SIGNAL( stateChanged(int) ),
           this, SLOT( slotIdentityCheckboxChanged() ) );
  ++row;
  label = new QLabel( i18n("&Sender identity:"), this );
  gl->addWidget( label, row, 0 );
  mIdentityComboBox = new KPIMIdentities::IdentityCombo( KMKernel::self()->identityManager(), this );
  label->setBuddy( mIdentityComboBox );
  gl->addWidget( mIdentityComboBox, row, 1 );
  mIdentityComboBox->setWhatsThis(
      i18n( "Select the sender identity to be used when writing new mail "
        "or replying to mail in this folder. This means that if you are in "
        "one of your work folders, you can make KMail use the corresponding "
        "sender email address, signature and signing or encryption keys "
        "automatically. Identities can be set up in the main configuration "
        "dialog. (Settings -> Configure KMail)") );
#if 0
  // folder contents
  if ( ( !mIsLocalSystemFolder || mIsResourceFolder ) &&
       mDlg->folder() && mDlg->folder()->folderType() != KMFolderTypeImap &&
       kmkernel->iCalIface().isEnabled() ) {
    // Only do make this settable, if the IMAP resource is enabled
    // and it's not the personal folders (those must not be changed)
    ++row;
    label = new QLabel( i18n("&Folder contents:"), this );
    gl->addWidget( label, row, 0 );
    mContentsComboBox = new KComboBox( this );
    label->setBuddy( mContentsComboBox );
    gl->addWidget( mContentsComboBox, row, 1 );

    mContentsComboBox->addItem( folderContentDesc( ContentsTypeMail ) );
    mContentsComboBox->addItem( folderContentDesc( ContentsTypeCalendar ) );
    mContentsComboBox->addItem( folderContentDesc( ContentsTypeContact ) );
    mContentsComboBox->addItem( folderContentDesc( ContentsTypeNote ) );
    mContentsComboBox->addItem( folderContentDesc( ContentsTypeTask ) );
    mContentsComboBox->addItem( folderContentDesc( ContentsTypeJournal ) );
    if ( mDlg->folder() )
      mContentsComboBox->setCurrentIndex( mDlg->folder()->storage()->contentsType() );
    connect ( mContentsComboBox, SIGNAL ( activated( int ) ),
              this, SLOT( slotFolderContentsSelectionChanged( int ) ) );
    if ( mDlg->folder()->isReadOnly() || mIsResourceFolder )
      mContentsComboBox->setEnabled( false );
  } else {
    mContentsComboBox = 0;
  }

  mIncidencesForComboBox = 0;
  mAlarmsBlockedCheckBox = 0;

  // Kolab incidences-for annotation.
  // Show incidences-for combobox if the contents type can be changed (new folder),
  // or if it's set to calendar or task (existing folder)
  if ( ( GlobalSettings::self()->theIMAPResourceStorageFormat() ==
         GlobalSettings::EnumTheIMAPResourceStorageFormat::XML ) &&
      mContentsComboBox ) {
    ++row;
    QLabel* label = new QLabel( i18n( "Generate free/&busy and activate alarms for:" ), this );
    gl->addWidget( label, row, 0 );
    mIncidencesForComboBox = new KComboBox( this );
    label->setBuddy( mIncidencesForComboBox );
    gl->addWidget( mIncidencesForComboBox, row, 1 );

    mIncidencesForComboBox->addItem( i18n( "Nobody" ) );
    mIncidencesForComboBox->addItem( i18n( "Admins of This Folder" ) );
    mIncidencesForComboBox->addItem( i18n( "All Readers of This Folder" ) );
    const QString whatsThisForMyOwnFolders =
      i18n( "This setting defines which users sharing "
          "this folder should get \"busy\" periods in their freebusy lists "
          "and should see the alarms for the events or tasks in this folder. "
          "The setting applies to Calendar and Task folders only "
          "(for tasks, this setting is only used for alarms).\n\n"
          "Example use cases: if the boss shares a folder with his secretary, "
          "only the boss should be marked as busy for his meetings, so he should "
          "select \"Admins\", since the secretary has no admin rights on the folder.\n"
          "On the other hand if a working group shares a Calendar for "
          "group meetings, all readers of the folders should be marked "
          "as busy for meetings.\n"
          "A company-wide folder with optional events in it would use \"Nobody\" "
          "since it is not known who will go to those events." );

    mIncidencesForComboBox->setWhatsThis( whatsThisForMyOwnFolders );

    ++row;
    const QString whatsThisForReadOnlyFolders =
      i18n( "This setting allows you to disable alarms for folders shared by "
          "others. ");
    mAlarmsBlockedCheckBox = new QCheckBox( this );
    mAlarmsBlockedCheckBox->setText( i18n( "Block free/&busy and alarms locally" ) );
    gl->addWidget( mAlarmsBlockedCheckBox, row, 0, 1, 1 );
    mAlarmsBlockedCheckBox->setWhatsThis( whatsThisForReadOnlyFolders );

    if ( mDlg->folder()->storage()->contentsType() != KMail::ContentsTypeCalendar
      && mDlg->folder()->storage()->contentsType() != KMail::ContentsTypeTask ) {
        mIncidencesForComboBox->setEnabled( false );
        mAlarmsBlockedCheckBox->setEnabled( false );
    }
  } else {
    mIncidencesForComboBox = 0;
  }

  if ( mDlg->folder()->folderType() == KMFolderTypeCachedImap ) {
    mSharedSeenFlagsCheckBox = new QCheckBox( this );
    mSharedSeenFlagsCheckBox->setText( i18n( "Share unread state with all users" ) );
    ++row;
    gl->addWidget( mSharedSeenFlagsCheckBox, row, 0, 1, 1 );
    mSharedSeenFlagsCheckBox->setWhatsThis( i18n( "If enabled, the unread state of messages in this folder will be the same "
        "for all users having access to this folder. If disabled (the default), every user with access to this folder has their "
        "own unread state." ) );
  }
#endif
  topLayout->addStretch( 100 ); // eat all superfluous space
}

#if 0
void FolderDialogGeneralTab::initializeWithValuesFromFolder( KMFolder* folder ) {

  if (folder->folderType() == KMFolderTypeImap)
  {
    KMFolderImap* imapFolder = static_cast<KMFolderImap*>(folder->storage());
    bool checked = imapFolder->includeInMailCheck();
    mNewMailCheckBox->setChecked(checked);
  }

  if ( mIncidencesForComboBox ) {
    KMFolderCachedImap* dimap = static_cast<KMFolderCachedImap *>( folder->storage() );
    mIncidencesForComboBox->setCurrentIndex( dimap->incidencesFor() );
    mIncidencesForComboBox->setDisabled( mDlg->folder()->isReadOnly() );
  }
  if ( mAlarmsBlockedCheckBox ) {
    KMFolderCachedImap* dimap = static_cast<KMFolderCachedImap *>( folder->storage() );
    mAlarmsBlockedCheckBox->setChecked( dimap->alarmsBlocked() );
  }
  if ( mSharedSeenFlagsCheckBox ) {
    KMFolderCachedImap *dimap = static_cast<KMFolderCachedImap*>( folder->storage() );
    ImapAccountBase *account = dynamic_cast<ImapAccountBase*>( dimap->account() );
    mSharedSeenFlagsCheckBox->setChecked( dimap->sharedSeenFlags() );
    mSharedSeenFlagsCheckBox->setDisabled( folder->isReadOnly() );
    if ( account && account->hasCapability( "x-kmail-sharedseen" ) )
      mSharedSeenFlagsCheckBox->show();
    else
      mSharedSeenFlagsCheckBox->hide();
  }
}

//-----------------------------------------------------------------------------
void FolderDialogGeneralTab::slotFolderContentsSelectionChanged( int )
{
  KMail::FolderContentsType type =
    static_cast<KMail::FolderContentsType>( mContentsComboBox->currentIndex() );
  if( type != KMail::ContentsTypeMail && GlobalSettings::self()->hideGroupwareFolders() ) {
    QString message = i18n("You have configured this folder to contain groupware information "
        "and the general configuration option to hide groupware folders is "
        "set. That means that this folder will disappear once the configuration "
        "dialog is closed. If you want to remove the folder again, you will need "
        "to temporarily disable hiding of groupware folders to be able to see it.");
    KMessageBox::information( this, message );
  }

  const bool enable = type == KMail::ContentsTypeCalendar || type == KMail::ContentsTypeTask;
  if ( mIncidencesForComboBox )
      mIncidencesForComboBox->setEnabled( enable );
  if ( mAlarmsBlockedCheckBox )
      mAlarmsBlockedCheckBox->setEnabled( enable );
}


//-----------------------------------------------------------------------------
bool FolderDialogGeneralTab::save()
{
  KMFolder* folder = mDlg->folder();
  QString fldName, oldFldName;
  KMFolderCachedImap* dimap = 0;
  if ( folder->folderType() == KMFolderTypeCachedImap )
    dimap = static_cast<KMFolderCachedImap *>( mDlg->folder()->storage() );

  if ( !mIsLocalSystemFolder || mIsResourceFolder )
  {
    oldFldName = mDlg->folder()->name();
    if (!mNameEdit->text().isEmpty())
      fldName = mNameEdit->text();
    else
      fldName = oldFldName;

    if ( mDlg->parentFolder() &&
         mDlg->parentFolder()->folderType() != KMFolderTypeImap &&
         mDlg->parentFolder()->folderType() != KMFolderTypeCachedImap )
      fldName.remove('/');
    fldName.remove(QRegExp("^\\.*"));
    if (fldName.isEmpty()) fldName = i18n("unnamed");

    // Set type field
    if ( mContentsComboBox ) {
      KMail::FolderContentsType type =
        static_cast<KMail::FolderContentsType>( mContentsComboBox->currentIndex() );
      folder->storage()->setContentsType( type );
    }

    if ( dimap ) {
      if ( mIncidencesForComboBox ) {
        KMFolderCachedImap::IncidencesFor incfor =
               static_cast<KMFolderCachedImap::IncidencesFor>( mIncidencesForComboBox->currentIndex() );
        if ( dimap->incidencesFor() != incfor ) {
          dimap->setIncidencesFor( incfor );
          dimap->writeConfig();
        }
      }
      if ( mAlarmsBlockedCheckBox && mAlarmsBlockedCheckBox->isChecked() != dimap->alarmsBlocked() ) {
        dimap->setAlarmsBlocked( mAlarmsBlockedCheckBox->isChecked() );
        dimap->writeConfig();
      }
    }

    if( folder->folderType() == KMFolderTypeImap )
    {
      KMFolderImap* imapFolder = static_cast<KMFolderImap*>( folder->storage() );
      imapFolder->setIncludeInMailCheck(
          mNewMailCheckBox->isChecked() );
    }
  }

  if ( dimap && mSharedSeenFlagsCheckBox &&
       mSharedSeenFlagsCheckBox->isChecked() != dimap->sharedSeenFlags() ) {
    dimap->setSharedSeenFlags( mSharedSeenFlagsCheckBox->isChecked() );
    dimap->writeConfig();
  }

  // make sure everything is on disk, connected slots will call readConfig()
  // when creating a new folder.
  folder->storage()->writeConfig();

  return true;
}
#endif

void CollectionGeneralPage::load(const Akonadi::Collection & col)
{
  QString displayName;
  if ( col.hasAttribute<Akonadi::EntityDisplayAttribute>() ) {
    displayName = col.attribute<Akonadi::EntityDisplayAttribute>()->displayName();
  }

  if ( displayName.isEmpty() )
    mNameEdit->setText( col.name() );
  else
    mNameEdit->setText( displayName );

  mFolderCollection = new FolderCollection( col );

  // folder identity
  mIdentityComboBox->setCurrentIdentity( mFolderCollection->identity() );
  mUseDefaultIdentityCheckBox->setChecked( mFolderCollection->useDefaultIdentity() );

  // ignore new mail
  mNotifyOnNewMailCheckBox->setChecked( !mFolderCollection->ignoreNewMail() );

  const bool keepInFolder = !mFolderCollection->isReadOnly() && mFolderCollection->putRepliesInSameFolder();
  mKeepRepliesInSameFolderCheckBox->setChecked( keepInFolder );
  mKeepRepliesInSameFolderCheckBox->setDisabled( mFolderCollection->isReadOnly() );
  mHideInSelectionDialogCheckBox->setChecked( mFolderCollection->hideInSelectionDialog() );
}

void CollectionGeneralPage::save(Collection & col)
{
  if ( col.hasAttribute<Akonadi::EntityDisplayAttribute>() &&
       !col.attribute<Akonadi::EntityDisplayAttribute>()->displayName().isEmpty() )
    col.attribute<Akonadi::EntityDisplayAttribute>()->setDisplayName( mNameEdit->text() );
  else
    col.setName( mNameEdit->text() );

  if ( mFolderCollection ) {
    mFolderCollection->setIdentity( mIdentityComboBox->currentIdentity() );
    mFolderCollection->setUseDefaultIdentity( mUseDefaultIdentityCheckBox->isChecked() );

    mFolderCollection->setIgnoreNewMail( !mNotifyOnNewMailCheckBox->isChecked() );
    mFolderCollection->setPutRepliesInSameFolder( mKeepRepliesInSameFolderCheckBox->isChecked() );
    mFolderCollection->setHideInSelectionDialog( mHideInSelectionDialogCheckBox->isChecked() );
  }
}

void CollectionGeneralPage::slotFolderNameChanged( const QString& str )
{
  //TODO .????
  //enableButtonOk( !str.isEmpty() );
}

void CollectionGeneralPage::slotIdentityCheckboxChanged()
{
  mIdentityComboBox->setEnabled( !mUseDefaultIdentityCheckBox->isChecked() );
}


#include "collectiongeneralpage.moc"
