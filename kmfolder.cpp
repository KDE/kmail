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
#include "kmfoldertree.h"
#include "kmailicalifaceimpl.h"

#include <cerrno>

#ifdef Q_WS_WIN32
#include <io.h>
#endif

#include <kdebug.h>
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
    mIdentity( 0 ), // default identity
    mPutRepliesInSameFolder( false ),
    mIgnoreNewMail( false )
{
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
    QString msg = i18n("<qt>Error while creating file <b>%1</b>:<br>%2</qt>", aFolderName, strerror(rc));
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
  connect( mStorage, SIGNAL( msgChanged( KMFolder*, quint32 , int ) ),
           SIGNAL( msgChanged( KMFolder*, quint32 , int ) ) );
  connect( mStorage, SIGNAL( msgHeaderChanged( KMFolder*, int ) ),
           SIGNAL( msgHeaderChanged( KMFolder*, int ) ) );
  connect( mStorage, SIGNAL( statusMsg( const QString& ) ),
           SIGNAL( statusMsg( const QString& ) ) );
  connect( mStorage, SIGNAL( numUnreadMsgsChanged( KMFolder* ) ),
           SIGNAL( numUnreadMsgsChanged( KMFolder* ) ) );
  connect( mStorage, SIGNAL( removed( KMFolder*, bool ) ),
           SIGNAL( removed( KMFolder*, bool ) ) );

  connect( mStorage, SIGNAL( contentsTypeChanged( KMail::FolderContentsType ) ),
                this, SLOT( slotContentsTypeChanged( KMail::FolderContentsType ) ) );

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
  delete mAcctList;
  if ( mHasIndex ) mStorage->deregisterFromMessageDict();
  delete mStorage;
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

  mIdentity = configGroup.readEntry("Identity", 0 );

  setUserWhoField( configGroup.readEntry( "WhoField" ), false );
  uint savedId = configGroup.readEntry( "Id" , 0 );
  // make sure that we don't overwrite a valid id
  if ( savedId != 0 && mId == 0 )
    mId = savedId;
  mPutRepliesInSameFolder = configGroup.readEntry( "PutRepliesInSameFolder", false );
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

  configGroup.writeEntry("Identity", mIdentity);

  configGroup.writeEntry("WhoField", mUserWhoField);
  configGroup.writeEntry("Id", mId);
  configGroup.writeEntry( "PutRepliesInSameFolder", mPutRepliesInSameFolder );
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
  if( mChild )
    return mChild;

  QString childName = '.' + fileName() + ".directory";
  QString childDir = path() + '/' + childName;
  if (access(QFile::encodeName(childDir), W_OK) != 0) // Not there or not writable
  {
#ifdef Q_WS_WIN32
    const int mkdirResult = mkdir(QFile::encodeName(childDir));
#else
    const int mkdirResult = mkdir(QFile::encodeName(childDir), S_IRWXU);
#endif
      if ( mkdirResult != 0 
      && chmod(QFile::encodeName(childDir), S_IRWXU) != 0) {
      QString wmsg = QString(" '%1': %2").arg(childDir).arg(strerror(errno));
      KMessageBox::information(0,i18n("Failed to create folder") + wmsg);
      return 0;
    }
  }

  KMFolderDirType newType = KMStandardDir;
  if( folderType() == KMFolderTypeCachedImap )
    newType = KMDImapDir;
  else if( folderType() == KMFolderTypeImap )
    newType = KMImapDir;

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
  return mStorage->noContent();
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

void KMFolder::take( const QList<KMMessage*>& msgList )
{
  mStorage->take( msgList );
}

int KMFolder::addMsg( KMMessage* msg, int* index_return )
{
  return mStorage->addMsg( msg, index_return );
}

int KMFolder::addMsgKeepUID( KMMessage* msg, int* index_return )
{
  return mStorage->addMsgKeepUID( msg, index_return );
}

int KMFolder::addMsg( QList<KMMessage*>& list, QList<int>& index_return )
{
  return mStorage->addMsg( list, index_return );
}

void KMFolder::emitMsgAddedSignals( int idx )
{
  mStorage->emitMsgAddedSignals( idx );
}

void KMFolder::removeMsg( int i, bool imapQuiet )
{
  mStorage->removeMsg( i, imapQuiet );
}

void KMFolder::removeMsg( QList<KMMessage*> msgList, bool imapQuiet ) // TODO const ref
{
  mStorage->removeMsg( msgList, imapQuiet );
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

int KMFolder::open( const char *owner )
{
  return mStorage->open( owner );
}

int KMFolder::canAccess()
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
    for( it = post.begin(); it != post.end(); ++it ) {
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

void KMFolder::setIdentity( uint identity )
{
  mIdentity = identity;
  kmkernel->slotRequestConfigSync();
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
  if ( mUserWhoField == whoField )
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
    kDebug(5006) << "Illegal setting " << whoField << " for userWhoField!"
                  << endl;
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
  escapedName.replace( "[", "%(" );
  escapedName.replace( "]", "%)" );
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
  return !isSystemFolder();
}

void KMFolder::slotContentsTypeChanged( KMail::FolderContentsType type )
{
  kmkernel->iCalIface().folderContentsTypeChanged( this, type );
  emit iconsChanged();
}

#include "kmfolder.moc"
