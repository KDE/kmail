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

#ifndef __KMAIL_MESSAGELISTVIEW_CORE_AGGREGATION_H__
#define __KMAIL_MESSAGELISTVIEW_CORE_AGGREGATION_H__

class QDataStream;

#include <QList>
#include <QPair>
#include <QString>

#include "messagelistview/core/optionset.h"

namespace KMail
{

namespace MessageListView
{

namespace Core
{

/**
 * A set of aggregation options that can be applied to the MessageListView::Model in a single shot.
 * The set defines the behaviours related to the population of the model, threading
 * of messages, grouping and sorting.
 */
class Aggregation : public OptionSet
{
public:

  /**
   * Message grouping.
   * If you add values here please look at the implementations of the enumerate* functions
   * and add appropriate descriptors.
   */
  enum Grouping
  {
    NoGrouping,                          ///< Don't group messages at all
    GroupByDate,                         ///< Group the messages by the date of the thread leader
    GroupByDateRange,                    ///< Use smart (thread leader) date ranges ("Today","Yesterday","Last Week"...)
    GroupBySenderOrReceiver,             ///< Group by sender (incoming) or receiver (outgoing) field
    GroupBySender,                       ///< Group by sender, always
    GroupByReceiver                      ///< Group by receiver, always
    // Never add enum entries in the middle: always add them at the end (numeric values are stored in configuration)
    // TODO: Group by message status: "Important messages", "Urgent messages", "To reply", "To do" etc...
    // TODO: Group by message unread status: "Unread messages", "Read messages" (maybe "New" ?)
  };

  /**
   * How to sort the groups
   * If you add values here please look at the implementations of the enumerate* functions
   * and add appropriate descriptors.
   */
  enum GroupSorting
  {
    NoGroupSorting,                      ///< Don't sort the groups at all, add them as they come in
    SortGroupsByDateTime,                ///< Sort groups by date/time of the group
    SortGroupsByDateTimeOfMostRecent,    ///< Sort groups by date/time of the most recent message
    SortGroupsBySenderOrReceiver,        ///< Sort groups by sender or receiver (makes sense only with GroupBySenderOrReceiver)
    SortGroupsBySender,                  ///< Sort groups by sender (makes sense only with GroupBySender)
    SortGroupsByReceiver                 ///< Sort groups by receiver (makes sense only with GroupByReceiver)
    // Never add enum entries in the middle: always add them at the end (numeric values are stored in configuration)
  };

  /**
   * The "generic" sort direction: used for groups and for messages
   * If you add values here please look at the implementations of the enumerate* functions
   * and add appropriate descriptors.
   */
  enum SortDirection
  {
    Ascending,
    Descending
  };

  /**
   * The available group expand policies.
   * If you add values here please look at the implementations of the enumerate* functions
   * and add appropriate descriptors.
   */
  enum GroupExpandPolicy
  {
    NeverExpandGroups,                   ///< Never expand groups during a view fill algorithm
    ExpandRecentGroups,                  ///< Makes sense only with GroupByDate or GroupByDateRange
    AlwaysExpandGroups                   ///< All groups are expanded as they are inserted
    // Never add enum entries in the middle: always add them at the end (numeric values are stored in configuration)
  };

  /**
   * The available threading methods.
   * If you add values here please look at the implementations of the enumerate* functions
   * and add appropriate descriptors.
   */
  enum Threading
  {
    NoThreading,                         ///< Perform no threading at all
    PerfectOnly,                         ///< Thread by "In-Reply-To" field only
    PerfectAndReferences,                ///< Thread by "In-Reply-To" and "References" fields
    PerfectReferencesAndSubject          ///< Thread by all of the above and try to match subjects too
    // Never add enum entries in the middle: always add them at the end (numeric values are stored in configuration)
  };

  /**
   * The available thread leading options. Meaningless when threading is set to NoThreading.
   * If you add values here please look at the implementations of the enumerate* functions
   * and add appropriate descriptors.
   */
  enum ThreadLeader
  {
    TopmostMessage,                      ///< The thread grouping is computed from the topmost message (very similar to least recent, but might be different if timezones or machine clocks are screwed)
    MostRecentMessage                    ///< The thread grouping is computed from the most recent message
    // Never add enum entries in the middle: always add them at the end (numeric values are stored in configuration)
  };

  /**
   * The available thread expand policies. Meaningless when threading is set to NoThreading.
   * If you add values here please look at the implementations of the enumerate* functions
   * and add appropriate descriptors.
   */
  enum ThreadExpandPolicy
  {
    NeverExpandThreads,                                ///< Never expand any thread, this is fast
    ExpandThreadsWithNewMessages,                      ///< Expand threads with new messages only
    ExpandThreadsWithUnreadMessages,                   ///< Expand threads with unread messages (this includes new)
    AlwaysExpandThreads,                               ///< Expand all threads (this might be very slow)
    ExpandThreadsWithUnreadOrImportantMessages         ///< Expand threads with "hot" messages (this includes new, unread, important, todo)
    // Never add enum entries in the middle: always add them at the end (numeric values are stored in configuration)
  };

