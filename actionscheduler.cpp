/*  Action Scheduler

    This file is part of KMail, the KDE mail client.
    Copyright (c) Don Sanders <sanders@kde.org>

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

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/
#include <kdebug.h> // FIXME

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "actionscheduler.h"

#include "filterlog.h"
#include "messageproperty.h"
#include "kmfilter.h"
#include "kmfolderindex.h"
#include "kmfoldermgr.h"
#include "kmmsgdict.h"
#include "kmcommands.h"
#include "kmheaders.h"
#include "accountmanager.h"
using KMail::AccountManager;

#include <qtimer.h>
#include <kconfig.h>
#include <kstandarddirs.h>

using namespace KMail;
typedef QPtrList<KMMsgBase> KMMessageList;


KMFolderMgr* ActionScheduler::tempFolderMgr = 0;
int ActionScheduler::refCount = 0;
int ActionScheduler::count = 0;
QValueList<ActionScheduler*> *ActionScheduler::schedulerList = 0;
bool ActionScheduler::sEnabled = false;
bool ActionScheduler::sEnabledChecked = false;

ActionScheduler::ActionScheduler(KMFilterMgr::FilterSet set,
				 QValueList<KMFilter*> filters,
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
  mAccountId = 0;
  mAccount = false;
  lastCommand = 0;
  lastJob = 0;
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
  timeOutTimer = new QTimer( this );
  connect( timeOutTimer, SIGNAL(timeout()), this, SLOT(timeOut()));
  fetchTimeOutTimer = new QTimer( this );
  connect( fetchTimeOutTimer, SIGNAL(timeout()), this, SLOT(fetchTimeOut()));

  QValueList<KMFilter*>::Iterator it = filters.begin();
  for (; it != filters.end(); ++it)
    mFilters.append( **it );
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
  if (!schedulerList)
      schedulerList = new QValueList<ActionScheduler*>;
  schedulerList->append( this );
}

ActionScheduler::~ActionScheduler()
{
  schedulerList->remove( this );
  tempCloseFolders();
  mSrcFolder->close("actionschedsrc");

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
  srcFolder->open("actionschedsrc");
  if (mSrcFolder) {
    disconnect( mSrcFolder, SIGNAL(msgAdded(KMFolder*, Q_UINT32)),
		this, SLOT(msgAdded(KMFolder*, Q_UINT32)) );
    mSrcFolder->close("actionschedsrc");
  }
  mSrcFolder = srcFolder;
  int i = 0;
  for (i = 0; i < mSrcFolder->count(); ++i)
    enqueue( mSrcFolder->getMsgBase( i )->getMsgSerNum() );
  if (mSrcFolder)
    connect( mSrcFolder, SIGNAL(msgAdded(KMFolder*, Q_UINT32)),
	     this, SLOT(msgAdded(KMFolder*, Q_UINT32)) );
}

void ActionScheduler::setFilterList( QValueList<KMFilter*> filters )
{
  mFiltersAreQueued = true;
  mQueuedFilters.clear();
  
  QValueList<KMFilter*>::Iterator it = filters.begin();
  for (; it != filters.end(); ++it)
    mQueuedFilters.append( **it );
  if (!mExecuting) {
      mFilters = mQueuedFilters;
      mFiltersAreQueued = false;
      mQueuedFilters.clear();
  }
}

int ActionScheduler::tempOpenFolder( KMFolder* aFolder )
{
  assert( aFolder );
  tempCloseFoldersTimer->stop();
  if ( aFolder == mSrcFolder.operator->() )
    return 0;

  int rc = aFolder->open("actionsched");
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
      folder->close("actionsched");
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
  if (mResult != ResultOk) {
      if ((mResult != ResultCriticalError) && 
	  !mExecuting && !mExecutingLock && !mFetchExecuting) {
	  mResult = ResultOk; // Recoverable error
	  if (!mFetchSerNums.isEmpty()) {
	      mFetchSerNums.push_back( mFetchSerNums.first() );
	      mFetchSerNums.pop_front();
	  }
      } else
	  return; // An error has already occurred don't even try to process this msg
  }
  if (MessageProperty::filtering( serNum )) {
    // Not good someone else is already filtering this msg
    mResult = ResultError;
    if (!mExecuting && !mFetchExecuting)
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
  KMMsgDict::instance()->getLocation( serNum, &folder, &idx );
  // It's possible that the message has been deleted or moved into a
  // different folder
  if (folder && (idx != -1)) {
    // everything is ok
    tempOpenFolder( folder ); // just in case msg has moved
    msg = folder->getMsgBase( idx );
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
  KMMsgDict::instance()->getLocation( serNum, &folder, &idx );
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
  if (mResult != ResultOk) {
    // Must handle errors immediately
    emit result( mResult );
    return;
  }

  if (!mExecuting) {

  if (!mFetchSerNums.isEmpty()) {
    // Possibly if (mResult == ResultOk) should cancel job and start again.
    // Believe smarter logic to bail out if an error has occurred is required.
    // Perhaps should be testing for mFetchExecuting or at least set it to true
    fetchMessageTimer->start( 0, true ); // give it a bit of time at a test
    return;
  } else {
    mFetchExecuting = false;
  }

  if (mSerNums.begin() != mSerNums.end()) {
    mExecuting = true;
    processMessageTimer->start( 0, true );
    return;
  }

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

  //  Note: Perhaps this could be improved. We shouldn't give up straight away
  //        if !mFetchSerNums.isEmpty (becausing transferInProgress is true
  //        for some messages). Instead we should delay for a minute or so and
  //        again.
  if (mFetchMessageIt == mFetchSerNums.end() && !mFetchSerNums.isEmpty()) {
    mResult = ResultError;
  }
  if ((mFetchMessageIt == mFetchSerNums.end()) || (mResult != ResultOk)) {
    mFetchExecuting = false;
    if (!mSrcFolder->count())
      mSrcFolder->expunge();
    finishTimer->start( 0, true );
    return;
  }

  //If we got this far then there's a valid message to work with
  KMMsgBase *msgBase = messageBase( *mFetchMessageIt );

  if ((mResult != ResultOk) || (!msgBase)) {
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
    fetchTimeOutTime = QTime::currentTime();
    fetchTimeOutTimer->start( 60 * 1000, true );
    FolderJob *job = msg->parent()->createJob( msg );
    connect( job, SIGNAL(messageRetrieved( KMMessage* )),
	     SLOT(messageFetched( KMMessage* )) );
    lastJob = job;
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
  fetchTimeOutTimer->stop();
  if (!msg) {
      // Should never happen, but sometimes does;
      fetchMessageTimer->start( 0, true );
      return;
  }
	
  mFetchSerNums.remove( msg->getMsgSerNum() );

  // Note: This may not be necessary. What about when it's time to 
  //       delete the original message?
  //       Is the new serial number being set correctly then?
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
  } else {
    fetchMessageTimer->start( 0, true );
  }
  if (mFetchUnget && msg->parent())
    msg->parent()->unGetMsg( msg->parent()->find( msg ));
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
    if (!mExecuting && !mFetchExecuting)
      finishTimer->start( 0, true );
  } else {
    // Everything is ok async filter this message
    mSerNums.append( serNum );

    if (!mExecuting) {
      // Note: Need to (re)start incomplete msg filtering chain
      //       The state of mFetchExecuting is of some concern.
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

  if (mMessageIt == mSerNums.end() && !mSerNums.isEmpty()) {
    mExecuting = false;
    processMessageTimer->start( 600, true );
  }

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
  if ( FilterLog::instance()->isLogging() ) {
    FilterLog::instance()->addSeparator();
  }
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
      ((mSet & KMFilterMgr::Inbound) && (*mFilterIt).applyOnInbound() &&
       (!mAccount ||
	(mAccount && (*mFilterIt).applyOnAccount(mAccountId)))) ||
      ((mSet & KMFilterMgr::Explicit) && (*mFilterIt).applyOnExplicit())) {

      // filter is applicable
    if ( FilterLog::instance()->isLogging() ) {
      QString logText( i18n( "<b>Evaluating filter rules:</b> " ) );
      logText.append( (*mFilterIt).pattern()->asString() );
      FilterLog::instance()->add( logText, FilterLog::patternDesc );
    }
    if (mAlwaysMatch ||
	(*mFilterIt).pattern()->matches( *mMessageIt )) {
      if ( FilterLog::instance()->isLogging() ) {
        FilterLog::instance()->add( i18n( "<b>Filter rules have matched.</b>" ), 
                                    FilterLog::patternResult );
      }
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
      if ( FilterLog::instance()->isLogging() ) {
        QString logText( i18n( "<b>Applying filter action:</b> %1" )
                        .arg( mFilterAction->displayString() ) );
        FilterLog::instance()->add( logText, FilterLog::appliedAction );
      }
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
    kdDebug(5006) << "The original serial number is missing. "
                  << "Cannot complete the filtering." << endl;
    mExecutingLock = false;
    processMessageTimer->start( 0, true );
    return;
  } else {
    if (!folder) // no filter folder specified leave in current place
      folder = orgMsg->parent();
  }

  mIgnore = true;
  assert( msg->parent() == mSrcFolder.operator->() );
  mSrcFolder->take( mSrcFolder->find( msg ) );
  mSrcFolder->addMsg( msg );
  mIgnore = false;

  if (msg && folder && kmkernel->folderIsTrash( folder ))
    KMFilterAction::sendMDN( msg, KMime::MDN::Deleted );

  timeOutTime = QTime::currentTime();
  KMCommand *cmd = new KMMoveCommand( folder, msg );
  connect( cmd, SIGNAL( completed( KMCommand * ) ),
	   this, SLOT( moveMessageFinished( KMCommand * ) ) );
  cmd->start();
  // sometimes the move command doesn't complete so time out after a minute
  // and move onto the next message
  lastCommand = cmd;
  timeOutTimer->start( 60 * 1000, true );
}

void ActionScheduler::moveMessageFinished( KMCommand *command )
{
  timeOutTimer->stop();
  if ( command->result() != KMCommand::OK )
    mResult = ResultError;

  if (!mSrcFolder->count())
    mSrcFolder->expunge();

  // in case the message stayed in the current folder TODO optimize
  if ( mHeaders )
    mHeaders->clearSelectableAndAboutToBeDeleted( mOriginalSerNum );
  KMMessage *msg = 0;
  ReturnCode mOldReturnCode = mResult;
  if (mOriginalSerNum) {
    msg = message( mOriginalSerNum );
    emit filtered( mOriginalSerNum );
  }
  
  mResult = mOldReturnCode; // ignore errors in deleting original message
  KMCommand *cmd = 0;
  if (msg && msg->parent()) {
    cmd = new KMMoveCommand( 0, msg );
//    cmd->start(); // Note: sensitive logic here.
  }

  if (mResult == ResultOk) {
    mExecutingLock = false;
    if (cmd)
	connect( cmd, SIGNAL( completed( KMCommand * ) ),
		 this, SLOT( processMessage() ) );
    else
	processMessageTimer->start( 0, true );
  } else {
    // Note: An alternative to consider is just calling 
    //       finishTimer->start and returning
    if (cmd)
	connect( cmd, SIGNAL( completed( KMCommand * ) ),
		 this, SLOT( finish() ) );
    else
	finishTimer->start( 0, true );
  }
  if (cmd)
    cmd->start();
  // else moveMessageFinished should call finish
}

void ActionScheduler::copyMessageFinished( KMCommand *command )
{
  if ( command->result() != KMCommand::OK )
    actionMessage( KMFilterAction::ErrorButGoOn );
  else 
    actionMessage();
}

void ActionScheduler::timeOut()
{
  // Note: This is a good place for a debug statement
  assert( lastCommand );
  // sometimes imap jobs seem to just stall so give up and move on
  disconnect( lastCommand, SIGNAL( completed( KMCommand * ) ),
	      this, SLOT( moveMessageFinished( KMCommand * ) ) );
  lastCommand = 0;
  mExecutingLock = false;
  mExecuting = false;
  finishTimer->start( 0, true );
  if (mOriginalSerNum) // Try again
      execFilters( mOriginalSerNum );
}

void ActionScheduler::fetchTimeOut()
{
  // Note: This is a good place for a debug statement
  assert( lastJob );
  // sometimes imap jobs seem to just stall so give up and move on
  disconnect( lastJob, SIGNAL(messageRetrieved( KMMessage* )),
	      this, SLOT(messageFetched( KMMessage* )) );
  lastJob->kill();
  lastJob = 0;
  fetchMessageTimer->start( 0, true );
}

QString ActionScheduler::debug()
{
    QString res;
    QValueList<ActionScheduler*>::iterator it;
    int i = 1;
    for ( it = schedulerList->begin(); it != schedulerList->end(); ++it ) {
	res.append( QString( "ActionScheduler #%1.\n" ).arg( i ) );
	if ((*it)->mAccount && kmkernel->find( (*it)->mAccountId )) {
	    res.append( QString( "Account %1, Name %2.\n" )
			.arg( (*it)->mAccountId )
			.arg( kmkernel->acctMgr()->find( (*it)->mAccountId )->name() ) );
	}
	res.append( QString( "mExecuting %1, " ).arg( (*it)->mExecuting ? "true" : "false" ) );
	res.append( QString( "mExecutingLock %1, " ).arg( (*it)->mExecutingLock ? "true" : "false" ) );
	res.append( QString( "mFetchExecuting %1.\n" ).arg( (*it)->mFetchExecuting ? "true" : "false" ) );
	res.append( QString( "mOriginalSerNum %1.\n" ).arg( (*it)->mOriginalSerNum ) );
	res.append( QString( "mMessageIt %1.\n" ).arg( ((*it)->mMessageIt != 0) ? *(*it)->mMessageIt : 0 ) );
	res.append( QString( "mSerNums count %1, " ).arg( (*it)->mSerNums.count() ) );
	res.append( QString( "mFetchSerNums count %1.\n" ).arg( (*it)->mFetchSerNums.count() ) );
	res.append( QString( "mResult " ) );
	if ((*it)->mResult == ResultOk)
	    res.append( QString( "ResultOk.\n" ) );
	else if ((*it)->mResult == ResultError)
	    res.append( QString( "ResultError.\n" ) );
	else if ((*it)->mResult == ResultCriticalError)
	    res.append( QString( "ResultCriticalError.\n" ) );
	else
	    res.append( QString( "Unknown.\n" ) );
	    
	++i;
    }
    return res;
}

bool ActionScheduler::isEnabled()
{
    if (sEnabledChecked)
	return sEnabled;

    sEnabledChecked = true;
    KConfig* config = KMKernel::config();
    KConfigGroupSaver saver(config, "General");
    sEnabled = config->readBoolEntry("action-scheduler", false);
    return sEnabled;
}

bool ActionScheduler::ignoreChanges( bool ignore )
{
  bool oldValue = mIgnore;
  mIgnore = ignore;
  return oldValue;
}

#include "actionscheduler.moc"
