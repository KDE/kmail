// -*- mode: C++; c-file-style: "gnu" -*-
/**
 * kmfolderdialog.cpp
 *
 * Copyright (c) 1997-2004 KMail Developers
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */


#include "kmfolderdialog.h"
#include "kmacctfolder.h"
#include "kmfoldermgr.h"
#include <kpimidentities/identitycombo.h>
#include "kmfolderimap.h"
#include "kmfoldercachedimap.h"
#include "kmfolder.h"
#include "kmcommands.h"
#include "folderdialogacltab.h"
#include "folderdialogquotatab.h"
#include "kmailicalifaceimpl.h"
#include "globalsettings.h"
#include "folderrequester.h"
#include "mainfolderview.h"
#include "messagelistview/storagemodel.h"
#include "messagelist/core/aggregationcombobox.h"
#include "messagelist/core/aggregationconfigbutton.h"
#include "messagelist/core/themecombobox.h"
#include "messagelist/core/themeconfigbutton.h"

#include <keditlistbox.h>
#include <klineedit.h>
#include <klocale.h>
#include <knuminput.h>
#include <kmessagebox.h>
#include <kicondialog.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kvbox.h>

#include <QCheckBox>
#include <QLayout>
#include <QRegExp>
#include <QLabel>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QList>
#include <QVBoxLayout>
#include <QRadioButton>

#include <assert.h>

#include "templatesconfiguration.h"
#include "templatesconfiguration_kfg.h"

using namespace KMail;

static QString inCaseWeDecideToRenameTheTab( I18N_NOOP( "Permissions (ACL)" ) );

//-----------------------------------------------------------------------------
KMFolderDialog::KMFolderDialog( KMFolder *aFolder, KMFolderDir *aFolderDir,
                                MainFolderView *aParent, const QString& aCap,
                                const QString& aName) :
  KPageDialog( aParent ),
  mFolder( aFolder ),
  mFolderDir( aFolderDir ),
  mParentFolder( 0 ),
  mIsNewFolder( aFolder == 0 ),
  mShortcutTab( 0 ),					// for possible future expansion
  mExpiryTab( 0 ),
  mMaillistTab( 0 )

{
  setFaceType( Tabbed );
  setCaption( aCap );
  setButtons( Ok|Cancel );
  setDefaultButton( Ok );
  setObjectName( "KMFolderDialog" );
  setModal( true );

  kDebug();

  QStringList folderNames;
  QList<QPointer<KMFolder> > folders;
  // get all folders but search and folders that can not have children

  aParent->createFolderList(
      &folderNames, &folders,
      MainFolderView::IncludeLocalFolders | MainFolderView::IncludeImapFolders |
      MainFolderView::IncludeCachedImapFolders | MainFolderView::SkipFoldersWithNoChildren
    );

  if( mFolderDir ) {
    // search the parent folder of the folder
    FolderList::ConstIterator it;
    int i = 1;
    for( it = folders.constBegin(); it != folders.constEnd(); ++it, ++i ) {
      if( (*it)->child() == mFolderDir ) {
        mParentFolder = *it;
        break;
      }
    }
  }

  FolderDialogTab* tab;
  QFrame *box;

  box = new KVBox;
  addPage( box, i18nc("@title:tab General settings for a folder.", "General") );
  tab = new FolderDialogGeneralTab( this, aName, box );
  addTab( tab );

  box = new KVBox;
  addPage( box, i18nc( "@title:tab View settings for a folder.", "View" ) );
  tab = new FolderDialogViewTab( this, box );
  addTab( tab );

  // Don't add template tab for special folders
  if (!mFolder->isSystemFolder() || mFolder->isMainInbox())
  {
    box = new KVBox( this );
    addPage( box, i18n("Templates") );
    tab = new FolderDialogTemplatesTab( this, box );
    addTab( tab );
  }

  KMFolder* refFolder = mFolder ? mFolder : mParentFolder;
  KMFolderType folderType = refFolder ? refFolder->folderType() : KMFolderTypeUnknown;
  bool noContent = mFolder ? mFolder->storage()->noContent() : false;
  if ( !noContent && refFolder && ( folderType == KMFolderTypeImap || folderType == KMFolderTypeCachedImap ) ) {
    if ( FolderDialogACLTab::supports( refFolder ) ) {
      box = new KVBox;
      addPage( box, i18n("Access Control") );
      tab = new FolderDialogACLTab( this, box );
      addTab( tab );
    }
  }

  if ( !noContent && refFolder && ( folderType == KMFolderTypeImap || folderType == KMFolderTypeCachedImap ) ) {
    if ( FolderDialogQuotaTab::supports( refFolder ) ) {
      box = new KVBox;
      addPage( box, i18n("Quota") );
      tab = new FolderDialogQuotaTab( this, box );
      addTab( tab );
    }
  }

  box = new KVBox( this );				// this tab last of all
  mMaintenanceTab = addPage( box, i18n("Maintenance") );
  tab = new FolderDialogMaintenanceTab( this, box );
  addTab( tab );

  for ( int i = 0 ; i < mTabs.count() ; ++i )
    mTabs[i]->load();
  connect( this, SIGNAL( okClicked() ), SLOT( slotOk() ) );
  connect( this, SIGNAL( applyClicked() ), SLOT( slotApply() ) );
}


