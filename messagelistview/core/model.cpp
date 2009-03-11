/******************************************************************************
 *
 *  Copyright 2008 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

//
// This class is a rather huge monster. It's something that resembles a QAbstractItemModel
// (because it has to provide the interface for a QTreeView) but isn't entirely one
// (for optimization reasons). It basically manages a tree of items of two types:
// GroupHeaderItem and MessageItem. Be sure to read the docs for ViewItemJob.
//
// A huge credit here goes to Till Adam which seems to have written most
// (if not all) of the original KMail threading code. The KMHeaders implementation,
// the documentation and his clever ideas were my starting points and essential tools.
// This is why I'm adding his copyright entry (copied from headeritem.cpp) here even if
// he didn't write a byte in this file until now :)
//
//                                       Szymon Tomasz Stefanek, 03 Aug 2008 04:50 (am)
//
// This class contains ideas from:
//
//   kmheaders.cpp / kmheaders.h, headeritem.cpp / headeritem.h
//   Copyright: (c) 2004 Till Adam < adam at kde dot org >
//

#include "messagelistview/core/model.h"
#include "messagelistview/core/view.h"
#include "messagelistview/core/filter.h"
#include "messagelistview/core/groupheaderitem.h"
#include "messagelistview/core/messageitem.h"
#include "messagelistview/core/modelinvariantrowmapper.h"
#include "messagelistview/core/storagemodelbase.h"
#include "messagelistview/core/theme.h"
#include "messagelistview/core/manager.h"
#include "messagelistview/core/messageitemsetmanager.h"

#include <libkdepim/broadcaststatus.h> // KPIM::BroadcastStatus
#include <libkdepim/messagestatus.h>

#include <QTimer>
#include <QDateTime>
#include <QScrollBar>

#include <KLocale>
#include <KDebug>
#include <kcursorsaver.h>

namespace KMail
{

namespace MessageListView
{

namespace Core
{

/**
 * A job in a "View Fill" or "View Cleanup" or "View Update" task.
 *
 * For a "View Fill" task a job is a set of messages
 * that are contiguous in the storage. The set is expressed as a range
 * of row indexes. The task "sweeps" the storage in the specified
 * range, creates the appropriate Item instances and places them
 * in the right position in the tree.
 *
 * The idea is that in a single instance and for the same StorageModel
 * the jobs should never "cover" the same message twice. This assertion
 * is enforced all around this source file.
 *
 * For a "View Cleanup" task the job is a list of ModelInvariantIndex
 * objects (that are in fact MessageItem objects) that need to be removed
 * from the view.
 *
 * For a "View Update" task the job is a list of ModelInvariantIndex
 * objects (that are in fact MessageItem objects) that need to be updated.
 *
 * The interesting fact is that all the tasks need
 * very similar operations to be performed on the message tree.
 *
 * For a "View Fill" we have 5 passes.
 *
 * Pass 1 scans the underlying storage, creates the MessageItem objects
 * (which are subclasses of ModelInvariantIndex) and retrieves invariant
 * storage indexes for them. It also builds threading caches and
 * attempts to do some "easy" threading. If it succeeds in threading
 * and some conditions apply then it also attaches the items to the view.
 * Any unattached message is placed in a list.
 *
 * Pass 2 scans the list of messages that haven't been attached in
 * the first pass and performs perfect and reference based threading.
 * Since grouping of messages may depend on the "shape" of the thread
 * then certain threads aren't attacched to the view yet.
 * Unassigned messages get stuffed into a list waiting for Pass3
 * or directly to a list waiting for Pass4 (that is, Pass3 may be skipped
 * if there is no hope to find an imperfect parent by subject based threading).
 *
 * Pass 3 scans the list of messages that haven't been attached in
 * the first and second passes and performs subject based threading.
 * Since grouping of messages may depend on the "shape" of the thread
 * then certain threads aren't attacched to the view yet.
 * Anything unattached gets stuffed into the list waiting for Pass4.
 *
 * Pass 4 scans the unattached threads and puts them in the appropriate
 * groups. After this pass nothing is unattached.
 *
 * Pass 5 eventually re-sorts the groups and removes the empty ones.
 *
 * For a "View Cleanup" we still have 5 passes.
 *
 * Pass 1 scans the list of invalidated ModelInvariantIndex-es, casts
 * them to MessageItem objects and detaches them from the view.
 * The orphan children of the destroyed items get stuffed in the list
 * of unassigned messages that has been used also in the "View Fill" task above.
 *
 * Pass 2, 3, 4 and 5: same as "View Fill", just operating on the "orphaned"
 * messages that need to be reattached to the view.
 *
 * For a "View Update" we still have 5 passes.
 *
 * Pass 1 scans the list of ModelInvariantIndex-es that need an update, casts
 * them to MessageItem objects and handles the updates from storage.
 * The updates may cause a regrouping so items might be stuffed in one
 * of the lists for pass 4 or 5.
 *
 * Pass 2, 3 and 4 are simply empty.
 *
 * Pass 5: same as "View Fill", just operating on groups that require updates
 * after the messages have been moved in pass 1.
 *
 * That's why we in fact have Pass1Fill, Pass1Cleanup, Pass1Update, Pass2, Pass3, Pass4 and Pass5 below.
 * Pass1Fill, Pass1Cleanup and Pass1Update are exclusive and all of them proceed with Pass2 when finished.
 */
class ViewItemJob
{
public:
  enum Pass
  {
    Pass1Fill,            ///< Build threading caches, *TRY* to do some threading, try to attach something to the view
    Pass1Cleanup,         ///< Kill messages, build list of orphans
    Pass1Update,          ///< Update messages
    Pass2,                ///< Thread everything by using caches, try to attach more to the view
    Pass3,                ///< Do more threading (this time try to guess), try to attach more to the view
    Pass4,                ///< Attach anything is still unattacched
    Pass5                 ///< Eventually Re-sort group headers and remove the empty ones
  };
private:
  // Data for "View Fill" jobs
  int mStartIndex;        ///< The first index (in the underlying storage) of this job
  int mCurrentIndex;      ///< The current index (in the underlying storage) of this job
  int mEndIndex;          ///< The last index (in the underlying storage) of this job

  // Data for "View Cleanup" jobs
  QList< ModelInvariantIndex * > * mInvariantIndexList; ///< Owned list of shallow pointers

  // Common data

  // The maximum time that we can spend "at once" inside viewItemJobStep() (milliseconds)
  // The bigger this value, the larger chunks of work we do at once and less the time
  // we loose in "breaking and resuming" the job. On the other side large values tend
  // to make the view less responsive up to a "freeze" perception if this value is larger
  // than 2000.
  int mChunkTimeout;

  // The interval between two fillView steps. The larger the interval, the more interactivity
  // we have. The shorter the interval the more work we get done per second.
  int mIdleInterval;

  // The minimum number of messages we process in every viewItemJobStep() call
  // The larger this value the less time we loose in checking the timeout every N messages.
  // On the other side, making this very large may make the view less responsive
  // if we're processing very few messages at a time and very high values (say > 10000) may
  // eventually make our job unbreakable until the end.
  int mMessageCheckCount;
  Pass mCurrentPass;

  // If this parameter is true then this job uses a "disconnected" UI.
  // It's FAR faster since we don't need to call beginInsertRows()/endInsertRows()
  // and we simply emit a layoutChanged() at the end. It can be done only as the first
  // job though: subsequent jobs can't use layoutChanged() as it looses the expanded
  // state of items.
  bool mDisconnectUI;
public:
  /**
   * Creates a "View Fill" operation job
   */
  ViewItemJob( int startIndex, int endIndex, int chunkTimeout, int idleInterval, int messageCheckCount, bool disconnectUI = false )
    : mStartIndex( startIndex ), mCurrentIndex( startIndex ), mEndIndex( endIndex ),
      mInvariantIndexList( 0 ),
      mChunkTimeout( chunkTimeout ), mIdleInterval( idleInterval ),
      mMessageCheckCount( messageCheckCount ), mCurrentPass( Pass1Fill ),
      mDisconnectUI( disconnectUI ) {};

  /**
   * Creates a "View Cleanup" or "View Update" operation job
   */
  ViewItemJob( Pass pass, QList< ModelInvariantIndex * > * invariantIndexList, int chunkTimeout, int idleInterval, int messageCheckCount )
    : mStartIndex( 0 ), mCurrentIndex( 0 ), mEndIndex( invariantIndexList->count() - 1 ),
      mInvariantIndexList( invariantIndexList ),
      mChunkTimeout( chunkTimeout ), mIdleInterval( idleInterval ),
      mMessageCheckCount( messageCheckCount ), mCurrentPass( pass ),
      mDisconnectUI( false ) {};

  ~ViewItemJob()
  {
    if ( mInvariantIndexList )
      delete mInvariantIndexList;
  }
public:
  int startIndex() const
    { return mStartIndex; };
  void setStartIndex( int startIndex )
    { mStartIndex = startIndex; mCurrentIndex = startIndex; };
  int currentIndex() const
    { return mCurrentIndex; };
  void setCurrentIndex( int currentIndex )
    { mCurrentIndex = currentIndex; };
  int endIndex() const
    { return mEndIndex; };
  void setEndIndex( int endIndex )
    { mEndIndex = endIndex; };
  Pass currentPass() const
    { return mCurrentPass; };
  void setCurrentPass( Pass pass )
    { mCurrentPass = pass; };
  int idleInterval() const
    { return mIdleInterval; };
  int chunkTimeout() const
    { return mChunkTimeout; };
  int messageCheckCount() const
    { return mMessageCheckCount; };
  QList< ModelInvariantIndex * > * invariantIndexList() const
    { return mInvariantIndexList; };
  bool disconnectUI() const
    { return mDisconnectUI; };
};


Model::Model( View *pParent )
  : QAbstractItemModel( pParent ),
    mStorageModel( 0 ), mView( pParent )
{
  mAggregation = 0;
  mTheme = 0;
  mSortOrder = 0;
  mFilter = 0;
  mPersistentSetManager = 0;
  mInLengthyJobBatch = false;
  mUniqueIdOfLastSelectedMessageInFolder = 0;
  mLastSelectedMessageInFolder = 0;
  mLoading = false;

  mRootItem = new Item( Item::InvisibleRoot );
  mRootItem->setViewable( 0, true );

  mViewItemJobs = new QList< ViewItemJob * >();
  mGroupHeaderItemHash = new QHash< QString, GroupHeaderItem * >();
  mGroupHeadersThatNeedUpdate = new QHash< GroupHeaderItem *, GroupHeaderItem * >();
  mThreadingCacheMessageIdMD5ToMessageItem = new QHash< QString, MessageItem * >();
  mThreadingCacheMessageInReplyToIdMD5ToMessageItem = new QMultiHash< QString, MessageItem * >();
  mThreadingCacheMessageSubjectMD5ToMessageItem = new QHash< QString, QList< MessageItem * > * >();
  mUnassignedMessageListForPass2 = new QList< MessageItem * >();
  mUnassignedMessageListForPass3 = new QList< MessageItem * >();
  mUnassignedMessageListForPass4 = new QList< MessageItem * >();
  mOrphanChildrenHash = new QHash< MessageItem *, MessageItem * >();
  mFillStepTimer = new QTimer( this );
  mFillStepTimer->setSingleShot( true );
  mInvariantRowMapper = new ModelInvariantRowMapper();
  mModelForItemFunctions= this;
  connect( mFillStepTimer, SIGNAL( timeout() ),
           SLOT( viewItemJobStep() ) );

  mCachedTodayLabel = i18n( "Today" );
  mCachedYesterdayLabel = i18n( "Yesterday" );
  mCachedUnknownLabel = i18nc( "Unknown date",
                               "Unknown" );
  mCachedLastWeekLabel = i18n( "Last Week" );
  mCachedTwoWeeksAgoLabel = i18n( "Two Weeks Ago" );
  mCachedThreeWeeksAgoLabel = i18n( "Three Weeks Ago" );
  mCachedFourWeeksAgoLabel = i18n( "Four Weeks Ago" );

  mCachedWatchedOrIgnoredStatusBits = KPIM::MessageStatus::statusIgnored().toQInt32() | KPIM::MessageStatus::statusWatched().toQInt32();
  mCachedNewStatusBits = KPIM::MessageStatus::statusNew().toQInt32();
}

Model::~Model()
{
  setStorageModel( 0 );
  clearJobList();
  delete mViewItemJobs;
  clearUnassignedMessageLists();
  delete mUnassignedMessageListForPass2;
  delete mUnassignedMessageListForPass3;
  delete mUnassignedMessageListForPass4;
  clearOrphanChildrenHash();
  delete mOrphanChildrenHash;
  delete mGroupHeaderItemHash;
  delete mGroupHeadersThatNeedUpdate;
  delete mThreadingCacheMessageIdMD5ToMessageItem;
  delete mThreadingCacheMessageInReplyToIdMD5ToMessageItem;
  void clearThreadingCacheMessageSubjectMD5ToMessageItem();
  delete mThreadingCacheMessageSubjectMD5ToMessageItem;
  if ( mPersistentSetManager )
    delete mPersistentSetManager;
  // Delete the invariant row mapper before removing the items.
  // It's faster since the items will not need to call the invariant
  delete mInvariantRowMapper;
  delete mRootItem;
}

void Model::setAggregation( const Aggregation * aggregation )
{
  mAggregation = aggregation;
  mView->setRootIsDecorated( mAggregation->grouping() == Aggregation::NoGrouping );
}

void Model::setTheme( const Theme * theme )
{
  mTheme = theme;
}

void Model::setSortOrder( const SortOrder * sortOrder )
{
  mSortOrder = sortOrder;
}

void Model::setFilter( const Filter *filter )
{
  mFilter = filter;

  QList< Item * > * childList = mRootItem->childItems();
  if ( !childList )
    return;

  QModelIndex idx; // invalid

  KCursorSaver busy( KBusyPtr::busy() );

  for ( QList< Item * >::Iterator it = childList->begin(); it != childList->end(); ++it )
    applyFilterToSubtree( *it, idx );
}

bool Model::applyFilterToSubtree( Item * item, const QModelIndex &parentIndex )
{
  // This function applies the current filter (eventually empty)
  // to a message tree starting at "item".

  Q_ASSERT( mModelForItemFunctions );  // The UI must be not disconnected
  Q_ASSERT( item );                    // the item must obviously be valid
  Q_ASSERT( item->isViewable() );      // the item must be viewable

  // Apply to children first

  QList< Item * > * childList = item->childItems();

  bool childrenMatch = false;

  QModelIndex thisIndex = index( item, 0 );

  if ( childList )
  {
    for ( QList< Item * >::Iterator it = childList->begin(); it != childList->end(); ++it )
    {
      if ( applyFilterToSubtree( *it, thisIndex ) )
        childrenMatch = true;
    }
  }

  if ( !mFilter ) // empty filter always matches (but does not expand items)
  {
    mView->setRowHidden( thisIndex.row(), parentIndex, false );
    return true;
  }

  if ( childrenMatch )
  {
    mView->setRowHidden( thisIndex.row(), parentIndex, false );
#if 0
    // Expanding parents of matching items is an EXTREMELY desiderable feature... but...
    //
    // FIXME TrollTech: THIS IS PATHETICALLY SLOW
    //                  It can take ~20 minutes on a tree with ~11000 items.
    //                  Without this call the same tree is scanned in a couple of seconds.
    //                  The complexity growth is almost certainly (close to) exponential.
    //
    // It ends up in _very_ deep recursive stacks like these:
    //
    // #0  0x00002b37e1e03f03 in QTreeViewPrivate::viewIndex (this=0xbd9ff0, index=@0x7fffd327a420) at itemviews/qtreeview.cpp:3195
    // #1  0x00002b37e1e07ea6 in QTreeViewPrivate::layout (this=0xbd9ff0, i=8239) at itemviews/qtreeview.cpp:3013
    // #2  0x00002b37e1e0810e in QTreeViewPrivate::layout (this=0xbd9ff0, i=8238) at itemviews/qtreeview.cpp:2994
    // #3  0x00002b37e1e0810e in QTreeViewPrivate::layout (this=0xbd9ff0, i=8237) at itemviews/qtreeview.cpp:2994
    // #4  0x00002b37e1e0810e in QTreeViewPrivate::layout (this=0xbd9ff0, i=8236) at itemviews/qtreeview.cpp:2994
    // #5  0x00002b37e1e0810e in QTreeViewPrivate::layout (this=0xbd9ff0, i=8235) at itemviews/qtreeview.cpp:2994
    // #6  0x00002b37e1e0810e in QTreeViewPrivate::layout (this=0xbd9ff0, i=8234) at itemviews/qtreeview.cpp:2994
    // #7  0x00002b37e1e0810e in QTreeViewPrivate::layout (this=0xbd9ff0, i=8233) at itemviews/qtreeview.cpp:2994
    // #8  0x00002b37e1e0810e in QTreeViewPrivate::layout (this=0xbd9ff0, i=8232) at itemviews/qtreeview.cpp:2994
    // #9  0x00002b37e1e0810e in QTreeViewPrivate::layout (this=0xbd9ff0, i=8231) at itemviews/qtreeview.cpp:2994
    // #10 0x00002b37e1e0810e in QTreeViewPrivate::layout (this=0xbd9ff0, i=8230) at itemviews/qtreeview.cpp:2994
    // #11 0x00002b37e1e0810e in QTreeViewPrivate::layout (this=0xbd9ff0, i=8229) at itemviews/qtreeview.cpp:2994
    // #12 0x00002b37e1e0810e in QTreeViewPrivate::layout (this=0xbd9ff0, i=8228) at itemviews/qtreeview.cpp:2994
    // #13 0x00002b37e1e0810e in QTreeViewPrivate::layout (this=0xbd9ff0, i=8227) at itemviews/qtreeview.cpp:2994
    // #14 0x00002b37e1e0810e in QTreeViewPrivate::layout (this=0xbd9ff0, i=8226) at itemviews/qtreeview.cpp:2994
    // #15 0x00002b37e1e0810e in QTreeViewPrivate::layout (this=0xbd9ff0, i=8225) at itemviews/qtreeview.cpp:2994
    // #16 0x00002b37e1e0810e in QTreeViewPrivate::layout (this=0xbd9ff0, i=8224) at itemviews/qtreeview.cpp:2994
    // #17 0x00002b37e1e0810e in QTreeViewPrivate::layout (this=0xbd9ff0, i=8223) at itemviews/qtreeview.cpp:2994
    // ....
    //
    // UPDATE: Olivier Goffart seems to have fixed this: re-check and re-enable for qt 4.5.
    //

    if ( !mView->isExpanded( thisIndex ) )
      mView->expand( thisIndex );
#endif
    return true;
  }

  if ( item->type() == Item::Message )
  {
    if ( mFilter->match( ( MessageItem * )item ) )
    {
      mView->setRowHidden( thisIndex.row(), parentIndex, false );
      return true;
    }
  } // else this is a group header and it never explicitly matches

  // filter doesn't match, hide the item
  mView->setRowHidden( thisIndex.row(), parentIndex, true );

  return false;
}

int Model::columnCount( const QModelIndex & parent ) const
{
  if ( !mTheme )
    return 0;
  if ( parent.column() > 0 )
    return 0;
  return mTheme->columns().count();
}

QVariant Model::data( const QModelIndex &, int ) const
{
  // This is very inefficient: we don't use it actually
  return QVariant();
}

QVariant Model::headerData(int section, Qt::Orientation, int role) const
{
  if ( !mTheme )
    return QVariant();

  Theme::Column * column = mTheme->column( section );
  if ( !column )
    return QVariant();

 if ( role != Qt::DisplayRole )
    return QVariant();

  if ( mStorageModel && column->isSenderOrReceiver() )
  {
    if ( mStorageModel->containsOutboundMessages() )
      return QVariant( i18n( "Receiver" ) );
    return QVariant( i18n( "Sender" ) );
  }

  return QVariant( column->label() );
}

QModelIndex Model::index( Item *item, int column ) const
{
  Q_ASSERT( mModelForItemFunctions ); // should be never called with disconnected UI

  // FIXME: This function is a bottleneck
  Item * par = item->parent();
  if ( !par )
  {
    if ( item != mRootItem )
      item->dump(QString());
    return QModelIndex();
  }
  int indexGuess = item->indexGuess();
  if ( par->childItemHasIndex( item, indexGuess ) ) // This is 30% of the bottleneck
    return createIndex( indexGuess, column, item );

  indexGuess = par->indexOfChildItem( item ); // This is 60% of the bottleneck
  if ( indexGuess < 0 )
    return QModelIndex(); // BUG

  item->setIndexGuess( indexGuess );
  return createIndex( indexGuess, column, item );
}

