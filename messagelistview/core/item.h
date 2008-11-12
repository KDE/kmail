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

#ifndef __KMAIL_MESSAGELISTVIEW_CORE_ITEM_H__
#define __KMAIL_MESSAGELISTVIEW_CORE_ITEM_H__

#include <QList>
#include <QString>
#include <QDate>

#include <time.h> // for time_t

#include <libkdepim/messagestatus.h>

#include "messagelistview/core/model.h"

// See the MessageListView::Item::insertChildItem() function below for an explaination of this macro.
#if __GNUC__ >= 3
  #define GCC_DONT_INLINE_THIS __attribute__((noinline))
#else
  #define GCC_DONT_INLINE_THIS
#endif


namespace KMail
{

namespace MessageListView
{

namespace Core
{

/**
 * A single item of the MessageListView tree managed by MessageListView::Model.
 *
 * This class stores basic information needed in all the subclasses which
 * at the moment of writing are MessageItem and GroupHeaderItem.
 */
class Item
{
  friend class Model;

public:
  /**
   * The type of the Item.
   */
  enum Type
  {
    GroupHeader,                              ///< This item is a GroupHeaderItem
    Message,                                  ///< This item is a MessageItem
    InvisibleRoot                             ///< This item is just Item and it's the only InvisibleRoot per Model.
  };

  /**
   * Specifies the initial expand status for the item that should be applied
   * when it's attacched to the viewable tree. Needed as a workaround for
   * QTreeView limitations in handling item expansion.
   */
  enum InitialExpandStatus
  {
    ExpandNeeded,                             ///< Must expand when this item becomes viewable
    NoExpandNeeded,                           ///< No expand needed at all
    ExpandExecuted                            ///< Item already expanded
  };

private:
  Type mType;                                 ///< The type of this item
  QList< Item * > *mChildItems;               ///< List of children, may be 0
  Item * mParent;                             ///< The parent view item
  time_t mMaxDate;                            ///< The maximum date in the subtree
  time_t mDate;                               ///< The date of the message (or group date)
  size_t mSize;                               ///< The size of the message in bytes
  QString mSender;                            ///< The sender of the message (or group sender)
  QString mReceiver;                          ///< The receiver of the message (or group receiver)
  QString mSenderOrReceiver;                  ///< Depending on the folder setting: sender or receiver
  int mThisItemIndexGuess;                    ///< The guess for the index in the parent's child list
  QString mSubject;                           ///< The subject of the message (or group subject)
  bool mIsViewable;                           ///< Is this item attacched to the viewable root ?
  InitialExpandStatus mInitialExpandStatus;   ///< The expand status we have to honor when we attach to the viewable root
  QString mFormattedSize;                     ///< The size above formatted as string, this is done only on request
  QString mFormattedDate;                     ///< The formatted date of the message, formatting takes time so it is done only on request
  QString mFormattedMaxDate;                  ///< The maximum date above formatted (lazily)
  KPIM::MessageStatus mStatus;                ///< The status of the message (may be extended to groups in the future)
protected:

  /**
   * Creates an Item. Only derived classes and MessageListView::Model should access this.
   */
  Item( Type type )
    : mType( type ), mChildItems( 0 ), mParent( 0 ), mThisItemIndexGuess( 0 ),
      mIsViewable( false ), mInitialExpandStatus( NoExpandNeeded )
    {};

public:

  /**
   * Destroys the Item. Should be protected just like the constructor but the QList<>
   * helpers need to access it, so it's public actually.
   */
  virtual ~Item();

  /**
   * Returns the type of this item. The Type can be set only in the constructor.
   */
  Type type() const
    { return mType; };

  /**
   * The initial expand status we have to honor when attacching to the viewable root.
   */
  InitialExpandStatus initialExpandStatus() const
    { return mInitialExpandStatus; };

  /**
   * Set the initial expand status we have to honor when attacching to the viewable root.
   */
  void setInitialExpandStatus( InitialExpandStatus initialExpandStatus )
    { mInitialExpandStatus = initialExpandStatus; };

