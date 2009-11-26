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
//TODO port to akonadi #include "kmfoldermbox.h"
#include "folderstorage.h"
//TODO port to akonadi #include "kmfoldercachedimap.h"
//TODO port to akonadi #include "kmfoldersearch.h"
//TODO port to akonadi #include "kmfolderimap.h"
#include <kpimidentities/identitymanager.h>
#include <kpimidentities/identity.h>
#include "expirejob.h"
#include "compactionjob.h"
// TODO port to akonadi #include "kmailicalifaceimpl.h"
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

KMFolder::KMFolder( /*KMFolderDir* aParent,*/ const QString& aFolderName,
                    KMFolderType aFolderType, bool withIndex, bool exportedSernums )
  : /*KMFolderNode( aParent, aFolderName ),*/ mStorage(0),
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
#if 0 //TODO port to akonadi
  if( aFolderType == KMFolderTypeCachedImap )
    mStorage = new KMFolderCachedImap( this, aFolderName.toLatin1() );
  else if( aFolderType == KMFolderTypeImap )
    mStorage = new KMFolderImap( this, aFolderName.toLatin1() );
  else if( aFolderType == KMFolderTypeMaildir )
    mStorage = new KMFolderMaildir( this, aFolderName.toLatin1() );
  else
  if( aFolderType == KMFolderTypeSearch )
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
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

KMFolder::~KMFolder()
{
#if 0  //TODO port to akonadi
  mStorage->close( "~KMFolder", true );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  delete mAcctList;
#if 0  //TODO port to akonadi
  if ( mHasIndex ) mStorage->deregisterFromMessageDict();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  delete mStorage;
}

bool KMFolder::hasDescendant( KMFolder *fld ) const
{
	return false;
}

KMFolder * KMFolder::ownerFolder() const
{
	return 0;
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
#if 0
  if ( savedId != 0 && mId == 0 )
    mId = savedId;
#endif
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

#if 0  //TODO port to akonadi
  configGroup.writeEntry( "UseDefaultIdentity", mUseDefaultIdentity );
  if ( !mUseDefaultIdentity && ( !mStorage || !mStorage->account() ||
                           mIdentity != mStorage->account()->identityId() ) )
      configGroup.writeEntry("Identity", mIdentity);
  else
      configGroup.deleteEntry("Identity");

#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif

  configGroup.writeEntry("WhoField", mUserWhoField);
//  configGroup.writeEntry("Id", mId);
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
#if 0  //TODO port to akonadi
  return mStorage ? mStorage->folderType() : KMFolderTypeUnknown;
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return KMFolderTypeUnknown;
#endif
}

QString KMFolder::fileName() const
{
#if 0  //TODO port to akonadi
  return mStorage ? mStorage->fileName() : QString();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return "";
#endif
}

QString KMFolder::location() const
{
#if 0  //TODO port to akonadi
  return mStorage ? mStorage->location() : QString();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return QString();
#endif
}

QString KMFolder::indexLocation() const
{
#if 0  //TODO port to akonadi
  return mStorage ? mStorage->indexLocation() : QString();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return QString();
#endif
}

QString KMFolder::sortedLocation() const
{
#if 0  //TODO port to akonadi
  return mStorage ? mStorage->sortedLocation() : QString();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return QString();
#endif
}

QString KMFolder::idsLocation() const
{
#if 0  //TODO port to akonadi
  return mStorage ? mStorage->idsLocation() : QString();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return QString();
#endif
}

QString KMFolder::subdirLocation() const
{
#if 0
  QString sLocation( path() );

  if( !sLocation.isEmpty() )
    sLocation += '/';
  sLocation += '.' + FolderStorage::dotEscape( fileName() ) + ".directory";

  return sLocation;
#endif
return QString();
}


bool KMFolder::noContent() const
{
#if 0  //TODO port to akonadi
  return mStorage ? mStorage->noContent() : true;
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return true;
#endif
}

