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

#ifndef __KMAIL_MESSAGELISTVIEW_CORE_MODEL_H__
#define __KMAIL_MESSAGELISTVIEW_CORE_MODEL_H__

#include <QAbstractItemModel>
#include <QList>
#include <QHash>
#include <QMultiHash>
#include <QDate>

#include "messagelistview/core/aggregation.h"
#include "messagelistview/core/enums.h"
#include "messagelistview/core/sortorder.h"

#include <time.h> // time_t

class QTimer;
class QTime;

namespace KMail
{

namespace MessageListView
{

namespace Core
{

typedef unsigned long int MessageItemSetReference;

class ViewItemJob;
class Filter;
class GroupHeaderItem;
class Item;
class Manager;
class MessageItem;
class Theme;
class StorageModel;
class ModelInvariantRowMapper;
class MessageItemSetManager;
class View;

/**
 * This class manages the huge tree of displayable objects: GroupHeaderItems and MessageItems.
 * The tree is exposed via a 'hacked' QAbstractItemModel interface to a QTreeView
 * subclass (which is MessageListView::View).
 *
 * The keypoint in this class is that it has to be non-blocking in manipulating the tree:
 * fill, cleanup and update operations are performed in timed chunks. Perfect non-blocking
 * behaviour is not possible since there are some small operations that basically can't be
 * splitted in chunks. However, these exceptions apply to a minority of tasks and
 * in the average case the user will not notice.
 *
 * The data for building the tree is obtained from a subclass of StorageModel. The
 * StorageModel must offer a consistent rappresentation of a "flat" folder containing
 * messages.
 */
class Model : public QAbstractItemModel
{
  friend class Item;
  friend class Manager;

  Q_OBJECT
public:

  /**
   * Creates the mighty Model attached to the specified View.
   */
  Model( View *pParent );

  /**
   * Destroys the mighty model along with the tree of items it manages.
   */
  ~Model();

private:

  /**
   * The currently set storage model: shallow pointer.
   */
  StorageModel *mStorageModel;

  /**
   * The currently set aggregation mode: shallow pointer set by Widget
   */
  const Aggregation * mAggregation;

  /**
   * The currently used theme: shallow pointer
   */
  const Theme * mTheme;

  /**
   * The currently used sort order. Pointer not owned by us, but by the Widget.
   */
  const SortOrder * mSortOrder;

  /**
   * The filter to apply on messages. Shallow. Never 0.
   */
  const Filter *mFilter;

  /**
   * The timer involved in breaking the "fill" operation in steps
   */
  QTimer *mFillStepTimer;

  /**
   * Group Key (usually the label) -> GroupHeaderItem, used to quickly find groups, pointers are shallow copies
   */
  QHash< QString, GroupHeaderItem * > * mGroupHeaderItemHash;

  /**
   * Threading cache.
   * MessageIdMD5 -> MessageItem, pointers are shallow copies
   */
  QHash< QString, MessageItem * > * mThreadingCacheMessageIdMD5ToMessageItem;

  /**
   * Threading cache.
   * MessageInReplyToIdMD5 -> MessageItem, pointers are shallow copies
   */
  QMultiHash< QString, MessageItem * > * mThreadingCacheMessageInReplyToIdMD5ToMessageItem;

  /**
   * Threading cache.
   * SubjectMD5 -> MessageItem, pointers are shallow copies
   */
  QHash< QString, QList< MessageItem * > * > * mThreadingCacheMessageSubjectMD5ToMessageItem;

  /**
   * List of group headers that either need to be re-sorted or must be removed because empty
   */
  QHash< GroupHeaderItem *, GroupHeaderItem * > * mGroupHeadersThatNeedUpdate;

  /**
   * List of unassigned messages, used to handle threading in two passes, pointers are owned!
   */
  QList< MessageItem * > * mUnassignedMessageListForPass2;

  /**
   * List of unassigned messages, used to handle threading in two passes, pointers are owned!
   */
  QList< MessageItem * > * mUnassignedMessageListForPass3;

