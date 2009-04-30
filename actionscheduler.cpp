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

#include "actionscheduler.h"

#include "filterlog.h"
#include "messageproperty.h"
#include "kmfilter.h"
#include "kmfolderindex.h"
#include "kmfolderimap.h"
#include "kmfoldermgr.h"
#include "kmmsgdict.h"
#include "kmcommands.h"
#include "kmheaders.h"
#include "broadcaststatus.h"
#include "accountmanager.h"
using KMail::AccountManager;

#include <kdebug.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <kconfiggroup.h>

#include <QTimer>

using namespace KMail;


KMFolderMgr* ActionScheduler::tempFolderMgr = 0;
int ActionScheduler::refCount = 0;
int ActionScheduler::count = 0;
QList<ActionScheduler*> *ActionScheduler::schedulerList = 0;
bool ActionScheduler::sEnabled = false;
bool ActionScheduler::sEnabledChecked = false;

ActionScheduler::ActionScheduler(KMFilterMgr::FilterSet set,
                                 QList<KMFilter*> filters,
                                 KMHeaders *headers,
                                 KMFolder *srcFolder)
  : mSet( set ),
    mHeaders( headers ),
    mIgnoreFilterSet( false )
{
  ++count;
  ++refCount;
  mExecuting = false;
  mExecutingLock = false;
  mFetchExecuting = false;
  mFiltersAreQueued = false;
  mResult = ResultOk;
  mAutoDestruct = false;
  mAlwaysMatch = false;
  mAccountId = 0;
  mAccount = false;
  lastCommand = 0;
  lastJob = 0;
  finishTimer = new QTimer( this );
  finishTimer->setSingleShot( true );
  connect( finishTimer, SIGNAL(timeout()), this, SLOT(finish()));
  fetchMessageTimer = new QTimer( this );
  fetchMessageTimer->setSingleShot( true );
  connect( fetchMessageTimer, SIGNAL(timeout()), this, SLOT(fetchMessage()));
  tempCloseFoldersTimer = new QTimer( this );
  tempCloseFoldersTimer->setSingleShot( true );
  connect( tempCloseFoldersTimer, SIGNAL(timeout()), this, SLOT(tempCloseFolders()));
  processMessageTimer = new QTimer( this );
  processMessageTimer->setSingleShot( true );
  connect( processMessageTimer, SIGNAL(timeout()), this, SLOT(processMessage()));
  filterMessageTimer = new QTimer( this );
  filterMessageTimer->setSingleShot( true );
  connect( filterMessageTimer, SIGNAL(timeout()), this, SLOT(filterMessage()));
  timeOutTimer = new QTimer( this );
  timeOutTimer->setSingleShot( true );
  connect( timeOutTimer, SIGNAL(timeout()), this, SLOT(timeOut()));
  fetchTimeOutTimer = new QTimer( this );
  fetchTimeOutTimer->setSingleShot( true );
  connect( fetchTimeOutTimer, SIGNAL(timeout()), this, SLOT(fetchTimeOut()));

  QList<KMFilter*>::Iterator it = filters.begin();
  for (; it != filters.end(); ++it)
    mFilters.append( *it );
  mDestFolder = 0;
  if (srcFolder) {
    mDeleteSrcFolder = false;
    setSourceFolder( srcFolder );
  } else {
    QString tmpName;
    tmpName.setNum( count );
    if (!tempFolderMgr)
      tempFolderMgr = new KMFolderMgr(KStandardDirs::locateLocal("data","kmail/filter"));
    KMFolder *tempFolder = tempFolderMgr->findOrCreate( tmpName );
    tempFolder->expunge();
    mDeleteSrcFolder = true;
    setSourceFolder( tempFolder );
  }
  if (!schedulerList)
      schedulerList = new QList<ActionScheduler*>;
  schedulerList->append( this );
}