QModelIndex Model::index( int row, int column, const QModelIndex &parent ) const
{
  if ( !mModelForItemFunctions )
    return QModelIndex(); // called with disconnected UI

#ifdef READD_THIS_IF_YOU_WANT_TO_PASS_MODEL_TEST
  if ( column < 0 )
    return QModelIndex(); // senseless column (we could optimize by skipping this check but ModelTest from trolltech is pedantic)
#endif

  const Item *item;
  if ( parent.isValid() )
  {
    item = static_cast< const Item * >( parent.internalPointer() );
    if ( !item )
      return QModelIndex(); // should never happen
  } else {
    item = mRootItem;
  }

  if ( parent.column() > 0 )
    return QModelIndex(); // parent column is not 0: shouldn't have children (as per Qt documentation)

  Item * child = item->childItem( row );
  if ( !child )
    return QModelIndex(); // no such row in parent
  return createIndex( row, column, child );
}

QModelIndex Model::parent( const QModelIndex &modelIndex ) const
{
  Q_ASSERT( mModelForItemFunctions ); // should be never called with disconnected UI

#ifdef READD_THIS_IF_YOU_WANT_TO_PASS_MODEL_TEST
  if ( !modelIndex.isValid() )
    return QModelIndex(); // should never happen
#endif
  Item *item = static_cast< Item * >( modelIndex.internalPointer() );
  if ( !item )
    return QModelIndex();
  Item *par = item->parent();
  if ( !par )
    return QModelIndex(); // should never happen
  //return index( par, modelIndex.column() );
  return index( par, 0 ); // parents are always in column 0 (as per Qt documentation)
}

int Model::rowCount( const QModelIndex &parent ) const
{
  if ( !mModelForItemFunctions )
    return 0; // called with disconnected UI

  const Item *item;
  if ( parent.isValid() )
  {
    item = static_cast< const Item * >( parent.internalPointer() );
    if ( !item )
      return 0; // should never happen
  } else {
    item = mRootItem;
  }

  if ( !item->isViewable() )
    return 0;

  return item->childItemCount();
}


void Model::setStorageModel( StorageModel *storageModel, PreSelectionMode preSelectionMode )
{
  if( mFillStepTimer->isActive() )
    mFillStepTimer->stop();

  if ( mStorageModel )
  {
    // Save the current selection
    MessageItem * lastSelectedMessageItem = mView->currentMessageItem( false );
    Manager::instance()->savePreSelectedMessageForStorageModel(
        mStorageModel,
        lastSelectedMessageItem ? lastSelectedMessageItem->uniqueId() : 0
      );
  }

  // Kill pre-selection at this stage
  mPreSelectionMode = PreSelectNone;
  mUniqueIdOfLastSelectedMessageInFolder = 0;
  mLastSelectedMessageInFolder = 0;

  // Reset the row mapper before removing items
  // This is faster since the items don't need to access the mapper.
  mInvariantRowMapper->modelReset();

  clearJobList();
  clearUnassignedMessageLists();
  clearOrphanChildrenHash();
  mGroupHeaderItemHash->clear();
  mGroupHeadersThatNeedUpdate->clear();
  mThreadingCacheMessageIdMD5ToMessageItem->clear();
  mThreadingCacheMessageInReplyToIdMD5ToMessageItem->clear();
  clearThreadingCacheMessageSubjectMD5ToMessageItem();
  mViewItemJobStepChunkTimeout = 100;
  mViewItemJobStepIdleInterval = 10;
  mViewItemJobStepMessageCheckCount = 10;
  if ( mPersistentSetManager )
  {
    delete mPersistentSetManager;
    mPersistentSetManager = 0;
  }
  mTodayDate = QDate::currentDate();

  if ( mStorageModel )
  {
    disconnect( mStorageModel, SIGNAL( rowsInserted( const QModelIndex &, int, int ) ),
                this, SLOT( slotStorageModelRowsInserted( const QModelIndex &, int, int ) ) );
    disconnect( mStorageModel, SIGNAL( rowsRemoved( const QModelIndex &, int, int ) ),
                this, SLOT( slotStorageModelRowsRemoved( const QModelIndex &, int, int ) ) );

    disconnect( mStorageModel, SIGNAL( layoutChanged() ),
                this, SLOT( slotStorageModelLayoutChanged() ) );
    disconnect( mStorageModel, SIGNAL( modelReset() ),
                this, SLOT( slotStorageModelLayoutChanged() ) );

    disconnect( mStorageModel, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ),
                this, SLOT( slotStorageModelDataChanged( const QModelIndex &, const QModelIndex & ) ) );
  }

  mRootItem->killAllChildItems();

  // FIXME: CLEAR THE FILTER HERE AS WE CAN'T APPLY IT WITH UI DISCONNECTED!

  mStorageModel = storageModel;

  reset();
  //emit headerDataChanged();

  mView->modelHasBeenReset();
  mView->selectionModel()->clearSelection();

  if ( !mStorageModel )
    return; // no folder: nothing to fill

  mPreSelectionMode = preSelectionMode;
  mUniqueIdOfLastSelectedMessageInFolder = Manager::instance()->preSelectedMessageForStorageModel( mStorageModel );

  connect( mStorageModel, SIGNAL( rowsInserted( const QModelIndex &, int, int ) ),
           this, SLOT( slotStorageModelRowsInserted( const QModelIndex &, int, int ) ) );
  connect( mStorageModel, SIGNAL( rowsRemoved( const QModelIndex &, int, int ) ),
           this, SLOT( slotStorageModelRowsRemoved( const QModelIndex &, int, int ) ) );

  connect( mStorageModel, SIGNAL( layoutChanged() ),
           this, SLOT( slotStorageModelLayoutChanged() ) );
  connect( mStorageModel, SIGNAL( modelReset() ),
           this, SLOT( slotStorageModelLayoutChanged() ) );

  connect( mStorageModel, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ),
           this, SLOT( slotStorageModelDataChanged( const QModelIndex &, const QModelIndex & ) ) );

  if ( mStorageModel->rowCount() == 0 )
    return; // folder empty: nothing to fill

  // Here we use different strategies based on user preference and the folder size.
  // The knobs we can tune are:
  //
  // - The number of jobs used to scan the whole folder and their order
  //
  //   There are basically two approaches to this. One is the "single big job"
  //   approach. It scans the folder from the beginning to the end in a single job
  //   entry. The job passes are done only once. It's advantage is that it's simplier
  //   and it's less likely to generate imperfect parent threadings. The bad
  //   side is that since the folders are "sort of" date ordered then the most interesting
  //   messages show up at the end of the work. Not nice for large folders.
  //   The other approach uses two jobs. This is a bit slower but smarter strategy.
  //   First we scan the latest 1000 messages and *then* take care of the older ones.
  //   This will show up the most interesting messages almost immediately. (Well...
  //   All this assuming that the underlying storage always appends the newly arrived messages)
  //   The strategy is slower since it  generates some imperfect parent threadings which must be
  //   adjusted by the second job. For instance, in my kernel mailing list folder this "smart" approach
  //   generates about 150 additional imperfectly threaded children... but the "today"
  //   messages show up almost immediately. The two-chunk job also makes computing
  //   the percentage user feedback a little harder and might break some optimization
  //   in the insertions (we're able to optimize appends and prepends but a chunked
  //   job is likely to split our work at a boundary where messages are always inserted
  //   in the middle of the list).
  //
  // - The maximum time to spend inside a single job step
  //
  //   The larger this time, the greater the number of messages per second that this
  //   engine can process but also greater time with frozen UI -> less interactivity.
  //   Reasonable values start at 50 msecs. Values larger than 300 msecs are very likely
  //   to be percieved by the user as UI non-reactivity.
  //
  // - The number of messages processed in each job step subchunk.
  //
  //   A job subchunk is processed without checking the maximum time above. This means
  //   that each job step will process at least the number of messages specified by this value.
  //   Very low values mean that we respect the maximum time very carefully but we also
  //   waste time to check if we ran out of time :)
  //   Very high values are likely to cause the engine to not respect the maximum step time.
  //   Reasonable values go from 5 to 100.
  //
  // - The "idle" time between two steps
  //
  //   The lower this time, the greater the number of messages per second that this
  //   engine can process but also lower time for the UI to process events -> less interactivity.
  //   A value of 0 here means that Qt will trigger the timer as soon as it has some
  //   idle time to spend. UI events will be still processed but slowdowns are possible.
  //   0 is reasonable though. Values larger than 200 will tend to make the total job
  //   completion times high.
  //

  // If we have no filter it seems that we can apply a huge optimization.
  // We disconnect the UI for the first huge filling job. This allows us
  // to save the extremely expensive beginInsertRows()/endInsertRows() calls
  // and call a single layoutChanged() at the end. This slows down a lot item
  // expansion. But on the other side if only few items need to be expanded
  // then this strategy is better. If filtering is enabled then this strategy
  // isn't applicable (because filtering requires interaction with the UI
  // while the data is loading).

  // So...

  // For the very first small chunk it's ok to work with disconnected UI as long
  // as we have no filter. The first small chunk is always 1000 messages, so
  // even if all of them are expanded, it's still somewhat acceptable.
  bool canDoFirstSmallChunkWithDisconnectedUI = !mFilter;

  // Larger works need a bigger condition: few messages must be expanded in the end.
  bool canDoJobWithDisconnectedUI =
          // we have no filter
          !mFilter &&
          (
            // we do no threading at all
            ( mAggregation->threading() == Aggregation::NoThreading ) ||
            // or we never expand threads
            ( mAggregation->threadExpandPolicy() == Aggregation::NeverExpandThreads ) ||
            // or we expand threads but we'll be going to expand really only a few
            (
              // so we don't expand them all
              ( mAggregation->threadExpandPolicy() != Aggregation::AlwaysExpandThreads ) &&
              // and we'd expand only a few in fact
              ( mStorageModel->initialUnreadRowCountGuess() < 1000 )
            )
          );

  switch ( mAggregation->fillViewStrategy() )
  {
    case Aggregation::FavorInteractivity:
      // favor interactivity
      if ( ( !canDoJobWithDisconnectedUI ) && ( mStorageModel->rowCount() > 3000 ) ) // empiric value
      {
        // First a small job with the most recent messages. Large chunk, small (but non zero) idle interval
        // and a larger number of messages to process at once.
        ViewItemJob * job1 = new ViewItemJob( mStorageModel->rowCount() - 1000, mStorageModel->rowCount() - 1, 200, 20, 100, canDoFirstSmallChunkWithDisconnectedUI );
        mViewItemJobs->append( job1 );
        // Then a larger job with older messages. Small chunk, bigger idle interval, small number of messages to
        // process at once.
        ViewItemJob * job2 = new ViewItemJob( 0, mStorageModel->rowCount() - 1001, 100, 50, 10, false );
        mViewItemJobs->append( job2 );

        // We could even extremize this by splitting the folder in several
        // chunks and scanning them from the newest to the oldest... but the overhead
        // due to imperfectly threaded children would be probably too big.
      } else {
        // small folder or can be done with disconnected UI: single chunk work.
        // Lag the CPU a bit more but not too much to destroy even the earliest interactivity.
        ViewItemJob * job = new ViewItemJob( 0, mStorageModel->rowCount() - 1, 150, 30, 30, canDoJobWithDisconnectedUI );
        mViewItemJobs->append( job );
      }
    break;
    case Aggregation::FavorSpeed:
      // More batchy jobs, still interactive to a certain degree
      if ( ( !canDoJobWithDisconnectedUI ) && ( mStorageModel->rowCount() > 3000 ) ) // empiric value
      {
        // large folder, but favor speed
        ViewItemJob * job1 = new ViewItemJob( mStorageModel->rowCount() - 1000, mStorageModel->rowCount() - 1, 250, 0, 100, canDoFirstSmallChunkWithDisconnectedUI );
        mViewItemJobs->append( job1 );
        ViewItemJob * job2 = new ViewItemJob( 0, mStorageModel->rowCount() - 1001, 200, 0, 10, false );
        mViewItemJobs->append( job2 );
      } else {
        // small folder or can be done with disconnected UI and favor speed: single chunk work.
        // Lag the CPU more, get more work done
        ViewItemJob * job = new ViewItemJob( 0, mStorageModel->rowCount() - 1, 250, 0, 100, canDoJobWithDisconnectedUI );
        mViewItemJobs->append( job );
      }
    break;
    case Aggregation::BatchNoInteractivity:
    {
      // one large job, never interrupt, block UI
      ViewItemJob * job = new ViewItemJob( 0, mStorageModel->rowCount() - 1, 60000, 0, 100000, canDoJobWithDisconnectedUI );
      mViewItemJobs->append( job );
    }
    break;
    default:
      kWarning() << "Unrecognized fill view strategy" << endl;
      Q_ASSERT( false );
    break;
  }

  mLoading = true;

  viewItemJobStep();
}

void Model::checkIfDateChanged()
{
  // This function is called by MessageListView::Core::Manager once in a while (every 1 minute or sth).
  // It is used to check if the current date has changed (with respect to mTodayDate).
  //
  // Our message items cache the formatted dates (as formatting them
  // on the fly would be too expensive). We also cache the labels of the groups which often display dates.
  // When the date changes we would need to fix all these strings.
  //
  // A dedicated algorithm to refresh the labels of the items would be either too complex
  // or would block on large trees. Fixing the labels of the groups is also quite hard...
  //
  // So to keep the things simple we just reload the view.

  if ( !mStorageModel )
    return; // nothing to do

  if ( mLoading )
    return; // not now

  if ( !mViewItemJobs->isEmpty() )
    return; // not now

  if ( mTodayDate.day() == QDate::currentDate().day() )
    return; // date not changed

  // date changed, reload the view (and try to preserve the current selection)
  setStorageModel( mStorageModel, PreSelectLastSelected );
}


void Model::abortMessagePreSelection()
{
  // This is used to abort a message pre-selection before we actually could apply it.
  mPreSelectionMode = PreSelectNone;
  mUniqueIdOfLastSelectedMessageInFolder = 0;
  mLastSelectedMessageInFolder = 0;
}

void Model::applyMessagePreSelection( PreSelectionMode preSelectionMode )
{
  if ( mLoading )
  {
    // override the current mode
    mPreSelectionMode = preSelectionMode;
    // will be applied when loading completes
    return;
  }

  switch( preSelectionMode )
  {
    case PreSelectFirstUnread:
      mView->selectFirstMessageItem( MessageTypeUnreadOnly, false ); // don't center
    break;
    case PreSelectFirstUnreadCentered:
      mView->selectFirstMessageItem( MessageTypeUnreadOnly, true ); // center
    break;
    case PreSelectFirstNew:
      if ( !mView->selectFirstMessageItem( MessageTypeNewOnly, false ) ) // don't center
        mView->selectFirstMessageItem( MessageTypeUnreadOnly, false ); // try to fallback to unread
    break;
    case PreSelectFirstNewCentered:
      if ( !mView->selectFirstMessageItem( MessageTypeNewOnly, true ) ) // center
        mView->selectFirstMessageItem( MessageTypeUnreadOnly, true ); // try to fallback to unread
    break;
    case PreSelectFirstNewOrUnread:
      mView->selectFirstMessageItem( MessageTypeNewOrUnreadOnly, false ); // don't center
    break;
    case PreSelectFirstNewOrUnreadCentered:
      mView->selectFirstMessageItem( MessageTypeNewOrUnreadOnly, true ); // center
    break;
    case PreSelectLastSelected:
      // has no meaning in this state
    break;
    case PreSelectNone:
      // explicitly ignored
    break;
    default:
      kWarning() << "Unrecognized pre-selection mode " << (int)preSelectionMode;
    break;
  }
}


//
// The "view fill" algorithm implemented in the functions below is quite smart but also quite complex.
// It's governed by the following goals:
//
// - Be flexible: allow different configurations from "unsorted flat list" to a "grouped and threaded
//     list with different sorting algorightms applied to each aggregation level"
// - Be reasonably fast
// - Be non blocking: UI shouldn't freeze while the algorithm is running
// - Be interruptible: user must be able to abort the execution and just switch to another folder in the middle
//

void Model::clearUnassignedMessageLists()
{
  // This is a bit tricky...
  // The three unassigned message lists contain messages that have been created
  // but not yet attached to the view. There may be two major cases for a message:
  // - it has no parent -> it must be deleted and it will delete its children too
  // - it has a parent -> it must NOT be deleted since it will be deleted by its parent.

  // Sometimes the things get a little complicated since in Pass2 and Pass3
  // we have transitional states in that the MessageItem object can be in two of these lists.

  QList< MessageItem * >::Iterator it;

  if ( !mUnassignedMessageListForPass2->isEmpty() )
  {
    // We're actually in Pass1* or Pass2: everything is mUnassignedMessageListForPass2
    // Something may *also* be in mUnassignedMessageListForPass3 and mUnassignedMessageListForPass4
    // but that are duplicates for sure.

    // We can't just sweep the list and delete parentless items since each delete
    // could kill children which are somewhere AFTER in the list: accessing the children
    // would then lead to a SIGSEGV. We first sweep the list gathering parentless
    // items and *then* delete them without accessing the parented ones.

    QList< MessageItem * > parentless;

    for ( it = mUnassignedMessageListForPass2->begin(); it != mUnassignedMessageListForPass2->end(); ++it )
    {
      if( !( *it )->parent() )
        parentless.append( *it );
    }

    for ( it = parentless.begin(); it != parentless.end(); ++it )
      delete *it;

    mUnassignedMessageListForPass2->clear();
    // Any message these list contain was also in mUnassignedMessageListForPass2
    mUnassignedMessageListForPass3->clear();
    mUnassignedMessageListForPass4->clear();
    return;
  }

  // mUnassignedMessageListForPass2 is empty

  if ( !mUnassignedMessageListForPass3->isEmpty() )
  {
    // We're actually at the very end of Pass2 or inside Pass3
    // Pass2 pushes stuff in mUnassignedMessageListForPass3 *or* mUnassignedMessageListForPass4
    // Pass3 pushes stuff from mUnassignedMessageListForPass3 to mUnassignedMessageListForPass4
    // So if we're in Pass2 then the two lists contain distinct messages but if we're in Pass3
    // then the two lists may contain the same messages.

    if ( !mUnassignedMessageListForPass4->isEmpty() )
    {
      // We're actually in Pass3: the messiest one.

      QHash< MessageItem *, MessageItem * > itemsToDelete;

      for ( it = mUnassignedMessageListForPass3->begin(); it != mUnassignedMessageListForPass3->end(); ++it )
      {
        if( !( *it )->parent() )
          itemsToDelete.insert( *it, *it );
      }

      for ( it = mUnassignedMessageListForPass4->begin(); it != mUnassignedMessageListForPass4->end(); ++it )
      {
        if( !( *it )->parent() )
          itemsToDelete.insert( *it, *it );
      }

      for ( QHash< MessageItem *, MessageItem * >::Iterator it3 = itemsToDelete.begin(); it3 != itemsToDelete.end(); ++it3 )
        delete ( *it3 );

      mUnassignedMessageListForPass3->clear();
      mUnassignedMessageListForPass4->clear();
      return;
    }

    // mUnassignedMessageListForPass4 is empty so we must be at the end of a very special kind of Pass2
    // We have the same problem as in mUnassignedMessageListForPass2.
    QList< MessageItem * > parentless;

    for ( it = mUnassignedMessageListForPass3->begin(); it != mUnassignedMessageListForPass3->end(); ++it )
    {
      if( !( *it )->parent() )
        parentless.append( *it );
    }

    for ( it = parentless.begin(); it != parentless.end(); ++it )
      delete *it;

    mUnassignedMessageListForPass3->clear();
    return;
  }

  // mUnassignedMessageListForPass3 is empty
  if ( !mUnassignedMessageListForPass4->isEmpty() )
  {
    // we're in Pass4.. this is easy.

    // We have the same problem as in mUnassignedMessageListForPass2.
    QList< MessageItem * > parentless;

    for (  it = mUnassignedMessageListForPass4->begin(); it != mUnassignedMessageListForPass4->end(); ++it )
    {
      if( !( *it )->parent() )
        parentless.append( *it );
    }

    for ( it = parentless.begin(); it != parentless.end(); ++it )
      delete *it;

    mUnassignedMessageListForPass4->clear();
    return;
  }
}

