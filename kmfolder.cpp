// -*- mode: C++; c-file-style: "gnu" -*-
// kmfolder.cpp
// Author: Stefan Taferner <taferner@alpin.or.at>

#include <config.h>

#include "kmfolder.h"
#include "kmfolderdir.h"
#include "folderstorage.h"
#include "kmfoldercachedimap.h"
#include "kmfoldersearch.h"
#include "kmfolderimap.h"
#include "kmfoldermgr.h"


KMFolder::KMFolder( KMFolderDir* aParent, const QString& aFolderName,
                    KMFolderType aFolderType )
  : KMFolderNode( aParent, aFolderName ), mParent( aParent )
{
  if( aFolderType == KMFolderTypeCachedImap )
    mStorage = new KMFolderCachedImap( this, aFolderName.latin1() );
  else if( aFolderType == KMFolderTypeImap )
    mStorage = new KMFolderImap( this, aFolderName.latin1() );
  else if( aFolderType == KMFolderTypeMaildir )
    mStorage = new KMFolderMaildir( this, aFolderName.latin1() );
  else if( aFolderType == KMFolderTypeSearch )
    mStorage = new KMFolderSearch( this, aFolderName.latin1() );
  else
    mStorage = new KMFolderMbox( this, aFolderName.latin1() );

  if ( aParent ) {
    connect( mStorage, SIGNAL( msgAdded( KMFolder*, Q_UINT32 ) ),
             aParent->manager(), SIGNAL( msgAdded( KMFolder*, Q_UINT32 ) ) );
    connect( mStorage, SIGNAL( msgRemoved( KMFolder*, Q_UINT32 ) ),
             parent()->manager(), SIGNAL( msgRemoved( KMFolder*, Q_UINT32 ) ) );
    connect( this, SIGNAL( msgChanged( KMFolder*, Q_UINT32, int ) ),
             parent()->manager(), SIGNAL( msgChanged( KMFolder*, Q_UINT32, int ) ) );
    connect( this, SIGNAL( msgHeaderChanged( KMFolder*,  int ) ),
             parent()->manager(), SIGNAL( msgHeaderChanged( KMFolder*, int ) ) );
  }

  // Resend all mStorage signals
  connect( mStorage, SIGNAL( changed() ), SIGNAL( changed() ) );
  connect( mStorage, SIGNAL( cleared() ), SIGNAL( cleared() ) );
  connect( mStorage, SIGNAL( expunged() ), SIGNAL( expunged() ) );
  connect( mStorage, SIGNAL( iconsChanged() ), SIGNAL( iconsChanged() ) );
  connect( mStorage, SIGNAL( nameChanged() ), SIGNAL( nameChanged() ) );
  connect( mStorage, SIGNAL( msgRemoved( KMFolder*, Q_UINT32 ) ),
           SIGNAL( msgRemoved( KMFolder*, Q_UINT32 ) ) );
  connect( mStorage, SIGNAL( msgRemoved( int, QString, QString ) ),
           SIGNAL( msgRemoved( int, QString, QString ) ) );
  connect( mStorage, SIGNAL( msgRemoved( KMFolder* ) ),
           SIGNAL( msgRemoved( KMFolder* ) ) );
  connect( mStorage, SIGNAL( msgAdded( int ) ), SIGNAL( msgAdded( int ) ) );
  connect( mStorage, SIGNAL( msgAdded( KMFolder*, Q_UINT32 ) ),
           SIGNAL( msgAdded( KMFolder*, Q_UINT32 ) ) );
  connect( mStorage, SIGNAL( msgChanged( KMFolder*, Q_UINT32 , int ) ),
           SIGNAL( msgChanged( KMFolder*, Q_UINT32 , int ) ) );
  connect( mStorage, SIGNAL( msgHeaderChanged( KMFolder*, int ) ),
           SIGNAL( msgHeaderChanged( KMFolder*, int ) ) );
  connect( mStorage, SIGNAL( statusMsg( const QString& ) ),
           SIGNAL( statusMsg( const QString& ) ) );
  connect( mStorage, SIGNAL( numUnreadMsgsChanged( KMFolder* ) ),
           SIGNAL( numUnreadMsgsChanged( KMFolder* ) ) );
  connect( mStorage, SIGNAL( syncRunning( KMFolder*, bool ) ),
           SIGNAL( syncRunning( KMFolder*, bool ) ) );
}

