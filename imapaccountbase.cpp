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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "imapaccountbase.h"
using KMail::SieveConfig;

#include "kmacctmgr.h"
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

#include <kapplication.h>
#include <kdebug.h>
#include <kconfig.h>
#include <klocale.h>
#include <kmessagebox.h>
using KIO::MetaData;
#include <kio/passdlg.h>
using KIO::PasswordDialog;
#include <kio/scheduler.h>
#include <kio/slave.h>
#include <mimelib/bodypart.h>
#include <mimelib/body.h>
#include <mimelib/headers.h>
#include <mimelib/message.h>
//using KIO::Scheduler; // use FQN below

#include <qregexp.h>
#include <qstylesheet.h>

namespace KMail {

  static const unsigned short int imapDefaultPort = 143;

  //
  //
  // Ctor and Dtor
  //
  //

  ImapAccountBase::ImapAccountBase( KMAcctMgr * parent, const QString & name, uint id )
    : NetworkAccount( parent, name, id ),
      mPrefix( "/" ),
      mTotal( 0 ),
      mCountUnread( 0 ),
      mCountLastUnread( 0 ),
      mAutoExpunge( true ),
      mHiddenFolders( false ),
      mOnlySubscribedFolders( false ),
      mLoadOnDemand( true ),
      mListOnlyOpenFolders( false ),
      mProgressEnabled( false ),
      mErrorDialogIsActive( false ),
      mPasswordDialogIsActive( false ),
      mACLSupport( true ),
      mAnnotationSupport( true ),
      mSlaveConnected( false ),
      mSlaveConnectionError( false ),
      mListDirProgressItem( 0 )
  {
    mPort = imapDefaultPort;
    mBodyPartList.setAutoDelete(true);
    KIO::Scheduler::connect(SIGNAL(slaveError(KIO::Slave *, int, const QString &)),
                            this, SLOT(slotSchedulerSlaveError(KIO::Slave *, int, const QString &)));
    KIO::Scheduler::connect(SIGNAL(slaveConnected(KIO::Slave *)),
                            this, SLOT(slotSchedulerSlaveConnected(KIO::Slave *)));
    connect(&mNoopTimer, SIGNAL(timeout()), SLOT(slotNoopTimeout()));
    connect(&mIdleTimer, SIGNAL(timeout()), SLOT(slotIdleTimeout()));
  }

  ImapAccountBase::~ImapAccountBase() {
    kdWarning( mSlave, 5006 )
      << "slave should have been destroyed by subclass!" << endl;
  }

  void ImapAccountBase::init() {
    mPrefix = '/';
    mAutoExpunge = true;
    mHiddenFolders = false;
    mOnlySubscribedFolders = false;
    mLoadOnDemand = true;
    mListOnlyOpenFolders = false;
    mProgressEnabled = false;
  }

  void ImapAccountBase::pseudoAssign( const KMAccount * a ) {
    NetworkAccount::pseudoAssign( a );

    const ImapAccountBase * i = dynamic_cast<const ImapAccountBase*>( a );
    if ( !i ) return;

    setPrefix( i->prefix() );
    setAutoExpunge( i->autoExpunge() );
    setHiddenFolders( i->hiddenFolders() );
    setOnlySubscribedFolders( i->onlySubscribedFolders() );
    setLoadOnDemand( i->loadOnDemand() );
    setListOnlyOpenFolders( i->listOnlyOpenFolders() );
  }

  unsigned short int ImapAccountBase::defaultPort() const {
    return imapDefaultPort;
  }

  QString ImapAccountBase::protocol() const {
    return useSSL() ? IMAP_SSL_PROTOCOL : IMAP_PROTOCOL;
  }

  //
  //
  // Getters and Setters
  //
  //

  void ImapAccountBase::setPrefix( const QString & prefix ) {
    mPrefix = prefix;
    mPrefix.remove( QRegExp( "[%*\"]" ) );
    if ( mPrefix.isEmpty() || mPrefix[0] != '/' )
      mPrefix.prepend( '/' );
    if ( mPrefix[ mPrefix.length() - 1 ] != '/' )
      mPrefix += '/';
#if 1
    setPrefixHook(); // ### needed while KMFolderCachedImap exists
#else
    if ( mFolder ) mFolder->setImapPath( mPrefix );
#endif
  }