void Model::clearThreadingCacheMessageSubjectMD5ToMessageItem()
{
  qDeleteAll( *mThreadingCacheMessageSubjectMD5ToMessageItem );
  mThreadingCacheMessageSubjectMD5ToMessageItem->clear();
}

void Model::clearOrphanChildrenHash()
{
  for ( QHash< MessageItem *, MessageItem * >::Iterator it = mOrphanChildrenHash->begin(); it != mOrphanChildrenHash->end(); ++it )
  {
    //Q_ASSERT( !( *it )->parent() ); <-- this assert can actually fail for items that get a temporary parent assigned (to preserve the selection).
    delete ( *it );
  }
  mOrphanChildrenHash->clear();
}

void Model::clearJobList()
{
  if ( mViewItemJobs->isEmpty() )
    return;

  if ( mInLengthyJobBatch )
  {
    mInLengthyJobBatch = false;
    mView->modelJobBatchTerminated();
  }

  for( QList< ViewItemJob * >::Iterator it = mViewItemJobs->begin(); it != mViewItemJobs->end() ; ++it )
    delete ( *it );
  mViewItemJobs->clear();

  mModelForItemFunctions = this; // make sure it's true, as there remains no job with disconnected UI
}


void Model::attachGroup( GroupHeaderItem *ghi )
{
  if ( ghi->parent() )
  {
    if (
         ( ( ghi )->childItemCount() > 0 ) && // has children
         ( ghi )->isViewable() && // is actually attached to the viewable root
         mModelForItemFunctions && // the UI is not disconnected
         mView->isExpanded( index( ghi, 0 ) ) // is actually expanded
      )
      saveExpandedStateOfSubtree( ghi );

    // FIXME: This *WILL* break selection and current index... :/

    ghi->parent()->takeChildItem( mModelForItemFunctions, ghi );
  }

  ghi->setParent( mRootItem );

  // I'm using a macro since it does really improve readability.
  // I'm NOT using a helper function since gcc will refuse to inline some of
  // the calls because they make this function grow too much.
#define INSERT_GROUP_WITH_COMPARATOR( _ItemComparator ) \
      switch( mSortOrder->groupSortDirection() ) \
      { \
        case SortOrder::Ascending: \
          mRootItem->insertChildItem< _ItemComparator, true >( mModelForItemFunctions, ghi ); \
        break; \
        case SortOrder::Descending: \
          mRootItem->insertChildItem< _ItemComparator, false >( mModelForItemFunctions, ghi ); \
        break; \
        default: /* should never happen... */ \
          mRootItem->appendChildItem( mModelForItemFunctions, ghi ); \
        break; \
      }

  switch( mSortOrder->groupSorting() )
  {
    case SortOrder::SortGroupsByDateTime:
      INSERT_GROUP_WITH_COMPARATOR( ItemDateComparator )
    break;
    case SortOrder::SortGroupsByDateTimeOfMostRecent:
      INSERT_GROUP_WITH_COMPARATOR( ItemMaxDateComparator )
    break;
    case SortOrder::SortGroupsBySenderOrReceiver:
      INSERT_GROUP_WITH_COMPARATOR( ItemSenderOrReceiverComparator )
    break;
    case SortOrder::SortGroupsBySender:
      INSERT_GROUP_WITH_COMPARATOR( ItemSenderComparator )
    break;
    case SortOrder::SortGroupsByReceiver:
      INSERT_GROUP_WITH_COMPARATOR( ItemReceiverComparator )
    break;
    case SortOrder::NoGroupSorting:
      mRootItem->appendChildItem( mModelForItemFunctions, ghi );
    break;
    default: // should never happen
      mRootItem->appendChildItem( mModelForItemFunctions, ghi );
    break;
  }

  if ( ghi->initialExpandStatus() == Item::ExpandNeeded ) // this actually is a "non viewable expanded state"
    if ( ghi->childItemCount() > 0 )
      if ( mModelForItemFunctions ) // the UI is not disconnected
        syncExpandedStateOfSubtree( ghi );

  // A group header is always viewable, when attached: apply the filter, if we have it.
  if ( mFilter )
  {
    Q_ASSERT( mModelForItemFunctions ); // UI must be NOT disconnected
    // apply the filter to subtree
    applyFilterToSubtree( ghi, QModelIndex() );
  }
}

void Model::saveExpandedStateOfSubtree( Item *root )
{
  Q_ASSERT( mModelForItemFunctions ); // UI must be NOT disconnected here
  Q_ASSERT( root );

  root->setInitialExpandStatus( Item::ExpandNeeded );

  QList< Item * > * children = root->childItems();
  if ( !children )
    return;

  for( QList< Item * >::Iterator it = children->begin(); it != children->end(); ++it )
  {
    if (
         ( ( *it )->childItemCount() > 0 ) && // has children
         ( *it )->isViewable() && // is actually attached to the viewable root
         mView->isExpanded( index( *it, 0 ) ) // is actually expanded
       )
      saveExpandedStateOfSubtree( *it );
  }
}

void Model::syncExpandedStateOfSubtree( Item *root )
{
  Q_ASSERT( mModelForItemFunctions ); // UI must be NOT disconnected here

  // WE ASSUME that:
  // - the item is viewable
  // - its initialExpandStatus() is Item::ExpandNeeded
  // - it has at least one children (well.. this is not a strict requirement, but it's a waste of resources to expand items that don't have children)

  QModelIndex idx = index( root, 0 );

  //if ( !mView->isExpanded( idx ) ) // this is O(logN!) in Qt.... very ugly... but it should never happen here
  mView->expand( idx ); // sync the real state in the view
  root->setInitialExpandStatus( Item::ExpandExecuted );

  QList< Item * > * children = root->childItems();
  if ( !children )
    return;

  for( QList< Item * >::Iterator it = children->begin(); it != children->end(); ++it )
  {
    if ( ( *it )->initialExpandStatus() == Item::ExpandNeeded )
    {
      if ( ( *it )->childItemCount() > 0 )
        syncExpandedStateOfSubtree( *it );
    }
  }
}

static inline QString get_capitalized_long_day_name( int dayOfWeek )
{
  // The day name should be capitalized in the group labels
  // This fixes some complaints from translators that have mapped
  // their standard day names to lowercase versions for general
  // use but still want them to be capitalized in the headers...
  QString name = QDate::longDayName( dayOfWeek );
  if ( name.isEmpty() )
      return name;
  QString copy = name;
  copy[ 0 ] = name.at( 0 ).toUpper();
  return copy;
}

static inline QString get_capitalized_month_name( int month )
{
  // The month name should be capitalized in the group labels
  // This fixes some complaints from translators that have mapped
  // their standard month names to lowercase versions for general
  // use but still want them to be capitalized in the headers...
  QString name = QDate::longMonthName( month );
  if ( name.isEmpty() )
      return name;
  QString copy = name;
  copy[ 0 ] = name.at( 0 ).toUpper();
  return copy;
}

void Model::attachMessageToGroupHeader( MessageItem *mi )
{
  QString groupLabel;
  time_t date;

  // compute the group header label and the date
  switch( mAggregation->grouping() )
  {
    case Aggregation::GroupByDate:
    {
      QDateTime dt;
      switch ( mAggregation->threadLeader() )
      {
        case Aggregation::MostRecentMessage:
          date = mi->maxDate();
        break;
        case Aggregation::TopmostMessage:
          date = mi->date();
        break;
        default:
          // should never happen
          date = mi->date();
        break;
      }

      dt.setTime_t( date );

      QDate dDate = dt.date();
      if ( static_cast< uint >( date ) == static_cast< uint >( -1 ) )
      {
        groupLabel = mCachedUnknownLabel;
      } else if(
          ( dDate.day() == mTodayDate.day() ) &&
          ( dDate.month() == mTodayDate.month() ) &&
          ( dDate.year() == mTodayDate.year() )
        )
      {
        // today
        groupLabel = mCachedTodayLabel;
      } else if ( dDate.daysTo( mTodayDate ) == 1 )
      {
        // yesterday
        groupLabel = mCachedYesterdayLabel;
      } else if(
          ( dDate.month() == mTodayDate.month() ) &&
          ( dDate.year() == mTodayDate.year() )
        )
      {
        // within this month
        int todayWeekNumber = mTodayDate.weekNumber();
        int dateWeekNumber = dDate.weekNumber();
        if ( dateWeekNumber == todayWeekNumber )
        {
          // within this week
          groupLabel = get_capitalized_long_day_name( dDate.dayOfWeek() );
        } else {
          // not this week
          // FIXME: After 4.2 think about a configurable date format.
          //        At the moment KMime::DateFormatter doesn't support date-only formatting.
          //        KDateTime is not better than QDate in this case.
          //        A configurable date-only format should be probably tweaked into KMime::DateFormatter
          //        but this can't be done with the string freeze in effect.
          groupLabel = dDate.toString( Qt::DefaultLocaleShortDate );
        }
      } else {
        // not within this month
        // FIXME: After 4.2 think about a configurable date format.
        //        At the moment KMime::DateFormatter doesn't support date-only formatting.
        //        KDateTime is not better than QDate in this case.
        //        A configurable date-only format should be probably tweaked into KMime::DateFormatter
        //        but this can't be done with the string freeze in effect.
        groupLabel = dDate.toString( Qt::DefaultLocaleShortDate );
      }

    }
    break;
    case Aggregation::GroupByDateRange:
    {
      // "smart" date ranges
      QDateTime dt;
      switch ( mAggregation->threadLeader() )
      {
        case Aggregation::MostRecentMessage:
          date = mi->maxDate();
        break;
        case Aggregation::TopmostMessage:
          date = mi->date();
        break;
        default:
          // should never happen
          date = mi->date();
        break;
      }

      dt.setTime_t( date );

      QDate dDate = dt.date();
      if ( static_cast< uint >( date ) == static_cast< uint >( -1 ) )
      {
        groupLabel = mCachedUnknownLabel;
      } else if(
          ( dDate.day() == mTodayDate.day() ) &&
          ( dDate.month() == mTodayDate.month() ) &&
          ( dDate.year() == mTodayDate.year() )
        )
      {
        // today
        groupLabel = mCachedTodayLabel;
      } else if ( dDate.daysTo( mTodayDate ) == 1 )
      {
        // yesterday
        groupLabel = mCachedYesterdayLabel;
      } else if(
          ( dDate.month() == mTodayDate.month() ) &&
          ( dDate.year() == mTodayDate.year() )
        )
      {
        // within this month
        int todayWeekNumber = mTodayDate.weekNumber();
        int dateWeekNumber = dDate.weekNumber();
        if ( dateWeekNumber == todayWeekNumber )
        {
          // within this week
          groupLabel = get_capitalized_long_day_name( dDate.dayOfWeek() );
        } else {
          // previous weeks
          int weekDiff = todayWeekNumber - dateWeekNumber;
          switch( weekDiff )
          {
            case 1:
              groupLabel = mCachedLastWeekLabel;
            break;
            case 2:
              groupLabel = mCachedTwoWeeksAgoLabel;
            break;
            case 3:
              groupLabel = mCachedThreeWeeksAgoLabel;
            break;
            case 4:
              groupLabel = mCachedFourWeeksAgoLabel;
            break;
            default:
              groupLabel = mCachedUnknownLabel; // should never happen
            break;
          }
        }
      } else {
        if(
          ( dDate.year() == mTodayDate.year() )
        )
        {
          // group by months, this year (so no year appended)
          groupLabel = get_capitalized_month_name( dDate.month() );
        } else {
          // group by months
          groupLabel = QString( "%1 %2" ).arg( get_capitalized_month_name( dDate.month() ) ).arg( dDate.year() );
        }
      }
    }
    break;
    case Aggregation::GroupBySenderOrReceiver:
      date = mi->date();
      groupLabel = mi->senderOrReceiver();
    break;
    case Aggregation::GroupBySender:
      date = mi->date();
      groupLabel = mi->sender();
    break;
    case Aggregation::GroupByReceiver:
      date = mi->date();
      groupLabel = mi->receiver();
    break;
    case Aggregation::NoGrouping:
      // append directly to root
      attachMessageToParent( mRootItem, mi );
      return;
    break;
    default:
      // should never happen
      attachMessageToParent( mRootItem, mi );
      return;
    break;
  }

  GroupHeaderItem * ghi;

  ghi = mGroupHeaderItemHash->value( groupLabel, 0 );
  if( !ghi )
  {
    // not found

    ghi = new GroupHeaderItem( groupLabel );
    ghi->initialSetup( date, mi->size(), mi->sender(), mi->receiver(), mi->senderOrReceiver() );

    switch( mAggregation->groupExpandPolicy() )
    {
      case Aggregation::NeverExpandGroups:
        // nothing to do
      break;
      case Aggregation::AlwaysExpandGroups:
        // expand always
        ghi->setInitialExpandStatus( Item::ExpandNeeded );
      break;
      case Aggregation::ExpandRecentGroups:
        // expand only if "close" to today
        if ( mViewItemJobStepStartTime > ghi->date() )
        {
           if ( ( mViewItemJobStepStartTime - ghi->date() ) < ( 3600 * 72 ) )
             ghi->setInitialExpandStatus( Item::ExpandNeeded );
        } else {
           if ( ( ghi->date() - mViewItemJobStepStartTime ) < ( 3600 * 72 ) )
             ghi->setInitialExpandStatus( Item::ExpandNeeded );
        }
      break;
      default:
        // b0rken
      break;
    }

    attachMessageToParent( ghi, mi );

    attachGroup( ghi ); // this will expand the group if required

    mGroupHeaderItemHash->insert( groupLabel, ghi );
  } else {
    // the group was already there (certainly viewable)

    // This function may be also called to re-group a message.
    // That is, to eventually find a new group for a message that has changed
    // its properties (but was already attacched to a group).
    // So it may happen that we find out that in fact re-grouping wasn't really
    // needed because the message is already in the correct group.
    if ( mi->parent() == ghi )
      return; // nothing to be done

    attachMessageToParent( ghi, mi );
  }
}

MessageItem * Model::findMessageParent( MessageItem * mi )
{
  Q_ASSERT( mAggregation->threading() != Aggregation::NoThreading ); // caller must take care of this

  // This function attempts to find a thread parent for the item "mi"
  // which actually may already have a children subtree.

  // Forged or plain broken message trees are dangerous here.
  // For example, a message tree with circular references like
  //
  //        Message mi, Id=1, In-Reply-To=2
  //          Message childOfMi, Id=2, In-Reply-To=1
  //
  // is perfectly possible and will cause us to find childOfMi
  // as parent of mi. This will then create a loop in the message tree
  // (which will then no longer be a tree in fact) and cause us to freeze
  // once we attempt to climb the parents. We need to take care of that.

  bool bMessageWasThreadable = false;
  MessageItem * pParent;

  // First of all try to find a "perfect parent", that is the message for that
  // we have the ID in the "In-Reply-To" field. This is actually done by using
  // MD5 caches of the message ids because of speed. Collisions are very unlikely.

  QString md5 = mi->inReplyToIdMD5();
  if ( !md5.isEmpty() )
  {
    // have an In-Reply-To field MD5
    pParent = mThreadingCacheMessageIdMD5ToMessageItem->value( md5, 0 );
    if(pParent)
    {
      // Take care of circular references
      if (
           ( mi == pParent ) ||              // self referencing message
           (
             ( mi->childItemCount() > 0 ) && // mi already has children, this is fast to determine
             pParent->hasAncestor( mi )      // pParent is in the mi's children tree
           )
         )
      {
        kWarning() << "Circular In-Reply-To reference loop detected in the message tree" << endl;
        mi->setThreadingStatus( MessageItem::NonThreadable );
        return 0; // broken message: throw it away
      }
      mi->setThreadingStatus( MessageItem::PerfectParentFound );
      return pParent; // got a perfect parent for this message
    }

    // got no perfect parent
    bMessageWasThreadable = true; // but the message was threadable
  }

  if ( mAggregation->threading() == Aggregation::PerfectOnly )
  {
    mi->setThreadingStatus( bMessageWasThreadable ? MessageItem::ParentMissing : MessageItem::NonThreadable );
    return 0; // we're doing only perfect parent matches
  }

  // Try to use the "References" field. In fact we have the MD5 of the
  // (n-1)th entry in References.
  //
  // Original rationale from KMHeaders:
  //
  // If we don't have a replyToId, or if we have one and the
  // corresponding message is not in this folder, as happens
  // if you keep your outgoing messages in an OUTBOX, for
  // example, try the list of references, because the second
  // to last will likely be in this folder. replyToAuxIdMD5
  // contains the second to last one.

  md5 = mi->referencesIdMD5();
  if ( !md5.isEmpty() )
  {
    pParent = mThreadingCacheMessageIdMD5ToMessageItem->value( md5, 0 );
    if(pParent)
    {
      // Take care of circular references
      if (
           ( mi == pParent ) ||              // self referencing message
           (
             ( mi->childItemCount() > 0 ) && // mi already has children, this is fast to determine
             pParent->hasAncestor( mi )      // pParent is in the mi's children tree
           )
         )
      {
        kWarning() << "Circular reference loop detected in the message tree" << endl;
        mi->setThreadingStatus( MessageItem::NonThreadable );
        return 0; // broken message: throw it away
      }
      mi->setThreadingStatus( MessageItem::ImperfectParentFound );
      return pParent; // got an imperfect parent for this message
    }

    // got no imperfect parent
    bMessageWasThreadable = true; // but the message was threadable
  }

  if ( mAggregation->threading() == Aggregation::PerfectAndReferences )
  {
    mi->setThreadingStatus( bMessageWasThreadable ? MessageItem::ParentMissing : MessageItem::NonThreadable );
    return 0; // we're doing only perfect parent matches
  }

  Q_ASSERT( mAggregation->threading() == Aggregation::PerfectReferencesAndSubject );

  // We are supposed to do subject based threading but we can't do it now.
  // This is because the subject based threading *may* be wrong and waste
  // time by creating circular references (that we'd need to detect and fix).
  // We first try the perfect and references based threading on all the messages
  // and then run subject based threading only on the remaining ones.

  mi->setThreadingStatus( ( bMessageWasThreadable || mi->subjectIsPrefixed() ) ? MessageItem::ParentMissing : MessageItem::NonThreadable );
  return 0;
}

// Subject threading cache stuff

#if 0
// Debug helpers
void dump_iterator_and_list( QList< MessageItem * >::Iterator &iter, QList< MessageItem * > *list )
{
  kDebug() << "Threading cache part dump" << endl;
  if ( iter == list->end() )
    kDebug() << "Iterator pointing to end of the list" << endl;
  else
    kDebug() << "Iterator pointing to " << *iter << " subject [" << (*iter)->subject() << "] date [" << (*iter)->date() << "]" << endl;

  for ( QList< MessageItem * >::Iterator it = list->begin(); it != list->end(); ++it )
  {
    kDebug() << "List element " << *it << " subject [" << (*it)->subject() << "] date [" << (*it)->date() << "]" << endl;
  }

  kDebug() << "End of threading cache part dump" << endl;
}

void dump_list( QList< MessageItem * > *list )
{
  kDebug() << "Threading cache part dump" << endl;

  for ( QList< MessageItem * >::Iterator it = list->begin(); it != list->end(); ++it )
  {
    kDebug() << "List element " << *it << " subject [" << (*it)->subject() << "] date [" << (*it)->date() << "]" << endl;
  }

  kDebug() << "End of threading cache part dump" << endl;
}
#endif // debug helpers

// a helper class used in a qLowerBound() call below
class MessageLessThanByDate
{
public:
  inline bool operator()( const MessageItem * mi1, const MessageItem * mi2 ) const
  {
    return mi1->date() < mi2->date();
  }
};

void Model::addMessageToSubjectBasedThreadingCache( MessageItem * mi )
{
  // Unfortunately the entries in the cache can't be sorted by date. This is
  // because the date may be updated (and it would cause our lists to become unsorted).
  // Also "unknown" dates often popup so the "appending" optimization is voided anyway.
  // We use plain pointer comparison then.

  QList< MessageItem * > * messagesWithTheSameStrippedSubject = mThreadingCacheMessageSubjectMD5ToMessageItem->value( mi->strippedSubjectMD5(), 0 );
  if ( !messagesWithTheSameStrippedSubject )
  {
    messagesWithTheSameStrippedSubject = new QList< MessageItem * >();
    mThreadingCacheMessageSubjectMD5ToMessageItem->insert( mi->strippedSubjectMD5(), messagesWithTheSameStrippedSubject );
    messagesWithTheSameStrippedSubject->append( mi );
    return;
  }

  Q_ASSERT( !messagesWithTheSameStrippedSubject->contains( mi ) );

  // Binary search based insertion
  QList< MessageItem * >::Iterator it = qLowerBound( messagesWithTheSameStrippedSubject->begin(), messagesWithTheSameStrippedSubject->end(), mi, MessageLessThanByDate() );
  messagesWithTheSameStrippedSubject->insert( it, mi );
}

