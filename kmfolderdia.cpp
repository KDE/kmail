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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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
#include "kmkernel.h"
#include "kmcommands.h"
#include "mailinglist-magic.h"
#include "kmfoldertree.h"
#include "folderdiaacltab.h"
#include "kmailicalifaceimpl.h"
#include "kmmainwidget.h"
#include "globalsettings.h"

#include <keditlistbox.h>
#include <klineedit.h>
#include <klocale.h>
#include <knuminput.h>
#include <kmessagebox.h>
#include <kicondialog.h>
#include <kconfig.h>
#include <kdebug.h>
#include <klistview.h>

#include <qcheckbox.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qregexp.h>
#include <qlabel.h>
#include <qvbox.h>
#include <qwhatsthis.h>

#include <assert.h>
#include <qhbuttongroup.h>
#include <qradiobutton.h>

#include "kmfolderdia.moc"

using namespace KMail;

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
  mPositionInFolderList( 0 ),
  mIsNewFolder( aFolder == 0 )
{
  kdDebug(5006)<<"KMFolderDialog::KMFolderDialog()" << endl;

  if( !mFolder ) {
    // new folder can be subfolder of any other folder
    aParent->createFolderList(&mFolderNameList, &mFolders, true, true,
                              true, false, true, false);
  }
  else if( mFolder->folderType() != KMFolderTypeImap
           && mFolder->folderType() != KMFolderTypeCachedImap ) {
    // already existant local folder can only be moved locally
    aParent->createFolderList(&mFolderNameList, &mFolders, true, false,
                              false, false, true, false);
  }
  else {
    // already existant IMAP folder can't be moved, but we add all
    // IMAP folders so that the correct parent folder can be shown
    aParent->createFolderList(&mFolderNameList, &mFolders, false, true,
                              true, false, true, false);
  }

  mFolderNameList.prepend( i18n( "Local Folders" ) );

  if( mFolderDir ) {
    // search the parent folder of the folder
//    kdDebug(5006) << "search the parent folder of the folder" << endl;
    FolderList::ConstIterator it;
    int i = 1;
    for( it = mFolders.begin(); it != mFolders.end(); ++it, ++i ) {
//      kdDebug(5006) << "checking folder '" << (*it)->label() << "'" << endl;
      if( (*it)->child() == mFolderDir ) {
        mParentFolder = *it;
        mPositionInFolderList = i;
        break;
      }
    }
  }

  // Now create the folder list for the "move expired message to..." combo
  aParent->createFolderList(&mMoveToFolderNameList, &mMoveToFolderList, true, true,
                            true, false, true, true); // all except search folders


  FolderDiaTab* tab;
  QVBox* box;

  box = addVBoxPage( i18n("General") );
  tab = new FolderDiaGeneralTab( this, aName, box );
  addTab( tab );

  if ( !mFolder || !mFolder->noContent() )
  {
    box = addVBoxPage( i18n("Old Message Expiry") );
    tab = new FolderDiaExpiryTab( this, box );
    addTab( tab );

    box = addVBoxPage( i18n("Mailing List") );
    tab = new FolderDiaMailingListTab( this, box );
    addTab( tab );
  }

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
    mIsNewFolder = false; // so it's new anymore :)
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

//----------------------------------------------------------------------------
KMail::FolderDiaGeneralTab::FolderDiaGeneralTab( KMFolderDialog* dlg,
                                                 const QString& aName,
                                                 QWidget* parent, const char* name )
  : FolderDiaTab( parent, name ), mDlg( dlg )
{
  QLabel *label;

  QVBoxLayout *topLayout = new QVBoxLayout( this, 0, KDialog::spacingHint() );

  QGroupBox *fpGroup = new QGroupBox( i18n("Folder Position"), this, "fpGroup" );
  fpGroup->setColumnLayout( 0, Qt::Vertical );

  topLayout->addWidget( fpGroup );

  QHBoxLayout *hl = new QHBoxLayout( fpGroup->layout() );
  hl->setSpacing( 6 );

  label = new QLabel( i18n("&Name:"), fpGroup );
  hl->addWidget( label );

  mNameEdit = new KLineEdit( fpGroup );
  if( !mDlg->folder() )
    mNameEdit->setFocus();
  mNameEdit->setText( mDlg->folder() ? mDlg->folder()->label() : i18n("unnamed") );
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
  QGroupBox *iconGroup = new QGroupBox( i18n("Folder Icons"), this, "iconGroup" );
  iconGroup->setColumnLayout( 0,  Qt::Vertical );

  topLayout->addWidget( iconGroup );

  QVBoxLayout *ivl = new QVBoxLayout( iconGroup->layout() );
  ivl->setSpacing( 6 );

  QHBoxLayout *ihl = new QHBoxLayout( ivl );
  mIconsCheckBox = new QCheckBox( i18n("Use custom &icons"), iconGroup );
  mIconsCheckBox->setChecked( false );
  ihl->addWidget( mIconsCheckBox );
  ihl->addStretch( 2 );

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

  mMailboxTypeGroupBox = new QGroupBox( i18n("Folder Type"), this, "mMailboxTypeGroupBox" );
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
  mMailboxTypeComboBox->setEnabled( !mDlg->folder() );
  ml->addWidget( mMailboxTypeComboBox );
  ml->addStretch( 1 );

  mBelongsToComboBox->insertStringList( mDlg->folderNameList() );
  // we want to know if the activated changes
  connect( mBelongsToComboBox, SIGNAL(activated(int)), SLOT(slotUpdateItems(int)) );

  QGroupBox *idGroup = new QGroupBox(  i18n("Identity" ), this );
  idGroup->setColumnLayout( 0, Qt::Vertical );
  QHBoxLayout *idLayout = new QHBoxLayout(idGroup->layout());
  idLayout->setSpacing( 6 );
  topLayout->addWidget( idGroup );

  label = new QLabel( i18n("&Sender:"), idGroup );
  idLayout->addWidget( label );
  mIdentityComboBox = new KPIM::IdentityCombo( kmkernel->identityManager(), idGroup );
  label->setBuddy( mIdentityComboBox );
  idLayout->addWidget( mIdentityComboBox, 3 );

  QGroupBox* senderGroup = new QGroupBox( i18n("Show Sender/Receiver"), this, "senderGroup" );
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
  if (mDlg->folder()) whoField = mDlg->folder()->userWhoField();
  if (whoField.isEmpty()) mShowSenderReceiverComboBox->setCurrentItem(0);
  if (whoField == "From") mShowSenderReceiverComboBox->setCurrentItem(1);
  if (whoField == "To") mShowSenderReceiverComboBox->setCurrentItem(2);

  sl->addWidget( mShowSenderReceiverComboBox );
  sl->addStretch( 1 );

  if ( kmkernel->iCalIface().isEnabled() &&
       !kmkernel->iCalIface().isResourceImapFolder( mDlg->folder() ) ) {
    // Only do make this settable, if the IMAP resource is enabled
    // and it's not the personal folders (those must not be changed)
    QGroupBox *typeGroup = new QGroupBox( i18n("Contents" ), this );
    typeGroup->setColumnLayout( 0, Qt::Vertical );
    QHBoxLayout *typeLayout = new QHBoxLayout( typeGroup->layout() );
    typeLayout->setSpacing( 6 );
    topLayout->addWidget( typeGroup );
    label = new QLabel( i18n("&Folder contents:"), typeGroup );
    typeLayout->addWidget( label );
    mContentsComboBox = new QComboBox( typeGroup );
    label->setBuddy( mContentsComboBox );
    typeLayout->addWidget( mContentsComboBox, 3 );

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
  } else {
    mContentsComboBox = 0;
  }

  // Kolab incidences-for annotation.
  // Show incidences-for combobox if the contents type can be changed (new folder),
  // or if it's set to calendar or task (existing folder)
  if ( ( GlobalSettings::theIMAPResourceStorageFormat() ==
         GlobalSettings::EnumTheIMAPResourceStorageFormat::XML ) &&
       ( mContentsComboBox ||
         ( mDlg->folder() && ( mDlg->folder()->storage()->contentsType() == KMail::ContentsTypeCalendar
                               || mDlg->folder()->storage()->contentsType() == KMail::ContentsTypeTask ) ) ) ) {
    mIncidencesForGroup = new QGroupBox( i18n("Relevance of Events and Tasks" ), this );
    mIncidencesForGroup->setColumnLayout( 0, Qt::Vertical );
    QHBoxLayout *relevanceLayout = new QHBoxLayout( mIncidencesForGroup->layout() );
    relevanceLayout->setSpacing( 6 );
    topLayout->addWidget( mIncidencesForGroup );

    QLabel* label = new QLabel( i18n( "Generate free/&busy and activate alarms for:" ), mIncidencesForGroup );
    relevanceLayout->addWidget( label );
    mIncidencesForComboBox = new QComboBox( mIncidencesForGroup );
    label->setBuddy( mIncidencesForComboBox );
    relevanceLayout->addWidget( mIncidencesForComboBox, 3 );

    QWhatsThis::add( mIncidencesForComboBox,
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
                           "since it is not known who will go to those events." ) );

    mIncidencesForComboBox->insertItem( i18n( "Nobody" ) );
    mIncidencesForComboBox->insertItem( i18n( "Admins of this folder" ) );
    mIncidencesForComboBox->insertItem( i18n( "All readers of this folder" ) );

    //connect ( mIncidencesForComboBox, SIGNAL ( activated( int ) ),
    //          this, SLOT( slotIncidencesForChanged( int ) ) );
    if ( mContentsComboBox && mDlg->folder()&& mIncidencesForGroup ) {
      KMail::FolderContentsType type = mDlg->folder()->storage()->contentsType();
      mIncidencesForGroup->setEnabled( type == KMail::ContentsTypeCalendar ||
                                       type == KMail::ContentsTypeTask );
    }

  } else {
    mIncidencesForComboBox = 0;
    mIncidencesForGroup = 0;
  }

  // should this folder be included in new-mail-checks?
  QGroupBox* newmailGroup = new QGroupBox( i18n("Check for New Mail"), this, "newmailGroup" );
  newmailGroup->setColumnLayout( 0,  Qt::Vertical );
  topLayout->addWidget( newmailGroup );

  QHBoxLayout *nml = new QHBoxLayout( newmailGroup->layout() );
  nml->setSpacing( 6 );
  mNewMailCheckBox = new QCheckBox( i18n("Include in check" ), newmailGroup );
  // default is on
  mNewMailCheckBox->setChecked(true);
  nml->addWidget( mNewMailCheckBox );
  nml->addStretch( 1 );

  // should new mail in this folder be ignored?
  QGroupBox* notifyGroup = new QGroupBox( i18n("New Mail Notification"), this,
                                          "notifyGroup" );
  notifyGroup->setColumnLayout( 0, Qt::Vertical );
  topLayout->addWidget( notifyGroup );

  QHBoxLayout *hbl = new QHBoxLayout( notifyGroup->layout() );
  hbl->setSpacing( KDialog::spacingHint() );
  mIgnoreNewMailCheckBox =
    new QCheckBox( i18n("Ignore new mail in this folder" ), notifyGroup );
  QWhatsThis::add( mIgnoreNewMailCheckBox,
                   i18n( "Check this option if you do not want to be notified "
                         "about new mail that is moved to this folder; this "
                         "is useful, for example, for ignoring spam." ) );
  hbl->addWidget( mIgnoreNewMailCheckBox );
  hbl->addStretch( 1 );

  // should replies to mails in this folder be kept in this same folder?
  QGroupBox* replyGroup = new QGroupBox( i18n("Reply Handling"), this,
                                             "replyGroup" );
  replyGroup->setColumnLayout( 0, Qt::Vertical );
  topLayout->addWidget( replyGroup );

  hbl = new QHBoxLayout( replyGroup->layout() );
  hbl->setSpacing( KDialog::spacingHint() );
  mKeepRepliesInSameFolderCheckBox =
    new QCheckBox( i18n("Keep replies in this folder" ), replyGroup );
  QWhatsThis::add( mKeepRepliesInSameFolderCheckBox,
                   i18n( "Check this option if you want replies you write "
                         "to mails in this folder to be put in this same folder "
                         "after sending, instead of in the configured sent-mail folder." ) );
  hbl->addWidget( mKeepRepliesInSameFolderCheckBox );
  hbl->addStretch( 1 );

  topLayout->addStretch( 100 ); // eat all superfluous space

  KMFolder* parentFolder = mDlg->parentFolder();

  if ( parentFolder ) {
    mBelongsToComboBox->setCurrentItem( mDlg->positionInFolderList() );
    slotUpdateItems( mDlg->positionInFolderList() );
  }

  if ( mDlg->folder() ) {
    // existing folder
    initializeWithValuesFromFolder( mDlg->folder() );

    // mailbox folder type
    switch ( mDlg->folder()->folderType() ) {
      case KMFolderTypeSearch:
        mMailboxTypeComboBox->setCurrentItem( 2 );
        belongsToLabel->hide();
        mBelongsToComboBox->hide();
        newmailGroup->hide();
        break;
      case KMFolderTypeMaildir:
        mMailboxTypeComboBox->setCurrentItem( 1 );
        newmailGroup->hide();
        break;
      case KMFolderTypeMbox:
        mMailboxTypeComboBox->setCurrentItem( 0 );
        newmailGroup->hide();
        break;
      case KMFolderTypeImap:
        belongsToLabel->setEnabled( false );
        mBelongsToComboBox->setEnabled( false );
        mMailboxTypeGroupBox->hide();
        break;
      case KMFolderTypeCachedImap:
        belongsToLabel->setEnabled( false );
        mBelongsToComboBox->setEnabled( false );
        mMailboxTypeGroupBox->hide();
        newmailGroup->hide();
        break;
      default: ;
    }
  }

  else if ( parentFolder ) {
    // new folder
    initializeWithValuesFromFolder( parentFolder );

    // mailbox folder type
    switch ( parentFolder->folderType() ) {
      case KMFolderTypeSearch:
        mMailboxTypeComboBox->setCurrentItem( 2 );
        belongsToLabel->hide();
        mBelongsToComboBox->hide();
        newmailGroup->hide();
        break;
      case KMFolderTypeMaildir:
        newmailGroup->hide();
        break;
      case KMFolderTypeMbox:
        newmailGroup->hide();
        break;
      case KMFolderTypeImap:
        mMailboxTypeGroupBox->hide();
        break;
      case KMFolderTypeCachedImap:
        mMailboxTypeGroupBox->hide();
        newmailGroup->hide();
        break;
      default: ;
    }
  }

  // Musn't be able to edit details for a system folder.
  // Make sure we don't bomb out if there isn't a folder
  // object yet (i.e. just about to create new folder).

  if ( mDlg->folder() && mDlg->folder()->isSystemFolder() &&
       mDlg->folder()->folderType() != KMFolderTypeImap &&
       mDlg->folder()->folderType() != KMFolderTypeCachedImap ) {
    fpGroup->hide();
    iconGroup->hide();
    mMailboxTypeGroupBox->hide();
    idGroup->hide();
  }
}