  void ImapAccountBase::setAutoExpunge( bool expunge ) {
    mAutoExpunge = expunge;
  }

  void ImapAccountBase::setHiddenFolders( bool show ) {
    mHiddenFolders = show;
  }

  void ImapAccountBase::setOnlySubscribedFolders( bool show ) {
    mOnlySubscribedFolders = show;
  }

  void ImapAccountBase::setLoadOnDemand( bool load ) {
    mLoadOnDemand = load;
  }

  void ImapAccountBase::setListOnlyOpenFolders( bool only ) {
    mListOnlyOpenFolders = only;
  }

  //
  //
  // read/write config
  //
  //

  void ImapAccountBase::readConfig( /*const*/ KConfig/*Base*/ & config ) {
    NetworkAccount::readConfig( config );

    setPrefix( config.readEntry( "prefix", "/" ) );
    setAutoExpunge( config.readBoolEntry( "auto-expunge", false ) );
    setHiddenFolders( config.readBoolEntry( "hidden-folders", false ) );
    setOnlySubscribedFolders( config.readBoolEntry( "subscribed-folders", false ) );
    setLoadOnDemand( config.readBoolEntry( "loadondemand", false ) );
    setListOnlyOpenFolders( config.readBoolEntry( "listOnlyOpenFolders", false ) );
  }

  void ImapAccountBase::writeConfig( KConfig/*Base*/ & config ) /*const*/ {
    NetworkAccount::writeConfig( config );

    config.writeEntry( "prefix", prefix() );
    config.writeEntry( "auto-expunge", autoExpunge() );
    config.writeEntry( "hidden-folders", hiddenFolders() );
    config.writeEntry( "subscribed-folders", onlySubscribedFolders() );
    config.writeEntry( "loadondemand", loadOnDemand() );
    config.writeEntry( "listOnlyOpenFolders", listOnlyOpenFolders() );
  }

  //
  //
  // Network processing
  //
  //

  MetaData ImapAccountBase::slaveConfig() const {
    MetaData m = NetworkAccount::slaveConfig();

    m.insert( "auth", auth() );
    if ( autoExpunge() )
      m.insert( "expunge", "auto" );

    return m;
  }

  ImapAccountBase::ConnectionState ImapAccountBase::makeConnection() {

    if ( mSlave && mSlaveConnected ) return Connected;
    if ( mPasswordDialogIsActive ) return Connecting;

    if( mAskAgain || ( ( passwd().isEmpty() || login().isEmpty() ) &&
                         auth() != "GSSAPI" ) ) {

      Q_ASSERT( !mSlave ); // disconnected on 'wrong login' error already, or first try
      QString log = login();
      QString pass = passwd();
      // We init "store" to true to indicate that we want to have the
      // "keep password" checkbox. Then, we set [Passwords]Keep to
      // storePasswd(), so that the checkbox in the dialog will be
      // init'ed correctly:
      bool store = true;
      KConfigGroup passwords( KGlobal::config(), "Passwords" );
      passwords.writeEntry( "Keep", storePasswd() );
      QString msg = i18n("You need to supply a username and a password to "
			 "access this mailbox.");
      mPasswordDialogIsActive = true;
      if ( PasswordDialog::getNameAndPassword( log, pass, &store, msg, false,
					       QString::null, name(),
					       i18n("Account:") )
          != QDialog::Accepted ) {
        mPasswordDialogIsActive = false;
        mAskAgain = false;
        emit connectionResult( KIO::ERR_USER_CANCELED, QString::null );
        return Error;
      }
      mPasswordDialogIsActive = false;
      // The user has been given the chance to change login and
      // password, so copy both from the dialog:
      setPasswd( pass, store );
      setLogin( log );
      mAskAgain = false;
    }
    // already waiting for a connection?
    if ( mSlave && !mSlaveConnected ) return Connecting;

    mSlaveConnected = false;
    mSlave = KIO::Scheduler::getConnectedSlave( getUrl(), slaveConfig() );
    if ( !mSlave ) {
      KMessageBox::error(0, i18n("Could not start process for %1.")
			 .arg( getUrl().protocol() ) );
      return Error;
    }
    if ( mSlave->isConnected() ) {
      mSlaveConnected = true;
      return Connected;
    }

    return Connecting;
  }

