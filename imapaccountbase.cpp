/** -*- c++ -*-
 * imapaccountbase.cpp
 *
 * Copyright (c) 2000-2002 Michael Haeckel <haeckel@kde.org>
 * Copyright (c) 2002 Marc Mutz <mutz@kde.org>
 *
 * This file is based on work on pop3 and imap account implementations
 * by Don Sanders <sanders@kde.org> and Michael Haeckel <haeckel@kde.org>
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "imapaccountbase.h"

using KMail::SieveConfig;
#include "accountmanager.h"
using KMail::AccountManager;
#include "kmfolder.h"
#include "broadcaststatus.h"
using KPIM::BroadcastStatus;
#include "kmmainwin.h"
#include "kmfolderimap.h"
#include "kmmainwidget.h"
#include "kmmainwin.h"
#include "kmmsgpart.h"
#include "acljobs.h"
#include "kmfoldercachedimap.h"
#include "bodyvisitor.h"
using KMail::BodyVisitor;
#include "imapjob.h"
using KMail::ImapJob;
#include "protocols.h"
#include "progressmanager.h"
using KPIM::ProgressManager;
#include "kmfoldermgr.h"
#include "listjob.h"

#include <kdebug.h>
#include <kconfiggroup.h>
#include <kconfig.h>
#include <klocale.h>
#include <kmessagebox.h>
using KIO::MetaData;
#include <kio/passworddialog.h>
using KIO::PasswordDialog;
#include <kio/scheduler.h>
#include <kio/slave.h>
#include <mimelib/bodypart.h>
#include <mimelib/body.h>
#include <mimelib/headers.h>
#include <mimelib/message.h>
//using KIO::Scheduler; // use FQN below

#include <QList>
#include <QRegExp>
#include <QTextDocument>
#include <QByteArray>
#include <q3stylesheet.h>

namespace KMail {

static const unsigned short int imapDefaultPort = 143;

//
//
// Ctor and Dtor
//
//

ImapAccountBase::ImapAccountBase( AccountManager *parent, const QString &name, uint id )
  : NetworkAccount( parent, name, id ),
    mTotal( 0 ),
    mCountUnread( 0 ),
    mCountLastUnread( 0 ),
    mAutoExpunge( true ),
    mHiddenFolders( false ),
    mOnlySubscribedFolders( false ),
    mOnlyLocallySubscribedFolders( false ),
    mLoadOnDemand( true ),
    mListOnlyOpenFolders( false ),
    mProgressEnabled( false ),
    mErrorDialogIsActive( false ),
    mPasswordDialogIsActive( false ),
    mACLSupport( true ),
    mAnnotationSupport( true ),
    mQuotaSupport( true ),
    mSlaveConnected( false ),
    mSlaveConnectionError( false ),
    mCheckingSingleFolder( false ),
    mListDirProgressItem( 0 )
{
  mPort = imapDefaultPort;
  KIO::Scheduler::connect( SIGNAL(slaveError(KIO::Slave *, int, const QString &)),
                           this, SLOT(slotSchedulerSlaveError(KIO::Slave *, int, const QString &)) );
  KIO::Scheduler::connect( SIGNAL(slaveConnected(KIO::Slave *)),
                           this, SLOT(slotSchedulerSlaveConnected(KIO::Slave *)) );
  connect( &mNoopTimer, SIGNAL(timeout()), SLOT(slotNoopTimeout()) );
  connect( &mIdleTimer, SIGNAL(timeout()), SLOT(slotIdleTimeout()) );
}

ImapAccountBase::~ImapAccountBase()
{
  qDeleteAll (mBodyPartList );
  kWarning( mSlave, 5006 )
    << "slave should have been destroyed by subclass!" << endl;
}

void ImapAccountBase::init()
{
  mAutoExpunge = true;
  mHiddenFolders = false;
  mOnlySubscribedFolders = false;
  mOnlyLocallySubscribedFolders = false;
  mLoadOnDemand = true;
  mListOnlyOpenFolders = false;
  mProgressEnabled = false;
}

void ImapAccountBase::pseudoAssign( const KMAccount *a )
{
  NetworkAccount::pseudoAssign( a );

  const ImapAccountBase *i = dynamic_cast<const ImapAccountBase*>( a );
  if ( !i ) {
    return;
  }

  setAutoExpunge( i->autoExpunge() );
  setHiddenFolders( i->hiddenFolders() );
  setOnlySubscribedFolders( i->onlySubscribedFolders() );
  setOnlyLocallySubscribedFolders( i->onlyLocallySubscribedFolders() );
  setLoadOnDemand( i->loadOnDemand() );
  setListOnlyOpenFolders( i->listOnlyOpenFolders() );
  setNamespaces( i->namespaces() );
  setNamespaceToDelimiter( i->namespaceToDelimiter() );
  localBlacklistFromStringList( i->locallyBlacklistedFolders() );
}

unsigned short int ImapAccountBase::defaultPort() const
{
  return imapDefaultPort;
}

QString ImapAccountBase::protocol() const
{
  return useSSL() ? IMAP_SSL_PROTOCOL : IMAP_PROTOCOL;
}

//
//
// Getters and Setters
//
//

void ImapAccountBase::setAutoExpunge( bool expunge )
{
  mAutoExpunge = expunge;
}

void ImapAccountBase::setHiddenFolders( bool show )
{
  mHiddenFolders = show;
}

void ImapAccountBase::setOnlySubscribedFolders( bool show )
{
  mOnlySubscribedFolders = show;
}

void ImapAccountBase::setOnlyLocallySubscribedFolders( bool show )
{
  mOnlyLocallySubscribedFolders = show;
}

void ImapAccountBase::setLoadOnDemand( bool load )
{
  mLoadOnDemand = load;
}

void ImapAccountBase::setListOnlyOpenFolders( bool only )
{
  mListOnlyOpenFolders = only;
}

//
//
// read/write config
//
//

void ImapAccountBase::readConfig( KConfigGroup &config )
{
  NetworkAccount::readConfig( config );

  setAutoExpunge( config.readEntry( "auto-expunge", false ) );
  setHiddenFolders( config.readEntry( "hidden-folders", false ) );
  setOnlySubscribedFolders( config.readEntry( "subscribed-folders", false ) );
  setOnlyLocallySubscribedFolders( config.readEntry( "locally-subscribed-folders", false ) );
  setLoadOnDemand( config.readEntry( "loadondemand", false ) );
  setListOnlyOpenFolders( config.readEntry( "listOnlyOpenFolders", false ) );
  // read namespaces
  nsMap map;
  QStringList list = config.readEntry( QString::number( PersonalNS) , QStringList() );
  if ( !list.isEmpty() ) {
    map[PersonalNS] = list.replaceInStrings( "\"", "" );
  }
  list = config.readEntry( QString::number( OtherUsersNS) , QStringList()  );
  if ( !list.isEmpty() ) {
    map[OtherUsersNS] = list.replaceInStrings( "\"", "" );
  }
  list = config.readEntry( QString::number( SharedNS ), QStringList() );
  if ( !list.isEmpty() ) {
    map[SharedNS] = list.replaceInStrings( "\"", "" );
  }
  setNamespaces( map );
  // read namespace - delimiter
  const namespaceDelim entries = config.entryMap();
  namespaceDelim namespaceToDelimiter;
  for ( namespaceDelim::ConstIterator it = entries.begin();
        it != entries.end(); ++it ) {
    if ( it.key().startsWith( "Namespace:" ) ) {
      QString key = it.key().right( it.key().length() - 10 );
      namespaceToDelimiter[key] = it.value();
    }
  }
  setNamespaceToDelimiter( namespaceToDelimiter );
  mOldPrefix = config.readEntry( "prefix" );
  if ( !mOldPrefix.isEmpty() ) {
    makeConnection();
  }
  localBlacklistFromStringList( config.readEntry( "locallyUnsubscribedFolders", QStringList() ) );
}

void ImapAccountBase::writeConfig( KConfigGroup &config )
{
  NetworkAccount::writeConfig( config );

  config.writeEntry( "auto-expunge", autoExpunge() );
  config.writeEntry( "hidden-folders", hiddenFolders() );
  config.writeEntry( "subscribed-folders", onlySubscribedFolders() );
  config.writeEntry( "locally-subscribed-folders", onlyLocallySubscribedFolders() );
  config.writeEntry( "loadondemand", loadOnDemand() );
  config.writeEntry( "listOnlyOpenFolders", listOnlyOpenFolders() );
  QString data;
  for ( nsMap::Iterator it = mNamespaces.begin(); it != mNamespaces.end(); ++it ) {
    if ( !it.value().isEmpty() ) {
      data = "\"" + it.value().join("\",\"") + "\"";
      config.writeEntry( QString::number( it.key() ), data );
    }
  }
  QString key;
  for ( namespaceDelim::ConstIterator it = mNamespaceToDelimiter.begin();
        it != mNamespaceToDelimiter.end(); ++it ) {
    key = "Namespace:" + it.key();
    config.writeEntry( key, it.value() );
  }
  config.writeEntry( "locallyUnsubscribedFolders", locallyBlacklistedFolders() );
}

//
//
// Network processing
//
//

MetaData ImapAccountBase::slaveConfig() const
{
  MetaData m = NetworkAccount::slaveConfig();

  m.insert( "auth", auth() );
  if ( autoExpunge() ) {
    m.insert( "expunge", "auto" );
  }

  return m;
}

ImapAccountBase::ConnectionState ImapAccountBase::makeConnection()
{
  if ( mSlave && mSlaveConnected ) {
    return Connected;
  }
  if ( mPasswordDialogIsActive ) {
    return Connecting;
  }

  if ( mAskAgain ||
       ( ( passwd().isEmpty() || login().isEmpty() ) && auth() != "GSSAPI" ) ) {

    Q_ASSERT( !mSlave ); // disconnected on 'wrong login' error already, or first try
    QString log = login();
    QString pass = passwd();
    // We init "store" to true to indicate that we want to have the
    // "keep password" checkbox. Then, we set [Passwords]Keep to
    // storePasswd(), so that the checkbox in the dialog will be
    // init'ed correctly:
    KConfigGroup passwords( KGlobal::config(), "Passwords" );
    passwords.writeEntry( "Keep", storePasswd() );
    QString msg = i18n("You need to supply a username and a password to "
                       "access this mailbox.");
    mPasswordDialogIsActive = true;

    PasswordDialog dlg( msg, log, true /* store pw */, true, KMKernel::self()->mainWin() );
    dlg.setPlainCaption( i18n("Authorization Dialog") );
    dlg.addCommentLine( i18n("Account:"), name() );
    int ret = dlg.exec();
    if (ret != QDialog::Accepted ) {
      mPasswordDialogIsActive = false;
      mAskAgain = false;
      emit connectionResult( KIO::ERR_USER_CANCELED, QString() );
      return Error;
    }
    mPasswordDialogIsActive = false;
    // The user has been given the chance to change login and
    // password, so copy both from the dialog:
    setPasswd( dlg.password(), dlg.keepPassword() );
    setLogin( dlg.username() );
    mAskAgain = false;
  }
  // already waiting for a connection?
  if ( mSlave && !mSlaveConnected ) {
    return Connecting;
  }

  mSlaveConnected = false;
  mSlave = KIO::Scheduler::getConnectedSlave( getUrl(), slaveConfig() );
  if ( !mSlave ) {
    KMessageBox::error(0, i18n("Could not start process for %1.",
                               getUrl().protocol() ) );
    return Error;
  }
  if ( mSlave->isConnected() ) {
    slotSchedulerSlaveConnected( mSlave );
    return Connected;
  }

  return Connecting;
}

