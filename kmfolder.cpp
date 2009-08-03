/* -*- mode: C++; c-file-style: "gnu" -*-
 * kmail: KDE mail client
 * Copyright (c) 1996-1998 Stefan Taferner <taferner@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "kmfolder.h"
#include "kmfolderdir.h"
#include "kmfoldermbox.h"
#include "folderstorage.h"
#include "kmfoldercachedimap.h"
#include "kmfoldersearch.h"
#include "kmfolderimap.h"
#include "kmfoldermgr.h"
#include <kpimidentities/identitymanager.h>
#include <kpimidentities/identity.h>
#include "expirejob.h"
#include "compactionjob.h"
#include "kmailicalifaceimpl.h"
#include "kmaccount.h"

#include <errno.h>
#include <unistd.h> // W_OK

#include <kdebug.h>
#include <kde_file.h> // KDE_mkdir
#include <klocale.h>
#include <kshortcut.h>
#include <kmessagebox.h>

#include <QFile>
#include <QFileInfo>
#include <QList>

KMFolder::KMFolder( KMFolderDir* aParent, const QString& aFolderName,
                    KMFolderType aFolderType, bool withIndex, bool exportedSernums )
  : KMFolderNode( aParent, aFolderName ), mStorage(0),
    mChild( 0 ),
    mIsSystemFolder( false ),
    mHasIndex( withIndex ),
    mExportsSernums( exportedSernums ),
    mMoveInProgress( false ),
    mExpireMessages( false ), mUnreadExpireAge( 28 ),
    mReadExpireAge( 14 ), mUnreadExpireUnits( expireNever ),
    mReadExpireUnits( expireNever ),
    mExpireAction( ExpireDelete ),
    mUseCustomIcons( false ), mMailingListEnabled( false ),
    mAcctList( 0 ),
    mPutRepliesInSameFolder( false ),
    mHideInSelectionDialog( false ),
    mIgnoreNewMail( false )
{
  mIdentity = kmkernel->identityManager()->defaultIdentity().uoid();
  if( aFolderType == KMFolderTypeCachedImap )
    mStorage = new KMFolderCachedImap( this, aFolderName.toLatin1() );
  else if( aFolderType == KMFolderTypeImap )
    mStorage = new KMFolderImap( this, aFolderName.toLatin1() );
  else if( aFolderType == KMFolderTypeMaildir )
    mStorage = new KMFolderMaildir( this, aFolderName.toLatin1() );
  else if( aFolderType == KMFolderTypeSearch )
    mStorage = new KMFolderSearch( this, aFolderName.toLatin1() );
  else
    mStorage = new KMFolderMbox( this, aFolderName.toLatin1() );

  assert( mStorage );

  QFileInfo dirinfo;
  dirinfo.setFile( mStorage->location() );
  if ( !dirinfo.exists() ) {
    int rc = mStorage->create();
    QString msg = i18n("<qt>Error while creating file <b>%1</b>:<br />%2</qt>", aFolderName, strerror(rc));
    if ( rc ) {
      KMessageBox::information(0, msg);
    }
  }

  if ( aParent ) {
    connect( mStorage, SIGNAL( msgAdded( KMFolder*, quint32 ) ),
             aParent->manager(), SIGNAL( msgAdded( KMFolder*, quint32 ) ) );
    connect( mStorage, SIGNAL( msgRemoved( KMFolder*, quint32 ) ),
             parent()->manager(), SIGNAL( msgRemoved( KMFolder*, quint32 ) ) );
    connect( this, SIGNAL( msgChanged( KMFolder*, quint32, int ) ),
             parent()->manager(), SIGNAL( msgChanged( KMFolder*, quint32, int ) ) );
    connect( this, SIGNAL( msgHeaderChanged( KMFolder*,  int ) ),
             parent()->manager(), SIGNAL( msgHeaderChanged( KMFolder*, int ) ) );
    connect( mStorage, SIGNAL( invalidated( KMFolder* ) ),
             parent()->manager(), SIGNAL( folderInvalidated( KMFolder* ) ) );
  }

  // Resend all mStorage signals
  connect( mStorage, SIGNAL( changed() ), SIGNAL( changed() ) );
  connect( mStorage, SIGNAL( compacted() ), SIGNAL( compacted() ) );
  connect( mStorage, SIGNAL( cleared() ), SIGNAL( cleared() ) );
  connect( mStorage, SIGNAL( expunged( KMFolder* ) ),
           SIGNAL( expunged( KMFolder* ) ) );
  connect( mStorage, SIGNAL( nameChanged() ), SIGNAL( nameChanged() ) );
  connect( mStorage, SIGNAL( msgRemoved( KMFolder*, quint32 ) ),
           SIGNAL( msgRemoved( KMFolder*, quint32 ) ) );
  connect( mStorage, SIGNAL( msgRemoved( int, const QString& ) ),
           SIGNAL( msgRemoved( int, const QString& ) ) );
  connect( mStorage, SIGNAL( msgRemoved( KMFolder* ) ),
           SIGNAL( msgRemoved( KMFolder* ) ) );
  connect( mStorage, SIGNAL( msgAdded( int ) ), SIGNAL( msgAdded( int ) ) );
  connect( mStorage, SIGNAL( msgAdded( KMFolder*, quint32 ) ),
           SIGNAL( msgAdded( KMFolder*, quint32 ) ) );
  connect( mStorage, SIGNAL( msgChanged( KMFolder*, quint32, int ) ),
           SIGNAL( msgChanged( KMFolder*, quint32, int ) ) );
  connect( mStorage, SIGNAL( msgHeaderChanged( KMFolder*, int ) ),
           SIGNAL( msgHeaderChanged( KMFolder*, int ) ) );
  connect( mStorage, SIGNAL( statusMsg( const QString& ) ),
           SIGNAL( statusMsg( const QString& ) ) );
  connect( mStorage, SIGNAL( numUnreadMsgsChanged( KMFolder* ) ),
           SIGNAL( numUnreadMsgsChanged( KMFolder* ) ) );
  connect( mStorage, SIGNAL( removed( KMFolder*, bool ) ),
           SIGNAL( removed( KMFolder*, bool ) ) );
  connect( mStorage, SIGNAL(noContentChanged()),
           SIGNAL(noContentChanged()) );

  connect( mStorage, SIGNAL( contentsTypeChanged( KMail::FolderContentsType ) ),
                this, SLOT( slotContentsTypeChanged( KMail::FolderContentsType ) ) );

  connect( mStorage, SIGNAL( folderSizeChanged() ),
           this, SLOT( slotFolderSizeChanged() ) );

  connect( kmkernel->identityManager(), SIGNAL( changed() ),
           this, SLOT( slotIdentitiesChanged() ) );

  //FIXME: Centralize all the readConfig calls somehow - Zack
  // Meanwhile, readConfig must be done before registerWithMessageDict, since
  // that one can call writeConfig in some circumstances - David
  mStorage->readConfig();

   // trigger from here, since it needs a fully constructed FolderStorage
  if ( mExportsSernums )
    mStorage->registerWithMessageDict();
  if ( !mHasIndex )
    mStorage->setAutoCreateIndex( false );

  if ( mId == 0 && aParent )
    mId = aParent->manager()->createId();
}

KMFolder::~KMFolder()
{
  mStorage->close( "~KMFolder", true );
  delete mAcctList;
  if ( mHasIndex ) mStorage->deregisterFromMessageDict();
  delete mStorage;
}

bool KMFolder::hasDescendant( KMFolder *fld ) const
{
  if ( !fld )
    return false;
  KMFolderDir * pdir = fld->parent();
  if ( !pdir )
    return false;
  fld = pdir->owner();
  if ( fld == this )
    return true;
  return hasDescendant( fld );
}

KMFolder * KMFolder::ownerFolder() const
{
  if ( !mParent )
    return 0;
  return mParent->owner();
}


void KMFolder::readConfig( KConfigGroup & configGroup )
{
  // KConfigGroup configGroup(config, "");
  if ( !configGroup.readEntry( "SystemLabel" ).isEmpty() )
    mSystemLabel = configGroup.readEntry( "SystemLabel" );
  mExpireMessages = configGroup.readEntry( "ExpireMessages", false );
  mReadExpireAge = configGroup.readEntry( "ReadExpireAge", 3 );
  mReadExpireUnits = (ExpireUnits)configGroup.readEntry( "ReadExpireUnits", (int)expireMonths );
  mUnreadExpireAge = configGroup.readEntry( "UnreadExpireAge", 12 );
  mUnreadExpireUnits = (ExpireUnits)
      configGroup.readEntry( "UnreadExpireUnits", (int)expireNever );
  mExpireAction = configGroup.readEntry( "ExpireAction", "Delete") == "Move" ? ExpireMove : ExpireDelete;
  mExpireToFolderId = configGroup.readEntry( "ExpireToFolder" );

  mUseCustomIcons = configGroup.readEntry( "UseCustomIcons", false );
  mNormalIconPath = configGroup.readEntry( "NormalIconPath" );
  mUnreadIconPath = configGroup.readEntry( "UnreadIconPath" );

  mMailingListEnabled = configGroup.readEntry( "MailingListEnabled", false );
  mMailingList.readConfig( configGroup );

  mUseDefaultIdentity = configGroup.readEntry( "UseDefaultIdentity", true );
  uint defaultIdentity = kmkernel->identityManager()->defaultIdentity().uoid();
  mIdentity = configGroup.readEntry("Identity", defaultIdentity );
  slotIdentitiesChanged();

  setUserWhoField( configGroup.readEntry( "WhoField" ), false );
  uint savedId = configGroup.readEntry( "Id", 0 );
  // make sure that we don't overwrite a valid id
  if ( savedId != 0 && mId == 0 )
    mId = savedId;
  mPutRepliesInSameFolder = configGroup.readEntry( "PutRepliesInSameFolder", false );
  mHideInSelectionDialog = configGroup.readEntry( "HideInSelectionDialog", false );
  mIgnoreNewMail = configGroup.readEntry( "IgnoreNewMail", false );

  if ( mUseCustomIcons )
    emit iconsChanged();

  QString shortcut( configGroup.readEntry( "Shortcut" ) );
  if ( !shortcut.isEmpty() ) {
    KShortcut sc( shortcut );
    setShortcut( sc );
  }
}

void KMFolder::writeConfig( KConfigGroup & configGroup ) const
{
  // KConfigGroup configGroup(config, "");
  configGroup.writeEntry("SystemLabel", mSystemLabel);
  configGroup.writeEntry("ExpireMessages", mExpireMessages);
  configGroup.writeEntry("ReadExpireAge", mReadExpireAge);
  configGroup.writeEntry("ReadExpireUnits", (int)mReadExpireUnits);
  configGroup.writeEntry("UnreadExpireAge", mUnreadExpireAge);
  configGroup.writeEntry("UnreadExpireUnits", (int)mUnreadExpireUnits);
  configGroup.writeEntry("ExpireAction", mExpireAction == ExpireDelete ? "Delete" : "Move");
  configGroup.writeEntry("ExpireToFolder", mExpireToFolderId);

  configGroup.writeEntry("UseCustomIcons", mUseCustomIcons);
  configGroup.writeEntry("NormalIconPath", mNormalIconPath);
  configGroup.writeEntry("UnreadIconPath", mUnreadIconPath);

  configGroup.writeEntry("MailingListEnabled", mMailingListEnabled);
  mMailingList.writeConfig( configGroup );


  configGroup.writeEntry( "UseDefaultIdentity", mUseDefaultIdentity );
  if ( !mUseDefaultIdentity && ( !mStorage || !mStorage->account() ||
                           mIdentity != mStorage->account()->identityId() ) )
      configGroup.writeEntry("Identity", mIdentity);
  else
      configGroup.deleteEntry("Identity");

  configGroup.writeEntry("WhoField", mUserWhoField);
  configGroup.writeEntry("Id", mId);
  configGroup.writeEntry( "PutRepliesInSameFolder", mPutRepliesInSameFolder );
  configGroup.writeEntry( "HideInSelectionDialog", mHideInSelectionDialog );
  configGroup.writeEntry( "IgnoreNewMail", mIgnoreNewMail );
  if ( !mShortcut.isEmpty() )
    configGroup.writeEntry( "Shortcut", mShortcut.toString() );
  else
    configGroup.deleteEntry( "Shortcut" );
}

KMFolderType KMFolder::folderType() const
{
  return mStorage ? mStorage->folderType() : KMFolderTypeUnknown;
}

QString KMFolder::fileName() const
{
  return mStorage ? mStorage->fileName() : QString();
}

QString KMFolder::location() const
{
  return mStorage ? mStorage->location() : QString();
}

QString KMFolder::indexLocation() const
{
  return mStorage ? mStorage->indexLocation() : QString();
}

QString KMFolder::sortedLocation() const
{
  return mStorage ? mStorage->sortedLocation() : QString();
}

QString KMFolder::idsLocation() const
{
  return mStorage ? mStorage->idsLocation() : QString();
}

QString KMFolder::subdirLocation() const
{
  QString sLocation( path() );

  if( !sLocation.isEmpty() )
    sLocation += '/';
  sLocation += '.' + FolderStorage::dotEscape( fileName() ) + ".directory";

  return sLocation;
}

KMFolderDir* KMFolder::createChildFolder()
{
  if ( mChild ) {
    return mChild;
  }

  QString childName = '.' + fileName() + ".directory";
  QString childDir = path() + '/' + childName;
  if ( access( QFile::encodeName( childDir ), W_OK ) != 0 ) {
    // childDir does not exist or is not writable, so create it.
    if ( KDE_mkdir( QFile::encodeName(childDir), S_IRWXU ) != 0 &&
         chmod( QFile::encodeName(childDir), S_IRWXU) != 0 ) {
      QString wmsg = QString( " '%1': %2" ).arg( childDir ).arg( strerror( errno ) );
      KMessageBox::information( 0, i18n( "Failed to create folder" ) + wmsg );
      return 0;
    }
  }

  KMFolderDirType newType = KMStandardDir;
  if ( folderType() == KMFolderTypeCachedImap ) {
    newType = KMDImapDir;
  } else if ( folderType() == KMFolderTypeImap ) {
    newType = KMImapDir;
  }

  mChild = new KMFolderDir( this, parent(), childName, newType );
  if( !mChild )
    return 0;
  mChild->reload();
  parent()->append( mChild );
  return mChild;
}

void KMFolder::setChild( KMFolderDir* aChild )
{
  mChild = aChild;
  mStorage->updateChildrenState();
}

bool KMFolder::noContent() const
{
  return mStorage ? mStorage->noContent() : true;
}

void KMFolder::setNoContent( bool aNoContent )
{
  mStorage->setNoContent( aNoContent );
}

bool KMFolder::noChildren() const
{
  return mStorage->noChildren();
}

void KMFolder::setNoChildren( bool aNoChildren )
{
  mStorage->setNoChildren( aNoChildren );
}

KMMessage* KMFolder::getMsg( int idx )
{
  return mStorage->getMsg( idx );
}

KMMsgInfo* KMFolder::unGetMsg( int idx )
{
  return mStorage->unGetMsg( idx );
}

bool KMFolder::isMessage( int idx )
{
  return mStorage->isMessage( idx );
}

DwString KMFolder::getDwString( int idx )
{
  return mStorage->getDwString( idx );
}

void KMFolder::ignoreJobsForMessage( KMMessage* m )
{
  mStorage->ignoreJobsForMessage( m );
}

FolderJob* KMFolder::createJob( KMMessage *msg, FolderJob::JobType jt,
                                KMFolder *folder, const QString &partSpecifier,
                                const AttachmentStrategy *as ) const
{
  return mStorage->createJob( msg, jt, folder, partSpecifier, as );
}

FolderJob* KMFolder::createJob( QList<KMMessage*>& msgList,
                                const QString& sets,
                                FolderJob::JobType jt, KMFolder *folder ) const
{
  return mStorage->createJob( msgList, sets, jt, folder );
}

const KMMsgBase* KMFolder::getMsgBase( int idx ) const
{
  return mStorage->getMsgBase( idx );
}

KMMsgBase* KMFolder::getMsgBase( int idx )
{
  return mStorage->getMsgBase( idx );
}

const KMMsgBase* KMFolder::operator[]( int idx ) const
{
  return mStorage->operator[]( idx );
}

KMMsgBase* KMFolder::operator[]( int idx )
{
  return mStorage->operator[]( idx );
}

KMMessage* KMFolder::take( int idx )
{
  return mStorage->take( idx );
}

void KMFolder::takeMessages( const QList<KMMessage*>& msgList )
{
  mStorage->takeMessages( msgList );
}

int KMFolder::addMsg( KMMessage* msg, int* index_return )
{
  return mStorage->addMsg( msg, index_return );
}

int KMFolder::addMsgKeepUID( KMMessage* msg, int* index_return )
{
  return mStorage->addMsgKeepUID( msg, index_return );
}

int KMFolder::addMessages( QList<KMMessage*>& list, QList<int>& index_return )
{
  return mStorage->addMessages( list, index_return );
}

void KMFolder::emitMsgAddedSignals( int idx )
{
  mStorage->emitMsgAddedSignals( idx );
}

void KMFolder::removeMsg( int i, bool imapQuiet )
{
  mStorage->removeMsg( i, imapQuiet );
}

void KMFolder::removeMessages( QList<KMMessage*> msgList, bool imapQuiet ) // TODO const ref
{
  mStorage->removeMessages( msgList, imapQuiet );
}

int KMFolder::expungeOldMsg( int days )
{
  return mStorage->expungeOldMsg( days );
}

int KMFolder::moveMsg( KMMessage* msg, int* index_return )
{
  return mStorage->moveMsg( msg, index_return );
}

int KMFolder::moveMsg(QList<KMMessage*> q, int* index_return )
{
  return mStorage->moveMsg( q, index_return );
}

int KMFolder::find( const KMMsgBase* msg ) const
{
  return mStorage->find( msg );
}

int KMFolder::find( const KMMessage* msg ) const
{
  return mStorage->find( msg );
}

int KMFolder::count( bool cache ) const
{
  return mStorage->count( cache );
}

int KMFolder::countUnread()
{
  return mStorage->countUnread();
}

int KMFolder::countUnreadRecursive()
{
  KMFolder *folder;
  int count = countUnread();
  KMFolderDir *dir = child();
  if (!dir)
    return count;

  QList<KMFolderNode*>::const_iterator it;
  for ( it = dir->constBegin(); it != dir->constEnd(); ++it )
    if ( !( (*it)->isDir() ) ) {
      folder = static_cast<KMFolder*>( (*it) );
      count += folder->countUnreadRecursive();
    }

  return count;
}

void KMFolder::msgStatusChanged( const MessageStatus& oldStatus,
                                 const MessageStatus& newStatus, int idx )
{
  mStorage->msgStatusChanged( oldStatus, newStatus, idx );
}

void KMFolder::msgTagListChanged( int idx )
{
  emit msgHeaderChanged( this, idx );
}


int KMFolder::open( const char *owner )
{
  return mStorage->open( owner );
}

bool KMFolder::canAccess() const
{
  return mStorage->canAccess();
}

void KMFolder::close( const char *owner, bool force )
{
  // do not emit closed() in here - as this would regain too early
  mStorage->close( owner, force );
}

void KMFolder::sync()
{
  mStorage->sync();
}

bool KMFolder::isOpened() const
{
  return mStorage->isOpened();
}

void KMFolder::markNewAsUnread()
{
  mStorage->markNewAsUnread();
}

void KMFolder::markUnreadAsRead()
{
  mStorage->markUnreadAsRead();
}

void KMFolder::remove()
{
  /* The storage needs to be open before remove is called, otherwise
     it will not unregister the corresponding serial numbers from
     the message dict, since its message list is empty, and the .ids
     file contents are not loaded. That can lead to lookups in the
     dict returning stale pointers to the folder later. */
  mStorage->open("folderremoval");
  mStorage->remove();
}