  bool ImapAccountBase::handleJobError( KIO::Job *job, const QString& context, bool abortSync )
  {
    return handleError( job->error(), job->errorText(), job, context, abortSync );
  }

  // Called when we're really all done.
  void ImapAccountBase::postProcessNewMail() {
    setCheckingMail(false);
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
    BroadcastStatus::instance()->setStatusMsgTransmissionCompleted(
        name(), newMails);
  }

  //-----------------------------------------------------------------------------
  void ImapAccountBase::changeSubscription( bool subscribe, QString imapPath )
  {
    // change the subscription of the folder
    KURL url = getUrl();
    url.setPath(imapPath);

    QByteArray packedArgs;
    QDataStream stream( packedArgs, IO_WriteOnly);

    if (subscribe)
      stream << (int) 'u' << url;
    else
      stream << (int) 'U' << url;

    // create the KIO-job
    if (makeConnection() != Connected) // ## doesn't handle Connecting
      return;
    KIO::SimpleJob *job = KIO::special(url, packedArgs, FALSE);
    KIO::Scheduler::assignJobToSlave(mSlave, job);
    jobData jd( url.url(), NULL );
    // a bit of a hack to save one slot
    if (subscribe) jd.onlySubscribed = true;
    else jd.onlySubscribed = false;
    insertJob(job, jd);

    connect(job, SIGNAL(result(KIO::Job *)),
        SLOT(slotSubscriptionResult(KIO::Job *)));
  }

  //-----------------------------------------------------------------------------
  void ImapAccountBase::slotSubscriptionResult( KIO::Job * job )
  {
    // result of a subscription-job
    JobIterator it = findJob( job );
    if ( it == jobsEnd() ) return;
    bool onlySubscribed = (*it).onlySubscribed;
    QString path = static_cast<KIO::SimpleJob*>(job)->url().path();
    if (job->error())
    {
      handleJobError( job, i18n( "Error while trying to subscribe to %1:" ).arg( path ) + '\n' );
      // ## emit subscriptionChanged here in case anyone needs it to support continue/cancel
    }
    else
    {
      emit subscriptionChanged( path, onlySubscribed );
      if (mSlave) removeJob(job);
    }
  }

  //-----------------------------------------------------------------------------
  // TODO imapPath can be removed once parent can be a KMFolderImapBase or whatever
  void ImapAccountBase::getUserRights( KMFolder* parent, const QString& imapPath )
  {
    // There isn't much point in asking the server about a user's rights on his own inbox,
    // it might not be the effective permissions (at least with Cyrus, one can admin his own inbox,
    // even after a SETACL that removes the admin permissions. Other imap servers apparently
    // don't even allow removing one's own admin permission, so this code won't hurt either).
    if ( imapPath == "/INBOX/" ) {
      if ( parent->folderType() == KMFolderTypeImap )
        static_cast<KMFolderImap*>( parent->storage() )->setUserRights( ACLJobs::All );
      else if ( parent->folderType() == KMFolderTypeCachedImap )
        static_cast<KMFolderCachedImap*>( parent->storage() )->setUserRights( ACLJobs::All );
      emit receivedUserRights( parent ); // warning, you need to connect first to get that one
      return;
    }

    KURL url = getUrl();
    url.setPath(imapPath);

    ACLJobs::GetUserRightsJob* job = ACLJobs::getUserRights( mSlave, url );

    jobData jd( url.url(), parent );
    jd.cancellable = true;
    insertJob(job, jd);

    connect(job, SIGNAL(result(KIO::Job *)),
            SLOT(slotGetUserRightsResult(KIO::Job *)));
  }