bool ImapAccountBase::handleJobError( KIO::Job *job, const QString &context, bool abortSync )
{
  JobIterator it = findJob( job );
  if ( it != jobsEnd() && (*it).progressItem ) {
    (*it).progressItem->setComplete();
    (*it).progressItem = 0;
  }
  return handleError( job->error(), job->errorText(), job, context, abortSync );
}

// Called when we're really all done.
void ImapAccountBase::postProcessNewMail( bool showStatusMsg )
{
  setCheckingMail( false );
  int newMails = 0;
  if ( mCountUnread > 0 && mCountUnread > mCountLastUnread ) {
    newMails = mCountUnread  - mCountLastUnread;
    mCountLastUnread = mCountUnread;
    mCountUnread = 0;
    checkDone( true, CheckOK );
  } else {
    mCountUnread = 0;
    checkDone( false, CheckOK );
  }
  if ( showStatusMsg ) {
    BroadcastStatus::instance()->
      setStatusMsgTransmissionCompleted( name(), newMails );
  }
}

//-----------------------------------------------------------------------------
void ImapAccountBase::changeSubscription( bool subscribe, const QString &imapPath )
{
  // change the subscription of the folder
  KUrl url = getUrl();
  url.setPath( imapPath );

  QByteArray packedArgs;
  QDataStream stream( &packedArgs, QIODevice::WriteOnly );

  if ( subscribe ) {
    stream << (int) 'u' << url;
  } else {
    stream << (int) 'U' << url;
  }

  // create the KIO-job
  if ( makeConnection() != Connected ) {
    return;// ## doesn't handle Connecting
  }
  KIO::SimpleJob *job = KIO::special( url, packedArgs, false );
  KIO::Scheduler::assignJobToSlave( mSlave, job );
  jobData jd( url.url(), NULL );
  // a bit of a hack to save one slot
  if ( subscribe ) {
    jd.onlySubscribed = true;
  } else {
    jd.onlySubscribed = false;
  }
  insertJob( job, jd );

  connect(job, SIGNAL(result(KJob *)),
          SLOT(slotSubscriptionResult(KJob *)));
}