KMFolder::~KMFolder()
{
  delete mStorage;
}

KMFolderType KMFolder::folderType() const
{
  return mStorage->folderType();
}

QString KMFolder::fileName() const
{
  return mStorage->fileName();
}

QString KMFolder::location() const
{
  return mStorage->location();
}

QString KMFolder::indexLocation() const
{
  return mStorage->indexLocation();
}

QString KMFolder::subdirLocation() const
{
  return mStorage->subdirLocation();
}

KMFolderDir* KMFolder::child() const
{
  return mStorage->child();
}

KMFolderDir* KMFolder::createChildFolder()
{
  return mStorage->createChildFolder();
}

void KMFolder::setChild( KMFolderDir* aChild )
{
  mStorage->setChild( aChild );
}

bool KMFolder::noContent() const
{
  return mStorage->noContent();
}

void KMFolder::setNoContent( bool aNoContent )
{
  mStorage->setNoContent( aNoContent );
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

QCString& KMFolder::getMsgString( int idx, QCString& mDest )
{
  return mStorage->getMsgString( idx,  mDest );
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
                                KMFolder *folder, QString partSpecifier, 
                                const AttachmentStrategy *as ) const
{
  return mStorage->createJob( msg, jt, folder, partSpecifier, as );
}

FolderJob* KMFolder::createJob( QPtrList<KMMessage>& msgList,
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

void KMFolder::take( QPtrList<KMMessage> msgList )
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

void KMFolder::emitMsgAddedSignals( int idx )
{
  mStorage->emitMsgAddedSignals( idx );
}

bool KMFolder::canAddMsgNow( KMMessage* aMsg, int* aIndex_ret )
{
  return mStorage->canAddMsgNow( aMsg, aIndex_ret );
}

void KMFolder::removeMsg( int i, bool imapQuiet )
{
  mStorage->removeMsg( i, imapQuiet );
}

void KMFolder::removeMsg( QPtrList<KMMessage> msgList, bool imapQuiet )
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

int KMFolder::moveMsg(QPtrList<KMMessage> q, int* index_return )
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
  return mStorage->countUnreadRecursive();
}

void KMFolder::msgStatusChanged( const KMMsgStatus oldStatus,
                                 const KMMsgStatus newStatus, int idx )
{
  mStorage->msgStatusChanged( oldStatus, newStatus, idx );
}

int KMFolder::open()
{
  return mStorage->open();
}

int KMFolder::canAccess()
{
  return mStorage->canAccess();
}

void KMFolder::close( bool force )
{
  mStorage->close( force );
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

int KMFolder::create( bool imap )
{
  return mStorage->create( imap );
}

int KMFolder::remove()
{
  return mStorage->remove();
}

int KMFolder::expunge()
{
  return mStorage->expunge();
}

int KMFolder::compact()
{
  return mStorage->compact();
}

int KMFolder::rename( const QString& newName, KMFolderDir *aParent )
{
  return mStorage->rename( newName, aParent );
}

bool KMFolder::autoCreateIndex() const
{
  return mStorage->autoCreateIndex();
}

void KMFolder::setAutoCreateIndex( bool b )
{
  mStorage->setAutoCreateIndex( b );
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

bool KMFolder::isSystemFolder() const
{
  return mStorage->isSystemFolder();
}

void KMFolder::setSystemFolder( bool itIs )
{
  mStorage->setSystemFolder( itIs );
}

QString KMFolder::label() const
{
  return mStorage->label();
}

void KMFolder::setLabel( const QString& lbl )
{
  mStorage->setLabel( lbl );
}

const char* KMFolder::type() const
{
  return mStorage->type();
}

QCString KMFolder::protocol() const
{
  return mStorage->protocol();
}

bool KMFolder::hasAccounts() const
{
  return mStorage->hasAccounts();
}

void KMFolder::setMailingList( bool enabled )
{
  mStorage->setMailingList( enabled );
}

bool KMFolder::isMailingList() const
{
  return mStorage->isMailingList();
}

void KMFolder::setMailingListPostAddress( const QString& address )
{
  mStorage->setMailingListPostAddress( address );
}

QString KMFolder::mailingListPostAddress() const
{
  return mStorage->mailingListPostAddress();
}

void KMFolder::setMailingListAdminAddress( const QString& address )
{
  mStorage->setMailingListAdminAddress( address );
}

QString KMFolder::mailingListAdminAddress() const
{
  return mStorage->mailingListAdminAddress();
}

void KMFolder::setIdentity( uint identity )
{
  mStorage->setIdentity( identity );
}

uint KMFolder::identity() const
{
  return mStorage->identity();
}

QString KMFolder::whoField() const
{
  return mStorage->whoField();
}

void KMFolder::setWhoField(const QString& aWhoField )
{
  mStorage->setWhoField( aWhoField );
}

QString KMFolder::userWhoField()
{
  return mStorage->userWhoField();
}

void KMFolder::setUserWhoField( const QString& whoField, bool writeConfig )
{
  mStorage->setUserWhoField( whoField, writeConfig );
}

void KMFolder::correctUnreadMsgsCount()
{
  mStorage->correctUnreadMsgsCount();
}

QString KMFolder::idString() const
{
  return mStorage->idString();
}

void KMFolder::setAutoExpire( bool enabled )
{
  mStorage->setAutoExpire( enabled );
}

bool KMFolder::isAutoExpire() const
{
  return mStorage->isAutoExpire();
}

void KMFolder::setUnreadExpireAge( int age )
{
  mStorage->setUnreadExpireAge( age );
}

void KMFolder::setUnreadExpireUnits( ExpireUnits units )
{
  mStorage->setUnreadExpireUnits( units );
}

void KMFolder::setReadExpireAge( int age )
{
  mStorage->setReadExpireAge( age );
}

void KMFolder::setReadExpireUnits( ExpireUnits units )
{
  mStorage->setReadExpireUnits( units );
}

int KMFolder::getUnreadExpireAge() const
{
  return mStorage->getUnreadExpireAge();
}

int KMFolder::getReadExpireAge() const
{
  return mStorage->getReadExpireAge();
}

ExpireUnits KMFolder::getUnreadExpireUnits() const
{
  return mStorage->getUnreadExpireUnits();
}

ExpireUnits KMFolder::getReadExpireUnits() const
{
  return mStorage->getReadExpireUnits();
}

void KMFolder::expireOldMessages()
{
  mStorage->expireOldMessages();
}

int KMFolder::writeIndex( bool createEmptyIndex )
{
  return mStorage->writeIndex( createEmptyIndex );
}

void KMFolder::fillMsgDict( KMMsgDict* dict )
{
  mStorage->fillMsgDict( dict );
}

int KMFolder::writeMsgDict( KMMsgDict* dict)
{
  return mStorage->writeMsgDict( dict );
}

int KMFolder::touchMsgDict()
{
  return mStorage->touchMsgDict();
}

int KMFolder::appendtoMsgDict( int idx )
{
  return mStorage->appendtoMsgDict( idx );
}

void KMFolder::setRDict( KMMsgDictREntry* rentry )
{
  mStorage->setRDict( rentry );
}

KMMsgDictREntry* KMFolder::rDict() const
{
  return mStorage->rDict();
}

void KMFolder::setStatus( int idx, KMMsgStatus status, bool toggle )
{
  mStorage->setStatus( idx, status, toggle );
}

void KMFolder::setStatus( QValueList<int>& ids, KMMsgStatus status,
                          bool toggle )
{
  mStorage->setStatus( ids, status, toggle);
}

bool KMFolder::useCustomIcons() const
{
  return mStorage->useCustomIcons();
}

void KMFolder::setUseCustomIcons( bool useCustomIcons )
{
  mStorage->setUseCustomIcons( useCustomIcons );
}

QString KMFolder::normalIconPath() const
{
  return mStorage->normalIconPath();
}

QString KMFolder::unreadIconPath() const
{
  return mStorage->unreadIconPath();
}

void KMFolder::setIconPaths( const QString &normalPath,
                             const QString &unreadPath )
{
  mStorage->setIconPaths( normalPath, unreadPath );
}

void KMFolder::removeJobs()
{
  mStorage->removeJobs();
}

size_t KMFolder::crlf2lf( char* str, const size_t strLen )
{
  return FolderStorage::crlf2lf( str, strLen );
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


#include "kmfolder.moc"