int KMFolder::expunge()
{
  return mStorage->expunge();
}

int KMFolder::rename( const QString& newName, KMFolderDir *aParent )
{
  return mStorage->rename( newName, aParent );
}

bool KMFolder::dirty() const
{
  return mStorage->dirty();
}

void KMFolder::setDirty( bool f )
{
  mStorage->setDirty( f );
}

bool KMFolder::needsCompacting() const
{
  return mStorage->needsCompacting();
}

void KMFolder::setNeedsCompacting( bool f )
{
  mStorage->setNeedsCompacting( f );
}

void KMFolder::quiet( bool beQuiet )
{
  mStorage->quiet( beQuiet );
}

bool KMFolder::isReadOnly() const
{
  return mStorage->isReadOnly();
}

bool KMFolder::isWritable() const
{
  return !mStorage->isReadOnly() && mStorage->canDeleteMessages();
}

bool KMFolder::canDeleteMessages() const
{
  return mStorage->canDeleteMessages();
}

QString KMFolder::label() const
{
  if ( !mSystemLabel.isEmpty() )
     return mSystemLabel;
  if ( !mLabel.isEmpty() )
     return mLabel;
  if ( isSystemFolder() )
     return i18n( name().toUtf8() );
  return name();
}

//-----------------------------------------------------------------------------
QString KMFolder::prettyUrl() const
{
  QString parentUrl;
  if ( parent() )
    parentUrl = parent()->prettyUrl();
  if ( !parentUrl.isEmpty() )
    return parentUrl + '/' + label();
  else
    return label();
}

