// KMAcctExpPop.cpp
// Authors: Don Sanders, (based on kmacctpop by)
//          Stefan Taferner and Markus Wuebben

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmacctexppop.h"

#include "broadcaststatus.h"
using KPIM::BroadcastStatus;
#include "progressmanager.h"
#include "kmfoldermgr.h"
#include "kmfiltermgr.h"
#include "kmpopfiltercnfrmdlg.h"
#include "kmkernel.h"
#include "protocols.h"
#include "kmdict.h"

#include <kdebug.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmainwindow.h>
#include <kio/scheduler.h>
#include <kio/passdlg.h>
#include <kconfig.h>
using KIO::MetaData;

static const unsigned short int pop3DefaultPort = 110;

//-----------------------------------------------------------------------------
KMAcctExpPop::KMAcctExpPop(KMAcctMgr* aOwner, const QString& aAccountName, uint id)
  : NetworkAccount(aOwner, aAccountName, id),
    headerIt(headersOnServer)
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
  mUidsOfSeenMsgsDict.setAutoDelete( false );
  mUidsOfNextSeenMsgsDict.setAutoDelete( false );

  headersOnServer.setAutoDelete(true);
  connect(&processMsgsTimer,SIGNAL(timeout()),SLOT(slotProcessPendingMsgs()));
  KIO::Scheduler::connect(
    SIGNAL(slaveError(KIO::Slave *, int, const QString &)),
    this, SLOT(slotSlaveError(KIO::Slave *, int, const QString &)));

  mHeaderDeleteUids.clear();
  mHeaderDownUids.clear();
  mHeaderLaterUids.clear();
}


//-----------------------------------------------------------------------------
KMAcctExpPop::~KMAcctExpPop()
{
  if (job) {
    job->kill();
    mMsgsPendingDownload.clear();
    processRemainingQueuedMessages();
    saveUidList();
  }
}


//-----------------------------------------------------------------------------
QString KMAcctExpPop::type(void) const
{
  return "pop";
}

QString KMAcctExpPop::protocol() const {
  return useSSL() ? POP_SSL_PROTOCOL : POP_PROTOCOL;
}

unsigned short int KMAcctExpPop::defaultPort() const {
  return pop3DefaultPort;
}

//-----------------------------------------------------------------------------
void KMAcctExpPop::init(void)
{
  NetworkAccount::init();

  mUsePipelining = FALSE;
  mLeaveOnServer = FALSE;
  mFilterOnServer = FALSE;
  //tz todo
  mFilterOnServerCheckSize = 50000;
}

//-----------------------------------------------------------------------------
void KMAcctExpPop::pseudoAssign( const KMAccount * a ) {
  slotAbortRequested();
  NetworkAccount::pseudoAssign( a );

  const KMAcctExpPop * p = dynamic_cast<const KMAcctExpPop*>( a );
  if ( !p ) return;

  setUsePipelining( p->usePipelining() );
  setLeaveOnServer( p->leaveOnServer() );
  setFilterOnServer( p->filterOnServer() );
  setFilterOnServerCheckSize( p->filterOnServerCheckSize() );
}