  /**
   * List of unassigned messages, used to handle threading in two passes, pointers are owned!
   */
  QList< MessageItem * > * mUnassignedMessageListForPass4;

  /**
   * Hash of orphan children used in Pass1Cleanup.
   */
  QHash< MessageItem *, MessageItem * > * mOrphanChildrenHash;

  /**
   * Pending fill view jobs, pointers are owned
   */
  QList< ViewItemJob * > * mViewItemJobs;

  /**
   * The today's date. Set when the StorageModel is set and thus grouping is performed.
   * This is used to put the today's messages in the "Today" group, for instance.
   */
  QDate mTodayDate;

  /**
   * Owned invisible root item, useful to implement algorithms that not need
   * to handle the special case of parentless items
   */
  Item *mRootItem;

  /**
   * The view we're attacched to. Shallow pointer (the View owns us).
   */
  View *mView;

  /**
   * The time at the current ViewItemJob step started. Used to compute the time we
   * spent inside this step and eventually jump out on timeout.
   */
  time_t mViewItemJobStepStartTime;

  /**
   * The timeout for a single ViewItemJob step
   */
  int mViewItemJobStepChunkTimeout;

  /**
   * The idle time between two ViewItemJob steps
   */
  int mViewItemJobStepIdleInterval;

  /**
   * The number of messages we process at once in a ViewItemJob step without
   * checking the timeouts above.
   */
  int mViewItemJobStepMessageCheckCount;

  /**
   * Our mighty ModelInvariantRowMapper: used to workaround an
   * issue related to the Model/View architecture.
   *
   * \sa ModelInvariantRowMapper
   */
  ModelInvariantRowMapper * mInvariantRowMapper;

  /**
   * The label for the "Today" group item, cached, so we don't translate it multiple times.
   */
  QString mCachedTodayLabel;

  /**
   * The label for the "Yesterday" group item, cached, so we don't translate it multiple times.
   */
  QString mCachedYesterdayLabel;

  /**
   * The label for the "Unknown" group item, cached, so we don't translate it multiple times.
   */
  QString mCachedUnknownLabel;

  /**
   * The label for the "Last Week" group item, cached, so we don't translate it multiple times.
   */
  QString mCachedLastWeekLabel;

  /**
   * The label for the "Two Weeks Ago" group item, cached, so we don't translate it multiple times.
   */
  QString mCachedTwoWeeksAgoLabel;

  /**
   * The label for the "Three Weeks Ago" group item, cached, so we don't translate it multiple times.
   */
  QString mCachedThreeWeeksAgoLabel;

  /**
   * The label for the "Four Weeks Ago" group item, cached, so we don't translate it multiple times.
   */
  QString mCachedFourWeeksAgoLabel;

  /**
   * Cached bits that we use for fast status checks
   */
  qint32 mCachedWatchedOrIgnoredStatusBits;

  /**
   * Cached bits that we use for fast status checks
   */
  qint32 mCachedNewStatusBits;

  /**
   * Cached bits that we use for fast status checks
   */
  qint32 mCachedNewOrUnreadStatusBits;

  /**
   * Flag signaling a possibly long job batch. This is checked by other
   * classes and used to display some kind of "please wait" feedback to the user.
   */
  bool mInLengthyJobBatch;

  /**
   * We need to save the current item before each job step. This is because
   * our job may cause items to be reparented (thus removed and readded with the current Qt API)
   * and QTreeView will loose the current setting. We also use this to force the current
   * to a specific item after a cleanup job.
   */
  Item * mCurrentItemToRestoreAfterViewItemJobStep; 

  /**
   * Set to true in the first large loading job.
   * Reset to false when the job finishes.
   *
   * Please note that this is NOT set for later jobs: only for the first (possibly huge) one.
   */
  bool mLoading;

  /**
   * Pre-selection is the action of automatically selecting a message just after the folder
   * has finished loading. We may want to select the message that was selected the last
   * time this folder has been open, or we may want to select the first unread message.
   * We also may want to do no pre-selection at all (for example, when the user
   * starts navigating the view before the pre-selection could actually be made
   * and pre-selecting would confuse him). This member holds the option.
   *
   * See also setStorageModel() and abortMessagePreSelection()
   */
  PreSelectionMode mPreSelectionMode;