//--------------------------------------------------------------------------
QString KMFolder::mailingListPostAddress() const
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

void KMFolder::setMailingListEnabled( bool enabled )
{
  mMailingListEnabled = enabled;
  mStorage->writeConfig();
}

void KMFolder::setMailingList( const MailingList& mlist )
{
  mMailingList = mlist;
  mStorage->writeConfig();
}

void KMFolder::setUseDefaultIdentity( bool useDefaultIdentity )
{
  mUseDefaultIdentity = useDefaultIdentity;
  if ( mUseDefaultIdentity )
    mIdentity = kmkernel->identityManager()->defaultIdentity().uoid();
  kmkernel->slotRequestConfigSync();
}

void KMFolder::setIdentity( uint identity )
{
  mIdentity = identity;
  kmkernel->slotRequestConfigSync();
}

uint KMFolder::identity() const
{
  // if we don't have one set ourselves, check our account
  if ( mUseDefaultIdentity && mStorage )
    if ( KMAccount *act = mStorage->account() )
      return act->identityId();
  return mIdentity;
}

void KMFolder::setWhoField(const QString& aWhoField )
{
  mWhoField = aWhoField;
#if 0
  // This isn't saved in the config anyway
  mStorage->writeConfig();
#endif
}

void KMFolder::setUserWhoField( const QString& whoField, bool writeConfig )
{
  if ( mUserWhoField == whoField && !whoField.isEmpty() )
    return;

  if ( whoField.isEmpty() )
  {
    // default setting
    const KPIMIdentities::Identity & identity =
      kmkernel->identityManager()->identityForUoidOrDefault( mIdentity );

    if ( isSystemFolder() && folderType() != KMFolderTypeImap ) {
      // local system folders
      if ( this == kmkernel->inboxFolder() ||
           this == kmkernel->trashFolder() )
        mWhoField = "From";
      if ( this == kmkernel->outboxFolder() ||
           this == kmkernel->sentFolder() ||
           this == kmkernel->templatesFolder() ||
           this == kmkernel->draftsFolder() )
        mWhoField = "To";
    } else if ( identity.drafts() == idString() ||
                identity.templates() == idString() ||
                identity.fcc() == idString() )
      // drafts, templates or sent of the identity
      mWhoField = "To";
    else
      mWhoField = "From";
  } else if ( whoField == "From" || whoField == "To" )
    // set the whoField according to the user-setting
    mWhoField = whoField;
  else {
    // this should not happen...
    kDebug() << "Illegal setting" << whoField << "for userWhoField!";
    return; // don't use the value
  }
  mUserWhoField = whoField;

  if (writeConfig)
    mStorage->writeConfig();
  emit viewConfigChanged();
}

