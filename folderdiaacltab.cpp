// -*- mode: C++; c-file-style: "gnu" -*-
/**
 * folderdiaacltab.cpp
 *
 * Copyright (c) 2004 David Faure <faure@kde.org>
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

#include "folderdiaacltab.h"
#include "acljobs.h"
#include "kmfolderimap.h"
#include "kmfoldercachedimap.h"
#include "kmacctcachedimap.h"
#include "kmfolder.h"

#include <addressesdialog.h>
#include <kabc/addresseelist.h>
#include <distributionlist.h>
#include <kabc/stdaddressbook.h>
#include <kaddrbook.h>
#include <kpushbutton.h>
#include <kdebug.h>
#include <klocale.h>

#include <qlayout.h>
#include <qlabel.h>
#include <qvbox.h>
#include <qvbuttongroup.h>
#include <qwidgetstack.h>
#include <qradiobutton.h>
#include <qwhatsthis.h>

#include <assert.h>
#include <kmessagebox.h>

using namespace KMail;

// In case your kdelibs is < 3.3
#ifndef I18N_NOOP2
#define I18N_NOOP2( comment,x ) x
#endif

// The set of standard permission sets
static const struct {
  unsigned int permissions;
  const char* userString;
} standardPermissions[] = {
  { 0, I18N_NOOP2( "Permissions", "None" ) },
  { ACLJobs::List | ACLJobs::Read, I18N_NOOP2( "Permissions", "Read" ) },
  { ACLJobs::List | ACLJobs::Read | ACLJobs::Insert | ACLJobs::Post, I18N_NOOP2( "Permissions", "Append" ) },
  { ACLJobs::AllWrite, I18N_NOOP2( "Permissions", "Write" ) },
  { ACLJobs::All, I18N_NOOP2( "Permissions", "All" ) }
};


KMail::ACLEntryDialog::ACLEntryDialog( IMAPUserIdFormat userIdFormat, const QString& caption, QWidget* parent, const char* name )
  : KDialogBase( parent, name, true /*modal*/, caption,
                 KDialogBase::Ok|KDialogBase::Cancel, KDialogBase::Ok, true /*sep*/ )
  , mUserIdFormat( userIdFormat )
{
  QWidget *page = new QWidget( this );
  setMainWidget(page);
  QGridLayout *topLayout = new QGridLayout( page, 3 /*rows*/, 3 /*cols*/, 0, spacingHint() );

  QLabel *label = new QLabel( i18n( "&User identifier:" ), page );
  topLayout->addWidget( label, 0, 0 );

  mUserIdLineEdit = new KLineEdit( page );
  topLayout->addWidget( mUserIdLineEdit, 0, 1 );
  label->setBuddy( mUserIdLineEdit );
  QWhatsThis::add( mUserIdLineEdit, i18n( "The User Identifier is the login of the user on the IMAP server. This can be a simple user name or the full email address of the user; the login for your own account on the server will tell you which one it is." ) );

  QPushButton* kabBtn = new QPushButton( "...", page );
  topLayout->addWidget( kabBtn, 0, 2 );

  mButtonGroup = new QVButtonGroup( i18n( "Permissions" ), page );
  topLayout->addMultiCellWidget( mButtonGroup, 1, 1, 0, 2 );

  for ( unsigned int i = 0;
        i < sizeof( standardPermissions ) / sizeof( *standardPermissions );
        ++i ) {
    QRadioButton* cb = new QRadioButton( i18n( "Permissions", standardPermissions[i].userString ), mButtonGroup );
    // We store the permission value (bitfield) as the id of the radiobutton in the group
    mButtonGroup->insert( cb, standardPermissions[i].permissions );
  }
  topLayout->setRowStretch(2, 10);

  connect( mUserIdLineEdit, SIGNAL( textChanged( const QString& ) ), SLOT( slotChanged() ) );
  connect( kabBtn, SIGNAL( clicked() ), SLOT( slotSelectAddresses() ) );
  connect( mButtonGroup, SIGNAL( clicked( int ) ), SLOT( slotChanged() ) );
  enableButtonOK( false );

  mUserIdLineEdit->setFocus();
  // Ensure the lineedit is rather wide so that email addresses can be read in it
  incInitialSize( QSize( 200, 0 ) );
}

