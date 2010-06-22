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

#include "foldercollection.h"
#include <kdebug.h>
#include "kmkernel.h"
#include "kmmainwidget.h"
#include "util.h"
#include "imapsettings.h"
#include <kpimidentities/identitymanager.h>
#include <kpimidentities/identity.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/collectiondeletejob.h>
#include <kio/jobuidelegate.h>
#include "kmcommands.h"
#include "expirejob.h"
#include "foldershortcutactionmanager.h"

#include <QMutex>
#include <QMutexLocker>
#include <QWeakPointer>

using namespace Akonadi;

static QMutex mapMutex;
static QMap<Collection::Id,QWeakPointer<FolderCollection> > fcMap;

QSharedPointer<FolderCollection> FolderCollection::forCollection( const Akonadi::Collection& coll )
{
  QMutexLocker lock( &mapMutex );

  QSharedPointer<FolderCollection> sptr = fcMap.value( coll.id() ).toStrongRef();

  if ( !sptr ) {
    sptr = QSharedPointer<FolderCollection>( new FolderCollection( coll, true ) );
    fcMap.insert( coll.id(), sptr );
  }
  return sptr;
}

FolderCollection::FolderCollection( const Akonadi::Collection & col, bool writeconfig )
  : mCollection( col ),
    mExpireMessages( false ),
    mUnreadExpireAge( 28 ),
    mReadExpireAge( 14 ),
    mUnreadExpireUnits( ExpireNever ),
    mReadExpireUnits( ExpireNever ),
    mExpireAction( ExpireDelete ),
    mIgnoreNewMail( false ),
    mPutRepliesInSameFolder( false ),
    mHideInSelectionDialog( false ),
    mWriteConfig( writeconfig ),
    mOldIgnoreNewMail( false )
{
  assert( col.isValid() );
  mIdentity = KMKernel::self()->identityManager()->defaultIdentity().uoid();

  readConfig();
  connect( KMKernel::self()->identityManager(), SIGNAL( changed() ),
           this, SLOT( slotIdentitiesChanged() ) );
}

FolderCollection::~FolderCollection()
{
//   kDebug()<<" FolderCollection::~FolderCollection";
  if ( mWriteConfig )
    writeConfig();
}

QString FolderCollection::name() const
{
  return mCollection.name();
}

bool FolderCollection::isSystemFolder() const
{
  return KMKernel::self()->isSystemFolderCollection( mCollection );
}

bool FolderCollection::isStructural() const
{
  return mCollection.contentMimeTypes().isEmpty();
}

bool FolderCollection::isReadOnly() const
{
  return mCollection.rights() & Akonadi::Collection::ReadOnly;
}

bool FolderCollection::canDeleteMessages() const
{
  return mCollection.rights() & Akonadi::Collection::CanDeleteItem;
}

bool FolderCollection::canCreateMessages() const
{
  return mCollection.rights() & Akonadi::Collection::CanCreateItem;
}

qint64 FolderCollection::count() const
{
  return mCollection.statistics().count();
}

Akonadi::Collection::Rights FolderCollection::rights() const
{
  return mCollection.rights();
}

Akonadi::CollectionStatistics FolderCollection::statistics() const
{
  return mCollection.statistics();
}

Akonadi::Collection FolderCollection::collection()
{
  return mCollection;
}

void FolderCollection::slotIdentitiesChanged()
{
  uint defaultIdentity = KMKernel::self()->identityManager()->defaultIdentity().uoid();
  // The default identity may have changed, therefore set it again
  // if necessary
  if ( mUseDefaultIdentity )
    mIdentity = defaultIdentity;

  // Fall back to the default identity if the one used currently is invalid
  if ( KMKernel::self()->identityManager()->identityForUoid( mIdentity ).isNull() ) {
    mIdentity = defaultIdentity;
    mUseDefaultIdentity = true;
  }
}

QString FolderCollection::configGroupName() const
{
  return "Folder-"+ QString::number( mCollection.id() );
}

