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
#include "kmbroadcaststatus.h"
#include "kmmainwin.h"
#include "kmfolderimap.h"
#include "kmmainwidget.h"
#include "kmmainwin.h"
#include "kmmsgpart.h"
#include "bodyvisitor.h"
using KMail::BodyVisitor;
#include "imapjob.h"
using KMail::ImapJob;
#include "protocols.h"

#include <kdebug.h>
#include <kconfig.h>
#include <klocale.h>
#include <kmessagebox.h>
using KIO::MetaData;
#include <kio/passdlg.h>
using KIO::PasswordDialog;
#include <kio/scheduler.h>
#include <mimelib/bodypart.h>
#include <mimelib/body.h>
#include <mimelib/headers.h>
#include <mimelib/message.h>
//using KIO::Scheduler; // use FQN below

#include <qregexp.h>
#include "acljobs.h"
#include "kmfoldercachedimap.h"

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
      mCountRemainChecks( 0 ),
      mAutoExpunge( true ),
      mHiddenFolders( false ),
      mOnlySubscribedFolders( false ),
      mLoadOnDemand( true ),
      mProgressEnabled( false ),
      mIdle( true ),
      mErrorDialogIsActive( false ),
      mPasswordDialogIsActive( false ),
      mACLSupport( true ),
      mCreateInbox( false )
  {
    mPort = imapDefaultPort;
    mBodyPartList.setAutoDelete(true);
    KIO::Scheduler::connect(SIGNAL(slaveError(KIO::Slave *, int, const QString &)),
                            this, SLOT(slotSchedulerSlaveError(KIO::Slave *, int, const QString &)));
    KIO::Scheduler::connect(SIGNAL(slaveConnected(KIO::Slave *)),
                            this, SLOT(slotSchedulerSlaveConnected(KIO::Slave *)));
    connect(KMBroadcastStatus::instance(), SIGNAL(signalAbortRequested()),
          this, SLOT(slotAbortRequested()));
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
  }

  void ImapAccountBase::writeConfig( KConfig/*Base*/ & config ) /*const*/ {
    NetworkAccount::writeConfig( config );

    config.writeEntry( "prefix", prefix() );
    config.writeEntry( "auto-expunge", autoExpunge() );
    config.writeEntry( "hidden-folders", hiddenFolders() );
    config.writeEntry( "subscribed-folders", onlySubscribedFolders() );
    config.writeEntry( "loadondemand", loadOnDemand() );
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
    if ( mSlave ) return Connected;

    if ( mPasswordDialogIsActive ) return Connecting;
    if( mAskAgain || passwd().isEmpty() || login().isEmpty() ) {
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
        checkDone(false, 0);
        mPasswordDialogIsActive = false;
        return Error;
      }
      mPasswordDialogIsActive = false;
      // The user has been given the chance to change login and
      // password, so copy both from the dialog:
      setPasswd( pass, store );
      setLogin( log );
      mAskAgain = false; // ### taken from kmacctexppop
    }

    mSlave = KIO::Scheduler::getConnectedSlave( getUrl(), slaveConfig() );
    if ( !mSlave ) {
      KMessageBox::error(0, i18n("Could not start process for %1.")
			 .arg( getUrl().protocol() ) );
      return Error;
    }

    return Connecting;
  }

  // Deprecated method for error handling. Please port to handleJobError.
  void ImapAccountBase::slotSlaveError(KIO::Slave *aSlave, int errorCode,
                                       const QString &errorMsg )
  {
    if (aSlave != mSlave) return;
    handleJobError( errorCode, errorMsg, 0, QString::null, true );
  }

  void ImapAccountBase::postProcessNewMail( KMFolder * folder ) {

    disconnect( folder->storage(), SIGNAL(numUnreadMsgsChanged(KMFolder*)),
                this, SLOT(postProcessNewMail(KMFolder*)) );

    mCountRemainChecks--;

    // count the unread messages
    mCountUnread += folder->countUnread();
    if (mCountRemainChecks == 0)
    {
      // all checks are done
      KMBroadcastStatus::instance()->setStatusMsgTransmissionCompleted(
          name(), mCountUnread );
      if (mCountUnread > 0 && mCountUnread > mCountLastUnread) {
        checkDone(true, mCountUnread);
        mCountLastUnread = mCountUnread;
      } else {
        checkDone(false, 0);
      }
      setCheckingMail(false);
      mCountUnread = 0;
    }
  }

  //-----------------------------------------------------------------------------
  void ImapAccountBase::displayProgress()
  {
    if (mProgressEnabled == mapJobData.isEmpty())
    {
      mProgressEnabled = !mapJobData.isEmpty();
      KMBroadcastStatus::instance()->setStatusProgressEnable( "I" + mName,
          mProgressEnabled );
    }
    mIdle = FALSE;
    if (mapJobData.isEmpty())
      mIdleTimer.start(15000);
    else
      mIdleTimer.stop();
    int total = 0, done = 0;
    for (QMap<KIO::Job*, jobData>::Iterator it = mapJobData.begin();
        it != mapJobData.end(); ++it)
    {
      total += (*it).total;
      done += (*it).done;
    }
    if (total == 0)
    {
      mTotal = 0;
      return;
    }
    if (total > mTotal) mTotal = total;
    done += mTotal - total;
    KMBroadcastStatus::instance()->setStatusProgressPercent( "I" + mName,
        100*done / mTotal );
  }

  //-----------------------------------------------------------------------------
  void ImapAccountBase::listDirectory(QString path, ListType subscription,
      bool secondStep, KMFolder* parent, bool reset, bool complete)
  {
    if (makeConnection() == Error) // ## doesn't handle Connecting
      return;
    // create jobData
    jobData jd;
    jd.total = 1; jd.done = 0;
    // reset for a new listing
    if (reset)
      mHasInbox = false;
    // this is needed if you have a prefix
    // as the INBOX is located in your root ("/") and needs a special listing
    jd.inboxOnly = !secondStep && prefix() != "/"
      && path == prefix() && !mHasInbox;
    jd.onlySubscribed = (subscription != List);
    if (parent) jd.parent = parent;
    if (!secondStep) mCreateInbox = FALSE;
    // make the URL
    QString type = "LIST";
    if (subscription == ListSubscribed)
      type = "LSUB";
    else if (subscription == ListSubscribedNoCheck)
      type = "LSUBNOCHECK";
    KURL url = getUrl();
    url.setPath(((jd.inboxOnly) ? QString("/") : path)
        + ";TYPE=" + type
        + ((complete) ? ";SECTION=COMPLETE" : QString::null));
    mSubfolderNames.clear();
    mSubfolderPaths.clear();
    mSubfolderMimeTypes.clear();
    // and go
    KIO::SimpleJob *job = KIO::listDir(url, FALSE);
    KIO::Scheduler::assignJobToSlave(mSlave, job);
    insertJob(job, jd);
    connect(job, SIGNAL(result(KIO::Job *)),
        this, SLOT(slotListResult(KIO::Job *)));
    connect(job, SIGNAL(entries(KIO::Job *, const KIO::UDSEntryList &)),
        this, SLOT(slotListEntries(KIO::Job *, const KIO::UDSEntryList &)));
  }

  //-----------------------------------------------------------------------------
  void ImapAccountBase::slotListEntries(KIO::Job * job, const KIO::UDSEntryList & uds)
  {
    JobIterator it = findJob( job );
    if ( it == jobsEnd() ) return;
    QString name;
    KURL url;
    QString mimeType;
    for (KIO::UDSEntryList::ConstIterator udsIt = uds.begin();
        udsIt != uds.end(); udsIt++)
    {
      mimeType = QString::null;
      for (KIO::UDSEntry::ConstIterator eIt = (*udsIt).begin();
          eIt != (*udsIt).end(); eIt++)
      {
        // get the needed information
        if ((*eIt).m_uds == KIO::UDS_NAME)
          name = (*eIt).m_str;
        else if ((*eIt).m_uds == KIO::UDS_URL)
          url = KURL((*eIt).m_str, 106); // utf-8
        else if ((*eIt).m_uds == KIO::UDS_MIME_TYPE)
          mimeType = (*eIt).m_str;
      }
      if ((mimeType == "inode/directory" || mimeType == "message/digest"
            || mimeType == "message/directory")
          && name != ".." && (hiddenFolders() || name.at(0) != '.')
          && (!(*it).inboxOnly || name.upper() == "INBOX"))
      {
        if (((*it).inboxOnly ||
              url.path() == "/INBOX/") && name.upper() == "INBOX" &&
            !mHasInbox)
        {
          // our INBOX
          mCreateInbox = TRUE;
        }

        // Some servers send _lots_ of duplicates
        // This check is too slow for huge lists
        if (mSubfolderPaths.count() > 100 ||
            mSubfolderPaths.findIndex(url.path()) == -1)
        {
          mSubfolderNames.append(name);
          mSubfolderPaths.append(url.path());
          mSubfolderMimeTypes.append(mimeType);
        }
      }
    }
  }

  //-----------------------------------------------------------------------------
  void ImapAccountBase::slotListResult(KIO::Job * job)
  {
    JobIterator it = findJob( job );
    if ( it == jobsEnd() ) return;
    if (job->error())
    {
      // PENDING(dfaure) handleJobError
      slotSlaveError( mSlave, job->error(), job->errorText() );
    }
    else
    {
      // transport the information, include the jobData
      emit receivedFolders(mSubfolderNames, mSubfolderPaths,
          mSubfolderMimeTypes, *it);
      removeJob(it);
    }
    mSubfolderNames.clear();
    mSubfolderPaths.clear();
    mSubfolderMimeTypes.clear();
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
    if (job->error())
    {
      // PENDING(dfaure) handleJobError
      slotSlaveError( mSlave, job->error(),
          job->errorText() );
    } else {
      emit subscriptionChanged(
          static_cast<KIO::SimpleJob*>(job)->url().path(), (*it).onlySubscribed );
    }
    if (mSlave) removeJob(job);
  }

  //-----------------------------------------------------------------------------
  // TODO imapPath can be removed once parent can be a KMFolderImapBase or whatever
  void ImapAccountBase::getUserRights( KMFolder* parent, const QString& imapPath )
  {
    KURL url = getUrl();
    url.setPath(imapPath);

    ACLJobs::GetUserRightsJob* job = ACLJobs::getUserRights( mSlave, url );

    jobData jd( url.url(), parent );
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
  // Do not remove imapPath, FolderDiaACLTab needs to call this with parent==0.
  void ImapAccountBase::getACL( KMFolder* parent, const QString& imapPath )
  {
    KURL url = getUrl();
    url.setPath(imapPath);

    ACLJobs::GetACLJob* job = ACLJobs::getACL( mSlave, url );
    jobData jd( url.url(), parent );
    insertJob(job, jd);

    connect(job, SIGNAL(result(KIO::Job *)),
            SLOT(slotGetACLResult(KIO::Job *)));
  }

  void ImapAccountBase::slotGetACLResult( KIO::Job* _job )
  {
    ACLJobs::GetACLJob* job = static_cast<ACLJobs::GetACLJob *>( _job );
    JobIterator it = findJob( job );
    if ( it == jobsEnd() ) return;

    KMFolder* folder = (*it).parent; // can be 0
    emit receivedACL( folder, job, job->entries() );
    if (mSlave) removeJob(job);
  }

  void ImapAccountBase::slotIdleTimeout()
  {
    if ( mIdle ) {
      if ( mSlave )
        KIO::Scheduler::disconnectSlave(mSlave);
      mSlave = 0;
      mIdleTimer.stop();
    } else {
      if ( mSlave ) {
        QByteArray packedArgs;
        QDataStream stream( packedArgs, IO_WriteOnly );

        stream << ( int ) 'N';

        KIO::SimpleJob *job = KIO::special( getUrl(), packedArgs, false );
        KIO::Scheduler::assignJobToSlave(mSlave, job);
        connect( job, SIGNAL(result( KIO::Job * ) ),
          this, SLOT( slotSimpleResult( KIO::Job * ) ) );
      }else {
        mIdleTimer.stop();
      }
    }
  }

  void ImapAccountBase::slotAbortRequested()
  {
    killAllJobs();
  }


  //-----------------------------------------------------------------------------
  void ImapAccountBase::slotSchedulerSlaveError(KIO::Slave *aSlave, int errorCode,
      const QString &errorMsg)
  {
      if (aSlave != mSlave) return;
      // was: slotSlaveError( aSlave, errorCode, errorMsg );
      handleJobError( errorCode, errorMsg, 0, QString::null, true );
      emit connectionResult( errorCode );
  }

  //-----------------------------------------------------------------------------
  void ImapAccountBase::slotSchedulerSlaveConnected(KIO::Slave *aSlave)
  {
      if (aSlave != mSlave) return;
      emit connectionResult( 0 ); // success
  }

  //-----------------------------------------------------------------------------
#if 0 // KMAcctImap and KMAcctCachedImap have their own reimplementation, so this one isn't useful
      // KMAcctCachedImap has an improved version (with support for continue/cancel)
      // Someone should port KMAcctImap to it, and then it can be moved here.
  void ImapAccountBase::slotSlaveError(KIO::Slave *aSlave, int errorCode,
      const QString &errorMsg)
  {
    if (aSlave != mSlave) return;
    if (errorCode == KIO::ERR_SLAVE_DIED) slaveDied();
    if (errorCode == KIO::ERR_COULD_NOT_LOGIN && !mStorePasswd) mAskAgain = TRUE;
    killAllJobs();
    // check if we still display an error
    if ( !mErrorDialogIsActive )
    {
      mErrorDialogIsActive = true;
      KMessageBox::messageBox(kmkernel->mainWin(), KMessageBox::Error,
            KIO::buildErrorString(errorCode, errorMsg),
            i18n("Error"));
      mErrorDialogIsActive = false;
    } else
      kdDebug(5006) << "suppressing error:" << errorMsg << endl;
  }
#endif

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
    if (checkingMail())
    {
      disconnect (this, SIGNAL(finishedCheck(bool)),
          this, SLOT(slotCheckQueuedFolders()));
      connect (this, SIGNAL(finishedCheck(bool)),
          this, SLOT(slotCheckQueuedFolders()));
    } else {
      slotCheckQueuedFolders();
    }
  }

  //-----------------------------------------------------------------------------
  void ImapAccountBase::slotCheckQueuedFolders()
  {
    disconnect (this, SIGNAL(finishedCheck(bool)),
          this, SLOT(slotCheckQueuedFolders()));

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
       bool cont = handleJobError( errorCode, job->errorText(), job, i18n( "Error while uploading status of messages to server: " ) + '\n' );
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

} // namespace KMail

#include "imapaccountbase.moc"
