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

#ifndef __KMAIL_MESSAGELISTVIEW_PANE_H__
#define __KMAIL_MESSAGELISTVIEW_PANE_H__

#include <KTabWidget>
#include <QList>

#include "messagelistview/core/enums.h"

class KMMessage;
class KMMsgBase;
class KMFolder;
class KMMainWidget;

class QToolButton;
class QIcon;

namespace KPIM
{
  class MessageStatus;
}

namespace KMail
{

class MessageTreeCollection;

namespace MessageListView
{

class Widget;
class MessageSet;

/**
 * This is the main MessageListView panel for KMail.
 * It contains multiple MessageListView::Widget tabs
 * so it can actually display multiple folders at once.
 *
 * On the other side, at the time of writing, the whole
 * KMail source code assumes that there is a single folder
 * being displayed at once. This widget then takes care
 * of appearing as a "single folder display" to anything
 * outside the MessageListView namespace.
 *
 * This is why some functions you might expect to be public
 * are actually protected or private.
 */
class Pane : public KTabWidget
{
  friend class MessageSet;
  friend class Widget;

  Q_OBJECT

public:
  /**
   * Creates a MessageListView::Pane with the specified parent
   * that must be in the children tree of the specified mainWidget.
   */
  Pane( KMMainWidget * mainWidget, QWidget *parent );
  /**
   * Destroys the MessageListView::Pane
   */
  ~Pane();

protected:
  KMMainWidget * mMainWidget;        ///< The KMMainWidget we're bound to
  Widget * mCurrentWidget;           ///< We try hard to keep this != 0
  KMFolder * mCurrentFolder;         ///< The folder we currently announce as current

  QToolButton * mNewTabButton;       ///< The "New Tab" button in the top left corner
  QToolButton * mCloseTabButton;     ///< The "Close Tab" button in the top right corner
public:

  /**
   * Returns the KMMainWidget this Pane is currently bound to
   */
  KMMainWidget * mainWidget() const
    { return mMainWidget; };

  /**
   * Returns the folder we're currently announcing as current
   * or 0 if there is no such folder.
   * Note that this might not be the only folder we display
   * but it's always the one we display in the current tab.
   */
  KMFolder * currentFolder() const
    { return mCurrentFolder; };

  /**
   * Returns true if the specified folder is already open in _some_ tab
   * (not necessairly the current one).
   */
  bool isFolderOpen( KMFolder * fld ) const;

  /**
   * Returns the current message for the current folder as a KMMessage.
   * May return 0 if there is no current message or no current folder.
   */
  KMMessage * currentMessage() const;

  /**
   * Returns the current message for the current folder as a KMMsgBase.
   * May return 0 if there is no current message or no current folder.
   */
  KMMsgBase * currentMsgBase() const;

  /**
   * Returns the currently selected KMMsgBases in the current folder.
   * The list may be empty if there are no messages or no current folder or no current
   * MessageItem. The list is valid only until you return control to Qt. Don't keep it any longer.
   * If you want to fetch complete message structures you may use the
   * (possibly more expensive) selectionAsMessageList().
   *
   * If includeCollapsedChildren is true then the children of the selected but
   * collapsed items are also added to the list.
   */
  QList< KMMsgBase * > selectionAsMsgBaseList( bool includeCollapsedChildren = true ) const;

  /**
   * Returns the currently selected KMMessages in the current folder.
   * The list may be empty if there are no messages or no current folder or no current
   * MessageItem.
   * The list is valid only until you return control to Qt. Don't keep it any longer.
   * If you want to fetch headers only you may use the
   * (possibly less expensive) selectionAsMsgBaseList().
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
   * Returns the KMMsgBase objects belonging to the current thread. The current
   * thread is the one that contains the currentMessageItem().
   * The list may be empty if there are no messages or no current folder or no current
   * MessageItem. The list is valid only until you return control to Qt. Don't keep it any longer.
   * If you want to fetch complete message structures you may use the
   * (possibly more expensive) currentThreadAsMessageList().
   */
  QList< KMMsgBase * > currentThreadAsMsgBaseList() const;

  /**
   * Returns the currently selected KMMessages in the current folder.
   * The list may be empty if there are no messages or no current folder or no current
   * MessageItem. The list is valid only until you return control to Qt. Don't keep it any longer.
   * If you want to fetch headers only you may use the
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
   * Creates a persistent message set from the selection in the currently
   * active Widget. Returns 0 if there is no currently active widget
   * or if has no selected folder.
   *
   * If includeCollapsedChildren is true then the children of the selected but
   * collapsed items are also added to the set.
   * 
   * At any later stage you can retrieve the messages still contained in
   * the set. You are responsable of deleting the returned object when
   * you no longer need it. The returned pointer is in fact a QObject so you
   * can reparent it to another QObject to get it deleted.
   */
  MessageSet * createMessageSetFromSelection( bool includeCollapsedChildren = true );