//-----------------------------------------------------------------------------
void ImapAccountBase::slotSubscriptionResult( KJob *job )
{
  // result of a subscription-job
  JobIterator it = findJob( static_cast<KIO::Job*>( job ) );
  if ( it == jobsEnd() ) {
    return;
  }
  bool onlySubscribed = (*it).onlySubscribed;
  QString path = static_cast<KIO::SimpleJob*>(job)->url().path();
  if ( job->error() ) {
    handleJobError( static_cast<KIO::Job*>(job),
                    i18n( "Error while trying to subscribe to %1:", path ) + '\n' );
    // ## emit subscriptionChanged here in case anyone needs it to support continue/cancel
  } else {
    emit subscriptionChanged( path, onlySubscribed );
    if ( mSlave ) {
      removeJob( static_cast<KIO::Job*>( job ) );
    }
  }
}

//-----------------------------------------------------------------------------
// TODO imapPath can be removed once parent can be a KMFolderImapBase or whatever
void ImapAccountBase::getUserRights( KMFolder *parent, const QString &imapPath )
{
  // There isn't much point in asking the server about a user's rights
  // on his own inbox, it might not be the effective permissions
  // (at least with Cyrus, one can admin his own inbox, even after
  // a SETACL that removes the admin permissions. Other imap servers
  // apparently don't even allow removing one's own admin permission,
  // so this code won't hurt either).
  if ( imapPath == "/INBOX/" ) {
    if ( parent->folderType() == KMFolderTypeImap ) {
      static_cast<KMFolderImap*>( parent->storage() )->
        setUserRights( ACLJobs::All );
    } else if ( parent->folderType() == KMFolderTypeCachedImap ) {
      static_cast<KMFolderCachedImap*>( parent->storage() )->
        setUserRights( ACLJobs::All );
    }
    // warning, you need to connect first to get that one
    emit receivedUserRights( parent );
    return;
  }

  KUrl url = getUrl();
  url.setPath( imapPath );

  ACLJobs::GetUserRightsJob *job = ACLJobs::getUserRights( mSlave, url );

  jobData jd( url.url(), parent );
  jd.cancellable = true;
  insertJob( job, jd );

  connect( job, SIGNAL(result(KJob *)),
           SLOT(slotGetUserRightsResult(KJob *)) );
}

void ImapAccountBase::slotGetUserRightsResult( KJob *_job )
{
  ACLJobs::GetUserRightsJob *job = static_cast<ACLJobs::GetUserRightsJob *>( _job );
  JobIterator it = findJob( job );
  if ( it == jobsEnd() ) {
    return;
  }

  KMFolder* folder = (*it).parent;
  if ( job->error() ) {
    if ( job->error() == KIO::ERR_UNSUPPORTED_ACTION ) {
      // when the imap server doesn't support ACLs
      mACLSupport = false;
    } else {
      kWarning(5006) << "slotGetUserRightsResult: " << job->errorString() << endl;
    }
  } else {
#ifndef NDEBUG
    //kDebug(5006) << "User Rights: " << ACLJobs::permissionsToString( job->permissions() ) << endl;
#endif
    // Store the permissions
    if ( folder->folderType() == KMFolderTypeImap ) {
      static_cast<KMFolderImap*>( folder->storage() )->setUserRights( job->permissions() );
    } else if ( folder->folderType() == KMFolderTypeCachedImap ) {
      static_cast<KMFolderCachedImap*>( folder->storage() )->setUserRights( job->permissions() );
    }
  }
  if ( mSlave ) {
    removeJob( job );
  }
  emit receivedUserRights( folder );
}

//-----------------------------------------------------------------------------
void ImapAccountBase::getACL( KMFolder *parent, const QString &imapPath )
{
  KUrl url = getUrl();
  url.setPath( imapPath );

  ACLJobs::GetACLJob* job = ACLJobs::getACL( mSlave, url );
  jobData jd( url.url(), parent );
  jd.cancellable = true;
  insertJob( job, jd );

  connect( job, SIGNAL(result(KJob *)),
           SLOT(slotGetACLResult(KJob *)) );
}

void ImapAccountBase::slotGetACLResult( KJob *_job )
{
  ACLJobs::GetACLJob* job = static_cast<ACLJobs::GetACLJob *>( _job );
  JobIterator it = findJob( job );
  if ( it == jobsEnd() ) {
    return;
  }

  KMFolder *folder = (*it).parent;
  emit receivedACL( folder, job, job->entries() );
  if ( mSlave ) {
    removeJob( job );
  }
}

//-----------------------------------------------------------------------------
// Do not remove imapPath, FolderDiaQuotaTab needs to call this with parent==0.
void ImapAccountBase::getStorageQuotaInfo( KMFolder *parent, const QString &imapPath )
{
  if ( !mSlave ) {
    return;
  }
  KUrl url = getUrl();
  url.setPath( imapPath );

  QuotaJobs::GetStorageQuotaJob *job = QuotaJobs::getStorageQuota( mSlave, url );
  jobData jd( url.url(), parent );
  jd.cancellable = true;
  insertJob( job, jd );

  connect( job, SIGNAL(result(KIO::Job *)),
           SLOT(slotGetStorageQuotaInfoResult(KIO::Job *)) );
}

void ImapAccountBase::slotGetStorageQuotaInfoResult( KIO::Job *_job )
{
  QuotaJobs::GetStorageQuotaJob *job = static_cast<QuotaJobs::GetStorageQuotaJob *>( _job );
  JobIterator it = findJob( job );
  if ( it == jobsEnd() ) {
    return;
  }
  if ( job->error() && job->error() == KIO::ERR_UNSUPPORTED_ACTION ) {
    setHasNoQuotaSupport();
  }

  KMFolder *folder = (*it).parent; // can be 0
  emit receivedStorageQuotaInfo( folder, job, job->storageQuotaInfo() );
  if ( mSlave ) {
    removeJob( job );
  }
}