void KMFolderDialog::addTab( FolderDialogTab* tab )
{
  connect( tab, SIGNAL( readyForAccept() ),
           this, SLOT( slotReadyForAccept() ) );
  connect( tab, SIGNAL( cancelAccept() ),
           this, SLOT( slotCancelAccept() ) );
  //connect( tab, SIGNAL(changed( bool )),
  //         this, SLOT(slotChanged( bool )) );
  mTabs.append( tab );
}


bool KMFolderDialog::setPage( KMMainWidget::PropsPage whichPage )
{
  switch ( whichPage )
  {
case KMMainWidget::PropsGeneral:      break;		// the default page

case KMMainWidget::PropsShortcut:     if ( !mShortcutTab ) return ( false );
                                      setCurrentPage( mShortcutTab );
                                      break;

case KMMainWidget::PropsMailingList:  if ( !mMaillistTab ) return ( false );
                                      setCurrentPage( mMaillistTab );
                                      break;

case KMMainWidget::PropsExpire:       if ( !mExpiryTab ) return ( false );
                                      setCurrentPage( mExpiryTab );
                                      break;
  }
  return ( true );
}


// Not used yet (no button), but ready to be used :)
void KMFolderDialog::slotApply()
{
  if ( mFolder.isNull() && !mIsNewFolder ) // deleted meanwhile?
    return;

  for ( int i = 0 ; i < mTabs.count() ; ++i )
    mTabs[i]->save();
  if ( !mFolder.isNull() && mIsNewFolder ) // we just created it
    mIsNewFolder = false; // so it's not new anymore :)
}

// Called when pressing Ok
// We want to apply the changes first (which is async), before closing the dialog,
// in case of errors during the upload.
void KMFolderDialog::slotOk()
{
  if ( mFolder.isNull() && !mIsNewFolder ) { // deleted meanwhile?
    KDialog::accept();
    return;
  }

  mDelayedSavingTabs = 0; // number of tabs which need delayed saving
  for ( int i = 0 ; i < mTabs.count() ; ++i ) {
    FolderDialogTab::AcceptStatus s = mTabs[i]->accept();
    if ( s == FolderDialogTab::Canceled ) {
      slotCancelAccept();
      return;
    }
    else if ( s == FolderDialogTab::Delayed )
      ++mDelayedSavingTabs;
  }

  if ( mDelayedSavingTabs )
    enableButtonOk( false );
  else
    KDialog::accept();
}

void KMFolderDialog::slotReadyForAccept()
{
  --mDelayedSavingTabs;
  if ( mDelayedSavingTabs == 0 )
    KDialog::accept();
}

void KMFolderDialog::slotCancelAccept()
{
  mDelayedSavingTabs = -1;
  enableButtonOk( true );
  // Don't try to create it twice
  if ( !mFolder.isNull() )
    mIsNewFolder = false;

  // Other tabs might call slotReadyForAccept. -1 ensures that it won't close the dialog,
  // but the OK button being enabled means that people might succeed in running
  // the same job from save more than once.
  // Solution: mAcceptCanceled = true instead of -1.
  // Bah for now we only have one tab which can delay saving -> later.
}

void KMFolderDialog::slotChanged( bool )
{
  // TODO, support for 'changed', and Apply button.
  // sample code for here: KCMultiDialog calls bool changed() on every KCModuleProxy...
}

void KMFolderDialog::setFolder( KMFolder* folder )
{
  Q_ASSERT( mFolder.isNull() );
  mFolder = folder;
}