void Model::removeMessageFromSubjectBasedThreadingCache( MessageItem * mi )
{
  QList< MessageItem * > * messagesWithTheSameStrippedSubject = mThreadingCacheMessageSubjectMD5ToMessageItem->value( mi->strippedSubjectMD5(), 0 );
  Q_ASSERT( messagesWithTheSameStrippedSubject );

  QList< MessageItem * >::Iterator it = qLowerBound( messagesWithTheSameStrippedSubject->begin(), messagesWithTheSameStrippedSubject->end(), mi );
  Q_ASSERT( it != messagesWithTheSameStrippedSubject->end() );
  Q_ASSERT( *it == mi );

  messagesWithTheSameStrippedSubject->erase( it );
  if ( messagesWithTheSameStrippedSubject->isEmpty() )
  {
    mThreadingCacheMessageSubjectMD5ToMessageItem->remove( mi->strippedSubjectMD5() );
    delete messagesWithTheSameStrippedSubject;
  }
}

MessageItem * Model::guessMessageParent( MessageItem * mi )
{
  // This function implements subject based threading
  // It attempts to guess a thread parent for the item "mi"
  // which actually may already have a children subtree.

  // We have all the problems of findMessageParent() plus the fact that
  // we're actually guessing (and often we may be *wrong*).

  Q_ASSERT( mAggregation->threading() == Aggregation::PerfectReferencesAndSubject ); // caller must take care of this
  Q_ASSERT( mi->subjectIsPrefixed() ); // caller must take care of this
  Q_ASSERT( mi->threadingStatus() == MessageItem::ParentMissing );


  // Do subject based threading
  QString md5 = mi->strippedSubjectMD5();
  if ( !md5.isEmpty() )
  {
    QList< MessageItem * > * messagesWithTheSameStrippedSubject = mThreadingCacheMessageSubjectMD5ToMessageItem->value( mi->strippedSubjectMD5(), 0 );

    if ( messagesWithTheSameStrippedSubject )
    {
      Q_ASSERT( messagesWithTheSameStrippedSubject->count() > 0 );

      // Need to find the message with the maximum date lower than the one of this message

      time_t maxTime = (time_t)0;
      MessageItem * pParent = 0;

      // Here'we re really guessing so circular references are possible
      // even on perfectly valid trees. This is why we don't consider it
      // an error but just continue searching.

      // FIXME: This might be speed up with an initial binary search (?)
      // ANSWER: No. We can't rely on date order (as it can be updated on the fly...)

      for ( QList< MessageItem * >::Iterator it = messagesWithTheSameStrippedSubject->begin(); it != messagesWithTheSameStrippedSubject->end(); ++it )
      {
        int delta = mi->date() - ( *it )->date();

        // We don't take into account messages with a delta smaller than 120.
        // Assuming that our date() values are correct (that is, they take into
        // account timezones etc..) then one usually needs more than 120 seconds
        // to answer to a message. Better safe than sorry.

        // This check also includes negative deltas so messages later than mi aren't considered

        if ( delta < 120 )
          break; // The list is ordered by date (ascending) so we can stop searching here

        // About the "magic" 3628899 value here comes a Till's comment from the original KMHeaders:
        //
        //   "Parents more than six weeks older than the message are not accepted. The reasoning being
        //   that if a new message with the same subject turns up after such a long time, the chances
        //   that it is still part of the same thread are slim. The value of six weeks is chosen as a
        //   result of a poll conducted on kde-devel, so it's probably bogus. :)"

        if ( delta < 3628899 )
        {
          // Compute the closest.
          if ( ( maxTime < ( *it )->date() ) )
          {
            // This algorithm *can* be (and often is) wrong.
            // Take care of circular threading which is really possible at this level.
            // If mi contains (*it) inside its children subtree then we have
            // found such a circular threading problem.

            // Note that here we can't have *it == mi because of the delta >= 120 check above.

            if ( ( mi->childItemCount() == 0 ) || !( *it )->hasAncestor( mi ) )
            {
              maxTime = ( *it )->date();
              pParent = ( *it );
            }
          }
        }
      }

      if ( pParent )
      {
        mi->setThreadingStatus( MessageItem::ImperfectParentFound );
        return pParent; // got an imperfect parent for this message
      }
    }
  }

  return 0;
}

//
// A little template helper, hopefully inlineable.
//
// Return true if the specified message item is in the wrong position
// inside the specified parent and needs re-sorting. Return false otherwise.
// Both parent and messageItem must not be null.
//
// Checking if a message needs re-sorting instead of just re-sorting it
// is very useful since re-sorting is an expensive operation.
//
template< class ItemComparator > bool messageItemNeedsReSorting(
            SortOrder::SortDirection messageSortDirection, Item * parent, MessageItem *messageItem )
{
  if ( ( messageSortDirection == SortOrder::Ascending )
    || ( parent->type() == Item::Message ) )
  {
    return parent->childItemNeedsReSorting< ItemComparator, true >( messageItem );
  }
  return parent->childItemNeedsReSorting< ItemComparator, false >( messageItem );
}

bool Model::handleItemPropertyChanges( int propertyChangeMask, Item * parent, Item * item )
{
  // The facts:
  //
  // - If dates changed:
  //   - If we're sorting messages by min/max date then at each level the messages might need resorting.
  //   - If the thread leader is the most recent message of a thread then the uppermost
  //     message of the thread might need re-grouping.
  //   - If the groups are sorted by min/max date then the group might need re-sorting too.
  //
  // This function explicitly doesn't re-apply the filter when ActionItemStatus changes.
  // This is because filters must be re-applied due to a broader range of status variations:
  // this is done in viewItemJobStepInternalForJobPass1Update() instead (which is the only
  // place in that ActionItemStatus may be set).

  if( parent->type() == Item::InvisibleRoot )
  {
    // item is either a message or a group attacched to the root.
    // It might need resorting.
    if ( item->type() == Item::GroupHeader )
    {
      // item is a group header attacched to the root.
      if (
           (
             // max date changed
             ( propertyChangeMask & MaxDateChanged ) &&
             // groups sorted by max date
             ( mSortOrder->groupSorting() == SortOrder::SortGroupsByDateTimeOfMostRecent )
           ) || (
             // date changed
             ( propertyChangeMask & DateChanged ) &&
             // groups sorted by date
             ( mSortOrder->groupSorting() == SortOrder::SortGroupsByDateTime )
           )
         )
      {
        // This group might need re-sorting.

        // Groups are large container of messages so it's likely that
        // another message inserted will cause this group to be marked again.
        // So we wait until the end to do the grand final re-sorting: it will be done in Pass4.
        mGroupHeadersThatNeedUpdate->insert( static_cast< GroupHeaderItem * >( item ), static_cast< GroupHeaderItem * >( item ) );
      }
    } else {
      // item is a message. It might need re-sorting.

      // Since sorting is an expensive operation, we first check if it's *really* needed.
      // Re-sorting will actually not change min/max dates at all and
      // will not climb up the parent's ancestor tree.

      switch ( mSortOrder->messageSorting() )
      {
        case SortOrder::SortMessagesByDateTime:
          if ( propertyChangeMask & DateChanged ) // date changed
          {
            if ( messageItemNeedsReSorting< ItemDateComparator >( mSortOrder->messageSortDirection(), parent, static_cast< MessageItem * >( item ) ) )
              attachMessageToParent( parent, static_cast< MessageItem * >( item ) );
          } // else date changed, but it doesn't match sorting order: no need to re-sort
        break;
        case SortOrder::SortMessagesByDateTimeOfMostRecent:
          if ( propertyChangeMask & MaxDateChanged ) // max date changed
          {
            if ( messageItemNeedsReSorting< ItemMaxDateComparator >( mSortOrder->messageSortDirection(), parent, static_cast< MessageItem * >( item ) ) )
              attachMessageToParent( parent, static_cast< MessageItem * >( item ) );
          } // else max date changed, but it doesn't match sorting order: no need to re-sort
        break;
        case SortOrder::SortMessagesByActionItemStatus:
          if ( propertyChangeMask & ActionItemStatusChanged ) // todo status changed
          {
            if ( messageItemNeedsReSorting< ItemActionItemStatusComparator >( mSortOrder->messageSortDirection(), parent, static_cast< MessageItem * >( item ) ) )
              attachMessageToParent( parent, static_cast< MessageItem * >( item ) );
          } // else to do status changed, but it doesn't match sorting order: no need to re-sort
        break;
        default:
          // this kind of message sorting isn't affected by the property changes: nothing to do.
        break;
      }
    }

    return false; // the invisible root isn't affected by any change.
  }

  if ( parent->type() == Item::GroupHeader )
  {
    // item is a message attacched to a GroupHeader.
    // It might need re-grouping or re-sorting (within the same group)

    // Check re-grouping here.
    if (
         (
           // max date changed
           ( propertyChangeMask & MaxDateChanged ) &&
           // thread leader is most recent message
           ( mAggregation->threadLeader() == Aggregation::MostRecentMessage )
         ) || (
           // date changed
           ( propertyChangeMask & DateChanged ) &&
           // thread leader the topmost message
           ( mAggregation->threadLeader() == Aggregation::TopmostMessage )
         )
       )
    {
      // Might really need re-grouping.
      // attachMessageToGroupHeader() will find the right group for this message
      // and if it's different than the current it will move it.
      attachMessageToGroupHeader( static_cast< MessageItem * >( item ) );
      // Re-grouping fixes the properties of the involved group headers
      // so at exit of attachMessageToGroupHeader() the parent can't be affected
      // by the change anymore.
      return false;
    }

    // Re-grouping wasn't needed. Re-sorting might be.

  } // else item is a message attacched to another message and might need re-sorting only.

  // Check if message needs re-sorting.

  switch ( mSortOrder->messageSorting() )
  {
    case SortOrder::SortMessagesByDateTime:
      if ( propertyChangeMask & DateChanged ) // date changed
      {
        if ( messageItemNeedsReSorting< ItemDateComparator >( mSortOrder->messageSortDirection(), parent, static_cast< MessageItem * >( item ) ) )
          attachMessageToParent( parent, static_cast< MessageItem * >( item ) );
      } // else date changed, but it doesn't match sorting order: no need to re-sort
    break;
    case SortOrder::SortMessagesByDateTimeOfMostRecent:
      if ( propertyChangeMask & MaxDateChanged ) // max date changed
      {
        if ( messageItemNeedsReSorting< ItemMaxDateComparator >( mSortOrder->messageSortDirection(), parent, static_cast< MessageItem * >( item ) ) )
          attachMessageToParent( parent, static_cast< MessageItem * >( item ) );
      } // else max date changed, but it doesn't match sorting order: no need to re-sort
    break;
    case SortOrder::SortMessagesByActionItemStatus:
      if ( propertyChangeMask & ActionItemStatusChanged ) // todo status changed
      {
        if ( messageItemNeedsReSorting< ItemActionItemStatusComparator >( mSortOrder->messageSortDirection(), parent, static_cast< MessageItem * >( item ) ) )
          attachMessageToParent( parent, static_cast< MessageItem * >( item ) );
      } // else to do status changed, but it doesn't match sorting order: no need to re-sort
    break;
    default:
      // this kind of message sorting isn't affected by property changes: nothing to do.
    break;
  }

  return true; // parent might be affected too.
}

void Model::messageDetachedUpdateParentProperties( Item *oldParent, MessageItem *mi )
{
  Q_ASSERT( oldParent );
  Q_ASSERT( mi );
  Q_ASSERT( oldParent != mRootItem );


  // oldParent might have its properties changed because of the child removal.
  // propagate the changes up.
  for(;;)
  {
    // pParent is not the root item now. This is assured by how we enter this loop
    // and by the fact that handleItemPropertyChanges returns false when grandParent
    // is Item::InvisibleRoot. We could actually assert it here...

    // Check if its dates need an update.
    int propertyChangeMask;

    if ( ( mi->maxDate() == oldParent->maxDate() ) && oldParent->recomputeMaxDate() )
      propertyChangeMask = MaxDateChanged;
    else
      break; // from the for(;;) loop

    // One of the oldParent properties has changed for sure

    Item * grandParent = oldParent->parent();

    // If there is no grandParent then oldParent isn't attacched to the view.
    // Re-sorting / re-grouping isn't needed for sure.
    if ( !grandParent )
      break; // from the for(;;) loop

    // The following function will return true if grandParent may be affected by the change.
    // If the grandParent isn't affected, we stop climbing.
    if ( !handleItemPropertyChanges( propertyChangeMask, grandParent, oldParent ) )
      break; // from the for(;;) loop

    // Now we need to climb up one level and check again.
    oldParent = grandParent;
  } // for(;;) loop

  // If the last message was removed from a group header then this group will need an update
  // for sure. We will need to remove it (unless a message is attacched back to it)
  if ( oldParent->type() == Item::GroupHeader )
  {
    if ( oldParent->childItemCount() == 0 )
      mGroupHeadersThatNeedUpdate->insert( static_cast< GroupHeaderItem * >( oldParent ), static_cast< GroupHeaderItem * >( oldParent ) );
  }
}

void Model::propagateItemPropertiesToParent( Item * item )
{
  Item * pParent = item->parent();
  Q_ASSERT( pParent );
  Q_ASSERT( pParent != mRootItem );

  for(;;)
  {
    // pParent is not the root item now. This is assured by how we enter this loop
    // and by the fact that handleItemPropertyChanges returns false when grandParent
    // is Item::InvisibleRoot. We could actually assert it here...

    // Check if its dates need an update.
    int propertyChangeMask;

    if ( item->maxDate() > pParent->maxDate() )
    {
      pParent->setMaxDate( item->maxDate() );
      propertyChangeMask = MaxDateChanged;
    } else {
      // No parent dates have changed: no further work is needed. Stop climbing here.
      break; // from the for(;;) loop
    }

    // One of the pParent properties has changed.

    Item * grandParent = pParent->parent();

    // If there is no grandParent then pParent isn't attacched to the view.
    // Re-sorting / re-grouping isn't needed for sure.
    if ( !grandParent )
      break; // from the for(;;) loop

    // The following function will return true if grandParent may be affected by the change.
    // If the grandParent isn't affected, we stop climbing.
    if ( !handleItemPropertyChanges( propertyChangeMask, grandParent, pParent ) )
      break; // from the for(;;) loop

    // Now we need to climb up one level and check again.
    pParent = grandParent;

  } // for(;;)
}