  /**
   * Is this item attached to the viewable root ?
   */
  bool isViewable() const
    { return mIsViewable; };

  /**
   * Return true if Item pointed by it is an ancestor of this item (that is,
   * if it is it's parent, parent of it's parent, parent of it's parent of it's parent etc...
   */
  bool hasAncestor( const Item * it ) const
    { return mParent ? ( mParent == it ? true : mParent->hasAncestor( it ) ) : false; };

  /**
   * Makes this item viewable, that is, notifies it's existence to any listener
   * attacched to the "rowsInserted()" signal, most notably QTreeView.
   *
   * This will also make all the children viewable.
   */
  void setViewable( Model *model, bool bViewable );

  /**
   * Return the list of child items. May be null.
   */
  QList< Item * > * childItems() const
    { return mChildItems; };

  /**
   * Returns the child item at position idx or 0 if idx is out of the allowable range.
   */
  Item * childItem( int idx ) const
    {
      if ( idx < 0 )
        return 0;
      if ( !mChildItems )
        return 0;
      if ( mChildItems->count() <= idx )
        return 0;
      return mChildItems->at( idx );
    };

  /**
   * Returns the first child item, if any.
   */
  Item * firstChildItem() const
    { return mChildItems ? ( mChildItems->count() > 0 ? mChildItems->at( 0 ) : 0 ) : 0; };

  /**
   * Returns the item that is visually below the specified child if this item.
   * Note that the returned item may belong to a completely different subtree.
   */
  Item * itemBelowChild( Item * child );

  /**
   * Returns the item that is visually above the specified child if this item.
   * Note that the returned item may belong to a completely different subtree.
   */
  Item * itemAboveChild( Item * child );

  /**
   * Returns the deepest item in the subtree originating at this item.
   */
  Item * deepestItem();

  /**
   * Returns the item that is visually below this item in the tree.
   * Note that the returned item may belong to a completely different subtree.
   */
  Item * itemBelow();

  /**
   * Returns the item that is visually above this item in the tree.
   * Note that the returned item may belong to a completely different subtree.
   */
  Item * itemAbove();

  /**
   * Debug helper. Dumps the structure of this subtree.
   */
  void dump( const QString &prefix );

  /**
   * Returns the number of children of this Item.
   */
  int childItemCount() const
    { return mChildItems ? mChildItems->count() : 0; };

  /**
   * Convenience function that returns true if this item has children.
   */
  bool hasChildren() const
    { return childItemCount() > 0; };

  /**
   * A structure used with MessageListView::Item::childItemStats().
   * Contains counts of total, new and unread messages in a subtree.
   */
  class ChildItemStats
  {
  public:
    unsigned int mTotalChildCount;   // total
    unsigned int mNewChildCount;     // new+unread
    unsigned int mUnreadChildCount;  // unread only
  public:
    ChildItemStats()
      : mTotalChildCount( 0 ), mNewChildCount( 0 ), mUnreadChildCount( 0 )
      {};
  };

  /**
   * Gathers statistics about child items.
   * For performance purposes assumes that this item has children.
   * You MUST check it before calling it.
   */
  void childItemStats( ChildItemStats &stats ) const;

  /**
   * Returns the actual index of the child Item item or -1 if
   * item is not a child of this Item.
   */
  int indexOfChildItem( Item *item ) const
    { return mChildItems ? mChildItems->indexOf( item ) : -1; };

  /**
   * Returns the cached guess for the index of this item in the parent's child list.
   *
   * This is used to speed up the index lookup with the following algorithm:
   * Ask the parent if this item is at the position specified by index guess (this costs ~O(1)).
   * If the position matches we have finished, if it doesn't then perform
   * a linear search via indexOfChildItem() (which costs ~O(n)).
   */
  int indexGuess() const
    { return mThisItemIndexGuess; };

  /**
   * Updates the cached guess for the index of this item in the parent's child list.
   * See indexGuess() for more information.
   */
  void setIndexGuess( int index )
    { mThisItemIndexGuess = index; };

