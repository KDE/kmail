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

#include <kpushbutton.h>
#include <kdebug.h>

#include <qlayout.h>
#include <qlabel.h>
#include <qvbox.h>
#include <qvbuttongroup.h>
#include <qwidgetstack.h>
#include <qradiobutton.h>
#include <qwhatsthis.h>

#include <assert.h>

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
  { ACLJobs::List | ACLJobs::Read | ACLJobs::Insert, I18N_NOOP2( "Permissions", "Append" ) },
  { ACLJobs::AllWrite, I18N_NOOP2( "Permissions", "Write" ) },
  { ACLJobs::All, I18N_NOOP2( "Permissions", "All" ) }
};


KMail::ACLEntryDialog::ACLEntryDialog( const QString& caption, QWidget* parent, const char* name )
  : KDialogBase( parent, name, true /*modal*/, caption,
                 KDialogBase::Ok|KDialogBase::Cancel, KDialogBase::Ok, true /*sep*/ )
{
  QWidget *page = new QWidget( this );
  setMainWidget(page);
  QGridLayout *topLayout = new QGridLayout( page, 3, 2, 0, spacingHint() );

  QLabel *label = new QLabel( i18n( "&User Identifier" ), page );
  topLayout->addWidget( label, 0, 0 );

  mUserIdLineEdit = new KLineEdit( page );
  topLayout->addWidget( mUserIdLineEdit, 0, 1 );
  label->setBuddy( mUserIdLineEdit );
  QWhatsThis::add( mUserIdLineEdit, i18n( "The User Identifier is the login of the user on the IMAP server. This can be a simple user name or the full email address of the user; the login for your own account on the server will tell you which one it is." ) );

  mButtonGroup = new QVButtonGroup( i18n( "Permissions" ), page );
  topLayout->addMultiCellWidget( mButtonGroup, 1, 1, 0, 1 );

  for ( unsigned int i = 0;
        i < sizeof( standardPermissions ) / sizeof( *standardPermissions );
        ++i ) {
    QRadioButton* cb = new QRadioButton( i18n( "Permissions", standardPermissions[i].userString ), mButtonGroup );
    // We store the permission value (bitfield) as the id of the radiobutton in the group
    mButtonGroup->insert( cb, standardPermissions[i].permissions );
  }
  topLayout->setRowStretch(2, 10);

  connect( mUserIdLineEdit, SIGNAL( textChanged( const QString& ) ), SLOT( slotChanged() ) );
  connect( mButtonGroup, SIGNAL( clicked( int ) ), SLOT( slotChanged() ) );
  enableButtonOK( false );
}

void KMail::ACLEntryDialog::slotChanged()
{
  enableButtonOK( !mUserIdLineEdit->text().isEmpty() && mButtonGroup->selected() != 0 );
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
      mJob( 0 ), mModified( false ) {}

  void load( const ACLJobs::ACLListEntry& entry );

  QString userId() const { return text( 0 ); }
  void setUserId( const QString& userId ) { setText( 0, userId ); }

  unsigned int permissions() const { return mPermissions; }
  void setPermissions( unsigned int permissions );

  bool isModified() const { return mModified; }
  void setModified( bool b ) { mModified = b; }

  KIO::Job* job() const { return mJob; }
  void setJob( KIO::Job* job ) { mJob = job; }

private:
  KIO::Job* mJob;
  unsigned int mPermissions;
  bool mModified;
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

void KMail::FolderDiaACLTab::ListViewItem::load( const ACLJobs::ACLListEntry& entry )
{
  setUserId( entry.userid );
  mPermissions = entry.permissions;
  setText( 1, permissionsToUserString( entry.permissions, entry.internalRightsList ) );
}

////

KMail::FolderDiaACLTab::FolderDiaACLTab( KMFolderDialog* dlg, QWidget* parent, const char* name )
  : FolderDiaTab( parent, name ), mDlg( dlg ),
    mJobCounter( 0 ),
    mChanged( false ), mAccepting( false )
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
  mAddACL = new KPushButton( i18n( "Add entry" ), buttonBox );
  mEditACL = new KPushButton( i18n( "Modify entry" ), buttonBox );
  mRemoveACL = new KPushButton( i18n( "Remove entry" ), buttonBox );
  QSpacerItem* spacer = new QSpacerItem( 0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding );
  static_cast<QBoxLayout *>( buttonBox->layout() )->addItem( spacer );

  connect( mAddACL, SIGNAL( clicked() ), SLOT( slotAddACL() ) );
  connect( mEditACL, SIGNAL( clicked() ), SLOT( slotEditACL() ) );
  connect( mRemoveACL, SIGNAL( clicked() ), SLOT( slotRemoveACL() ) );
  mEditACL->setEnabled( false );
  mRemoveACL->setEnabled( false );

  connect( this, SIGNAL( changed(bool) ), SLOT( slotChanged(bool) ) );
}

