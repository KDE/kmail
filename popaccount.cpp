/*
    This file is part of KMail, the KDE mail client.
    Copyright (c) 2000 Don Sanders <sanders@kde.org>

    Based on popaccount by:
      Stefan Taferner <taferner@kde.org>
      Markus Wuebben <markus.wuebben@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "popaccount.h"

#include "broadcaststatus.h"
using KPIM::BroadcastStatus;
#include "progressmanager.h"
#include "kmfoldermgr.h"
#include "kmfiltermgr.h"
#include "kmpopfiltercnfrmdlg.h"
#include "protocols.h"
#include "kmglobal.h"
#include "util.h"
#include "accountmanager.h"

#include <kdebug.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmainwindow.h>
#include <kio/scheduler.h>
#include <kio/passworddialog.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kio/jobuidelegate.h>
using KIO::MetaData;

static const unsigned short int pop3DefaultPort = 110;

namespace KMail {
//-----------------------------------------------------------------------------
PopAccount::PopAccount(AccountManager* aOwner, const QString& aAccountName, uint id)
  : NetworkAccount(aOwner, aAccountName, id),
    mPopFilterConfirmationDialog( 0 ),
    mHeaderIndex( 0 )
{
  init();
  job = 0;
  mSlave = 0;
  mPort = defaultPort();
  stage = Idle;
  indexOfCurrentMsg = -1;
  curMsgStrm = 0;
  processingDelay = 2*100;
  mProcessing = false;
  dataCounter = 0;

  connect(&processMsgsTimer,SIGNAL(timeout()),SLOT(slotProcessPendingMsgs()));
  KIO::Scheduler::connect(
    SIGNAL(slaveError(KIO::Slave *, int, const QString &)),
    this, SLOT(slotSlaveError(KIO::Slave *, int, const QString &)));

  mHeaderDeleteUids.clear();
  mHeaderDownUids.clear();
  mHeaderLaterUids.clear();
}


//-----------------------------------------------------------------------------
PopAccount::~PopAccount()
{
  if (job) {
    job->kill();
    mMsgsPendingDownload.clear();
    processRemainingQueuedMessages();
    saveUidList();
  }
}

//-----------------------------------------------------------------------------
QString PopAccount::protocol() const {
  return useSSL() ? POP_SSL_PROTOCOL : POP_PROTOCOL;
}

unsigned short int PopAccount::defaultPort() const {
  return pop3DefaultPort;
}

//-----------------------------------------------------------------------------
void PopAccount::init(void)
{
  NetworkAccount::init();

  mUsePipelining = false;
  mLeaveOnServer = false;
  mLeaveOnServerDays = -1;
  mLeaveOnServerCount = -1;
  mLeaveOnServerSize = -1;
  mFilterOnServer = false;
  //tz todo
  mFilterOnServerCheckSize = 50000;
}

//-----------------------------------------------------------------------------
void PopAccount::pseudoAssign( const KMAccount * a ) {
  slotAbortRequested();
  NetworkAccount::pseudoAssign( a );

  const PopAccount * p = dynamic_cast<const PopAccount*>( a );
  if ( !p ) return;

  setUsePipelining( p->usePipelining() );
  setLeaveOnServer( p->leaveOnServer() );
  setLeaveOnServerDays( p->leaveOnServerDays() );
  setLeaveOnServerCount( p->leaveOnServerCount() );
  setLeaveOnServerSize( p->leaveOnServerSize() );
  setFilterOnServer( p->filterOnServer() );
  setFilterOnServerCheckSize( p->filterOnServerCheckSize() );
}

//-----------------------------------------------------------------------------
void PopAccount::processNewMail(bool _interactive)
{
  if (stage == Idle) {

    if ( (mAskAgain || passwd().isEmpty() || mLogin.isEmpty()) &&
      mAuth != "GSSAPI" ) {
      QString passwd = NetworkAccount::passwd();
      bool b = storePasswd();
      if (KIO::PasswordDialog::getNameAndPassword(mLogin, passwd, &b,
        i18n("You need to supply a username and a password to access this "
        "mailbox."), false, QString(), mName, i18n("Account:"))
        != QDialog::Accepted)
      {
        checkDone( false, CheckAborted );
        return;
      } else {
        setPasswd( passwd, b );
        if ( b ) {
          kmkernel->acctMgr()->writeConfig( true );
        }
        mAskAgain = false;
      }
    }

    QString seenUidList = KStandardDirs::locateLocal( "data", "kmail/" + mLogin + ':' + '@' +
                                       mHost + ':' + QString("%1").arg(mPort) );
    KConfig config( seenUidList );
    KConfigGroup group( &config, "<default>" );
    QStringList uidsOfSeenMsgs = group.readEntry( "seenUidList" , QStringList() );
    mUidsOfSeenMsgsDict.clear();
    mUidsOfSeenMsgsDict.reserve( KMail::nextPrime( ( uidsOfSeenMsgs.count() * 11 ) / 10 ) );
    for ( int i = 0; i < uidsOfSeenMsgs.count(); ++i ) {
      // we use mUidsOfSeenMsgsDict to just provide fast random access to the
      // keys, so we can store the index that corresponds to the index of
      // mTimeOfSeenMsgsVector for use in PopAccount::slotData()
      mUidsOfSeenMsgsDict.insert( uidsOfSeenMsgs[i].toLatin1(), i );
    }
    QList<int> timeOfSeenMsgs = group.readEntry( "seenUidTimeList",QList<int>() );
    // If the counts differ then the config file has presumably been tampered
    // with and so to avoid possible unwanted message deletion we'll treat
    // them all as newly seen by clearing the seen times vector
    if ( timeOfSeenMsgs.count() == mUidsOfSeenMsgsDict.count() )
      mTimeOfSeenMsgsVector = timeOfSeenMsgs.toVector();
    else
      mTimeOfSeenMsgsVector.clear();
    QStringList downloadLater = group.readEntry( "downloadLater" , QStringList() );
    for ( int i = 0; i < downloadLater.count(); ++i ) {
      mHeaderLaterUids.insert( downloadLater[i].toLatin1() );
    }
    mUidsOfNextSeenMsgsDict.clear();
    mTimeOfNextSeenMsgsMap.clear();
    mSizeOfNextSeenMsgsDict.clear();

    interactive = _interactive;
    mUidlFinished = false;
    startJob();
  }
  else {
    checkDone( false, CheckIgnored );
    return;
  }
}


//-----------------------------------------------------------------------------
void PopAccount::readConfig(KConfigGroup& config)
{
  NetworkAccount::readConfig(config);

  mUsePipelining = config.readEntry("pipelining", false );
  mLeaveOnServer = config.readEntry("leave-on-server", false );
  mLeaveOnServerDays = config.readEntry("leave-on-server-days", -1 );
  mLeaveOnServerCount = config.readEntry("leave-on-server-count", -1 );
  mLeaveOnServerSize = config.readEntry("leave-on-server-size", -1 );
  mFilterOnServer = config.readEntry("filter-on-server", false );
  mFilterOnServerCheckSize = config.readEntry("filter-os-check-size", (uint) 50000 );
}


//-----------------------------------------------------------------------------
void PopAccount::writeConfig(KConfigGroup& config)
{
  NetworkAccount::writeConfig(config);

  config.writeEntry("pipelining", mUsePipelining);
  config.writeEntry("leave-on-server", mLeaveOnServer);
  config.writeEntry("leave-on-server-days", mLeaveOnServerDays);
  config.writeEntry("leave-on-server-count", mLeaveOnServerCount);
  config.writeEntry("leave-on-server-size", mLeaveOnServerSize);
  config.writeEntry("filter-on-server", mFilterOnServer);
  config.writeEntry("filter-os-check-size", mFilterOnServerCheckSize);
}


//-----------------------------------------------------------------------------
void PopAccount::setUsePipelining(bool b)
{
  mUsePipelining = b;
}

//-----------------------------------------------------------------------------
void PopAccount::setLeaveOnServer(bool b)
{
  mLeaveOnServer = b;
}

//-----------------------------------------------------------------------------
void PopAccount::setLeaveOnServerDays(int days)
{
  mLeaveOnServerDays = days;
}

//-----------------------------------------------------------------------------
void PopAccount::setLeaveOnServerCount(int count)
{
  mLeaveOnServerCount = count;
}

//-----------------------------------------------------------------------------
void PopAccount::setLeaveOnServerSize(int size)
{
  mLeaveOnServerSize = size;
}

//---------------------------------------------------------------------------
void PopAccount::setFilterOnServer(bool b)
{
  mFilterOnServer = b;
}

//---------------------------------------------------------------------------
void PopAccount::setFilterOnServerCheckSize(unsigned int aSize)
{
  mFilterOnServerCheckSize = aSize;
}

//-----------------------------------------------------------------------------
void PopAccount::connectJob() {
  KIO::Scheduler::assignJobToSlave(mSlave, job);
  connect(job, SIGNAL( data( KIO::Job*, const QByteArray &)),
         SLOT( slotData( KIO::Job*, const QByteArray &)));
  connect(job, SIGNAL( result( KJob * ) ),
         SLOT( slotResult( KJob * ) ) );
  connect(job, SIGNAL(infoMessage( KJob*, const QString &, const QString & )),
         SLOT( slotMsgRetrieved(KJob*, const QString &, const QString &)));
}


//-----------------------------------------------------------------------------
void PopAccount::slotCancel()
{
  mMsgsPendingDownload.clear();
  processRemainingQueuedMessages();
  saveUidList();
  slotJobFinished();

  // Close the pop filter confirmation dialog. Otherwise, KMail crashes because
  // slotJobFinished(), which creates that dialog, will try to continue downloading
  // when the user closes the dialog.
  if ( mPopFilterConfirmationDialog )
    mPopFilterConfirmationDialog->reject();
}


//-----------------------------------------------------------------------------
void PopAccount::slotProcessPendingMsgs()
{
  if (mProcessing) // not reentrant
    return;
  mProcessing = true;

  while ( !msgsAwaitingProcessing.isEmpty() ) {
    // note we can actually end up processing events in processNewMsg
    // this happens when send receipts is turned on
    // hence the check for re-entry at the start of this method.
    // -sanders Update processNewMsg should no longer process events

    KMMessage *msg = msgsAwaitingProcessing.dequeue();
    const bool addedOk = processNewMsg( msg ); //added ok? Error displayed if not.

    if ( !addedOk ) {
      mMsgsPendingDownload.clear();
      break;
    }

    const QByteArray curId = msgIdsAwaitingProcessing.dequeue();
    const QByteArray curUid = msgUidsAwaitingProcessing.dequeue();
    idsOfMsgsToDelete.insert( curId );
    mUidsOfNextSeenMsgsDict.insert( curUid, 1 );
    mTimeOfNextSeenMsgsMap.insert( curUid, time(0) );
  }

  msgsAwaitingProcessing.clear();
  msgIdsAwaitingProcessing.clear();
  msgUidsAwaitingProcessing.clear();
  mProcessing = false;
}


//-----------------------------------------------------------------------------
void PopAccount::slotAbortRequested()
{
  if (stage == Idle) return;
  disconnect( mMailCheckProgressItem, SIGNAL( progressItemCanceled( KPIM::ProgressItem* ) ),
           this, SLOT( slotAbortRequested() ) );
  stage = Quit;
  if (job) job->kill();
  job = 0;
  mSlave = 0;
  slotCancel();
}


//-----------------------------------------------------------------------------
void PopAccount::startJob()
{
  // Run the precommand
  if (!runPrecommand(precommand()))
    {
      KMessageBox::sorry(0,
                         i18n("Could not execute precommand: %1", precommand()),
                         i18n("KMail Error Message"));
      checkDone( false, CheckError );
      return;
    }
  // end precommand code

  KUrl url = getUrl();

  if ( !url.isValid() ) {
    KMessageBox::error(0, i18n("Source URL is malformed"),
                          i18n("Kioslave Error Message") );
    return;
  }

  mMsgsPendingDownload.clear();
  idsOfMsgs.clear();
  mUidForIdMap.clear();
  idsOfMsgsToDelete.clear();
  //delete any headers if there are some this have to be done because of check again
  qDeleteAll( mHeadersOnServer );
  mHeadersOnServer.clear();
  headers = false;
  indexOfCurrentMsg = -1;

  Q_ASSERT( !mMailCheckProgressItem );
  mMailCheckProgressItem = KPIM::ProgressManager::createProgressItem(
    "MailCheck" + mName,
    mName,
    i18n("Preparing transmission from \"%1\"...", mName),
    true, // can be canceled
    useSSL() || useTLS() );
  connect( mMailCheckProgressItem, SIGNAL( progressItemCanceled( KPIM::ProgressItem* ) ),
           this, SLOT( slotAbortRequested() ) );

  numBytes = 0;
  numBytesRead = 0;
  stage = List;
  mSlave = KIO::Scheduler::getConnectedSlave( url, slaveConfig() );
  if (!mSlave)
  {
    slotSlaveError(0, KIO::ERR_CANNOT_LAUNCH_PROCESS, url.protocol());
    return;
  }
  url.setPath( "/index" );
  job = KIO::get( url, KIO::NoReload, KIO::HideProgressInfo );
  connectJob();
}

MetaData PopAccount::slaveConfig() const {
  MetaData m = NetworkAccount::slaveConfig();

  m.insert("progress", "off");
  m.insert("pipelining", (mUsePipelining) ? "on" : "off");
  if (mAuth == "PLAIN" || mAuth == "LOGIN" || mAuth == "CRAM-MD5" ||
      mAuth == "DIGEST-MD5" || mAuth == "NTLM" || mAuth == "GSSAPI") {
    m.insert("auth", "SASL");
    m.insert("sasl", mAuth);
  } else if ( mAuth == "*" )
    m.insert("auth", "USER");
  else
    m.insert("auth", mAuth);

  return m;
}

//-----------------------------------------------------------------------------
// one message is finished
// add data to a KMMessage
void PopAccount::slotMsgRetrieved(KJob*, const QString & infoMsg, const QString &)
{
  if (infoMsg != "message complete") return;
  KMMessage *msg = new KMMessage;
  msg->setComplete(true);
  // Make sure to use LF as line ending to make the processing easier
  // when piping through external programs
  uint newSize = Util::crlf2lf( curMsgData.data(), curMsgData.size() );
  curMsgData.resize( newSize );
  msg->fromString( curMsgData , true );
  if ( stage == Head ) {
    KMPopHeaders *header = mHeadersOnServer[ mHeaderIndex ];
    int size = mMsgsPendingDownload[ header->id() ];
    kDebug(5006) <<"Size of Message:" << size;
    msg->setMsgLength( size );
    header->setHeader( msg );
    ++mHeaderIndex;
    slotGetNextHdr();
  } else {
    //kDebug(5006) << kfuncinfo <<"stage == Retr";
    //kDebug(5006) <<"curMsgData.size() =" << curMsgData.size();
    msg->setMsgLength( curMsgData.size() );
    msgsAwaitingProcessing.enqueue( msg );
    msgIdsAwaitingProcessing.enqueue( idsOfMsgs[indexOfCurrentMsg] );
    msgUidsAwaitingProcessing.enqueue( mUidForIdMap[ idsOfMsgs[indexOfCurrentMsg] ] );
    slotGetNextMsg();
  }
}


//-----------------------------------------------------------------------------
// finit state machine to cycle trow the stages
void PopAccount::slotJobFinished() {
  QStringList emptyList;
  if (stage == List) {
    kDebug(5006) <<"stage == List";
    // set the initial size of mUidsOfNextSeenMsgsDict to the number of
    // messages on the server + 10%
    mUidsOfNextSeenMsgsDict.reserve( KMail::nextPrime( ( idsOfMsgs.count() * 11 ) / 10 ) );
    KUrl url = getUrl();
    url.setPath( "/uidl" );
    job = KIO::get( url, KIO::NoReload, KIO::HideProgressInfo );
    connectJob();
    stage = Uidl;
  }
  else if (stage == Uidl) {
    kDebug(5006) <<"stage == Uidl";
    mUidlFinished = true;

    if ( mLeaveOnServer && mUidForIdMap.isEmpty() &&
        mUidsOfNextSeenMsgsDict.isEmpty() && !idsOfMsgs.isEmpty() ) {
      KMessageBox::sorry(0, i18n("Your POP3 server (Account: %1) does not support "
      "the UIDL command: this command is required to determine, in a reliable way, "
      "which of the mails on the server KMail has already seen before;\n"
      "the feature to leave the mails on the server will therefore not "
      "work properly.", NetworkAccount::name()) );
      // An attempt to work around buggy pop servers, these seem to be popular.
      mUidsOfNextSeenMsgsDict = mUidsOfSeenMsgsDict;
    }

    //check if filter on server
    if (mFilterOnServer == true) {
      for ( QMap<QByteArray, int>::const_iterator hids = mMsgsPendingDownload.begin();
            hids != mMsgsPendingDownload.end(); ++hids ) {
          kDebug(5006) <<"Length:" << hids.value();
          //check for mails bigger mFilterOnServerCheckSize
          if ( (unsigned int)hids.value() >= mFilterOnServerCheckSize ) {
            kDebug(5006) <<"bigger than" << mFilterOnServerCheckSize;
            const QByteArray uid = mUidForIdMap[ hids.key() ];
            KMPopHeaders *header = new KMPopHeaders( hids.key(), uid, Later );
            //set Action if already known
            if ( mHeaderDeleteUids.contains( uid ) ) {
              header->setAction(Delete);
            }
            else if ( mHeaderDownUids.contains( uid ) ) {
              header->setAction(Down);
            }
            else if ( mHeaderLaterUids.contains( uid ) ) {
              header->setAction(Later);
            }
            mHeadersOnServer.append( header );
          }
      }
      // delete the uids so that you don't get them twice in the list
      mHeaderDeleteUids.clear();
      mHeaderDownUids.clear();
      mHeaderLaterUids.clear();
    }
    // kDebug(5006) <<"Num of Msgs to Filter:" << mHeadersOnServer.count();
    // if there are mails which should be checkedc download the headers
    if ( ( mHeadersOnServer.count() > 0 ) && ( mFilterOnServer == true ) ) {
      KUrl url = getUrl();
      QByteArray headerIds = mHeadersOnServer[0]->id();
      for ( int i = 1; i < mHeadersOnServer.count(); ++i ) {
        headerIds += ',';
        headerIds += mHeadersOnServer[i]->id();
      }
      mHeaderIndex = 0;
      url.setPath( "/headers/" + headerIds );
      job = KIO::get( url, KIO::NoReload, KIO::HideProgressInfo );
      connectJob();
      slotGetNextHdr();
      stage = Head;
    }
    else {
      stage = Retr;
      numMsgs = mMsgsPendingDownload.count();
      numBytesToRead = 0;
      idsOfMsgs.clear();
      QByteArray ids;
      if ( numMsgs > 0 ) {
        for ( QMap<QByteArray, int>::const_iterator it = mMsgsPendingDownload.begin();
              it != mMsgsPendingDownload.end(); ++it ) {
          numBytesToRead += it.value();
          ids += it.key() + ',';
          idsOfMsgs.append( it.key() );
        }
        ids.chop( 1 ); // cut off the trailing ','
      }
      KUrl url = getUrl();
      url.setPath( "/download/" + ids );
      job = KIO::get( url, KIO::NoReload, KIO::HideProgressInfo );
      connectJob();
      slotGetNextMsg();
      processMsgsTimer.start(processingDelay);
    }
  }
  else if (stage == Head) {
    kDebug(5006) <<"stage == Head";

    // All headers have been downloaded, check which mail you want to get
    // data is in list mHeadersOnServer

    // check if headers apply to a filter
    // if set the action of the filter
    KMPopFilterAction action;
    bool dlgPopup = false;
    for ( int i = 0; i < mHeadersOnServer.count(); ++i ) {
      KMPopHeaders *header = mHeadersOnServer[i];
      action = (KMPopFilterAction)kmkernel->popFilterMgr()->process( header->header() );
      //debug todo
      switch ( action ) {
        case NoAction:
          kDebug(5006) <<"PopFilterAction = NoAction";
          break;
        case Later:
          kDebug(5006) <<"PopFilterAction = Later";
          break;
        case Delete:
          kDebug(5006) <<"PopFilterAction = Delete";
          break;
        case Down:
          kDebug(5006) <<"PopFilterAction = Down";
          break;
        default:
          kDebug(5006) <<"PopFilterAction = default oops!";
          break;
      }
      switch ( action ) {
        case NoAction:
          //kDebug(5006) <<"PopFilterAction = NoAction";
          dlgPopup = true;
          break;
        case Later:
          if (kmkernel->popFilterMgr()->showLaterMsgs())
            dlgPopup = true;
          // fall through
        default:
          header->setAction( action );
          header->setRuleMatched( true );
          break;
      }
    }

    // if there are some messages which are not coverd by a filter
    // show the dialog
    headers = true;
    if ( dlgPopup ) {
      mPopFilterConfirmationDialog =
          new KMPopFilterCnfrmDlg( mHeadersOnServer, this->name(),
                                   kmkernel->popFilterMgr()->showLaterMsgs() );
      mPopFilterConfirmationDialog->exec();
    }

    // The only case were the result code of the dialog is QDialog::Rejected
    // should be when the mail check in canceled in slotCancel().
    // This here would likely break if there is still a mail check active
    // and Rejected is the result. But that shouldn't happen.
    if ( mPopFilterConfirmationDialog->result() == QDialog::Accepted ) {

      for ( int i = 0; i < mHeadersOnServer.count(); ++i ) {
        const KMPopHeaders *header = mHeadersOnServer[i];
        if ( header->action() == Delete || header->action() == Later) {
          //remove entries from the lists when the mails should not be downloaded
          //(deleted or downloaded later)
          mMsgsPendingDownload.remove( header->id() );
          if ( header->action() == Delete ) {
            mHeaderDeleteUids.insert( header->uid() );
            mUidsOfNextSeenMsgsDict.insert( header->uid(), 1 );
            idsOfMsgsToDelete.insert( header->id() );
            mTimeOfNextSeenMsgsMap.insert( header->uid(), time(0) );
          }
          else {
            mHeaderLaterUids.insert( header->uid() );
          }
        }
        else if ( header->action() == Down ) {
          mHeaderDownUids.insert( header->uid() );
        }
      }

      qDeleteAll( mHeadersOnServer );
      mHeadersOnServer.clear();
      stage = Retr;
      numMsgs = mMsgsPendingDownload.count();
      numBytesToRead = 0;
      idsOfMsgs.clear();
      QByteArray ids;
      if ( numMsgs > 0 ) {
        for ( QMap<QByteArray, int>::const_iterator it = mMsgsPendingDownload.begin();
              it != mMsgsPendingDownload.end(); ++it ) {
          numBytesToRead += it.value();
          ids += it.key() + ',';
          idsOfMsgs.append( it.key() );
        }
        ids.chop( 1 ); // cut off the trailing ','
      }
      KUrl url = getUrl();
      url.setPath( "/download/" + ids );
      job = KIO::get( url, KIO::NoReload, KIO::HideProgressInfo );
      connectJob();
      slotGetNextMsg();
      processMsgsTimer.start(processingDelay);
    }
    delete mPopFilterConfirmationDialog;
    mPopFilterConfirmationDialog = 0;
  }
  else if (stage == Retr) {
    mMailCheckProgressItem->setProgress( 100 );
    processRemainingQueuedMessages();

    mHeaderDeleteUids.clear();
    mHeaderDownUids.clear();
    mHeaderLaterUids.clear();

    kmkernel->folderMgr()->syncAllFolders();

    KUrl url = getUrl();
    QList< QPair<time_t, QByteArray> > idsToSave;
    // Check if we want to keep any messages
    if ( mLeaveOnServer && !idsOfMsgsToDelete.isEmpty() ) {
      // Keep all messages on server
      if ( mLeaveOnServerDays == -1 && mLeaveOnServerCount <= 0 &&
           mLeaveOnServerSize <= 0 )
        idsOfMsgsToDelete.clear();
      // Delete old messages
      else if ( mLeaveOnServerDays > 0 && !mTimeOfNextSeenMsgsMap.isEmpty() ) {
        time_t timeLimit = time(0) - (86400 * mLeaveOnServerDays);
        kDebug(5006) <<"timeLimit is" << timeLimit;
        for ( QSet<QByteArray>::const_iterator it = idsOfMsgsToDelete.begin();
              it != idsOfMsgsToDelete.end(); ++it ) {
          time_t msgTime = mTimeOfNextSeenMsgsMap[ mUidForIdMap[*it] ];
          kDebug(5006) <<"id:" << *it <<" msgTime:" << msgTime;
          if ( msgTime >= timeLimit || msgTime == 0 ) {
            kDebug(5006) <<"Saving msg id" << *it;
            QPair<time_t, QByteArray> pair( msgTime, *it );
            idsToSave.append( pair );
          }
        }
      }
      // sort the idsToSave list so that in the following we remove the oldest messages
      qSort( idsToSave );
      // Delete more old messages if there are more than mLeaveOnServerCount
      if ( mLeaveOnServerCount > 0 ) {
        int numToDelete = idsToSave.count() - mLeaveOnServerCount;
        kDebug(5006) <<"numToDelete is" << numToDelete;
        if ( numToDelete > 0 && numToDelete < idsToSave.count() ) {
          // get rid of the first numToDelete messages
          #ifdef DEBUG
          for ( int i = 0; i < numToDelete; ++i )
            kDebug(5006) <<"deleting msg id" << idsToSave[i].second;
          #endif
          idsToSave = idsToSave.mid( numToDelete );
        }
        else if ( numToDelete >= idsToSave.count() )
          idsToSave.clear();
      }
      // Delete more old messages until we're under mLeaveOnServerSize MBs
      if ( mLeaveOnServerSize > 0 ) {
        const long limitInBytes = mLeaveOnServerSize * ( 1024 * 1024 );
        long sizeOnServer = 0;
        int firstMsgToKeep = idsToSave.count() - 1;
        for ( ; firstMsgToKeep >= 0 && sizeOnServer <= limitInBytes; --firstMsgToKeep ) {
          sizeOnServer +=
            mSizeOfNextSeenMsgsDict[ mUidForIdMap[ idsToSave[firstMsgToKeep].second ] ];
        }
        if ( sizeOnServer > limitInBytes )
          firstMsgToKeep++;
        #ifdef DEBUG
        for ( int i = 0; i < firstMsgToKeep; ++i )
          kDebug(5006) <<"deleting msg id" << idsToSave[i].second;
        #endif
        if ( firstMsgToKeep > 0 )
          idsToSave = idsToSave.mid( firstMsgToKeep );
      }
      // Save msgs from deletion
      kDebug(5006) <<"Going to save" << idsToSave.count();
      for ( int i = 0; i < idsToSave.count(); ++i ) {
        kDebug(5006) <<"keeping msg id" << idsToSave[i].second;
        idsOfMsgsToDelete.remove( idsToSave[i].second );
      }
    }
    // If there are messages to delete then delete them
    if ( !idsOfMsgsToDelete.isEmpty() ) {
      stage = Dele;
      mMailCheckProgressItem->setStatus(
        i18np( "Fetched 1 message from %2. Deleting messages from server...",
              "Fetched %1 messages from %2. Deleting messages from server...",
              numMsgs ,
          mHost ) );
      QSet<QByteArray>::const_iterator it = idsOfMsgsToDelete.begin();
      QByteArray ids = *it;
      ++it;
      for ( ; it != idsOfMsgsToDelete.end(); ++it ) {
        ids += ',';
        ids += *it;
      }
      url.setPath( "/remove/" + ids );
      kDebug(5006) <<"url:" << url.prettyUrl();
    } else {
      stage = Quit;
      mMailCheckProgressItem->setStatus(
        i18np( "Fetched 1 message from %2. Terminating transmission...",
              "Fetched %1 messages from %2. Terminating transmission...",
              numMsgs ,
          mHost ) );
      url.setPath( "/commit" );
      kDebug(5006) <<"url:" << url.prettyUrl();
    }
    job = KIO::get( url, KIO::NoReload, KIO::HideProgressInfo );
    connectJob();
  }
  else if (stage == Dele) {
    kDebug(5006) <<"stage == Dele";
    // remove the uids of all messages which have been deleted
    for ( QSet<QByteArray>::const_iterator it = idsOfMsgsToDelete.begin();
          it != idsOfMsgsToDelete.end(); ++it ) {
      mUidsOfNextSeenMsgsDict.remove( mUidForIdMap[*it] );
    }
    idsOfMsgsToDelete.clear();
    mMailCheckProgressItem->setStatus(
      i18np( "Fetched 1 message from %2. Terminating transmission...",
            "Fetched %1 messages from %2. Terminating transmission...",
            numMsgs ,
        mHost ) );
    KUrl url = getUrl();
    url.setPath( "/commit" );
    job = KIO::get( url, KIO::NoReload, KIO::HideProgressInfo );
    stage = Quit;
    connectJob();
  }
  else if (stage == Quit) {
    kDebug(5006) <<"stage == Quit";
    saveUidList();
    job = 0;
    if (mSlave) KIO::Scheduler::disconnectSlave(mSlave);
    mSlave = 0;
    stage = Idle;
    if( mMailCheckProgressItem ) { // do this only once...
      bool canceled = !kmkernel || kmkernel->mailCheckAborted() || mMailCheckProgressItem->canceled();
      int numMessages = canceled ? indexOfCurrentMsg : idsOfMsgs.count();
      BroadcastStatus::instance()->setStatusMsgTransmissionCompleted(
        this->name(), numMessages, numBytes, numBytesRead, numBytesToRead, mLeaveOnServer, mMailCheckProgressItem );
      mMailCheckProgressItem->setComplete();
      mMailCheckProgressItem = 0;
      checkDone( ( numMessages > 0 ), canceled ? CheckAborted : CheckOK );
    }
  }
}


//-----------------------------------------------------------------------------
void PopAccount::processRemainingQueuedMessages()
{
  kDebug(5006) ;
  slotProcessPendingMsgs(); // Force processing of any messages still in the queue
  processMsgsTimer.stop();

  stage = Quit;
  if ( kmkernel && kmkernel->folderMgr() ) {
    kmkernel->folderMgr()->syncAllFolders();
  }
}


//-----------------------------------------------------------------------------
void PopAccount::saveUidList()
{
  kDebug(5006) ;
  // Don't update the seen uid list unless we successfully got
  // a new list from the server
  if (!mUidlFinished) return;

  QStringList uidsOfNextSeenMsgs;
  QList<int> seenUidTimeList;
  for ( QHash<QByteArray,int>::const_iterator it = mUidsOfNextSeenMsgsDict.begin();
       it != mUidsOfNextSeenMsgsDict.end(); ++it ) {
    uidsOfNextSeenMsgs.append( it.key() );
    seenUidTimeList.append( mTimeOfNextSeenMsgsMap[ it.key() ] );
  }
  QString seenUidList = KStandardDirs::locateLocal( "data", "kmail/" + mLogin + ':' + '@' +
                                      mHost + ':' + QString::number( mPort ) );
  KConfig config( seenUidList );
  KConfigGroup group( &config, "<default>" );
  group.writeEntry( "seenUidList", uidsOfNextSeenMsgs );
  group.writeEntry( "seenUidTimeList", seenUidTimeList );
  QByteArray laterList;
  laterList.reserve( mHeaderLaterUids.count() * 5 ); // what's the average size of a uid?
  foreach( const QByteArray& uid, mHeaderLaterUids.values() ) {
      if ( !laterList.isEmpty() )
          laterList += ',';
      laterList.append( uid );
  }
  group.writeEntry( "downloadLater", laterList.constData() );
  config.sync();
}


//-----------------------------------------------------------------------------
void PopAccount::slotGetNextMsg()
{
  curMsgData.resize(0);
  numMsgBytesRead = 0;
  curMsgLen = 0;
  delete curMsgStrm;
  curMsgStrm = 0;

  if ( !mMsgsPendingDownload.isEmpty() ) {
    // get the next message
    QMap<QByteArray, int>::iterator next = mMsgsPendingDownload.begin();
    curMsgStrm = new QDataStream( &curMsgData, QIODevice::WriteOnly );
    curMsgLen = next.value();
    ++indexOfCurrentMsg;
    kDebug(5006) << QString("Length of message about to get %1").arg( curMsgLen );
    mMsgsPendingDownload.erase( next );
  }
}


//-----------------------------------------------------------------------------
void PopAccount::slotData( KIO::Job* job, const QByteArray &data)
{
  if (data.size() == 0) {
    kDebug(5006) <<"Data: <End>";
    if ((stage == Retr) && (numMsgBytesRead < curMsgLen))
      numBytesRead += curMsgLen - numMsgBytesRead;
    else if (stage == Head){
      kDebug(5006) <<"Head: <End>";
    }
    return;
  }

  int oldNumMsgBytesRead = numMsgBytesRead;
  if (stage == Retr) {
    headers = false;
    curMsgStrm->writeRawData( data.data(), data.size() );
    numMsgBytesRead += data.size();
    if (numMsgBytesRead > curMsgLen)
      numMsgBytesRead = curMsgLen;
    numBytesRead += numMsgBytesRead - oldNumMsgBytesRead;
    dataCounter++;
    if (dataCounter % 5 == 0)
    {
      QString msg;
      if (numBytes != numBytesToRead && mLeaveOnServer)
      {
        msg = ki18n("Fetching message %1 of %2 (%3 of %4 KB) for %5@%6 "
                    "(%7 KB remain on the server).")
           .subs( indexOfCurrentMsg+1 ).subs( numMsgs )
           .subs( numBytesRead/1024 ).subs( numBytesToRead/1024 )
           .subs( mLogin ).subs( mHost ).subs( numBytes/1024 )
           .toString();
      }
      else
      {
        msg = ki18n("Fetching message %1 of %2 (%3 of %4 KB) for %5@%6.")
           .subs( indexOfCurrentMsg+1 ).subs( numMsgs ).subs( numBytesRead/1024 )
           .subs( numBytesToRead/1024 ).subs( mLogin ).subs( mHost )
           .toString();
      }
      mMailCheckProgressItem->setStatus( msg );
      mMailCheckProgressItem->setProgress(
        (numBytesToRead <= 100) ? 50  // We never know what the server tells us
        // This way of dividing is required for > 21MB of mail
        : (numBytesRead / (numBytesToRead / 100)) );
    }
    return;
  }

  if (stage == Head) {
    curMsgStrm->writeRawData( data.data(), data.size() );
    return;
  }

  // otherwise stage is List Or Uidl
  QByteArray qdata = data.simplified(); // Workaround for Maillennium POP3/UNIBOX
  const int spc = qdata.indexOf( ' ' );

  //Get rid of the null-terminating character if that exists.
  //Because mUidsOfSeenMsgsDict doesn't have those either, comparing the
  //values would otherwise fail.
  if ( qdata.at( qdata.size() - 1 ) == 0 )
    qdata.chop( 1 );

  if (spc > 0) {
    if (stage == List) {
      QByteArray length = qdata.mid(spc+1);
      const int spaceInLengthPos = length.indexOf(' ');
      if ( spaceInLengthPos != -1 )
        length.truncate( spaceInLengthPos );
      const int len = length.toInt();
      numBytes += len;
      QByteArray id = qdata.left(spc);
      idsOfMsgs.append( id );
      mMsgsPendingDownload.insert( id, len );
    }
    else { // stage == Uidl
      const QByteArray id = qdata.left(spc);
      const QByteArray uid = qdata.mid(spc + 1);
      mSizeOfNextSeenMsgsDict.insert( uid, mMsgsPendingDownload[id] );
      if ( mUidsOfSeenMsgsDict.contains( uid ) ) {
        if ( mMsgsPendingDownload.contains( id ) ) {
          mMsgsPendingDownload.remove( id );
        }
        else
          kDebug(5006) <<"PopAccount::slotData synchronization failure.";
        idsOfMsgsToDelete.insert( id );
        mUidsOfNextSeenMsgsDict.insert( uid, 1 );
        if ( mTimeOfSeenMsgsVector.empty() ) {
          mTimeOfNextSeenMsgsMap.insert( uid, time(0) );
        }
        else {
          mTimeOfNextSeenMsgsMap.insert( uid,
            mTimeOfSeenMsgsVector[ mUidsOfSeenMsgsDict[uid] ] );
        }
      }
      mUidForIdMap.insert( id, uid );
    }
  }
  else {
    stage = Idle;
    if (job) job->kill();
    job = 0;
    mSlave = 0;
    KMessageBox::error(0, i18n( "Unable to complete LIST operation." ),
                          i18n("Invalid Response From Server"));
    return;
  }
}


//-----------------------------------------------------------------------------
void PopAccount::slotResult( KJob* )
{
  if (!job) return;
  if ( job->error() )
  {
    if (interactive) {
      if (headers) { // nothing to be done for headers
        idsOfMsgs.clear();
      }
      if (stage == Head && job->error() == KIO::ERR_COULD_NOT_READ)
      {
        KMessageBox::error(0, i18n("Your server does not support the "
          "TOP command. Therefore it is not possible to fetch the headers "
          "of large emails first, before downloading them."));
        slotCancel();
        return;
      }
      // force the dialog to be shown next time the account is checked
      if (!mStorePasswd) mPasswd = "";
	  job->ui()->setWindow( 0 );
	  job->ui()->showErrorMessage();
    }
    slotCancel();
  }
  else
    slotJobFinished();
}


//-----------------------------------------------------------------------------
void PopAccount::slotSlaveError(KIO::Slave *aSlave, int error,
  const QString &errorMsg)
{
  if (aSlave != mSlave) return;
  if (error == KIO::ERR_SLAVE_DIED) mSlave = 0;

  // explicitly disconnect the slave if the connection went down
  if ( error == KIO::ERR_CONNECTION_BROKEN && mSlave ) {
    KIO::Scheduler::disconnectSlave( mSlave );
    mSlave = 0;
  }

  if (interactive && kmkernel) {
    KMessageBox::error(kmkernel->mainWin(), KIO::buildErrorString(error, errorMsg));
  }


  stage = Quit;
  if (error == KIO::ERR_COULD_NOT_LOGIN && !mStorePasswd)
    mAskAgain = true;
  /* We need a timer, otherwise slotSlaveError of the next account is also
     executed, if it reuses the slave, because the slave member variable
     is changed too early */
  QTimer::singleShot(0, this, SLOT(slotCancel()));
}

//-----------------------------------------------------------------------------
void PopAccount::slotGetNextHdr(){
  kDebug(5006) <<"slotGetNextHeader";

  curMsgData.resize(0);
  delete curMsgStrm;
  curMsgStrm = 0;

  curMsgStrm = new QDataStream( &curMsgData, QIODevice::WriteOnly );
}

void PopAccount::killAllJobs( bool ) {
  // must reimpl., but we don't use it yet
}

} // namespace KMail
#include "popaccount.moc"