  /**
   * The Unique Id of the message item to pre-select when mPreSelectionMode
   * is PreSelectLastSelected. This message is "caught" when it "pops up" during
   * the loading process. This unique id is stored by MessageListView::Manager.
   *
   * See also setStorageModel() and abortMessagePreSelection()
   */
  unsigned long mUniqueIdOfLastSelectedMessageInFolder;

  /**
   * The above UniqueId is "translated" to a message pointer when it's fetched
   * from the storage. This message is then selected when it becomes viewable
   * (so at the end of the job). 0 If we have no message to select.
   *
   * See also setStorageModel() and abortMessagePreSelection()
   */
  MessageItem * mLastSelectedMessageInFolder;

  /**
   * The "persistent message item sets" are (guess what?) sets of messages
   * that can be referenced globally via a persistent id. The MessageItemSetManager
   * and this class keep the persistent sets coherent: messages that are deleted
   * are automatically removed from all the sets.
   *
   * Users of this class typically create persistent sets when they start
   * an asynchronous job and they query them back on the way or when the job is terminated.
   *
   * So mPersistentSetManager is in fact the manager for the outstanding "user" jobs.
   * 0 if no jobs are pending (so there are no persistent sets at the moment).
   */
  MessageItemSetManager * mPersistentSetManager;

  /**
   * This pointer is passed to the Item functions that insert children.
   * When we work with disconnected UI this pointer becomes 0.
   */
  Model * mModelForItemFunctions;

public:

  /**
   * Returns the StorageModel currently set.
   */
  StorageModel * storageModel() const
    { return mStorageModel; };

  /**
   * Sets the storage model from that the messages to be displayed should be fetched.
   * The model is then reset and a new fill operation is started. The fill operation may
   * or may not complete before setStorageModel() returns. This depends on the fill
   * strategy and the size of the folder. You can check if the fill operation has
   * completed by looking at the return value of isLoading().
   *
   * Pre-selection is the action of automatically selecting a message just after the folder
   * has finished loading. We may want to select the message that was selected the last
   * time this folder has been open, or we may want to select the first unread message.
   * We also may want to do no pre-selection at all (for example, when the user
   * starts navigating the view before the pre-selection could actually be made
   * and pre-selecting would confuse him). The pre-selection is applied once
   * loading is complete.
   */
  void setStorageModel( StorageModel *storageModel, PreSelectionMode preSelectionMode = PreSelectLastSelected );

  /**
   * Aborts any pending message pre-selection. This may be done if the user
   * starts navigating the view and selecting items before we actually could
   * apply the pre-selection. 
   */
  void abortMessagePreSelection();

  /**
   * This function attempts to apply the specified pre-selection mode in any model state.
   * This isn't always possible so you should specify the pre-selection mode in setStorageModel()
   * if possible. However, this function is useful to implement coherent behaviour when navigating
   * multiple views with the keyboard.
   *
   * If the model is still loading then the pre-selection is scheduled to be applied
   * at the end of the job. PreSelectNone is equivalent to calling abortMessagePreSelection().
   * If the model is no longer loading then the pre-selection is applied immediately with
   * the following restrictions:
   *
   * - PreSelectNone is explicitly ignored (does NOT unselect the current item)
   * - PreSelectLastSelected has no effect (since it would simply re-select the currently selected message)
   */
  void applyMessagePreSelection( PreSelectionMode preSelectionMode );

  /**
   * Returns the hidden root item that all the messages are (or will be) attached to.
   */
  Item * rootItem() const
    { return mRootItem; };

  /**
   * Returns true if the view is currently loading, that is
   * it's in the first (possibly lenghty) job batch after attacching to a StorageModel.
   */
  bool isLoading() const
    { return mLoading; };

  /**
   * Returns the message item that is at the _current_ storage row index
   * or zero if no such storage item is found.
   */
  MessageItem * messageItemByStorageRow( int row ) const;