ActionScheduler::~ActionScheduler()
{
  schedulerList->removeAll( this );
  tempCloseFolders();
  disconnect( mSrcFolder, SIGNAL(closed()),
              this, SLOT(folderClosedOrExpunged()) );
  disconnect( mSrcFolder, SIGNAL(expunged(KMFolder*)),
              this, SLOT(folderClosedOrExpunged()) );
  mSrcFolder->close( "actionschedsrc" );

  if ( mDeleteSrcFolder ) {
    tempFolderMgr->remove( mSrcFolder );
  }

  --refCount;
  if ( refCount == 0 ) {
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

void ActionScheduler::setIgnoreFilterSet( bool ignore )
{
  mIgnoreFilterSet = ignore;
}

void ActionScheduler::setDefaultDestinationFolder( KMFolder *destFolder )
{
  mDestFolder = destFolder;
}

void ActionScheduler::setSourceFolder( KMFolder *srcFolder )
{
  srcFolder->open( "actionschedsrc" );
  if ( mSrcFolder ) {
    disconnect( mSrcFolder, SIGNAL(msgAdded(KMFolder*, quint32)),
		this, SLOT(msgAdded(KMFolder*, quint32)) );
    disconnect( mSrcFolder, SIGNAL(closed()),
                this, SLOT(folderClosedOrExpunged()) );
    disconnect( mSrcFolder, SIGNAL(expunged(KMFolder*)),
                this, SLOT(folderClosedOrExpunged()) );
    mSrcFolder->close( "actionschedsrc" );
  }
  mSrcFolder = srcFolder;
  int i = 0;
  for ( i = 0; i < mSrcFolder->count(); ++i ) {
    enqueue( mSrcFolder->getMsgBase( i )->getMsgSerNum() );
  }
  if ( mSrcFolder ) {
    connect( mSrcFolder, SIGNAL(msgAdded(KMFolder*, quint32)),
             this, SLOT(msgAdded(KMFolder*, quint32)) );
    connect( mSrcFolder, SIGNAL(closed()),
             this, SLOT(folderClosedOrExpunged()) );
    connect( mSrcFolder, SIGNAL(expunged(KMFolder*)),
             this, SLOT(folderClosedOrExpunged()) );
  }
}

void ActionScheduler::setFilterList( QList<KMFilter*> filters )
{
  mFiltersAreQueued = true;
  mQueuedFilters.clear();

  QList<KMFilter*>::Iterator it = filters.begin();
  for (; it != filters.end(); ++it)
    mQueuedFilters.append( *it );
  if (!mExecuting) {
      mFilters = mQueuedFilters;
      mFiltersAreQueued = false;
      mQueuedFilters.clear();
  }
}

void ActionScheduler::folderClosedOrExpunged()
{
  // mSrcFolder has been closed. reopen it.
  if ( mSrcFolder )
  {
    mSrcFolder->open( "actionsched" );
  }
}

int ActionScheduler::tempOpenFolder( KMFolder *aFolder )
{
  assert( aFolder );
  tempCloseFoldersTimer->stop();
  if ( aFolder == mSrcFolder.operator->() ) {
    return 0;
  }

  int rc = aFolder->open( "actionsched" );
  if ( rc ) {
    return rc;
  }

  mOpenFolders.append( aFolder );
  return 0;
}

void ActionScheduler::tempCloseFolders()
{
  // close temp opened folders
  QList<QPointer<KMFolder> >::ConstIterator it;
  for ( it = mOpenFolders.begin(); it != mOpenFolders.end(); ++it ) {
    KMFolder *folder = *it;
    if ( folder ) {
      folder->close( "actionsched" );
    }
  }
  mOpenFolders.clear();
}

void ActionScheduler::execFilters( const QList<quint32> serNums )
{
  QList<quint32>::ConstIterator it;
  for ( it = serNums.begin(); it != serNums.end(); ++it ) {
    execFilters( *it );
  }
}

void ActionScheduler::execFilters( const QList<KMMsgBase*> msgList )
{
  KMMsgBase *msgBase;
  foreach ( msgBase, msgList ) {
    execFilters( msgBase->getMsgSerNum() );
  }
}

void ActionScheduler::execFilters( KMMsgBase *msgBase )
{
  execFilters( msgBase->getMsgSerNum() );
}

void ActionScheduler::execFilters(quint32 serNum)
{
  if ( mResult != ResultOk ) {
    if ( ( mResult != ResultCriticalError ) &&
         !mExecuting && !mExecutingLock && !mFetchExecuting ) {
      mResult = ResultOk; // Recoverable error
      if ( !mFetchSerNums.isEmpty() ) {
        mFetchSerNums.push_back( mFetchSerNums.first() );
        mFetchSerNums.pop_front();
      }
    }
    else {
      kDebug() << "Not starting filtering because of error. mExecuting=" << mExecuting
               << "mExecutingLock=" << mExecutingLock
               << "mFetchExecuting=" << mFetchExecuting;

      // An error has already occurred don't even try to process this msg
      return;
    }
  }

  if (MessageProperty::filtering( serNum )) {
    // Not good someone else is already filtering this msg
    mResult = ResultError;
    if (!mExecuting && !mFetchExecuting)
      finishTimer->start( 0 );
  } else {
    // Everything is ok async fetch this message
    mFetchSerNums.append( serNum );
    if (!mFetchExecuting) {
      //Need to (re)start incomplete msg fetching chain
      mFetchExecuting = true;
      fetchMessageTimer->start( 0 );
    }
  }
}

KMMsgBase *ActionScheduler::messageBase(quint32 serNum)
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
    finishTimer->start( 0 );
  }
  return msg;
}