KMFolderDir* KMFolderDialog::folderDir() const
{
  return mFolderDir;
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

//----------------------------------------------------------------------------
// Used by the "General" and "Maintenance" tabs
static QString folderContentDesc( KMail::FolderContentsType type )
{
  switch ( type )
  {
case ContentsTypeMail:     return ( i18nc( "type of folder content", "Mail" ) );
case ContentsTypeCalendar: return ( i18nc( "type of folder content", "Calendar" ) );
case ContentsTypeContact:  return ( i18nc( "type of folder content", "Contacts" ) );
case ContentsTypeNote:     return ( i18nc( "type of folder content", "Notes" ) );
case ContentsTypeTask:     return ( i18nc( "type of folder content", "Tasks" ) );
case ContentsTypeJournal:  return ( i18nc( "type of folder content", "Journal" ) );
default:                   return ( i18nc( "type of folder content", "Unknown" ) );
  }
}

// Used by the "Maintenance" tab
// Not every description we need here is supported by KAccount::displayNameForType()
static QString folderTypeDesc( KMFolderType type )
{
  switch ( type )
  {
case KMFolderTypeMbox:       return ( i18nc( "type of folder storage", "Mailbox" ) );
case KMFolderTypeMaildir:    return ( i18nc( "type of folder storage", "Maildir" ) );
case KMFolderTypeCachedImap: return ( i18nc( "type of folder storage", "Disconnected IMAP" ) );
case KMFolderTypeImap:       return ( i18nc( "type of folder storage", "IMAP" ) );
case KMFolderTypeSearch:     return ( i18nc( "type of folder storage", "Search" ) );
default:                     return ( i18nc( "type of folder storage", "Unknown" ) );
  }
}

//----------------------------------------------------------------------------
KMail::FolderDialogGeneralTab::FolderDialogGeneralTab( KMFolderDialog* dlg,
                                                 const QString& aName,
                                                 QWidget* parent, const char* name )
  : FolderDialogTab( dlg, parent, name ),
    mSharedSeenFlagsCheckBox( 0 )
{
  mIsLocalSystemFolder = mDlg->folder()->isSystemFolder();
  mIsResourceFolder = kmkernel->iCalIface().isStandardResourceFolder( mDlg->folder() );

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
                  i18nc( "@info:whatsthis", "Check this option if you do not want that this folder "
                         "is shown in the folder selection dialogs, like the <interface>"
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
  mIdentityComboBox = new KPIMIdentities::IdentityCombo( kmkernel->identityManager(), this );
  label->setBuddy( mIdentityComboBox );
  gl->addWidget( mIdentityComboBox, row, 1 );
  mIdentityComboBox->setWhatsThis(
      i18n( "Select the sender identity to be used when writing new mail "
        "or replying to mail in this folder. This means that if you are in "
        "one of your work folders, you can make KMail use the corresponding "
        "sender email address, signature and signing or encryption keys "
        "automatically. Identities can be set up in the main configuration "
        "dialog. (Settings -> Configure KMail)") );

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
  topLayout->addStretch( 100 ); // eat all superfluous space

  initializeWithValuesFromFolder( mDlg->folder() );
}

void FolderDialogGeneralTab::load()
{
  // Nothing here, all is done in the ctor
}

void FolderDialogGeneralTab::initializeWithValuesFromFolder( KMFolder* folder ) {
  if ( !folder )
    return;

  // folder identity
  mIdentityComboBox->setCurrentIdentity( folder->identity() );
  mUseDefaultIdentityCheckBox->setChecked( folder->useDefaultIdentity() );

  // ignore new mail
  mNotifyOnNewMailCheckBox->setChecked( !folder->ignoreNewMail() );

  const bool keepInFolder = !folder->isReadOnly() && folder->putRepliesInSameFolder();
  mKeepRepliesInSameFolderCheckBox->setChecked( keepInFolder );
  mKeepRepliesInSameFolderCheckBox->setDisabled( folder->isReadOnly() );
  mHideInSelectionDialogCheckBox->setChecked( folder->hideInSelectionDialog() );

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
void FolderDialogGeneralTab::slotFolderNameChanged( const QString& str )
{
  mDlg->enableButtonOk( !str.isEmpty() );
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
void FolderDialogGeneralTab::slotIdentityCheckboxChanged()
{
  mIdentityComboBox->setEnabled( !mUseDefaultIdentityCheckBox->isChecked() );
}

//-----------------------------------------------------------------------------
bool FolderDialogGeneralTab::save()
{
  KMFolder* folder = mDlg->folder();
  folder->setIdentity( mIdentityComboBox->currentIdentity() );
  folder->setUseDefaultIdentity( mUseDefaultIdentityCheckBox->isChecked() );

  folder->setIgnoreNewMail( !mNotifyOnNewMailCheckBox->isChecked() );
  folder->setPutRepliesInSameFolder( mKeepRepliesInSameFolderCheckBox->isChecked() );
  folder->setHideInSelectionDialog( mHideInSelectionDialogCheckBox->isChecked() );

  QString fldName, oldFldName;
  if ( !mIsLocalSystemFolder || mIsResourceFolder )
  {
    QString acctName;
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

    if ( folder->folderType() == KMFolderTypeCachedImap ) {
      KMFolderCachedImap* dimap = static_cast<KMFolderCachedImap *>( mDlg->folder()->storage() );
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
      if ( mSharedSeenFlagsCheckBox && mSharedSeenFlagsCheckBox->isChecked() != dimap->sharedSeenFlags() ) {
        dimap->setSharedSeenFlags( mSharedSeenFlagsCheckBox->isChecked() );
        dimap->writeConfig();
      }
    }

    if( folder->folderType() == KMFolderTypeImap )
    {
      KMFolderImap* imapFolder = static_cast<KMFolderImap*>( folder->storage() );
      imapFolder->setIncludeInMailCheck(
          mNewMailCheckBox->isChecked() );
    }
    // make sure everything is on disk, connected slots will call readConfig()
    // when creating a new folder.
    folder->storage()->writeConfig();
    // Renamed an existing folder? We don't check for oldName == newName on
    // purpose here. The folder might be pending renaming on the next dimap
    // sync already, in which case the old name would still be around and
    // something like Calendar -> CalendarFoo -> Calendar inbetween syncs would
    // fail. Therefor let the folder sort it out itself, whether the rename is
    // a noop or not.
    if ( !oldFldName.isEmpty() )
    {
      kmkernel->folderMgr()->renameFolder( folder, fldName );
    } else {
      kmkernel->folderMgr()->contentsChanged();
    }
  }
  return true;
}

//----------------------------------------------------------------------------
FolderDialogViewTab::FolderDialogViewTab( KMFolderDialog * dlg, QWidget * parent )
: FolderDialogTab( dlg, parent, 0 )
{
  mIsLocalSystemFolder = mDlg->folder()->isSystemFolder();
  mIsResourceFolder = kmkernel->iCalIface().isStandardResourceFolder( mDlg->folder() );

  QVBoxLayout * topLayout = new QVBoxLayout( this );
  topLayout->setSpacing( KDialog::spacingHint() );
  topLayout->setMargin( 0 );

  // Musn't be able to edit details for non-resource, system folder.
  if ( !mIsLocalSystemFolder || mIsResourceFolder )
  {
    // icons
    mIconsCheckBox = new QCheckBox( i18n( "Use custom &icons"), this );
    mIconsCheckBox->setChecked( false );

    mNormalIconLabel = new QLabel( i18nc( "Icon used for folders with no unread messages.", "&Normal:" ), this );
    mNormalIconLabel->setEnabled( false );

    mNormalIconButton = new KIconButton( this );
    mNormalIconLabel->setBuddy( mNormalIconButton );
    mNormalIconButton->setIconType( KIconLoader::NoGroup, KIconLoader::Place, false );
    mNormalIconButton->setIconSize( 16 );
    mNormalIconButton->setStrictIconSize( true );
    mNormalIconButton->setFixedSize( 28, 28 );
    // Can't use iconset here.
    mNormalIconButton->setIcon( "folder" );
    mNormalIconButton->setEnabled( false );

    mUnreadIconLabel = new QLabel( i18nc( "Icon used for folders which do have unread messages.", "&Unread:" ), this );
    mUnreadIconLabel->setEnabled( false );

    mUnreadIconButton = new KIconButton( this );
    mUnreadIconLabel->setBuddy( mUnreadIconButton );
    mUnreadIconButton->setIconType( KIconLoader::NoGroup, KIconLoader::Place, false );
    mUnreadIconButton->setIconSize( 16 );
    mUnreadIconButton->setStrictIconSize( true );
    mUnreadIconButton->setFixedSize( 28, 28 );
    // Can't use iconset here.
    mUnreadIconButton->setIcon( "folder-open" );
    mUnreadIconButton->setEnabled( false );

    QHBoxLayout * iconHLayout = new QHBoxLayout();
    iconHLayout->addWidget( mIconsCheckBox );
    iconHLayout->addStretch( 2 );
    iconHLayout->addWidget( mNormalIconLabel );
    iconHLayout->addWidget( mNormalIconButton );
    iconHLayout->addWidget( mUnreadIconLabel );
    iconHLayout->addWidget( mUnreadIconButton );
    iconHLayout->addStretch( 1 );
    topLayout->addLayout( iconHLayout );

    connect( mIconsCheckBox, SIGNAL( toggled( bool ) ),
             mNormalIconLabel, SLOT( setEnabled( bool ) ) );
    connect( mIconsCheckBox, SIGNAL( toggled( bool ) ),
             mNormalIconButton, SLOT( setEnabled( bool ) ) );
    connect( mIconsCheckBox, SIGNAL( toggled( bool ) ),
             mUnreadIconButton, SLOT( setEnabled( bool ) ) );
    connect( mIconsCheckBox, SIGNAL( toggled( bool ) ),
             mUnreadIconLabel, SLOT( setEnabled( bool ) ) );

    connect( mNormalIconButton, SIGNAL( iconChanged( const QString& ) ),
             this, SLOT( slotChangeIcon( const QString& ) ) );
  }

  // sender or receiver column
  const QString senderReceiverColumnTip = i18n( "Show Sender/Receiver Column in List of Messages" );

  QLabel * senderReceiverColumnLabel = new QLabel( i18n( "Sho&w column:" ), this );
  mShowSenderReceiverComboBox = new KComboBox( this );
  mShowSenderReceiverComboBox->setToolTip( senderReceiverColumnTip );
  senderReceiverColumnLabel->setBuddy( mShowSenderReceiverComboBox );
  mShowSenderReceiverComboBox->insertItem( 0, i18nc( "@item:inlistbox Show default value.", "Default" ) );
  mShowSenderReceiverComboBox->insertItem( 1, i18nc( "@item:inlistbox Show sender.", "Sender" ) );
  mShowSenderReceiverComboBox->insertItem( 2, i18nc( "@item:inlistbox Show receiver.", "Receiver" ) );

  QHBoxLayout * senderReceiverColumnHLayout = new QHBoxLayout();
  senderReceiverColumnHLayout->addWidget( senderReceiverColumnLabel );
  senderReceiverColumnHLayout->addWidget( mShowSenderReceiverComboBox );
  topLayout->addLayout( senderReceiverColumnHLayout );

  // message list
  QGroupBox * messageListGroup = new QGroupBox( i18n( "Message List" ), this );
  QVBoxLayout * messageListGroupLayout = new QVBoxLayout( messageListGroup );
  messageListGroupLayout->setSpacing( KDialog::spacingHint() );
  topLayout->addWidget( messageListGroup );

  // message list aggregation
  mUseDefaultAggregationCheckBox = new QCheckBox( i18n( "Use default aggregation" ), messageListGroup );
  messageListGroupLayout->addWidget( mUseDefaultAggregationCheckBox );
  connect( mUseDefaultAggregationCheckBox, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotAggregationCheckboxChanged() ) );

  mAggregationComboBox = new MessageList::Core::AggregationComboBox( messageListGroup );

  QLabel * aggregationLabel = new QLabel( i18n( "Aggregation" ), messageListGroup );
  aggregationLabel->setBuddy( mAggregationComboBox );

  using MessageList::Core::AggregationConfigButton;
  AggregationConfigButton * aggregationConfigButton = new AggregationConfigButton( messageListGroup, mAggregationComboBox );
  // Make sure any changes made in the aggregations configure dialog are reflected in the combo.
  connect( aggregationConfigButton, SIGNAL( configureDialogCompleted() ),
           this, SLOT( slotSelectFolderAggregation() ) );

  QHBoxLayout * aggregationLayout = new QHBoxLayout();
  aggregationLayout->addWidget( aggregationLabel, 1 );
  aggregationLayout->addWidget( mAggregationComboBox, 1 );
  aggregationLayout->addWidget( aggregationConfigButton, 0 );
  messageListGroupLayout->addLayout( aggregationLayout );

  // message list theme
  mUseDefaultThemeCheckBox = new QCheckBox( i18n( "Use default theme" ), messageListGroup );
  messageListGroupLayout->addWidget( mUseDefaultThemeCheckBox );
  connect( mUseDefaultThemeCheckBox, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotThemeCheckboxChanged() ) );

  mThemeComboBox = new MessageList::Core::ThemeComboBox( messageListGroup );

  QLabel * themeLabel = new QLabel( i18n( "Theme" ), messageListGroup );
  themeLabel->setBuddy( mThemeComboBox );

  using MessageList::Core::ThemeConfigButton;
  ThemeConfigButton * themeConfigButton = new ThemeConfigButton( messageListGroup, mThemeComboBox );
  // Make sure any changes made in the themes configure dialog are reflected in the combo.
  connect( themeConfigButton, SIGNAL( configureDialogCompleted() ),
           this, SLOT( slotSelectFolderTheme() ) );

  QHBoxLayout * themeLayout = new QHBoxLayout();
  themeLayout->addWidget( themeLabel, 1 );
  themeLayout->addWidget( mThemeComboBox, 1 );
  themeLayout->addWidget( themeConfigButton, 0 );
  messageListGroupLayout->addLayout( themeLayout );

  topLayout->addStretch( 100 );

  initializeWithValuesFromFolder( mDlg->folder() );
}

void FolderDialogViewTab::load()
{
  // All loading is done in the constructor.
}

bool FolderDialogViewTab::save()
{
  KMFolder * folder = mDlg->folder();

  // folder icons
  if ( !mIsLocalSystemFolder || mIsResourceFolder )
  {
    // Update the tree if new icon paths are different and not empty or if
    // useCustomIcons changed.
    if ( folder->useCustomIcons() != mIconsCheckBox->isChecked() )
    {
      folder->setUseCustomIcons( mIconsCheckBox->isChecked() );
      // Reset icons, useCustomIcons was turned off.
      if( !folder->useCustomIcons() )
        folder->setIconPaths( "", "" );
    }
    if ( folder->useCustomIcons() &&
         ( ( mNormalIconButton->icon() != folder->normalIconPath() &&
             !mNormalIconButton->icon().isEmpty() ) ||
           ( mUnreadIconButton->icon() != folder->unreadIconPath() &&
             !mUnreadIconButton->icon().isEmpty() ) ) )
    {
      folder->setIconPaths( mNormalIconButton->icon(), mUnreadIconButton->icon() );
    }
  }

  // sender or receiver column
  if ( mShowSenderReceiverComboBox->currentIndex() == 1 )
    folder->setUserWhoField( "From" );
  else if ( mShowSenderReceiverComboBox->currentIndex() == 2 )
    folder->setUserWhoField( "To" );
  else
    folder->setUserWhoField( "" );

  // message list aggregation
  MessageListView::StorageModel messageListStorageModel( folder );
  const bool usePrivateAggregation = !mUseDefaultAggregationCheckBox->isChecked();
  mAggregationComboBox->writeStorageModelConfig( &messageListStorageModel, usePrivateAggregation );

  // message list theme
  const bool usePrivateTheme = !mUseDefaultThemeCheckBox->isChecked();
  mThemeComboBox->writeStorageModelConfig( &messageListStorageModel, usePrivateTheme );

  return true;
}

void FolderDialogViewTab::slotChangeIcon( const QString & icon )
{
    mUnreadIconButton->setIcon( icon );
}

void FolderDialogViewTab::slotAggregationCheckboxChanged()
{
  mAggregationComboBox->setEnabled( !mUseDefaultAggregationCheckBox->isChecked() );
}

void FolderDialogViewTab::slotThemeCheckboxChanged()
{
  mThemeComboBox->setEnabled( !mUseDefaultThemeCheckBox->isChecked() );
}

void FolderDialogViewTab::slotSelectFolderAggregation()
{
  MessageListView::StorageModel messageListStorageModel( mDlg->folder() );
  bool usesPrivateAggregation = false;
  mAggregationComboBox->readStorageModelConfig( &messageListStorageModel, usesPrivateAggregation );
  mUseDefaultAggregationCheckBox->setChecked( !usesPrivateAggregation );
}

void FolderDialogViewTab::slotSelectFolderTheme()
{
  MessageListView::StorageModel messageListStorageModel( mDlg->folder() );
  bool usesPrivateTheme = false;
  mThemeComboBox->readStorageModelConfig( &messageListStorageModel, usesPrivateTheme );
  mUseDefaultThemeCheckBox->setChecked( !usesPrivateTheme );
}

void FolderDialogViewTab::initializeWithValuesFromFolder( KMFolder * folder )
{
  if ( !folder )
    return;

  // folder icons
  if ( !mIsLocalSystemFolder || mIsResourceFolder )
  {
    const bool customIcons = folder->useCustomIcons();
    mIconsCheckBox->setChecked( customIcons );
    mNormalIconLabel->setEnabled( customIcons );
    mNormalIconButton->setEnabled( customIcons );
    mUnreadIconLabel->setEnabled( customIcons );
    mUnreadIconButton->setEnabled( customIcons );

    const QString normalIconPath = folder->normalIconPath();
    if( !normalIconPath.isEmpty() )
      mNormalIconButton->setIcon( normalIconPath );

    const QString unreadIconPath = folder->unreadIconPath();
    if( !unreadIconPath.isEmpty() )
      mUnreadIconButton->setIcon( unreadIconPath );
  }

  // sender or receiver column
  const QString whoField = mDlg->folder()->userWhoField();
  if ( whoField.isEmpty() )
    mShowSenderReceiverComboBox->setCurrentIndex( 0 );
  else if ( whoField == "From" )
    mShowSenderReceiverComboBox->setCurrentIndex( 1 );
  else if ( whoField == "To" )
    mShowSenderReceiverComboBox->setCurrentIndex( 2 );

  // message list aggregation
  slotSelectFolderAggregation();

  // message list theme
  slotSelectFolderTheme();
}

//----------------------------------------------------------------------------
KMail::FolderDialogTemplatesTab::FolderDialogTemplatesTab( KMFolderDialog *dlg,
                                                     QWidget *parent )
  : FolderDialogTab( dlg, parent, 0 )
{
  mIsLocalSystemFolder = mDlg->folder()->isSystemFolder();

  QVBoxLayout *topLayout = new QVBoxLayout( this );
  topLayout->setMargin( 0 );
  topLayout->setSpacing( KDialog::spacingHint() );

  QHBoxLayout *topItems = new QHBoxLayout( this );
  topLayout->addLayout( topItems );

  mCustom = new QCheckBox( i18n("&Use custom message templates in this folder"), this );
  topItems->addWidget( mCustom, Qt::AlignLeft );

  mWidget = new TemplatesConfiguration( this, "folder-templates" );
  mWidget->setEnabled( false );

  // Move the help label outside of the templates configuration widget,
  // so that the help can be read even if the widget is not enabled.
  topItems->addStretch( 9 );
  topItems->addWidget( mWidget->helpLabel(), Qt::AlignRight );

  topLayout->addWidget( mWidget );

  QHBoxLayout *btns = new QHBoxLayout();
  btns->setSpacing( KDialog::spacingHint() );
  mCopyGlobal = new KPushButton( i18n("&Copy Global Templates"), this );
  mCopyGlobal->setEnabled( false );
  btns->addWidget( mCopyGlobal );
  topLayout->addLayout( btns );

  connect( mCustom, SIGNAL(toggled( bool )),
           mWidget, SLOT(setEnabled( bool )) );
  connect( mCustom, SIGNAL(toggled( bool )),
           mCopyGlobal, SLOT(setEnabled( bool )) );

  connect( mCopyGlobal, SIGNAL(clicked()),
           this, SLOT(slotCopyGlobal()) );

  initializeWithValuesFromFolder( mDlg->folder() );

  connect( mWidget, SIGNAL(changed()),
           this, SLOT(slotEmitChanged( void )) );
}

void FolderDialogTemplatesTab::load()
{
}

void FolderDialogTemplatesTab::initializeWithValuesFromFolder( KMFolder* folder ) {
  if ( !folder )
    return;

  mFolder = folder;

  QString fid = folder->idString();

  Templates t( fid );

  mCustom->setChecked(t.useCustomTemplates());

  mIdentity = folder->identity();

  mWidget->loadFromFolder( fid, mIdentity );
}

//-----------------------------------------------------------------------------
bool FolderDialogTemplatesTab::save()
{
  KMFolder* folder = mDlg->folder();

  QString fid = folder->idString();
  Templates t(fid);

  kDebug() << "use custom templates for folder" << fid <<":" << mCustom->isChecked();
  t.setUseCustomTemplates(mCustom->isChecked());
  t.writeConfig();

  mWidget->saveToFolder(fid);

  return true;
}


void FolderDialogTemplatesTab::slotEmitChanged() {}

void FolderDialogTemplatesTab::slotCopyGlobal() {
  if ( mIdentity ) {
    mWidget->loadFromIdentity( mIdentity );
  }
  else {
    mWidget->loadFromGlobal();
  }
}


//----------------------------------------------------------------------------
KMail::FolderDialogMaintenanceTab::FolderDialogMaintenanceTab( KMFolderDialog *dlg,
                                                               QWidget *parent )
  : FolderDialogTab( dlg, parent ),
    mRebuildIndexButton( 0 ),
    mRebuildImapButton( 0 ),
    mCompactStatusLabel( 0 ),
    mCompactNowButton( 0 )
{
  QVBoxLayout *topLayout = new QVBoxLayout( this );
  topLayout->setMargin( 0 );
  topLayout->setSpacing( KDialog::spacingHint() );

  mFolder = dlg->folder();
  //kDebug() << "        folder:" << mFolder->name();
  //kDebug() << "          type:" << mFolder->folderType();
  //kDebug() << "   storagetype:" << mFolder->storage()->folderType();
  //kDebug() << "  contentstype:" << mFolder->storage()->contentsType();
  //kDebug() << "      filename:" << mFolder->fileName();
  //kDebug() << "      location:" << mFolder->location();
  //kDebug() << "      indexloc:" << mFolder->indexLocation() ;
  //kDebug() << "     subdirloc:" << mFolder->subdirLocation();
  //kDebug() << "         count:" << mFolder->count();
  //kDebug() << "        unread:" << mFolder->countUnread();
  //kDebug() << "  needscompact:" << mFolder->needsCompacting();
  //kDebug() << "   compactable:" << mFolder->storage()->compactable();

  QGroupBox *filesGroup = new QGroupBox( i18n("Files"), this );
  QFormLayout *box = new QFormLayout( filesGroup );
  box->setSpacing( KDialog::spacingHint() );

  QString contentsDesc = folderContentDesc( mFolder->storage()->contentsType() );
  QLabel *label = new QLabel( contentsDesc, filesGroup );
  // Passing a QLabel rather than QString to addRow(), so that it doesn't
  // get a buddy set (except in the cases where we do want one).
  box->addRow( new QLabel( i18nc( "@label:textbox Folder content type (eg. Mail)", "Contents:" ),
                           filesGroup ), label );

  KMFolderType folderType = mFolder->folderType();
  QString folderDesc = folderTypeDesc( folderType );
  label = new QLabel( folderDesc, filesGroup );
  box->addRow( new QLabel( i18n("Folder type:"), filesGroup ), label );

  KLineEdit *label2 = new KLineEdit( mFolder->location(), filesGroup );
  label2->setReadOnly( true );
  box->addRow( i18n("Location:"), label2 );

  mFolderSizeLabel = new QLabel( i18nc( "folder size", "Not available" ), filesGroup );
  box->addRow( new QLabel( i18n("Size:"), filesGroup ), mFolderSizeLabel );
  connect( mFolder, SIGNAL(folderSizeChanged( KMFolder * )),SLOT(updateFolderIndexSizes()) );

  label2 = new KLineEdit( mFolder->indexLocation(), filesGroup );
  label2->setReadOnly( true );
  box->addRow( i18n("Index:"), label2 );

  mIndexSizeLabel = new QLabel( i18nc( "folder size", "Not available" ), filesGroup );
  box->addRow( new QLabel( i18n("Size:"), filesGroup ), mIndexSizeLabel );

  if ( folderType == KMFolderTypeMaildir )	// see KMMainWidget::slotTroubleshootMaildir()
  {
    mRebuildIndexButton = new KPushButton( i18n("Recreate Index"), filesGroup );
    QObject::connect( mRebuildIndexButton, SIGNAL(clicked()), SLOT(slotRebuildIndex()) );

    QHBoxLayout *hbl = new QHBoxLayout();	// to get an unstretched, right aligned button
    hbl->addStretch( 1 );
    hbl->addWidget( mRebuildIndexButton );
    box->addRow( QString(), hbl );
  }

  if ( folderType == KMFolderTypeCachedImap )
  {
    mRebuildImapButton = new KPushButton( i18n("Rebuild Local IMAP Cache"), filesGroup );
    QObject::connect( mRebuildImapButton, SIGNAL(clicked()), SLOT(slotRebuildImap()) );

    QHBoxLayout *hbl = new QHBoxLayout();
    hbl->addStretch( 1 );
    hbl->addWidget( mRebuildImapButton );
    box->addRow( QString(), hbl );
  }

  topLayout->addWidget( filesGroup );

  QGroupBox *messagesGroup = new QGroupBox( i18n("Messages"), this);
  box = new QFormLayout( messagesGroup );
  box->setSpacing( KDialog::spacingHint() );

  label = new QLabel( QString::number( mFolder->count() ), messagesGroup );
  box->addRow( new QLabel( i18n("Total messages:"), messagesGroup ), label );

  label = new QLabel( QString::number( mFolder->countUnread() ), messagesGroup );
  box->addRow( new QLabel( i18n("Unread messages:"), messagesGroup ), label );

  // Compaction is only sensible and currently supported for mbox folders.
  //
  // FIXME: should "compaction supported" be an attribute of the folder
  // storage type (mFolder->storage()->isCompactionSupported() or something
  // like that)?
  if ( folderType == KMFolderTypeMbox )			// compaction only sensible for this
  {
    mCompactStatusLabel = new QLabel( i18nc( "compaction status", "Unknown" ), messagesGroup );
    box->addRow( new QLabel( i18n("Compaction:"), messagesGroup ), mCompactStatusLabel );

    mCompactNowButton = new KPushButton( i18n("Compact Now"), messagesGroup );
    mCompactNowButton->setEnabled( false );
    connect( mCompactNowButton, SIGNAL(clicked()), SLOT(slotCompactNow()) );

    QHBoxLayout *hbl = new QHBoxLayout();
    hbl->addStretch( 1 );
    hbl->addWidget( mCompactNowButton );
    box->addRow( QString(), hbl );
  }

  topLayout->addWidget( messagesGroup );

  topLayout->addStretch( 100 );
}


// What happened to KIO::convertSizeWithBytes? - that would be really useful here
static QString convertSizeWithBytes( qint64 size )
{
  QString s1 = KIO::convertSize( size );
  QString s2 = i18nc( "File size in bytes", "%1 B", size );
  return ( s1 == s2 ) ? s1 : i18n( "%1 (%2)", s1, s2 );
}


void FolderDialogMaintenanceTab::load()
{
  updateControls();
  updateFolderIndexSizes();
}


bool FolderDialogMaintenanceTab::save()
{
  return true;
}


void FolderDialogMaintenanceTab::updateControls()
{
  if ( mCompactStatusLabel )
  {
    QString s;
    if ( mFolder->needsCompacting() )
    {
      if ( mFolder->storage()->compactable() ) s = i18nc( "compaction status", "Possible");
      else s = i18nc( "compaction status", "Possible, but unsafe");
    }
    else s = i18nc( "compaction status", "Not required");
    mCompactStatusLabel->setText( s );
  }

  if ( mCompactNowButton )
    mCompactNowButton->setEnabled( mFolder->needsCompacting() );
}


void FolderDialogMaintenanceTab::updateFolderIndexSizes()
{
  qint64 folderSize = mFolder->storage()->folderSize();
  if ( folderSize != -1 )
  {
    mFolderSizeLabel->setText( convertSizeWithBytes( folderSize ) );
  }

  KUrl u;
  u.setPath( mFolder->indexLocation() );
  if ( u.isValid() && u.isLocalFile() )
  {
    // the index should always be a file
    QFileInfo fi( u.toLocalFile() );
    mIndexSizeLabel->setText( convertSizeWithBytes( fi.size() ) );
  }
}


void FolderDialogMaintenanceTab::slotCompactNow()
{
  if ( !mFolder->needsCompacting() ) return;

  if ( !mFolder->storage()->compactable() )
  {
    if ( KMessageBox::warningContinueCancel( this,
                                             i18nc( "@info",
      "Compacting folder <resource>%1</resource> may not be safe.<nl/>"
      "<warning>This may result in index or mailbox corruption.</warning><nl/>"
      "Ensure that you have a recent backup of the mailbox and messages.", mFolder->label() ),
                                             i18nc( "@title", "Really compact folder?" ),
                                             KGuiItem( i18nc( "@action:button", "Compact Folder" ) ),
                                             KStandardGuiItem::cancel(), QString(),
                                             KMessageBox::Notify | KMessageBox::Dangerous )
                                             != KMessageBox::Continue ) return;
    mFolder->storage()->enableCompaction();
  }

  // finding and activating the action, because
  // KMMainWidget::slotCompactFolder() and similar actions are protected
  QAction *act = kmkernel->getKMMainWidget()->action( "compact" );
  if ( act ) act->activate( QAction::Trigger );

  updateControls();
  updateFolderIndexSizes();
}


void FolderDialogMaintenanceTab::slotRebuildIndex()
{
  QAction *act = kmkernel->getKMMainWidget()->action( "troubleshoot_maildir" );
  if ( !act ) return;

  act->activate( QAction::Trigger );
  updateFolderIndexSizes();
}


void FolderDialogMaintenanceTab::slotRebuildImap()
{
  QAction *act = kmkernel->getKMMainWidget()->action( "troubleshoot_folder" );
  if ( act ) act->activate( QAction::Trigger );
}


#include "kmfolderdialog.moc"
