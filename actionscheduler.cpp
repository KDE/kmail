/* Action Scheduler
 *
 * Author: Don Sanders <sanders@kde.org>
 * License: GPL
 */

#include "actionscheduler.h"

#include "messageproperty.h"
#include "kmfilter.h"
#include "kmfolderindex.h"
#include "kmfoldermgr.h"
#include "kmmsgdict.h"
#include "kmcommands.h"
#include "kmheaders.h"

#include <qtimer.h>
#include <kconfig.h>
#include <kstandarddirs.h>

using namespace KMail;
typedef QPtrList<KMMsgBase> KMMessageList;

KMFolderMgr* ActionScheduler::tempFolderMgr = 0;
int ActionScheduler::refCount = 0;
int ActionScheduler::count = 0;

ActionScheduler::ActionScheduler(KMFilterMgr::FilterSet set,
				 QPtrList<KMFilter> filters,
				 KMHeaders *headers,
				 KMFolder *srcFolder)
             :mSet( set ), mHeaders( headers )
{
  ++count;
  ++refCount;
  mExecuting = false;
  mExecutingLock = false;
  mFetchExecuting = false;
  mFiltersAreQueued = false;
  mResult = ResultOk;
  mIgnore = false;
  mAutoDestruct = false;
  mAlwaysMatch = false;
  KMFilter *filter;
  finishTimer = new QTimer( this );
  connect( finishTimer, SIGNAL(timeout()), this, SLOT(finish()));
  fetchMessageTimer = new QTimer( this );
  connect( fetchMessageTimer, SIGNAL(timeout()), this, SLOT(fetchMessage()));
  tempCloseFoldersTimer = new QTimer( this );
  connect( tempCloseFoldersTimer, SIGNAL(timeout()), this, SLOT(tempCloseFolders()));
  processMessageTimer = new QTimer( this );
  connect( processMessageTimer, SIGNAL(timeout()), this, SLOT(processMessage()));
  filterMessageTimer = new QTimer( this );
  connect( filterMessageTimer, SIGNAL(timeout()), this, SLOT(filterMessage()));
  
  for (filter = filters.first(); filter; filter = filters.next())
    mFilters.append( *filter );
  mDestFolder = 0;
  if (srcFolder) {
    mDeleteSrcFolder = false;
    setSourceFolder( srcFolder );
  } else {
    QString tmpName;
    tmpName.setNum( count );
    if (!tempFolderMgr)
      tempFolderMgr = new KMFolderMgr(locateLocal("data","kmail/filter"));
    KMFolder *tempFolder = tempFolderMgr->findOrCreate( tmpName );
    tempFolder->expunge();
    mDeleteSrcFolder = true;
    setSourceFolder( tempFolder );
  }
}

ActionScheduler::~ActionScheduler()
{
  tempCloseFolders();
  mSrcFolder->close();
  
  if (mDeleteSrcFolder)
    tempFolderMgr->remove(mSrcFolder);

  --refCount;
  if (refCount == 0) {
    delete tempFolderMgr;
    tempFolderMgr = 0;
  }
}

void ActionScheduler::setAutoDestruct( bool autoDestruct )
{
  mAutoDestruct = autoDestruct;
}

void ActionScheduler::setAlwaysMatch( bool alwaysMatch )
{
  mAlwaysMatch = alwaysMatch;
}

void ActionScheduler::setDefaultDestinationFolder( KMFolder *destFolder )
{
  mDestFolder = destFolder;
}

void ActionScheduler::setSourceFolder( KMFolder *srcFolder )
{
  srcFolder->open();
  if (mSrcFolder) {
    disconnect( mSrcFolder, SIGNAL(msgAdded(KMFolder*, Q_UINT32)),
		this, SLOT(msgAdded(KMFolder*, Q_UINT32)) );
    mSrcFolder->close();
  }
  mSrcFolder = srcFolder;
  int i = 0;
  for (i = 0; i < mSrcFolder->count(); ++i)
    enqueue( mSrcFolder->getMsgBase( i )->getMsgSerNum() );
  if (mSrcFolder)
    connect( mSrcFolder, SIGNAL(msgAdded(KMFolder*, Q_UINT32)),
	     this, SLOT(msgAdded(KMFolder*, Q_UINT32)) );
}