//-----------------------------------------------------------------------------
void KMAcctExpPop::processNewMail(bool _interactive)
{
  if (stage == Idle) {

    if ( (mAskAgain || passwd().isEmpty() || mLogin.isEmpty()) &&
      mAuth != "GSSAPI" ) {
      QString passwd = NetworkAccount::passwd();
      bool b = storePasswd();
      if (KIO::PasswordDialog::getNameAndPassword(mLogin, passwd, &b,
        i18n("You need to supply a username and a password to access this "
        "mailbox."), FALSE, QString::null, mName, i18n("Account:"))
        != QDialog::Accepted)
      {
        checkDone( false, CheckAborted );
        return;
      } else {
        setPasswd( passwd, b );
        mAskAgain = FALSE;
      }
    }

    QString seenUidList = locateLocal( "data", "kmail/" + mLogin + ":" + "@" +
                                       mHost + ":" + QString("%1").arg(mPort) );
    KConfig config( seenUidList );
    QStringList uidsOfSeenMsgs = config.readListEntry( "seenUidList" );
    mUidsOfSeenMsgsDict.clear();
    mUidsOfSeenMsgsDict.resize( KMail::nextPrime( ( uidsOfSeenMsgs.count() * 11 ) / 10 ) );
    for ( QStringList::ConstIterator it = uidsOfSeenMsgs.begin();
          it != uidsOfSeenMsgs.end(); ++it ) {
      // we use mUidsOfSeenMsgsDict to provide fast random access to the keys,
      // so we simply set the values to (const int *)1
      mUidsOfSeenMsgsDict.insert( *it, (const int *)1 );
    }
    QStringList downloadLater = config.readListEntry( "downloadLater" );
    for ( QStringList::Iterator it = downloadLater.begin(); it != downloadLater.end(); ++it ) {
        mHeaderLaterUids.insert( *it, true );
    }
    mUidsOfNextSeenMsgsDict.clear();

    interactive = _interactive;
    mUidlFinished = FALSE;
    startJob();
  }
  else {
    checkDone( false, CheckIgnored );
    return;
  }
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::readConfig(KConfig& config)
{
  NetworkAccount::readConfig(config);

  mUsePipelining = config.readNumEntry("pipelining", FALSE);
  mLeaveOnServer = config.readNumEntry("leave-on-server", FALSE);
  mFilterOnServer = config.readNumEntry("filter-on-server", FALSE);
  mFilterOnServerCheckSize = config.readUnsignedNumEntry("filter-os-check-size", 50000);
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::writeConfig(KConfig& config)
{
  NetworkAccount::writeConfig(config);

  config.writeEntry("pipelining", mUsePipelining);
  config.writeEntry("leave-on-server", mLeaveOnServer);
  config.writeEntry("filter-on-server", mFilterOnServer);
  config.writeEntry("filter-os-check-size", mFilterOnServerCheckSize);
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::setUsePipelining(bool b)
{
  mUsePipelining = b;
}

//-----------------------------------------------------------------------------
void KMAcctExpPop::setLeaveOnServer(bool b)
{
  mLeaveOnServer = b;
}


//---------------------------------------------------------------------------
void KMAcctExpPop::setFilterOnServer(bool b)
{
  mFilterOnServer = b;
}

//---------------------------------------------------------------------------
void KMAcctExpPop::setFilterOnServerCheckSize(unsigned int aSize)
{
  mFilterOnServerCheckSize = aSize;
}

//-----------------------------------------------------------------------------
void KMAcctExpPop::connectJob() {
  KIO::Scheduler::assignJobToSlave(mSlave, job);
  if (stage != Dele)
  connect(job, SIGNAL( data( KIO::Job*, const QByteArray &)),
	  SLOT( slotData( KIO::Job*, const QByteArray &)));
  connect(job, SIGNAL( result( KIO::Job * ) ),
	  SLOT( slotResult( KIO::Job * ) ) );
  connect(job, SIGNAL(infoMessage( KIO::Job*, const QString & )),
          SLOT( slotMsgRetrieved(KIO::Job*, const QString &)));
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::slotCancel()
{
  mMsgsPendingDownload.clear();
  processRemainingQueuedMessages();
  saveUidList();
  slotJobFinished();
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::slotProcessPendingMsgs()
{
  if (mProcessing) // not reentrant
    return;
  mProcessing = true;

  bool addedOk;
  QValueList<KMMessage*>::Iterator cur = msgsAwaitingProcessing.begin();
  QStringList::Iterator curId = msgIdsAwaitingProcessing.begin();
  QStringList::Iterator curUid = msgUidsAwaitingProcessing.begin();

  while (cur != msgsAwaitingProcessing.end()) {
    // note we can actually end up processing events in processNewMsg
    // this happens when send receipts is turned on
    // hence the check for re-entry at the start of this method.
    // -sanders Update processNewMsg should no longer process events

    addedOk = processNewMsg(*cur); //added ok? Error displayed if not.

    if (!addedOk) {
      mMsgsPendingDownload.clear();
      msgIdsAwaitingProcessing.clear();
      msgUidsAwaitingProcessing.clear();
      break;
    }
    else {
      idsOfMsgsToDelete.append( *curId );
      mUidsOfNextSeenMsgsDict.insert( *curUid, (const int *)1 );
    }
    ++cur;
    ++curId;
    ++curUid;
  }

  msgsAwaitingProcessing.clear();
  msgIdsAwaitingProcessing.clear();
  msgUidsAwaitingProcessing.clear();
  mProcessing = false;
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::slotAbortRequested()
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
void KMAcctExpPop::startJob()
{
  // Run the precommand
  if (!runPrecommand(precommand()))
    {
      KMessageBox::sorry(0,
                         i18n("Could not execute precommand: %1").arg(precommand()),
                         i18n("KMail Error Message"));
      checkDone( false, CheckError );
      return;
    }
  // end precommand code

  KURL url = getUrl();

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
  headersOnServer.clear();
  headers = false;
  indexOfCurrentMsg = -1;

  Q_ASSERT( !mMailCheckProgressItem );
  mMailCheckProgressItem = KPIM::ProgressManager::createProgressItem(
    "MailCheck" + mName,
    mName,
    i18n("Preparing transmission from \"%1\"...").arg(mName),
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
  url.setPath(QString("/index"));
  job = KIO::get( url, false, false );
  connectJob();
}

MetaData KMAcctExpPop::slaveConfig() const {
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
void KMAcctExpPop::slotMsgRetrieved(KIO::Job*, const QString & infoMsg)
{
  if (infoMsg != "message complete") return;
  KMMessage *msg = new KMMessage;
  msg->setComplete(true);
  // Make sure to use LF as line ending to make the processing easier
  // when piping through external programs
  uint newSize = KMFolder::crlf2lf( curMsgData.data(), curMsgData.size() );
  curMsgData.resize( newSize );
  msg->fromByteArray( curMsgData , true );
  if (stage == Head)
  {
    int size = mMsgsPendingDownload[ headerIt.current()->id() ];
    kdDebug(5006) << "Size of Message: " << size << endl;
    msg->setMsgLength( size );
    headerIt.current()->setHeader(msg);
    ++headerIt;
    slotGetNextHdr();
  } else {
    //kdDebug(5006) << kfuncinfo << "stage == Retr" << endl;
    //kdDebug(5006) << "curMsgData.size() = " << curMsgData.size() << endl;
    msg->setMsgLength( curMsgData.size() );
    msgsAwaitingProcessing.append(msg);
    msgIdsAwaitingProcessing.append(idsOfMsgs[indexOfCurrentMsg]);
    msgUidsAwaitingProcessing.append( mUidForIdMap[idsOfMsgs[indexOfCurrentMsg]] );
    slotGetNextMsg();
  }
}


//-----------------------------------------------------------------------------
// finit state machine to cycle trow the stages
void KMAcctExpPop::slotJobFinished() {
  QStringList emptyList;
  if (stage == List) {
    kdDebug(5006) << k_funcinfo << "stage == List" << endl;
    // set the initial size of mUidsOfNextSeenMsgsDict to the number of
    // messages on the server + 10%
    mUidsOfNextSeenMsgsDict.resize( KMail::nextPrime( ( idsOfMsgs.count() * 11 ) / 10 ) );
    KURL url = getUrl();
    url.setPath(QString("/uidl"));
    job = KIO::get( url, false, false );
    connectJob();
    stage = Uidl;
  }
  else if (stage == Uidl) {
    kdDebug(5006) << k_funcinfo << "stage == Uidl" << endl;
    mUidlFinished = TRUE;

    if ( mLeaveOnServer && mUidForIdMap.isEmpty() &&
         mUidsOfNextSeenMsgsDict.isEmpty() && !idsOfMsgs.isEmpty() ) {
      KMessageBox::sorry(0, i18n("Your POP3 server does not support the UIDL "
      "command: this command is required to determine, in a reliable way, "
      "which of the mails on the server KMail has already seen before;\n"
      "the feature to leave the mails on the server will therefore not "
      "work properly."));
      // An attempt to work around buggy pop servers, these seem to be popular.
      mUidsOfNextSeenMsgsDict = mUidsOfSeenMsgsDict;
    }

    //check if filter on server
    if (mFilterOnServer == true) {
      QMap<QString, int>::Iterator hids;
      for ( hids = mMsgsPendingDownload.begin();
            hids != mMsgsPendingDownload.end(); hids++ ) {
          kdDebug(5006) << "Length: " << hids.data() << endl;
          //check for mails bigger mFilterOnServerCheckSize
          if ( (unsigned int)hids.data() >= mFilterOnServerCheckSize ) {
            kdDebug(5006) << "bigger than " << mFilterOnServerCheckSize << endl;
              headersOnServer.append(new KMPopHeaders( hids.key(),
                                                       mUidForIdMap[hids.key()],
                                                       Later));//TODO
            //set Action if already known
            if( mHeaderDeleteUids.contains( headersOnServer.current()->uid() ) ) {
              headersOnServer.current()->setAction(Delete);
            }
            else if( mHeaderDownUids.contains( headersOnServer.current()->uid() ) ) {
              headersOnServer.current()->setAction(Down);
            }
            else if( mHeaderLaterUids.contains( headersOnServer.current()->uid() ) ) {
              headersOnServer.current()->setAction(Later);
            }
          }
      }
      // delete the uids so that you don't get them twice in the list
      mHeaderDeleteUids.clear();
      mHeaderDownUids.clear();
      mHeaderLaterUids.clear();
    }
    // kdDebug(5006) << "Num of Msgs to Filter: " << headersOnServer.count() << endl;
    // if there are mails which should be checkedc download the headers
    if ((headersOnServer.count() > 0) && (mFilterOnServer == true)) {
      headerIt.toFirst();
      KURL url = getUrl();
      QString headerIds;
      while (headerIt.current())
      {
        headerIds += headerIt.current()->id();
        if (!headerIt.atLast()) headerIds += ",";
        ++headerIt;
      }
      headerIt.toFirst();
      url.setPath(QString("/headers/") + headerIds);
      job = KIO::get( url, false, false );
      connectJob();
      slotGetNextHdr();
      stage = Head;
    }
    else {
      stage = Retr;
      numMsgs = mMsgsPendingDownload.count();
      numBytesToRead = 0;
      QMap<QString, int>::Iterator len;
      for ( len  = mMsgsPendingDownload.begin();
            len != mMsgsPendingDownload.end(); len++ )
        numBytesToRead += len.data();
      idsOfMsgs = QStringList( mMsgsPendingDownload.keys() );
      KURL url = getUrl();
      url.setPath( "/download/" + idsOfMsgs.join(",") );
      job = KIO::get( url, false, false );
      connectJob();
      slotGetNextMsg();
      processMsgsTimer.start(processingDelay);
    }
  }
  else if (stage == Head) {
    kdDebug(5006) << k_funcinfo << "stage == Head" << endl;

    // All headers have been downloaded, check which mail you want to get
    // data is in list headersOnServer

    // check if headers apply to a filter
    // if set the action of the filter
    KMPopFilterAction action;
    bool dlgPopup = false;
    for (headersOnServer.first(); headersOnServer.current(); headersOnServer.next()) {
      action = (KMPopFilterAction)kmkernel->popFilterMgr()->process(headersOnServer.current()->header());
      //debug todo
      switch ( action ) {
        case NoAction:
          kdDebug(5006) << "PopFilterAction = NoAction" << endl;
          break;
        case Later:
          kdDebug(5006) << "PopFilterAction = Later" << endl;
          break;
        case Delete:
          kdDebug(5006) << "PopFilterAction = Delete" << endl;
          break;
        case Down:
          kdDebug(5006) << "PopFilterAction = Down" << endl;
          break;
        default:
          kdDebug(5006) << "PopFilterAction = default oops!" << endl;
          break;
      }
      switch ( action ) {
        case NoAction:
          //kdDebug(5006) << "PopFilterAction = NoAction" << endl;
          dlgPopup = true;
          break;
        case Later:
          if (kmkernel->popFilterMgr()->showLaterMsgs())
            dlgPopup = true;
        default:
          headersOnServer.current()->setAction(action);
          headersOnServer.current()->setRuleMatched(true);
          break;
      }
    }

    // if there are some messages which are not coverd by a filter
    // show the dialog
    headers = true;
    if (dlgPopup) {
      KMPopFilterCnfrmDlg dlg(&headersOnServer, this->name(), kmkernel->popFilterMgr()->showLaterMsgs());
      dlg.exec();
    }

    for (headersOnServer.first(); headersOnServer.current(); headersOnServer.next()) {
      if (headersOnServer.current()->action() == Delete ||
          headersOnServer.current()->action() == Later) {
        //remove entries from the lists when the mails should not be downloaded
        //(deleted or downloaded later)
        if ( mMsgsPendingDownload.contains( headersOnServer.current()->id() ) ) {
          mMsgsPendingDownload.remove( headersOnServer.current()->id() );
        }
        if (headersOnServer.current()->action() == Delete) {
          mHeaderDeleteUids.insert(headersOnServer.current()->uid(), true);
          mUidsOfNextSeenMsgsDict.insert( headersOnServer.current()->uid(),
                                          (const int *)1 );
          idsOfMsgsToDelete.append(headersOnServer.current()->id());
        }
        else {
          mHeaderLaterUids.insert(headersOnServer.current()->uid(), true);
        }
      }
      else if (headersOnServer.current()->action() == Down) {
        mHeaderDownUids.insert(headersOnServer.current()->uid(), true);
      }
    }

    headersOnServer.clear();
    stage = Retr;
    numMsgs = mMsgsPendingDownload.count();
    numBytesToRead = 0;
    QMap<QString, int>::Iterator len;
    for (len = mMsgsPendingDownload.begin();
         len != mMsgsPendingDownload.end(); len++)
      numBytesToRead += len.data();
    idsOfMsgs = QStringList( mMsgsPendingDownload.keys() );
    KURL url = getUrl();
    url.setPath( "/download/" + idsOfMsgs.join(",") );
    job = KIO::get( url, false, false );
    connectJob();
    slotGetNextMsg();
    processMsgsTimer.start(processingDelay);
  }
  else if (stage == Retr) {
    mMailCheckProgressItem->setProgress( 100 );
    processRemainingQueuedMessages();

    mHeaderDeleteUids.clear();
    mHeaderDownUids.clear();
    mHeaderLaterUids.clear();

    kmkernel->folderMgr()->syncAllFolders();

    KURL url = getUrl();
    if (mLeaveOnServer || idsOfMsgsToDelete.isEmpty()) {
      stage = Quit;
      mMailCheckProgressItem->setStatus(
        i18n( "Fetched 1 message from %1. Terminating transmission...",
              "Fetched %n messages from %1. Terminating transmission...",
              numMsgs )
        .arg( mHost ) );
      url.setPath(QString("/commit"));
      job = KIO::get(url, false, false );
    }
    else {
      stage = Dele;
      mMailCheckProgressItem->setStatus(
        i18n( "Fetched 1 message from %1. Deleting messages from server...",
              "Fetched %n messages from %1. Deleting messages from server...",
              numMsgs )
        .arg( mHost ) );
      url.setPath("/remove/" + idsOfMsgsToDelete.join(","));
      kdDebug(5006) << "url: " << url.prettyURL() << endl;
      job = KIO::get( url, false, false );
    }
    connectJob();
  }
  else if (stage == Dele) {
    kdDebug(5006) << k_funcinfo << "stage == Dele" << endl;
    // remove the uids of all messages which have been deleted
    for ( QStringList::ConstIterator it = idsOfMsgsToDelete.begin();
          it != idsOfMsgsToDelete.end(); ++it ) {
      mUidsOfNextSeenMsgsDict.remove( mUidForIdMap[*it] );
    }
    idsOfMsgsToDelete.clear();
    mMailCheckProgressItem->setStatus(
      i18n( "Fetched 1 message from %1. Terminating transmission...",
            "Fetched %n messages from %1. Terminating transmission...",
            numMsgs )
      .arg( mHost ) );
    KURL url = getUrl();
    url.setPath(QString("/commit"));
    job = KIO::get( url, false, false );
    stage = Quit;
    connectJob();
  }
  else if (stage == Quit) {
    kdDebug(5006) << k_funcinfo << "stage == Quit" << endl;
    saveUidList();
    job = 0;
    if (mSlave) KIO::Scheduler::disconnectSlave(mSlave);
    mSlave = 0;
    stage = Idle;
    if( mMailCheckProgressItem ) { // do this only once...
      bool canceled = kmkernel->mailCheckAborted() || mMailCheckProgressItem->canceled();
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
void KMAcctExpPop::processRemainingQueuedMessages()
{
  kdDebug(5006) << k_funcinfo << endl;
  slotProcessPendingMsgs(); // Force processing of any messages still in the queue
  processMsgsTimer.stop();

  stage = Quit;
  kmkernel->folderMgr()->syncAllFolders();
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::saveUidList()
{
  kdDebug(5006) << k_funcinfo << endl;
  // Don't update the seen uid list unless we successfully got
  // a new list from the server
  if (!mUidlFinished) return;

  QStringList uidsOfNextSeenMsgs;
  QDictIterator<int> it( mUidsOfNextSeenMsgsDict );
  for( ; it.current(); ++it )
    uidsOfNextSeenMsgs.append( it.currentKey() );
  QString seenUidList = locateLocal( "data", "kmail/" + mLogin + ":" + "@" +
				     mHost + ":" + QString("%1").arg(mPort) );
  KConfig config( seenUidList );
  config.writeEntry( "seenUidList", uidsOfNextSeenMsgs );
  config.writeEntry( "downloadLater", QStringList( mHeaderLaterUids.keys() ) );
  config.sync();
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::slotGetNextMsg()
{
  QMap<QString, int>::Iterator next = mMsgsPendingDownload.begin();

  curMsgData.resize(0);
  numMsgBytesRead = 0;
  curMsgLen = 0;
  delete curMsgStrm;
  curMsgStrm = 0;

  if ( next != mMsgsPendingDownload.end() ) {
    // get the next message
    int nextLen = next.data();
    curMsgStrm = new QDataStream( curMsgData, IO_WriteOnly );
    curMsgLen = nextLen;
    ++indexOfCurrentMsg;
    kdDebug(5006) << QString("Length of message about to get %1").arg( nextLen ) << endl;
    mMsgsPendingDownload.remove( next.key() );
  }
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::slotData( KIO::Job* job, const QByteArray &data)
{
  if (data.size() == 0) {
    kdDebug(5006) << "Data: <End>" << endl;
    if ((stage == Retr) && (numMsgBytesRead < curMsgLen))
      numBytesRead += curMsgLen - numMsgBytesRead;
    else if (stage == Head){
      kdDebug(5006) << "Head: <End>" << endl;
    }
    return;
  }

  int oldNumMsgBytesRead = numMsgBytesRead;
  if (stage == Retr) {
    headers = false;
    curMsgStrm->writeRawBytes( data.data(), data.size() );
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
	msg = i18n("Fetching message %1 of %2 (%3 of %4 KB) from %5 "
		   "(%6 KB remain on the server).")
	  .arg(indexOfCurrentMsg+1).arg(numMsgs).arg(numBytesRead/1024)
	  .arg(numBytesToRead/1024).arg(mHost).arg(numBytes/1024);
      }
      else
      {
	msg = i18n("Fetching message %1 of %2 (%3 of %4 KB) from %5.")
	  .arg(indexOfCurrentMsg+1).arg(numMsgs).arg(numBytesRead/1024)
	  .arg(numBytesToRead/1024).arg(mHost);
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
    curMsgStrm->writeRawBytes( data.data(), data.size() );
    return;
  }

  // otherwise stage is List Or Uidl
  QString qdata = data;
  qdata = qdata.simplifyWhiteSpace(); // Workaround for Maillennium POP3/UNIBOX
  int spc = qdata.find( ' ' );
  if (spc > 0) {
    if (stage == List) {
      QString length = qdata.mid(spc+1);
      if (length.find(' ') != -1) length.truncate(length.find(' '));
      int len = length.toInt();
      numBytes += len;
      QString id = qdata.left(spc);
      idsOfMsgs.append( id );
      mMsgsPendingDownload.insert( id, len );
    }
    else { // stage == Uidl
      const QString id = qdata.left(spc);
      const QString uid = qdata.mid(spc + 1);
      if ( mUidsOfSeenMsgsDict.find( uid ) != 0 ) {
        if ( mMsgsPendingDownload.contains( id ) ) {
          mMsgsPendingDownload.remove( id );
        }
        else
          kdDebug(5006) << "KMAcctExpPop::slotData synchronization failure." << endl;
        idsOfMsgsToDelete.append( id );
        mUidsOfNextSeenMsgsDict.insert( uid, (const int *)1 );
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
void KMAcctExpPop::slotResult( KIO::Job* )
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
      job->showErrorDialog();
    }
    slotCancel();
  }
  else
    slotJobFinished();
}


//-----------------------------------------------------------------------------
void KMAcctExpPop::slotSlaveError(KIO::Slave *aSlave, int error,
  const QString &errorMsg)
{
  if (aSlave != mSlave) return;
  if (error == KIO::ERR_SLAVE_DIED) mSlave = 0;

  // explicitely disconnect the slave if the connection went down
  if ( error == KIO::ERR_CONNECTION_BROKEN && mSlave ) {
    KIO::Scheduler::disconnectSlave( mSlave );
    mSlave = 0;
  }

  if (interactive) {
    KMessageBox::error(kmkernel->mainWin(), KIO::buildErrorString(error, errorMsg));
  }


  stage = Quit;
  if (error == KIO::ERR_COULD_NOT_LOGIN && !mStorePasswd)
    mAskAgain = TRUE;
  /* We need a timer, otherwise slotSlaveError of the next account is also
     executed, if it reuses the slave, because the slave member variable
     is changed too early */
  QTimer::singleShot(0, this, SLOT(slotCancel()));
}

//-----------------------------------------------------------------------------
void KMAcctExpPop::slotGetNextHdr(){
  kdDebug(5006) << "slotGetNextHeader" << endl;

  curMsgData.resize(0);
  delete curMsgStrm;
  curMsgStrm = 0;

  curMsgStrm = new QDataStream( curMsgData, IO_WriteOnly );
}

void KMAcctExpPop::killAllJobs( bool ) {
  // must reimpl., but we don't use it yet
}

#include "kmacctexppop.moc"