KMMessage *ActionScheduler::message(quint32 serNum)
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
    finishTimer->start( 0 );
  }
  return msg;
}

void ActionScheduler::finish()
{
  // FIXME: In case of errors, the locks don't get reset. Which means when filtering
  //        one message errors out, and we will later filter another one, filtering
  //        will not start because of this.
  //        There is some cleanup code below, but this might not be the best thing
  //        to do here: When a single message fails, we want to continue with the
  //        rest and not completely reset everything...
  if (mResult != ResultOk) {
    // Must handle errors immediately
    emit result( mResult );
    return;
  }
  kDebug() << "Filtering finished.";

  if (!mExecuting) {

    if (!mFetchSerNums.isEmpty()) {
      // Possibly if (mResult == ResultOk) should cancel job and start again.
      // Believe smarter logic to bail out if an error has occurred is required.
      // Perhaps should be testing for mFetchExecuting or at least set it to true
      fetchMessageTimer->start( 0 ); // give it a bit of time at a test
      return;
    } else {
      mFetchExecuting = false;
    }

    if (mSerNums.begin() != mSerNums.end()) {
      mExecuting = true;
      processMessageTimer->start( 0 );
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
      tempCloseFoldersTimer->start( 60*1000 );
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
      deleteLater();
  }
  // else a message may be in the process of being fetched or filtered
  // wait until both of these commitments are finished  then this
  // method should be called again.
}

void ActionScheduler::fetchMessage()
{
  QList<quint32>::Iterator mFetchMessageIt = mFetchSerNums.begin();
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
    finishTimer->start( 0 );
    return;
  }

  //If we got this far then there's a valid message to work with
  KMMsgBase *msgBase = messageBase( *mFetchMessageIt );

  if ( !msgBase || mResult != ResultOk ) {
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
    fetchTimeOutTimer->start( 60 * 1000 );
    FolderJob *job = msg->parent()->createJob( msg );
    connect( job, SIGNAL(messageRetrieved( KMMessage* )),
             SLOT(messageFetched( KMMessage* )) );
    lastJob = job;
    job->start();
  } else {
    mFetchExecuting = false;
    mResult = ResultError;
    finishTimer->start( 0 );
    return;
  }
}