  /**
   * Sets the Aggregation mode.
   * Does not reload the model in any way: you need to call setStorageModel( storageModel() ) for this to happen.
   * The pointer ownership remains of the caller which must ensure its validity until the next
   * call to setAggretation() or until this Model dies. The caller, in fact, is Widget which
   * takes care of meeting the above conditions. The aggregation pointer must not be null.
   */
  void setAggregation( const Aggregation * aggregation );

  /**
   * Sets the Theme.
   * Does not reload the model in any way: you need to call setStorageModel( storageModel() ) for this to happen.
   * The pointer ownership remains of the caller which must ensure its validity until the next
   * call to setTheme() or until this Model dies. The caller, in fact, is Widget which
   * takes care of meeting the above conditions. The theme pointer must not be null.
   */
  void setTheme( const Theme * theme );

  /**
   * Sets the sort order. As with setTheme() and setAggregation(), this does not reload the
   * model in any way.
   */
  void setSortOrder( const SortOrder * sortOrder );

  /**
   * Sets the Filter to be applied on messages. filter may be null (no filter is applied).
   * The pointer ownership remains of the caller which must ensure its validity until the next
   * call to setFilter() or until this Model dies. The caller, in fact, is Widget which
   * takes care of meeting the above conditions. The Filter pointer may be null.
   */
  void setFilter( const Filter *filter );

  /**
   * Creates a persistent set for the specified MessageItems and
   * returns its reference. Later you can use this reference
   * to retrieve the list of MessageItems that are still valid.
   * See persistentSetActualMessageList() for that.
   *
   * Persistent sets consume resources (both memory and CPU time
   * while manipulating the view) so be sure to call deletePersistentSet()
   * when you no longer need it.
   */
  MessageItemSetReference createPersistentSet( const QList< MessageItem * > &items );

  /**
   * Returns the list of MessageItems that are still existing in the
   * set pointed by the specified reference. This list will contain
   * at most the messages that you have passed to createPersistentSet()
   * but may contain less (even 0) if these MessageItem object were removed
   * from the view for some reason.
   */
  QList< MessageItem * > persistentSetCurrentMessageItemList( MessageItemSetReference ref );

  /**
   * Deletes the persistent set pointed by the specified reference.
   * If the set does not exist anymore, nothing happens.
   */
  void deletePersistentSet( MessageItemSetReference ref );

protected:

  void fillView();

  /**
   * This is called by MessageListView::Manager once in a while.
   * It is a good place to check if the date has changed and
   * trigger a view reload.
   */
  void checkIfDateChanged();

protected slots:
  void viewItemJobStep();

private:
  /**
   * Attempt to find the threading parent for the specified message item.
   * Sets the message threading status to the appropriate value.
   *
   * This function performs In-Reply-To and References threading.
   */
  MessageItem * findMessageParent( MessageItem *mi );
  /**
   * Attempt to find the threading parent for the specified message item.
   * Sets the message threading status to the appropriate value.
   *
   * This function performs Subject based threading.
   */
  MessageItem * guessMessageParent( MessageItem *mi );

  void attachMessageToParent( Item *pParent, MessageItem *mi );
  void messageDetachedUpdateParentProperties( Item *oldParent, MessageItem *mi );
  void attachMessageToGroupHeader( MessageItem *mi );
  void attachGroup( GroupHeaderItem *ghi );