void ActionScheduler::setFilterList( QPtrList<KMFilter> filters )
{
  mFiltersAreQueued = true;
  mQueuedFilters.clear();
  KMFilter *filter;
  for (filter = filters.first(); filter; filter = filters.next())
    mQueuedFilters.append( *filter );
}

int ActionScheduler::tempOpenFolder( KMFolder* aFolder )
{
  assert( aFolder );
  tempCloseFoldersTimer->stop();
  if ( aFolder == mSrcFolder.operator->() )
    return 0;
  
  int rc = aFolder->open();
  if (rc)
    return rc;

  mOpenFolders.append( aFolder );
  return 0;
}

void ActionScheduler::tempCloseFolders()
{
  // close temp opened folders
  QValueListConstIterator<QGuardedPtr<KMFolder> > it;
  for (it = mOpenFolders.begin(); it != mOpenFolders.end(); ++it) {
    KMFolder *folder = *it;
    if (folder)
      folder->close();
  }
  mOpenFolders.clear();
}

void ActionScheduler::execFilters(const QValueList<Q_UINT32> serNums)
{
  QValueListConstIterator<Q_UINT32> it;
  for (it = serNums.begin(); it != serNums.end(); ++it)
    execFilters( *it );
}

void ActionScheduler::execFilters(const QPtrList<KMMsgBase> msgList)
{
  KMMsgBase *msgBase;
  QPtrList<KMMsgBase> list = msgList;
  for (msgBase = list.first(); msgBase; msgBase = list.next())
    execFilters( msgBase->getMsgSerNum() );
}

void ActionScheduler::execFilters(KMMsgBase* msgBase)
{
  execFilters( msgBase->getMsgSerNum() );
}

void ActionScheduler::execFilters(Q_UINT32 serNum)
{
  if (mResult != ResultOk)
    return; // An error has already occurred don't even try to process this msg

  if (MessageProperty::filtering( serNum )) {
    // Not good someone else is already filtering this msg
    mResult = ResultError;
    if (!mExecuting)
      finishTimer->start( 0, true );
  } else {
    // Everything is ok async fetch this message
    mFetchSerNums.append( serNum );
    if (!mFetchExecuting) {
      //Need to (re)start incomplete msg fetching chain
      mFetchExecuting = true;
      fetchMessageTimer->start( 0, true );
    }
  }
}

KMMsgBase *ActionScheduler::messageBase(Q_UINT32 serNum)
{
  int idx = -1;
  KMFolder *folder = 0;
  KMMsgBase *msg = 0;
  kmkernel->msgDict()->getLocation( serNum, &folder, &idx );
  // It's possible that the message has been deleted or moved into a
  // different folder
  if (folder && (idx != -1)) {
    // everything is ok
    msg = folder->getMsgBase( idx );
    tempOpenFolder( folder ); // just in case msg has moved
  } else {
    // the message is gone!
    mResult = ResultError;
    finishTimer->start( 0, true );
  }
  return msg;
}

KMMessage *ActionScheduler::message(Q_UINT32 serNum)
{
  int idx = -1;
  KMFolder *folder = 0;
  KMMessage *msg = 0;
  kmkernel->msgDict()->getLocation( serNum, &folder, &idx );
  // It's possible that the message has been deleted or moved into a
  // different folder
  if (folder && (idx != -1)) {
    // everything is ok
    msg = folder->getMsg( idx );
    tempOpenFolder( folder ); // just in case msg has moved
  } else {
    // the message is gone!
    mResult = ResultError;
    finishTimer->start( 0, true );
  }
  return msg;
}