void KMFolder::correctUnreadMsgsCount()
{
  mStorage->correctUnreadMsgsCount();
}

QString KMFolder::idString() const
{
  KMFolderNode* folderNode = parent();
  if (!folderNode)
    return "";
  while ( folderNode->parent() )
    folderNode = folderNode->parent();
  QString myPath = path();
  int pathLen = myPath.length() - folderNode->path().length();
  QString relativePath = myPath.right( pathLen );
  if (!relativePath.isEmpty())
    relativePath = relativePath.right( relativePath.length() - 1 ) + '/';
  QString escapedName = name();
  /* Escape [ and ] as they are disallowed for kconfig sections and that is
     what the idString is primarily used for. */
  escapedName.replace( '[', "%(" );
  escapedName.replace( ']', "%)" );
  return relativePath + escapedName;
}

void KMFolder::setAutoExpire( bool enabled )
{
  if( enabled != mExpireMessages ) {
    mExpireMessages = enabled;
    mStorage->writeConfig();
  }
}

void KMFolder::setUnreadExpireAge( int age )
{
  if( age >= 0 && age != mUnreadExpireAge ) {
    mUnreadExpireAge = age;
    mStorage->writeConfig();
  }
}

void KMFolder::setUnreadExpireUnits( ExpireUnits units )
{
  if (units >= expireNever && units < expireMaxUnits)
    mUnreadExpireUnits = units;
    mStorage->writeConfig();
}