  /**
   * Creates a persistent message set from the current thread in the currently
   * active Widget. Returns 0 if there is no currently active widget
   * or if has no selected folder.
   * 
   * At any later stage you can retrieve the messages still contained in
   * the set. You are responsable of deleting the returned object when
   * you no longer need it. The returned pointer is in fact a QObject so you
   * can reparent it to another QObject to get it deleted.
   */
  MessageSet * createMessageSetFromCurrentThread();


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
   * Sets the current folder to be displayed by this Pane.
   * If the specified folder is already open in one of the tabs
   * then that tab is made current (and no reloading happens).
   * If the specified folder is not open yet then behaviour
   * depends on the preferEmptyTab value as follows.
   * If preferEmptyTab is set to false then the (new) folder is loaded
   * in the current tab. If preferEmptyTab is set to true then the (new) folder is
   * loaded in the first empty tab (or a new one if there are no empty ones).
   *
   * Pre-selection is the action of automatically selecting a message just after the folder
   * has finished loading. See Model::setStorageModel() for more information.
   */
  void setCurrentFolder(
      KMFolder *fld,
      bool preferEmptyTab = false,
      Core::PreSelectionMode preSelectionMode = Core::PreSelectLastSelected
    );

  /**
   * Makes sure that the specified KMMsgBase is current and is the only
   * selected in the view. If the message was already current and selected
   * then nothing happens. Otherwise all the selection related signals are
   * emitted. If the specified KMMsgBase is not found in the current storage
   * then both the selection and the current item are cleared.
   */
  void activateMessage( KMMsgBase *msg );

  /**
   * Returns the KPIM::MessageStatus in the current quicksearch field.
   */
  KPIM::MessageStatus currentFilterStatus() const;

  /**
   * Returns the search term in the current quicksearch field.
   */
  QString currentFilterSearchString() const;

  /**
   * Returns true if the current folder display is threaded, false otherwise
   * (or if there is no current folder display).
   */
  bool isThreaded() const;

  /**
   * If expand is true then it expands the current thread, otherwise
   * collapses it.
   */
  void setCurrentThreadExpanded( bool expand );

  /**
   * If expand is true then it expands all the threads, otherwise
   * collapses them.
   */
  void setAllThreadsExpanded( bool expand );

  /**
   * Selects the next message item in the view.
   *
   * messageTypeFilter can be used to restrict the selection to only certain message types.
   *
   * If expandSelection is true then the previous selection is retained, otherwise it's cleared.
   * If centerItem is true then the specified item will be positioned
   * at the center of the view, if possible.
   * If loop is true then the "next" algorithm will restart from the beginning
   * of the list if the end is reached, otherwise it will just stop returning false.
   */
  bool selectNextMessageItem( Core::MessageTypeFilter messageTypeFilter, bool expandSelection, bool centerItem, bool loop );

  /**
   * Selects the previous message item in the view. If expandSelection is
   * true then the previous selection is retained, otherwise it's cleared.
   * If centerItem is true then the specified item will be positioned
   * at the center of the view, if possible.
   *
   * messageTypeFilter can be used to restrict the selection to only certain message types.
   *
   * If loop is true then the "previous" algorithm will restart from the end
   * of the list if the beginning is reached, otherwise it will just stop returning false.
   */
  bool selectPreviousMessageItem( Core::MessageTypeFilter messageTypeFilter, bool expandSelection, bool centerItem, bool loop );

  /**
   * Focuses the next message item in the view without actually selecting it.
   *
   * messageTypeFilter can be used to restrict the selection to only certain message types.
   *
   * If centerItem is true then the specified item will be positioned
   * at the center of the view, if possible.
   * If loop is true then the "next" algorithm will restart from the beginning
   * of the list if the end is reached, otherwise it will just stop returning false.
   */
  bool focusNextMessageItem( Core::MessageTypeFilter messageTypeFilter, bool centerItem, bool loop );

  /**
   * Focuses the previous message item in the view without actually selecting it.
   *
   * messageTypeFilter can be used to restrict the selection to only certain message types.
   *
   * If centerItem is true then the specified item will be positioned
   * at the center of the view, if possible.
   * If loop is true then the "previous" algorithm will restart from the end
   * of the list if the beginning is reached, otherwise it will just stop returning false.
   */
  bool focusPreviousMessageItem( Core::MessageTypeFilter messageTypeFilter, bool centerItem, bool loop );

  /**
   * Selects the currently focused message item. May do nothing if the
   * focused message item is already selected (which is very likely).
   * If centerItem is true then the specified item will be positioned
   * at the center of the view, if possible.
   */
  void selectFocusedMessageItem( bool centerItem );