void ImapAccountBase::slotNoopTimeout()
{
  if ( mSlave ) {
    QByteArray packedArgs;
    QDataStream stream( &packedArgs, QIODevice::WriteOnly );

    stream << ( int ) 'N';

    KIO::SimpleJob *job = KIO::special( getUrl(), packedArgs, false );
    KIO::Scheduler::assignJobToSlave( mSlave, job );
    connect( job, SIGNAL(result( KJob * ) ),
             this, SLOT( slotSimpleResult( KJob * ) ) );
  } else {
    /* Stop the timer, we have disconnected. We have to make sure it is
       started again when a new slave appears. */
    mNoopTimer.stop();
  }
}

void ImapAccountBase::slotIdleTimeout()
{
  if ( mSlave ) {
    KIO::Scheduler::disconnectSlave( mSlave );
    mSlave = 0;
    mSlaveConnected = false;
    /* As for the noop timer, we need to make sure this one is started
       again when a new slave goes up. */
    mIdleTimer.stop();
  }
}

void ImapAccountBase::slotAbortRequested( KPIM::ProgressItem *item )
{
  if ( item ) {
    item->setComplete();
  }
  killAllJobs();
}

//-----------------------------------------------------------------------------
void ImapAccountBase::slotSchedulerSlaveError( KIO::Slave *aSlave,
                                               int errorCode,
                                               const QString &errorMsg )
{
  if ( aSlave != mSlave ) {
    return;
  }
  handleError( errorCode, errorMsg, 0, QString(), true );
  if ( mAskAgain ) {
    if ( makeConnection() != ImapAccountBase::Error ) {
      return;
    }
  }

  if ( !mSlaveConnected ) {
    mSlaveConnectionError = true;
    resetConnectionList( this );
    if ( mSlave ) {
      KIO::Scheduler::disconnectSlave( slave() );
      mSlave = 0;
    }
  }
  emit connectionResult( errorCode, errorMsg );
}

//-----------------------------------------------------------------------------
void ImapAccountBase::slotSchedulerSlaveConnected( KIO::Slave *aSlave )
{
  if ( aSlave != mSlave ) {
    return;
  }
  mSlaveConnected = true;
  mNoopTimer.start( 60000 ); // make sure we start sending noops
  emit connectionResult( 0, QString() ); // success

  if ( mNamespaces.isEmpty() || mNamespaceToDelimiter.isEmpty() ) {
    connect( this, SIGNAL( namespacesFetched( const ImapAccountBase::nsDelimMap& ) ),
             this, SLOT( slotSaveNamespaces( const ImapAccountBase::nsDelimMap& ) ) );
    getNamespaces();
  }

  // get capabilities
  QByteArray packedArgs;
  QDataStream stream( &packedArgs, QIODevice::WriteOnly );
  stream << (int) 'c';
  KIO::SimpleJob *job = KIO::special( getUrl(), packedArgs, false );
  KIO::Scheduler::assignJobToSlave( mSlave, job );
  connect( job, SIGNAL(infoMessage(KJob*, const QString&, const QString &)),
	   SLOT(slotCapabilitiesResult(KJob*, const QString&, const QString&)) );
}

//-----------------------------------------------------------------------------
void ImapAccountBase::slotCapabilitiesResult( KJob*, const QString& result, const QString & )
{
  mCapabilities = result.toLower().split( ' ', QString::SkipEmptyParts );
  kDebug(5006) << "capabilities:" << mCapabilities << endl;
}

//-----------------------------------------------------------------------------
void ImapAccountBase::getNamespaces()
{
  disconnect( this, SIGNAL( connectionResult(int, const QString&) ),
              this, SLOT( getNamespaces() ) );
  if ( makeConnection() != Connected || !mSlave ) {
    kDebug(5006) << "getNamespaces - wait for connection" << endl;
    if ( mNamespaces.isEmpty() || mNamespaceToDelimiter.isEmpty() ) {
      // when the connection is established slotSchedulerSlaveConnected notifies us
    } else {
      // getNamespaces was called by someone else
      connect( this, SIGNAL( connectionResult(int, const QString&) ),
               this, SLOT( getNamespaces() ) );
    }
    return;
  }

  QByteArray packedArgs;
  QDataStream stream( &packedArgs, QIODevice::WriteOnly );
  stream << (int) 'n';
  jobData jd;
  jd.total = 1; jd.done = 0; jd.cancellable = true;
  jd.progressItem = ProgressManager::createProgressItem(
    ProgressManager::getUniqueID(),
    i18n("Retrieving Namespaces"),
    QString(), true, useSSL() || useTLS() );
  jd.progressItem->setTotalItems( 1 );
  connect ( jd.progressItem,
            SIGNAL( progressItemCanceled( KPIM::ProgressItem* ) ),
            this,
            SLOT( slotAbortRequested( KPIM::ProgressItem* ) ) );
  KIO::SimpleJob *job = KIO::special( getUrl(), packedArgs, false );
  KIO::Scheduler::assignJobToSlave( mSlave, job );
  insertJob( job, jd );
  connect( job, SIGNAL( infoMessage(KJob*, const QString&, const QString&) ),
           SLOT( slotNamespaceResult(KJob*, const QString&, const QString&) ) );
}

//-----------------------------------------------------------------------------
void ImapAccountBase::slotNamespaceResult( KJob *job, const QString &str, const QString& )
{
  JobIterator it = findJob( static_cast<KIO::Job*>( job ) );
  if ( it == jobsEnd() ) {
    return;
  }

  nsDelimMap map;
  namespaceDelim nsDelim;
  QStringList ns = str.split( ",", QString::SkipEmptyParts );
  for ( QStringList::Iterator it = ns.begin(); it != ns.end(); ++it ) {
    // split, allow empty parts as we can get empty namespaces
    QStringList parts = (*it).split( "=" );
    imapNamespace section = imapNamespace( parts[0].toInt() );
    if ( map.contains( section ) ) {
      nsDelim = map[section];
    } else {
      nsDelim.clear();
    }
    // map namespace to delimiter
    nsDelim[parts[1]] = parts[2];
    map[section] = nsDelim;
  }
  removeJob( it );

  kDebug(5006) << "namespaces fetched" << endl;
  emit namespacesFetched( map );
}

