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
#include "kmkernel.h"
#include "filterlog.h"
#include "messageproperty.h"
#include "kmfilter.h"
#include "kmcommands.h"
#include "broadcaststatus.h"

#include <kdebug.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <kconfiggroup.h>

#include <akonadi/itemfetchjob.h>
#include <kmime/kmime_message.h>

#include <QTimer>

using namespace KMail;

#if 0
KMFolderMgr* ActionScheduler::tempFolderMgr = 0;
#endif
int ActionScheduler::refCount = 0;
int ActionScheduler::count = 0;
QList<ActionScheduler*> *ActionScheduler::schedulerList = 0;
bool ActionScheduler::sEnabled = false;
bool ActionScheduler::sEnabledChecked = false;

ActionScheduler::ActionScheduler(KMFilterMgr::FilterSet set,
                                 QList<KMFilter*> filters,
                                 const Akonadi::Collection & srcFolder)
  : mSet( set ),
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
  processMessageTimer = new QTimer( this );
  processMessageTimer->setSingleShot( true );
  connect( processMessageTimer, SIGNAL(timeout()), this, SLOT(processMessage()));
  filterMessageTimer = new QTimer( this );
  filterMessageTimer->setSingleShot( true );
  connect( filterMessageTimer, SIGNAL(timeout()), this, SLOT(filterMessage()));
  timeOutTimer = new QTimer( this );
  timeOutTimer->setSingleShot( true );
  connect( timeOutTimer, SIGNAL(timeout()), this, SLOT(timeOut()));

  QList<KMFilter*>::Iterator it = filters.begin();
  for (; it != filters.end(); ++it)
    mFilters.append( *it );
  //mDestFolder = 0;
  if (srcFolder.isValid()) {
    mDeleteSrcFolder = false;
    setSourceFolder( srcFolder );
  } else {
    QString tmpName;
    tmpName.setNum( count );
#if 0
    if (!tempFolderMgr)
      tempFolderMgr = new KMFolderMgr(KStandardDirs::locateLocal("data","kmail/filter"));
    KMFolder *tempFolder = tempFolderMgr->findOrCreate( tmpName );
    tempFolder->expunge();
    mDeleteSrcFolder = true;
    setSourceFolder( tempFolder );
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  }
  if (!schedulerList)
      schedulerList = new QList<ActionScheduler*>;
  schedulerList->append( this );
}