void KMFolder::setReadExpireAge( int age )
{
  if( age >= 0 && age != mReadExpireAge ) {
    mReadExpireAge = age;
    mStorage->writeConfig();
  }
}

void KMFolder::setReadExpireUnits( ExpireUnits units )
{
  if (units >= expireNever && units <= expireMaxUnits)
    mReadExpireUnits = units;
    mStorage->writeConfig();
}


void KMFolder::setExpireAction( ExpireAction a )
{
  if ( a != mExpireAction ) {
    mExpireAction = a;
    mStorage->writeConfig();
  }
}

void KMFolder::setExpireToFolderId( const QString& id )
{
  if ( id != mExpireToFolderId ) {
    mExpireToFolderId = id;
    mStorage->writeConfig();
  }
}


static int daysToExpire( int number, ExpireUnits units )
{
  switch (units) {
  case expireDays: // Days
    return number;
  case expireWeeks: // Weeks
    return number * 7;
  case expireMonths: // Months - this could be better rather than assuming 31day months.
    return number * 31;
  default: // this avoids a compiler warning (not handled enumeration values)
    ;
  }
  return -1;
}

void KMFolder::daysToExpire(int& unreadDays, int& readDays) {
  unreadDays = ::daysToExpire( getUnreadExpireAge(), getUnreadExpireUnits() );
  readDays = ::daysToExpire( getReadExpireAge(), getReadExpireUnits() );
}

