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

#ifndef __KMAIL_MESSAGELISTVIEW_WIDGET_H__
#define __KMAIL_MESSAGELISTVIEW_WIDGET_H__

#include "messagelistview/core/widgetbase.h"
#include "messagelistview/core/enums.h"

#include <QList>
#include <QIcon>

class KMMessage;
class KMMsgBase;
class KMFolder;
class KMMainWidget;

class QTimer;

namespace KMail
{

class MessageTreeCollection;

namespace MessageListView
{

namespace Core
{
  typedef unsigned long int MessageItemSetReference;

  class GroupHeaderItem;
  class MessageItem;
}

class Pane;

/**
 * KMail dependant MessageListView::Core::Widget subclass.
 *
 * Rappresents a single tab in MessageListView::Pane and
 * provides the KMail specific implementation of view*() virtual
 * handlers.
 */
class Widget : public Core::Widget
{
  Q_OBJECT
public:
  Widget( KMMainWidget *mainWidget, Pane *parent );
  ~Widget();

private:
  /**
   * Our owning MessageListView::Pane
   */
  Pane * mPane;

  /**
   * Our owning KMMainWidget
   */
  KMMainWidget * mMainWidget;

  /**
   * The base icon for the folder.
   */
  QIcon mFolderIconBase;

  enum
  {
    BusyAnimationSteps = 4
  };

  /**
   * The "busy icon" for the folder
   */
  QIcon mFolderIconBusy[ BusyAnimationSteps ];

  /**
   * The timer we use to animate the icon while busy
   */
  QTimer * mIconAnimationTimer;

  /**
   * The step of the icon animation
   */
  int mIconAnimationStep;

  // These two are leftovers from the old KMail days.
  // It's part of the logic that calls unGetMsg() on the previously
  // selected message... this will be replaced by proper reference counting one day...
  int mLastSelectedMessage;
  bool mReleaseMessageAfterCurrentChange;
public:
  /**
   * Sets the current folder for this Widget. fld may be 0.
   * Also sets the icon for the current folder: Widget will eventually manipulate it
   * and ask Pane to set it as tab icon.
   *
   * Pre-selection is the action of automatically selecting a message just after the folder
   * has finished loading. See Model::setStorageModel() for more informations.
   */
  void setFolder( KMFolder *fld, const QIcon &icon, Core::PreSelectionMode preSelectionMode = Core::PreSelectLastSelected );

  /**
   * Returns the current folder. May be 0.
   */
  KMFolder * folder() const;

  /**
   * Returns the current MessageItem in the current folder.
   * May return 0 if there is no current message or no current folder.
   */
  Core::MessageItem * currentMessageItem() const;

  /**
   * Returns the current message for the current folder as KMMsgBase.
   * May return 0 if there is no current message or no current folder.
   * This may be faster than currentMsgBase() but returns headers only.
   */
  KMMsgBase * currentMsgBase() const;

  /**
   * Returns the current message for the current folder as KMMessage.
   * May return 0 if there is no current message or no current folder.
   * This may be slower than currentMsgBase() but returns the whole message.
   */
  KMMessage * currentMessage() const;

  /**
   * Returns the Core::MessageItem that corresponds to the specified KMMsgBase.
   * Please note that this function may be time consuming since it basically
   * performs a linear search.
   */
  Core::MessageItem * messageItemFromMsgBase( KMMsgBase * msg ) const;

  /**
   * Returns the Core::MessageItem that corresponds to the specified KMMessage.
   * Please note that this function may be time consuming since it basically
   * performs a linear search.
   */
  Core::MessageItem * messageItemFromMessage( KMMessage * msg ) const;

  /**
   * Returns the currently selected MessageItems in the current folder.
   * The list may be empty if there are no selected messages or no current folder.
   * The list is valid only until you return control to Qt. Don't keep it any longer.
   *
   * If includeCollapsedChildren is true then the children of the selected but
   * collapsed items are also added to the list.
   */
  QList< Core::MessageItem * > selectionAsMessageItemList( bool includeCollapsedChildren = true ) const;

  /**
   * Returns the currently selected KMMsgBases in the current folder.
   * The list may be empty if there are no selected messages or no current folder.
   * The list is valid only until you return control to Qt. Don't keep it any longer.
   * If you want to fetch complete message structures you may use the
   * (possibly more expensive) selectionAsMessageList().
   *
   * If includeCollapsedChildren is true then the children of the selected but
   * collapsed items are also added to the list.
   */
  QList< KMMsgBase * > selectionAsMsgBaseList( bool includeCollapsedChildren = true ) const;

