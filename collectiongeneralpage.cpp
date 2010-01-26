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

#include <akonadi/attributefactory.h>
#include <akonadi/agentmanager.h>
#include <akonadi/collection.h>
#include <akonadi/entitydisplayattribute.h>

#include "collectionannotationsattribute.h"

// TODO Where should these be?
#define KOLAB_FOLDERTYPE "/vendor/kolab/folder-type"
#define KOLAB_INCIDENCESFOR "/vendor/kolab/incidences-for"
#define KOLAB_SHAREDSEEN "/vendor/cmu/cyrus-imapd/sharedseen"

#include <kmkernel.h>
#include "foldercollection.h"
#include "util.h"

using namespace Akonadi;

CollectionGeneralPage::CollectionGeneralPage(QWidget * parent) :
    CollectionPropertiesPage( parent ), mFolderCollection( 0 )
{
  setPageTitle(  i18nc("@title:tab General settings for a folder.", "General"));

}

CollectionGeneralPage::~CollectionGeneralPage()
{
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

static QString incidencesForToString( CollectionGeneralPage::IncidencesFor r )
{
  kDebug() << r;
  switch ( r ) {
  case CollectionGeneralPage::IncForNobody:
    return "nobody";
  case CollectionGeneralPage::IncForAdmins:
    return "admins";
  case CollectionGeneralPage::IncForReaders:
    return "readers";
  }
  return QString(); // can't happen
}

static CollectionGeneralPage::IncidencesFor incidencesForFromString( const QString &str )
{
  if ( str == "nobody" ) {
    return CollectionGeneralPage::IncForNobody;
  }
  if ( str == "admins" ) {
    return CollectionGeneralPage::IncForAdmins;
  }
  if ( str == "readers" ) {
    return CollectionGeneralPage::IncForReaders;
  }
  return CollectionGeneralPage::IncForAdmins; // by default
}


//----------------------------------------------------------------------------
// Used by the "General" and "Maintenance" tabs
static QString folderContentDesc( CollectionGeneralPage::FolderContentsType type )
{
  switch ( type )
  {
  case CollectionGeneralPage::ContentsTypeMail:     return ( i18nc( "type of folder content", "Mail" ) );
  case CollectionGeneralPage::ContentsTypeCalendar: return ( i18nc( "type of folder content", "Calendar" ) );
  case CollectionGeneralPage::ContentsTypeContact:  return ( i18nc( "type of folder content", "Contacts" ) );
  case CollectionGeneralPage::ContentsTypeNote:     return ( i18nc( "type of folder content", "Notes" ) );
  case CollectionGeneralPage::ContentsTypeTask:     return ( i18nc( "type of folder content", "Tasks" ) );
  case CollectionGeneralPage::ContentsTypeJournal:  return ( i18nc( "type of folder content", "Journal" ) );
  default:                   return ( i18nc( "type of folder content", "Unknown" ) );
  }
}

void CollectionGeneralPage::init(const Akonadi::Collection &col)
{
  mIsLocalSystemFolder = KMKernel::self()->isSystemFolderCollection( col );
  mIsImapFolder = KMKernel::self()->isImapFolder( col );

#if 0
  mIsResourceFolder = kmkernel->iCalIface().isStandardResourceFolder( mDlg->folder() );
#endif
  //TODO port it
  mIsResourceFolder = false;
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
#endif
    label->setBuddy( mNameEdit );
    hl->addWidget( mNameEdit );
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
  if ( KMKernel::self()->isImapFolder( col ) ) {
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

  CollectionGeneralPage::FolderContentsType contentsType = CollectionGeneralPage::ContentsTypeMail;

  Collection colCopy = col;

  CollectionAnnotationsAttribute *attr = colCopy.attribute<CollectionAnnotationsAttribute>( Entity::AddIfMissing );
  QMap<QByteArray, QByteArray> annotations = attr->annotations();

  bool sharedSeen = annotations.value(KOLAB_SHAREDSEEN) == "true";

  IncidencesFor incidencesFor = incidencesForFromString( annotations.value(KOLAB_INCIDENCESFOR) );

#if 0 // Should not be needed in akonadi powered kmail.
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

    mContentsComboBox->setCurrentIndex( contentsType );

    connect ( mContentsComboBox, SIGNAL ( activated( int ) ),
        this, SLOT( slotFolderContentsSelectionChanged( int ) ) );
    if ( mFolderCollection->isReadOnly() || mIsResourceFolder )
      mContentsComboBox->setEnabled( false );

  } else {
#endif
    mContentsComboBox = 0;
#if 0
  }
#endif
  mIncidencesForComboBox = 0;
  mAlarmsBlockedCheckBox = 0;

  // Kolab incidences-for annotation.
  // Show incidences-for combobox if the contents type can be changed (new folder),
  // or if it's set to calendar or task (existing folder)
  if ( col.contentMimeTypes().contains( "application/x-vnd.akonadi.calendar.event" ) ) {
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

    kDebug() << "SETTING" << incidencesFor;
    mIncidencesForComboBox->setCurrentIndex( incidencesFor );
#if 0
    ++row;
    const QString whatsThisForReadOnlyFolders =
      i18n( "This setting allows you to disable alarms for folders shared by "
          "others. ");
    mAlarmsBlockedCheckBox = new QCheckBox( this );
    mAlarmsBlockedCheckBox->setText( i18n( "Block free/&busy and alarms locally" ) );
    gl->addWidget( mAlarmsBlockedCheckBox, row, 0, 1, 1 );
    mAlarmsBlockedCheckBox->setWhatsThis( whatsThisForReadOnlyFolders );
#endif
  } else {
    mIncidencesForComboBox = 0;
  }

  if ( KMKernel::self()->isImapFolder( col ) ) {
    mSharedSeenFlagsCheckBox = new QCheckBox( this );
    mSharedSeenFlagsCheckBox->setText( i18n( "Share unread state with all users" ) );
    mSharedSeenFlagsCheckBox->setChecked( sharedSeen );
    ++row;
    gl->addWidget( mSharedSeenFlagsCheckBox, row, 0, 1, 1 );
    mSharedSeenFlagsCheckBox->setWhatsThis( i18n( "If enabled, the unread state of messages in this folder will be the same "
        "for all users having access to this folder. If disabled (the default), every user with access to this folder has their "
        "own unread state." ) );
  } else {
    mSharedSeenFlagsCheckBox = 0;
  }

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
#endif


void CollectionGeneralPage::load(const Akonadi::Collection & col)
{
  mFolderCollection = FolderCollection::forCollection( col );
  init( col );
  QString displayName;
  if ( col.hasAttribute<Akonadi::EntityDisplayAttribute>() ) {
    displayName = col.attribute<Akonadi::EntityDisplayAttribute>()->displayName();
  }

  if ( !mIsLocalSystemFolder || mIsResourceFolder ) {
    if ( displayName.isEmpty() )
      mNameEdit->setText( col.name() );
    else
      mNameEdit->setText( displayName );
  }

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

  if ( !mIsLocalSystemFolder || mIsResourceFolder ) {

    if ( col.hasAttribute<Akonadi::EntityDisplayAttribute>() &&
         !col.attribute<Akonadi::EntityDisplayAttribute>()->displayName().isEmpty() )
      col.attribute<Akonadi::EntityDisplayAttribute>()->setDisplayName( mNameEdit->text() );
    else if( !mNameEdit->text().isEmpty() )
      col.setName( mNameEdit->text() );
  }

  Akonadi::CollectionAnnotationsAttribute *annotationsAttribute = col.attribute<Akonadi::CollectionAnnotationsAttribute>( Entity::AddIfMissing );

  QMap<QByteArray, QByteArray> annotations = annotationsAttribute->annotations();
  if ( mSharedSeenFlagsCheckBox && mSharedSeenFlagsCheckBox->isEnabled() )
  {
    annotations[ KOLAB_SHAREDSEEN ] = mSharedSeenFlagsCheckBox->isChecked() ? "true" : "false";
  }

  if ( mIncidencesForComboBox && mIncidencesForComboBox->isEnabled() )
    annotations[ KOLAB_INCIDENCESFOR ] = incidencesForToString( static_cast<IncidencesFor>( mIncidencesForComboBox->currentIndex() ) ).toLatin1();

  annotationsAttribute->setAnnotations(annotations);

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