void KMail::ACLEntryDialog::slotChanged()
{
  enableButtonOK( !mUserIdLineEdit->text().isEmpty() && mButtonGroup->selected() != 0 );
}

static QString addresseeToUserId( const KABC::Addressee& addr, IMAPUserIdFormat userIdFormat )
{
  QString email = addr.preferredEmail();
  if ( userIdFormat == FullEmail )
    return email;
  else { // mUserIdFormat == UserName
    email.truncate( email.find( '@' ) );
    return email;
  }
}

void KMail::ACLEntryDialog::slotSelectAddresses()
{
  KPIM::AddressesDialog dlg( this );
  dlg.setShowCC( false );
  dlg.setShowBCC( false );
  if ( mUserIdFormat == FullEmail ) // otherwise we have no way to go back from userid to email
    dlg.setSelectedTo( userIds() );
  if ( dlg.exec() != QDialog::Accepted )
    return;

  const QStringList distrLists = dlg.toDistributionLists();
  QString txt;
  if ( !distrLists.isEmpty() ) {
    for( QStringList::ConstIterator it = distrLists.begin(); it != distrLists.end(); ++it ) {
      if ( !txt.isEmpty() )
        txt += ", ";
      txt += *it; // put the distr list name here, don't expand until saving
    }
  }
  const KABC::Addressee::List lst = dlg.toAddresses();
  if ( !lst.isEmpty() ) {
    for( QValueList<KABC::Addressee>::ConstIterator it = lst.begin(); it != lst.end(); ++it ) {
      if ( !txt.isEmpty() )
        txt += ", ";
      txt += addresseeToUserId( *it, mUserIdFormat );
    }
  }
  mUserIdLineEdit->setText( txt );
}

void KMail::ACLEntryDialog::setValues( const QString& userId, unsigned int permissions )
{
  mUserIdLineEdit->setText( userId );
  mButtonGroup->setButton( permissions );
  enableButtonOK( !userId.isEmpty() );
}

QString KMail::ACLEntryDialog::userId() const
{
  return mUserIdLineEdit->text();
}

QStringList KMail::ACLEntryDialog::userIds() const
{
  QStringList lst = QStringList::split( ",", mUserIdLineEdit->text() );
  for( QStringList::Iterator it = lst.begin(); it != lst.end(); ++it ) {
    // Strip white space (in particular, due to ", ")
    *it = (*it).stripWhiteSpace();
  }
  return lst;
}

unsigned int KMail::ACLEntryDialog::permissions() const
{
  return mButtonGroup->selectedId();
}

// class KMail::FolderDiaACLTab::ListView : public KListView
// {
// public:
//   ListView( QWidget* parent, const char* name = 0 ) : KListView( parent, name ) {}
// };

class KMail::FolderDiaACLTab::ListViewItem : public KListViewItem
{
public:
  ListViewItem( QListView* listview )
    : KListViewItem( listview, listview->lastItem() ),
      mModified( false ), mNew( false ) {}

  void load( const ACLListEntry& entry );
  void save( ACLList& list, KABC::AddressBook* addressBook, IMAPUserIdFormat userIdFormat );

  QString userId() const { return text( 0 ); }
  void setUserId( const QString& userId ) { setText( 0, userId ); }

  unsigned int permissions() const { return mPermissions; }
  void setPermissions( unsigned int permissions );

  bool isModified() const { return mModified; }
  void setModified( bool b ) { mModified = b; }

  // The fact that an item is new doesn't matter much.
  // This bool is only used to handle deletion differently
  bool isNew() const { return mNew; }
  void setNew( bool b ) { mNew = b; }

private:
  unsigned int mPermissions;
  QString mInternalRightsList; ///< protocol-dependent string (e.g. IMAP rights list)
  bool mModified;
  bool mNew;
};