void Model::attachMessageToParent( Item *pParent, MessageItem *mi )
{
  Q_ASSERT( pParent );
  Q_ASSERT( mi );

  // This function may be called to do a simple "re-sort" of the item inside the parent.
  // In that case mi->parent() is equal to pParent.
  bool oldParentWasTheSame;

  if ( mi->parent() )
  {
    Item * oldParent = mi->parent();

    // The item already had a parent and this means that we're moving it.
    oldParentWasTheSame = oldParent == pParent; // just re-sorting ?

    if ( mi->isViewable() ) // is actually
    {
      // The message is actually attached to the viewable root

      // Unfortunately we need to hack the model/view architecture
      // since it's somewhat flawed in this. At the moment of writing
      // there is simply no way to atomically move a subtree.
      // We must detach, call beginRemoveRows()/endRemoveRows(),
      // save the expanded state, save the selection, save the current item,
      // save the view position (YES! As we are removing items the view
      // will hopelessly jump around so we're just FORCED to break
      // the isolation from the view)...
      // ...*then* reattach, restore the expanded state, restore the selection,
      // restore the current item, restore the view position and pray
      // that nothing will fail in the (rather complicated) process....

      // Yet more unfortunately, while saving the expanded state might stop
      // at a certain (unexpanded) point in the tree, saving the selection
      // is hopelessly recursive up to the bare leafs.

      // Furthermore the expansion of items is a common case while selection
      // in the subtree is rare, so saving it would be a huge cost with
      // a low revenue.

      // This is why we just let the selection screw up. I hereby refuse to call
      // yet another expensive recursive function here :D

      // The current item saving can be somewhat optimized doing it once for
      // a single job step...

      if (
           ( ( mi )->childItemCount() > 0 ) && // has children
           mModelForItemFunctions && // the UI is not actually disconnected
           mView->isExpanded( index( mi, 0 ) ) // is actually expanded
         )
        saveExpandedStateOfSubtree( mi );
    }

    // If the parent is viewable (so mi was viewable too) then the beginRemoveRows()
    // and endRemoveRows() functions of this model will be called too.
    oldParent->takeChildItem( mModelForItemFunctions, mi );

    if ( ( !oldParentWasTheSame ) && ( oldParent != mRootItem ) )
      messageDetachedUpdateParentProperties( oldParent, mi );

  } else {
    // The item had no parent yet.
    oldParentWasTheSame = false;
  }

  // Take care of perfect / imperfect threading.
  // Items that are now perfectly threaded, but already have a different parent
  // might have been imperfectly threaded before. Remove them from the caches.
  // Items that are now imperfectly threaded must be added to the caches.
  //
  // If we're just re-sorting the item inside the same parent then the threading
  // caches don't need to be updated (since they actually depend on the parent).

  if ( !oldParentWasTheSame )
  {
    switch( mi->threadingStatus() )
    {
      case MessageItem::PerfectParentFound:
        if ( !mi->inReplyToIdMD5().isEmpty() )
          mThreadingCacheMessageInReplyToIdMD5ToMessageItem->remove( mi->inReplyToIdMD5(), mi );
      break;
      case MessageItem::ImperfectParentFound:
      case MessageItem::ParentMissing: // may be: temporary or just fallback assignment
        if ( !mi->inReplyToIdMD5().isEmpty() )
        {
          if ( !mThreadingCacheMessageInReplyToIdMD5ToMessageItem->contains( mi->inReplyToIdMD5(), mi ) )
            mThreadingCacheMessageInReplyToIdMD5ToMessageItem->insert( mi->inReplyToIdMD5(), mi );
        }
      break;
      case MessageItem::NonThreadable: // this also happens when we do no threading at all
        // make gcc happy
        Q_ASSERT( !mThreadingCacheMessageInReplyToIdMD5ToMessageItem->contains( mi->inReplyToIdMD5(), mi ) );
      break;
    }
  }

  // Set the new parent
  mi->setParent( pParent );

  // Propagate watched and ignored status
  if (
       ( pParent->status().toQInt32() & mCachedWatchedOrIgnoredStatusBits ) && // unlikely
       ( pParent->type() == Item::Message ) // likely
     )
  {
    // the parent is either watched or ignored: propagate to the child
    if ( pParent->status().isWatched() )
    {
      mi->setStatus( KPIM::MessageStatus::statusWatched() );
      mStorageModel->setMessageItemStatus( mi, KPIM::MessageStatus::statusWatched() );
    } else if ( pParent->status().isIgnored() )
    {
      mi->setStatus( KPIM::MessageStatus::statusIgnored() );
      mStorageModel->setMessageItemStatus( mi, KPIM::MessageStatus::statusIgnored() );
    }
  }

  // And insert into its child list

  // If pParent is viewable then the insert/append functions will call this model's
  // beginInsertRows() and endInsertRows() functions. This is EXTREMELY
  // expensive and ugly but it's the only way with the Qt4 imposed Model/View method.
  // Dude... (citation from Lost, if it wasn't clear).

  // I'm using a macro since it does really improve readability.
  // I'm NOT using a helper function since gcc will refuse to inline some of
  // the calls because they make this function grow too much.
#define INSERT_MESSAGE_WITH_COMPARATOR( _ItemComparator ) \
  if ( ( mSortOrder->messageSortDirection() == SortOrder::Ascending ) \
    || ( pParent->type() == Item::Message ) ) \
  { \
    pParent->insertChildItem< _ItemComparator, true >( mModelForItemFunctions, mi ); \
  } \
  else \
  { \
    pParent->insertChildItem< _ItemComparator, false >( mModelForItemFunctions, mi ); \
  }

  // If pParent is viewable then the insertion call will also set the child state to viewable.
  // Since mi MAY have children, then this call may make them viewable.
  switch( mSortOrder->messageSorting() )
  {
    case SortOrder::SortMessagesByDateTime:
      INSERT_MESSAGE_WITH_COMPARATOR( ItemDateComparator )
    break;
    case SortOrder::SortMessagesByDateTimeOfMostRecent:
      INSERT_MESSAGE_WITH_COMPARATOR( ItemMaxDateComparator )
    break;
    case SortOrder::SortMessagesBySize:
      INSERT_MESSAGE_WITH_COMPARATOR( ItemSizeComparator )
    break;
    case SortOrder::SortMessagesBySenderOrReceiver:
      INSERT_MESSAGE_WITH_COMPARATOR( ItemSenderOrReceiverComparator )
    break;
    case SortOrder::SortMessagesBySender:
      INSERT_MESSAGE_WITH_COMPARATOR( ItemSenderComparator )
    break;
    case SortOrder::SortMessagesByReceiver:
      INSERT_MESSAGE_WITH_COMPARATOR( ItemReceiverComparator )
    break;
    case SortOrder::SortMessagesBySubject:
      INSERT_MESSAGE_WITH_COMPARATOR( ItemSubjectComparator )
    break;
    case SortOrder::SortMessagesByActionItemStatus:
      INSERT_MESSAGE_WITH_COMPARATOR( ItemActionItemStatusComparator )
    break;
    case SortOrder::NoMessageSorting:
      pParent->appendChildItem( mModelForItemFunctions, mi );
    break;
    default: // should never happen
      pParent->appendChildItem( mModelForItemFunctions, mi );
    break;
  }

  // Decide if we need to expand parents
  bool childNeedsExpanding = ( mi->initialExpandStatus() == Item::ExpandNeeded );

  if ( pParent->initialExpandStatus() == Item::NoExpandNeeded )
  {
    switch( mAggregation->threadExpandPolicy() )
    {
      case Aggregation::NeverExpandThreads:
        // just do nothing unless this child has children and is already marked for expansion
        if ( childNeedsExpanding )
          pParent->setInitialExpandStatus( Item::ExpandNeeded );
      break;
      case Aggregation::ExpandThreadsWithNewMessages:
        // expand only if new (or it has children marked for expansion)
        if ( childNeedsExpanding || mi->status().isNew() )
          pParent->setInitialExpandStatus( Item::ExpandNeeded );
      break;
      case Aggregation::ExpandThreadsWithUnreadMessages:
        // expand only if unread or new (or it has children marked for expansion)
        if ( childNeedsExpanding || mi->status().isUnread() || mi->status().isNew() )
          pParent->setInitialExpandStatus( Item::ExpandNeeded );
      break;
      case Aggregation::ExpandThreadsWithUnreadOrImportantMessages:
        // expand only if unread, new, important or todo (or it has children marked for expansion)
        // FIXME: Wouldn't it be nice to be able to test for bitmasks in MessageStatus ?
        if ( childNeedsExpanding || mi->status().isUnread() || mi->status().isNew() || mi->status().isImportant() || mi->status().isToAct() )
          pParent->setInitialExpandStatus( Item::ExpandNeeded );
      break;
      case Aggregation::AlwaysExpandThreads:
        // expand everything
        pParent->setInitialExpandStatus( Item::ExpandNeeded );
      break;
      default:
        // BUG
      break;
    }
  } // else it's already marked for expansion or expansion has been already executed

  // expand parent first, if possible
  if ( pParent->initialExpandStatus() == Item::ExpandNeeded )
  {
    // If UI is not disconnected and parent is viewable, go up and expand
    if ( mModelForItemFunctions && pParent->isViewable() )
    {
      // Now expand parents as needed
      Item * parentToExpand = pParent;
      while ( parentToExpand )
      {
        if ( parentToExpand == mRootItem )
          break; // no need to set it expanded
        // parentToExpand is surely viewable (because this item is)
        if ( parentToExpand->initialExpandStatus() == Item::ExpandExecuted )
          break;

        mView->expand( index( parentToExpand, 0 ) );

        parentToExpand->setInitialExpandStatus( Item::ExpandExecuted );
        parentToExpand = parentToExpand->parent();
      }
    } else {
      // It isn't viewable or UI is disconnected: climb up marking only
      Item * parentToExpand = pParent->parent();
      while ( parentToExpand )
      {
        if ( parentToExpand == mRootItem )
          break; // no need to set it expanded
        parentToExpand->setInitialExpandStatus( Item::ExpandNeeded );
        parentToExpand = parentToExpand->parent();
      }
    }
  }

  if ( mi->isViewable() )
  {
    // mi is now viewable

    // sync subtree expanded status
    if ( childNeedsExpanding )
    {
      if ( mi->childItemCount() > 0 )
        if ( mModelForItemFunctions ) // the UI is not disconnected
          syncExpandedStateOfSubtree( mi ); // sync the real state in the view
    }

    // apply the filter, if needed
    if ( mFilter )
    {
      Q_ASSERT( mModelForItemFunctions ); // the UI must be NOT disconnected here

      // apply the filter to subtree
      if ( applyFilterToSubtree( mi, index( pParent, 0 ) ) )
      {
        // mi matched, expand parents (unconditionally)
        mView->ensureCurrentlyViewable( mi );
      }
    }
  }

  // Now we need to propagate the property changes the upper levels.

  // If we have just inserted a message inside the root then no work needs to be done:
  // no grouping is in effect and the message is already in the right place.
  if ( pParent == mRootItem )
    return;

  // If we have just removed the item from this parent and re-inserted it
  // then this operation was a simple re-sort. The code above didn't update
  // the properties when removing the item so we don't actually need
  // to make the updates back.
  if ( oldParentWasTheSame )
    return;

  // FIXME: OPTIMIZE THIS: First propagate changes THEN syncExpandedStateOfSubtree()
  //        and applyFilterToSubtree... (needs some thinking though).

  // Time to propagate up.
  propagateItemPropertiesToParent( mi );

  // Aaah.. we're done. Time for a thea ? :)
}

// FIXME: ThreadItem ?
//
// Foo Bar, Joe Thommason, Martin Rox ... Eddie Maiden                    <date of the thread>
// Title                                      <number of messages>, Last by xxx <inner status>
//
// When messages are added, mark it as dirty only (?)

Model::ViewItemJobResult Model::viewItemJobStepInternalForJobPass5( ViewItemJob *job, const QTime &tStart )
{
  // In this pass we scan the group headers that are in mGroupHeadersThatNeedUpdate.
  // Empty groups get deleted while the other ones are re-sorted.
  int elapsed;

  int curIndex = job->currentIndex();

  QHash< GroupHeaderItem *, GroupHeaderItem * >::Iterator it = mGroupHeadersThatNeedUpdate->begin();

  while ( it != mGroupHeadersThatNeedUpdate->end() )
  {
    if ( ( *it )->childItemCount() == 0 )
    {
      // group with no children, kill it
      ( *it )->parent()->takeChildItem( mModelForItemFunctions, *it );
      mGroupHeaderItemHash->remove( ( *it )->label() );
      delete *it;
    } else {
      // Group with children: probably needs re-sorting.

      // Re-sorting here is an expensive operation.
      // In fact groups have been put in the QHash above on the assumption
      // that re-sorting *might* be needed but no real (expensive) check
      // has been done yet. Also by sorting a single group we might actually
      // put the others in the right place.
      // So finally check if re-sorting is *really* needed.
      bool needsReSorting;

      // A macro really improves readability here.
#define CHECK_IF_GROUP_NEEDS_RESORTING( _ItemDateComparator ) \
          switch ( mSortOrder->groupSortDirection() ) \
          { \
            case SortOrder::Ascending: \
              needsReSorting = ( *it )->parent()->childItemNeedsReSorting< _ItemDateComparator, true >( *it ); \
            break; \
            case SortOrder::Descending: \
              needsReSorting = ( *it )->parent()->childItemNeedsReSorting< _ItemDateComparator, false >( *it ); \
            break; \
            default: /* should never happen */ \
              needsReSorting = false; \
            break; \
          }

      switch ( mSortOrder->groupSorting() )
      {
        case SortOrder::SortGroupsByDateTime:
          CHECK_IF_GROUP_NEEDS_RESORTING( ItemDateComparator )
        break;
        case SortOrder::SortGroupsByDateTimeOfMostRecent:
          CHECK_IF_GROUP_NEEDS_RESORTING( ItemMaxDateComparator )
        break;
        case SortOrder::SortGroupsBySenderOrReceiver:
          CHECK_IF_GROUP_NEEDS_RESORTING( ItemSenderOrReceiverComparator )
        break;
        case SortOrder::SortGroupsBySender:
          CHECK_IF_GROUP_NEEDS_RESORTING( ItemSenderComparator )
        break;
        case SortOrder::SortGroupsByReceiver:
          CHECK_IF_GROUP_NEEDS_RESORTING( ItemReceiverComparator )
        break;
        case SortOrder::NoGroupSorting:
          needsReSorting = false;
        break;
        default:
          // Should never happen... just assume re-sorting is not needed
          needsReSorting = false;
        break;
      }

      if ( needsReSorting )
        attachGroup( *it ); // it will first detach and then re-attach in the proper place
    }

    mGroupHeadersThatNeedUpdate->erase( it );
    it = mGroupHeadersThatNeedUpdate->begin();

    curIndex++;

    // FIXME: In fact a single update is likely to manipulate
    //        a subtree with a LOT of messages inside. If interactivity is favored
    //        we should check the time really more often.
    if ( ( curIndex % mViewItemJobStepMessageCheckCount ) == 0 )
    {
      elapsed = tStart.msecsTo( QTime::currentTime() );
      if ( ( elapsed > mViewItemJobStepChunkTimeout ) || ( elapsed < 0 ) )
      {
        if ( it != mGroupHeadersThatNeedUpdate->end() )
        {
          job->setCurrentIndex( curIndex );
          return ViewItemJobInterrupted;
        }
      }
    }

  }

  return ViewItemJobCompleted;
}



Model::ViewItemJobResult Model::viewItemJobStepInternalForJobPass4( ViewItemJob *job, const QTime &tStart )
{
  // In this pass we scan mUnassignedMessageListForPass4 which now
  // contains both items with parents and items without parents.
  // We scan mUnassignedMessageList for messages without parent (the ones that haven't been
  // attacched to the viewable tree yet) and find a suitable group for them. Then we simply
  // clear mUnassignedMessageList.

  // We call this pass "Grouping"

  int elapsed;

  int curIndex = job->currentIndex();
  int endIndex = job->endIndex();

  while ( curIndex <= endIndex )
  {
    MessageItem * mi = (*mUnassignedMessageListForPass4)[curIndex];
    if ( !mi->parent() )
    {
      // Unassigned item: thread leader, insert into the proper group.
      // Locate the group (or root if no grouping requested)
      attachMessageToGroupHeader( mi );
    } else {
      // A parent was already assigned in Pass3: we have nothing to do here
    }
    curIndex++;

    // FIXME: In fact a single call to attachMessageToGroupHeader() is likely to manipulate
    //        a subtree with a LOT of messages inside. If interactivity is favored
    //        we should check the time really more often.
    if ( ( curIndex % mViewItemJobStepMessageCheckCount ) == 0 )
    {
      elapsed = tStart.msecsTo( QTime::currentTime() );
      if ( ( elapsed > mViewItemJobStepChunkTimeout ) || ( elapsed < 0 ) )
      {
        if ( curIndex <= endIndex )
        {
          job->setCurrentIndex( curIndex );
          return ViewItemJobInterrupted;
        }
      }
    }
  }

  mUnassignedMessageListForPass4->clear();
  return ViewItemJobCompleted;
}

Model::ViewItemJobResult Model::viewItemJobStepInternalForJobPass3( ViewItemJob *job, const QTime &tStart )
{
  // In this pass we scan the mUnassignedMessageListForPass3 and try to do construct the threads
  // by using subject based threading. If subject based threading is not in effect then
  // this pass turns to a nearly-no-op: at the end of Pass2 we have swapped the lists
  // and mUnassignedMessageListForPass3 is actually empty.

  // We don't shrink the mUnassignedMessageListForPass3 for two reasons:
  // - It would mess up this chunked algorithm by shifting indexes
  // - mUnassignedMessageList is a QList which is basically an array. It's faster
  //   to traverse an array of N entries than to remove K>0 entries one by one and
  //   to traverse the remaining N-K entries.

  int elapsed;

  int curIndex = job->currentIndex();
  int endIndex = job->endIndex();

  while ( curIndex <= endIndex )
  {
    // If we're here, then threading is requested for sure.
    MessageItem * mi = (*mUnassignedMessageListForPass3)[curIndex];
    if ( ( !mi->parent() ) || ( mi->threadingStatus() == MessageItem::ParentMissing ) )
    {
      // Parent is missing (either "physically" with the item being not attacched or "logically"
      // with the item being attacched to a group or directly to the root.
      if ( mi->subjectIsPrefixed() )
      {
        // We can try to guess it
        MessageItem * mparent = guessMessageParent( mi );

        if ( mparent )
        {
          // imperfect parent found
          if ( mi->isViewable() )
          {
            // mi was already viewable, we're just trying to re-parent it better...
            attachMessageToParent( mparent, mi );
            if ( !mparent->isViewable() )
            {
              // re-attach it immediately (so current item is not lost)
              MessageItem * topmost = mparent->topmostMessage();
              Q_ASSERT( !topmost->parent() ); // groups are always viewable!
              topmost->setThreadingStatus( MessageItem::ParentMissing );
              attachMessageToGroupHeader( topmost );
            }
          } else {
            // mi wasn't viewable yet.. no need to attach parent
            attachMessageToParent( mparent, mi );
          }
          // and we're done for now
        } else {
          // so parent not found, (threadingStatus() is either MessageItem::ParentMissing or MessageItem::NonThreadable)
          Q_ASSERT( ( mi->threadingStatus() == MessageItem::ParentMissing ) || ( mi->threadingStatus() == MessageItem::NonThreadable ) );
          mUnassignedMessageListForPass4->append( mi ); // this is ~O(1)
          // and wait for Pass4
        }
      } else {
        // can't guess the parent as the subject isn't prefixed
        Q_ASSERT( ( mi->threadingStatus() == MessageItem::ParentMissing ) || ( mi->threadingStatus() == MessageItem::NonThreadable ) );
        mUnassignedMessageListForPass4->append( mi ); // this is ~O(1)
        // and wait for Pass4
      }
    } else {
      // Has a parent: either perfect parent already found or non threadable.
      // Since we don't end here if mi has status of parent missing then mi must not have imperfect parent.
      Q_ASSERT( mi->threadingStatus() != MessageItem::ImperfectParentFound );
      Q_ASSERT( mi->isViewable() );
    }

    curIndex++;

    // FIXME: In fact a single call to attachMessageToGroupHeader() is likely to manipulate
    //        a subtree with a LOT of messages inside. If interactivity is favored
    //        we should check the time really more often.
    if ( ( curIndex % mViewItemJobStepMessageCheckCount ) == 0 )
    {
      elapsed = tStart.msecsTo( QTime::currentTime() );
      if ( ( elapsed > mViewItemJobStepChunkTimeout ) || ( elapsed < 0 ) )
      {
        if ( curIndex <= endIndex )
        {
          job->setCurrentIndex( curIndex );
          return ViewItemJobInterrupted;
        }
      }
    }
  }

  mUnassignedMessageListForPass3->clear();
  return ViewItemJobCompleted;
}

Model::ViewItemJobResult Model::viewItemJobStepInternalForJobPass2( ViewItemJob *job, const QTime &tStart )
{
  // In this pass we scan the mUnassignedMessageList and try to do construct the threads.
  // If some thread leader message got attacched to the viewable tree in Pass1Fill then
  // we'll also attach all of its children too. The thread leaders we were unable
  // to attach in Pass1Fill and their children (which we find here) will make it to the small Pass3

  // We don't shrink the mUnassignedMessageList for two reasons:
  // - It would mess up this chunked algorithm by shifting indexes
  // - mUnassignedMessageList is a QList which is basically an array. It's faster
  //   to traverse an array of N entries than to remove K>0 entries one by one and
  //   to traverse the remaining N-K entries.

  // We call this pass "Threading"

  int elapsed;

  int curIndex = job->currentIndex();
  int endIndex = job->endIndex();

  while ( curIndex <= endIndex )
  {
    // If we're here, then threading is requested for sure.
    MessageItem * mi = (*mUnassignedMessageListForPass2)[curIndex];
    // The item may or may not have a parent.
    // If it has no parent or it has a temporary one (mi->parent() && mi->threadingStatus() == MessageItem::ParentMissing)
    // then we attempt to (re-)thread it. Otherwise we just do nothing (the job has already been done by the previous steps).
    if ( ( !mi->parent() ) || ( mi->threadingStatus() == MessageItem::ParentMissing ) )
    {
      MessageItem * mparent = findMessageParent( mi );

      if ( mparent )
      {
        // parent found, either perfect or imperfect
        if ( mi->isViewable() )
        {
          // mi was already viewable, we're just trying to re-parent it better...
          attachMessageToParent( mparent, mi );
          if ( !mparent->isViewable() )
          {
            // re-attach it immediately (so current item is not lost)
            MessageItem * topmost = mparent->topmostMessage();
            Q_ASSERT( !topmost->parent() ); // groups are always viewable!
            topmost->setThreadingStatus( MessageItem::ParentMissing );
            attachMessageToGroupHeader( topmost );
          }
        } else {
          // mi wasn't viewable yet.. no need to attach parent
          attachMessageToParent( mparent, mi );
        }
        // and we're done for now
      } else {
        // so parent not found, (threadingStatus() is either MessageItem::ParentMissing or MessageItem::NonThreadable)
        switch( mi->threadingStatus() )
        {
          case MessageItem::ParentMissing:
            if ( mAggregation->threading() == Aggregation::PerfectReferencesAndSubject )
            {
              // parent missing but still can be found in Pass3
              mUnassignedMessageListForPass3->append( mi ); // this is ~O(1)
            } else {
              // We're not doing subject based threading: will never be threaded, go straight to Pass4
              mUnassignedMessageListForPass4->append( mi ); // this is ~O(1)
            }
          break;
          case MessageItem::NonThreadable:
            // will never be threaded, go straight to Pass4
            mUnassignedMessageListForPass4->append( mi ); // this is ~O(1)
          break;
          default:
            // a bug for sure
            kWarning() << "ERROR: Invalid message threading status returned by findMessageParent()!";
            Q_ASSERT( false );
          break;
        }
      }
    } else {
      // Has a parent: either perfect parent already found or non threadable.
      // Since we don't end here if mi has status of parent missing then mi must not have imperfect parent.
      Q_ASSERT( mi->threadingStatus() != MessageItem::ImperfectParentFound );
      if ( !mi->isViewable() )
      {
        kWarning() << "Non viewable message " << mi << " subject " << mi->subject().toUtf8().data() << endl;
        Q_ASSERT( mi->isViewable() );
      }
    }

    curIndex++;

    // FIXME: In fact a single call to attachMessageToGroupHeader() is likely to manipulate
    //        a subtree with a LOT of messages inside. If interactivity is favored
    //        we should check the time really more often.
    if ( ( curIndex % mViewItemJobStepMessageCheckCount ) == 0 )
    {
      elapsed = tStart.msecsTo( QTime::currentTime() );
      if ( ( elapsed > mViewItemJobStepChunkTimeout ) || ( elapsed < 0 ) )
      {
        if ( curIndex <= endIndex )
        {
          job->setCurrentIndex( curIndex );
          return ViewItemJobInterrupted;
        }
      }
    }
  }

  mUnassignedMessageListForPass2->clear();
  return ViewItemJobCompleted;
}