  /**
   * Returns the currently selected KMMessages in the current folder.
   * The list may be empty if there are no selected messages or no current folder.
   * The list is valid only until you return control to Qt. Don't keep it any longer.
   * If you want to fetch headers only you may use the
   * (possibly less expensive) selectionAsMessageList().
   *
   * If includeCollapsedChildren is true then the children of the selected but
   * collapsed items are also added to the list.
   */
  QList< KMMessage * > selectionAsMessageList( bool includeCollapsedChildren = true ) const;

  /**
   * Returns the currently selected KMMsgBases in the current folder as a MessageTreeCollection.
   * The returned value may be 0 if there are no selected messages or no current folder.
   * The collection is valid only until you return control to Qt. Don't keep it any longer.
   * You're responsable of deleting the collection when you're done with it.
   *
   * If includeCollapsedChildren is true then the children of the selected but
   * collapsed items are also added to the collection.
   */
  MessageTreeCollection * selectionAsMessageTreeCollection( bool includeCollapsedChildren = true ) const;

  /**
   * Makes sure that the specified KMMsgBase is current and is the only
   * selected in the view. If the message was already current and selected
   * then nothing happens. Otherwise all the selection related signals are
   * emitted. If the specified KMMsgBase is not found in the current storage
   * then both the selection and the current item are cleared.
   */
  void activateMessageItemByMsgBase( KMMsgBase * msg );

  /**
   * Returns the MessageItems belonging to the current thread. The current
   * thread is the one that contains the currentMessageItem().
   * The list may be empty if there are no messages, no current folder or no
   * current MessageItem.
   * The items in the returned list are guaranteed to be valid only
   * inside the context of your current stack frame. Don't keep them any longer.
   */
  QList< Core::MessageItem * > currentThreadAsMessageItemList() const;

  /**
   * Returns the KMMsgBase objects belonging to the current thread. The current
   * thread is the one that contains the currentMessageItem().
   * The list may be empty if there are no messages, no current folder or no
   * current MessageItem. If you want to fetch complete message structures you may use the
   * (possibly more expensive) currentThreadAsMessageList().
   */
  QList< KMMsgBase * > currentThreadAsMsgBaseList() const;

  /**
   * Returns the KMMessages objects belonging to the current thread. The current
   * thread is the one that contains the currentMessageItem().
   * The list may be empty if there are no messages, no current folder or no
   * current MessageItem. If you want to fetch headers only you may use the
   * (possibly less expensive) currentThreadAsMsgBaseList().
   */
  QList< KMMessage * > currentThreadAsMessageList() const;

  /**
   * Returns the KMMsgBases objects belonging to the current thread as a MessageTreeCollection.
   * The current thread is the one that contains the currentMessageItem().
   * The returned value may be 0 if there are no selected messages or no current folder.
   * The collection is valid only until you return control to Qt. Don't keep it any longer.
   * You're responsable of deleting the collection when you're done with it.
   *
   * If includeCollapsedChildren is true then the children of the selected but
   * collapsed items are also added to the collection.
   */
  MessageTreeCollection * currentThreadAsMessageTreeCollection() const;

  /**
   * Fills the lists of the selected message serial numbers and of the selected+visible ones.
   * Returns true if the returned stats are valid (there is a current folder after all)
   * and false otherwise. This is called by KMMainWidget in a single place so we optimize by
   * making it a single sweep on the selection.
   *
   * If includeCollapsedChildren is true then the children of the selected but
   * collapsed items are also included in the stats
   */
  bool getSelectionStats(
      QList< Q_UINT32 > &selectedSernums,
      QList< Q_UINT32 > &selectedVisibleSernums,
      bool * allSelectedBelongToSameThread,
      bool includeCollapsedChildren = true
    ) const;

  /**
   * Fast function that determines if the selection is empty
   */
  bool selectionEmpty() const;

  /**
   * Creates a persistent set of messages from the current selection, if any.
   * If the set couldn't be created (probably because nothing is selected)
   * then static_cast< Core::MessageItemSetReference >( 0 ) is returned.
   * You can use the returned reference at any later stage to retrieve
   * the list of associated messages. 
   * Persistent sets consume resources (memory AND CPU time). Be sure
   * to call deletePersistentSet() when you no longer need it.
   */
  Core::MessageItemSetReference createPersistentSetFromMessageItemList( const QList< Core::MessageItem * > &items );