void FolderCollection::readConfig()
{
  const KConfigGroup configGroup( KMKernel::config(), configGroupName() );
  mExpireMessages = configGroup.readEntry( "ExpireMessages", false );
  mReadExpireAge = configGroup.readEntry( "ReadExpireAge", 3 );
  mReadExpireUnits = (ExpireUnits)configGroup.readEntry( "ReadExpireUnits", (int)ExpireMonths );
  mUnreadExpireAge = configGroup.readEntry( "UnreadExpireAge", 12 );
  mUnreadExpireUnits = (ExpireUnits)
      configGroup.readEntry( "UnreadExpireUnits", (int)ExpireNever );
  mExpireAction = configGroup.readEntry( "ExpireAction", "Delete") == "Move" ? ExpireMove : ExpireDelete;
  mExpireToFolderId = configGroup.readEntry( "ExpireToFolder" );

  mMailingListEnabled = configGroup.readEntry( "MailingListEnabled", false );
  mMailingList.readConfig( configGroup );

  mUseDefaultIdentity = configGroup.readEntry( "UseDefaultIdentity", true );
  uint defaultIdentity = KMKernel::self()->identityManager()->defaultIdentity().uoid();
  mIdentity = configGroup.readEntry("Identity", defaultIdentity );
  slotIdentitiesChanged();

  mPutRepliesInSameFolder = configGroup.readEntry( "PutRepliesInSameFolder", false );
  mHideInSelectionDialog = configGroup.readEntry( "HideInSelectionDialog", false );
  mIgnoreNewMail = configGroup.readEntry( "IgnoreNewMail", false );
  mOldIgnoreNewMail = mIgnoreNewMail;


  QString shortcut( configGroup.readEntry( "Shortcut" ) );
  if ( !shortcut.isEmpty() ) {
    KShortcut sc( shortcut );
    setShortcut( sc, 0 );
  }
}

bool FolderCollection::isValid() const
{
  return mCollection.isValid();
}

QString FolderCollection::idString() const
{
  return QString::number( mCollection.id() );
}

void FolderCollection::writeConfig() const
{
  KConfigGroup configGroup( KMKernel::config(), configGroupName() );
  configGroup.writeEntry("ExpireMessages", mExpireMessages);
  configGroup.writeEntry("ReadExpireAge", mReadExpireAge);
  configGroup.writeEntry("ReadExpireUnits", (int)mReadExpireUnits);
  configGroup.writeEntry("UnreadExpireAge", mUnreadExpireAge);
  configGroup.writeEntry("UnreadExpireUnits", (int)mUnreadExpireUnits);
  configGroup.writeEntry("ExpireAction", mExpireAction == ExpireDelete ? "Delete" : "Move");
  configGroup.writeEntry("ExpireToFolder", mExpireToFolderId);

  configGroup.writeEntry("MailingListEnabled", mMailingListEnabled);
  mMailingList.writeConfig( configGroup );


  configGroup.writeEntry( "UseDefaultIdentity", mUseDefaultIdentity );

  if ( !mUseDefaultIdentity ) {
    int identityId = -1;
    OrgKdeAkonadiImapSettingsInterface *imapSettingsInterface = KMail::Util::createImapSettingsInterface( mCollection.resource() );
    if ( imapSettingsInterface->isValid() ) {
      QDBusReply<int> reply = imapSettingsInterface->accountIdentity();
      if ( reply.isValid() ) {
        identityId = reply;
      }
    }
    delete imapSettingsInterface;
    if ( identityId > -1 && mIdentity != static_cast<uint>( identityId ) )
      configGroup.writeEntry("Identity", mIdentity);
    else
      configGroup.deleteEntry( "Identity" );
  }
  else
      configGroup.deleteEntry("Identity");

  configGroup.writeEntry( "PutRepliesInSameFolder", mPutRepliesInSameFolder );
  configGroup.writeEntry( "HideInSelectionDialog", mHideInSelectionDialog );
  configGroup.writeEntry( "IgnoreNewMail", mIgnoreNewMail );
  if ( !mShortcut.isEmpty() )
    configGroup.writeEntry( "Shortcut", mShortcut.toString() );
  else
    configGroup.deleteEntry( "Shortcut" );
  if ( mIgnoreNewMail != mOldIgnoreNewMail ) {
    KMKernel::self()->updateSystemTray();
  }
}