// internalRightsList is only used if permissions doesn't match the standard set
static QString permissionsToUserString( unsigned int permissions, const QString& internalRightsList )
{
  for ( unsigned int i = 0;
        i < sizeof( standardPermissions ) / sizeof( *standardPermissions );
        ++i ) {
    if ( permissions == standardPermissions[i].permissions )
      return i18n( "Permissions", standardPermissions[i].userString );
  }
  if ( internalRightsList.isEmpty() )
    return i18n( "Custom Permissions" ); // not very helpful, but shouldn't happen
  else
    return i18n( "Custom Permissions (%1)" ).arg( internalRightsList );
}

void KMail::FolderDiaACLTab::ListViewItem::setPermissions( unsigned int permissions )
{
  mPermissions = permissions;
  setText( 1, permissionsToUserString( permissions, QString::null ) );
}

void KMail::FolderDiaACLTab::ListViewItem::load( const ACLListEntry& entry )
{
  // Don't allow spaces in userids. If you need this, fix the slave->app communication,
  // since it uses space as a separator (imap4.cc, look for GETACL)
  // It's ok in distribution list names though, that's why this check is only done here
  // and also why there's no validator on the lineedit.
  if ( entry.userId.contains( ' ' ) )
    kdWarning(5006) << "Userid contains a space!!!  '" << entry.userId << "'" << endl;

  setUserId( entry.userId );
  mPermissions = entry.permissions;
  mInternalRightsList = entry.internalRightsList;
  setText( 1, permissionsToUserString( entry.permissions, entry.internalRightsList ) );
  mModified = entry.changed; // for dimap, so that earlier changes are still marked as changes
}

void KMail::FolderDiaACLTab::ListViewItem::save( ACLList& aclList, KABC::AddressBook* addressBook, IMAPUserIdFormat userIdFormat )
{
  // expand distribution lists
  KPIM::DistributionList list = KPIM::DistributionList::findByName( addressBook, userId(), false );
  if ( !list.isEmpty() ) {
    Q_ASSERT( mModified ); // it has to be new, it couldn't be stored as a distr list name....
    KPIM::DistributionList::Entry::List entryList = list.entries(addressBook);
    KPIM::DistributionList::Entry::List::ConstIterator it; // nice number of "::"!
    for( it = entryList.begin(); it != entryList.end(); ++it ) {
      QString email = (*it).email;
      if ( email.isEmpty() )
        email = addresseeToUserId( (*it).addressee, userIdFormat );
      ACLListEntry entry( email, QString::null, mPermissions );
      entry.changed = true;
      aclList.append( entry );
    }
  } else { // it wasn't a distribution list
    ACLListEntry entry( userId(), mInternalRightsList, mPermissions );
    if ( mModified ) {
      entry.internalRightsList = QString::null;
      entry.changed = true;
    }
    aclList.append( entry );
  }
}

////

KMail::FolderDiaACLTab::FolderDiaACLTab( KMFolderDialog* dlg, QWidget* parent, const char* name )
  : FolderDiaTab( parent, name ),
    mImapAccount( 0 ),
    mUserRights( 0 ),
    mDlg( dlg ),
    mChanged( false ), mAccepting( false ), mSaving( false )
{
  QVBoxLayout* topLayout = new QVBoxLayout( this );
  // We need a widget stack to show either a label ("no acl support", "please wait"...)
  // or a listview.
  mStack = new QWidgetStack( this );
  topLayout->addWidget( mStack );

  mLabel = new QLabel( mStack );
  mLabel->setAlignment( AlignHCenter | AlignVCenter | WordBreak );
  mStack->addWidget( mLabel );

  mACLWidget = new QHBox( mStack );
  mACLWidget->setSpacing( KDialog::spacingHint() );
  mListView = new KListView( mACLWidget );
  mListView->setAllColumnsShowFocus( true );
  mStack->addWidget( mACLWidget );
  mListView->addColumn( i18n( "User Id" ) );
  mListView->addColumn( i18n( "Permissions" ) );

  connect( mListView, SIGNAL(doubleClicked(QListViewItem*,const QPoint&,int)),
	   SLOT(slotEditACL(QListViewItem*)) );
  connect( mListView, SIGNAL(returnPressed(QListViewItem*)),
	   SLOT(slotEditACL(QListViewItem*)) );
  connect( mListView, SIGNAL(selectionChanged(QListViewItem*)),
	   SLOT(slotSelectionChanged(QListViewItem*)) );

  QVBox* buttonBox = new QVBox( mACLWidget );
  buttonBox->setSpacing( KDialog::spacingHint() );
  mAddACL = new KPushButton( i18n( "Add Entry" ), buttonBox );
  mEditACL = new KPushButton( i18n( "Modify Entry" ), buttonBox );
  mRemoveACL = new KPushButton( i18n( "Remove Entry" ), buttonBox );
  QSpacerItem* spacer = new QSpacerItem( 0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding );
  static_cast<QBoxLayout *>( buttonBox->layout() )->addItem( spacer );

  connect( mAddACL, SIGNAL( clicked() ), SLOT( slotAddACL() ) );
  connect( mEditACL, SIGNAL( clicked() ), SLOT( slotEditACL() ) );
  connect( mRemoveACL, SIGNAL( clicked() ), SLOT( slotRemoveACL() ) );
  mEditACL->setEnabled( false );
  mRemoveACL->setEnabled( false );

  connect( this, SIGNAL( changed(bool) ), SLOT( slotChanged(bool) ) );
}