Model::ViewItemJobResult Model::viewItemJobStepInternalForJobPass1Fill( ViewItemJob *job, const QTime &tStart )
{
  // In this pass we scan the a contiguous region of the underlying storage (that is
  // assumed to be FLAT) and create the corresponding MessageItem objects.
  // The deal is to show items to the user as soon as possible so in this pass we
  // *TRY* to attach them to the viewable tree (which is rooted on mRootItem).
  // Messages we're unable to attach for some reason (mainly due to threading) get appended
  // to mUnassignedMessageList and wait for Pass2.

  // We call this pass "Processing"

  int elapsed;

  // Should we use the receiver or the sender field for sorting ?
  bool bUseReceiver = ( mStorageModel->containsOutboundMessages() );

  // The begin storage index of our work
  int curIndex = job->currentIndex();
  // The end storage index of our work.
  int endIndex = job->endIndex();

  MessageItem * mi = 0;

  while( curIndex <= endIndex )
  {
    // Create the message item with no parent: we'll set it later
    if ( !mi )
    {
      mi = new MessageItem();
    } else {
      // a MessageItem discarded by a previous iteration: reuse it.
      Q_ASSERT( mi->parent() == 0 );
    }

    if ( !mStorageModel->initializeMessageItem( mi, curIndex, bUseReceiver ) )
    {
      // ugh
      kWarning() << "Fill of the MessageItem at storage row index " << curIndex << " failed" << endl;
      curIndex++;
      continue;
    }

    // If we're supposed to pre-select a specific message, check if it's this one.
    if ( mUniqueIdOfLastSelectedMessageInFolder != 0 )
    {
      // Yes.. a pre-selection is pending
      if( mUniqueIdOfLastSelectedMessageInFolder == mi->uniqueId() )
      {
        // Found, it's this one.
        // But actually it's not viewable (so not selectable). We must wait
        // until the end of the job to be 100% sure. So here we just translate
        // the unique id to a MessageItem pointer and wait.
        mLastSelectedMessageInFolder = mi;
        mUniqueIdOfLastSelectedMessageInFolder = 0; // already found, don't bother checking anymore
      }
    }

    // Ok.. it passed the initial checks: we will not be discarding it.
    // Make this message item an invariant index to the underlying model storage.
    mInvariantRowMapper->createModelInvariantIndex( curIndex, mi );


    // Attempt to do threading as soon as possible (to display items to the user)
    if ( mAggregation->threading() != Aggregation::NoThreading )
    {
      // Threading is requested

      // Fetch the data needed for proper threading
      // Add the item to the threading caches

      switch( mAggregation->threading() )
      {
        case Aggregation::PerfectReferencesAndSubject:
          mStorageModel->fillMessageItemThreadingData( mi, curIndex, StorageModel::PerfectThreadingReferencesAndSubject );

          // We also need to build the subject-based threading cache
          addMessageToSubjectBasedThreadingCache( mi );
        break;
        case Aggregation::PerfectAndReferences:
          mStorageModel->fillMessageItemThreadingData( mi, curIndex, StorageModel::PerfectThreadingPlusReferences );
        break;
        default:
          mStorageModel->fillMessageItemThreadingData( mi, curIndex, StorageModel::PerfectThreadingOnly );
        break;
      }

      // Perfect/References threading cache
      mThreadingCacheMessageIdMD5ToMessageItem->insert( mi->messageIdMD5(), mi );

      // Check if this item is a perfect parent for some imperfectly threaded
      // message (that is actually attacched to it, but not necessairly to the
      // viewable root). If it is, then remove the imperfect child from its
      // current parent rebuild the hierarchy on the fly.

      bool needsImmediateReAttach = false;

      if ( mThreadingCacheMessageInReplyToIdMD5ToMessageItem->count() > 0 ) // unlikely
      {
        QList< MessageItem * > lImperfectlyThreaded = mThreadingCacheMessageInReplyToIdMD5ToMessageItem->values( mi->messageIdMD5() );
        if ( !lImperfectlyThreaded.isEmpty() )
        {
          // must move all of the items in the perfect parent
          for ( QList< MessageItem * >::Iterator it = lImperfectlyThreaded.begin(); it != lImperfectlyThreaded.end(); ++it )
          {
            Q_ASSERT( ( *it )->parent() );
            Q_ASSERT( ( *it )->parent() != mi );
#if 1
            Q_ASSERT( ( ( *it )->threadingStatus() == MessageItem::ImperfectParentFound ) || ( ( *it )->threadingStatus() == MessageItem::ParentMissing ) );
#else
            if(!(( ( *it )->threadingStatus() == MessageItem::ImperfectParentFound ) || ( ( *it )->threadingStatus() == MessageItem::ParentMissing )))
            {
              kDebug() << "GOT A MESSAGE " << ( *it ) << " WITH THREADING STATUS " << ( *it )->threadingStatus();
              Q_ASSERT( false );
            }
#endif
            // If the item was already attached to the view then
            // re-attach it immediately. This will avoid a message
            // being displayed for a short while in the view and then
            // disappear until a perfect parent isn't found.
            if ( ( *it )->isViewable() )
              needsImmediateReAttach = true;

            ( *it )->setThreadingStatus( MessageItem::PerfectParentFound );
            attachMessageToParent( mi, *it );
          }
        }
      }

      // FIXME: Might look by "References" too, here... (?)

      // Attempt to do threading with anything we already have in caches until now
      // Note that this is likely to work since thread-parent messages tend
      // to come before thread-children messages in the folders (simply because of
      // date of arrival).

      Item * pParent;

      // First of all try to find a "perfect parent", that is the message for that
      // we have the ID in the "In-Reply-To" field. This is actually done by using
      // MD5 caches of the message ids because of speed. Collisions are very unlikely.

      QString md5 = mi->inReplyToIdMD5();

      if ( !md5.isEmpty() )
      {
        // Have an In-Reply-To field MD5.
        // In well behaved mailing lists 70% of the threadable messages get a parent here :)
        pParent = mThreadingCacheMessageIdMD5ToMessageItem->value( md5, 0 );

        if( pParent ) // very likely
        {
          if ( pParent == mi )
          {
            // Bad, bad message.. it has In-Reply-To equal to MessageId...
            // Will wait for Pass2 with References-Id only
            mUnassignedMessageListForPass2->append( mi );
          } else {
            // wow, got a perfect parent for this message!
            mi->setThreadingStatus( MessageItem::PerfectParentFound );
            attachMessageToParent( pParent, mi );
            // we're done with this message (also for Pass2)
          }
        } else {
          // got no parent
          // will have to wait Pass2
          mUnassignedMessageListForPass2->append( mi );
        }
      } else {
        // No In-Reply-To header.

        bool mightHaveOtherMeansForThreading;

        switch( mAggregation->threading() )
        {
          case Aggregation::PerfectReferencesAndSubject:
            mightHaveOtherMeansForThreading = mi->subjectIsPrefixed() || !mi->referencesIdMD5().isEmpty();
          break;
          case Aggregation::PerfectAndReferences:
            mightHaveOtherMeansForThreading = !mi->referencesIdMD5().isEmpty();
          break;
          case Aggregation::PerfectOnly:
            mightHaveOtherMeansForThreading = false;
          break;
          default:
            // BUG: there shouldn't be other values (NoThreading is ecluded in an upper branch)
            Q_ASSERT( false );
            mightHaveOtherMeansForThreading = false; // make gcc happy
          break;
        }

        if ( mightHaveOtherMeansForThreading )
        {
          // We might have other means for threading this message, wait until Pass2
          mUnassignedMessageListForPass2->append( mi );
        } else {
          // No other means for threading this message. This is either
          // a standalone message or a thread leader.
          // If there is no grouping in effect or thread leaders are just the "topmost"
          // messages then we might be done with this one.
          if (
               ( mAggregation->grouping() == Aggregation::NoGrouping ) ||
               ( mAggregation->threadLeader() == Aggregation::TopmostMessage )
            )
          {
            // We're done with this message: it will be surely either toplevel (no grouping in effect)
            // or a thread leader with a well defined group. Do it :)
            //kDebug() << "Setting message status from " << mi->threadingStatus() << " to non threadable (1) " << mi;
            mi->setThreadingStatus( MessageItem::NonThreadable );
            // Locate the parent group for this item
            attachMessageToGroupHeader( mi );
            // we're done with this message (also for Pass2)
          } else {
            // Threads belong to the most recent message in the thread. This means
            // that we have to wait until Pass2 or Pass3 to assign a group.
            mUnassignedMessageListForPass2->append( mi );
          }
        }
      }

      if ( needsImmediateReAttach && !mi->isViewable() )
      {
        // The item gathered previously viewable children. They must be immediately
        // re-shown. So this item must currently be attached to the view.
        // This is a temporary measure: it will be probably still moved.
        MessageItem * topmost = mi->topmostMessage();
        Q_ASSERT( topmost->threadingStatus() == MessageItem::ParentMissing );
        attachMessageToGroupHeader( topmost );
      }

    } else {
      // else no threading requested: we don't even need Pass2
      // set not threadable status (even if it might be not true, but in this mode we don't care)
      //kDebug() << "Setting message status from " << mi->threadingStatus() << " to non threadable (2) " << mi;
      mi->setThreadingStatus( MessageItem::NonThreadable );
      // locate the parent group for this item
      if ( mAggregation->grouping() == Aggregation::NoGrouping )
        attachMessageToParent( mRootItem, mi ); // no groups requested, attach directly to root
      else
        attachMessageToGroupHeader( mi );
      // we're done with this message (also for Pass2)
    }

    mi = 0; // this item was pushed somewhere, create a new one at next iteration
    curIndex++;

    if ( ( curIndex % mViewItemJobStepMessageCheckCount ) == 0 )
    {
      elapsed = tStart.msecsTo( QTime::currentTime() );
      if ( ( elapsed > mViewItemJobStepChunkTimeout ) || ( elapsed < 0 ) )
      {
        if ( curIndex <= endIndex )
        {
          job->setCurrentIndex( curIndex );
          if ( mi )
            delete mi;
          return ViewItemJobInterrupted;
        }
      }
    }
  }

  if ( mi )
    delete mi;
  return ViewItemJobCompleted;
}

Model::ViewItemJobResult Model::viewItemJobStepInternalForJobPass1Cleanup( ViewItemJob *job, const QTime &tStart )
{
  Q_ASSERT( mModelForItemFunctions ); // UI must be not disconnected here
  // In this pass we remove the MessageItem objects that are present in the job
  // and put their children in the unassigned message list.

  // Note that this list in fact contains MessageItem objects (we need dynamic_cast<>).
  QList< ModelInvariantIndex * > * invalidatedMessages = job->invariantIndexList();

  // We don't shrink the invalidatedMessages because it's basically an array.
  // It's faster to traverse an array of N entries than to remove K>0 entries
  // one by one and to traverse the remaining N-K entries.

  int elapsed;

  // The begin index of our work
  int curIndex = job->currentIndex();
  // The end index of our work.
  int endIndex = job->endIndex();

  if ( curIndex == job->startIndex() )
    Q_ASSERT( mOrphanChildrenHash->isEmpty() );

  while( curIndex <= endIndex )
  {
    // Get the underlying storage message data...
    MessageItem * dyingMessage = dynamic_cast< MessageItem * >( invalidatedMessages->at( curIndex ) );
    // This MUST NOT be null (otherwise we have a bug somewhere in this file).
    Q_ASSERT( dyingMessage );

    // If we were going to pre-select this message but we were interrupted
    // *before* it was actually made viewable, we just clear the pre-selection pointer
    // and unique id (abort pre-selection).
    if ( dyingMessage == mLastSelectedMessageInFolder )
    {
      mLastSelectedMessageInFolder = 0;
      mUniqueIdOfLastSelectedMessageInFolder = 0;
    }

    // remove the message from any pending user job
    if ( mPersistentSetManager )
    {
      mPersistentSetManager->removeMessageItemFromAllSets( dyingMessage );
      if ( mPersistentSetManager->setCount() < 1 )
      {
        delete mPersistentSetManager;
        mPersistentSetManager = 0;
      }
    }

    if ( dyingMessage->parent() )
    {
      // Handle saving the current selection: if this item was the current before the step
      // then zero it out. We have killed it and it's OK for the current item to change.

      if ( dyingMessage == mCurrentItemToRestoreAfterViewItemJobStep )
      {
        Q_ASSERT( dyingMessage->isViewable() );
        // Try to select the item below the removed one as it helps in doing a "readon" of emails:
        // you read a message, decide to delete it and then go to the next.
        // Qt tends to select the message above the removed one instead (this is a hardcoded logic in
        // QItemSelectionModelPrivate::_q_rowsAboutToBeRemoved()).
        mCurrentItemToRestoreAfterViewItemJobStep = mView->messageItemAfter( dyingMessage, MessageTypeAny, false );

        if ( !mCurrentItemToRestoreAfterViewItemJobStep )
        {
          // There is no item below. Try the item above.
          // We still do it better than qt which tends to find the *thread* above
          // instead of the item above.
          mCurrentItemToRestoreAfterViewItemJobStep = mView->messageItemBefore( dyingMessage, MessageTypeAny, false );
        }

        Q_ASSERT( (!mCurrentItemToRestoreAfterViewItemJobStep) || mCurrentItemToRestoreAfterViewItemJobStep->isViewable() );
      }

      if (
           dyingMessage->isViewable() &&
           ( ( dyingMessage )->childItemCount() > 0 ) && // has children
           mView->isExpanded( index( dyingMessage, 0 ) ) // is actually expanded
        )
        saveExpandedStateOfSubtree( dyingMessage );

      Item * oldParent = dyingMessage->parent();
      oldParent->takeChildItem( this, dyingMessage );

      // FIXME: This can generate many message movements.. it would be nicer
      //        to start from messages that are higher in the hierarchy so
      //        we would need to move less stuff above.

      if ( oldParent != mRootItem )
        messageDetachedUpdateParentProperties( oldParent, dyingMessage );

      // We might have already removed its parent from the view, so it
      // might already be in the orphan child hash...
      if ( dyingMessage->threadingStatus() == MessageItem::ParentMissing )
        mOrphanChildrenHash->remove( dyingMessage ); // this can turn to a no-op (dyingMessage not present in fact)

    } else {
      // The dying message had no parent: this should happen only if it's already an orphan

      Q_ASSERT( dyingMessage->threadingStatus() == MessageItem::ParentMissing );
      Q_ASSERT( mOrphanChildrenHash->contains( dyingMessage ) );
      Q_ASSERT( dyingMessage != mCurrentItemToRestoreAfterViewItemJobStep );

      mOrphanChildrenHash->remove( dyingMessage );
    }

    if ( mAggregation->threading() != Aggregation::NoThreading )
    {
      // Threading is requested: remove the message from threading caches.

      // Remove from the cache of potential parent items
      mThreadingCacheMessageIdMD5ToMessageItem->remove( dyingMessage->messageIdMD5() );

      // If we also have a cache for subject-based threading then remove the message from there too
      if( mAggregation->threading() == Aggregation::PerfectReferencesAndSubject )
        removeMessageFromSubjectBasedThreadingCache( dyingMessage );

      // If this message wasn't perfectly parented then it might still be in another cache.
      switch( dyingMessage->threadingStatus() )
      {
        case MessageItem::ImperfectParentFound:
        case MessageItem::ParentMissing:
          if ( !dyingMessage->inReplyToIdMD5().isEmpty() )
            mThreadingCacheMessageInReplyToIdMD5ToMessageItem->remove( dyingMessage->inReplyToIdMD5() );
        break;
        default:
          Q_ASSERT( !mThreadingCacheMessageInReplyToIdMD5ToMessageItem->contains( dyingMessage->inReplyToIdMD5(), dyingMessage ) );
          // make gcc happy
        break;
      }
    }

    while ( Item * childItem = dyingMessage->firstChildItem() )
    {
      MessageItem * childMessage = dynamic_cast< MessageItem * >( childItem );
      Q_ASSERT( childMessage );

      dyingMessage->takeChildItem( this, childMessage );

      if ( mAggregation->threading() != Aggregation::NoThreading )
      {
        if ( childMessage->threadingStatus() == MessageItem::PerfectParentFound )
        {
          // If the child message was perfectly parented then now it had
          // lost its perfect parent. Add to the cache of imperfectly parented.
          if ( !childMessage->inReplyToIdMD5().isEmpty() )
          {
            Q_ASSERT( !mThreadingCacheMessageInReplyToIdMD5ToMessageItem->contains( childMessage->inReplyToIdMD5(), childMessage ) );
            mThreadingCacheMessageInReplyToIdMD5ToMessageItem->insert( childMessage->inReplyToIdMD5(), childMessage );
          }
        }
      }

      // Parent is gone
      childMessage->setThreadingStatus( MessageItem::ParentMissing );

      // If the child (or any message in its subtree) is going to be selected,
      // then we must immediately reattach it to a temporary group in order for the
      // selection to be preserved across multiple steps. Otherwise we could end
      // with the child-to-be-selected being non viewable at the end
      // of the view job step. Attach to a temporary group.
      if (
           // child is going to be re-selected
           ( childMessage == mCurrentItemToRestoreAfterViewItemJobStep ) ||
           (
             // there is a message that is going to be re-selected
             mCurrentItemToRestoreAfterViewItemJobStep && 
             // that message is in the childMessage subtree
             mCurrentItemToRestoreAfterViewItemJobStep->hasAncestor( childMessage )
           )
         )
      {
        attachMessageToGroupHeader( childMessage );

        Q_ASSERT( childMessage->isViewable() );
      }

      mOrphanChildrenHash->insert( childMessage, childMessage );
    }

    delete dyingMessage;

    curIndex++;

    // FIXME: Maybe we should check smaller steps here since the
    //        code above can generate large message tree movements
    //        for each single item we sweep in the invalidatedMessages list.
    if ( ( curIndex % mViewItemJobStepMessageCheckCount ) == 0 )
    {
      elapsed = tStart.msecsTo( QTime::currentTime() );
      if ( ( elapsed > mViewItemJobStepChunkTimeout ) || ( elapsed < 0 ) )
      {
        if ( curIndex <= endIndex )
        {
          job->setCurrentIndex( curIndex );
          return ViewItemJobInterrupted;
        }
      }
    }
  }

  // We looped over the entire deleted message list.

  job->setCurrentIndex( endIndex + 1 );

  // A quick last cleaning pass: this is usually very fast so we don't have a real
  // Pass enumeration for it. We just include it as trailer of Pass1Cleanup to be executed
  // when job->currentIndex() > job->endIndex();

  // We move all the messages from the orphan child hash to the unassigned message
  // list and get them ready for the standard Pass2.

  QHash< MessageItem *, MessageItem * >::Iterator it = mOrphanChildrenHash->begin();

  curIndex = 0;

  while ( it != mOrphanChildrenHash->end() )
  {
    mUnassignedMessageListForPass2->append( *it );

    mOrphanChildrenHash->erase( it );

    it = mOrphanChildrenHash->begin();

    // This is still interruptible

    curIndex++;

    // FIXME: We could take "larger" steps here
    if ( ( curIndex % mViewItemJobStepMessageCheckCount ) == 0 )
    {
      elapsed = tStart.msecsTo( QTime::currentTime() );
      if ( ( elapsed > mViewItemJobStepChunkTimeout ) || ( elapsed < 0 ) )
      {
        if ( it != mOrphanChildrenHash->end() )
          return ViewItemJobInterrupted;
      }
    }
  }

  return ViewItemJobCompleted;
}


