// -*- mode: C++; c-file-style: "gnu" -*-
/**
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

#include "collectionaclpage.h"
#include <khbox.h>
#include <QStackedWidget>
#include "messageviewer/autoqpointer.h"
#include "imapaclattribute.h"
#include "util.h"
#include "imapsettings.h"

#include <akonadi/collection.h>
#include <akonadi/collectionmodifyjob.h>

#include <akonadi/contact/contactgroupexpandjob.h>
#include <akonadi/contact/contactgroupsearchjob.h>
#include <akonadi/contact/emailaddressselectiondialog.h>
#include <kabc/addresseelist.h>
#include <kio/jobuidelegate.h>
#include <kpushbutton.h>
#include <kdebug.h>
#include <klocale.h>
#include <kconfiggroup.h>
#include <KLineEdit>
#include <kpimutils/email.h>

#include <QDBusReply>
#include <QLabel>
#include <QRadioButton>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QButtonGroup>
#include <QGroupBox>
#include <QTreeWidget>

#include <assert.h>
#include <kmessagebox.h>
#include <kvbox.h>

// The set of standard permission sets
static const struct {
  KIMAP::Acl::Rights permissions;
  const char* userString;
} standardPermissions[] = {
  { KIMAP::Acl::None, I18N_NOOP2( "Permissions", "None" ) },
  { KIMAP::Acl::Lookup | KIMAP::Acl::Read | KIMAP::Acl::KeepSeen, I18N_NOOP2( "Permissions", "Read" ) },
  { KIMAP::Acl::Lookup | KIMAP::Acl::Read | KIMAP::Acl::KeepSeen | KIMAP::Acl::Insert | KIMAP::Acl::Post, I18N_NOOP2( "Permissions", "Append" ) },
  { KIMAP::Acl::Lookup | KIMAP::Acl::Read | KIMAP::Acl::KeepSeen | KIMAP::Acl::Insert | KIMAP::Acl::Post | KIMAP::Acl::Write | KIMAP::Acl::Create | KIMAP::Acl::Delete, I18N_NOOP2( "Permissions", "Write" ) },
  { KIMAP::Acl::Lookup | KIMAP::Acl::Read | KIMAP::Acl::KeepSeen | KIMAP::Acl::Insert | KIMAP::Acl::Post | KIMAP::Acl::Write | KIMAP::Acl::Create | KIMAP::Acl::Delete | KIMAP::Acl::Admin, I18N_NOOP2( "Permissions", "All" ) }
};

ACLEntryDialog::ACLEntryDialog( const QString& caption, QWidget* parent )
  : KDialog( parent )
{
  setCaption( caption );
  setButtons( Ok | Cancel );
  QWidget *page = new QWidget( this );
  setMainWidget(page);
  QGridLayout *topLayout = new QGridLayout( page );
  topLayout->setSpacing( spacingHint() );
  topLayout->setMargin( 0 );

  QLabel *label = new QLabel( i18n( "&User identifier:" ), page );
  topLayout->addWidget( label, 0, 0 );

  mUserIdLineEdit = new KLineEdit( page );
  topLayout->addWidget( mUserIdLineEdit, 0, 1 );
  label->setBuddy( mUserIdLineEdit );
  mUserIdLineEdit->setWhatsThis( i18n( "The User Identifier is the login of the user on the IMAP server. This can be a simple user name or the full email address of the user; the login for your own account on the server will tell you which one it is." ) );

  QPushButton* kabBtn = new QPushButton( i18n( "Se&lect..." ), page );
  topLayout->addWidget( kabBtn, 0, 2 );

  QGroupBox* groupBox = new QGroupBox( i18n( "Permissions" ), page );
  QVBoxLayout *vbox = new QVBoxLayout( groupBox );

  mButtonGroup = new QButtonGroup( groupBox );

  for ( unsigned int i = 0;
        i < sizeof( standardPermissions ) / sizeof( *standardPermissions );
        ++i ) {
    QRadioButton* cb = new QRadioButton( i18nc( "Permissions", standardPermissions[i].userString ), groupBox );
    vbox->addWidget( cb );
    // We store the permission value (bitfield) as the id of the radiobutton in the group
    mButtonGroup->addButton( cb, standardPermissions[i].permissions );
  }

  vbox->addStretch( 1 );

  topLayout->addWidget( groupBox, 1, 0, 1, 3 );
  topLayout->setRowStretch(2, 10);

  connect( mUserIdLineEdit, SIGNAL( textChanged( const QString& ) ), SLOT( slotChanged() ) );
  connect( kabBtn, SIGNAL( clicked() ), SLOT( slotSelectAddresses() ) );
  connect( mButtonGroup, SIGNAL( buttonClicked( int ) ), SLOT( slotChanged() ) );
  enableButtonOk( false );

  mUserIdLineEdit->setFocus();
  // Ensure the lineedit is rather wide so that email addresses can be read in it
  incrementInitialSize( QSize( 200, 0 ) );
}

void ACLEntryDialog::slotChanged()
{
  enableButtonOk( !mUserIdLineEdit->text().isEmpty() && mButtonGroup->checkedButton() != 0 );
}

void ACLEntryDialog::slotSelectAddresses()
{
  MessageViewer::AutoQPointer<Akonadi::EmailAddressSelectionDialog> dlg( new Akonadi::EmailAddressSelectionDialog( this ) );
  dlg->view()->view()->setSelectionMode( QAbstractItemView::MultiSelection );

  if ( dlg->exec() != QDialog::Accepted || !dlg )
    return;

  QStringList addresses;

  foreach ( const Akonadi::EmailAddressSelection &selection, dlg->selectedAddresses() )
    addresses << selection.quotedEmail();

  if ( !mUserIdLineEdit->text().isEmpty() )
    addresses.prepend( mUserIdLineEdit->text() );

  mUserIdLineEdit->setText( addresses.join( ", " ) );
}

void ACLEntryDialog::setValues( const QString& userId, KIMAP::Acl::Rights permissions )
{
  mUserIdLineEdit->setText( userId );

  QAbstractButton* button = mButtonGroup->button( permissions );
  if ( button )
    button->setChecked( true );

  enableButtonOk( !userId.isEmpty() );
}

QString ACLEntryDialog::userId() const
{
  return mUserIdLineEdit->text();
}

QStringList ACLEntryDialog::userIds() const
{
  return KPIMUtils::splitAddressList( mUserIdLineEdit->text() );
}

KIMAP::Acl::Rights ACLEntryDialog::permissions() const
{
  QAbstractButton* button = mButtonGroup->checkedButton();
  if( !button )
    return static_cast<KIMAP::Acl::Rights>(-1); // hm ?
  return static_cast<KIMAP::Acl::Rights>( mButtonGroup->id( button ) );
}

class CollectionAclPage::ListViewItem : public QTreeWidgetItem
{
public:
  ListViewItem( QTreeWidget* listview )
    : QTreeWidgetItem( listview ),
      mModified( false ), mNew( false ) {}

  void load( const QByteArray &id, KIMAP::Acl::Rights rights );

  QString userId() const { return text( 0 ); }
  void setUserId( const QString& userId ) { setText( 0, userId ); }

  KIMAP::Acl::Rights permissions() const { return mPermissions; }
  void setPermissions( KIMAP::Acl::Rights permissions );

  bool isModified() const { return mModified; }
  void setModified( bool b ) { mModified = b; }

  // The fact that an item is new doesn't matter much.
  // This bool is only used to handle deletion differently
  bool isNew() const { return mNew; }
  void setNew( bool b ) { mNew = b; }

private:
  KIMAP::Acl::Rights mPermissions;
  QString mInternalRightsList; ///< protocol-dependent string (e.g. IMAP rights list)
  bool mModified;
  bool mNew;
};

// internalRightsList is only used if permissions doesn't match the standard set
static QString permissionsToUserString( int permissions, const QString& internalRightsList )
{
  for ( int i = 0;
        i < sizeof( standardPermissions ) / sizeof( *standardPermissions );
        ++i ) {
    if ( permissions == standardPermissions[i].permissions )
      return i18nc( "Permissions", standardPermissions[i].userString );
  }
  if ( internalRightsList.isEmpty() )
    return i18n( "Custom Permissions" ); // not very helpful, but should not happen
  else
    return i18n( "Custom Permissions (%1)", internalRightsList );
}

void CollectionAclPage::ListViewItem::setPermissions( KIMAP::Acl::Rights permissions )
{
  mPermissions = permissions;
  setText( 1, permissionsToUserString( permissions, QString() ) );
}

void CollectionAclPage::ListViewItem::load( const QByteArray &id, KIMAP::Acl::Rights rights )
{
  // Don't allow spaces in userids. If you need this, fix the slave->app communication,
  // since it uses space as a separator (imap4.cc, look for GETACL)
  // It's ok in distribution list names though, that's why this check is only done here
  // and also why there's no validator on the lineedit.
  if ( id.contains( ' ' ) ) {
    kWarning() << "Userid contains a space:" << id;
  }

  setUserId( id );
  mPermissions = rights;
  mInternalRightsList = KIMAP::Acl::rightsToString( rights );
  setText( 1, permissionsToUserString( mPermissions, mInternalRightsList ) );
  mModified = true; // for dimap, so that earlier changes are still marked as changes
}

////

CollectionAclPage::CollectionAclPage( QWidget* parent )
  : CollectionPropertiesPage( parent ),
    mUserRights( KIMAP::Acl::None ),
    mChanged( false )
{
  setPageTitle( i18n("Access Control") );
  init();
}

void CollectionAclPage::init()
{
  QVBoxLayout* topLayout = new QVBoxLayout( this );
  // We need a widget stack to show either a label ("no acl support", "please wait"...)
  // or a listview.
  mStack = new QStackedWidget( this );
  topLayout->addWidget( mStack );

  mLabel = new QLabel( mStack );
  mLabel->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
  mLabel->setWordWrap( true );
  mStack->addWidget( mLabel );

  mACLWidget = new KHBox( mStack );
  mACLWidget->setSpacing( KDialog::spacingHint() );

  mListView = new QTreeWidget( mACLWidget );
  QStringList headerNames;
  headerNames << i18n("User Id") << i18n("Permissions");
  mListView->setHeaderItem( new QTreeWidgetItem( headerNames ) );
  mListView->setAllColumnsShowFocus( true );
  mListView->setAlternatingRowColors( true );
  mListView->setSortingEnabled( false );
  mListView->setRootIsDecorated( false );

  mStack->addWidget( mACLWidget );

  connect( mListView, SIGNAL( itemActivated( QTreeWidgetItem*, int ) ),
       SLOT( slotEditACL( QTreeWidgetItem* ) ) );
  connect( mListView, SIGNAL( itemSelectionChanged() ),
       SLOT( slotSelectionChanged() ) );
  KVBox* buttonBox = new KVBox( mACLWidget );
  buttonBox->setSpacing( KDialog::spacingHint() );
  mAddACL = new KPushButton( i18n( "Add Entry..." ), buttonBox );
  mEditACL = new KPushButton( i18n( "Modify Entry..." ), buttonBox );
  mRemoveACL = new KPushButton( i18n( "Remove Entry" ), buttonBox );
  QWidget *spacer = new QWidget( buttonBox );
  spacer->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Expanding );

  connect( mAddACL, SIGNAL( clicked() ), SLOT( slotAddACL() ) );
  connect( mEditACL, SIGNAL( clicked() ), SLOT( slotEditACL() ) );
  connect( mRemoveACL, SIGNAL( clicked() ), SLOT( slotRemoveACL() ) );
  mEditACL->setEnabled( false );
  mRemoveACL->setEnabled( false );
  mStack->setCurrentWidget( mACLWidget );
}

bool CollectionAclPage::canHandle( const Akonadi::Collection &collection ) const
{
  return collection.hasAttribute<Akonadi::ImapAclAttribute>();
}

void CollectionAclPage::load(const Akonadi::Collection & col)
{
  Akonadi::ImapAclAttribute *acls = col.attribute<Akonadi::ImapAclAttribute>();
  QMap<QByteArray, KIMAP::Acl::Rights> rights = acls->rights();

  mListView->clear();
  foreach ( const QByteArray &id, rights.keys() ) {
    ListViewItem* item = new ListViewItem( mListView );
    item->load( id, rights[id] );
    if ( !col.isValid() ) // new collection? everything is new then
      item->setModified( true );
  }

  OrgKdeAkonadiImapSettingsInterface *imapSettingsInterface = KMail::Util::createImapSettingsInterface( col.resource() );
  if ( imapSettingsInterface->isValid() ) {
    QDBusReply<QString> reply = imapSettingsInterface->userName();
    if ( reply.isValid() ) {
      mImapUserName = reply;
    }
  }
  delete imapSettingsInterface;

  mUserRights = rights[mImapUserName.toUtf8()];
  mStack->setCurrentWidget( mACLWidget );
  slotSelectionChanged();
}

void CollectionAclPage::save(Akonadi::Collection & col)
{
  if ( !mChanged ) {
    return;
  }

  Akonadi::ImapAclAttribute *acls = col.attribute<Akonadi::ImapAclAttribute>();
  QMap<QByteArray, KIMAP::Acl::Rights> rights;

  QTreeWidgetItemIterator it( mListView );
  while ( QTreeWidgetItem* item = *it ) {
    ListViewItem* ACLitem = static_cast<ListViewItem *>( item );

    // we can use job->exec() here, it is not a hot path
    Akonadi::ContactGroupSearchJob *searchJob = new Akonadi::ContactGroupSearchJob( this );
    searchJob->setQuery( Akonadi::ContactGroupSearchJob::Name, ACLitem->userId() );
    searchJob->setLimit( 1 );
    if ( !searchJob->exec() ) {
      ++it;
      continue;
    }

    if ( !searchJob->contactGroups().isEmpty() ) { // it has been a distribution list
      Akonadi::ContactGroupExpandJob *expandJob = new Akonadi::ContactGroupExpandJob( searchJob->contactGroups().first(), this );
      if ( expandJob->exec() ) {
        foreach ( const KABC::Addressee &contact, expandJob->contacts() ) {
          const QByteArray rawEmail = KPIMUtils::extractEmailAddress( contact.preferredEmail().toUtf8() );
          if ( !rawEmail.isEmpty() )
            rights[ rawEmail ] = ACLitem->permissions();
        }
      }
    } else { // it has been a normal contact
      const QByteArray rawEmail = KPIMUtils::extractEmailAddress( ACLitem->userId().toUtf8() );
      if ( !rawEmail.isEmpty() )
        rights[ rawEmail ] = ACLitem->permissions();
    }

    ++it;
  }

  acls->setRights( rights );

  new Akonadi::CollectionModifyJob( col, this );
}


void CollectionAclPage::slotEditACL(QTreeWidgetItem* item)
{
  if ( !item ) return;
  bool canAdmin = ( mUserRights & KIMAP::Acl::Admin );
  // Same logic as in slotSelectionChanged, but this is also needed for double-click IIRC
  if ( canAdmin && item ) {
    // Don't allow users to remove their own admin permissions - there's no way back
    ListViewItem* ACLitem = static_cast<ListViewItem *>( item );
    if ( mImapUserName == ACLitem->userId() && ( ACLitem->permissions() & KIMAP::Acl::Admin ) )
      canAdmin = false;
  }
  if ( !canAdmin ) return;

  ListViewItem* ACLitem = static_cast<ListViewItem *>( mListView->currentItem() );
  MessageViewer::AutoQPointer<ACLEntryDialog> dlg( new ACLEntryDialog( i18n( "Modify Permissions" ),
                                                                       this ) );
  dlg->setValues( ACLitem->userId(), ACLitem->permissions() );
  if ( dlg->exec() == QDialog::Accepted && dlg ) {
    QStringList userIds = dlg->userIds();
    Q_ASSERT( !userIds.isEmpty() ); // impossible, the OK button is disabled in that case
    ACLitem->setUserId( dlg->userIds().front() );
    ACLitem->setPermissions( dlg->permissions() );
    ACLitem->setModified( true );
    mChanged = true;
    if ( userIds.count() > 1 ) { // more emails were added, append them
      userIds.pop_front();
      addACLs( userIds, dlg->permissions() );
    }
  }
}

void CollectionAclPage::slotEditACL()
{
  slotEditACL( mListView->currentItem() );
}

void CollectionAclPage::addACLs( const QStringList& userIds, KIMAP::Acl::Rights permissions )
{
  for( QStringList::const_iterator it = userIds.constBegin(); it != userIds.constEnd(); ++it ) {
    ListViewItem* ACLitem = new ListViewItem( mListView );
    ACLitem->setUserId( *it );
    ACLitem->setPermissions( permissions );
    ACLitem->setModified( true );
    ACLitem->setNew( true );
  }
}

void CollectionAclPage::slotAddACL()
{
  MessageViewer::AutoQPointer<ACLEntryDialog> dlg( new ACLEntryDialog( i18n( "Add Permissions" ),
                                                                       this ) );
  if ( dlg->exec() == QDialog::Accepted && dlg ) {
    const QStringList userIds = dlg->userIds();
    addACLs( dlg->userIds(), dlg->permissions() );
    mChanged = true;
  }
}

void CollectionAclPage::slotSelectionChanged()
{
  QTreeWidgetItem* item = mListView->currentItem();

  bool canAdmin = ( mUserRights & KIMAP::Acl::Admin );
  bool canAdminThisItem = canAdmin;
  if ( canAdmin && item ) {
    // Don't allow users to remove their own admin permissions - there's no way back
    ListViewItem* ACLitem = static_cast<ListViewItem *>( item );
    if ( mImapUserName == ACLitem->userId() && ( ACLitem->permissions() & KIMAP::Acl::Admin ) )
      canAdminThisItem = false;
  }

  bool lvVisible = mStack->currentWidget() == mACLWidget;
  mAddACL->setEnabled( lvVisible && canAdmin );
  mEditACL->setEnabled( item && lvVisible && canAdminThisItem );
  mRemoveACL->setEnabled( item && lvVisible && canAdminThisItem );
}

void CollectionAclPage::slotRemoveACL()
{
  ListViewItem* ACLitem = static_cast<ListViewItem *>( mListView->currentItem() );
  if ( !ACLitem )
    return;
  if ( !ACLitem->isNew() ) {
    if ( mImapUserName == ACLitem->userId() ) {
      if ( KMessageBox::Cancel == KMessageBox::warningContinueCancel( topLevelWidget(),
         i18n( "Do you really want to remove your own permissions for this folder? You will not be able to access it afterwards." ), i18n( "Remove" ) ) )
        return;
    }
    mRemovedACLs.append( ACLitem->userId() );
  }
  delete ACLitem;
  mChanged = true;
}

#include "collectionaclpage.moc"