// Warning before save() this will return the url of the _parent_ folder, when creating a new one
KURL KMail::FolderDiaACLTab::imapURL() const
{
  KURL url = mImapAccount->getUrl();
  url.setPath( mImapPath );
  return url;
}

void KMail::FolderDiaACLTab::initializeWithValuesFromFolder( KMFolder* folder )
{
  // This can be simplified once KMFolderImap and KMFolderCachedImap have a common base class
  mFolderType = folder->folderType();
  if ( mFolderType == KMFolderTypeImap ) {
    KMFolderImap* folderImap = static_cast<KMFolderImap*>( folder->storage() );
    mImapPath = folderImap->imapPath();
    mImapAccount = folderImap->account();
    mUserRights = folderImap->userRights();
  }
  else if ( mFolderType == KMFolderTypeCachedImap ) {
    KMFolderCachedImap* folderImap = static_cast<KMFolderCachedImap*>( folder->storage() );
    mImapPath = folderImap->imapPath();
    mImapAccount = folderImap->account();
    mUserRights = folderImap->userRights();
  }
  else
    assert( 0 ); // see KMFolderDialog constructor
}

void KMail::FolderDiaACLTab::load()
{
  if ( mDlg->folder() ) {
    // existing folder
    initializeWithValuesFromFolder( mDlg->folder() );
  } else if ( mDlg->parentFolder() ) {
    // new folder
    initializeWithValuesFromFolder( mDlg->parentFolder() );
    mChanged = true; // ensure that saving happens
  }

  // KABC knows email addresses.
  // We want LDAP userids.
  // Depending on the IMAP server setup, the userid can be the full email address,
  // or just the username part of it.
  // To know which one it is, we currently have a hidden config option,
  // but the default value is determined from the current user's own id.
  QString defaultFormat = "fullemail";
  // warning mImapAccount can be 0 if creating a subsubsubfolder with dimap...  (bug?)
  if ( mImapAccount && mImapAccount->login().find('@') == -1 )
    defaultFormat = "username"; // no @ found, so we assume it's just the username
  KConfigGroup configGroup( kmkernel->config(), "IMAP" );
  QString str = configGroup.readEntry( "UserIdFormat", defaultFormat );
  mUserIdFormat = FullEmail;
  if ( str == "username" )
    mUserIdFormat = UserName;

  if ( mFolderType == KMFolderTypeCachedImap ) {
    KMFolder* folder = mDlg->folder() ? mDlg->folder() : mDlg->parentFolder();
    KMFolderCachedImap* folderImap = static_cast<KMFolderCachedImap*>( folder->storage() );
    if ( mUserRights == -1 ) { // error
      mLabel->setText( i18n( "Error retrieving user permissions." ) );
    } else if ( mUserRights == 0 ) { // not listed yet
      mLabel->setText( i18n( "Information not retrieved from server yet, please use \"Check Mail\"." ) );
      // TODO: save mUserRights and mACLList into a config file so that this almost never happens
    } else {
      /* We either synced, or we read user rights from the config, so we can 
         assume the server supports acls and an empty list means we haven't
         synced yet. */
      if ( folderImap->aclList().isEmpty() ) {
        mLabel->setText( i18n( "Information not retrieved from server yet, please use \"Check Mail\"." ) );
      } else {
        loadFinished( folderImap->aclList() );
      }
    }
    return;
  }

  // Loading, for online IMAP, consists of four steps:
  // 1) connect
  // 2) get user rights
  // 3) load ACLs

  // First ensure we are connected
  mStack->raiseWidget( mLabel );
  if ( !mImapAccount ) { // hmmm?
    mLabel->setText( i18n( "Error: no IMAP account defined for this folder" ) );
    return;
  }
  KMFolder* folder = mDlg->folder() ? mDlg->folder() : mDlg->parentFolder();
  if ( folder && folder->storage() == mImapAccount->rootFolder() )
    return; // nothing to be done for the (virtual) account folder
  mLabel->setText( i18n( "Connecting to server %1, please wait..." ).arg( mImapAccount->host() ) );
  ImapAccountBase::ConnectionState state = mImapAccount->makeConnection();
  if ( state == ImapAccountBase::Error ) { // Cancelled by user, or slave can't start
    slotConnectionResult( -1, QString::null );
  } else if ( state == ImapAccountBase::Connecting ) {
    connect( mImapAccount, SIGNAL( connectionResult(int, const QString&) ),
             this, SLOT( slotConnectionResult(int, const QString&) ) );
  } else { // Connected
    slotConnectionResult( 0, QString::null );
  }
}