Model::ViewItemJobResult Model::viewItemJobStepInternalForJobPass1Update( ViewItemJob *job, const QTime &tStart )
{
  Q_ASSERT( mModelForItemFunctions ); // UI must be not disconnected here

  // In this pass we simply update the MessageItem objects that are present in the job.

  // Note that this list in fact contains MessageItem objects (we need dynamic_cast<>).
  QList< ModelInvariantIndex * > * messagesThatNeedUpdate = job->invariantIndexList();

  // We don't shrink the messagesThatNeedUpdate because it's basically an array.
  // It's faster to traverse an array of N entries than to remove K>0 entries
  // one by one and to traverse the remaining N-K entries.

  int elapsed;

  // The begin index of our work
  int curIndex = job->currentIndex();
  // The end index of our work.
  int endIndex = job->endIndex();

  bool viewportNeedsUpdate = false;

  while( curIndex <= endIndex )
  {
    // Get the underlying storage message data...
    MessageItem * message = dynamic_cast< MessageItem * >( messagesThatNeedUpdate->at( curIndex ) );
    // This MUST NOT be null (otherwise we have a bug somewhere in this file).
    Q_ASSERT( message );

    int row = mInvariantRowMapper->modelInvariantIndexToModelIndexRow( message );

    if ( row < 0 )
    {
      // Must have been invalidated (so it's basically about to be deleted)
      Q_ASSERT( !message->isValid() );
      // Skip it here.
      curIndex++;
      continue;
    }

    time_t prevDate = message->date();
    time_t prevMaxDate = message->maxDate();
    bool toDoStatus = message->status().isToAct();

    mStorageModel->updateMessageItemData( message, row );

    int propertyChangeMask = 0;

    if ( prevDate != message->date() )
       propertyChangeMask |= DateChanged;
    if ( prevMaxDate != message->maxDate() )
       propertyChangeMask |= MaxDateChanged;
    if ( toDoStatus != message->status().isToAct() )
       propertyChangeMask |= ActionItemStatusChanged;

    if ( propertyChangeMask )
    {
      // Some message data has changed
      // We could emit dataChanged() so the viewport would be updated...
      // but this is AGAIN a huge performance cost. Since we don't actually
      // use the standard painting code nothing will screw up if we don't
      // emit this signal (I hope.. :D). We just set a flag that will cause
      // us to update the viewport on exit instead.
      //
      // ... hm.. now that I think of it.. what the heck is the meaning of
      // topLeft and bottomRight parameters of dataChanged() in a tree ?
      // I guess that it should be always topLeft == bottomRight == itemIndex...
      // But anyway, the view jumps like crazy when this signal is emitted.

      // QModelIndex idx = index( message, 0 );
      // emit dataChanged( idx, idx );

      viewportNeedsUpdate = true;

      // now we need to handle the changes that might cause re-grouping/re-sorting
      // and propagate them to the parents.

      Item * pParent = message->parent();

      if ( pParent && ( pParent != mRootItem ) )
      {
        // The following function will return true if itemParent may be affected by the change.
        // If the itemParent isn't affected, we stop climbing.
        if ( handleItemPropertyChanges( propertyChangeMask, pParent, message ) )
        {
          Q_ASSERT( message->parent() ); // handleItemPropertyChanges() must never leave an item detached

          // Note that actually message->parent() may be different than pParent since
          // handleItemPropertyChanges() may have re-grouped it.

          // Time to propagate up.
          propagateItemPropertiesToParent( message );
        }
      } // else there is no parent so the item isn't attached to the view: re-grouping/re-sorting not needed.
    } // else message data didn't change an there is nothing interesting to do

    // (re-)apply the filter, if needed
    if ( mFilter && message->isViewable() )
    {
      // We explicitly avoid handling one particular case: the filter
      // set to "new messages only". This is because simply clicking
      // on the message would then remove the "new" status and the message
      // would disappear from the list. This is not what we want :)
      if ( ! (mFilter->statusMask() & mCachedNewStatusBits ) )
      {
        // In all the other cases we (re-)apply the filter to the topmost subtree that this message is in.
        Item * pTopMostNonRoot = message->topmostNonRoot();

        Q_ASSERT( pTopMostNonRoot );
        Q_ASSERT( pTopMostNonRoot != mRootItem );
        Q_ASSERT( pTopMostNonRoot->parent() == mRootItem );

        // FIXME: The call below works, but it's expensive when we are updating
        //        a lot of items with filtering enabled. This is because the updated
        //        items are likely to be in the same subtree which we then filter multiple times.
        //        A point for us is that when filtering there shouldn't be really many
        //        items in the view so the user isn't going to update a lot of them at once...
        //        Well... anyway, the alternative would be to write yet another
        //        specialized routine that would update only the "message" item
        //        above and climb up eventually hiding parents (without descending the sibling subtrees again).
        //        If people complain about performance in this particular case I'll consider that solution.

        applyFilterToSubtree( pTopMostNonRoot, QModelIndex() );
      }
    } // otherwise there is no filter or the item isn't viewable: very likely
      // left detached while propagating property changes. Will filter it
      // on reattach.

    // Done updating this message

    curIndex++;

    // FIXME: Maybe we should check smaller steps here since the
    //        code above can generate large message tree movements
    //        for each single item we sweep in the messagesThatNeedUpdate list.
    if ( ( curIndex % mViewItemJobStepMessageCheckCount ) == 0 )
    {
      elapsed = tStart.msecsTo( QTime::currentTime() );
      if ( ( elapsed > mViewItemJobStepChunkTimeout ) || ( elapsed < 0 ) )
      {
        if ( curIndex <= endIndex )
        {
          job->setCurrentIndex( curIndex );
          if ( viewportNeedsUpdate )
            mView->viewport()->update();
          return ViewItemJobInterrupted;
        }
      }
    }
  }

  if ( viewportNeedsUpdate )
    mView->viewport()->update();
  return ViewItemJobCompleted;
}


Model::ViewItemJobResult Model::viewItemJobStepInternalForJob( ViewItemJob *job, const QTime &tStart )
{
  // This function does a timed chunk of work for a single Fill View job.
  // It attempts to process messages until a timeout forces it to return to the caller.

  // A macro would improve readability here but since this is a good point
  // to place debugger breakpoints then we need it explicited.
  // A (template) helper would need to pass many parameters and would not be inlined...

  int elapsed;

  if ( job->currentPass() == ViewItemJob::Pass1Fill )
  {
    // We're in Pass1Fill of the job.
    switch ( viewItemJobStepInternalForJobPass1Fill( job, tStart ) )
    {
      case ViewItemJobInterrupted:
        // current job interrupted by timeout: propagate status to caller
        return ViewItemJobInterrupted;
      break;
      case ViewItemJobCompleted:
        // pass 1 has been completed
        job->setCurrentPass( ViewItemJob::Pass2 );
        job->setStartIndex( 0 );
        job->setEndIndex( mUnassignedMessageListForPass2->count() - 1 );
        // take care of small jobs which never timeout by themselves because
        // of a small number of messages. At the end of each job check
        // the time used and if we're timeoutting and there is another job
        // then interrupt.
        elapsed = tStart.msecsTo( QTime::currentTime() );
        if ( ( elapsed > mViewItemJobStepChunkTimeout ) || ( elapsed < 0 ) )
        {
          return ViewItemJobInterrupted;
        } // else proceed with the next pass
      break;
      default:
        // This is *really* a BUG
        kWarning() << "ERROR: Model::viewItemJobStepInternalForJobPass1Fill() returned an invalid result";
        Q_ASSERT( false );
      break;
    }
  } else if ( job->currentPass() == ViewItemJob::Pass1Cleanup )
  {
    // We're in Pass1Cleanup of the job.
    switch ( viewItemJobStepInternalForJobPass1Cleanup( job, tStart ) )
    {
      case ViewItemJobInterrupted:
        // current job interrupted by timeout: propagate status to caller
        return ViewItemJobInterrupted;
      break;
      case ViewItemJobCompleted:
        // pass 1 has been completed
        job->setCurrentPass( ViewItemJob::Pass2 );
        job->setStartIndex( 0 );
        job->setEndIndex( mUnassignedMessageListForPass2->count() - 1 );
        // take care of small jobs which never timeout by themselves because
        // of a small number of messages. At the end of each job check
        // the time used and if we're timeoutting and there is another job
        // then interrupt.
        elapsed = tStart.msecsTo( QTime::currentTime() );
        if ( ( elapsed > mViewItemJobStepChunkTimeout ) || ( elapsed < 0 ) )
        {
          return ViewItemJobInterrupted;
        } // else proceed with the next pass
      break;
      default:
        // This is *really* a BUG
        kWarning() << "ERROR: Model::viewItemJobStepInternalForJobPass1Cleanup() returned an invalid result";
        Q_ASSERT( false );
      break;
    }
  } else if ( job->currentPass() == ViewItemJob::Pass1Update )
  {
    // We're in Pass1Update of the job.
    switch ( viewItemJobStepInternalForJobPass1Update( job, tStart ) )
    {
      case ViewItemJobInterrupted:
        // current job interrupted by timeout: propagate status to caller
        return ViewItemJobInterrupted;
      break;
      case ViewItemJobCompleted:
        // pass 1 has been completed
        // Since Pass2, Pass3 and Pass4 are empty for an Update operation
        // we simply skip them. (TODO: Triple-verify this assertion...).
        job->setCurrentPass( ViewItemJob::Pass5 );
        job->setStartIndex( 0 );
        job->setEndIndex( mGroupHeadersThatNeedUpdate->count() - 1 );
        // take care of small jobs which never timeout by themselves because
        // of a small number of messages. At the end of each job check
        // the time used and if we're timeoutting and there is another job
        // then interrupt.
        elapsed = tStart.msecsTo( QTime::currentTime() );
        if ( ( elapsed > mViewItemJobStepChunkTimeout ) || ( elapsed < 0 ) )
        {
          return ViewItemJobInterrupted;
        } // else proceed with the next pass
      break;
      default:
        // This is *really* a BUG
        kWarning() << "ERROR: Model::viewItemJobStepInternalForJobPass1Cleanup() returned an invalid result";
        Q_ASSERT( false );
      break;
    }
  }

  // Pass1Fill/Pass1Cleanup/Pass1Update has been already completed.

  if ( job->currentPass() == ViewItemJob::Pass2 )
  {
    // We're in Pass2 of the job.
    switch ( viewItemJobStepInternalForJobPass2( job, tStart ) )
    {
      case ViewItemJobInterrupted:
        // current job interrupted by timeout: propagate status to caller
        return ViewItemJobInterrupted;
      break;
      case ViewItemJobCompleted:
        // pass 2 has been completed
        job->setCurrentPass( ViewItemJob::Pass3 );
        job->setStartIndex( 0 );
        job->setEndIndex( mUnassignedMessageListForPass3->count() - 1 );
        // take care of small jobs which never timeout by themselves because
        // of a small number of messages. At the end of each job check
        // the time used and if we're timeoutting and there is another job
        // then interrupt.
        elapsed = tStart.msecsTo( QTime::currentTime() );
        if ( ( elapsed > mViewItemJobStepChunkTimeout ) || ( elapsed < 0 ) )
          return ViewItemJobInterrupted;
        // else proceed with the next pass
      break;
      default:
        // This is *really* a BUG
        kWarning() << "ERROR: Model::viewItemJobStepInternalForJobPass2() returned an invalid result";
        Q_ASSERT( false );
      break;
    }
  }

  if ( job->currentPass() == ViewItemJob::Pass3 )
  {
    // We're in Pass3 of the job.
    switch ( viewItemJobStepInternalForJobPass3( job, tStart ) )
    {
      case ViewItemJobInterrupted:
        // current job interrupted by timeout: propagate status to caller
        return ViewItemJobInterrupted;
      break;
      case ViewItemJobCompleted:
        // pass 3 has been completed
        job->setCurrentPass( ViewItemJob::Pass4 );
        job->setStartIndex( 0 );
        job->setEndIndex( mUnassignedMessageListForPass4->count() - 1 );
        // take care of small jobs which never timeout by themselves because
        // of a small number of messages. At the end of each job check
        // the time used and if we're timeoutting and there is another job
        // then interrupt.
        elapsed = tStart.msecsTo( QTime::currentTime() );
        if ( ( elapsed > mViewItemJobStepChunkTimeout ) || ( elapsed < 0 ) )
          return ViewItemJobInterrupted;
        // else proceed with the next pass
      break;
      default:
        // This is *really* a BUG
        kWarning() << "ERROR: Model::viewItemJobStepInternalForJobPass3() returned an invalid result";
        Q_ASSERT( false );
      break;
    }
  }

  if ( job->currentPass() == ViewItemJob::Pass4 )
  {
    // We're in Pass4 of the job.
    switch ( viewItemJobStepInternalForJobPass4( job, tStart ) )
    {
      case ViewItemJobInterrupted:
        // current job interrupted by timeout: propagate status to caller
        return ViewItemJobInterrupted;
      break;
      case ViewItemJobCompleted:
        // pass 4 has been completed
        job->setCurrentPass( ViewItemJob::Pass5 );
        job->setStartIndex( 0 );
        job->setEndIndex( mGroupHeadersThatNeedUpdate->count() - 1 );
        // take care of small jobs which never timeout by themselves because
        // of a small number of messages. At the end of each job check
        // the time used and if we're timeoutting and there is another job
        // then interrupt.
        elapsed = tStart.msecsTo( QTime::currentTime() );
        if ( ( elapsed > mViewItemJobStepChunkTimeout ) || ( elapsed < 0 ) )
          return ViewItemJobInterrupted;
        // else proceed with the next pass
      break;
      default:
        // This is *really* a BUG
        kWarning() << "ERROR: Model::viewItemJobStepInternalForJobPass4() returned an invalid result";;
        Q_ASSERT( false );
      break;
    }
  }

  // Pass4 has been already completed. Proceed to Pass5.
  return viewItemJobStepInternalForJobPass5( job, tStart );
}

// FIXME: Remove for release
#define WANT_FILL_VIEW_STATS

Model::ViewItemJobResult Model::viewItemJobStepInternal()
{
  // This function does a timed chunk of work in our View Fill operation.
  // It attempts to do processing until it either runs out of jobs
  // to be done or a timeout forces it to interrupt and jump back to the caller.

  QTime tStart = QTime::currentTime();
  int elapsed;

  while( !mViewItemJobs->isEmpty() )
  {
    // Have a job to do.
    ViewItemJob * job = mViewItemJobs->first();

#ifdef WANT_FILL_VIEW_STATS
    // BEGIN STATS (REMOVE FOR RELEASE)
    static int statsTotalMessages = 0;
    static QTime tStatsTotalStart;
    if ( job->currentPass() == ViewItemJob::Pass1Fill && job->currentIndex() == job->startIndex() )
    {
      statsTotalMessages = job->endIndex() - job->startIndex();
      tStatsTotalStart = QTime::currentTime();
    }
    // END STATS (REMOVE FOR RELEASE)
#endif //WANT_FILL_VIEW_STATS

    mViewItemJobStepIdleInterval = job->idleInterval();
    mViewItemJobStepChunkTimeout = job->chunkTimeout();
    mViewItemJobStepMessageCheckCount = job->messageCheckCount();

    if ( job->disconnectUI() )
    {
      mModelForItemFunctions = 0; // disconnect the UI for this job
      Q_ASSERT( mLoading ); // this must be true in the first job
      // FIXME: Should assert yet more that this is the very first job for this StorageModel
      //        Asserting only mLoading is not enough as we could be using a two-jobs loading strategy
      //        or this could be a job enqueued before the first job has completed.
    }

    switch( viewItemJobStepInternalForJob( job, tStart ) )
    {
      case ViewItemJobInterrupted:
      {
        // current job interrupted by timeout: will propagate status to caller
        // but before this, give some feedback to the user

        // FIXME: This is now inaccurate, think of something else
        switch( job->currentPass() )
        {
          case ViewItemJob::Pass1Fill:
          case ViewItemJob::Pass1Cleanup:
          case ViewItemJob::Pass1Update:
            KPIM::BroadcastStatus::instance()->setStatusMsg( i18n("Processed %1 Messages of %2", job->currentIndex() - job->startIndex(), ( job->endIndex() - job->startIndex() ) + 1 ) );
          break;
          case ViewItemJob::Pass2:
            KPIM::BroadcastStatus::instance()->setStatusMsg( i18n("Threaded %1 Messages of %2", job->currentIndex() - job->startIndex(), ( job->endIndex() - job->startIndex() ) + 1 ) );
          break;
          case ViewItemJob::Pass3:
            KPIM::BroadcastStatus::instance()->setStatusMsg( i18n("Threaded %1 Messages of %2", job->currentIndex() - job->startIndex(), ( job->endIndex() - job->startIndex() ) + 1 ) );
          break;
          case ViewItemJob::Pass4:
            KPIM::BroadcastStatus::instance()->setStatusMsg( i18n("Grouped %1 Threads of %2", job->currentIndex() - job->startIndex(), ( job->endIndex() - job->startIndex() ) + 1 ) );
          break;
          case ViewItemJob::Pass5:
            KPIM::BroadcastStatus::instance()->setStatusMsg( i18n("Updated %1 Groups of %2", job->currentIndex() - job->startIndex(), ( job->endIndex() - job->startIndex() ) + 1 ) );
          break;
        }
        return ViewItemJobInterrupted;
      }
      break;
      case ViewItemJobCompleted:

#ifdef WANT_FILL_VIEW_STATS
        // BEGIN STATS (REMOVE FOR RELEASE)
        {
          int msecs = tStatsTotalStart.msecsTo( QTime::currentTime() );
          double dMsgSec = (statsTotalMessages * 1000.0) / msecs;
          kDebug() << "FILLED THE VIEW AT " << dMsgSec << " MSG/SEC";
        }
        // END STATS (REMOVE FOR RELEASE)
#endif //WANT_FILL_VIEW_STATS

        // If this job worked with a disconnected UI, emit layoutChanged()
        // to reconnect it. We go back to normal operation now.
        if ( job->disconnectUI() )
        {
          mModelForItemFunctions = this;
          // This call would destroy the expanded state of items.
          // This is why when mModelForItemFunctions was 0 we didn't actually expand them
          // but we just set a "ExpandNeeded" mark...
          kDebug() << "Emitting layoutChanged()";
          mView->modelAboutToEmitLayoutChanged();
          emit layoutChanged();
          mView->modelEmittedLayoutChanged();
          kDebug() << "Emitted layoutChanged()";

#ifdef WANT_FILL_VIEW_STATS
          // BEGIN STATS (REMOVE FOR RELEASE)
          tStatsTotalStart = QTime::currentTime();
          // END STATS (REMOVE FOR RELEASE)
#endif //WANT_FILL_VIEW_STATS

          // expand all the items that need it in a single sweep

          // FIXME: This takes quite a lot of time, it could be made an interruptible job

          QList< Item * > * rootChildItems = mRootItem->childItems();
          if ( rootChildItems )
          {
            for ( QList< Item * >::Iterator it = rootChildItems->begin(); it != rootChildItems->end() ;++it )
            {
              if ( ( *it )->initialExpandStatus() == Item::ExpandNeeded )
                syncExpandedStateOfSubtree( *it );
            }
          }

#ifdef WANT_FILL_VIEW_STATS
          // BEGIN STATS (REMOVE FOR RELEASE)
          kDebug() << "EXPANDING THE TREE TOOK " << tStatsTotalStart.msecsTo( QTime::currentTime() ) << " MSECS";
          // END STATS (REMOVE FOR RELEASE)
#endif //WANT_FILL_VIEW_STATS

        }

        // this job has been completed
        delete mViewItemJobs->takeFirst();
        // take care of small jobs which never timeout by themselves because
        // of a small number of messages. At the end of each job check
        // the time used and if we're timeoutting and there is another job
        // then interrupt.
        elapsed = tStart.msecsTo( QTime::currentTime() );
        if ( ( elapsed > mViewItemJobStepChunkTimeout ) || ( elapsed < 0 ) )
        {
          if ( !mViewItemJobs->isEmpty() )
            return ViewItemJobInterrupted;
          // else it's completed in fact
        } // else proceed with the next job
      break;
      default:
        // This is *really* a BUG
        kWarning() << "ERROR: Model::viewItemJobStepInternalForJob() returned an invalid result";
        Q_ASSERT( false );
      break;
    }
  }

  // no more jobs

  KPIM::BroadcastStatus::instance()->setStatusMsg( i18nc( "@info:status Finished view fill", "Ready" ) );

  return ViewItemJobCompleted;
}