  /**
   * The available message sorting options.
   * If you add values here please look at the implementations of the enumerate* functions
   * and add appropriate descriptors.
   */
  enum MessageSorting
  {
    NoMessageSorting,                    ///< Don't sort the messages at all
    SortMessagesByDateTime,              ///< Sort the messages by date and time
    SortMessagesByDateTimeOfMostRecent,  ///< Sort the messages by date and time of the most recent message in subtree
    SortMessagesBySenderOrReceiver,      ///< Sort the messages by sender or receiver
    SortMessagesBySender,                ///< Sort the messages by sender
    SortMessagesByReceiver,              ///< Sort the messages by receiver
    SortMessagesBySubject,               ///< Sort the messages by subject
    SortMessagesBySize,                  ///< Sort the messages by size
    SortMessagesByToDoStatus             ///< Sort the messages by the TODO flag of status
    // Warning: Never add enum entries in the middle: always add them at the end (numeric values are stored in configuration)
  };

  /**
   * The available fill view strategies.
   * If you add values here please look at the implementations of the enumerate* functions
   * and add appropriate descriptors.
   */
  enum FillViewStrategy
  {
    FavorInteractivity,                  ///< Do small chunks of work, small intervals between chunks to allow for UI event processing
    FavorSpeed,                          ///< Do larger chunks of work, zero intervals between chunks
    BatchNoInteractivity                 ///< Do one large chunk, no interactivity at all
    // Warning: Never add enum entries in the middle: always add them at the end (numeric values are stored in configuration)
  };

private:
  Grouping mGrouping;
  GroupSorting mGroupSorting;
  SortDirection mGroupSortDirection;
  GroupExpandPolicy mGroupExpandPolicy;
  Threading mThreading;
  ThreadLeader mThreadLeader;
  ThreadExpandPolicy mThreadExpandPolicy;
  MessageSorting mMessageSorting;
  SortDirection mMessageSortDirection;
  FillViewStrategy mFillViewStrategy;

public:
  Aggregation();
  Aggregation( const Aggregation &opt );
  Aggregation(
      const QString &name,
      const QString &description,
      Grouping grouping,
      GroupSorting groupSorting,
      SortDirection groupSortDirection,
      GroupExpandPolicy groupExpandPolicy,
      Threading threading,
      ThreadLeader threadLeader,
      ThreadExpandPolicy threadExpandPolicy,
      MessageSorting messageSorting,
      SortDirection messageSortDirection,
      FillViewStrategy fillViewStrategy
    );

public:
  /**
   * Returns the currently set Grouping option.
   */
  Grouping grouping() const
    { return mGrouping; };

  /**
   * Sets the Grouping option.
   */
  void setGrouping( Grouping g )
    { mGrouping = g; };

  /**
   * Enumerates the available grouping options as a QList of
   * pairs in that the first item is the localized description of the
   * option value and the second item is the integer option value itself.
   */
  static QList< QPair< QString, int > > enumerateGroupingOptions();

  /**
   * Returns the GroupSorting currently set
   */
  GroupSorting groupSorting() const
    { return mGroupSorting; };

  /**
   * Sets the GroupSorting option.
   * Please note that when Grouping is set to NoGrouping then
   * this option has no meaning (and by policy should be set to NoGroupSorting).
   */
  void setGroupSorting( GroupSorting gs )
    { mGroupSorting = gs; };

  /**
   * Enumerates the group sorting options compatible with the specified Grouping.
   * The returned descriptors are pairs in that the first item is the localized description
   * of the option value and the second item is the integer option value itself.
   * If the returned list is empty then the value of the option is meaningless in the current context.
   */
  static QList< QPair< QString, int > > enumerateGroupSortingOptions( Grouping g );

  /**
   * Returns the current group SortDirection.
   */
  SortDirection groupSortDirection() const
    { return mGroupSortDirection; };

  /**
   * Sets the SortDirection for the groups.
   * Note that this option has no meaning if group sorting is set to NoGroupSorting.
   */
  void setGroupSortDirection( SortDirection groupSortDirection )
    { mGroupSortDirection = groupSortDirection; };

  /**
   * Enumerates the group sort direction options compatible with the specified Grouping and GroupSorting.
   * The returned descriptors are pairs in that the first item is the localized description
   * of the option value and the second item is the integer option value itself.
   * If the returned list is empty then the value of the option is meaningless in the current context.
   */
  static QList< QPair< QString, int > > enumerateGroupSortDirectionOptions( Grouping g, GroupSorting gs );

  /**
   * Returns the current GroupExpandPolicy.
   */
  GroupExpandPolicy groupExpandPolicy() const
    { return mGroupExpandPolicy; };

  /**
   * Sets the SortDirection for the groups.
   * Note that this option has no meaning if grouping is set to NoGrouping.
   */
  void setGroupExpandPolicy( GroupExpandPolicy groupExpandPolicy )
    { mGroupExpandPolicy = groupExpandPolicy; };