void ActionScheduler::messageFetched( KMMessage *msg )
{
  fetchTimeOutTimer->stop();
  if (!msg) {
      // Should never happen, but sometimes does;
      fetchMessageTimer->start( 0 );
      return;
  }

  mFetchSerNums.removeAll( msg->getMsgSerNum() );

  // Note: This may not be necessary. What about when it's time to
  //       delete the original message?
  //       Is the new serial number being set correctly then?
  if ((mSet & KMFilterMgr::Explicit) ||
      (msg->headerField( "X-KMail-Filtered" ).isEmpty())) {
    QString serNumS;
    serNumS.setNum( msg->getMsgSerNum() );
    KMMessage *newMsg = new KMMessage;
    newMsg->fromDwString(msg->asDwString());
    newMsg->setStatus(msg->status());
    newMsg->setComplete(msg->isComplete());
    newMsg->setHeaderField( "X-KMail-Filtered", serNumS );
    mSrcFolder->addMsg( newMsg );
  } else {
    // msg was already filtered (by another instance of KMail)
    emit filtered( msg->getMsgSerNum() );
    fetchMessageTimer->start( 0 );
  }
  if (mFetchUnget && msg->parent())
    msg->parent()->unGetMsg( msg->parent()->find( msg ));
  return;
}

void ActionScheduler::msgAdded( KMFolder*, quint32 serNum )
{
  if ( !mIgnoredSerNums.removeOne(serNum) )
    enqueue( serNum );
}

void ActionScheduler::enqueue(quint32 serNum)
{
  if (mResult != ResultOk)
    return; // An error has already occurred don't even try to process this msg

  if (MessageProperty::filtering( serNum )) {
    // Not good someone else is already filtering this msg
    mResult = ResultError;
    if (!mExecuting && !mFetchExecuting)
      finishTimer->start( 0 );
  } else {
    // Everything is ok async filter this message
    mSerNums.append( serNum );

    if (!mExecuting) {
      // Note: Need to (re)start incomplete msg filtering chain
      //       The state of mFetchExecuting is of some concern.
      mExecuting = true;
      mMessageIt = mSerNums.begin();
      processMessageTimer->start( 0 );
    }
  }
}

void ActionScheduler::processMessage()
{
  QString statusMsg = i18n( "%1 messages waiting to be filtered",
                            mFetchSerNums.count() );
  KPIM::BroadcastStatus::instance()->setStatusMsg( statusMsg );

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
    processMessageTimer->start( 600 );
  }

  if ((mMessageIt == mSerNums.end()) || (mResult != ResultOk)) {
    mExecutingLock = false;
    mExecuting = false;
    kDebug() << "Stopping filtering, error or end of message list.";
    finishTimer->start( 0 );
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
    int mode = mdnConfig.readEntry( "default-policy", 0 );
    if (!mode || mode < 0 || mode > 3)
      mdnEnabled = false;
  }
  mdnEnabled = true; // For 3.2 force all mails to be complete

  if ((msg && msg->isComplete()) ||
      (msg && !(*mFilterIt)->requiresBody(msg) && !mdnEnabled))
  {
    // We have a complete message or
    // we can work with an incomplete message
    // Get a write lock on the message while it's being filtered
    msg->setTransferInProgress( true );
    filterMessageTimer->start( 0 );
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
    finishTimer->start( 0 );
    return;
  }
}

void ActionScheduler::messageRetrieved(KMMessage* msg)
{
  // Get a write lock on the message while it's being filtered
  msg->setTransferInProgress( true );
  filterMessageTimer->start( 0 );
}