void FolderCollection::setShortcut( const KShortcut &sc, KMMainWidget *main )
{
  if ( mShortcut != sc ) {
    mShortcut = sc;
    if ( main) {
      main->folderShortcutActionManager()->shortcutChanged( mCollection );
    }
  }
}

void FolderCollection::setUseDefaultIdentity( bool useDefaultIdentity )
{
  mUseDefaultIdentity = useDefaultIdentity;
  if ( mUseDefaultIdentity )
    mIdentity = KMKernel::self()->identityManager()->defaultIdentity().uoid();
  KMKernel::self()->slotRequestConfigSync();
}

void FolderCollection::setIdentity( uint identity )
{
  mIdentity = identity;
  KMKernel::self()->slotRequestConfigSync();
}

uint FolderCollection::identity() const
{
  if ( mUseDefaultIdentity ) {
    int identityId = -1;
    OrgKdeAkonadiImapSettingsInterface *imapSettingsInterface = KMail::Util::createImapSettingsInterface( mCollection.resource() );
    if ( imapSettingsInterface->isValid() ) {
      QDBusReply<bool> useDefault = imapSettingsInterface->useDefaultIdentity();
      if( useDefault.isValid() && useDefault.value() )
        return mIdentity;
      
       QDBusReply<int> remoteAccountIdent = imapSettingsInterface->accountIdentity();
      if ( remoteAccountIdent.isValid() && remoteAccountIdent.value() > 0 ) {
        identityId = remoteAccountIdent;
      }
    }
    delete imapSettingsInterface;
    if ( identityId != -1 && !KMKernel::self()->identityManager()->identityForUoid( identityId ).isNull() )
      return identityId;
  }
  return mIdentity;
}


void FolderCollection::setAutoExpire( bool enabled )
{
  if( enabled != mExpireMessages ) {
    mExpireMessages = enabled;
    writeConfig();
  }
}

void FolderCollection::setUnreadExpireAge( int age )
{
  if( age >= 0 && age != mUnreadExpireAge ) {
    mUnreadExpireAge = age;
    writeConfig();
  }
}

void FolderCollection::setUnreadExpireUnits( ExpireUnits units )
{
  if (units >= ExpireNever && units < ExpireMaxUnits) {
    mUnreadExpireUnits = units;
    writeConfig();
  }
}

void FolderCollection::setReadExpireAge( int age )
{
  if( age >= 0 && age != mReadExpireAge ) {
    mReadExpireAge = age;
    writeConfig();
  }
}

void FolderCollection::setReadExpireUnits( ExpireUnits units )
{
  if (units >= ExpireNever && units <= ExpireMaxUnits) {
    mReadExpireUnits = units;
    writeConfig();
  }
}

QString FolderCollection::mailingListPostAddress() const
{
  if ( mMailingList.features() & MailingList::Post ) {
    KUrl::List::const_iterator it;
    KUrl::List post = mMailingList.postURLS();
    for( it = post.constBegin(); it != post.constEnd(); ++it ) {
      // We check for isEmpty because before 3.3 postAddress was just an
      // email@kde.org and that leaves protocol() field in the kurl class
      if ( (*it).protocol() == "mailto" || (*it).protocol().isEmpty() )
        return (*it).path();
    }
  }
  return QString();
}

void FolderCollection::setExpireAction( ExpireAction a )
{
  if ( a != mExpireAction ) {
    mExpireAction = a;
    writeConfig();
  }
}