  void ImapAccountBase::slotGetUserRightsResult( KIO::Job* _job )
  {
    ACLJobs::GetUserRightsJob* job = static_cast<ACLJobs::GetUserRightsJob *>( _job );
    JobIterator it = findJob( job );
    if ( it == jobsEnd() ) return;

    KMFolder* folder = (*it).parent;
    if ( job->error() ) {
      if ( job->error() == KIO::ERR_UNSUPPORTED_ACTION ) // that's when the imap server doesn't support ACLs
          mACLSupport = false;
      else
        kdWarning(5006) << "slotGetUserRightsResult: " << job->errorString() << endl;
    } else {
#ifndef NDEBUG
      //kdDebug(5006) << "User Rights: " << ACLJobs::permissionsToString( job->permissions() ) << endl;
#endif
      // Store the permissions
      if ( folder->folderType() == KMFolderTypeImap )
        static_cast<KMFolderImap*>( folder->storage() )->setUserRights( job->permissions() );
      else if ( folder->folderType() == KMFolderTypeCachedImap )
        static_cast<KMFolderCachedImap*>( folder->storage() )->setUserRights( job->permissions() );
    }
    if (mSlave) removeJob(job);
    emit receivedUserRights( folder );
  }

  //-----------------------------------------------------------------------------
  void ImapAccountBase::getACL( KMFolder* parent, const QString& imapPath )
  {
    KURL url = getUrl();
    url.setPath(imapPath);

    ACLJobs::GetACLJob* job = ACLJobs::getACL( mSlave, url );
    jobData jd( url.url(), parent );
    jd.cancellable = true;
    insertJob(job, jd);

    connect(job, SIGNAL(result(KIO::Job *)),
            SLOT(slotGetACLResult(KIO::Job *)));
  }

  void ImapAccountBase::slotGetACLResult( KIO::Job* _job )
  {
    ACLJobs::GetACLJob* job = static_cast<ACLJobs::GetACLJob *>( _job );
    JobIterator it = findJob( job );
    if ( it == jobsEnd() ) return;

    KMFolder* folder = (*it).parent;
    emit receivedACL( folder, job, job->entries() );
    if (mSlave) removeJob(job);
  }


  void ImapAccountBase::slotNoopTimeout()
  {
      if ( mSlave ) {
        QByteArray packedArgs;
        QDataStream stream( packedArgs, IO_WriteOnly );

        stream << ( int ) 'N';

        KIO::SimpleJob *job = KIO::special( getUrl(), packedArgs, false );
        KIO::Scheduler::assignJobToSlave(mSlave, job);
        connect( job, SIGNAL(result( KIO::Job * ) ),
          this, SLOT( slotSimpleResult( KIO::Job * ) ) );
      } else {
        /* Stop the timer, we have disconnected. We have to make sure it is
           started again when a new slave appears. */
        mNoopTimer.stop();
      }
  }

  void ImapAccountBase::slotIdleTimeout()
  {
      if ( mSlave ) {
        KIO::Scheduler::disconnectSlave(mSlave);
        mSlave = 0;
        mSlaveConnected = false;
        /* As for the noop timer, we need to make sure this one is started
           again when a new slave goes up. */
        mIdleTimer.stop();
      }
  }

  void ImapAccountBase::slotAbortRequested( KPIM::ProgressItem* item )
  {
    if ( item )
      item->setComplete();
    killAllJobs();
  }


  //-----------------------------------------------------------------------------
  void ImapAccountBase::slotSchedulerSlaveError(KIO::Slave *aSlave, int errorCode,
      const QString &errorMsg)
  {
      if (aSlave != mSlave) return;
      handleError( errorCode, errorMsg, 0, QString::null, true );
      if ( mAskAgain )
        makeConnection();
      else {
        if ( !mSlaveConnected )
          mSlaveConnectionError = true;
        emit connectionResult( errorCode, errorMsg );
      }
  }

  //-----------------------------------------------------------------------------
  void ImapAccountBase::slotSchedulerSlaveConnected(KIO::Slave *aSlave)
  {
      if (aSlave != mSlave) return;
      mSlaveConnected = true;
      emit connectionResult( 0, QString::null ); // success
  }

  //-----------------------------------------------------------------------------
  void ImapAccountBase::slotSimpleResult(KIO::Job * job)
  {
    JobIterator it = findJob( job );
    bool quiet = false;
    if (it != mapJobData.end()) {
      quiet = (*it).quiet;
      if ( !(job->error() && !quiet) ) // the error handler removes in that case
        removeJob(it);
    }
    if (job->error()) {
      if (!quiet)
        handleJobError(job, QString::null );
      else {
        if ( job->error() == KIO::ERR_CONNECTION_BROKEN && slave() ) {
          // make sure ERR_CONNECTION_BROKEN is properly handled and the slave
          // disconnected even when quiet()
          KIO::Scheduler::disconnectSlave( slave() );
          mSlave = 0;
        }
        if (job->error() == KIO::ERR_SLAVE_DIED)
          slaveDied();
      }
    }
  }