// This can probably be removed once KMFolderImap and KMFolderCachedImap have a common base class
KMail::ImapAccountBase* KMail::FolderDiaACLTab::imapAccount() const
{
  KMFolder* folder = mDlg->folder();
  if ( folder->folderType() == KMFolderTypeImap )
    return static_cast<KMFolderImap*>( folder->storage() )->account();
  else if ( folder->folderType() == KMFolderTypeCachedImap )
    return static_cast<KMFolderCachedImap*>( folder->storage() )->account();
  else
    assert( 0 ); // see KMFolderDialog constructor
  return 0;
}

#if 0 // not needed right now
// This can probably be removed once KMFolderImap and KMFolderCachedImap have a common base class
QString KMail::FolderDiaACLTab::imapPath() const
{
  KMFolder* folder = mDlg->folder();
  if ( folder->folderType() == KMFolderTypeImap )
    return static_cast<KMFolderImap*>( folder->storage() )->imapPath();
  else if ( folder->folderType() == KMFolderTypeCachedImap )
    return static_cast<KMFolderCachedImap*>( folder->storage() )->imapPath();
  else
    assert( 0 ); // see KMFolderDialog constructor
  return 0;
}
#endif

// This can probably be removed once KMFolderImap and KMFolderCachedImap have a common base class
KURL KMail::FolderDiaACLTab::imapURL() const
{
  ImapAccountBase* account = imapAccount();
  KURL url = account->getUrl();
  KMFolder* folder = mDlg->folder();
  if ( folder->folderType() == KMFolderTypeImap )
    url.setPath( static_cast<KMFolderImap*>( folder->storage() )->imapPath() );
  else if ( folder->folderType() == KMFolderTypeCachedImap )
    url.setPath( static_cast<KMFolderCachedImap*>( folder->storage() )->imapPath() );
  else
    assert( 0 ); // see KMFolderDialog constructor
  return url;
}

void KMail::FolderDiaACLTab::load()
{
  // First ensure we are connected
  ImapAccountBase* account = imapAccount();
  mLabel->setText( i18n( "Connecting to server %1, please wait..." ).arg( account->host() ) );
  mStack->raiseWidget( mLabel );
  ImapAccountBase::ConnectionState state = account->makeConnection();
  if ( state == ImapAccountBase::Error ) { // Cancelled by user, or slave can't start
    slotConnectionResult( 1 ); // any error code != 0
  } else if ( state == ImapAccountBase::Connecting ) {
    connect( account, SIGNAL( connectionResult(int) ),
             this, SLOT( slotConnectionResult(int) ) );
  } else { // Connected
    slotConnectionResult( 0 );
  }
}

void KMail::FolderDiaACLTab::slotConnectionResult( int errorCode )
{
  ImapAccountBase* account = imapAccount();
  disconnect( account, SIGNAL( connectionResult(int) ),
              this, SLOT( slotConnectionResult(int) ) );
  if ( errorCode ) {
    // Error (error message already shown by the account)
    mLabel->setText( i18n( "Error connecting to server %1" ).arg( account->host() ) );
    return;
  }

  ACLJobs::GetACLJob* job = ACLJobs::getACL( account->slave(), imapURL() );

  ImapAccountBase::jobData jd;
  jd.total = 1; jd.done = 0; jd.parent = NULL;
  account->insertJob(job, jd);

  connect(job, SIGNAL(result(KIO::Job *)),
          SLOT(slotGetACLResult(KIO::Job *)));
}

void KMail::FolderDiaACLTab::slotGetACLResult(KIO::Job *job)
{
  ImapAccountBase* account = imapAccount();
  ImapAccountBase::JobIterator it = account->findJob( job );
  if ( it == account->jobsEnd() ) return;
  account->removeJob( it );
  if ( job->error() ) {
    mLabel->setText( i18n( "Error retrieving access control list (ACL) from server\n%1" ).arg( job->errorString() ) );
    return;
  }

  ACLJobs::GetACLJob* aclJob = static_cast<ACLJobs::GetACLJob *>( job );
  // Now we can populate the listview
  const QValueVector<ACLJobs::ACLListEntry>& aclList = aclJob->entries();
  for( QValueVector<ACLJobs::ACLListEntry>::ConstIterator it = aclList.begin(); it != aclList.end(); ++it ) {
    ListViewItem* item = new ListViewItem( mListView );
    item->load( *it );
  }
  mStack->raiseWidget( mACLWidget );
  slotSelectionChanged( mListView->selectedItem() );
}

void KMail::FolderDiaACLTab::slotEditACL(QListViewItem* item)
{
  if ( !item ) return;
  ListViewItem* ACLitem = static_cast<ListViewItem *>( mListView->currentItem() );
  ACLEntryDialog dlg( i18n( "Modify Permissions" ), this );
  dlg.setValues( ACLitem->userId(), ACLitem->permissions() );
  if ( dlg.exec() == QDialog::Accepted ) {
    ACLitem->setUserId( dlg.userId() );
    ACLitem->setPermissions( dlg.permissions() );
    ACLitem->setModified( true );
    emit changed(true);
  }
}