//-----------------------------------------------------------------------------
void ImapAccountBase::slotSaveNamespaces( const ImapAccountBase::nsDelimMap &map )
{
  kDebug(5006) << "slotSaveNamespaces " << name() << endl;
  // extract the needed information
  mNamespaces.clear();
  mNamespaceToDelimiter.clear();
  for ( uint i = 0; i < 3; ++i ) {
    imapNamespace section = imapNamespace( i );
    namespaceDelim ns = map[ section ];
    namespaceDelim::ConstIterator it;
    QStringList list;
    for ( it = ns.begin(); it != ns.end(); ++it ) {
      list += it.key();
      mNamespaceToDelimiter[ it.key() ] = it.value();
    }
    if ( !list.isEmpty() ) {
      mNamespaces[section] = list;
    }
  }
  // see if we need to migrate an old prefix
  if ( !mOldPrefix.isEmpty() ) {
    migratePrefix();
  }
  emit namespacesFetched();
}

//-----------------------------------------------------------------------------
void ImapAccountBase::migratePrefix()
{
  if ( !mOldPrefix.isEmpty() && mOldPrefix != "/" ) {
    // strip /
    if ( mOldPrefix.startsWith("/") ) {
      mOldPrefix = mOldPrefix.right( mOldPrefix.length() - 1 );
    }
    if ( mOldPrefix.endsWith("/") ) {
      mOldPrefix = mOldPrefix.left( mOldPrefix.length() - 1 );
    }
    QStringList list = mNamespaces[PersonalNS];
    bool done = false;
    for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it ) {
      if ( (*it).startsWith( mOldPrefix ) ) {
        // should be ok
        done = true;
        kDebug(5006) << "migratePrefix - no migration needed" << endl;
        break;
      }
    }
    if ( !done ) {
      QString msg = i18n("KMail has detected a prefix entry in the "
                         "configuration of the account \"%1\" which is obsolete with the "
                         "support of IMAP namespaces.", name() );
      if ( list.contains( QString() ) ) {
        // replace empty entry with the old prefix
        list.removeAll( QString() );
        list += mOldPrefix;
        mNamespaces[PersonalNS] = list;
        if ( mNamespaceToDelimiter.contains( QString() ) ) {
          QString delim = mNamespaceToDelimiter[QString()];
          mNamespaceToDelimiter.remove( QString() );
          mNamespaceToDelimiter[mOldPrefix] = delim;
        }
        kDebug(5006) << "migratePrefix - replaced empty with " << mOldPrefix << endl;
        msg += i18n("The configuration was automatically migrated but you should check "
                    "your account configuration.");
      } else if ( list.count() == 1 ) {
        // only one entry in the personal namespace so replace it
        QString old = list.first();
        list.clear();
        list += mOldPrefix;
        mNamespaces[PersonalNS] = list;
        if ( mNamespaceToDelimiter.contains( old ) ) {
          QString delim = mNamespaceToDelimiter[old];
          mNamespaceToDelimiter.remove( old );
          mNamespaceToDelimiter[mOldPrefix] = delim;
        }
        kDebug(5006) << "migratePrefix - replaced single with " << mOldPrefix << endl;
        msg += i18n("The configuration was automatically migrated but you should check "
                    "your account configuration.");
      } else {
        kDebug(5006) << "migratePrefix - migration failed" << endl;
        msg += i18n("It was not possible to migrate your configuration automatically "
                    "so please check your account configuration.");
      }
      KMessageBox::information( kmkernel->getKMMainWidget(), msg );
    }
  } else {
    kDebug(5006) << "migratePrefix - no migration needed" << endl;
  }
  mOldPrefix.clear();
}

//-----------------------------------------------------------------------------
QString ImapAccountBase::namespaceForFolder( FolderStorage* storage )
{
  QString path;
  if ( storage->folderType() == KMFolderTypeImap ) {
    path = static_cast<KMFolderImap*>( storage )->imapPath();
  } else if ( storage->folderType() == KMFolderTypeCachedImap ) {
    path = static_cast<KMFolderCachedImap*>( storage )->imapPath();
  }

  nsMap::Iterator it;
  for ( it = mNamespaces.begin(); it != mNamespaces.end(); ++it ) {
    QStringList::Iterator strit;
    for ( strit = it.value().begin(); strit != it.value().end(); ++strit ) {
      QString ns = *strit;
      if ( ns.endsWith("/") || ns.endsWith(".") ) {
        // strip delimiter for the comparison
        ns = ns.left( ns.length()-1 );
      }
      // first ignore an empty prefix as it would match always
      if ( !ns.isEmpty() && path.contains( ns ) ) {
        return ( *strit );
      }
    }
  }
  return QString();
}

//-----------------------------------------------------------------------------
QString ImapAccountBase::delimiterForNamespace( const QString &prefix )
{
  kDebug(5006) << "delimiterForNamespace " << prefix << endl;
  // try to match exactly
  if ( mNamespaceToDelimiter.contains( prefix ) ) {
    return mNamespaceToDelimiter[prefix];
  }

  // then try if the prefix is part of a namespace
  // exclude empty namespace
  for ( namespaceDelim::ConstIterator it = mNamespaceToDelimiter.begin();
        it != mNamespaceToDelimiter.end(); ++it ) {
    // the namespace definition sometimes contains the delimiter
    // make sure we also match this version
    QString stripped = it.key().left( it.key().length() - 1 );
    if ( !it.key().isEmpty() &&
         ( prefix.contains( it.key() ) || prefix.contains( stripped ) ) ) {
      return it.value();
    }
  }
  // see if we have an empty namespace
  if ( mNamespaceToDelimiter.contains( QString() ) ) {
    return mNamespaceToDelimiter[QString()];
  }
  // well, we tried
  kDebug(5006) << "delimiterForNamespace - not found" << endl;
  return QString();
}

//-----------------------------------------------------------------------------
QString ImapAccountBase::delimiterForFolder( FolderStorage *storage )
{
  QString prefix = namespaceForFolder( storage );
  QString delim = delimiterForNamespace( prefix );
  return delim;
}

//-----------------------------------------------------------------------------
void ImapAccountBase::slotSimpleResult( KJob *job )
{
  JobIterator it = findJob( static_cast<KIO::Job*>( job ) );
  bool quiet = false;
  if ( it != mapJobData.end() ) {
    quiet = (*it).quiet;
    if ( !( job->error() && !quiet ) ) {
      // the error handler removes in that case
      removeJob( it );
    }
  }
  if ( job->error() ) {
    if ( !quiet ) {
      handleJobError( static_cast<KIO::Job*>( job ), QString() );
    } else {
      if ( job->error() == KIO::ERR_CONNECTION_BROKEN && slave() ) {
        // make sure ERR_CONNECTION_BROKEN is properly handled and the slave
        // disconnected even when quiet()
        KIO::Scheduler::disconnectSlave( slave() );
        mSlave = 0;
      }
      if (job->error() == KIO::ERR_SLAVE_DIED) {
        slaveDied();
      }
    }
  }
}