void KMail::FolderDiaACLTab::slotConnectionResult( int errorCode, const QString& errorMsg )
{
  disconnect( mImapAccount, SIGNAL( connectionResult(int, const QString&) ),
              this, SLOT( slotConnectionResult(int, const QString&) ) );
  if ( errorCode ) {
    if ( errorCode == -1 ) // unspecified error
      mLabel->setText( i18n( "Error connecting to server %1" ).arg( mImapAccount->host() ) );
    else
      // Connection error (error message box already shown by the account)
      mLabel->setText( KIO::buildErrorString( errorCode, errorMsg ) );
    return;
  }

  if ( mUserRights == 0 ) {
    connect( mImapAccount, SIGNAL( receivedUserRights( KMFolder* ) ),
             this, SLOT( slotReceivedUserRights( KMFolder* ) ) );
    KMFolder* folder = mDlg->folder() ? mDlg->folder() : mDlg->parentFolder();
    mImapAccount->getUserRights( folder, mImapPath );
  }
  else
    startListing();
}

void KMail::FolderDiaACLTab::slotReceivedUserRights( KMFolder* folder )
{
  if ( !mImapAccount->hasACLSupport() ) {
    mLabel->setText( i18n( "This IMAP server does not have support for access control lists (ACL)" ) );
    return;
  }

  if ( folder == mDlg->folder() ? mDlg->folder() : mDlg->parentFolder() ) {
    KMFolderImap* folderImap = static_cast<KMFolderImap*>( folder->storage() );
    mUserRights = folderImap->userRights();
    startListing();
  }
}

void KMail::FolderDiaACLTab::startListing()
{
  // List ACLs of folder - or its parent, if creating a new folder
  mImapAccount->getACL( mDlg->folder() ? mDlg->folder() : mDlg->parentFolder(), mImapPath );
  connect( mImapAccount, SIGNAL(receivedACL( KMFolder*, KIO::Job*, const KMail::ACLList& )),
           this, SLOT(slotReceivedACL( KMFolder*, KIO::Job*, const KMail::ACLList& )) );
}