void ActionScheduler::finish()
{
  if (mResult == ResultCriticalError) {
    // Must handle critical errors immediately
    emit result( mResult );
    return;
  }
      
  if (!mFetchExecuting && !mExecuting) {
    // If an error has occurred and a permanent source folder has
    // been set then move all the messages left in the source folder
    // to the inbox. If no permanent source folder has been set
    // then abandon filtering of queued messages.
    if (!mDeleteSrcFolder && !mDestFolder.isNull() ) {
      while ( mSrcFolder->count() > 0 ) {
	KMMessage *msg = mSrcFolder->getMsg( 0 );
	mDestFolder->moveMsg( msg );
      }
      
      // Wait a little while before closing temp folders, just in case
      // new messages arrive for filtering.
      tempCloseFoldersTimer->start( 60*1000, true );
    }
    mSerNums.clear(); //abandon
    mFetchSerNums.clear(); //abandon

    if (mFiltersAreQueued)
      mFilters = mQueuedFilters;
    mQueuedFilters.clear();
    mFiltersAreQueued = false;
    ReturnCode aResult = mResult;
    mResult = ResultOk;
    mExecutingLock = false;
    emit result( aResult );
    if (mAutoDestruct)
      delete this;
  }
  // else a message may be in the process of being fetched or filtered
  // wait until both of these commitments are finished  then this 
  // method should be called again.
}

void ActionScheduler::fetchMessage()
{
  QValueListIterator<Q_UINT32> mFetchMessageIt = mFetchSerNums.begin();
  while (mFetchMessageIt != mFetchSerNums.end()) {
    if (!MessageProperty::transferInProgress(*mFetchMessageIt))
      break;
    ++mFetchMessageIt;
  }
  if (mFetchMessageIt == mFetchSerNums.end() && !mFetchSerNums.isEmpty())
    mResult = ResultError;
  if ((mFetchMessageIt == mFetchSerNums.end()) || (mResult != ResultOk)) {
    mFetchExecuting = false;
    if (!mSrcFolder->count())
      mSrcFolder->expunge();
    finishTimer->start( 0, true );
    return;
  }

  //If we got this far then there's a valid message to work with
  KMMsgBase *msgBase = messageBase( *mFetchMessageIt );
  if (mResult != ResultOk) {
    mFetchExecuting = false;
    return;
  }
  mFetchUnget = msgBase->isMessage();
  KMMessage *msg = message( *mFetchMessageIt );
  if (mResult != ResultOk) {
    mFetchExecuting = false;
    return;
  }

  if (msg && msg->isComplete()) {
    messageFetched( msg );
  } else if (msg) {
    FolderJob *job = msg->parent()->createJob( msg );
    connect( job, SIGNAL(messageRetrieved( KMMessage* )),
	     SLOT(messageFetched( KMMessage* )) );
    job->start();
  } else {
    mFetchExecuting = false;
    mResult = ResultError;
    finishTimer->start( 0, true );
    return;
  }
}

void ActionScheduler::messageFetched( KMMessage *msg )
{
  mFetchSerNums.remove( mFetchSerNums.begin() );

  if ((mSet & KMFilterMgr::Explicit) ||
      (msg->headerField( "X-KMail-Filtered" ).isEmpty())) {
    QString serNumS;
    serNumS.setNum( msg->getMsgSerNum() );
    KMMessage *newMsg = new KMMessage;
    newMsg->fromString(msg->asString());
    newMsg->setStatus(msg->status());
    newMsg->setComplete(msg->isComplete());
    newMsg->setHeaderField( "X-KMail-Filtered", serNumS );
    mSrcFolder->addMsg( newMsg );
  }
  if (mFetchUnget && msg->parent())
    msg->parent()->unGetMsg( msg->parent()->find( msg ));
  fetchMessageTimer->start( 0, true );
  return;
}

