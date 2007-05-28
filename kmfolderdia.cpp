// -*- mode: C++; c-file-style: "gnu" -*-
/**
 * kmfolderdia.cpp
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

#include <config.h>

#include "kmfolderdia.h"
#include "kmacctfolder.h"
#include "kmfoldermgr.h"
#include <libkpimidentities/identitycombo.h>
#include "kmfolderimap.h"
#include "kmfoldercachedimap.h"
#include "kmfolder.h"
#include "kmheaders.h"
#include "kmcommands.h"
#include "kmfoldertree.h"
#include "folderdiaacltab.h"
#include "folderdiaquotatab.h"
#include "kmailicalifaceimpl.h"
#include "globalsettings.h"
#include "folderrequester.h"

#include <keditlistbox.h>
#include <klineedit.h>
#include <klocale.h>
#include <knuminput.h>
#include <kmessagebox.h>
#include <kicondialog.h>
#include <kconfig.h>
#include <kdebug.h>
#include <klistview.h>
#include <kpushbutton.h>

#include <qcheckbox.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qregexp.h>
#include <qlabel.h>
#include <qvbox.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

#include <assert.h>
#include <qhbuttongroup.h>
#include <qradiobutton.h>
#include <qtextedit.h>

#include "templatesconfiguration.h"
#include "templatesconfiguration_kfg.h"

#include "kmfolderdia.moc"

using namespace KMail;

static QString inCaseWeDecideToRenameTheTab( I18N_NOOP( "Permissions (ACL)" ) );

//-----------------------------------------------------------------------------
KMFolderDialog::KMFolderDialog(KMFolder *aFolder, KMFolderDir *aFolderDir,
			       KMFolderTree* aParent, const QString& aCap,
			       const QString& aName):
  KDialogBase( KDialogBase::Tabbed,
               aCap, KDialogBase::Ok|KDialogBase::Cancel,
               KDialogBase::Ok, aParent, "KMFolderDialog", TRUE ),
  mFolder( aFolder ),
  mFolderDir( aFolderDir ),
  mParentFolder( 0 ),
  mIsNewFolder( aFolder == 0 ),
  mFolderTree( aParent )
{
  kdDebug(5006)<<"KMFolderDialog::KMFolderDialog()" << endl;

  QStringList folderNames;
  QValueList<QGuardedPtr<KMFolder> > folders;
  // get all folders but search and folders that can not have children
  aParent->createFolderList(&folderNames, &folders, true, true,
                            true, false, true, false);

  if( mFolderDir ) {
    // search the parent folder of the folder
    FolderList::ConstIterator it;
    int i = 1;
    for( it = folders.begin(); it != folders.end(); ++it, ++i ) {
      if( (*it)->child() == mFolderDir ) {
        mParentFolder = *it;
        break;
      }
    }
  }

  FolderDiaTab* tab;
  QVBox* box;

  box = addVBoxPage( i18n("General") );
  tab = new FolderDiaGeneralTab( this, aName, box );
  addTab( tab );
  box = addVBoxPage( i18n("Templates") );
  tab = new FolderDiaTemplatesTab( this, box );
  addTab( tab );

  KMFolder* refFolder = mFolder ? mFolder : mParentFolder;
  KMFolderType folderType = refFolder ? refFolder->folderType() : KMFolderTypeUnknown;
  bool noContent = mFolder ? mFolder->storage()->noContent() : false;
  if ( !noContent && refFolder && ( folderType == KMFolderTypeImap || folderType == KMFolderTypeCachedImap ) ) {
    if ( FolderDiaACLTab::supports( refFolder ) ) {
      box = addVBoxPage( i18n("Access Control") );
      tab = new FolderDiaACLTab( this, box );
      addTab( tab );
    }
  }
  if ( !noContent && refFolder && ( folderType == KMFolderTypeImap || folderType == KMFolderTypeCachedImap ) ) {
    if ( FolderDiaQuotaTab::supports( refFolder ) ) {
      box = addVBoxPage( i18n("Quota") );
      tab = new FolderDiaQuotaTab( this, box );
      addTab( tab );
    }
  }

  for ( unsigned int i = 0 ; i < mTabs.count() ; ++i )
    mTabs[i]->load();
}

void KMFolderDialog::addTab( FolderDiaTab* tab )
{
  connect( tab, SIGNAL( readyForAccept() ),
           this, SLOT( slotReadyForAccept() ) );
  connect( tab, SIGNAL( cancelAccept() ),
           this, SLOT( slotCancelAccept() ) );
  //connect( tab, SIGNAL(changed( bool )),
  //         this, SLOT(slotChanged( bool )) );
  mTabs.append( tab );
}

// Not used yet (no button), but ready to be used :)
void KMFolderDialog::slotApply()
{
  if ( mFolder.isNull() && !mIsNewFolder ) { // deleted meanwhile?
    KDialogBase::slotApply();
    return;
  }
  for ( unsigned int i = 0 ; i < mTabs.count() ; ++i )
    mTabs[i]->save();
  if ( !mFolder.isNull() && mIsNewFolder ) // we just created it
    mIsNewFolder = false; // so it's not new anymore :)
  KDialogBase::slotApply();
}

// Called when pressing Ok
// We want to apply the changes first (which is async), before closing the dialog,
// in case of errors during the upload.
void KMFolderDialog::slotOk()
{
  if ( mFolder.isNull() && !mIsNewFolder ) { // deleted meanwhile?
    KDialogBase::slotOk();
    return;
  }

  mDelayedSavingTabs = 0; // number of tabs which need delayed saving
  for ( unsigned int i = 0 ; i < mTabs.count() ; ++i ) {
    FolderDiaTab::AcceptStatus s = mTabs[i]->accept();
    if ( s == FolderDiaTab::Canceled ) {
      slotCancelAccept();
      return;
    }
    else if ( s == FolderDiaTab::Delayed )
      ++mDelayedSavingTabs;
  }

  if ( mDelayedSavingTabs )
    enableButtonOK( false );
  else
    KDialogBase::slotOk();
}

void KMFolderDialog::slotReadyForAccept()
{
  --mDelayedSavingTabs;
  if ( mDelayedSavingTabs == 0 )
    KDialogBase::slotOk();
}

void KMFolderDialog::slotCancelAccept()
{
  mDelayedSavingTabs = -1;
  enableButtonOK( true );
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

static void addLine( QWidget *parent, QVBoxLayout* layout )
{
   QFrame *line = new QFrame( parent, "line" );
   line->setGeometry( QRect( 80, 150, 250, 20 ) );
   line->setFrameShape( QFrame::HLine );
   line->setFrameShadow( QFrame::Sunken );
   line->setFrameShape( QFrame::HLine );
   layout->addWidget( line );
}

//----------------------------------------------------------------------------
KMail::FolderDiaGeneralTab::FolderDiaGeneralTab( KMFolderDialog* dlg,
                                                 const QString& aName,
                                                 QWidget* parent, const char* name )
  : FolderDiaTab( parent, name ), mDlg( dlg )
{


  mIsLocalSystemFolder = mDlg->folder()->isSystemFolder() &&
       mDlg->folder()->folderType() != KMFolderTypeImap &&
       mDlg->folder()->folderType() != KMFolderTypeCachedImap;

  QLabel *label;

  QVBoxLayout *topLayout = new QVBoxLayout( this, 0, KDialog::spacingHint() );

  // Musn't be able to edit details for a system folder.
  if ( !mIsLocalSystemFolder ) {

    QHBoxLayout *hl = new QHBoxLayout( topLayout );
    hl->setSpacing( KDialog::spacingHint() );

    label = new QLabel( i18n("&Name:"), this );
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


    //start icons group
    QVBoxLayout *ivl = new QVBoxLayout( topLayout );
    ivl->setSpacing( KDialog::spacingHint() );

    QHBoxLayout *ihl = new QHBoxLayout( ivl );
    mIconsCheckBox = new QCheckBox( i18n("Use custom &icons"), this );
    mIconsCheckBox->setChecked( false );
    ihl->addWidget( mIconsCheckBox );
    ihl->addStretch( 2 );

    mNormalIconLabel = new QLabel( i18n("&Normal:"), this );
    mNormalIconLabel->setEnabled( false );
    ihl->addWidget( mNormalIconLabel );

    mNormalIconButton = new KIconButton( this );
    mNormalIconLabel->setBuddy( mNormalIconButton );
    mNormalIconButton->setIconType( KIcon::NoGroup , KIcon::Any, true );
    mNormalIconButton->setIconSize( 16 );
    mNormalIconButton->setStrictIconSize( true );
    mNormalIconButton->setFixedSize( 28, 28 );
    // Can't use iconset here
    mNormalIconButton->setIcon( "folder" );
    mNormalIconButton->setEnabled( false );
    ihl->addWidget( mNormalIconButton );

    mUnreadIconLabel = new QLabel( i18n("&Unread:"), this );
    mUnreadIconLabel->setEnabled( false );
    ihl->addWidget( mUnreadIconLabel );

    mUnreadIconButton = new KIconButton( this );
    mUnreadIconLabel->setBuddy( mUnreadIconButton );
    mUnreadIconButton->setIconType( KIcon::NoGroup, KIcon::Any, true );
    mUnreadIconButton->setIconSize( 16 );
    mUnreadIconButton->setStrictIconSize( true );
    mUnreadIconButton->setFixedSize( 28, 28 );
    // Can't use iconset here
    mUnreadIconButton->setIcon( "folder_open" );
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
    addLine( this, topLayout);
  }


  // should new mail in this folder be ignored?
  QHBoxLayout *hbl = new QHBoxLayout( topLayout );
  hbl->setSpacing( KDialog::spacingHint() );
  mNotifyOnNewMailCheckBox =
    new QCheckBox( i18n("Act on new/unread mail in this folder" ), this );
  QWhatsThis::add( mNotifyOnNewMailCheckBox,
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

    QHBoxLayout *nml = new QHBoxLayout( topLayout );
    nml->setSpacing( KDialog::spacingHint() );
    mNewMailCheckBox = new QCheckBox( i18n("Include this folder in mail checks"), this );
    // default is on
    mNewMailCheckBox->setChecked(true);
    nml->addWidget( mNewMailCheckBox );
    nml->addStretch( 1 );
  }

  // should replies to mails in this folder be kept in this same folder?
  hbl = new QHBoxLayout( topLayout );
  hbl->setSpacing( KDialog::spacingHint() );
  mKeepRepliesInSameFolderCheckBox =
    new QCheckBox( i18n("Keep replies in this folder" ), this );
  QWhatsThis::add( mKeepRepliesInSameFolderCheckBox,
                   i18n( "Check this option if you want replies you write "
                         "to mails in this folder to be put in this same folder "
                         "after sending, instead of in the configured sent-mail folder." ) );
  hbl->addWidget( mKeepRepliesInSameFolderCheckBox );
  hbl->addStretch( 1 );

  addLine( this, topLayout );

  // use grid layout for the following combobox settings
  QGridLayout *gl = new QGridLayout( topLayout, 3, 2, KDialog::spacingHint() );
  gl->setColStretch( 1, 100 ); // make the second column use all available space
  int row = -1;

  // sender or receiver column?
  ++row;
  QString tip = i18n("Show Sender/Receiver Column in List of Messages");

  QLabel *sender_label = new QLabel( i18n("Sho&w column:" ), this );
  gl->addWidget( sender_label, row, 0 );
  mShowSenderReceiverComboBox = new QComboBox( this );
  QToolTip::add( mShowSenderReceiverComboBox, tip );
  sender_label->setBuddy(mShowSenderReceiverComboBox);
  gl->addWidget( mShowSenderReceiverComboBox, row, 1 );
  mShowSenderReceiverComboBox->insertItem(i18n("Default"), 0);
  mShowSenderReceiverComboBox->insertItem(i18n("Sender"), 1);
  mShowSenderReceiverComboBox->insertItem(i18n("Receiver"), 2);

  QString whoField;
  if (mDlg->folder()) whoField = mDlg->folder()->userWhoField();
  if (whoField.isEmpty()) mShowSenderReceiverComboBox->setCurrentItem(0);
  else if (whoField == "From") mShowSenderReceiverComboBox->setCurrentItem(1);
  else if (whoField == "To") mShowSenderReceiverComboBox->setCurrentItem(2);


  // sender identity
  ++row;
  label = new QLabel( i18n("&Sender identity:"), this );
  gl->addWidget( label, row, 0 );
  mIdentityComboBox = new KPIM::IdentityCombo( kmkernel->identityManager(), this );
  label->setBuddy( mIdentityComboBox );
  gl->addWidget( mIdentityComboBox, row, 1 );
  QWhatsThis::add( mIdentityComboBox,
      i18n( "Select the sender identity to be used when writing new mail "
        "or replying to mail in this folder. This means that if you are in "
        "one of your work folders, you can make KMail use the corresponding "
        "sender email address, signature and signing or encryption keys "
        "automatically. Identities can be set up in the main configuration "
        "dialog. (Settings -> Configure KMail)") );


  // folder contents
  if ( !mIsLocalSystemFolder && kmkernel->iCalIface().isEnabled() ) {
    // Only do make this settable, if the IMAP resource is enabled
    // and it's not the personal folders (those must not be changed)
    ++row;
    label = new QLabel( i18n("&Folder contents:"), this );
    gl->addWidget( label, row, 0 );
    mContentsComboBox = new QComboBox( this );
    label->setBuddy( mContentsComboBox );
    gl->addWidget( mContentsComboBox, row, 1 );

    mContentsComboBox->insertItem( i18n( "Mail" ) );
    mContentsComboBox->insertItem( i18n( "Calendar" ) );
    mContentsComboBox->insertItem( i18n( "Contacts" ) );
    mContentsComboBox->insertItem( i18n( "Notes" ) );
    mContentsComboBox->insertItem( i18n( "Tasks" ) );
    mContentsComboBox->insertItem( i18n( "Journal" ) );
    if ( mDlg->folder() )
      mContentsComboBox->setCurrentItem( mDlg->folder()->storage()->contentsType() );
    connect ( mContentsComboBox, SIGNAL ( activated( int ) ),
              this, SLOT( slotFolderContentsSelectionChanged( int ) ) );
    if ( mDlg->folder()->isReadOnly() )
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
    mIncidencesForComboBox = new QComboBox( this );
    label->setBuddy( mIncidencesForComboBox );
    gl->addWidget( mIncidencesForComboBox, row, 1 );

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

    QWhatsThis::add( mIncidencesForComboBox, whatsThisForMyOwnFolders );
    mIncidencesForComboBox->insertItem( i18n( "Nobody" ) );
    mIncidencesForComboBox->insertItem( i18n( "Admins of This Folder" ) );
    mIncidencesForComboBox->insertItem( i18n( "All Readers of This Folder" ) );
    ++row;
    const QString whatsThisForReadOnlyFolders =
      i18n( "This setting allows you to disable alarms for folders shared by "
          "others. ");
    mAlarmsBlockedCheckBox = new QCheckBox( this );
    gl->addWidget( mAlarmsBlockedCheckBox, row, 0 );
    label = new QLabel( i18n( "Block free/&busy and alarms locally" ), this );
    gl->addWidget( label, row, 1 );
    label->setBuddy( mAlarmsBlockedCheckBox );
    QWhatsThis::add( mAlarmsBlockedCheckBox, whatsThisForReadOnlyFolders );

    if ( mDlg->folder()->storage()->contentsType() != KMail::ContentsTypeCalendar
      && mDlg->folder()->storage()->contentsType() != KMail::ContentsTypeTask ) {
        mIncidencesForComboBox->setEnabled( false );
        mAlarmsBlockedCheckBox->setEnabled( false );
    }
  }
  topLayout->addStretch( 100 ); // eat all superfluous space

  initializeWithValuesFromFolder( mDlg->folder() );
}

void FolderDiaGeneralTab::load()
{
  // Nothing here, all is done in the ctor
}

void FolderDiaGeneralTab::initializeWithValuesFromFolder( KMFolder* folder ) {
  if ( !folder )
    return;

  if ( !mIsLocalSystemFolder ) {
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
  }

  // folder identity
  mIdentityComboBox->setCurrentIdentity( folder->identity() );
  // ignore new mail
  mNotifyOnNewMailCheckBox->setChecked( !folder->ignoreNewMail() );

  const bool keepInFolder = !folder->isReadOnly() && folder->putRepliesInSameFolder();
  mKeepRepliesInSameFolderCheckBox->setChecked( keepInFolder );
  mKeepRepliesInSameFolderCheckBox->setDisabled( folder->isReadOnly() );

  if (folder->folderType() == KMFolderTypeImap)
  {
    KMFolderImap* imapFolder = static_cast<KMFolderImap*>(folder->storage());
    bool checked = imapFolder->includeInMailCheck();
    mNewMailCheckBox->setChecked(checked);
  }

  if ( mIncidencesForComboBox ) {
    KMFolderCachedImap* dimap = static_cast<KMFolderCachedImap *>( folder->storage() );
    mIncidencesForComboBox->setCurrentItem( dimap->incidencesFor() );
    mIncidencesForComboBox->setDisabled( mDlg->folder()->isReadOnly() );
  }
  if ( mAlarmsBlockedCheckBox ) {
    KMFolderCachedImap* dimap = static_cast<KMFolderCachedImap *>( folder->storage() );
    mAlarmsBlockedCheckBox->setChecked( dimap->alarmsBlocked() );
  }
}

//-----------------------------------------------------------------------------
void FolderDiaGeneralTab::slotFolderNameChanged( const QString& str )
{
  mDlg->enableButtonOK( !str.isEmpty() );
}

//-----------------------------------------------------------------------------
void FolderDiaGeneralTab::slotFolderContentsSelectionChanged( int )
{
  KMail::FolderContentsType type =
    static_cast<KMail::FolderContentsType>( mContentsComboBox->currentItem() );
  if( type != KMail::ContentsTypeMail && GlobalSettings::self()->hideGroupwareFolders() ) {
    QString message = i18n("You have configured this folder to contain groupware information "
        "and the general configuration option to hide groupware folders is "
        "set. That means that this folder will disappear once the configuration "
        "dialog is closed. If you want to remove the folder again, you will need "
        "to temporarily disable hiding of groupware folders to be able to see it.");
    KMessageBox::information( this, message );
  }
  const bool enable = type == KMail::ContentsTypeCalendar ||
                                          type == KMail::ContentsTypeTask;
  if ( mIncidencesForComboBox )
      mIncidencesForComboBox->setEnabled( enable );
  if ( mAlarmsBlockedCheckBox )
      mAlarmsBlockedCheckBox->setEnabled( enable );
}

//-----------------------------------------------------------------------------
bool FolderDiaGeneralTab::save()
{
  KMFolder* folder = mDlg->folder();
  folder->setIdentity( mIdentityComboBox->currentIdentity() );
  // set whoField
  if (mShowSenderReceiverComboBox->currentItem() == 1)
    folder->setUserWhoField("From");
  else if (mShowSenderReceiverComboBox->currentItem() == 2)
    folder->setUserWhoField("To");
  else
    folder->setUserWhoField("");

  folder->setIgnoreNewMail( !mNotifyOnNewMailCheckBox->isChecked() );
  folder->setPutRepliesInSameFolder( mKeepRepliesInSameFolderCheckBox->isChecked() );

  QString fldName, oldFldName;
  if ( !mIsLocalSystemFolder )
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


    // Update the tree iff new icon paths are different and not empty or if
    // useCustomIcons changed.
    if ( folder->useCustomIcons() != mIconsCheckBox->isChecked() ) {
      folder->setUseCustomIcons( mIconsCheckBox->isChecked() );
      // Reset icons, useCustomIcons was turned off.
      if ( !folder->useCustomIcons() ) {
        folder->setIconPaths( "", "" );
      }
    }
    if ( folder->useCustomIcons() &&
        (( mNormalIconButton->icon() != folder->normalIconPath() ) &&
         ( !mNormalIconButton->icon().isEmpty())) ||
        (( mUnreadIconButton->icon() != folder->unreadIconPath() ) &&
         ( !mUnreadIconButton->icon().isEmpty())) ) {
      folder->setIconPaths( mNormalIconButton->icon(), mUnreadIconButton->icon() );
    }

    // Set type field
    if ( mContentsComboBox ) {
      KMail::FolderContentsType type =
        static_cast<KMail::FolderContentsType>( mContentsComboBox->currentItem() );
      folder->storage()->setContentsType( type );
    }

    if ( folder->folderType() == KMFolderTypeCachedImap ) {
      KMFolderCachedImap* dimap = static_cast<KMFolderCachedImap *>( mDlg->folder()->storage() );
      if ( mIncidencesForComboBox ) {
        KMFolderCachedImap::IncidencesFor incfor = KMFolderCachedImap::IncForAdmins;
        incfor = static_cast<KMFolderCachedImap::IncidencesFor>( mIncidencesForComboBox->currentItem() );
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

void FolderDiaGeneralTab::slotChangeIcon( QString icon ) // can't use a const-ref here, due to KIconButton's signal
{
    mUnreadIconButton->setIcon( icon );
}

//----------------------------------------------------------------------------
KMail::FolderDiaTemplatesTab::FolderDiaTemplatesTab( KMFolderDialog* dlg,
                                                     QWidget* parent )
  : FolderDiaTab( parent, 0 ), mDlg( dlg )
{

  mIsLocalSystemFolder = mDlg->folder()->isSystemFolder() &&
       mDlg->folder()->folderType() != KMFolderTypeImap &&
       mDlg->folder()->folderType() != KMFolderTypeCachedImap;

  QVBoxLayout *topLayout = new QVBoxLayout( this, 0, KDialog::spacingHint() );

  mCustom = new QCheckBox( i18n("&Use custom message templates"), this );
  topLayout->addWidget( mCustom );

  mWidget = new TemplatesConfiguration( this , "folder-templates" );
  mWidget->setEnabled( false );
  topLayout->addWidget( mWidget );

  QHBoxLayout *btns = new QHBoxLayout( topLayout, KDialog::spacingHint() );
  mCopyGlobal = new KPushButton( i18n("&Copy global templates"), this );
  mCopyGlobal->setEnabled( false );
  btns->addWidget( mCopyGlobal );

  connect( mCustom, SIGNAL(toggled(bool)),
        mWidget, SLOT(setEnabled(bool)) );
  connect( mCustom, SIGNAL(toggled(bool)),
        mCopyGlobal, SLOT(setEnabled(bool)) );

  connect( mCopyGlobal, SIGNAL(clicked()),
        this, SLOT(slotCopyGlobal()) );

  initializeWithValuesFromFolder( mDlg->folder() );

  connect( mWidget, SIGNAL( changed() ),
           this, SLOT( slotEmitChanged( void ) ) );
}

void FolderDiaTemplatesTab::load()
{

}

void FolderDiaTemplatesTab::initializeWithValuesFromFolder( KMFolder* folder ) {
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
bool FolderDiaTemplatesTab::save()
{
  KMFolder* folder = mDlg->folder();

  QString fid = folder->idString();
  Templates t(fid);

  kdDebug() << "use custom templates for folder " << fid << ": " << mCustom->isChecked() << endl;
  t.setUseCustomTemplates(mCustom->isChecked());
  t.writeConfig();

  mWidget->saveToFolder(fid);

  return true;
}


void FolderDiaTemplatesTab::slotEmitChanged() {}

void FolderDiaTemplatesTab::slotCopyGlobal() {
  if ( mIdentity ) {
    mWidget->loadFromIdentity( mIdentity );
  }
  else {
    mWidget->loadFromGlobal();
  }
}