void KMail::FolderDiaACLTab::slotReceivedACL( KMFolder* folder, KIO::Job* job, const KMail::ACLList& aclList )
{
  if ( folder == ( mDlg->folder() ? mDlg->folder() : mDlg->parentFolder() ) ) {
    disconnect( mImapAccount, SIGNAL(receivedACL( KMFolder*, KIO::Job*, const KMail::ACLList& )),
                this, SLOT(slotReceivedACL( KMFolder*, KIO::Job*, const KMail::ACLList& )) );

    if ( job && job->error() ) {
      if ( job->error() == KIO::ERR_UNSUPPORTED_ACTION )
        mLabel->setText( i18n( "This IMAP server does not have support for access control lists (ACL)" ) );
      else
        mLabel->setText( i18n( "Error retrieving access control list (ACL) from server\n%1" ).arg( job->errorString() ) );
      return;
    }

    loadFinished( aclList );
  }
}

void KMail::FolderDiaACLTab::loadListView( const ACLList& aclList )
{
  mListView->clear();
  for( ACLList::const_iterator it = aclList.begin(); it != aclList.end(); ++it ) {
    // -1 means deleted (for cachedimap), don't show those
    if ( (*it).permissions > -1 ) {
      ListViewItem* item = new ListViewItem( mListView );
      item->load( *it );
      if ( !mDlg->folder() ) // new folder? everything is new then
          item->setModified( true );
    }
  }
}

void KMail::FolderDiaACLTab::loadFinished( const ACLList& aclList )
{
  loadListView( aclList );
  if ( mDlg->folder() ) // not when creating a new folder
    mInitialACLList = aclList;
  mStack->raiseWidget( mACLWidget );
  slotSelectionChanged( mListView->selectedItem() );
}

void KMail::FolderDiaACLTab::slotEditACL(QListViewItem* item)
{
  if ( !item ) return;
  bool canAdmin = ( mUserRights & ACLJobs::Administer );
  // Same logic as in slotSelectionChanged, but this is also needed for double-click IIRC
  if ( canAdmin && mImapAccount && item ) {
    // Don't allow users to remove their own admin permissions - there's no way back
    ListViewItem* ACLitem = static_cast<ListViewItem *>( item );
    if ( mImapAccount->login() == ACLitem->userId() && ACLitem->permissions() == ACLJobs::All )
      canAdmin = false;
  }
  if ( !canAdmin ) return;

  ListViewItem* ACLitem = static_cast<ListViewItem *>( mListView->currentItem() );
  ACLEntryDialog dlg( mUserIdFormat, i18n( "Modify Permissions" ), this );
  dlg.setValues( ACLitem->userId(), ACLitem->permissions() );
  if ( dlg.exec() == QDialog::Accepted ) {
    QStringList userIds = dlg.userIds();
    Q_ASSERT( !userIds.isEmpty() ); // impossible, the OK button is disabled in that case
    ACLitem->setUserId( dlg.userIds().front() );
    ACLitem->setPermissions( dlg.permissions() );
    ACLitem->setModified( true );
    emit changed(true);
    if ( userIds.count() > 1 ) { // more emails were added, append them
      userIds.pop_front();
      addACLs( userIds, dlg.permissions() );
    }
  }
}

void KMail::FolderDiaACLTab::slotEditACL()
{
  slotEditACL( mListView->currentItem() );
}

void KMail::FolderDiaACLTab::addACLs( const QStringList& userIds, unsigned int permissions )
{
  for( QStringList::const_iterator it = userIds.begin(); it != userIds.end(); ++it ) {
    ListViewItem* ACLitem = new ListViewItem( mListView );
    ACLitem->setUserId( *it );
    ACLitem->setPermissions( permissions );
    ACLitem->setModified( true );
    ACLitem->setNew( true );
  }
}

void KMail::FolderDiaACLTab::slotAddACL()
{
  ACLEntryDialog dlg( mUserIdFormat, i18n( "Add Permissions" ), this );
  if ( dlg.exec() == QDialog::Accepted ) {
    const QStringList userIds = dlg.userIds();
    addACLs( dlg.userIds(), dlg.permissions() );
    emit changed(true);
  }
}