void ActionScheduler::filterMessage()
{
  if (mFilterIt == mFilters.end()) {
    moveMessage();
    return;
  }
  if ( mIgnoreFilterSet ||
      ((((mSet & KMFilterMgr::Outbound) && (*mFilterIt)->applyOnOutbound()) ||
      ((mSet & KMFilterMgr::Inbound) && (*mFilterIt)->applyOnInbound() &&
       (!mAccount ||
        (mAccount && (*mFilterIt)->applyOnAccount(mAccountId)))) ||
      ((mSet & KMFilterMgr::Explicit) && (*mFilterIt)->applyOnExplicit())))) {

      // filter is applicable
    if ( FilterLog::instance()->isLogging() ) {
      QString logText( i18n( "<b>Evaluating filter rules:</b> " ) );
      logText.append( (*mFilterIt)->pattern()->asString() );
      FilterLog::instance()->add( logText, FilterLog::patternDesc );
    }
    if (mAlwaysMatch || (*mFilterIt)->pattern()->matches( *mMessageIt )) {
      if ( FilterLog::instance()->isLogging() ) {
        FilterLog::instance()->add( i18n( "<b>Filter rules have matched.</b>" ),
                                    FilterLog::patternResult );
      }
      mFilterActionIt = (*mFilterIt)->actions()->begin();
      mFilterAction = (*mFilterActionIt);
      actionMessage();
      return;
    }
  }
  ++mFilterIt;
  filterMessageTimer->start( 0 );
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
        QString logText( i18n( "<b>Applying filter action:</b> %1",
                          mFilterAction->displayString() ) );
        FilterLog::instance()->add( logText, FilterLog::appliedAction );
      }
      KMFilterAction *action = mFilterAction;
//      mFilterAction = (*mFilterIt).actions()->next();
      if ( ++mFilterActionIt == (*mFilterIt)->actions()->end() )
        mFilterAction = 0;
      else mFilterAction = (*mFilterActionIt);
      action->processAsync( msg );
    }
  } else {
    // there are no more actions
    if ((*mFilterIt)->stopProcessingHere())
      mFilterIt = mFilters.end();
    else
      ++mFilterIt;
    filterMessageTimer->start( 0 );
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

  mSerNums.removeAll( *mMessageIt );

  KMMessage *orgMsg = 0;
  ReturnCode mOldReturnCode = mResult;
  if (mOriginalSerNum)
    orgMsg = message( mOriginalSerNum );
  mResult = mOldReturnCode; // ignore errors in deleting original message
  if (!orgMsg || !orgMsg->parent()) {
    // Original message is gone, no point filtering it anymore
    mSrcFolder->removeMsg( mSrcFolder->find( msg ) );
    kDebug(5006) << "The original serial number is missing."
                 << "Cannot complete the filtering.";
    mExecutingLock = false;
    processMessageTimer->start( 0 );
    return;
  } else {
    if (!folder) // no filter folder specified leave in current place
      folder = orgMsg->parent();
  }

  mIgnoredSerNums.append( msg->getMsgSerNum() );
  assert( msg->parent() == mSrcFolder.operator->() );
  mSrcFolder->take( mSrcFolder->find( msg ) );
  mSrcFolder->addMsg( msg );

  if (msg && folder && kmkernel->folderIsTrash( folder ))
    KMFilterAction::sendMDN( msg, KMime::MDN::Deleted );

  // If the target folder is an online IMAP folder, make sure to keep the same
  // serial number (otherwise the move commands thinks the message wasn't moved
  // correctly, which would trigger the error case in moveMessageFinished().
  Q_ASSERT( folder );
  if ( msg && folder && folder->storage() && dynamic_cast<KMFolderImap*>( folder->storage() ) )
    MessageProperty::setKeepSerialNumber( msg->getMsgSerNum(), true );

  timeOutTime = QTime::currentTime();
  KMCommand *cmd = new KMMoveCommand( folder, msg );
  connect( cmd, SIGNAL( completed( KMCommand * ) ),
           this, SLOT( moveMessageFinished( KMCommand * ) ) );
  cmd->start();
  // sometimes the move command doesn't complete so time out after a minute
  // and move onto the next message
  lastCommand = cmd;
  timeOutTimer->start( 60 * 1000 );
}