  /**
   * Returns true if the specified Item is a position idx in the list of children.
   * See indexGuess() for more information.
   */
  bool childItemHasIndex( const Item *item, int idx ) const
    { return mChildItems ? ( ( mChildItems->count() > idx ) ? ( mChildItems->at( idx ) == item ) : false ) : false; };

  /**
   * Returns the parent Item in the tree, or 0 if this item isn't attached to the tree.
   * Please note that even if this item has a non-zero parent, it can be still non viewable.
   * That is: the topmost parent of this item may be not attached to the viewable root.
   */
  Item * parent() const
    { return mParent; };

  /**
   * Sets the parent for this item. You should also take care of inserting
   * this item in the parent's child list.
   */
  void setParent( Item *pParent )
    { mParent = pParent; };

  /**
   * Returns the status associated to this Item.
   */
  const KPIM::MessageStatus & status() const
    { return mStatus; };

  /**
   * Sets the status associated to this Item.
   */
  void setStatus( const KPIM::MessageStatus &status )
    { mStatus = status; };

  /**
   * Returns a string describing the status e.g: "Read, Forwarded, Important"
   */
  QString statusDescription() const;

  /**
   * Returns the size of this item (size of the Message, mainly)
   */
  size_t size() const
    { return mSize; };

  /**
   * Sets the size of this item (size of the Message, mainly)
   */
  void setSize( size_t size )
    { mSize = size; mFormattedSize.clear(); };

  /**
   * A string with a text rappresentation of size(). This is computed on-the-fly
   * and cached until the size() changes.
   */
  const QString & formattedSize();

  /**
   * Returns the date of this item
   */
  const time_t date() const
    { return mDate; };

  /**
   * Sets the date of this item
   */
  void setDate( time_t date )
    { mDate = date; mFormattedDate.clear(); };

  /**
   * A string with a text rappresentation of date() obtained via Manager. This is computed on-the-fly
   * and cached until the size() changes.
   */
  const QString & formattedDate();

  /**
   * Returns the maximum date in the subtree originating from this item.
   * This is kept up-to-date by MessageListView::Model.
   */
  const time_t maxDate() const
    { return mMaxDate; };

  /**
   * Sets the maximum date in the subtree originating from this item.
   */
  void setMaxDate( time_t date )
    { mMaxDate = date; mFormattedMaxDate.clear(); };

  /**
   * A string with a text rappresentation of maxDate() obtained via Manager. This is computed on-the-fly
   * and cached until the size() changes.
   */
  const QString & formattedMaxDate();

  /**
   * Recompute the maximum date from the current children list.
   * Return true if the current max date changed and false otherwise.
   */
  bool recomputeMaxDate();

  /**
   * Returns the sender associated to this item.
   */
  const QString & sender() const
    { return mSender; };

  /**
   * Sets the sender associated to this item.
   */
  void setSender( const QString &sender )
    { mSender = sender; };

  /**
   * Returns the receiver associated to this item.
   */
  const QString & receiver() const
    { return mReceiver; };

  /**
   * Sets the sender associated to this item.
   */
  void setReceiver( const QString &receiver )
    { mReceiver = receiver; };

  /**
   * Returns the sender or the receiver, depending on the underlying StorageModel settings.
   */
  const QString & senderOrReceiver() const
    { return mSenderOrReceiver; };

  /**
   * Sets the sender or the receiver: this should depend on the underlying StorageModel settings.
   */
  void setSenderOrReceiver( const QString &senderOrReceiver )
    { mSenderOrReceiver = senderOrReceiver; };

  /**
   * Returns the subject associated to this Item.
   */
  const QString & subject() const
    { return mSubject; };

  /**
   * Sets the subject associated to this Item.
   */
  void setSubject( const QString &subject )
    { mSubject = subject; };