//-----------------------------------------------------------------------------
bool ImapAccountBase::handlePutError( KIO::Job *job, jobData &jd, KMFolder *folder )
{
  Q_ASSERT( !jd.msgList.isEmpty() );
  KMMessage* msg = jd.msgList.first();
  // Use double-quotes around the subject to keep the sentence readable,
  // but don't use double quotes around the sender since from() might
  // return a double-quoted name already
  const QString subject =
    msg->subject().isEmpty() ? i18n( "<unknown>" ) : QString("\"%1\"").arg( msg->subject() );
  const QString from = msg->from().isEmpty() ? i18n( "<unknown>" ) : msg->from();
  QString myError = "<p><b>" +
                    i18n("Error while uploading message") +
                    "</b></p><p>" +
                    i18n("Could not upload the message dated %1 from %2 with subject %3 on the server.", msg->dateStr(), Qt::escape( from ), Q3StyleSheet::escape( subject ) ) +
                    "</p><p>" +
                    i18n("The destination folder was %1, which has the URL %2.", Qt::escape( folder->label() ), Q3StyleSheet::escape( jd.htmlURL() ) ) +
                    "</p><p>" +
                    i18n("The error message from the server communication is here:") +
                    "</p>";
  return handleJobError( job, myError );
}

//-----------------------------------------------------------------------------
bool ImapAccountBase::handleError( int errorCode, const QString &errorMsg,
                                   KIO::Job* job, const QString& context, bool abortSync )
{
  // Copy job's data before a possible killAllJobs
  QStringList errors;
  if ( job && job->error() != KIO::ERR_SLAVE_DEFINED /*workaround for kdelibs-3.2*/) {
    errors = job->detailedErrorStrings();
  }

  bool jobsKilled = true;
  switch( errorCode ) {
  case KIO::ERR_SLAVE_DIED: slaveDied(); killAllJobs( true ); break;
  case KIO::ERR_COULD_NOT_AUTHENTICATE: // bad password
    mAskAgain = true;
    // fallthrough intended
  case KIO::ERR_CONNECTION_BROKEN:
  case KIO::ERR_COULD_NOT_CONNECT:
  case KIO::ERR_SERVER_TIMEOUT:
    // These mean that we'll have to reconnect on the next attempt,
    // so disconnect and set mSlave to 0.
    killAllJobs( true );
    break;
  case KIO::ERR_COULD_NOT_LOGIN:
  case KIO::ERR_USER_CANCELED:
    killAllJobs( false );
    break;
  default:
    if ( abortSync ) {
      killAllJobs( false );
    } else {
      jobsKilled = false;
    }
    break;
  }

  // check if we still display an error
  if ( !mErrorDialogIsActive && errorCode != KIO::ERR_USER_CANCELED ) {
    mErrorDialogIsActive = true;
    QString msg = context + '\n' + KIO::buildErrorString( errorCode, errorMsg );
    QString caption = i18n("Error");

    if ( jobsKilled || errorCode == KIO::ERR_COULD_NOT_LOGIN ) {
      if ( errorCode == KIO::ERR_SERVER_TIMEOUT || errorCode == KIO::ERR_CONNECTION_BROKEN ) {
        msg = i18n("The connection to the server %1 was unexpectedly closed or timed out. It will be re-established automatically if possible.",
                   name() );
        KMessageBox::information( QApplication::activeWindow(), msg, caption, "kmailConnectionBrokenErrorDialog" );
        // Show it in the status bar, in case the user has ticked "don't show again"
        if ( errorCode == KIO::ERR_CONNECTION_BROKEN ) {
          KPIM::BroadcastStatus::instance()->setStatusMsg(
            i18n(  "The connection to account %1 was broken.", name() ) );
        } else if ( errorCode == KIO::ERR_SERVER_TIMEOUT ) {
          KPIM::BroadcastStatus::instance()->setStatusMsg(
            i18n(  "The connection to account %1 timed out.", name() ) );
        }
      } else {
        if ( !errors.isEmpty() ) {
          KMessageBox::detailedError( QApplication::activeWindow(), msg,
                                      errors.join("\n").prepend("<qt>"), caption );
        } else {
          KMessageBox::error( QApplication::activeWindow(), msg, caption );
        }
      }
    } else { // i.e. we have a chance to continue, ask the user about it
      if ( errors.count() >= 3 ) { // there is no detailedWarningContinueCancel... (#86517)
        msg = QString( "<qt>") + context + errors[1] + '\n' + errors[2];
        caption = errors[0];
      }
      int ret = KMessageBox::warningContinueCancel( QApplication::activeWindow(), msg, caption );
      if ( ret == KMessageBox::Cancel ) {
        jobsKilled = true;
        killAllJobs( false );
      }
    }
    mErrorDialogIsActive = false;
  } else {
    if ( mErrorDialogIsActive ) {
      kDebug(5006) << "suppressing error:" << errorMsg << endl;
    }
  }
  if ( job && !jobsKilled ) {
    removeJob( job );
  }
  return !jobsKilled; // jobsKilled==false -> continue==true
}

//-----------------------------------------------------------------------------
void ImapAccountBase::cancelMailCheck()
{
  QMap<KJob*, jobData>::Iterator it = mapJobData.begin();
  while ( it != mapJobData.end() ) {
    kDebug(5006) << "cancelMailCheck: job is cancellable: " << (*it).cancellable << endl;
    if ( (*it).cancellable ) {
      it.key()->kill();
      QMap<KJob*, jobData>::Iterator rmit = it;
      ++it;
      mapJobData.erase( rmit );
      // We killed a job -> this kills the slave
      mSlave = 0;
    } else {
      ++it;
    }
  }

  QList<FolderJob*>::iterator jt;
  for ( jt = mJobList.begin(); jt != mJobList.end(); ++jt ) {
    if ( (*jt)->isCancellable() ) {
      FolderJob* job = (*jt);
      job->setPassiveDestructor( true );
      jt = mJobList.erase( jt );
      delete job;
    } else {
      ++jt;
    }
  }
}

//-----------------------------------------------------------------------------
QString ImapAccountBase::jobData::htmlURL() const
{
  KUrl u( url );
  return Qt::escape( u.prettyUrl() );
}

//-----------------------------------------------------------------------------
void ImapAccountBase::processNewMailSingleFolder( KMFolder *folder )
{
  mFoldersQueuedForChecking.append( folder );
  mCheckingSingleFolder = true;
  if ( checkingMail() ) {
    disconnect( this, SIGNAL( finishedCheck( bool, CheckStatus ) ),
                this, SLOT( slotCheckQueuedFolders() ) );
    connect( this, SIGNAL( finishedCheck( bool, CheckStatus ) ),
             this, SLOT( slotCheckQueuedFolders() ) );
  } else {
    slotCheckQueuedFolders();
  }
}