ActionScheduler::~ActionScheduler()
{
  schedulerList->removeAll( this );
#if 0
  if ( mDeleteSrcFolder ) {
    tempFolderMgr->remove( mSrcFolder );
  }
#endif
  --refCount;
#if 0
  if ( refCount == 0 ) {
    delete tempFolderMgr;
    tempFolderMgr = 0;
  }
#endif
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

void ActionScheduler::setSourceFolder( const Akonadi::Collection &srcFolder )
{
#if 0 //TODO port
  if ( mSrcFolder ) {
    disconnect( mSrcFolder, SIGNAL(msgAdded(KMFolder*, quint32)),
		this, SLOT(msgAdded(KMFolder*, quint32)) );
  }
  mSrcFolder = srcFolder;
  int i = 0;
  for ( i = 0; i < mSrcFolder->count(); ++i ) {
    enqueue( mSrcFolder->getMsgBase( i )->getMsgSerNum() );
  }
  if ( mSrcFolder ) {
    connect( mSrcFolder, SIGNAL(msgAdded(KMFolder*, quint32)),
             this, SLOT(msgAdded(KMFolder*, quint32)) );
  }
#endif
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

void ActionScheduler::execFilters( const Akonadi::Item & item )
{
  if ( mResult != ResultOk ) {
    if ( ( mResult != ResultCriticalError ) &&
         !mExecuting && !mExecutingLock && !mFetchExecuting ) {
      mResult = ResultOk; // Recoverable error
      if ( !mFetchItems.isEmpty() ) {
        mFetchItems.push_back( mFetchItems.first() );
        mFetchItems.pop_front();
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

  if (MessageProperty::filtering( item )) {
    // Not good someone else is already filtering this msg
    mResult = ResultError;
    if (!mExecuting && !mFetchExecuting)
      finishTimer->start( 0 );
  } else {
    // Everything is ok async fetch this message
    mFetchItems.append( item );
    if (!mFetchExecuting) {
      //Need to (re)start incomplete msg fetching chain
      mFetchExecuting = true;
      fetchMessageTimer->start( 0 );
    }
  }
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

    if (!mFetchItems.isEmpty()) {
      // Possibly if (mResult == ResultOk) should cancel job and start again.
      // Believe smarter logic to bail out if an error has occurred is required.
      // Perhaps should be testing for mFetchExecuting or at least set it to true
      fetchMessageTimer->start( 0 ); // give it a bit of time at a test
      return;
    } else {
      mFetchExecuting = false;
    }

    if (mItems.begin() != mItems.end()) {
      mExecuting = true;
      processMessageTimer->start( 0 );
      return;
    }
    // If an error has occurred and a permanent source folder has
    // been set then move all the messages left in the source folder
    // to the inbox. If no permanent source folder has been set
    // then abandon filtering of queued messages.
    if (!mDeleteSrcFolder && mDestFolder.isValid() ) {
#if 0 //TODO port to akonadi
      while ( mSrcFolder->count() > 0 ) {
        KMime::Message *msg = mSrcFolder->getMsg( 0 );
        mDestFolder->moveMsg( msg );
      }
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
    }

    mItems.clear(); //abandon
    mFetchItems.clear(); //abandon

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
  if ( mFetchItems.isEmpty() || mResult != ResultOk ) {
    mFetchExecuting = false;
    return;
  }

  const Akonadi::Item item = mFetchItems.first();
  if ( !item.isValid() || mResult != ResultOk ) {
    mFetchExecuting = false;
    return;
  }
#if 0 //TODO port to akonadi
  if ( item.isValid() && item.isComplete() ) {
    messageFetched( item );
  } else
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  if ( item.isValid() ) {
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( item, this );
    job->fetchScope().fetchFullPayload();
    connect( job, SIGNAL(result(KJob*)), SLOT(messageFetchResult(KJob*)) );
    lastJob = job;
  } else {
    mFetchExecuting = false;
    mResult = ResultError;
    finishTimer->start( 0 );
    return;
  }
}

void ActionScheduler::messageFetchResult(KJob* job)
{
  Akonadi::Item item;
  if ( job->error() ) {
    mResult = ResultError;
    kError() << job->errorText();
  } else {
    item = static_cast<Akonadi::ItemFetchJob*>( job )->items().first();
  }
  messageFetched( item );
}


void ActionScheduler::messageFetched( const Akonadi::Item &item )
{
  if ( !item.isValid() ) {
      // Should never happen, but sometimes does;
      fetchMessageTimer->start( 0 );
      return;
  }

  mFetchItems.removeAll( item );

#if 0 //TODO port to akonadi
  // Note: This may not be necessary. What about when it's time to
  //       delete the original message?
  //       Is the new serial number being set correctly then?
  if ((mSet & KMFilterMgr::Explicit) ||
      (msg->headerField( "X-KMail-Filtered" ).isEmpty())) {
    QString serNumS;
    serNumS.setNum( msg->getMsgSerNum() );
    KMime::Message *newMsg = new KMime::Message;
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
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

void ActionScheduler::msgAdded( const Akonadi::Collection &, const Akonadi::Item &item )
{
  if ( !mIgnoredItems.removeOne( item ) )
    enqueue( item );
}

void ActionScheduler::enqueue(const Akonadi::Item &item)
{
  if (mResult != ResultOk)
    return; // An error has already occurred don't even try to process this msg

  if ( MessageProperty::filtering( item ) ) {
    // Not good someone else is already filtering this msg
    mResult = ResultError;
    if ( !mExecuting && !mFetchExecuting )
      finishTimer->start( 0 );
  } else {
    // Everything is ok async filter this message
    mItems.append( item );

    if ( !mExecuting ) {
      // Note: Need to (re)start incomplete msg filtering chain
      //       The state of mFetchExecuting is of some concern.
      mExecuting = true;
      mMessageIt = mItems.begin();
      processMessageTimer->start( 0 );
    }
  }
}

void ActionScheduler::processMessage()
{
  QString statusMsg = i18np( "1 message waiting to be filtered",
                            "%1 messages waiting to be filtered",
                            mFetchItems.count() );
  KPIM::BroadcastStatus::instance()->setStatusMsg( statusMsg );

  if (mExecutingLock)
    return;
  mExecutingLock = true;
  mMessageIt = mItems.begin();

  if ( mMessageIt == mItems.end() ) {
    mExecuting = false;
    processMessageTimer->start( 600 );
  }

  if ( mMessageIt == mItems.end() || mResult != ResultOk ) {
    mExecutingLock = false;
    mExecuting = false;
    kDebug() << "Stopping filtering, error or end of message list.";
    finishTimer->start( 0 );
    return;
  }

  MessageProperty::setFiltering( *mMessageIt, true );
  MessageProperty::setFilterHandler( *mMessageIt, this );
  MessageProperty::setFilterFolder( *mMessageIt, mDestFolder );
  if ( FilterLog::instance()->isLogging() ) {
    FilterLog::instance()->addSeparator();
  }
  mFilterIt = mFilters.begin();

  if ( (*mMessageIt).isValid() )
  {
    // We have a complete message or
    // we can work with an incomplete message
    // Get a write lock on the message while it's being filtered
    filterMessageTimer->start( 0 );
    return;
  } else {
    mExecuting = false;
    mResult = ResultError;
    finishTimer->start( 0 );
    return;
  }
}

void ActionScheduler::filterMessage()
{
  if (mFilterIt == mFilters.end()) {
    moveMessage();
    return;
  }
#if 0 //TODO port to akonadi
  if ( mIgnoreFilterSet ||
      ((((mSet & KMFilterMgr::Outbound) && (*mFilterIt)->applyOnOutbound()) ||
      ((mSet & KMFilterMgr::Inbound) && (*mFilterIt)->applyOnInbound() &&
       (!mAccount ||
        (mAccount && (*mFilterIt)->applyOnAccount(mAccountId)))) ||
      ((mSet & KMFilterMgr::Explicit) && (*mFilterIt)->applyOnExplicit()))))
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  {
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
    if ( mMessageIt->isValid() ) {
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
      action->processAsync( *mMessageIt );
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
  if ( mMessageIt->hasPayload<KMime::Message::Ptr>() )
    return;
  const KMime::Message::Ptr msg = mMessageIt->payload<KMime::Message::Ptr>();

  Akonadi::Collection folder = MessageProperty::filterFolder( *mMessageIt );
#if 0 // TODO port to Akonadi
  QString serNumS = msg->headerField( "X-KMail-Filtered" );
  if (!serNumS.isEmpty())
    mOriginalSerNum = serNumS.toUInt();
  else
    mOriginalSerNum = 0;
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  MessageProperty::setFilterHandler( *mMessageIt, 0 );
  MessageProperty::setFiltering( *mMessageIt, false );

  KMime::Message::Ptr orgMsg;
  ReturnCode mOldReturnCode = mResult;
  if ( mOriginalItem.hasPayload<KMime::Message::Ptr>() )
    orgMsg = mOriginalItem.payload<KMime::Message::Ptr>();
  mResult = mOldReturnCode; // ignore errors in deleting original message
  if (!orgMsg || !orgMsg->parent()) {
    // Original message is gone, no point filtering it anymore
#if 0 // TODO port to Akonadi
    mSrcFolder->removeMsg( mSrcFolder->find( msg ) );
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif

    kDebug() << "The original serial number is missing."
                 << "Cannot complete the filtering.";
    mExecutingLock = false;
    processMessageTimer->start( 0 );
    return;
  } else {
    if (!folder.isValid()) // no filter folder specified leave in current place
      folder = mOriginalItem.parentCollection();
  }

  assert( mMessageIt->parentCollection() == mSrcFolder );
#if 0 // TODO port to Akonadi
  mSrcFolder->take( mSrcFolder->find( msg ) );
  mSrcFolder->addMsg( msg );
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif

  if ( mMessageIt->isValid() && folder.isValid() && kmkernel->folderIsTrash( folder ))
    KMFilterAction::sendMDN( msg, KMime::MDN::Deleted );

  timeOutTime = QTime::currentTime();
  KMCommand *cmd = new KMMoveCommand( folder, *mMessageIt );
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
  ReturnCode mOldReturnCode = mResult;
  if ( mOriginalItem.isValid() ) {
    emit filtered( mOriginalItem );
  }

  mResult = mOldReturnCode; // ignore errors in deleting original message

  // Delete the original message, because we have just moved the filtered message
  // from the temporary filtering folder to the target folder, and don't need the
  // old unfiltered message there anymore.
  KMCommand *cmd = 0;
  if ( mOriginalItem.isValid() && !movingFailed ) {
    cmd = new KMMoveCommand( Akonadi::Collection(), mOriginalItem );
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

  if ( mOriginalItem.isValid() ) // Try again
      execFilters( mOriginalItem );
}

QString ActionScheduler::debug()
{
  QString res;
  QList<ActionScheduler*>::iterator it;
  int i = 1;
  for ( it = schedulerList->begin(); it != schedulerList->end(); ++it ) {
    res.append( QString( "ActionScheduler #%1.\n" ).arg( i ) );
#if 0
    if ((*it)->mAccount && kmkernel->acctMgr()->find( (*it)->mAccountId )) {
      res.append( QString( "Account %1, Name %2.\n" )
          .arg( (*it)->mAccountId )
          .arg( kmkernel->acctMgr()->find( (*it)->mAccountId )->name() ) );
    }
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
    res.append( QString( "mExecuting %1, " ).arg( (*it)->mExecuting ? "true" : "false" ) );
    res.append( QString( "mExecutingLock %1, " ).arg( (*it)->mExecutingLock ? "true" : "false" ) );
    res.append( QString( "mFetchExecuting %1.\n" ).arg( (*it)->mFetchExecuting ? "true" : "false" ) );
    res.append( QString( "mOriginalItem %1.\n" ).arg( (*it)->mOriginalItem.id() ) );
    res.append( QString( "mItems count %1, " ).arg( (*it)->mItems.count() ) );
    res.append( QString( "mFetchItems count %1.\n" ).arg( (*it)->mFetchItems.count() ) );
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
  KSharedConfig::Ptr config = KMKernel::config();
  KConfigGroup group(config, "General");
  sEnabled = group.readEntry("action-scheduler", false );
  return sEnabled;
}

void ActionScheduler::addMsgToIgnored( const Akonadi::Item &item )
{
  kDebug() << "Adding ignored message:" << item.id();
  mIgnoredItems.append( item );
}

#include "actionscheduler.moc"