void KMail::FolderDiaACLTab::slotSelectionChanged(QListViewItem* item)
{
  bool canAdmin = ( mUserRights & ACLJobs::Administer );
  bool canAdminThisItem = canAdmin;
  if ( canAdmin && mImapAccount && item ) {
    // Don't allow users to remove their own admin permissions - there's no way back
    ListViewItem* ACLitem = static_cast<ListViewItem *>( item );
    if ( mImapAccount->login() == ACLitem->userId() && ACLitem->permissions() == ACLJobs::All )
      canAdminThisItem = false;
  }

  bool lvVisible = mStack->visibleWidget() == mACLWidget;
  mAddACL->setEnabled( lvVisible && canAdmin && !mSaving );
  mEditACL->setEnabled( item && lvVisible && canAdminThisItem && !mSaving );
  mRemoveACL->setEnabled( item && lvVisible && canAdminThisItem && !mSaving );
}

void KMail::FolderDiaACLTab::slotRemoveACL()
{
  ListViewItem* ACLitem = static_cast<ListViewItem *>( mListView->currentItem() );
  if ( !ACLitem )
    return;
  if ( !ACLitem->isNew() ) {
    if ( mImapAccount && mImapAccount->login() == ACLitem->userId() ) {
      if ( KMessageBox::Cancel == KMessageBox::warningContinueCancel( topLevelWidget(),
         i18n( "Do you really want to remove your own permissions for this folder? You will not be able to access it afterwards." ), i18n( "Remove" ) ) )
        return;
    }
    mRemovedACLs.append( ACLitem->userId() );
  }
  delete ACLitem;
  emit changed(true);
}

KMail::FolderDiaTab::AcceptStatus KMail::FolderDiaACLTab::accept()
{
  if ( !mChanged || !mImapAccount )
    return Accepted; // (no change made), ok for accepting the dialog immediately
  // If there were changes, we need to apply them first (which is async)
  save();
  if ( mFolderType == KMFolderTypeCachedImap )
    return Accepted; // cached imap: changes saved immediately into the folder
  // disconnected imap: async job[s] running
  mAccepting = true;
  return Delayed;
}

bool KMail::FolderDiaACLTab::save()
{
  if ( !mChanged || !mImapAccount ) // no changes
    return true;
  assert( mDlg->folder() ); // should have been created already

  // Expand distribution lists. This is necessary because after Apply
  // we would otherwise be able to "modify" the permissions for a distr list,
  // which wouldn't work since the ACLList and the server only know about the
  // individual addresses.
  // slotACLChanged would have trouble matching the item too.
  // After reloading we'd see the list expanded anyway,
  // so this is more consistent.
  // But we do it now and not when inserting it, because this allows to
  // immediately remove a wrongly inserted distr list without having to
  // remove 100 items.
  // Now, how to expand them? Playing with listviewitem iterators and inserting
  // listviewitems at the same time sounds dangerous, so let's just save into
  // ACLList and reload that.
  KABC::AddressBook *addressBook = KABC::StdAddressBook::self();
  ACLList aclList;
  for ( QListViewItem* item = mListView->firstChild(); item; item = item->nextSibling() ) {
    ListViewItem* ACLitem = static_cast<ListViewItem *>( item );
    ACLitem->save( aclList, addressBook, mUserIdFormat );
  }
  loadListView( aclList );

  // Now compare with the initial ACLList, because if the user renamed a userid
  // we have to add the old userid to the "to be deleted" list.
  for( ACLList::ConstIterator init = mInitialACLList.begin(); init != mInitialACLList.end(); ++init ) {
    bool isInNewList = false;
    QString uid = (*init).userId;
    for( ACLList::ConstIterator it = aclList.begin(); it != aclList.end() && !isInNewList; ++it )
      isInNewList = uid == (*it).userId;
    if ( !isInNewList && !mRemovedACLs.contains(uid) )
      mRemovedACLs.append( uid );
  }

  for ( QStringList::ConstIterator rit = mRemovedACLs.begin(); rit != mRemovedACLs.end(); ++rit ) {
    // We use permissions == -1 to signify deleting. At least on cyrus, setacl(0) or deleteacl are the same,
    // but I'm not sure if that's true for all servers.
    ACLListEntry entry( *rit, QString::null, -1 );
    entry.changed = true;
    aclList.append( entry );
  }

  // aclList is finally ready. We can save it (dimap) or apply it (imap).

  if ( mFolderType == KMFolderTypeCachedImap ) {
    // Apply the changes to the aclList stored in the folder.
    // We have to do this now and not before, so that cancel really cancels.
    KMFolderCachedImap* folderImap = static_cast<KMFolderCachedImap*>( mDlg->folder()->storage() );
    folderImap->setACLList( aclList );
    return true;
  }

  mACLList = aclList;

  KMFolderImap* parentImap = mDlg->parentFolder() ? static_cast<KMFolderImap*>( mDlg->parentFolder()->storage() ) : 0;

  if ( mDlg->isNewFolder() ) {
    // The folder isn't created yet, wait for it
    // It's a two-step process (mkdir+listDir) so we wait for the dir listing to be complete
    connect( parentImap, SIGNAL( directoryListingFinished(KMFolderImap*) ),
             this, SLOT( slotDirectoryListingFinished(KMFolderImap*) ) );
  } else {
      slotDirectoryListingFinished( parentImap );
  }
  return true;
}