  /**
   * Selects the first message item in the view that matches the specified Core::MessageTypeFilter.
   * If centerItem is true then the specified item will be positioned
   * at the center of the view, if possible.
   *
   * If the current view is already loaded then the request will
   * be satisfied immediately (well... if an unread message exists at all).
   * If the current view is still loading then the selection of the first
   * message will be scheduled to be executed when loading terminates.
   *
   * So this function doesn't actually guarantee that an unread or new message
   * was selected when the call returns. Take care :)
   *
   * The function returns true if a message was selected and false otherwise.
   */
  bool selectFirstMessage( Core::MessageTypeFilter messageTypeFilter, bool centerItem );

  /**
   * Selects all the items in the current folder.
   */
  void selectAll();

  /**
   * Reloads global configuration and eventually reloads all the views.
   */
  void reloadGlobalConfiguration();

  /**
   * Sets the focus on the quick search line of the currently active tab.
   */
  void focusQuickSearch();

signals:
  /**
   * Emitted when a message has been selected in the folder we're announcing as current.
   * Note that it *may* happen that the msg parameter is NULL. This is because
   * the underlying storage may fail to provide a message. You should be prepared
   * to handle this situation.
   *
   * This signal is emitted when a SINGLE message is selected in the view, probably
   * by clicking on it or by simple keyboard navigation. When multiple items
   * are selected at once (by shift+clicking, for example) then you will get
   * this signal only for the last clicked message (or at all, if the last shift+clicked
   * thing is a group header...). You should handle selection changed in this case.
   */
  void messageSelected( KMMessage *msg );

  /**
   * Emitted when a message has been activated in the folder we're announcing as current.
   */
  void messageActivated( KMMessage *msg );

  /**
   * Emitted when selection in the view changes.
   */
  void selectionChanged();

  /**
   * Emitted when a message wants its status to be changed
   */
  void messageStatusChangeRequest( KMMsgBase *msg, const KPIM::MessageStatus &set, const KPIM::MessageStatus &clear );

  /**
   * Emitted when the current folder has changed
   */
  void currentFolderChanged( KMFolder *fld );

  /**
   * Emitted when a full search is requested.
   */
  void fullSearchRequest();

protected slots:

  void slotCurrentTabChanged( int idx );
  void slotNewTab();
  void slotCloseCurrentTab();
  void slotMessageSelected( KMMessage * msg );
  void slotMessageActivated( KMMessage * msg );
  void slotSelectionChanged();
  void slotTabContextMenuRequest( QWidget *tab, const QPoint &pos );
  void slotActionTabCloseRequest();
  void slotActionTabCloseAllButThis();
  void slotTabDragMoveEvent( const QDragMoveEvent *e, bool &accept );

protected:

  /**
   * Returns the currently active Widget (tab).
   * 
   * You shouldn't mess with this as we appear to KMMainWidget as a single folder view:
   * use the public wrappers above.
   */
  Widget * currentMessageListViewWidget() const
    { return mCurrentWidget; };

  /**
   * Returns the (first) Widget that currently displays the specified folder.
   * If such a folder isn't displayed in any of the widgets then 0 is returned.
   * By passing a fld of 0 you can find out the first empty widget, if any.
   */
  Widget * messageListViewWidgetWithFolder( KMFolder * fld ) const;

  /**
   * Creates a new tab with a new Widget instance. The Widget's View is actually empty.
   */
  Widget * addNewWidget();

  /**
   * This is used by MessageSet::currentMsgBaseList().
   * Returns the list of messages bound to the specified MessageSet.
   * The list will actually contain at most the number of messages
   * that were present in the set at the time it was created, but
   * it _may_ contain less (even none) if some of them were destroyed
   * in the meantime.
   */
  QList< KMMsgBase * > messageSetContentsAsMsgBaseList( MessageSet * set );

  /**
   * This is used by MessageSet::markAsAboutToBeRemoved().
   * If bMark is true marks the messages in the specified set as "about to be removed"
   * so they appear "dimmer" in the view and can't be selected.
   * If bMark is false then the "about to be removed" flag is cleared.
   * The MessageSet must have been created by createMessageSetFromSelection();
   */
  void messageSetMarkAsAboutToBeRemoved( MessageSet * set, bool bMark );

  /**
   * This is used by MessageSet::select().
   * The MessageSet must have been created by createMessageSetFromSelection();
   */
  void messageSetSelect( MessageSet * set, bool clearSelectionFirst );

  /**
   * This is called by the MessageSet destructor in order to free resources.
   */
  void messageSetDestroyed( MessageSet *set );

  /**
   * This is called by Widget as a request to update it's tab icon
   */
  void widgetIconChangeRequest( Widget * w, const QIcon &icon );

  /**
   * Hides/shows the tab bar (if the proper global option is enabled).
   * Enables or disables the "close tab" button.
   */
  void updateTabControls();

private:
  void internalSetCurrentWidget( Widget * newCurrentWidget );
  void internalSetCurrentFolder( KMFolder * folder );
};

} // namespace MessageListView

} // namespace KMail


#endif //!__KMAIL_MESSAGELISTVIEW_PANE_H__