void FolderCollection::setExpireToFolderId( const QString& id )
{
  if ( id != mExpireToFolderId ) {
    mExpireToFolderId = id;
    writeConfig();
  }
}

void FolderCollection::setMailingListEnabled( bool enabled )
{
  mMailingListEnabled = enabled;
  writeConfig();
}

void FolderCollection::setMailingList( const MailingList& mlist )
{
  mMailingList = mlist;
  writeConfig();
}

static int daysToExpire( int number, FolderCollection::ExpireUnits units )
{
  switch (units) {
  case FolderCollection::ExpireDays: // Days
    return number;
  case FolderCollection::ExpireWeeks: // Weeks
    return number * 7;
  case FolderCollection::ExpireMonths: // Months - this could be better rather than assuming 31day months.
    return number * 31;
  default: // this avoids a compiler warning (not handled enumeration values)
    ;
  }
  return -1;
}

void FolderCollection::daysToExpire(int& unreadDays, int& readDays) {
  unreadDays = ::daysToExpire( getUnreadExpireAge(), getUnreadExpireUnits() );
  readDays = ::daysToExpire( getReadExpireAge(), getReadExpireUnits() );
}


void FolderCollection::markNewAsUnread()
{
  if ( mCollection.isValid() ) {
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( mCollection,this );
    connect( job, SIGNAL( result( KJob* ) ), this, SLOT( slotMarkNewAsUnreadfetchDone( KJob* ) ) );
  }
}

void FolderCollection::slotMarkNewAsUnreadfetchDone( KJob * job )
{
  if ( job->error() )
    return;

  Akonadi::ItemFetchJob *fjob = dynamic_cast<Akonadi::ItemFetchJob*>( job );
  Q_ASSERT( fjob );

  Akonadi::Item::List items;
  foreach( const Akonadi::Item &item, fjob->items() ) {
    MessageStatus status;
    status.setStatusFromFlags( item.flags() );
    if ( !status.isNew() ) {
      items.append( item );
    }
  }

  if ( items.empty() )
    return;
  KMCommand *command = new KMSetStatusCommand( MessageStatus::statusUnread(), items );
  command->start();
}

void FolderCollection::markUnreadAsRead()
{
  if ( mCollection.isValid() ) {
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( mCollection,this );
    job->fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
    connect( job, SIGNAL( result( KJob* ) ), this, SLOT( slotMarkNewAsReadfetchDone( KJob* ) ) );
  }
}

void FolderCollection::slotMarkNewAsReadfetchDone( KJob * job)
{
  if ( job->error() )
    return;

  Akonadi::ItemFetchJob *fjob = dynamic_cast<Akonadi::ItemFetchJob*>( job );
  Q_ASSERT( fjob );
  Akonadi::Item::List items;
  foreach( const Akonadi::Item &item, fjob->items() ) {
    MessageStatus status;
    status.setStatusFromFlags( item.flags() );
    if (status.isNew() || status.isUnread()) {
      items.append( item );
    }
  }
  if ( items.empty() )
    return;
  KMCommand *command = new KMSetStatusCommand( MessageStatus::statusRead(), items );
  command->start();
}


void FolderCollection::removeCollection()
{
  Akonadi::CollectionDeleteJob *job = new Akonadi::CollectionDeleteJob( mCollection );
  connect( job, SIGNAL( result( KJob* ) ), this, SLOT( slotDeletionCollectionResult( KJob* ) ) );
}

void FolderCollection::slotDeletionCollectionResult( KJob * job )
{
  if ( job->error() ) {
    if ( static_cast<KIO::Job*>( job )->ui() ) {
      // handle errors
      static_cast<KIO::Job*>(job)->ui()->showErrorMessage();
    }
  }
}

void FolderCollection::expireOldMessages( bool immediate )
{
  KMail::ScheduledExpireTask* task = new KMail::ScheduledExpireTask(mCollection, immediate);
  kmkernel->jobScheduler()->registerTask( task );
}