  /**
   * This is meant to be called right after the constructor.
   * It sets up several items at once (so even if not inlined it's still a single call)
   * and it skips some calls that can be avoided at constructor time.
   */
  void initialSetup(
      time_t date, size_t size, const QString &sender,
      const QString &receiver, const QString &senderOrReceiver
    )
  {
    mDate = mMaxDate = date;
    mSize = size;
    mSender = sender;
    mReceiver = receiver;
    mSenderOrReceiver = senderOrReceiver;
  };

  /**
   * This is meant to be called right after the constructor for MessageItem objects.
   * It sets up several items at once (so even if not inlined it's still a single call).
   */
  void setSubjectAndStatus(
      const QString &subject, const KPIM::MessageStatus &status
    )
  {
    mSubject = subject;
    mStatus = status;
  };

  /**
   * Appends an Item to this item's child list.
   * The Model is used for beginInsertRows()/endInsertRows() calls.
   */
  int appendChildItem( Model *model, Item *child );

  /**
   * Appends a child item without inserting it via the model.
   * This is useful in ThemeEditor which doesn't use a custom model for the items.
   * You shouldn't need to use this function...
   */
  void rawAppendChildItem( Item * child );

  /**
   * Removes a child from this item's child list without deleting it.
   * The Model is used for beginRemoveRows()/endRemoveRows() calls.
   */
  void takeChildItem( Model *model, Item *child );

  /**
   * Implements "in the middle" insertions of child items.
   * The template argument class must export a static inline bool firstGreaterOrEqual( Item *, Item * )
   * function which must return true when the first parameter item is considered to be greater
   * or equal to the second parameter item and false otherwise.
   *
   * The insertion function *IS* our very bottleneck on flat views
   * (when there are items with a lot of children). This is somewhat pathological...
   * beside the binary search based insertion sort we actually can only do "statement level" optimization.
   * I've found no better algorithms so far. If someone has a clever idea, please write to pragma
   * at kvirc dot net :)
   *
   * GCC_DONT_INLINE_THIS is a macro defined above to __attribute__((noinline))
   * if the current compiler is gcc. Without this attribute gcc attempts to inline THIS
   * function inside the callers. The problem is that while inlining this function
   * it doesn't inline the INNER comparison functions (which we _WANT_ to be inlined) 
   * because they would make the caller function too big.
   *
   * This is what gcc reports with -Winline:
   *
   * /home/pragma/kmail-soc/kmail/messagelistview/item.h:352: warning: inlining failed in call to
   *   'static bool KMail::MessageListView::ItemSubjectComparator::firstGreaterOrEqual(KMail::MessageListView::Item*, KMail::MessageListView::Item*)':
   *    --param large-function-growth limit reached while inlining the caller
   * /home/pragma/kmail-soc/kmail/messagelistview/model.cpp:239: warning: called from here
   *
   * The comparison functions then appear in the symbol table:
   *
   * etherea kmail # nm /usr/kde/4.0/lib/libkmailprivate.so | grep Comparator
   * 00000000005d2c10 W _ZN5KMail15MessageListView18ItemDateComparator19firstGreaterOrEqualEPNS0_4ItemES3_
   * 00000000005d2cb0 W _ZN5KMail15MessageListView20ItemSenderComparator19firstGreaterOrEqualEPNS0_4ItemES3_
   * 00000000005d2c50 W _ZN5KMail15MessageListView21ItemSubjectComparator19firstGreaterOrEqualEPNS0_4ItemES3_
   * ...
   *
   * With this attribute, instead, gcc doesn't complain at all and the inner comparisons
   * *seem* to be inlined correctly (there is no sign of them in the symbol table).
   */
  template< class ItemComparator, bool bAscending > int GCC_DONT_INLINE_THIS insertChildItem( Model *model, Item *child )
  {
    if ( !mChildItems )
      return appendChildItem( model, child );

    int cnt = mChildItems->count();
    if ( cnt < 1 )
      return appendChildItem( model, child );

    int idx;
    Item * pivot;

    if ( bAscending )
    {
      pivot = mChildItems->at( cnt - 1 );

      if ( ItemComparator::firstGreaterOrEqual( child, pivot ) ) // gcc: <-- inline this instead, thnx
        return appendChildItem( model, child ); // this is very likely in date based comparisons (FIXME: not in other ones)

      // Binary search based insertion
      int l = 0;
      int h = cnt - 1;

      for(;;)
      {
        idx = (l + h) / 2;
        pivot = mChildItems->at( idx );
        if ( ItemComparator::firstGreaterOrEqual( pivot, child ) ) // gcc: <-- inline this instead, thnx
        {
          if ( l < h )
            h = idx - 1;
          else
            break;
        } else {
          if ( l < h )
            l = idx + 1;
          else {
            idx++;
            break;
          }
        }
      }
    } else {

      pivot = mChildItems->at( 0 );
      if ( ItemComparator::firstGreaterOrEqual( child, pivot ) ) // gcc: <-- inline this instead, thnx
        idx = 0;  // this is very likely in date based comparisons (FIXME: not in other ones)
      else {

        // Binary search based insertion
        int l = 0;
        int h = cnt - 1;

        for(;;)
        {
          idx = (l + h) / 2;
          pivot = mChildItems->at( idx );
          if ( ItemComparator::firstGreaterOrEqual( child, pivot ) ) // gcc: <-- inline this instead, thnx
          {
            if ( l < h )
              h = idx - 1;
            else
              break;
          } else {
            if ( l < h )
              l = idx + 1;
            else {
              idx++;
              break;
            }
          }
        }
      }
    }

    Q_ASSERT( idx >= 0 );
    Q_ASSERT( idx <= mChildItems->count() );

    if ( mIsViewable && model )
      model->beginInsertRows( model->index( this, 0 ), idx, idx ); // BLEAH :D

    mChildItems->insert( idx, child );
    child->setIndexGuess( idx );
    if ( mIsViewable )
    {
      if ( model )
        model->endInsertRows(); // BLEAH :D
      child->setViewable( model, true );
    }

    return idx;
  }