void ActionScheduler::msgAdded( KMFolder*, Q_UINT32 serNum )
{
  if (!mIgnore)
    enqueue( serNum );
}

void ActionScheduler::enqueue(Q_UINT32 serNum)
{
  if (mResult != ResultOk)
    return; // An error has already occurred don't even try to process this msg

  if (MessageProperty::filtering( serNum )) {
    // Not good someone else is already filtering this msg
    mResult = ResultError;
    if (!mExecuting)
      finishTimer->start( 0, true );
  } else {
    // Everything is ok async filter this message
    mSerNums.append( serNum );

    if (!mExecuting) {
      //Need to (re)start incomplete msg filtering chain
      mExecuting = true;
      mMessageIt = mSerNums.begin();
      processMessageTimer->start( 0, true );
    }
  }
}

void ActionScheduler::processMessage()
{
  if (mExecutingLock)
    return;
  mExecutingLock = true;
  mMessageIt = mSerNums.begin();
  while (mMessageIt != mSerNums.end()) {
    if (!MessageProperty::transferInProgress(*mMessageIt))
      break;
    ++mMessageIt;
  }
  if (mMessageIt == mSerNums.end() && !mSerNums.isEmpty())
    mResult = ResultError;
  if ((mMessageIt == mSerNums.end()) || (mResult != ResultOk)) {
    mExecutingLock = false;
    mExecuting = false;
    finishTimer->start( 0, true );
    return;
  }

  //If we got this far then there's a valid message to work with
  KMMsgBase *msgBase = messageBase( *mMessageIt );
  if (mResult != ResultOk) {
    mExecuting = false;
    return;
  }
  
  MessageProperty::setFiltering( *mMessageIt, true );
  MessageProperty::setFilterHandler( *mMessageIt, this );
  MessageProperty::setFilterFolder( *mMessageIt, mDestFolder );
  mFilterIt = mFilters.begin();

  mUnget = msgBase->isMessage();
  KMMessage *msg = message( *mMessageIt );
  if (mResult != ResultOk) {
    mExecuting = false;
    return;
  }

  bool mdnEnabled = true;
  {
    KConfigGroup mdnConfig( kmkernel->config(), "MDN" );
    int mode = mdnConfig.readNumEntry( "default-policy", 0 );
    if (!mode || mode < 0 || mode > 3)
      mdnEnabled = false;
  }
  mdnEnabled = true; // For 3.2 force all mails to be complete

  if ((msg && msg->isComplete()) ||
      (msg && !(*mFilterIt).requiresBody(msg) && !mdnEnabled))
  {
    // We have a complete message or
    // we can work with an incomplete message
    // Get a write lock on the message while it's being filtered
    msg->setTransferInProgress( true );
    filterMessageTimer->start( 0, true );
    return;
  }
  if (msg) {
    FolderJob *job = msg->parent()->createJob( msg );
    connect( job, SIGNAL(messageRetrieved( KMMessage* )),
	     SLOT(messageRetrieved( KMMessage* )) );
    job->start();
  } else {
    mExecuting = false;
    mResult = ResultError;
    finishTimer->start( 0, true );
    return;
  }
}

void ActionScheduler::messageRetrieved(KMMessage* msg)
{
  // Get a write lock on the message while it's being filtered
  msg->setTransferInProgress( true );
  filterMessageTimer->start( 0, true );
}

void ActionScheduler::filterMessage()
{
  if (mFilterIt == mFilters.end()) {
    moveMessage();
    return;
  }
  if (((mSet & KMFilterMgr::Outbound) && (*mFilterIt).applyOnOutbound()) ||
      ((mSet & KMFilterMgr::Inbound) && (*mFilterIt).applyOnInbound()) ||
      ((mSet & KMFilterMgr::Explicit) && (*mFilterIt).applyOnExplicit())) {
      // filter is applicable
    if (mAlwaysMatch ||
	(*mFilterIt).pattern()->matches( *mMessageIt )) {
      mFilterAction = (*mFilterIt).actions()->first();
      actionMessage();
      return;
    }
  }
  ++mFilterIt;
  filterMessageTimer->start( 0, true );
}