  enum ViewItemJobResult
  {
    ViewItemJobCompleted,
    ViewItemJobInterrupted
  };
  ViewItemJobResult viewItemJobStepInternal();
  ViewItemJobResult viewItemJobStepInternalForJob( ViewItemJob *job, const QTime &tStart );
  ViewItemJobResult viewItemJobStepInternalForJobPass1Fill( ViewItemJob *job, const QTime &tStart );
  ViewItemJobResult viewItemJobStepInternalForJobPass1Cleanup( ViewItemJob *job, const QTime &tStart );
  ViewItemJobResult viewItemJobStepInternalForJobPass1Update( ViewItemJob *job, const QTime &tStart );
  ViewItemJobResult viewItemJobStepInternalForJobPass2( ViewItemJob *job, const QTime &tStart );
  ViewItemJobResult viewItemJobStepInternalForJobPass3( ViewItemJob *job, const QTime &tStart );
  ViewItemJobResult viewItemJobStepInternalForJobPass4( ViewItemJob *job, const QTime &tStart );
  ViewItemJobResult viewItemJobStepInternalForJobPass5( ViewItemJob *job, const QTime &tStart );
  void clearJobList();
  void clearUnassignedMessageLists();
  void clearOrphanChildrenHash();
  void clearThreadingCacheMessageSubjectMD5ToMessageItem();
  void addMessageToSubjectBasedThreadingCache( MessageItem * mi );
  void removeMessageFromSubjectBasedThreadingCache( MessageItem * mi );
  /**
   * Sync the expanded state of the subtree with the specified root.
   * This will cause the items that are marked with Item::ExpandNeeded to be
   * expanded also in the view. For optimization purposes the specified root
   * is assumed to be marked as Item::ExpandNeeded so be sure to check it
   * before calling this function.
   */
  void syncExpandedStateOfSubtree( Item *root );
  /**
   * Save the expanded state of the subtree with the specified root.
   * The state will be saved in the initialExpandStatus() variable.
   * For optimization purposes the specified root is assumed to be expanded
   * and viewable.
   */
  void saveExpandedStateOfSubtree( Item *root );

  enum PropertyChanges
  {
    DateChanged = 1,
    MaxDateChanged = (1 << 1),
    ActionItemStatusChanged = (1 << 2),
    NewUnreadStatusChanged = (1 << 3)
  };

  /**
   * Handle the specified property changes in item. Depending on the item
   * position inside the parent and the types of item and parent the item
   * might need re-grouping or re-sorting. This function takes care of that.
   * It is meant to be called from somewhere inside viewItemJobStepInternal()
   * as it postpones group updates to Pass5.
   *
   * parent and item must not be null. propertyChangeMask should not be zero.
   *
   * Return true if parent might be affected by the item property changes
   * and false otherwise.
   */
  bool handleItemPropertyChanges( int propertyChangeMask, Item * parent, Item * item );

  /**
   * This one checks if the parent of item requires an update due to the
   * properties of item (that might have been changed or the item might
   * have been simply added to the parent). The properties
   * are propagated up to the root item. As optimization we ASSUME that
   * the item->parent() exists (is non 0) and is NOT the root item.
   * Be sure to check it before calling this function (it will assert in debug mode anyway).
   * ... ah... and don't be afraid: this is NOT (directly) recursive :)
   */
  void propagateItemPropertiesToParent( Item * item );

  /**
   * Recursively applies the current filter to the tree originating at the specified item.
   * The item is hidden if the filter doesn't match (the item or any children of it)
   * and this function returns false.
   * If the filter matches somewhere in the subtree then the item isn't hidden
   * and this function returns true.
   *
   * Assumes that the specified item is viewable.
   */
  bool applyFilterToSubtree( Item * item, const QModelIndex &parentIndex );

public:

  // Mandatory QAbstractItemModel interface.

  virtual int columnCount( const QModelIndex &parent = QModelIndex() ) const;
  virtual QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  virtual QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex() ) const;
  QModelIndex index( Item *item, int column ) const;
  virtual QModelIndex parent( const QModelIndex &index ) const;
  virtual int rowCount( const QModelIndex &parent = QModelIndex() ) const;
  virtual Qt::ItemFlags flags( const QModelIndex &index ) const;

protected slots:

  // Slots connected to the underlying StorageModel.

  void slotStorageModelRowsInserted( const QModelIndex &parent, int from, int to );
  void slotStorageModelRowsRemoved( const QModelIndex &parent, int from, int to );
  void slotStorageModelDataChanged( const QModelIndex &fromIndex, const QModelIndex &toIndex );
  void slotStorageModelLayoutChanged();
};

} // namespace Core

} // namespace MessageListView

} // namespace KMail

#endif //!__KMAIL_MESSAGELISTVIEW_CORE_MODEL_H__