void KMFolder::expireOldMessages( bool immediate )
{
  KMail::ScheduledExpireTask* task = new KMail::ScheduledExpireTask(this, immediate);
  kmkernel->jobScheduler()->registerTask( task );
  if ( immediate ) {
    // #82259: compact after expiring.
    compact( CompactLater );
  }
}

void KMFolder::compact( CompactOptions options )
{
  if ( options == CompactLater ) {
    KMail::ScheduledCompactionTask* task = new KMail::ScheduledCompactionTask(this, false);
    kmkernel->jobScheduler()->registerTask( task );
  } else {
    mStorage->compact( options == CompactSilentlyNow );
  }
}

KMFolder* KMFolder::trashFolder() const
{
  return mStorage ? mStorage->trashFolder() : 0;
}

int KMFolder::writeIndex( bool createEmptyIndex )
{
  return mStorage->writeIndex( createEmptyIndex );
}

void KMFolder::setStatus( int idx, const MessageStatus& status, bool toggle )
{
  mStorage->setStatus( idx, status, toggle );
}

void KMFolder::setStatus( QList<int>& ids, const MessageStatus& status,
                          bool toggle )
{
  mStorage->setStatus( ids, status, toggle);
}

void KMFolder::setIconPaths( const QString &normalPath,
                             const QString &unreadPath )
{
  mNormalIconPath = normalPath;
  mUnreadIconPath = unreadPath;
  mStorage->writeConfig();
  emit iconsChanged();
}