void FolderDiaGeneralTab::load()
{
  // Nothing here, all is done in the ctor
}

void FolderDiaGeneralTab::initializeWithValuesFromFolder( KMFolder* folder ) {
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

  // folder identity
  mIdentityComboBox->setCurrentIdentity( folder->identity() );

  // ignore new mail
  mIgnoreNewMailCheckBox->setChecked( folder->ignoreNewMail() );

  mKeepRepliesInSameFolderCheckBox->setChecked( folder->putRepliesInSameFolder() );

  if (folder->folderType() == KMFolderTypeImap)
  {
    KMFolderImap* imapFolder = static_cast<KMFolderImap*>(folder->storage());
    bool checked = imapFolder->includeInMailCheck();
    mNewMailCheckBox->setChecked(checked);
  }

  bool isImap = /*folder->folderType() == KMFolderTypeImap ||*/ folder->folderType() == KMFolderTypeCachedImap;
  if ( mIncidencesForGroup ) {
    if ( !isImap )
      mIncidencesForGroup->hide();
    else {
      KMFolderCachedImap* dimap = static_cast<KMFolderCachedImap *>( folder->storage() );
      mIncidencesForComboBox->setCurrentItem( dimap->incidencesFor() );
    }
  }
}

//-----------------------------------------------------------------------------
void FolderDiaGeneralTab::slotFolderNameChanged( const QString& str )
{
  mDlg->enableButtonOK( !str.isEmpty() );
}