//-----------------------------------------------------------------------------
void ImapAccountBase::slotCheckQueuedFolders()
{
  disconnect( this, SIGNAL( finishedCheck( bool, CheckStatus ) ),
              this, SLOT( slotCheckQueuedFolders() ) );

  QList<QPointer<KMFolder> > mSaveList = mMailCheckFolders;
  mMailCheckFolders = mFoldersQueuedForChecking;
  if ( kmkernel->acctMgr() ) {
    kmkernel->acctMgr()->singleCheckMail( this, true );
  }
  mMailCheckFolders = mSaveList;
  mFoldersQueuedForChecking.clear();
}

//-----------------------------------------------------------------------------
bool ImapAccountBase::checkingMail( KMFolder *folder )
{
  if ( checkingMail() && mFoldersQueuedForChecking.contains( folder ) ) {
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void ImapAccountBase::handleBodyStructure( QDataStream &stream, KMMessage *msg,
                                           const AttachmentStrategy *as )
{
  mBodyPartList.clear();
  mCurrentMsg = msg;
  // first delete old parts as we construct our own
  msg->deleteBodyParts();
  // make the parts and fill the mBodyPartList
  constructParts( stream, 1, 0, 0, msg->asDwMessage() );
  if ( mBodyPartList.count() == 1 ) {
    // we directly set the body later, at partsToLoad below
    msg->deleteBodyParts();
  }

  if ( !as ) {
    kWarning(5006) << k_funcinfo << " - found no attachment strategy!" << endl;
    return;
  }

  // see what parts have to loaded according to attachmentstrategy
  BodyVisitor *visitor = BodyVisitorFactory::getVisitor( as );
  visitor->visit( mBodyPartList );
  QList<KMMessagePart*> parts = visitor->partsToLoad();
  delete visitor;
  QList<KMMessagePart*>::const_iterator it;
  int partsToLoad = 0;
  // check how many parts we have to load
  for ( it = parts.begin(); it != parts.end(); ++it ) {
    if ( (*it) && (*it)->loadPart() ) {
      ++partsToLoad;
    }
  }
  // if the only body part is not text, part->loadPart() would return false
  // and that part is never loaded, so make sure it loads.
  // it seems that TEXT does load the single body part even if it is not text/*
  if ( mBodyPartList.count() == 1 && partsToLoad == 0 ) {
    partsToLoad = 1; // this causes the next test to succeed, and loads the whole message
  }

  if ( (mBodyPartList.count() * 0.5) < partsToLoad ) {
    // more than 50% of the parts have to be loaded anyway so it is faster
    // to load the message completely
    kDebug(5006) << "Falling back to normal mode" << endl;
    FolderJob *job =
      msg->parent()->createJob( msg, FolderJob::tGetMessage, 0, "TEXT" );
    job->start();
    return;
  }
  for ( it = parts.begin(); it != parts.end(); ++it ) {
    KMMessagePart *part = (*it);
    kDebug(5006) << "ImapAccountBase::handleBodyStructure - load " << part->partSpecifier()
                 << " (" << part->originalContentTypeStr() << ")" << endl;
    if ( part->loadHeaders() ) {
      kDebug(5006) << "load HEADER" << endl;
      FolderJob *job =
        msg->parent()->createJob( msg, FolderJob::tGetMessage, 0, part->partSpecifier()+".MIME" );
      job->start();
    }
    if ( part->loadPart() ) {
      kDebug(5006) << "load Part" << endl;
      FolderJob *job =
        msg->parent()->createJob( msg, FolderJob::tGetMessage, 0, part->partSpecifier() );
      job->start();
    }
  }
}

//-----------------------------------------------------------------------------
void ImapAccountBase::constructParts( QDataStream &stream, int count, KMMessagePart *parentKMPart,
                                      DwBodyPart *parent, const DwMessage *dwmsg )
{
  int children;
  for (int i = 0; i < count; i++) {
    stream >> children;
    KMMessagePart* part = new KMMessagePart( stream );
    part->setParent( parentKMPart );
    mBodyPartList.append( part );
    kDebug(5006) << "ImapAccountBase::constructParts - created id " << part->partSpecifier()
                 << " of type " << part->originalContentTypeStr() << endl;
    DwBodyPart *dwpart = mCurrentMsg->createDWBodyPart( part );

    if ( parent ) {
      // add to parent body
      parent->Body().AddBodyPart( dwpart );
      dwpart->Parse();
    } else if ( part->partSpecifier() != "0" &&
                !part->partSpecifier().endsWith(".HEADER") ) {
      // add to message
      dwmsg->Body().AddBodyPart( dwpart );
      dwpart->Parse();
    } else {
      dwpart = 0;
    }

    if ( !parentKMPart ) {
      parentKMPart = part;
    }

    if ( children > 0 ) {
      DwBodyPart *newparent = dwpart;
      const DwMessage *newmsg = dwmsg;
      if ( part->originalContentTypeStr() == "MESSAGE/RFC822" && dwpart &&
           dwpart->Body().Message() ) {
        // set the encapsulated message as the new message
        newparent = 0;
        newmsg = dwpart->Body().Message();
      }
      KMMessagePart* newParentKMPart = part;
      if ( part->partSpecifier().endsWith(".HEADER") ) {
        // we don't want headers as parent
        newParentKMPart = parentKMPart;
      }

      constructParts( stream, children, newParentKMPart, newparent, newmsg );
    }
  }
}

//-----------------------------------------------------------------------------
void ImapAccountBase::setImapStatus( KMFolder *folder, const QString &path,
                                     const QByteArray &flags )
{
  // set the status on the server, the uids are integrated in the path
  kDebug(5006) << "setImapStatus path=" << path << " to: " << flags << endl;
  KUrl url = getUrl();
  url.setPath( path );

  QByteArray packedArgs;
  QDataStream stream( &packedArgs, QIODevice::WriteOnly );

  stream << (int) 'S' << url << flags;

  if ( makeConnection() != Connected ) {
    return; // can't happen with dimap
  }

  KIO::SimpleJob *job = KIO::special( url, packedArgs, false );
  KIO::Scheduler::assignJobToSlave( slave(), job );
  ImapAccountBase::jobData jd( url.url(), folder );
  jd.path = path;
  insertJob( job, jd );
  connect( job, SIGNAL(result(KJob *)),
           SLOT(slotSetStatusResult(KJob *)) );
}
//-----------------------------------------------------------------------------
void ImapAccountBase::slotSetStatusResult( KJob *job )
{
  ImapAccountBase::JobIterator it = findJob( static_cast<KIO::Job*>( job ) );
  if ( it == jobsEnd() ) {
    return;
  }
  int errorCode = job->error();
  KMFolder * const parent = (*it).parent;
  const QString path = (*it).path;
  if ( errorCode && errorCode != KIO::ERR_CANNOT_OPEN_FOR_WRITING ) {
    bool cont = handleJobError( static_cast<KIO::Job*>(job),
                                i18n("Error while uploading status of messages to server: ") + '\n' );
    emit imapStatusChanged( parent, path, cont );
  } else {
    emit imapStatusChanged( parent, path, true );
    removeJob( it );
  }
}

//-----------------------------------------------------------------------------
void ImapAccountBase::setFolder( KMFolder *folder, bool addAccount )
{
  if ( folder ) {
    folder->setSystemLabel( name() );
    folder->setId( id() );
  }
  NetworkAccount::setFolder( folder, addAccount );
}

//-----------------------------------------------------------------------------
void ImapAccountBase::removeJob( JobIterator&it )
{
  if ( (*it).progressItem ) {
    (*it).progressItem->setComplete();
    (*it).progressItem = 0;
  }
  mapJobData.erase( it );
}

//-----------------------------------------------------------------------------
void KMail::ImapAccountBase::removeJob( KIO::Job *job )
{
  mapJobData.remove( job );
}

//-----------------------------------------------------------------------------
KPIM::ProgressItem* ImapAccountBase::listDirProgressItem()
{
  if ( !mListDirProgressItem ) {
    mListDirProgressItem = ProgressManager::createProgressItem(
      "ListDir" + name(),
      name(),
      i18n("retrieving folders"),
      true,
      useSSL() || useTLS() );
    connect ( mListDirProgressItem,
              SIGNAL( progressItemCanceled( KPIM::ProgressItem* ) ),
              this,
              SLOT( slotAbortRequested( KPIM::ProgressItem* ) ) );
    // Start with a guessed value of the old folder count plus 5%. As long
    // as the list of folders doesn't constantly change, that should be good
    // enough.
    unsigned int count = folderCount();
    mListDirProgressItem->setTotalItems( count + (unsigned int)(count*0.05) );
  }
  return mListDirProgressItem;
}

//-----------------------------------------------------------------------------
unsigned int ImapAccountBase::folderCount() const
{
  if ( !rootFolder() || !rootFolder()->folder() || !rootFolder()->folder()->child() ) {
    return 0;
  }
  return kmkernel->imapFolderMgr()->folderCount( rootFolder()->folder()->child() );
}

//-----------------------------------------------------------------------------
QString ImapAccountBase::addPathToNamespace( const QString &prefix )
{
  QString myPrefix = prefix;
  if ( !myPrefix.startsWith( "/" ) ) {
    myPrefix = '/' + myPrefix;
  }
  if ( !myPrefix.endsWith( "/" ) ) {
    myPrefix += '/';
  }

  return myPrefix;
}

//-----------------------------------------------------------------------------
bool ImapAccountBase::isNamespaceFolder( QString &name )
{
  QStringList ns = mNamespaces[OtherUsersNS];
  ns += mNamespaces[SharedNS];
  ns += mNamespaces[PersonalNS];
  QString nameWithDelimiter;
  for ( QStringList::Iterator it = ns.begin(); it != ns.end(); ++it ) {
    nameWithDelimiter = name + delimiterForNamespace( *it );
    if ( *it == name || *it == nameWithDelimiter ) {
      return true;
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
ImapAccountBase::nsDelimMap ImapAccountBase::namespacesWithDelimiter()
{
  nsDelimMap map;
  nsMap::ConstIterator it;
  for ( uint i = 0; i < 3; ++i ) {
    imapNamespace section = imapNamespace( i );
    QStringList namespaces = mNamespaces[section];
    namespaceDelim nsDelim;
    QStringList::Iterator lit;
    for ( lit = namespaces.begin(); lit != namespaces.end(); ++lit ) {
      nsDelim[*lit] = delimiterForNamespace( *lit );
    }
    map[section] = nsDelim;
  }
  return map;
}

//-----------------------------------------------------------------------------
QString ImapAccountBase::createImapPath( const QString &parent,
                                         const QString &folderName )
{
  QString newName = parent;
  // strip / at the end
  if ( newName.endsWith( "/" ) ) {
    newName = newName.left( newName.length() - 1 );
  }
  // add correct delimiter
  QString delim = delimiterForNamespace( newName );
  // should not happen...
  if ( delim.isEmpty() ) {
    delim = "/";
  }
  if ( !newName.isEmpty() &&
       !newName.endsWith( delim ) && !folderName.startsWith( delim ) ) {
    newName = newName + delim;
  }
  newName = newName + folderName;
  // add / at the end
  if ( !newName.endsWith("/") ) {
    newName = newName + '/';
  }

  return newName;
}

//-----------------------------------------------------------------------------
QString ImapAccountBase::createImapPath( FolderStorage *parent,
                                         const QString &folderName )
{
  QString path;
  if ( parent->folderType() == KMFolderTypeImap ) {
    path = static_cast<KMFolderImap*>( parent )->imapPath();
  } else if ( parent->folderType() == KMFolderTypeCachedImap ) {
    path = static_cast<KMFolderCachedImap*>( parent )->imapPath();
  } else {
    // error
    return path;
  }

  return createImapPath( path, folderName );
}

bool ImapAccountBase::locallySubscribedTo( const QString &imapPath )
{
  return mLocalSubscriptionBlackList.find( imapPath ) == mLocalSubscriptionBlackList.end();
}

void ImapAccountBase::changeLocalSubscription( const QString &imapPath, bool subscribe )
{
  if ( subscribe ) {
    // find in blacklist and remove from it
    mLocalSubscriptionBlackList.erase( imapPath );
  } else {
    // blacklist
    mLocalSubscriptionBlackList.insert( imapPath );
  }
}

QStringList ImapAccountBase::locallyBlacklistedFolders() const
{
  QStringList list;
  std::set<QString>::const_iterator it = mLocalSubscriptionBlackList.begin();
  std::set<QString>::const_iterator end = mLocalSubscriptionBlackList.end();
  for ( ; it != end ; ++it ) {
    list.append( *it );
  }
  return list;
}

void ImapAccountBase::localBlacklistFromStringList( const QStringList &list )
{
  for ( QStringList::ConstIterator it = list.constBegin( ); it != list.constEnd( ); ++it ) {
    mLocalSubscriptionBlackList.insert( *it );
  }
}

} // namespace KMail

#include "imapaccountbase.moc"
