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

#ifndef __KMAIL_MESSAGELISTVIEW_MESSAGESET_H__
#define __KMAIL_MESSAGELISTVIEW_MESSAGESET_H__

#include <QList>
#include <QObject>

class KMMsgBase;
class KMFolder;

namespace MessageList
{

namespace Core
{
  typedef unsigned long int MessageItemSetReference;

  class MessageItem;
} // Core

} // namespace MessageList


namespace KMail
{

namespace MessageListView
{

class Pane;
class Widget;

/**
 * A persistent set of messages in one of the MessageListView::Pane child Widgets.
 * The Pane can create such a set for you by the means of createMessageSetFromSelection()
 * and similar methods. The goal of this class is to define a perfectly clear
 * behaviour in the following situations.
 *
 * 1 When the Pane object dies
 *
 *   The MessageSet is invalidated and its functions turn to no-ops.
 *   There are no crashes: the Pane isn't just referenced anymore.
 *
 * 2 When the Widget that this set belongs to dies
 *
 *   The MessageSet is invalidated and its functions turn to no-ops.
 *   There are no crashes.
 *
 * 3 When the Widget (and the View inside) changes current folder
 *   or just stops displaying the set messages
 *
 *   The messages are silently removed from the set.
 *
 * 4 When the "physical storage" message objects are destroyed or moved
 *
 *   The messages appear as they have been removed from the set.
 *
 * 5 When the KMFolder that contains the message objects is destroyed
 *
 *   The message set becomes invalid.
 */
class MessageSet : public QObject
{
  friend class Pane;

  Q_OBJECT

protected:
  /**
   * Protected constructor. Use the Pane::createMessageSet* functions instead.
   */
  MessageSet( Pane * pane, Widget * widget, KMFolder * folder, MessageList::Core::MessageItemSetReference ref );

public:
  /**
   * Destroys the message set (but not the real messages nor the MessageItems in the View).
   */
  ~MessageSet();

private:
  Pane * mPane;                                           ///< The Pane that created this MessageSet
  Widget * mWidget;                                       ///< The Widget that was current at creation time
  KMFolder * mFolder;                                     ///< The folder that the messages belong to
  MessageList::Core::MessageItemSetReference mMessageItemSetReference; ///< The "core" set reference: don't look :)

public:

  /**
   * Returns true if this MessageSet is valid and false otherwise.
   * Please note that the invalidation may happen at any time
   * inside the main event loop. So be sure to check the return value
   * of this function after you have let some event processing occur.
   */
  bool isValid() const
    { return mFolder && mPane; };

  /**
   * Returns the KMFolder associated to this message set or 0
   * if the set is no longer valid.
   */
  KMFolder * folder() const
    { return mFolder; };

  /**
   * Returns the list of messages bound to this MessageSet.
   * The list will actually contain at most the number of messages
   * that were present in the set at the time it was created, but
   * it _may_ contain less (even none) if some of them were destroyed
   * in the meantime or the set is no longer valid for some reason.
   */ 
  QList< KMMsgBase * > contentsAsMsgBaseList();

  /**
   * If bMark is true marks the messages in this set as "about to be removed"
   * so they appear "dimmer" in the view and can't be selected.
   * If bMark is false then the "about to be removed" flag is cleared.
   * If this set is no longer valid then this function turns to a no-op.
   */
  void markAsAboutToBeRemoved( bool bMark );

  /**
   * Selects the messages currently present in this set.
   * If clearSelectionFirst is true then the selection is cleared first.
   * If this set is no longer valid then this function turns to a no-op.
   */
  void select( bool clearSelectionFirst = false );

protected:
  // Pane uses this stuff: don't look :P

  Pane * pane() const
    { return mPane; };

  Widget * widget() const
    { return mWidget; };

  MessageList::Core::MessageItemSetReference messageItemSetReference() const
    { return mMessageItemSetReference; };

private slots:

  void slotPaneDestroyed();
  void slotFolderDestroyed();

};

} // namespace MessageListView

} // namespace KMail


#endif //!__KMAIL_MESSAGELISTVIEW_MESSAGESET_H__