//-----------------------------------------------------------------------------
void FolderDiaGeneralTab::slotUpdateItems ( int current )
{
  KMFolder* selectedFolder = 0;
  // check if the index is valid (the top level has no entrance in the mDlg->folders())
  if (current > 0) selectedFolder = *mDlg->folders().at(current - 1);
  if (selectedFolder && (selectedFolder->folderType() == KMFolderTypeImap ||
			 selectedFolder->folderType() == KMFolderTypeCachedImap))
  {
    // deactivate stuff that is not available for imap
    mMailboxTypeGroupBox->setEnabled( false );
  } else {
    // activate it
    mMailboxTypeGroupBox->setEnabled( true );
  }
}

//-----------------------------------------------------------------------------
void FolderDiaGeneralTab::slotFolderContentsSelectionChanged( int )
{
  KMail::FolderContentsType type =
    static_cast<KMail::FolderContentsType>( mContentsComboBox->currentItem() );
  if( type != KMail::ContentsTypeMail && GlobalSettings::hideGroupwareFolders() ) {
    QString message = i18n("You have configured this folder to contain groupware information "
        "and the general configuration option to hide groupware folders is "
        "set. That means that this folder will disappear once the configuration "
        "dialog is closed. If you want to remove the folder again, you will need "
        "to temporarily disable hiding of groupware folders to be able to see it.");
    KMessageBox::information( this, message );
  }

  if ( mIncidencesForGroup )
    mIncidencesForGroup->setEnabled( type == KMail::ContentsTypeCalendar ||
                                     type == KMail::ContentsTypeTask );
}