  /**
   * Returns the list of messages associated to the specified MessageItemSet.
   * The returned list of messages is guaranteed to be safe: that is
   * only the messages that still exist are returned.
   */
  QList< KMMsgBase * > persistentSetContentsAsMsgBaseList( Core::MessageItemSetReference ref ) const;

  /**
   * Selects the items contained in the specified set.
   * If clearSelectionFirst is true then the selection is first cleared, otherwise
   * it's retained.
   */
  void selectPersistentSet( Core::MessageItemSetReference ref, bool clearSelectionFirst ) const;

  /**
   * If bMark is true this function marks the messages in the persistent set
   * as "about to be removed" so they appear dimmer and aren't selectable in the view.
   * If bMark is false then this function clears the "about to be removed" state.
   */
  void markPersistentSetAsAboutToBeRemoved( Core::MessageItemSetReference ref, bool bMark );

  /**
   * Destroys the specified persistent MessageItemSet.
   */
  void deletePersistentSet( Core::MessageItemSetReference ref );

  /**
   * Returns true if this drag can be accepted by the underlying view
   */
  bool canAcceptDrag( const QDragMoveEvent * e );

signals:
  /**
   * Emitted when a message is selected (that is, single clicked and thus made current in the view)
   * Note that this message CAN be 0 (when the current item is cleared, for example).
   *
   * This signal is emitted when a SINGLE message is selected in the view, probably
   * by clicking on it or by simple keyboard navigation. When multiple items
   * are selected at once (by shift+clicking, for example) then you will get
   * this signal only for the last clicked message (or at all, if the last shift+clicked
   * thing is a group header...). You should handle selection changed in this case.
   */
  void messageSelected( KMMessage *msg );

  /**
   * Emitted when a message is doubleclicked or activated by other input means
   */
  void messageActivated( KMMessage *msg );

  /**
   * Emitted when the selection in the view changes.
   */
  void selectionChanged();

  /**
   * Emitted when a message wants its status to be changed
   */
  void messageStatusChangeRequest( KMMsgBase *msg, const KPIM::MessageStatus &set, const KPIM::MessageStatus &clear );

protected:
  /**
   * Reimplemented from MessageListView::Core::Widget
   */
  virtual void viewMessageSelected( Core::MessageItem *msg );

  /**
   * Reimplemented from MessageListView::Core::Widget
   */
  virtual void viewMessageActivated( Core::MessageItem *msg );

  /**
   * Reimplemented from MessageListView::Core::Widget
   */
  virtual void viewSelectionChanged();

  /**
   * Reimplemented from MessageListView::Core::Widget
   */
  virtual void viewMessageListContextPopupRequest( const QList< Core::MessageItem * > &selectedItems, const QPoint &globalPos );

  /**
   * Reimplemented from MessageListView::Core::Widget
   */
  virtual void viewGroupHeaderContextPopupRequest( Core::GroupHeaderItem *group, const QPoint &globalPos );

  /**
   * Reimplemented from MessageListView::Core::Widget
   */
  virtual void viewDragEnterEvent( QDragEnterEvent * e );

  /**
   * Reimplemented from MessageListView::Core::Widget
   */
  virtual void viewDragMoveEvent( QDragMoveEvent * e );

  /**
   * Reimplemented from MessageListView::Core::Widget
   */
  virtual void viewDropEvent( QDropEvent * e );

  /**
   * Reimplemented from MessageListView::Core::Widget
   */
  virtual void viewStartDragRequest();

  /**
   * Reimplemented from MessageListView::Core::Widget
   */
  virtual void viewMessageStatusChangeRequest( Core::MessageItem *msg, const KPIM::MessageStatus &set, const KPIM::MessageStatus &clear );

  /**
   * Reimplemented from MessageListView::Core::Widget
   */
  virtual void viewJobBatchStarted();

  /**
   * Reimplemented from MessageListView::Core::Widget
   */
  virtual void viewJobBatchTerminated();

private:
  /**
   * Internal helper which re-builds the structure of the Core::MessageItem objects
   * given in the list. The message threads are re-built to the possible degree
   * (as list can contain even disjoint parts of multiple threads).
   */
  MessageTreeCollection * itemListToMessageTreeCollection( const QList< Core::MessageItem * > &list ) const;

private slots:
  void animateIcon();

};

} // namespace MessageListView

} // namespace KMail


#endif //!__KMAIL_MESSAGELISTVIEW_WIDGET_H__