  //-----------------------------------------------------------------------------
  bool ImapAccountBase::handlePutError( KIO::Job* job, jobData& jd, KMFolder* folder )
  {
    Q_ASSERT( !jd.msgList.isEmpty() );
    KMMessage* msg = jd.msgList.first();
    // Use double-quotes around the subject to keep the sentence readable,
    // but don't use double quotes around the sender since from() might return a double-quoted name already
    const QString subject = msg->subject().isEmpty() ? i18n( "<unknown>" ) : QString("\"%1\"").arg( msg->subject() );
    const QString from = msg->from().isEmpty() ? i18n( "<unknown>" ) : msg->from();
    QString myError = "<p><b>" + i18n("Error while uploading message")
      + "</b></p><p>"
      + i18n("Could not upload the message dated %1 from %2 with subject %3 on the server.").arg( msg->dateStr(), QStyleSheet::escape( from ), QStyleSheet::escape( subject ) )
      + "</p><p>"
      + i18n("The destination folder was %1, which has the URL %2.").arg( QStyleSheet::escape( folder->label() ), QStyleSheet::escape( jd.htmlURL() ) )
      + "</p><p>"
      + i18n("The error message from the server communication is here:") + "</p>";
    return handleJobError( job, myError );
  }

  //-----------------------------------------------------------------------------
  bool ImapAccountBase::handleError( int errorCode, const QString &errorMsg, KIO::Job* job, const QString& context, bool abortSync )
  {
    // Copy job's data before a possible killAllJobs
    QStringList errors;
    if ( job && job->error() != KIO::ERR_SLAVE_DEFINED /*workaround for kdelibs-3.2*/)
      errors = job->detailedErrorStrings();

    bool jobsKilled = true;
    switch( errorCode ) {
    case KIO::ERR_SLAVE_DIED: slaveDied(); killAllJobs( true ); break;
    case KIO::ERR_COULD_NOT_LOGIN: // bad password
      mAskAgain = true;
      // fallthrough intended
    case KIO::ERR_CONNECTION_BROKEN:
    case KIO::ERR_COULD_NOT_CONNECT:
      // These mean that we'll have to reconnect on the next attempt, so disconnect and set mSlave to 0.
      killAllJobs( true );
      break;
    case KIO::ERR_USER_CANCELED:
      killAllJobs( false );
      break;
    default:
      if ( abortSync )
        killAllJobs( false );
      else
        jobsKilled = false;
      break;
    }

    // check if we still display an error
    if ( !mErrorDialogIsActive && errorCode != KIO::ERR_USER_CANCELED )
    {
      mErrorDialogIsActive = true;
      QString msg = context + '\n' + KIO::buildErrorString( errorCode, errorMsg );
      QString caption = i18n("Error");

      if ( jobsKilled || errorCode == KIO::ERR_COULD_NOT_LOGIN ) {
        if ( !errors.isEmpty() )
            KMessageBox::detailedError( kapp->activeWindow(), msg, errors.join("\n").prepend("<qt>"), caption );
        else
            KMessageBox::error( kapp->activeWindow(), msg, caption );
      }
      else // i.e. we have a chance to continue, ask the user about it
      {
        if ( errors.count() >= 3 ) { // there is no detailedWarningContinueCancel... (#86517)
          msg = QString( "<qt>") + context + errors[1] + '\n' + errors[2];
          caption = errors[0];
        }
        int ret = KMessageBox::warningContinueCancel( kapp->activeWindow(), msg, caption );
        if ( ret == KMessageBox::Cancel ) {
          jobsKilled = true;
          killAllJobs( false );
        }
      }
      mErrorDialogIsActive = false;
    } else if ( errorCode != KIO::ERR_USER_CANCELED )
      kdDebug(5006) << "suppressing error:" << errorMsg << endl;

    if ( job && !jobsKilled )
      removeJob( job );
    return !jobsKilled; // jobsKilled==false -> continue==true
  }