void ActionScheduler::moveMessageFinished( KMCommand *command )
{
  timeOutTimer->stop();
  bool movingFailed = false;
  if ( command->result() != KMCommand::OK ) {

    // Simply ignore the error. Moving the message back might have failed because
    // of GoogleMail preventing duplicates (see bug 166150).
    // We ignore the error because otherwise filtering would not continue with
    // the next message, filtering would just stop until KMail is restarted.
    // Normally, finish() is supposed to clear the error state properly, but
    // it doesn't do that and changing it can break all kind of logic.
    //
    // Note that ignoring the error does not mean dataloss, it just means that
    // the filter actions will not work for the message. By setting movingFailed
    // to false, we prevent the deletion of the original source message.
    //mResult = ResultError;

    movingFailed = true;
    kWarning() << "Moving the message from the temporary filter folder to the "
                  "target folder failed. Message will stay unfiltered.";
    kWarning() << "This might be because you're using GMail, see bug 166150.";
  }

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

  // Delete the original message, because we have just moved the filtered message
  // from the temporary filtering folder to the target folder, and don't need the
  // old unfiltered message there anymore.
  KMCommand *cmd = 0;
  if ( msg && msg->parent() && !movingFailed ) {
    cmd = new KMMoveCommand( 0, msg );
  }

  // When the above deleting is finished, filtering that single mail is complete.
  // The next state is processMessage(), which will start filtering the next
  // message if we still have mails in the temporary filter folder.
  // If there are no unfiltered messages there or an error was encountered,
  // processMessage() will stop filtering by setting mExecuting to false
  // and then going to the finish() state.
  //
  // processMessage() will only do something if mExecutingLock if false.
  //
  // We can't advance to the finish() state just here, because that
  // wouldn't reset mExecuting at all, so we make sure that the next stage is
  // processMessage() in all cases.
  mExecutingLock = false;

  if ( cmd ) {
    connect( cmd, SIGNAL( completed( KMCommand * ) ),
             this, SLOT( processMessage() ) );
    cmd->start();
  }
  else
    processMessageTimer->start( 0 );
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
  finishTimer->start( 0 );
  if (mOriginalSerNum) // Try again
      execFilters( mOriginalSerNum );
}

void ActionScheduler::fetchTimeOut()
{
  // Note: This is a good place for a debug statement
  if( !lastJob )
    return;
  // sometimes imap jobs seem to just stall so give up and move on
  disconnect( lastJob, SIGNAL(messageRetrieved( KMMessage* )),
              this, SLOT(messageFetched( KMMessage* )) );
  lastJob->kill();
  lastJob = 0;
  fetchMessageTimer->start( 0 );
}

QString ActionScheduler::debug()
{
  QString res;
  QList<ActionScheduler*>::iterator it;
  int i = 1;
  for ( it = schedulerList->begin(); it != schedulerList->end(); ++it ) {
    res.append( QString( "ActionScheduler #%1.\n" ).arg( i ) );
    if ((*it)->mAccount && kmkernel->acctMgr()->find( (*it)->mAccountId )) {
      res.append( QString( "Account %1, Name %2.\n" )
          .arg( (*it)->mAccountId )
          .arg( kmkernel->acctMgr()->find( (*it)->mAccountId )->name() ) );
    }
    res.append( QString( "mExecuting %1, " ).arg( (*it)->mExecuting ? "true" : "false" ) );
    res.append( QString( "mExecutingLock %1, " ).arg( (*it)->mExecutingLock ? "true" : "false" ) );
    res.append( QString( "mFetchExecuting %1.\n" ).arg( (*it)->mFetchExecuting ? "true" : "false" ) );
    res.append( QString( "mOriginalSerNum %1.\n" ).arg( (*it)->mOriginalSerNum ) );
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
  KConfigGroup group(config, "General");
  sEnabled = group.readEntry("action-scheduler", false );
  return sEnabled;
}

void ActionScheduler::addMsgToIgnored( quint32 serNum )
{
  kDebug() << "Adding ignored message:" << serNum;
  mIgnoredSerNums.append( serNum );
}

#include "actionscheduler.moc"