  /**
   * Checks if the specified child item is actually in the wrong
   * position in the child list and returns true in that case.
   * Returns false if the item is already in the right position
   * and no re-sorting is needed.
   */
  template< class ItemComparator, bool bAscending > bool childItemNeedsReSorting( Item * child )
  {
    if ( !mChildItems )
      return false; // not my child! (ugh... should assert ?)

    int idx = child->indexGuess();
    if ( !childItemHasIndex( child, idx ) )
      idx = indexOfChildItem( child );

    if ( idx > 0 )
    {
      Item * prev = mChildItems->at( idx - 1 );
      if ( bAscending )
      {
        // child must be greater or equal to the previous item
        if ( !ItemComparator::firstGreaterOrEqual( child, prev ) )
          return true; // wrong order: needs re-sorting
      } else {
        // previous must be greater or equal to the child item
        if ( !ItemComparator::firstGreaterOrEqual( prev, child ) )
          return true; // wrong order: needs re-sorting
      }
    }

    if ( idx < ( mChildItems->count() - 1 ) )
    {
      Item * next = mChildItems->at( idx + 1 );
      if ( bAscending )
      {
        // next must be greater or equal to child
        if ( !ItemComparator::firstGreaterOrEqual( next, child ) )
          return true; // wrong order: needs re-sorting
      } else {
        // child must be greater or equal to next
        if ( !ItemComparator::firstGreaterOrEqual( child, next ) )
          return true; // wrong order: needs re-sorting
      }
    }

    return false;
  }

  /**
   * Kills all the child items without emitting any signal, recursively.
   * It should be used only when MessageListView::Model is reset() afterwards.
   */
  void killAllChildItems();
private:

  /**
   * Internal handler for managing the children list.
   */
  void childItemDead( Item * child );
};

/**
 * A helper class used with MessageListView::Item::childItemNeedsReSorting() and
 * MessageListView::Item::insertChildItem().
 */
class ItemSizeComparator
{
public:
  static inline bool firstGreaterOrEqual( Item * first, Item * second )
  {
    if ( first->size() < second->size() )
      return false;
    // When the sizes are equal compare by date too
    if ( first->size() == second->size() )
      return first->date() >= second->date();
    return true;
  }
};

/**
 * A helper class used with MessageListView::Item::childItemNeedsReSorting() and
 * MessageListView::Item::insertChildItem().
 */
class ItemDateComparator
{
public:
  static inline bool firstGreaterOrEqual( Item * first, Item * second )
  {
    if ( first->date() < second->date() )
      return false;
    // When the dates are equal compare by subject too
    // This is useful, for example, in kernel mailing list where people
    // send out multiple messages with patch parts at exactly the same time.
    if ( first->date() == second->date() )
      return first->subject() >= second->subject();
    return true;
  }
};

/**
 * A helper class used with MessageListView::Item::childItemNeedsReSorting() and
 * MessageListView::Item::insertChildItem().
 */
class ItemMaxDateComparator
{
public:
  static inline bool firstGreaterOrEqual( Item * first, Item * second )
  {
    if ( first->maxDate() < second->maxDate() )
      return false;
    if ( first->maxDate() == second->maxDate() )
      return first->subject() >= second->subject();
    return true;
  }
};

/**
 * A helper class used with MessageListView::Item::childItemNeedsReSorting() and
 * MessageListView::Item::insertChildItem().
 */
class ItemSubjectComparator
{
public:
  static inline bool firstGreaterOrEqual( Item * first, Item * second )
  {
    int ret = first->subject().compare( second->subject(), Qt::CaseInsensitive );
    if ( ret < 0 )
      return false;
    // compare by date when subjects are equal
    if ( ret == 0 )
      return first->date() >= second->date();
    return true;
  }
};

/**
 * A helper class used with MessageListView::Item::childItemNeedsReSorting() and
 * MessageListView::Item::insertChildItem().
 */
class ItemSenderComparator
{
public:
  static inline bool firstGreaterOrEqual( Item * first, Item * second )
  {
    int ret = first->sender().compare( second->sender(), Qt::CaseInsensitive );
    if ( ret < 0 )
      return false;
    // compare by date when senders are equal
    if ( ret == 0 )
      return first->date() >= second->date();
    return true;
  }
};

/**
 * A helper class used with MessageListView::Item::childItemNeedsReSorting() and
 * MessageListView::Item::insertChildItem().
 */
class ItemReceiverComparator
{
public:
  static inline bool firstGreaterOrEqual( Item * first, Item * second )
  {
    int ret = first->receiver().compare( second->receiver(), Qt::CaseInsensitive );
    if ( ret < 0 )
      return false;
    // compare by date when receivers are equal
    if ( ret == 0 )
      return first->date() >= second->date();
    return true;
  }
};

/**
 * A helper class used with MessageListView::Item::childItemNeedsReSorting() and
 * MessageListView::Item::insertChildItem().
 */
class ItemSenderOrReceiverComparator
{
public:
  static inline bool firstGreaterOrEqual( Item * first, Item * second )
  {
    int ret = first->senderOrReceiver().compare( second->senderOrReceiver(), Qt::CaseInsensitive );
    if ( ret < 0 )
      return false;
    // compare by date when sender/receiver are equal
    if ( ret == 0 )
      return first->date() >= second->date();
    return true;
  }
};

/**
 * A helper class used with MessageListView::Item::childItemNeedsReSorting() and
 * MessageListView::Item::insertChildItem().
 */
class ItemToDoStatusComparator
{
public:
  static inline bool firstGreaterOrEqual( Item * first, Item * second )
  {
    if ( first->status().isToAct() )
    {
      if ( second->status().isToAct() )
        return first->date() >= second->date();
      return true;
    }
    if ( second->status().isToAct() )
      return false;
    return first->date() >= second->date();
  }
};

} // namespace Core

} // namespace MessageListView

} // namespace KMail

#endif //!__KMAIL_MESSAGELISTVIEW_CORE_ITEM_H__