void KMail::FolderDiaACLTab::slotEditACL()
{
  slotEditACL( mListView->currentItem() );
}

void KMail::FolderDiaACLTab::slotAddACL()
{
  ACLEntryDialog dlg( i18n( "Add Permissions" ), this );
  if ( dlg.exec() == QDialog::Accepted ) {
    ListViewItem* ACLitem = new ListViewItem( mListView );
    ACLitem->setUserId( dlg.userId() );
    ACLitem->setPermissions( dlg.permissions() );
    ACLitem->setModified( true );
    emit changed(true);
  }
}

void KMail::FolderDiaACLTab::slotSelectionChanged(QListViewItem* item)
{
  bool lvVisible = mStack->visibleWidget() == mACLWidget;
  mAddACL->setEnabled( lvVisible );
  mEditACL->setEnabled( item && lvVisible );
  mRemoveACL->setEnabled( item && lvVisible );
}

void KMail::FolderDiaACLTab::ACLJobDone(KIO::Job* job)
{
  --mJobCounter;
  if ( job->error() ) {
    job->showErrorDialog( this );
    if ( mAccepting ) {
      emit cancelAccept();
      mAccepting = false; // don't emit readyForAccept anymore
    }
  }
  if ( mJobCounter == 0 && mAccepting )
    emit readyForAccept();
}

// Called by 'add' and 'edit' jobs fired from save()
void KMail::FolderDiaACLTab::slotSetACLResult(KIO::Job* job)
{
  bool ok = false;
  for ( QListViewItem* item = mListView->firstChild(); item; item = item->nextSibling() ) {
    ListViewItem* ACLitem = static_cast<ListViewItem *>( item );
    if ( ACLitem->job() == job ) {
      if ( !job->error() ) {
        // Success -> reset flags
        ACLitem->setModified( false );
      }
      ACLitem->setJob( 0 );
      ok = true;
      break;
    }
  }
  if ( !ok )
    kdWarning(5006) << k_funcinfo << " no item found for job " << job << endl;
  ACLJobDone( job );
}

void KMail::FolderDiaACLTab::slotDeleteACLResult(KIO::Job* job)
{
  if ( !job->error() ) {
    // Success -> remove from list
    ACLJobs::DeleteACLJob* delJob = static_cast<ACLJobs::DeleteACLJob *>( job );
    Q_ASSERT( mRemovedACLs.contains( delJob->userId() ) );
    mRemovedACLs.remove( delJob->userId() );
  }
  ACLJobDone( job );
}

void KMail::FolderDiaACLTab::slotRemoveACL()
{
  ListViewItem* ACLitem = static_cast<ListViewItem *>( mListView->currentItem() );
  if ( !ACLitem )
    return;
  mRemovedACLs.append( ACLitem->userId() );
  delete ACLitem;
  emit changed(true);
}

bool KMail::FolderDiaACLTab::accept()
{
  if ( !mChanged )
    return true; // no change, ok for accepting the dialog immediately
  // If there were changes, we need to apply them first (which is async)
  mAccepting = true;
  save();
  return false; // i.e. don't close yet
}

void KMail::FolderDiaACLTab::save()
{
  if ( !mChanged )
    return;
  mJobCounter = 0;
  ImapAccountBase* account = imapAccount();
  for ( QListViewItem* item = mListView->firstChild(); item; item = item->nextSibling() ) {
    ListViewItem* ACLitem = static_cast<ListViewItem *>( item );
    if ( ACLitem->isModified() ) {
      kdDebug(5006) << "Modified item: " << ACLitem->userId() << endl;

      KIO::Job* job = ACLJobs::setACL( account->slave(), imapURL(), ACLitem->userId(), ACLitem->permissions() );
      ACLitem->setJob( job );
      ImapAccountBase::jobData jd;
      jd.total = 1; jd.done = 0; jd.parent = NULL;
      account->insertJob(job, jd);

      connect(job, SIGNAL(result(KIO::Job *)),
              SLOT(slotSetACLResult(KIO::Job *)));
      ++mJobCounter;
    }
  }
  for( QStringList::Iterator rit = mRemovedACLs.begin(); rit != mRemovedACLs.end(); ++rit ) {
    kdDebug(5006) << "Removed item: " << (*rit) << endl;
    KIO::Job* job = ACLJobs::deleteACL( account->slave(), imapURL(), (*rit) );
    ImapAccountBase::jobData jd;
    jd.total = 1; jd.done = 0; jd.parent = NULL;
    account->insertJob(job, jd);

    connect(job, SIGNAL(result(KIO::Job *)),
            SLOT(slotDeleteACLResult(KIO::Job *)));
    ++mJobCounter;
  }
}

void KMail::FolderDiaACLTab::slotChanged( bool b )
{
  mChanged = b;
}

#include "folderdiaacltab.moc"