  /**
   * Enumerates the group sort direction options compatible with the specified Grouping.
   * The returned descriptors are pairs in that the first item is the localized description
   * of the option value and the second item is the integer option value itself.
   * If the returned list is empty then the value of the option is meaningless in the current context.
   */
  static QList< QPair< QString, int > > enumerateGroupExpandPolicyOptions( Grouping g );

  /**
   * Returns the current threading method.
   */
  Threading threading() const
    { return mThreading; };

  /**
   * Sets the threading method option.
   */
  void setThreading( Threading t )
    { mThreading = t; };

  /**
   * Enumerates the available threading method options.
   * The returned descriptors are pairs in that the first item is the localized description
   * of the option value and the second item is the integer option value itself.
   */
  static QList< QPair< QString, int > > enumerateThreadingOptions();

  /**
   * Returns the current thread leader determination method.
   */
  ThreadLeader threadLeader() const
    { return mThreadLeader; };

  /**
   * Sets the current thread leader determination method.
   * Please note that when Threading is set to NoThreading this value is meaningless
   * and by policy should be set to TopmostMessage.
   */
  void setThreadLeader( ThreadLeader tl )
    { mThreadLeader = tl; };

  /**
   * Enumerates the thread leader determination methods compatible with the specified Threading
   * and the specified Gouping options.
   * The returned descriptors are pairs in that the first item is the localized description
   * of the option value and the second item is the integer option value itself.
   * If the returned list is empty then the value of the option is meaningless in the current context.
   */
  static QList< QPair< QString, int > > enumerateThreadLeaderOptions( Grouping g, Threading t );

  /**
   * Returns the current thread expand policy.
   */  
  ThreadExpandPolicy threadExpandPolicy() const
    { return mThreadExpandPolicy; };

  /**
   * Sets the current thread expand policy.
   * Please note that when Threading is set to NoThreading this value is meaningless
   * and by policy should be set to NeverExpandThreads.
   */
  void setThreadExpandPolicy( ThreadExpandPolicy threadExpandPolicy )
    { mThreadExpandPolicy = threadExpandPolicy; };

  /**
   * Enumerates the thread expand policies compatible with the specified Threading option.
   * The returned descriptors are pairs in that the first item is the localized description
   * of the option value and the second item is the integer option value itself.
   * If the returned list is empty then the value of the option is meaningless in the current context.
   */
  static QList< QPair< QString, int > > enumerateThreadExpandPolicyOptions( Threading t );

  /**
   * Returns the current message sorting option
   */  
  MessageSorting messageSorting() const
    { return mMessageSorting; };

  /**
   * Sets the current message sorting option
   */  
  void setMessageSorting( MessageSorting ms )
    { mMessageSorting = ms; };

  /**
   * Enumerates the message sorting options compatible with the specified Threading setting.
   * The returned descriptors are pairs in that the first item is the localized description
   * of the option value and the second item is the integer option value itself.
   */
  static QList< QPair< QString, int > > enumerateMessageSortingOptions( Threading t );

  /**
   * Returns true if the ms parameter specifies a valid MessageSorting option.
   */
  static bool isValidMessageSorting( MessageSorting ms );

  /**
   * Returns the current message SortDirection.
   */
  SortDirection messageSortDirection() const
    { return mMessageSortDirection; };

  /**
   * Sets the SortDirection for the message.
   * Note that this option has no meaning if message sorting is set to NoMessageSorting.
   */
  void setMessageSortDirection( SortDirection messageSortDirection )
    { mMessageSortDirection = messageSortDirection; };

  /**
   * Enumerates the available message sorting directions for the specified MessageSorting option.
   * The returned descriptors are pairs in that the first item is the localized description
   * of the option value and the second item is the integer option value itself.
   * If the returned list is empty then the value of the option is meaningless in the current context.
   */
  static QList< QPair< QString, int > > enumerateMessageSortDirectionOptions( MessageSorting ms );

  /**
   * Returns the current fill view strategy.
   */
  FillViewStrategy fillViewStrategy() const
    { return mFillViewStrategy; };

  /**
   * Sets the current fill view strategy.
   */
  void setFillViewStrategy( FillViewStrategy fillViewStrategy )
    { mFillViewStrategy = fillViewStrategy; };

  /**
   * Enumerates the fill view strategies.
   * The returned descriptors are pairs in that the first item is the localized description
   * of the option value and the second item is the integer option value itself.
   */
  static QList< QPair< QString, int > > enumerateFillViewStrategyOptions();

  /**
   * Pure virtual reimplemented from OptionSet.
   */
  virtual void save( QDataStream &stream ) const;

  /**
   * Pure virtual reimplemented from OptionSet.
   */
  virtual bool load( QDataStream &stream );
};

} // namespace Core

} // namespace MessageListView

} // namespace KMail

#endif //!__KMAIL_MESSAGELISTVIEW_CORE_AGGREGATION_H__