void Model::viewItemJobStep()
{
  // A single step in the View Fill operation.
  // This function wraps viewItemJobStepInternal() which does the step job
  // and either completes it or stops because of a timeout.
  // If the job is stopped then we start a zero-msecs timer to call us
  // back and resume the job. Otherwise we're just done.

  mViewItemJobStepStartTime = ::time( 0 );

  if( mFillStepTimer->isActive() )
    mFillStepTimer->stop();

  if ( !mStorageModel )
    return; // nothing more to do


  // Save the current item in the view as our process may
  // cause items to be reparented (and QTreeView will forget the current item in the meantime).
  // This machinery is also needed when we're about to remove items from the view in
  // a cleanup job: we'll be trying to set as current the item after the one removed.

  QModelIndex currentIndexBeforeStep = mView->currentIndex();
  Item * currentItemBeforeStep = currentIndexBeforeStep.isValid() ?
      static_cast< Item * >( currentIndexBeforeStep.internalPointer() ) : 0;

  // mCurrentItemToRestoreAfterViewItemJobStep will be zeroed out if it's killed
  mCurrentItemToRestoreAfterViewItemJobStep = currentItemBeforeStep;

  // Save the current item position in the viewport as QTreeView fails to keep
  // the current item in the sample place when items are added or removed...
  QRect rectBeforeViewItemJobStep;

  // This is generally SLOW AS HELL...
  if ( mCurrentItemToRestoreAfterViewItemJobStep )
    rectBeforeViewItemJobStep = mView->visualRect( currentIndexBeforeStep );

  // FIXME: If the current item is NOT in the view, preserve the position
  //        of the top visible item. This will make the view move yet less.

  // Insulate the View from (very likely spurious) "currentChanged()" signals.
  mView->ignoreCurrentChanges( true );

  // And go to real work.
  switch( viewItemJobStepInternal() )
  {
    case ViewItemJobInterrupted:
      // Operation timed out, need to resume in a while
      if ( !mInLengthyJobBatch )
      {
        mInLengthyJobBatch = true;
        mView->modelJobBatchStarted();
      }
      mFillStepTimer->start( mViewItemJobStepIdleInterval ); // this is a single shot timer connected to viewItemJobStep()
      // and go dealing with current/selection out of the switch.
    break;
    case ViewItemJobCompleted:
      // done :)

      Q_ASSERT( mModelForItemFunctions ); // UI must be no (longer) disconnected in this state

      // Ask the view to remove the eventual busy indications
      if ( mInLengthyJobBatch )
      {
        mInLengthyJobBatch = false;
        mView->modelJobBatchTerminated();
      }

      if ( mLoading )
      {
        mLoading = false;
        mView->modelFinishedLoading();
      }

      // Apply pre-selection, if any
      if ( mPreSelectionMode != PreSelectNone )
      {
        mView->ignoreCurrentChanges( false );

        bool bSelectionDone = false;

        switch( mPreSelectionMode )
        {
          case PreSelectLastSelected:
            // fall down
          break;
          case PreSelectFirstUnread:
            bSelectionDone = mView->selectFirstMessageItem( MessageTypeUnreadOnly, false ); // don't center
          break;
          case PreSelectFirstUnreadCentered:
            bSelectionDone = mView->selectFirstMessageItem( MessageTypeUnreadOnly, true ); // center
          break;
          case PreSelectFirstNew:
            bSelectionDone = mView->selectFirstMessageItem( MessageTypeNewOnly, false ); // don't center
            if ( !bSelectionDone ) // try to fallback to unread
              bSelectionDone = mView->selectFirstMessageItem( MessageTypeUnreadOnly, false ); // don't center
          break;
          case PreSelectFirstNewCentered:
            bSelectionDone = mView->selectFirstMessageItem( MessageTypeNewOnly, true ); // center
            if ( !bSelectionDone ) // try to fallback to unread
              bSelectionDone = mView->selectFirstMessageItem( MessageTypeUnreadOnly, true ); // center
          break;
          case PreSelectFirstNewOrUnread:
            bSelectionDone = mView->selectFirstMessageItem( MessageTypeNewOrUnreadOnly, false ); // don't center
          break;
          case PreSelectFirstNewOrUnreadCentered:
            bSelectionDone = mView->selectFirstMessageItem( MessageTypeNewOrUnreadOnly, true ); // center
          break;
          case PreSelectNone:
            // deal with selection below
          break;
          default:
            kWarning() << "ERROR: Unrecognized pre-selection mode " << (int)mPreSelectionMode;
          break;
        }

        if ( ( !bSelectionDone ) && ( mPreSelectionMode != PreSelectNone ) )
        {
          // fallback to last selected, if possible
          if ( mLastSelectedMessageInFolder ) // we found it in the loading process: select and jump out
          {
            mView->setCurrentMessageItem( mLastSelectedMessageInFolder );
            bSelectionDone = true;
          }
        }

        mUniqueIdOfLastSelectedMessageInFolder = 0;
        mLastSelectedMessageInFolder = 0;
        mPreSelectionMode = PreSelectNone;

        if ( bSelectionDone )
          return; // already taken care of current / selection
      }
      // deal with current/selection out of the switch

    break;
    default:
      // This is *really* a BUG
      kWarning() << "ERROR: Model::viewItemJobStepInternal() returned an invalid result";
      Q_ASSERT( false );
    break;
  }

  // Everything else here deals with the selection

  // If UI is disconnected then we don't have anything else to do here
  if ( !mModelForItemFunctions )
  {
    mView->ignoreCurrentChanges( false );
    return;
  }

  // Restore current/selection and/or scrollbar position

  if ( mCurrentItemToRestoreAfterViewItemJobStep )
  {
    bool stillIgnoringCurrentChanges = true;

    // If the assert below fails then the previously current item got detached
    // and didn't get reattached in the step: this should never happen.
    Q_ASSERT( mCurrentItemToRestoreAfterViewItemJobStep->isViewable() );

    // Check if the current item changed
    QModelIndex currentIndexAfterStep = mView->currentIndex();
    Item * currentAfterStep = currentIndexAfterStep.isValid() ?
        static_cast< Item * >( currentIndexAfterStep.internalPointer() ) : 0;

    if ( mCurrentItemToRestoreAfterViewItemJobStep != currentAfterStep )
    {
      // QTreeView lost the current item...
      if ( mCurrentItemToRestoreAfterViewItemJobStep != currentItemBeforeStep )
      {
         // Some view job code expects us to actually *change* the current item.
         // This is done by the cleanup step which removes items and tries
         // to set as current the item *after* the removed one, if possible.
         // We need the view to handle the change though.
         stillIgnoringCurrentChanges = false;
         mView->ignoreCurrentChanges( false );
      } else {
         // we just have to restore the old current item. The code
         // outside shouldn't have noticed that we lost it (e.g. the message viewer
         // still should have the old message opened). So we don't need to
         // actually notify the view of the restored setting.
      }
      // Restore it
      mView->setCurrentIndex( index( mCurrentItemToRestoreAfterViewItemJobStep, 0 ) );
    } else {
      // The item we're expected to set as current is already current
      if ( mCurrentItemToRestoreAfterViewItemJobStep != currentItemBeforeStep )
      {
        // But we have changed it in the job step.
        // This means that: we have deleted the current item and chosen a
        // new candidate as current but Qt also has chosen it as candidate
        // and already made it current. The problem is that (as of Qt 4.4)
        // it probably didn't select it.
        if ( !mView->selectionModel()->hasSelection() )
        {
          stillIgnoringCurrentChanges = false;
          mView->ignoreCurrentChanges( false );

          QItemSelection selection;
          selection.append( QItemSelectionRange( index( mCurrentItemToRestoreAfterViewItemJobStep, 0 ) ) );
          mView->selectionModel()->select( selection, QItemSelectionModel::Select | QItemSelectionModel::Rows );
        }
      }
    }

    // FIXME: If it was selected before the change, then re-select it (it may happen that it's not)

    QRect rectAfterViewItemJobStep = mView->visualRect( index( mCurrentItemToRestoreAfterViewItemJobStep, 0 ) );
    if ( rectBeforeViewItemJobStep.y() != rectAfterViewItemJobStep.y() )
    {
      // QTreeView lost its position...
      mView->verticalScrollBar()->setValue( mView->verticalScrollBar()->value() + rectAfterViewItemJobStep.y() - rectBeforeViewItemJobStep.y() );
    }

    // and kill the insulation, if not yet done
    if ( stillIgnoringCurrentChanges )
      mView->ignoreCurrentChanges( false );

    return;
  }

  // Either there was no current item before, or it was lost in a cleanup step and another candidate for
  // current item couldn't be found (possibly empty view)
  mView->ignoreCurrentChanges( false );

  if ( currentItemBeforeStep )
  {
    // lost in a cleanup..
    // tell the view that we have a new current, this time with no insulation
    mView->slotSelectionChanged( QItemSelection(), QItemSelection() );
  }
}

void Model::slotStorageModelRowsInserted( const QModelIndex &parent, int from, int to )
{
  if ( parent.isValid() )
    return; // ugh... should never happen

  Q_ASSERT( from <= to );

  int count = ( to - from ) + 1;

  mInvariantRowMapper->modelRowsInserted( from, count );

  // look if no current job is in the middle

  int jobCount = mViewItemJobs->count();

  for ( int idx = 0; idx < jobCount; idx++ )
  {
    ViewItemJob * job = mViewItemJobs->at( idx );
    if ( job->currentPass() == ViewItemJob::Pass1Fill )
    {
      //
      // The following cases are possible:
      //
      //               from  to
      //                 |    |                              -> shift up job
      //               from             to
      //                 |              |                    -> shift up job
      //               from                            to
      //                 |                             |     -> shift up job
      //                           from   to
      //                             |     |                 -> split job
      //                           from                to
      //                             |                 |     -> split job
      //                                     from      to
      //                                       |       |     -> job unaffected
      //
      //
      // FOLDER
      // |-------------------------|---------|--------------|
      // 0                   currentIndex endIndex         count
      //                           +-- job --+

      //

      if ( from > job->endIndex() )
      {
        // The change is completely above the job, the job is not affected
      } else if( from > job->currentIndex() ) // and from <= job->endIndex()
      {
        // The change starts in the middle of the job in a way that it must be splitted in two.
        // The first part is unaffected by the shift and ranges from job->currentIndex() to from - 1.
        // We use the existing job for this.
        job->setEndIndex( from - 1 );

        Q_ASSERT( job->currentIndex() <= job->endIndex() );

        // The second part would range from "from" to job->endIndex() but must
        // be shifted up by count. We add a new job for this.
        ViewItemJob * newJob = new ViewItemJob( from + count, job->endIndex() + count, job->chunkTimeout(), job->idleInterval(), job->messageCheckCount() );

        Q_ASSERT( newJob->currentIndex() <= newJob->endIndex() );

        idx++; // we can skip this job in the loop, it's already ok
        jobCount++; // and our range increases by one.
        mViewItemJobs->insert( idx, newJob );

      } else {
        // The change starts below (or exactly on the beginning of) the job.
        // The job must be shifted up.
        job->setCurrentIndex( job->currentIndex() + count );
        job->setEndIndex( job->endIndex() + count );

        Q_ASSERT( job->currentIndex() <= job->endIndex() );
      }
    } else {
      // The job is a cleanup or in a later pass: the storage has been already accessed
      // and the messages created... no need to care anymore: the invariant row mapper will do the job.
    }
  }

  bool newJobNeeded = true;

  // Try to attach to an existing fill job, if any.
  // To enforce consistency we can attach only if the Fill job
  // is the last one in the list (might be eventually *also* the first,
  // and even being already processed but we must make sure that there
  // aren't jobs _after_ it).
  if ( jobCount > 0 )
  {
    ViewItemJob * job = mViewItemJobs->at( jobCount - 1 );
    if ( job->currentPass() == ViewItemJob::Pass1Fill )
    {
      if (
           // The job ends just before the added rows
           ( from == ( job->endIndex() + 1 ) ) &&
           // The job didn't reach the end of Pass1Fill yet
           ( job->currentIndex() <= job->endIndex() )
         )
      {
        // We can still attach this :)
        job->setEndIndex( to );
        Q_ASSERT( job->currentIndex() <= job->endIndex() );
        newJobNeeded = false;
      }
    }
  }

  if ( newJobNeeded )
  {
    // FIXME: Should take timing options from aggregation here ?
    ViewItemJob * job = new ViewItemJob( from, to, 100, 50, 10 );
    mViewItemJobs->append( job );
  }

  if ( !mFillStepTimer->isActive() )
    mFillStepTimer->start( mViewItemJobStepIdleInterval );
}

void Model::slotStorageModelRowsRemoved( const QModelIndex &parent, int from, int to )
{
  // This is called when the underlying StorageModel emits the rowsRemoved signal.

  if ( parent.isValid() )
    return; // ugh... should never happen

  // look if no current job is in the middle

  Q_ASSERT( from <= to );

  int count = ( to - from ) + 1;

  int jobCount = mViewItemJobs->count();

  for ( int idx = 0; idx < jobCount; idx++ )
  {
    ViewItemJob * job = mViewItemJobs->at( idx );
    if ( job->currentPass() == ViewItemJob::Pass1Fill )
    {
      //
      // The following cases are possible:
      //
      //               from  to
      //                 |    |                              -> shift down job
      //               from             to
      //                 |              |                    -> shift down and crop job
      //               from                            to
      //                 |                             |     -> kill job
      //                           from   to
      //                             |     |                 -> split job, crop and shift
      //                           from                to
      //                             |                 |     -> crop job
      //                                     from      to
      //                                       |       |     -> job unaffected
      //
      //
      // FOLDER
      // |-------------------------|---------|--------------|
      // 0                   currentIndex endIndex         count
      //                           +-- job --+

      //

      if ( from > job->endIndex() )
      {
        // The change is completely above the job, the job is not affected
      } else if( from > job->currentIndex() ) // and from <= job->endIndex()
      {
        // The change starts in the middle of the job and ends in the middle or after the job.

        // The first part is unaffected by the shift and ranges from job->currentIndex() to from - 1
        // We use the existing job for this.
        job->setEndIndex( from - 1 ); // stop before the first removed row

        Q_ASSERT( job->currentIndex() <= job->endIndex() );

        if ( to < job->endIndex() )
        {
          // The change ends inside the job and a part of it can be completed.
          // We create a new job for the shifted remaining part. It would actually
          // range from to + 1 up to job->endIndex(), but we need to shift it down by count.
          // since count = ( to - from ) + 1 so from = to + 1 - count

          ViewItemJob * newJob = new ViewItemJob( from, job->endIndex() - count, job->chunkTimeout(), job->idleInterval(), job->messageCheckCount() );

          Q_ASSERT( newJob->currentIndex() < newJob->endIndex() );

          idx++; // we can skip this job in the loop, it's already ok
          jobCount++; // and our range increases by one.
          mViewItemJobs->insert( idx, newJob );
        } // else the change includes completely the end of the job and no other part of it can be completed.
      } else {
        // The change starts below (or exactly on the beginning of) the job. ( from <= job->currentIndex() )
        if ( to >= job->endIndex() )
        {
          // The change completely covers the job: kill it
          delete job;
          mViewItemJobs->removeAt( idx );
          idx--;
          jobCount--;
        } else if ( to >= job->currentIndex() )
        {
          // The change partially covers the job. Only a part of it can be completed
          // and it must be shifted down. It would actually
          // range from to + 1 up to job->endIndex(), but we need to shift it down by count.
          // since count = ( to - from ) + 1 so from = to + 1 - count
          job->setCurrentIndex( from );
          job->setEndIndex( job->endIndex() - count );

          Q_ASSERT( job->currentIndex() <= job->endIndex() );
        } else {
          // The change is completely below the job: it must be shifted down.
          job->setCurrentIndex( job->currentIndex() - count );
          job->setEndIndex( job->endIndex() - count );
        }
      }
    } else {
      // The job is a cleanup or in a later pass: the storage has been already accessed
      // and the messages created... no need to care: we will invalidate the messages in a while.
    }
  }

  // This will invalidate the ModelInvariantIndex-es that have been removed and return
  // them all in a nice list that we can feed to a view removal job.
  QList< ModelInvariantIndex * > * invalidatedIndexes = mInvariantRowMapper->modelRowsRemoved( from, count );

  if ( invalidatedIndexes )
  {
    // Try to attach to an existing cleanup job, if any.
    // To enforce consistency we can attach only if the Cleanup job
    // is the last one in the list (might be eventually *also* the first,
    // and even being already processed but we must make sure that there
    // aren't jobs _after_ it).
    if ( jobCount > 0 )
    {
      ViewItemJob * job = mViewItemJobs->at( jobCount - 1 );
      if ( job->currentPass() == ViewItemJob::Pass1Cleanup )
      {
        if ( ( job->currentIndex() <= job->endIndex() ) && job->invariantIndexList() )
        {
          //kDebug() << "Appending " << invalidatedIndexes->count() << " invalidated indexes to existing cleanup job" << endl;
          // We can still attach this :)
          *( job->invariantIndexList() ) += *invalidatedIndexes;
          job->setEndIndex( job->endIndex() + invalidatedIndexes->count() );
          delete invalidatedIndexes;
          invalidatedIndexes = 0;
        }
      }
    }

    if ( invalidatedIndexes )
    {
      // Didn't append to any existing cleanup job.. create a new one

      //kDebug() << "Creating new cleanup job for " << invalidatedIndexes->count() << " invalidated indexes" << endl;
      // FIXME: Should take timing options from aggregation here ?
      ViewItemJob * job = new ViewItemJob( ViewItemJob::Pass1Cleanup, invalidatedIndexes, 100, 50, 10 );
      mViewItemJobs->append( job );
    }

    if ( !mFillStepTimer->isActive() )
      mFillStepTimer->start( mViewItemJobStepIdleInterval );
  }
}

void Model::slotStorageModelLayoutChanged()
{
  // need to reset everything...
  setStorageModel( mStorageModel );
}

void Model::slotStorageModelDataChanged( const QModelIndex &fromIndex, const QModelIndex &toIndex )
{
  Q_ASSERT( mStorageModel ); // must exist (and be the sender of the signal connected to this slot)

  int from = fromIndex.row();
  int to = toIndex.row();

  Q_ASSERT( from <= to );

  int count = ( to - from ) + 1;

  int jobCount = mViewItemJobs->count();

  // This will find out the ModelInvariantIndex-es that need an update and will return
  // them all in a nice list that we can feed to a view removal job.
  QList< ModelInvariantIndex * > * indexesThatNeedUpdate = mInvariantRowMapper->modelIndexRowRangeToModelInvariantIndexList( from, count );

  if ( indexesThatNeedUpdate )
  {
    // Try to attach to an existing update job, if any.
    // To enforce consistency we can attach only if the Update job
    // is the last one in the list (might be eventually *also* the first,
    // and even being already processed but we must make sure that there
    // aren't jobs _after_ it).
    if ( jobCount > 0 )
    {
      ViewItemJob * job = mViewItemJobs->at( jobCount - 1 );
      if ( job->currentPass() == ViewItemJob::Pass1Update )
      {
        if ( ( job->currentIndex() <= job->endIndex() ) && job->invariantIndexList() )
        {
          // We can still attach this :)
          *( job->invariantIndexList() ) += *indexesThatNeedUpdate;
          job->setEndIndex( job->endIndex() + indexesThatNeedUpdate->count() );
          delete indexesThatNeedUpdate;
          indexesThatNeedUpdate = 0;
        }
      }
    }

    if ( indexesThatNeedUpdate )
    {
      // Didn't append to any existing update job.. create a new one
      // FIXME: Should take timing options from aggregation here ?
      ViewItemJob * job = new ViewItemJob( ViewItemJob::Pass1Update, indexesThatNeedUpdate, 100, 50, 10 );
      mViewItemJobs->append( job );
    }

    if ( !mFillStepTimer->isActive() )
      mFillStepTimer->start( mViewItemJobStepIdleInterval );
  }

}

Qt::ItemFlags Model::flags( const QModelIndex &index ) const
{
  if ( !index.isValid() )
    return Qt::NoItemFlags;

  Q_ASSERT( mModelForItemFunctions ); // UI must be connected if a valid index was queried

  Item * it = static_cast< Item * >( index.internalPointer() );

  Q_ASSERT( it );

  if ( it->type() == Item::GroupHeader )
    return Qt::ItemIsEnabled;

  Q_ASSERT( it->type() == Item::Message );

  if ( !static_cast< MessageItem * >( it )->isValid() )
    return Qt::NoItemFlags; // not enabled, not selectable

  if ( static_cast< MessageItem * >( it )->aboutToBeRemoved() )
    return Qt::NoItemFlags; // not enabled, not selectable

  if ( static_cast< MessageItem * >( it )->status().isDeleted() )
    return Qt::NoItemFlags; // not enabled, not selectable

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

MessageItem * Model::messageItemByStorageRow( int row ) const
{
  if ( !mStorageModel )
    return 0;
  ModelInvariantIndex * idx = mInvariantRowMapper->modelIndexRowToModelInvariantIndex( row );
  if ( !idx )
    return 0;

  return static_cast< MessageItem * >( idx );
}


MessageItemSetReference Model::createPersistentSet( const QList< MessageItem * > &items )
{
  if ( !mPersistentSetManager )
    mPersistentSetManager = new MessageItemSetManager();

  MessageItemSetReference ref = mPersistentSetManager->createSet();
  for ( QList< MessageItem * >::ConstIterator it = items.constBegin(); it != items.constEnd(); ++it )
    mPersistentSetManager->addMessageItem( ref, *it );

  return ref;
}

QList< MessageItem * > Model::persistentSetCurrentMessageItemList( MessageItemSetReference ref )
{
  if ( !mPersistentSetManager )
    return QList< MessageItem * >();

  return mPersistentSetManager->messageItems( ref );
}

void Model::deletePersistentSet( MessageItemSetReference ref )
{
  if ( !mPersistentSetManager )
    return;

  mPersistentSetManager->removeSet( ref );

  if ( mPersistentSetManager->setCount() < 1 )
  {
    delete mPersistentSetManager;
    mPersistentSetManager = 0;
  }
}

} // namespace Core

} // namespace MessageListView

} // namespace KMail