void KMail::FolderDiaACLTab::slotDirectoryListingFinished(KMFolderImap* f)
{
  if ( !f ||
       f != static_cast<KMFolderImap*>( mDlg->parentFolder()->storage() ) ||
       !mDlg->folder() ||
       !mDlg->folder()->storage() ) {
    emit readyForAccept();
    return;
  }

  // When creating a new folder with online imap, update mImapPath
  KMFolderImap* folderImap = static_cast<KMFolderImap*>( mDlg->folder()->storage() );
  if ( !folderImap || folderImap->imapPath().isEmpty() )
    return;
  mImapPath = folderImap->imapPath();

  KIO::Job* job = ACLJobs::multiSetACL( mImapAccount->slave(), imapURL(), mACLList );
  ImapAccountBase::jobData jd;
  jd.total = 1; jd.done = 0; jd.parent = 0;
  mImapAccount->insertJob(job, jd);

  connect(job, SIGNAL(result(KIO::Job *)),
          SLOT(slotMultiSetACLResult(KIO::Job *)));
  connect(job, SIGNAL(aclChanged( const QString&, int )),
          SLOT(slotACLChanged( const QString&, int )) );
}

void KMail::FolderDiaACLTab::slotMultiSetACLResult(KIO::Job* job)
{
  ImapAccountBase::JobIterator it = mImapAccount->findJob( job );
  if ( it == mImapAccount->jobsEnd() ) return;
  mImapAccount->removeJob( it );

  if ( job->error() ) {
    job->showErrorDialog( this );
    if ( mAccepting ) {
      emit cancelAccept();
      mAccepting = false; // don't emit readyForAccept anymore
    }
  } else {
    if ( mAccepting )
      emit readyForAccept();
  }
}

void KMail::FolderDiaACLTab::slotACLChanged( const QString& userId, int permissions )
{
  // The job indicates success in changing the permissions for this user
  // -> we note that it's been done.
  bool ok = false;
  if ( permissions > -1 ) {
    for ( QListViewItem* item = mListView->firstChild(); item; item = item->nextSibling() ) {
      ListViewItem* ACLitem = static_cast<ListViewItem *>( item );
      if ( ACLitem->userId() == userId ) {
        ACLitem->setModified( false );
        ACLitem->setNew( false );
        ok = true;
        break;
      }
    }
  } else {
    uint nr = mRemovedACLs.remove( userId );
    ok = ( nr > 0 );
  }
  if ( !ok )
    kdWarning(5006) << k_funcinfo << " no item found for userId " << userId << endl;
}

void KMail::FolderDiaACLTab::slotChanged( bool b )
{
  mChanged = b;
}

bool KMail::FolderDiaACLTab::supports( KMFolder* refFolder )
{
  ImapAccountBase* imapAccount = 0;
  if ( refFolder->folderType() == KMFolderTypeImap )
    imapAccount = static_cast<KMFolderImap*>( refFolder->storage() )->account();
  else
    imapAccount = static_cast<KMFolderCachedImap*>( refFolder->storage() )->account();
  return imapAccount && imapAccount->hasACLSupport(); // support for ACLs (or not tried connecting yet)
}

#include "folderdiaacltab.moc"