//-----------------------------------------------------------------------------
bool FolderDiaGeneralTab::save()
{
  QString oldFldName;
  if( !mDlg->isNewFolder() ) oldFldName = mDlg->folder()->name();
  QString fldName = !mNameEdit->text().isEmpty() ? mNameEdit->text() : oldFldName;
  if ( mDlg->parentFolder() &&
       mDlg->parentFolder()->folderType() != KMFolderTypeImap &&
       mDlg->parentFolder()->folderType() != KMFolderTypeCachedImap )
    fldName.remove('/');
  fldName.remove(QRegExp("^\\.*"));
  if (fldName.isEmpty()) fldName = i18n("unnamed");

  // moving of IMAP folders is not yet supported
  if ( mDlg->isNewFolder() || !mDlg->folder()->isSystemFolder() )
  {
    QString acctName;
    KMFolderDir *selectedFolderDir = &(kmkernel->folderMgr()->dir());
    KMFolder *selectedFolder = 0;
    int curFolder = mBelongsToComboBox->currentItem();

    if (mMailboxTypeComboBox->currentItem() == 2) {
      selectedFolderDir = &(kmkernel->searchFolderMgr()->dir());
    }
    else if (curFolder != 0)
    {
      selectedFolder = *mDlg->folders().at(curFolder - 1);
      selectedFolderDir = selectedFolder->createChildFolder();
    }

    QString message = i18n( "<qt>Failed to create folder <b>%1</b>, folder already exists.</qt>" ).arg(fldName);
    if( selectedFolderDir->hasNamedFolder( fldName )
        && ( !( mDlg->folder()
                && ( selectedFolderDir == mDlg->folder()->parent() )
                && ( mDlg->folder()->name() == fldName ) ) ) )
    {
      KMessageBox::error( this, message );
      return false;
    }

    message = i18n( "<qt>Cannot move folder <b>%1</b> into a subfolder below itself.</qt>" ).arg(fldName);
    KMFolderDir* folderDir = selectedFolderDir;


    // Buggy?
    if( mDlg->folder() && mDlg->folder()->child() ) {
      while( ( folderDir != &kmkernel->folderMgr()->dir() )
             && ( folderDir != mDlg->folder()->parent() ) ) {
        if( folderDir->findRef( mDlg->folder() ) != -1 ) {
          KMessageBox::error( this, message );
          return false;
        }
        folderDir = folderDir->parent();
      }
    }
    // End buggy?


    if( mDlg->folder() && mDlg->folder()->child() && selectedFolderDir &&
        ( selectedFolderDir->path().find( mDlg->folder()->child()->path() + "/" ) == 0 ) ) {
      KMessageBox::error( this, message );
      return false;
    }

    if( mDlg->folder() && mDlg->folder()->child()
        && ( selectedFolderDir == mDlg->folder()->child() ) ) {
      KMessageBox::error( this, message );
      return false;
    }

    if( mDlg->isNewFolder() ) {

      if ( fldName.find( '/' ) != -1 ) {
        KMessageBox::error( this, i18n( "Folder names can't contain the / (slash) character, please choose another folder name" ) );
        return false;
      }
      message = i18n( "<qt>Failed to create folder <b>%1</b>."
            "</qt> " ).arg(fldName);

      if (selectedFolder && selectedFolder->folderType() == KMFolderTypeImap)
      {
        KMFolder *newFolder = kmkernel->imapFolderMgr()->createFolder( fldName, FALSE, KMFolderTypeImap, selectedFolderDir );
        if ( newFolder ) {
          mDlg->setFolder( newFolder );
          KMFolderImap* selectedStorage = static_cast<KMFolderImap*>(selectedFolder->storage());
          selectedStorage->createFolder(fldName); // create it on the server
          static_cast<KMFolderImap*>(mDlg->folder()->storage())->setAccount( selectedStorage->account() );
        } else {
          KMessageBox::error( this, message );
          return false;
        }
      } else if (selectedFolder && selectedFolder->folderType() == KMFolderTypeCachedImap){
        KMFolder *newFolder = kmkernel->dimapFolderMgr()->createFolder( fldName, FALSE, KMFolderTypeCachedImap, selectedFolderDir );
        if ( newFolder ) {
          mDlg->setFolder( newFolder );
          KMFolderCachedImap* selectedStorage = static_cast<KMFolderCachedImap*>(selectedFolder->storage());
          KMFolderCachedImap* newStorage = static_cast<KMFolderCachedImap*>(mDlg->folder()->storage());
          newStorage->initializeFrom( selectedStorage );
        } else {
          KMessageBox::error( this, message );
          return false;
        }
      } else if (mMailboxTypeComboBox->currentItem() == 2) {
        KMFolder *folder = kmkernel->searchFolderMgr()->createFolder(fldName, FALSE, KMFolderTypeSearch, &kmkernel->searchFolderMgr()->dir() );
        if ( folder ) {
          mDlg->setFolder( folder );
        } else {
          KMessageBox::error( this, message );
          return false;
        }
      } else if (mMailboxTypeComboBox->currentItem() == 1) {
        KMFolder *folder = kmkernel->folderMgr()->createFolder(fldName, FALSE, KMFolderTypeMaildir, selectedFolderDir );
        if ( folder ) {
          mDlg->setFolder( folder );
        } else {
          KMessageBox::error( this, message );
          return false;
        }

      } else {
        KMFolder *folder = kmkernel->folderMgr()->createFolder(fldName, FALSE, KMFolderTypeMbox, selectedFolderDir );
        if ( folder ) {
          mDlg->setFolder( folder );
        } else {
          KMessageBox::error( this, message );
          return false;
        }

      }
    }
    else if( mDlg->folder()->parent() != selectedFolderDir )
    {
      if( mDlg->folder()->folderType() == KMFolderTypeCachedImap ) {
        QString message = i18n("Moving IMAP folders is not supported");
        KMessageBox::error( this, message );
      } else {
        mDlg->folder()->rename(fldName, selectedFolderDir );
        kmkernel->folderMgr()->contentsChanged();
      }
    }
  }
  // Renamed an existing folder? We don't check for oldName == newName on 
  // purpose here. The folder might be pending renaming on the next dimap
  // sync already, in which case the old name would still be around and
  // something like Calendar -> CalendarFoo -> Calendar inbetween syncs would
  // fail. Therefor let the folder sort it out itself, whether the rename is
  // a noop or not.
  if ( !mDlg->isNewFolder() ) {
    mDlg->folder()->rename(fldName);
  }

  KMFolder* folder = mDlg->folder();
  if( folder ) {
    folder->setIdentity( mIdentityComboBox->currentIdentity() );

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
        // set whoField
    if (mShowSenderReceiverComboBox->currentItem() == 1)
      folder->setUserWhoField("From");
    else if (mShowSenderReceiverComboBox->currentItem() == 2)
      folder->setUserWhoField("To");
    else
      folder->setUserWhoField(QString::null);

    // Set type field
    if ( mContentsComboBox ) {
      KMail::FolderContentsType type =
        static_cast<KMail::FolderContentsType>( mContentsComboBox->currentItem() );
      folder->storage()->setContentsType( type );
    }

    if ( mIncidencesForComboBox && folder->folderType() == KMFolderTypeCachedImap ) {
      KMFolderCachedImap::IncidencesFor incfor =
        static_cast<KMFolderCachedImap::IncidencesFor>( mIncidencesForComboBox->currentItem() );
      KMFolderCachedImap* dimap = static_cast<KMFolderCachedImap *>( mDlg->folder()->storage() );
      if ( dimap->incidencesFor() != incfor ) {
        dimap->setIncidencesFor( incfor );
        dimap->writeConfig();
      }
    }

    folder->setIgnoreNewMail( mIgnoreNewMailCheckBox->isChecked() );

    folder->setPutRepliesInSameFolder( mKeepRepliesInSameFolderCheckBox->isChecked() );

    if( folder->folderType() == KMFolderTypeImap )
    {
      KMFolderImap* imapFolder = static_cast<KMFolderImap*>( folder->storage() );
      imapFolder->setIncludeInMailCheck(
          mNewMailCheckBox->isChecked() );
    }
    // make sure everything is on disk, connected slots will call readConfig()
    // when creating a new folder.
    folder->storage()->writeConfig();
  }
  kmkernel->folderMgr()->contentsChanged();

  if ( mDlg->isNewFolder() && folder )
    folder->close();
  return true;
}