void KMFolder::setNoContent( bool aNoContent )
{
#if 0  //TODO port to akonadi
  mStorage->setNoContent( aNoContent );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

bool KMFolder::noChildren() const
{
#if 0  //TODO port to akonadi
  return mStorage->noChildren();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return true;
#endif
}

void KMFolder::setNoChildren( bool aNoChildren )
{
#if 0  //TODO port to akonadi
  mStorage->setNoChildren( aNoChildren );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

KMime::Message* KMFolder::getMsg( int idx )
{
#if 0  //TODO port to akonadi
  return mStorage->getMsg( idx );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return 0;
#endif
}

bool KMFolder::isMessage( int idx )
{
#if 0  //TODO port to akonadi
  return mStorage->isMessage( idx );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return true;
#endif
}


void KMFolder::ignoreJobsForMessage( KMime::Message* m )
{
#if 0  //TODO port to akonadi
  mStorage->ignoreJobsForMessage( m );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

FolderJob* KMFolder::createJob( KMime::Message *msg, FolderJob::JobType jt,
                                KMFolder *folder, const QString &partSpecifier,
                                const MessageViewer::AttachmentStrategy *as ) const
{
#if 0  //TODO port to akonadi
  return mStorage->createJob( msg, jt, folder, partSpecifier, as );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return 0;
#endif
}

FolderJob* KMFolder::createJob( QList<KMime::Message*>& msgList,
                                const QString& sets,
                                FolderJob::JobType jt, KMFolder *folder ) const
{
#if 0  //TODO port to akonadi
  return mStorage->createJob( msgList, sets, jt, folder );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return 0;
#endif
}

const KMime::Message* KMFolder::getMsgBase( int idx ) const
{
#if 0  //TODO port to akonadi
  return mStorage->getMsgBase( idx );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return 0;
#endif
}

KMime::Message* KMFolder::getMsgBase( int idx )
{
#if 0  //TODO port to akonadi
  return mStorage->getMsgBase( idx );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return 0;
#endif
}

const KMime::Message* KMFolder::operator[]( int idx ) const
{
#if 0  //TODO port to akonadi
  return mStorage->operator[]( idx );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return 0;
#endif
}

KMime::Message* KMFolder::operator[]( int idx )
{
#if 0  //TODO port to akonadi
  return mStorage->operator[]( idx );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return 0;
#endif
}

KMime::Message* KMFolder::take( int idx )
{
#if 0  //TODO port to akonadi
  return mStorage->take( idx );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return 0;
#endif
}

void KMFolder::takeMessages( const QList<KMime::Message*>& msgList )
{
#if 0  //TODO port to akonadi
  mStorage->takeMessages( msgList );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

int KMFolder::addMsg( KMime::Message* msg, int* index_return )
{
#if 0  //TODO port to akonadi
  return mStorage->addMsg( msg, index_return );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return 0;
#endif
}

int KMFolder::addMsgKeepUID( KMime::Message* msg, int* index_return )
{
#if 0  //TODO port to akonadi
  return mStorage->addMsgKeepUID( msg, index_return );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return 0;
#endif
}

int KMFolder::addMessages( QList<KMime::Message*>& list, QList<int>& index_return )
{
#if 0  //TODO port to akonadi
  return mStorage->addMessages( list, index_return );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return 0;
#endif
}

void KMFolder::emitMsgAddedSignals( int idx )
{
#if 0  //TODO port to akonadi
  mStorage->emitMsgAddedSignals( idx );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

void KMFolder::removeMsg( int i, bool imapQuiet )
{
#if 0  //TODO port to akonadi
  mStorage->removeMsg( i, imapQuiet );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

void KMFolder::removeMessages( QList<KMime::Message*> msgList, bool imapQuiet ) // TODO const ref
{
#if 0  //TODO port to akonadi
  mStorage->removeMessages( msgList, imapQuiet );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

int KMFolder::expungeOldMsg( int days )
{
#if 0  //TODO port to akonadi
  return mStorage->expungeOldMsg( days );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return 0;
#endif
}

int KMFolder::moveMsg( KMime::Message* msg, int* index_return )
{
#if 0  //TODO port to akonadi
  return mStorage->moveMsg( msg, index_return );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return 0;
#endif
}

int KMFolder::moveMsg(QList<KMime::Message*> q, int* index_return )
{
#if 0  //TODO port to akonadi
  return mStorage->moveMsg( q, index_return );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return 0;
#endif
}

int KMFolder::find( const KMime::Content* msg ) const
{
#if 0  //TODO port to akonadi
  return mStorage->find( msg );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return 0;
#endif
}

int KMFolder::find( const KMime::Message* msg ) const
{
#if 0  //TODO port to akonadi
  return mStorage->find( msg );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return 0;
#endif
}

int KMFolder::count( bool cache ) const
{
#if 0  //TODO port to akonadi
  return mStorage->count( cache );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return 0;
#endif
}

int KMFolder::countUnread()
{
#if 0  //TODO port to akonadi
  return mStorage->countUnread();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return 0;
#endif
}


void KMFolder::msgStatusChanged( const MessageStatus& oldStatus,
                                 const MessageStatus& newStatus, int idx )
{
#if 0  //TODO port to akonadi
  mStorage->msgStatusChanged( oldStatus, newStatus, idx );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

void KMFolder::msgTagListChanged( int idx )
{
  emit msgHeaderChanged( this, idx );
}


int KMFolder::open( const char *owner )
{
#if 0  //TODO port to akonadi
  //Port to akonadi
  return mStorage->open( owner );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return 0;
#endif
}

bool KMFolder::canAccess() const
{
#if 0  //TODO port to akonadi
  return mStorage->canAccess();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return true;
#endif
}

void KMFolder::close( const char *owner, bool force )
{
#if 0  //TODO port to akonadi
  // do not emit closed() in here - as this would regain too early
  mStorage->close( owner, force );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

void KMFolder::sync()
{
#if 0  //TODO port to akonadi
  mStorage->sync();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

bool KMFolder::isOpened() const
{
#if 0  //TODO port to akonadi
  return mStorage->isOpened();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return true;
#endif
}

void KMFolder::markNewAsUnread()
{
#if 0  //TODO port to akonadi
  mStorage->markNewAsUnread();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

void KMFolder::markUnreadAsRead()
{
#if 0  //TODO port to akonadi
  mStorage->markUnreadAsRead();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

void KMFolder::remove()
{
#if 0  //TODO port to akonadi
  /* The storage needs to be open before remove is called, otherwise
     it will not unregister the corresponding serial numbers from
     the message dict, since its message list is empty, and the .ids
     file contents are not loaded. That can lead to lookups in the
     dict returning stale pointers to the folder later. */
  mStorage->open("folderremoval");
  mStorage->remove();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

int KMFolder::expunge()
{
#if 0  //TODO port to akonadi
  return mStorage->expunge();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return 0;
#endif
}


bool KMFolder::dirty() const
{
#if 0  //TODO port to akonadi
  return mStorage->dirty();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return false;
#endif
}

void KMFolder::setDirty( bool f )
{
#if 0  //TODO port to akonadi
  mStorage->setDirty( f );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

bool KMFolder::needsCompacting() const
{
#if 0  //TODO port to akonadi
  return mStorage->needsCompacting();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return false;
#endif
}

void KMFolder::setNeedsCompacting( bool f )
{
#if 0  //TODO port to akonadi
  mStorage->setNeedsCompacting( f );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

void KMFolder::quiet( bool beQuiet )
{
#if 0  //TODO port to akonadi
  mStorage->quiet( beQuiet );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

bool KMFolder::isReadOnly() const
{
#if 0  //TODO port to akonadi
  return mStorage->isReadOnly();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return false;
#endif
}

bool KMFolder::isWritable() const
{
#if 0  //TODO port to akonadi
  return !mStorage->isReadOnly() && mStorage->canDeleteMessages();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return false;
#endif
}

bool KMFolder::canDeleteMessages() const
{
#if 0  //TODO port to akonadi
  return mStorage->canDeleteMessages();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return false;
#endif
}

QString KMFolder::label() const
{
  if ( !mSystemLabel.isEmpty() )
     return mSystemLabel;
  if ( !mLabel.isEmpty() )
     return mLabel;
#if 0
  if ( isSystemFolder() )
     return i18n( name().toUtf8() );
  return name();
#endif
return QString();
}

//-----------------------------------------------------------------------------
QString KMFolder::prettyUrl() const
{
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
#if 0  //TODO port to akonadi
  mStorage->writeConfig();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

void KMFolder::setMailingList( const MailingList& mlist )
{
  mMailingList = mlist;
#if 0  //TODO port to akonadi
  mStorage->writeConfig();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
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
#if 0
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
#if 0  //TODO port to akonadi
  if (writeConfig)
    mStorage->writeConfig();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  emit viewConfigChanged();
#endif
}

void KMFolder::correctUnreadMsgsCount()
{
#if 0  //TODO port to akonadi
  mStorage->correctUnreadMsgsCount();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

QString KMFolder::idString() const
{
	return "";
}

void KMFolder::setAutoExpire( bool enabled )
{
  if( enabled != mExpireMessages ) {
    mExpireMessages = enabled;
#if 0  //TODO port to akonadi
    mStorage->writeConfig();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  }
}

void KMFolder::setUnreadExpireAge( int age )
{
  if( age >= 0 && age != mUnreadExpireAge ) {
    mUnreadExpireAge = age;
#if 0  //TODO port to akonadi
    mStorage->writeConfig();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  }
}

void KMFolder::setUnreadExpireUnits( ExpireUnits units )
{
  if (units >= expireNever && units < expireMaxUnits)
    mUnreadExpireUnits = units;
#if 0  //TODO port to akonadi
    mStorage->writeConfig();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

void KMFolder::setReadExpireAge( int age )
{
  if( age >= 0 && age != mReadExpireAge ) {
    mReadExpireAge = age;
#if 0  //TODO port to akonadi
    mStorage->writeConfig();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  }
}

void KMFolder::setReadExpireUnits( ExpireUnits units )
{
  if (units >= expireNever && units <= expireMaxUnits)
    mReadExpireUnits = units;
#if 0  //TODO port to akonadi
    mStorage->writeConfig();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}


void KMFolder::setExpireAction( ExpireAction a )
{
  if ( a != mExpireAction ) {
    mExpireAction = a;
#if 0  //TODO port to akonadi
    mStorage->writeConfig();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  }
}

void KMFolder::setExpireToFolderId( const QString& id )
{
  if ( id != mExpireToFolderId ) {
    mExpireToFolderId = id;
#if 0  //TODO port to akonadi
    mStorage->writeConfig();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
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
#if 0  //TODO port to akonadi
    mStorage->compact( options == CompactSilentlyNow );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  }
}

int KMFolder::writeIndex( bool createEmptyIndex )
{
#if 0  //TODO port to akonadi
  return mStorage->writeIndex( createEmptyIndex );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return 0;
#endif
}

void KMFolder::setStatus( int idx, const MessageStatus& status, bool toggle )
{
#if 0  //TODO port to akonadi
  mStorage->setStatus( idx, status, toggle );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

void KMFolder::setStatus( QList<int>& ids, const MessageStatus& status,
                          bool toggle )
{
#if 0  //TODO port to akonadi
  mStorage->setStatus( ids, status, toggle);
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

void KMFolder::setIconPaths( const QString &normalPath,
                             const QString &unreadPath )
{
  mNormalIconPath = normalPath;
  mUnreadIconPath = unreadPath;
#if 0  //TODO port to akonadi
  mStorage->writeConfig();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  emit iconsChanged();
}

void KMFolder::removeJobs()
{
#if 0  //TODO port to akonadi
  mStorage->removeJobs();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

int KMFolder::updateIndex()
{
#if 0  //TODO port to akonadi
  return mStorage->updateIndex();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return 0;
#endif
}

void KMFolder::reallyAddMsg( KMime::Message* aMsg )
{
#if 0  //TODO port to akonadi
  mStorage->reallyAddMsg( aMsg );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

void KMFolder::reallyAddCopyOfMsg( KMime::Message* aMsg )
{
#if 0  //TODO port to akonadi
  mStorage->reallyAddCopyOfMsg( aMsg );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
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
#if 0  //TODO port to akonadi
  return !isSystemFolder() && mStorage->isMoveable();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return 0;
#endif
}

void KMFolder::slotContentsTypeChanged( KMail::FolderContentsType type )
{
#if 0 //TODO port to akonadi
  kmkernel->iCalIface().folderContentsTypeChanged( this, type );
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  emit iconsChanged();
}

void KMFolder::slotFolderSizeChanged()
{
  emit folderSizeChanged( this );
#if 0
  KMFolder* papa = parent()->manager()->parentFolder( this );
  if ( papa && papa != this ) {
    papa->slotFolderSizeChanged();
  }
#endif
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