void KMFolder::removeJobs()
{
  mStorage->removeJobs();
}

int KMFolder::updateIndex()
{
  return mStorage->updateIndex();
}

void KMFolder::reallyAddMsg( KMMessage* aMsg )
{
  mStorage->reallyAddMsg( aMsg );
}

void KMFolder::reallyAddCopyOfMsg( KMMessage* aMsg )
{
  mStorage->reallyAddCopyOfMsg( aMsg );
}

void KMFolder::setShortcut( const KShortcut &sc )
{
  if ( mShortcut != sc ) {
    mShortcut = sc;
    emit shortcutChanged( this );
  }
}

bool KMFolder::isMoveable() const
{
  return !isSystemFolder() && mStorage->isMoveable();
}

void KMFolder::slotContentsTypeChanged( KMail::FolderContentsType type )
{
  kmkernel->iCalIface().folderContentsTypeChanged( this, type );
  emit iconsChanged();
}

void KMFolder::slotFolderSizeChanged()
{
  emit folderSizeChanged( this );
  KMFolder* papa = parent()->manager()->parentFolder( this );
  if ( papa && papa != this ) {
    papa->slotFolderSizeChanged();
  }
}

void KMFolder::slotIdentitiesChanged()
{
  uint defaultIdentity = kmkernel->identityManager()->defaultIdentity().uoid();

  // The default identity may have changed, therefore set it again
  // if necessary
  if ( mUseDefaultIdentity )
    mIdentity = defaultIdentity;

  // Fall back to the default identity if the one used currently is invalid
  if ( kmkernel->identityManager()->identityForUoid( mIdentity ).isNull() ) {
    mIdentity = defaultIdentity;
    mUseDefaultIdentity = true;
  }
}


#include "kmfolder.moc"