void FolderDiaGeneralTab::slotChangeIcon( QString icon ) // can't use a const-ref here, due to KIconButton's signal
{
    mUnreadIconButton->setIcon( icon );
}

//----------------------------------------------------------------------------
KMail::FolderDiaExpiryTab::FolderDiaExpiryTab( KMFolderDialog* dlg,
                                               QWidget* parent,
                                               const char* name )
  : FolderDiaTab( parent, name ), mDlg( dlg )
{
  QLabel *label;

  QVBoxLayout *topLayout = new QVBoxLayout( this, 0, KDialog::spacingHint() );

  // Checkbox for setting whether expiry is enabled on this folder.
  mExpireFolderCheckBox =
    new QCheckBox( i18n("E&xpire old messages in this folder"), this );
  QObject::connect( mExpireFolderCheckBox, SIGNAL( toggled( bool ) ),
                    this, SLOT( slotExpireFolder( bool ) ) );
  topLayout->addWidget( mExpireFolderCheckBox );

  QGridLayout *expLayout = new QGridLayout( topLayout );

  // Expiry time for read documents.
  label = new QLabel( i18n("Expire &read email after:"), this );
  label->setEnabled( false );
  QObject::connect( mExpireFolderCheckBox, SIGNAL( toggled( bool ) ),
		    label, SLOT( setEnabled( bool ) ) );
  expLayout->addWidget( label, 1, 0 );

  mReadExpiryTimeNumInput = new KIntNumInput( this );
  mReadExpiryTimeNumInput->setRange( 1, 500, 1, false );
  label->setBuddy( mReadExpiryTimeNumInput );
  expLayout->addWidget( mReadExpiryTimeNumInput, 1, 1 );

  mReadExpiryUnitsComboBox = new QComboBox( this );
  mReadExpiryUnitsComboBox->insertItem( i18n("Never") );
  mReadExpiryUnitsComboBox->insertItem( i18n("Day(s)") );
  mReadExpiryUnitsComboBox->insertItem( i18n("Week(s)") );
  mReadExpiryUnitsComboBox->insertItem( i18n("Month(s)") );
  expLayout->addWidget( mReadExpiryUnitsComboBox, 1, 2 );
  connect( mReadExpiryUnitsComboBox, SIGNAL( activated( int ) ),
           this, SLOT( slotReadExpiryUnitChanged( int ) ) );

  // Expiry time for unread documents.
  label = new QLabel( i18n("Expire unr&ead email after:"), this );
  label->setEnabled(false);
  QObject::connect( mExpireFolderCheckBox, SIGNAL( toggled( bool ) ),
		    label, SLOT( setEnabled( bool ) ) );
  expLayout->addWidget( label, 2, 0 );

  mUnreadExpiryTimeNumInput = new KIntNumInput( this );
  mUnreadExpiryTimeNumInput->setRange( 1, 500, 1, false );
  label->setBuddy( mUnreadExpiryTimeNumInput );
  expLayout->addWidget( mUnreadExpiryTimeNumInput, 2, 1 );

  mUnreadExpiryUnitsComboBox = new QComboBox( this );
  mUnreadExpiryUnitsComboBox->insertItem( i18n("Never") );
  mUnreadExpiryUnitsComboBox->insertItem( i18n("Day(s)") );
  mUnreadExpiryUnitsComboBox->insertItem( i18n("Week(s)") );
  mUnreadExpiryUnitsComboBox->insertItem( i18n("Month(s)") );
  expLayout->addWidget( mUnreadExpiryUnitsComboBox, 2, 2 );
  connect( mUnreadExpiryUnitsComboBox, SIGNAL( activated( int ) ),
           this, SLOT( slotUnreadExpiryUnitChanged( int ) ) );

  expLayout->setColStretch( 3, 100 );

  // delete or archive old messages
  QButtonGroup* radioBG = new QButtonGroup( this );
  radioBG->hide(); // just for the exclusive behavior
  mExpireActionDelete = new QRadioButton( i18n( "Delete old messages" ),
                                          this );
  radioBG->insert( mExpireActionDelete );
  topLayout->addWidget( mExpireActionDelete );

  QHBoxLayout *hbl = new QHBoxLayout( topLayout );
  mExpireActionMove = new QRadioButton( i18n( "Move old messages to:" ),
                                        this );
  radioBG->insert( mExpireActionMove );
  hbl->addWidget( mExpireActionMove );
  mExpireToFolderComboBox = new QComboBox( this );
  hbl->addWidget( mExpireToFolderComboBox );
  mExpireToFolderComboBox->insertStringList( mDlg->moveToFolderNameList() );
  hbl->addStretch( 100 );

  topLayout->addStretch( 100 ); // eat all superfluous space

  connect( mExpireFolderCheckBox, SIGNAL( toggled( bool ) ),
           mExpireActionDelete, SLOT( setEnabled( bool ) ) );
  connect( mExpireFolderCheckBox, SIGNAL( toggled( bool ) ),
           mExpireActionMove, SLOT( setEnabled( bool ) ) );
  connect( mExpireFolderCheckBox, SIGNAL( toggled( bool ) ),
           mExpireToFolderComboBox, SLOT( setEnabled( bool ) ) );
}