void ActionScheduler::actionMessage(KMFilterAction::ReturnCode res)
{
  if (res == KMFilterAction::CriticalError) {
    mResult = ResultCriticalError;
    finish(); //must handle critical errors immediately
  }
  if (mFilterAction) {
    KMMessage *msg = message( *mMessageIt );
    if (msg) {
      KMFilterAction *action = mFilterAction;
      mFilterAction = (*mFilterIt).actions()->next();
      action->processAsync( msg );
    }
  } else {
    // there are no more actions
    if ((*mFilterIt).stopProcessingHere())
      mFilterIt = mFilters.end();
    else
      ++mFilterIt;
    filterMessageTimer->start( 0, true );
  }
}

void ActionScheduler::moveMessage()
{
  KMMsgBase *msgBase = messageBase( *mMessageIt );
  if (!msgBase)
    return;

  MessageProperty::setTransferInProgress( *mMessageIt, false, true );
  KMMessage *msg = message( *mMessageIt );
  KMFolder *folder = MessageProperty::filterFolder( *mMessageIt );
  QString serNumS = msg->headerField( "X-KMail-Filtered" );
  if (!serNumS.isEmpty())
    mOriginalSerNum = serNumS.toUInt();
  else
    mOriginalSerNum = 0;
  MessageProperty::setFilterHandler( *mMessageIt, 0 );
  MessageProperty::setFiltering( *mMessageIt, false );
  mSerNums.remove( *mMessageIt );

  KMMessage *orgMsg = 0;
  ReturnCode mOldReturnCode = mResult;
  if (mOriginalSerNum)
    orgMsg = message( mOriginalSerNum );
  mResult = mOldReturnCode; // ignore errors in deleting original message
  if (!orgMsg || !orgMsg->parent()) {
    // Original message is gone, no point filtering it anymore
    mSrcFolder->removeMsg( mSrcFolder->find( msg ) );
    mExecutingLock = false;
    processMessageTimer->start( 0, true );
  } else {
    if (!folder) // no filter folder specified leave in current place
      folder = orgMsg->parent();
  }
  
  mIgnore = true;
  assert( msg->parent() == mSrcFolder.operator->() );
  mSrcFolder->take( mSrcFolder->find( msg ) );
  mSrcFolder->addMsg( msg );
  mIgnore = false;
  
  if (msg && kmkernel->folderIsTrash( folder ))
    KMFilterAction::sendMDN( msg, KMime::MDN::Deleted );

  KMCommand *cmd = new KMMoveCommand( folder, msg );
  connect ( cmd, SIGNAL( completed(bool) ),
	    this, SLOT( moveMessageFinished(bool) ) );
  cmd->start();
}

void ActionScheduler::moveMessageFinished(bool success)
{
  if ( !success )
    mResult = ResultError;

  if (!mSrcFolder->count())
    mSrcFolder->expunge();

  // in case the message stayed in the current folder TODO optimize
  if ( mHeaders )
    mHeaders->clearSelectableAndAboutToBeDeleted( mOriginalSerNum );
  KMMessage *msg = 0;
  ReturnCode mOldReturnCode = mResult;
  if (mOriginalSerNum)
    msg = message( mOriginalSerNum );
  mResult = mOldReturnCode; // ignore errors in deleting original message
  if (msg && msg->parent()) {
    KMCommand *cmd = new KMMoveCommand( 0, msg );
    cmd->start();
  }

  if (mResult == ResultOk) {
    mExecutingLock = false;
    processMessageTimer->start( 0, true );
  } else {
    finishTimer->start( 0, true );
  }
  // else moveMessageFinished should call finish
}

#include "actionscheduler.moc"