  //-----------------------------------------------------------------------------
  void ImapAccountBase::cancelMailCheck()
  {
    QMap<KIO::Job*, jobData>::Iterator it = mapJobData.begin();
    while ( it != mapJobData.end() ) {
      kdDebug(5006) << "cancelMailCheck: job is cancellable: " << (*it).cancellable << endl;
      if ( (*it).cancellable ) {
        it.key()->kill();
        QMap<KIO::Job*, jobData>::Iterator rmit = it;
        ++it;
        mapJobData.remove( rmit );
        // We killed a job -> this kills the slave
        mSlave = 0;
      } else
        ++it;
    }

    for( QPtrListIterator<FolderJob> it( mJobList ); it.current(); ++it ) {
      if ( it.current()->isCancellable() ) {
        FolderJob* job = it.current();
        job->setPassiveDestructor( true );
        mJobList.remove( job );
        delete job;
      } else
        ++it;
    }
  }


  //-----------------------------------------------------------------------------
  QString ImapAccountBase::jobData::htmlURL() const
  {
    KURL u(  url );
    return u.htmlURL();
  }

  //-----------------------------------------------------------------------------
  void ImapAccountBase::processNewMailSingleFolder(KMFolder* folder)
  {
    mFoldersQueuedForChecking.append(folder);
    if ( checkingMail() )
    {
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

    QValueList<QGuardedPtr<KMFolder> > mSaveList = mMailCheckFolders;
    mMailCheckFolders = mFoldersQueuedForChecking;
    kmkernel->acctMgr()->singleCheckMail(this, true);
    mMailCheckFolders = mSaveList;
    mFoldersQueuedForChecking.clear();
  }

  //-----------------------------------------------------------------------------
  bool ImapAccountBase::checkingMail( KMFolder *folder )
  {
    if (checkingMail() && mFoldersQueuedForChecking.contains(folder))
      return true;
    return false;
  }

  //-----------------------------------------------------------------------------
  void ImapAccountBase::handleBodyStructure( QDataStream & stream, KMMessage * msg,
                                             const AttachmentStrategy *as )
  {
    mBodyPartList.clear();
    mCurrentMsg = msg;
    // make the parts and fill the mBodyPartList
    constructParts( stream, 1, 0, 0, msg->asDwMessage() );
    if ( mBodyPartList.count() == 1 ) // we directly set the body later
      msg->deleteBodyParts();

    if ( !as )
    {
      kdWarning(5006) << "ImapAccountBase::handleBodyStructure - found no attachment strategy!" << endl;
      return;
    }

    // download parts according to attachmentstrategy
    BodyVisitor *visitor = BodyVisitorFactory::getVisitor( as );
    visitor->visit( mBodyPartList );
    QPtrList<KMMessagePart> parts = visitor->partsToLoad();
    QPtrListIterator<KMMessagePart> it( parts );
    KMMessagePart *part;
    while ( (part = it.current()) != 0 )
    {
      ++it;
      kdDebug(5006) << "ImapAccountBase::handleBodyStructure - load " << part->partSpecifier()
        << " (" << part->originalContentTypeStr() << ")" << endl;
      if ( part->loadHeaders() )
      {
        kdDebug(5006) << "load HEADER" << endl;
        FolderJob *job = msg->parent()->createJob(
            msg, FolderJob::tGetMessage, 0, part->partSpecifier()+".MIME" );
        job->start();
      }
      if ( part->loadPart() )
      {
        kdDebug(5006) << "load Part" << endl;
        FolderJob *job = msg->parent()->createJob(
            msg, FolderJob::tGetMessage, 0, part->partSpecifier() );
        job->start();
      }
    }
    delete visitor;
  }

  //-----------------------------------------------------------------------------
  void ImapAccountBase::constructParts( QDataStream & stream, int count, KMMessagePart* parentKMPart,
                                        DwBodyPart * parent, const DwMessage * dwmsg )
  {
    int children;
    for (int i = 0; i < count; i++)
    {
      stream >> children;
      KMMessagePart* part = new KMMessagePart( stream );
      part->setParent( parentKMPart );
      mBodyPartList.append( part );
      kdDebug(5006) << "ImapAccountBase::constructParts - created id " << part->partSpecifier()
        << " of type " << part->originalContentTypeStr() << endl;
      DwBodyPart *dwpart = mCurrentMsg->createDWBodyPart( part );
      dwpart->Parse(); // also creates an encapsulated DwMessage if necessary

//      kdDebug(5006) << "constructed dwpart " << dwpart << ",dwmsg " << dwmsg << ",parent " << parent
//       << ",dwparts msg " << dwpart->Body().Message() << endl;

      if ( parent )
      {
        // add to parent body
        parent->Body().AddBodyPart( dwpart );
      } else if ( part->partSpecifier() != "0" &&
                  !part->partSpecifier().endsWith(".HEADER") )
      {
        // add to message
        dwmsg->Body().AddBodyPart( dwpart );
      } else
        dwpart = 0;

      if ( !parentKMPart )
        parentKMPart = part;

      if (children > 0)
      {
        DwBodyPart* newparent = dwpart;
        const DwMessage* newmsg = dwmsg;
        if ( part->originalContentTypeStr() == "MESSAGE/RFC822" &&
             dwpart->Body().Message() )
        {
          // set the encapsulated message as new parent message
          newparent = 0;
          newmsg = dwpart->Body().Message();
        }
        KMMessagePart* newParentKMPart = part;
        if ( part->partSpecifier().endsWith(".HEADER") ) // we don't want headers as parent
          newParentKMPart = parentKMPart;

        constructParts( stream, children, newParentKMPart, newparent, newmsg );
      }
    }
  }

  //-----------------------------------------------------------------------------
  void ImapAccountBase::setImapStatus( KMFolder* folder, const QString& path, const QCString& flags )
  {
     // set the status on the server, the uids are integrated in the path
     kdDebug(5006) << "setImapStatus path=" << path << " to: " << flags << endl;
     KURL url = getUrl();
     url.setPath(path);

     QByteArray packedArgs;
     QDataStream stream( packedArgs, IO_WriteOnly);

     stream << (int) 'S' << url << flags;

     if ( makeConnection() != ImapAccountBase::Connected )
       return; // can't happen with dimap

     KIO::SimpleJob *job = KIO::special(url, packedArgs, FALSE);
     KIO::Scheduler::assignJobToSlave(slave(), job);
     ImapAccountBase::jobData jd( url.url(), folder );
     jd.path = path;
     insertJob(job, jd);
     connect(job, SIGNAL(result(KIO::Job *)),
           SLOT(slotSetStatusResult(KIO::Job *)));
  }
  //-----------------------------------------------------------------------------
  void ImapAccountBase::slotSetStatusResult(KIO::Job * job)
  {
     ImapAccountBase::JobIterator it = findJob(job);
     if ( it == jobsEnd() ) return;
     int errorCode = job->error();
     if (errorCode && errorCode != KIO::ERR_CANNOT_OPEN_FOR_WRITING)
     {
       bool cont = handleJobError( job, i18n( "Error while uploading status of messages to server: " ) + '\n' );
       emit imapStatusChanged( (*it).parent, (*it).path, cont );
     }
     else
     {
       emit imapStatusChanged( (*it).parent, (*it).path, true );
       removeJob(it);
     }
  }

  //-----------------------------------------------------------------------------
  void ImapAccountBase::setFolder(KMFolder* folder, bool addAccount)
  {
    if (folder)
    {
      folder->setSystemLabel(name());
      folder->setId(id());
    }
    NetworkAccount::setFolder(folder, addAccount);
  }

  //-----------------------------------------------------------------------------
  void ImapAccountBase::removeJob( JobIterator& it )
  {
    if( (*it).progressItem ) {
      (*it).progressItem->setComplete();
      (*it).progressItem = 0;
    }
    mapJobData.remove( it );
  }

  //-----------------------------------------------------------------------------
  void KMail::ImapAccountBase::removeJob( KIO::Job* job )
  {
    mapJobData.remove( job );
  }

  //-----------------------------------------------------------------------------
  KPIM::ProgressItem* ImapAccountBase::listDirProgressItem()
  {
    if ( !mListDirProgressItem )
    {
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

  unsigned int ImapAccountBase::folderCount() const
  {
    if ( !rootFolder() || !rootFolder()->folder() || !rootFolder()->folder()->child() )
      return 0;
    return kmkernel->imapFolderMgr()->folderCount( rootFolder()->folder()->child() );
  }

} // namespace KMail

#include "imapaccountbase.moc"