void FolderDiaExpiryTab::load()
{
  KMFolder* folder = mDlg->folder();
  if( folder ) {
    // settings for automatic deletion of old messages
    mExpireFolderCheckBox->setChecked( folder->isAutoExpire() );
    // Legal values for units are 0=never, 1=days, 2=weeks, 3=months.
    if( folder->getReadExpireUnits() >= 0
        && folder->getReadExpireUnits() < expireMaxUnits ) {
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
    if ( folder->expireAction() == KMFolder::ExpireDelete )
      mExpireActionDelete->setChecked( true );
    else
      mExpireActionMove->setChecked( true );
    QString destFolderID = folder->expireToFolderId();
    if ( !destFolderID.isEmpty() ) {
      KMFolderDialog::FolderList moveToFolderList = mDlg->moveToFolderList();
      KMFolder* destFolder = kmkernel->findFolderById( destFolderID );
      int pos = moveToFolderList.findIndex( QGuardedPtr<KMFolder>( destFolder ) );
      if ( pos > -1 )
        mExpireToFolderComboBox->setCurrentItem( pos );
    }
  } else { // new folder, use default values
    mReadExpiryTimeNumInput->setValue( 7 );
    mUnreadExpiryTimeNumInput->setValue(28);
    mExpireActionDelete->setChecked( true );
  }
  if( !folder || !folder->isAutoExpire() ) {
    mReadExpiryTimeNumInput->setEnabled( false );
    mReadExpiryUnitsComboBox->setEnabled( false );
    mUnreadExpiryTimeNumInput->setEnabled( false );
    mUnreadExpiryUnitsComboBox->setEnabled( false );
    mExpireActionDelete->setEnabled( false );
    mExpireActionMove->setEnabled( false );
    mExpireToFolderComboBox->setEnabled( false );
  }
  else {
    // disable the number fields if "Never" is selected
    mReadExpiryTimeNumInput->setEnabled( mReadExpiryUnitsComboBox->currentItem() != 0 );
    mUnreadExpiryTimeNumInput->setEnabled( mUnreadExpiryUnitsComboBox->currentItem() != 0 );
  }
}

//-----------------------------------------------------------------------------
bool FolderDiaExpiryTab::save()
{
  KMFolder* folder = mDlg->folder();
  if( !folder )
    return true;

  // Settings for auto expiry of old email messages.
  folder->setAutoExpire( mExpireFolderCheckBox->isChecked() );
  folder->setUnreadExpireAge( mUnreadExpiryTimeNumInput->value() );
  folder->setReadExpireAge( mReadExpiryTimeNumInput->value() );
  folder->setUnreadExpireUnits( static_cast<ExpireUnits>( mUnreadExpiryUnitsComboBox->currentItem() ) );
  folder->setReadExpireUnits( static_cast<ExpireUnits>( mReadExpiryUnitsComboBox->currentItem() ) );
  if ( mExpireActionDelete->isChecked() )
    folder->setExpireAction( KMFolder::ExpireDelete );
  else
    folder->setExpireAction( KMFolder::ExpireMove );
  KMFolder* expireToFolder =
    mDlg->moveToFolderList()[mExpireToFolderComboBox->currentItem()];
  if ( expireToFolder )
    folder->setExpireToFolderId( expireToFolder->idString() );

  return true;
}

/**
 * Called when the 'auto expire' toggle is clicked.
 * Enables/disables all widgets related to this.
 */
void FolderDiaExpiryTab::slotExpireFolder(bool expire)
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
void FolderDiaExpiryTab::slotReadExpiryUnitChanged( int value )
{
  // disable the number field if "Never" is selected
  mReadExpiryTimeNumInput->setEnabled( value != 0 );
}


/**
 * Enable/disable the number field if appropriate
 */
void FolderDiaExpiryTab::slotUnreadExpiryUnitChanged( int value )
{
  // disable the number field if "Never" is selected
  mUnreadExpiryTimeNumInput->setEnabled( value != 0 );
}

//----------------------------------------------------------------------------
FolderDiaMailingListTab::FolderDiaMailingListTab( KMFolderDialog* dlg,
                                                  QWidget* parent, const char* name )
  : FolderDiaTab( parent, name ), mDlg( dlg )
{
  QLabel* label;
  mLastItem = 0;

  QVBoxLayout *topLayout = new QVBoxLayout( this, 0, KDialog::spacingHint(),
                                            "topLayout" );

  QGroupBox *mlGroup = new QGroupBox( i18n("Associated Mailing List" ), this );
  mlGroup->setColumnLayout( 0,  Qt::Vertical );
  QVBoxLayout *groupLayout = new QVBoxLayout( mlGroup->layout() );
  topLayout->addWidget( mlGroup );

  mHoldsMailingList = new QCheckBox( i18n("&Folder holds a mailing list"), mlGroup );
  QObject::connect( mHoldsMailingList, SIGNAL(toggled(bool)),
                    SLOT(slotHoldsML(bool)) );
  groupLayout->addWidget( mHoldsMailingList );

  groupLayout->addSpacing( 10 );

  mDetectButton = new QPushButton( i18n("Detect Automatically"), mlGroup );
  mDetectButton->setEnabled( false );
  QObject::connect( mDetectButton, SIGNAL(pressed()), SLOT(slotDetectMailingList()) );
  groupLayout->addWidget( mDetectButton, 0, Qt::AlignHCenter );

  groupLayout->addSpacing( 10 );

  QHBoxLayout *handlerLayout = new QHBoxLayout( groupLayout );
  //FIXME: add QWhatsThis
  label = new QLabel( i18n("Preferred handler: "), mlGroup );
  QObject::connect( mHoldsMailingList, SIGNAL(toggled(bool)),
		    label, SLOT(setEnabled(bool)) );
  handlerLayout->addWidget( label, 0, Qt::AlignCenter );
  mMLHandlerCombo = new QComboBox( mlGroup );
  mMLHandlerCombo->insertItem( i18n("KMail"), MailingList::KMail );
  mMLHandlerCombo->insertItem( i18n("Browser"), MailingList::Browser );
  mMLHandlerCombo->setEnabled( false );
  handlerLayout->addWidget( mMLHandlerCombo, 0, Qt::AlignCenter );
  QObject::connect( mMLHandlerCombo, SIGNAL(activated(int)),
                    SLOT(slotMLHandling(int)) );
  label->setBuddy( mMLHandlerCombo );

  //groupLayout->addSpacing( 10 );

  QVBoxLayout *idLayout = new QVBoxLayout( groupLayout );
  label = new QLabel( i18n("<b>Mailing list description: </b>"), mlGroup );
  label->setEnabled( false );
  QObject::connect( mHoldsMailingList, SIGNAL(toggled(bool)),
		    label, SLOT(setEnabled(bool)) );
  idLayout->addWidget( label, 0 );
  mMLId = new QLabel( label, "", mlGroup );
  idLayout->addWidget( mMLId, 0 );
  mMLId->setEnabled( false );

  QGridLayout *mlLayout = new QGridLayout( groupLayout );
  mlLayout->setSpacing( 6 );
  // mlLayout->setColStretch(0, 1);
  // mlLayout->setColStretch(1, 100);

  label = new QLabel( i18n("&Address type:"), mlGroup );
  label->setEnabled(false);
  QObject::connect( mHoldsMailingList, SIGNAL(toggled(bool)),
		    label, SLOT(setEnabled(bool)) );
  mlLayout->addWidget( label, 0, 0, Qt::AlignTop );
  mAddressCombo = new QComboBox( mlGroup );
  label->setBuddy( mAddressCombo );
  mlLayout->addWidget( mAddressCombo, 0, 1, Qt::AlignTop );
  mAddressCombo->setEnabled( false );

  //FIXME: if the mailing list actions have either KAction's or toolbar buttons
  //       associated with them - remove this button since it's really silly
  //       here
  QPushButton *handleButton = new QPushButton( i18n( "Invoke Handler" ), mlGroup );
  handleButton->setEnabled( false );
  if( mDlg->folder())
  {
  	QObject::connect( mHoldsMailingList, SIGNAL(toggled(bool)),
			    handleButton, SLOT(setEnabled(bool)) );
	  QObject::connect( handleButton, SIGNAL(clicked()),
    	                SLOT(slotInvokeHandler()) );
  }
  mlLayout->addWidget( handleButton, 0, 2, Qt::AlignTop );

  mEditList = new KEditListBox( mlGroup );
  mEditList->setEnabled( false );
  mlLayout->addMultiCellWidget( mEditList, 1, 2, 0, 3, Qt::AlignTop );

  QStringList el;

  //Order is important because the activate handler and fillMLFromWidgets
  //depend on it
  el << i18n( "Post to List" )
     << i18n( "Subscribe to List" )
     << i18n( "Unsubscribe from List" )
     << i18n( "List Archives" )
     << i18n( "List Help" );
  mAddressCombo->insertStringList( el );
  QObject::connect( mAddressCombo, SIGNAL(activated(int)),
                    SLOT(slotAddressChanged(int)) );
}

void FolderDiaMailingListTab::load()
{
  if (mDlg->folder()) mMailingList = mDlg->folder()->mailingList();
  mMLId->setText( (mMailingList.id().isEmpty() ? i18n("Not available") : mMailingList.id()) );
  mMLHandlerCombo->setCurrentItem( mMailingList.handler() );
  mEditList->insertStringList( mMailingList.postURLS().toStringList() );

  mAddressCombo->setCurrentItem( mLastItem );
  mHoldsMailingList->setChecked( mDlg->folder() && mDlg->folder()->isMailingListEnabled() );
}

//-----------------------------------------------------------------------------
bool FolderDiaMailingListTab::save()
{
  KMFolder* folder = mDlg->folder();
  if( folder )
  {
    // settings for mailingList
    folder->setMailingListEnabled( mHoldsMailingList && mHoldsMailingList->isChecked() );
    fillMLFromWidgets();
    folder->setMailingList( mMailingList );
  }
  return true;
}

//----------------------------------------------------------------------------
void FolderDiaMailingListTab::slotHoldsML( bool holdsML )
{
  mMLHandlerCombo->setEnabled( holdsML );
  if ( mDlg->folder() && mDlg->folder()->count() )
    mDetectButton->setEnabled( holdsML );
  mAddressCombo->setEnabled( holdsML );
  mEditList->setEnabled( holdsML );
  mMLId->setEnabled( holdsML );
}

//----------------------------------------------------------------------------
void FolderDiaMailingListTab::slotDetectMailingList()
{
  if ( !mDlg->folder() ) return; // in case the folder was just created
  int num = mDlg->folder()->count();

  kdDebug(5006)<<k_funcinfo<<" Detecting mailing list"<<endl;

  // first try the currently selected message
  KMFolderTree *folderTree = static_cast<KMFolderTree *>( mDlg->parent() );
  int curMsgIdx = folderTree->mainWidget()->headers()->currentItemIndex();
  if ( curMsgIdx > 0 ) {
    KMMessage *mes = mDlg->folder()->getMsg( curMsgIdx );
    if ( mes )
      mMailingList = MailingList::detect( mes );
  }

  // next try the 5 most recently added messages
  if ( !( mMailingList.features() & MailingList::Post ) ) {
    const int maxchecks = 5;
    for( int i = --num; i > num-maxchecks; --i ) {
      KMMessage *mes = mDlg->folder()->getMsg( i );
      if ( !mes )
        continue;
      mMailingList = MailingList::detect( mes );
      if ( mMailingList.features() & MailingList::Post )
        break;
    }
  }
  if ( !(mMailingList.features() & MailingList::Post) ) {
    KMessageBox::error( this,
              i18n("KMail was unable to detect a mailing list in this folder. "
                   "Please fill the addresses by hand.") );
  } else {
    mMLId->setText( (mMailingList.id().isEmpty() ? i18n("Not available.") : mMailingList.id() ) );
    fillEditBox();
  }
}

//----------------------------------------------------------------------------
void FolderDiaMailingListTab::slotMLHandling( int element )
{
  mMailingList.setHandler( static_cast<MailingList::Handler>( element ) );
}

//----------------------------------------------------------------------------
void FolderDiaMailingListTab::slotAddressChanged( int i )
{
  fillMLFromWidgets();
  fillEditBox();
  mLastItem = i;
}

//----------------------------------------------------------------------------
void FolderDiaMailingListTab::fillMLFromWidgets()
{
  if ( !mHoldsMailingList->isChecked() )
    return;

  // make sure that email addresses are prepended by "mailto:"
  bool changed = false;
  QStringList oldList = mEditList->items();
  QStringList newList; // the correct string list
  for ( QStringList::ConstIterator it = oldList.begin();
        it != oldList.end(); ++it ) {
    if ( !(*it).startsWith("http:") && !(*it).startsWith("https:") &&
         !(*it).startsWith("mailto:") && ( (*it).find('@') != -1 ) ) {
      changed = true;
      newList << "mailto:" + *it;
    }
    else {
      newList << *it;
    }
  }
  if ( changed ) {
    mEditList->clear();
    mEditList->insertStringList( newList );
  }

  //mMailingList.setHandler( static_cast<MailingList::Handler>( mMLHandlerCombo->currentItem() ) );
  switch ( mLastItem ) {
  case 0:
    mMailingList.setPostURLS( mEditList->items() );
    break;
  case 1:
    mMailingList.setSubscribeURLS( mEditList->items() );
    break;
  case 2:
    mMailingList.setUnsubscribeURLS( mEditList->items() );
    break;
  case 3:
    mMailingList.setArchiveURLS( mEditList->items() );
    break;
  case 4:
    mMailingList.setHelpURLS( mEditList->items() );
    break;
  default:
    kdWarning( 5006 )<<"Wrong entry in the mailing list entry combo!"<<endl;
  }
}

void FolderDiaMailingListTab::fillEditBox()
{
  mEditList->clear();
  switch ( mAddressCombo->currentItem() ) {
  case 0:
    mEditList->insertStringList( mMailingList.postURLS().toStringList() );
    break;
  case 1:
    mEditList->insertStringList( mMailingList.subscribeURLS().toStringList() );
    break;
  case 2:
    mEditList->insertStringList( mMailingList.unsubscribeURLS().toStringList() );
    break;
  case 3:
    mEditList->insertStringList( mMailingList.archiveURLS().toStringList() );
    break;
  case 4:
    mEditList->insertStringList( mMailingList.helpURLS().toStringList() );
    break;
  default:
    kdWarning( 5006 )<<"Wrong entry in the mailing list entry combo!"<<endl;
  }
}

void FolderDiaMailingListTab::slotInvokeHandler()
{
  KMCommand *command =0;
  switch ( mAddressCombo->currentItem() ) {
  case 0:
    command = new KMMailingListPostCommand( this, mDlg->folder() );
    break;
  case 1:
    command = new KMMailingListSubscribeCommand( this, mDlg->folder() );
    break;
  case 2:
    command = new KMMailingListUnsubscribeCommand( this, mDlg->folder() );
    break;
  case 3:
    command = new KMMailingListArchivesCommand( this, mDlg->folder() );
    break;
  case 4:
    command = new KMMailingListHelpCommand( this, mDlg->folder() );
    break;
  default:
    kdWarning( 5006 )<<"Wrong entry in the mailing list entry combo!"<<endl;
  }
  if ( command ) command->start();
}


