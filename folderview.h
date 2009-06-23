/******************************************************************************
 *
 * Copyright (c) 2007 Volker Krause <vkrause@kde.org>
 * Copyright (c) 2008 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *****************************************************************************/

#ifndef __KMAIL_FOLDERVIEW_H__
#define __KMAIL_FOLDERVIEW_H__

#include "kmail_export.h"

#include <libkdepim/foldertreewidget.h>

#include <QList>
#include <QPointer>
#include <QStringList>

class KMAccount;
class KMFolderDir;
class KMMainWidget;
class KMFolder;
class KMenu;

class QAction;
class QEvent;
class QTimer;
class QTime;



namespace KMail
{

class FolderView;
class FolderViewItem;

// FIXME: Move to own file.
/**
 * @brief Global namespace related to dragging lists of KMFolders
 *
 * Contains the global list of KMFolder pointers that are currently
 * dragged around KMail (notably between instances of FolderView).
 */
class DraggedFolderList
{
public:
  /**
   * Set the global list of dragged KMFolders.
   * This list is to be considered valid only during a dnd operation.
   */
  static void set( QList< QPointer< KMFolder > > &list );

  /**
   * Clear the global list of dragged KMFolders.
   */
  static void clear();

  /**
   * Returns the global list of dragged KMFolders.
   */
  static QList< QPointer< KMFolder > > get();

  /**
   * Returns the mimetype associated to the dragged folder list.
   * This is an opaque (and rather distinct) string.
   */
  static QString mimeDataType();
};


/**
 * A concentrator class that handles an unique selection for all the
 * FolderView widgets present inside a single KMMainWidget.
 *
 * It's purpose is to keep the current selection of the
 * views synchronized and to provide an unique well identified source for the
 * folderActivated() signal.
 */
class FolderViewManager : public QObject
{
  friend class ::KMMainWidget;
  friend class FolderView;

  Q_OBJECT
protected:
  /**
   * This is intended to be called only by KMMainWidget.
   * Constructs an instance of a FolderViewManager.
   * The manager must be created early (before all the FolderView
   * instances are created) and destroyed late (after all the
   * FolderView instances are destroyed.
   *
   * There should be only one FolderViewManager per KMMainWidget.
   *
   * Please note that mainWidget is not used as parent of this
   * object. KMMainWidget actually deletes it in the constructor
   * and sets the relevant variable to 0 (so the dying views
   * can check it and avoid a crash at shutdown).
   */
  FolderViewManager( KMMainWidget *mainWidget, const char *name );

Q_SIGNALS:
  /**
   * Emitted when a KMFolder is activated in one of the FolderView widgets
   * managed by this instance. fld may be 0 if an item without a folder
   * is activated: this will usually mean "local root" or "searches".
   */
  void folderActivated( KMFolder *fld );

  /**
   * Also emitted when a KMFolder is activated in one of the FolderView widgets
   * managed by this instance. fld may be 0 if an item without a folder
   * is activated: this will usually mean "local root" or "searches".
   * This signal also provides information about the button being pressed.
   */
  void folderActivated( KMFolder *fld, bool middleButton );

protected:
  /**
   * Called by FolderView to register an instance. Not meant to be called
   * from other places.
   */
  void attachView( FolderView *view );

  /**
   * Called by FolderView to unregister an instance. Not meant to be called
   * from other places.
   */
  void detachView( FolderView *view );

  /**
   * Called by FolderView to signal that a KMFolder has been activated.
   * Actually triggers the folderActivated() signal and notifies
   * all the other managed views of the change.
   */
  void viewFolderActivated( FolderView *view, KMFolder *fld, bool middleButton );

private:
  QList<FolderView *> mFolderViews; ///< The list of all the active folder views.

};


/**
 * @brief The base class of all the folder views in KMail
 *
 * The items displayed can either contain a real KMFolder object
 * or be virtual entries that help organizing the structure
 * into a tree. In fact the "physical" folders managed by KMail
 * are spanned through multiple trees and handled by different
 * managers. This widget acts as a "glue" and other components
 * in the app refers to it when manipulating the folder tree
 * as a whole.
 *
 * There is a complex framework attached to the contextual
 * menu which allows manipulation of the folders. Some of the actions
 * are actually implemented in this widget while some others
 * are relayed to the global ones set up by KMMainWidget.
 *
 * This widget also implements an interesting machinery for lazy
 * message count updates. 
 *
 * Items requiring an update are marked as dirty and a short-timeout
 * timer is started (if not already running).
 * The timer is connected to the slotUpdateCounts() slot which
 * iterates through the items and updates the dirty ones.
 * 
 * Items are marked as dirty mostly in two paths.
 * 
 * The first one comes from the KMFolder signals which are connected to
 * each FolderViewItem. The FolderViewItem, in turn, marks
 * itself as "dirty" and asks the FolderView to fire up the timer.
 *  
 * When most (or all) of the items require a message count refresh
 * (at startup, for example, or when a column is hidden/shown) then
 * the second path jumps in. All the items are marked as dirty
 * and slotUpdateCounts() is called directly (since it's senseless
 * to wait for more items to become dirty).
 *
 * Since the count updates might be time consuming (finding out a
 * message count in a very large folder might take its time) then the
 * count refresh is done through idle processing. slotUpdateCounts()
 * works in timed chunks of ~100 msecs. When the slotUpdateCount() detects
 * that it's running for more than ~100 msecs but more job needs to be done,
 * it interrupts the processing and starts a zero-timeout timer connected to
 * itself.
 *
 * Another complex framework is the dnd handling. The user can actually
 * move, copy and sort folders by dragging them around, even between
 * different views. Some derived classes have subtle differences
 * in how drags are handled and thus this widget exposes
 * a virtual interface for them to be reimplemented.
 *
 * This widget contains few pure virtual functions that obviously
 * *must* be overridden in derived classes. This is just to make
 * sure you don't attempt to instantiate a plain FolderView which
 * isn't specialized.
 */
class KMAIL_EXPORT FolderView : public KPIM::FolderTreeWidget
{
  friend class ::KMMainWidget;
  friend class FolderViewItem;
  friend class FolderViewManager;

  Q_OBJECT

public: // constructors & destructors

  /**
   * Construct a FolderView and attach it to the specified parent.
   * mainWidget is the main KMail window we're attacched to and can
   * be obviously different than parent.
   *
   * @param configPrefix An unique configuration prefix string
   */
  explicit FolderView(
      KMMainWidget *mainWidget,
      FolderViewManager *manager,
      QWidget *parent,
      const QString &configPrefix,
      const char *name = 0 
    );

  ~FolderView();

public: // enums

  /**
   * Enumeration of logical column indexes.
   * Don't mess with it unless you also sync the addColumn() sequence
   * in the constructor.
   * In fact KPIM::FolderTreeWidget has a runtime interface for these
   * indexes but in this widget we KNOW that the indexes are constant
   * and we use this knowledge as an optimization.
   */
  enum ColumnLogicalIndex
  {
    LabelColumn,
    UnreadColumn,
    TotalColumn,
    DataSizeColumn
  };

  /**
   * Position relative to a reference item.
   * Used in setDropIndicatorData() and in other places around.
   */
  enum DropInsertPosition
  {
    AboveReference,  ///< Insertion is above the reference item
    BelowReference   ///< Insertion is below the reference item
  };

public: // publicly callable stuff

  /**
   * Returns the main widget that this widget is a child of.
   */
  KMMainWidget* mainWidget() const
    { return mMainWidget; }

  /**
   * Finds the first FolderViewItem that has the specified physical folder.
   * Obviously returns 0 if there is no such item in the view.
   */
  FolderViewItem * findItemByFolder( const KMFolder *folder );

  /**
   * Finds the first FolderViewItem that has the specified protocol and folderType()
   * Obviously returns 0 if there is no such item in the view.
   */
  FolderViewItem * findItemByProperties(
      KPIM::FolderTreeWidgetItem::Protocol proto,
      KPIM::FolderTreeWidgetItem::FolderType type
    );

  /**
   * Get/refresh the folder tree.
   * If openFoldersForUpdate is true then all the underlying
   * folders are opened for count updates. In this case reloading might be a time
   * consuming operation.
   */
  void reload( bool openFoldersForUpdate = false );

  /**
   * (Re-)read the configuration and perform an update
   */
  void readConfig();

  /**
   * Store configuration.
   */
  void writeConfig();

  /**
   * Performs a configuration file cleanup by removing
   * the entries that are not actually present in the tree view.
   */
  void cleanupConfigFile();

  /**
   * Returns the folder attacched to the current item
   * or 0 if there is no current item (or it has no folder)
   */
  KMFolder * currentFolder() const;

  /**
   * Returns the full path of the currently selected item
   */
  QString currentItemFullPath() const;

  /**
   * Attempts to make current the item containing the specified folder.
   * If folder is 0 then attempts to activate the Local/Root item.
   * In any case the current selection is cleared (so if the specified
   * item is not found, no current selection is present in the view).
   */
  void setCurrentFolder( KMFolder *folder );

  /**
   * Attempts to make current the next item that contains a folder with unread messages.
   * If confirm is true then a confirmation dialog is eventually shown.
   * Returns true if a next unread folder could be actually selected, false otherwise.
   */
  bool selectNextUnreadFolder( bool confirm = false );

  /**
   * Attempts to make current the previous item that contains a folder with unread messages.
   * Returns true if a previous unread folder could be actually selected, false otherwise.
   */
  bool selectPrevUnreadFolder();

  /**
   * Returns the list of currently selected folders.
   */
  QList< QPointer< KMFolder > > selectedFolders();

  /**
   * A list of flags that can be used in createFolderList
   */
  enum FolderListingFlags
  {
    IncludeLocalFolders = 1,         ///< Include local (Mbox | Maildir) folders in the listing
    IncludeImapFolders = 2,          ///< Include IMAP folders in the listing
    IncludeCachedImapFolders = 4,    ///< Include Disconnected IMAP folders in the listing
    IncludeSearchFolders = 8,        ///< Include search folders in the listing
    IncludeUnknownFolders = 16,      ///< Hm... :D
    IncludeAllFolders = IncludeLocalFolders | IncludeImapFolders | IncludeCachedImapFolders | IncludeSearchFolders | IncludeUnknownFolders,
    SkipFoldersWithNoContent = 32,   ///< Skip folders that have no content
    SkipFoldersWithNoChildren = 64,  ///< Skip folders that have no children
    IncludeFolderIndentation = 128,  ///< Add spacing indentation prefixes to the folder names
    AppendListing = 256              ///< Do not clear the buffer lists before adding entries
  };

  /**
   * Create a list of folders matching specified criteria.
   *
   * The passed list buffers are cleared at the entry of the function unless you pass AppendListing
   * in listingFlags.
   *
   * @param folderNames If not null, put the list of folder names in this list
   * @param folders If not null, put the list of folder pointers in this list
   * @param flags A combination of FolderListingFlags that specify the listing criteria
   */
  void createFolderList( QStringList *folderNames, QList< QPointer< KMFolder > > *folders, int flags );

  /**
   * Show an "add child folder" dialog and eventually add a child folder to fld.
   *
   * @param fld The parent folder for the new child
   * @param par The parent widget for the "add child folder" dialog.
   */
  void addChildFolder( KMFolder *fld, QWidget *par );

  /**
   * The possible tooltip display policies.
   */
  enum ToolTipDisplayPolicy
  {
    DisplayAlways,           ///< Always display a tooltip when hovering over an item
    DisplayWhenTextElided,   ///< Display the tooltip if the item text is actually elided
    DisplayNever             ///< Nevery display tooltips
  };

  /**
   * Sets the tooltip policy to one of the ToolTipDisplayPolicy states.
   */
  void setToolTipDisplayPolicy( ToolTipDisplayPolicy policy )
    { mToolTipDisplayPolicy = policy; };

  /**
   * Returns the currently set tooltip display policy.
   * This defaults to DisplayAlways but is also stored in the config
   * and is configurable by the means of the header context popup menu.
   */
  ToolTipDisplayPolicy toolTipDisplayPolicy() const
    { return mToolTipDisplayPolicy; };

  /**
   * The available sorting policies.
   */
  enum SortingPolicy
  {
    SortByCurrentColumn,      ///< Columns are clickable, sorting is by the current column
    SortByDragAndDropKey      ///< Columns are NOT clickable, sorting is done by drag and drop
  };

  /**
   * Sets the sorting policy for this view.
   * This defaults to SortByCurrentColumn but is also stored in the config
   * and is configurable by the means of the header context popup menu.
   */
  void setSortingPolicy( SortingPolicy policy );

  /**
   * Returns the currently set sorting policy.
   */
  SortingPolicy sortingPolicy() const
    { return mSortingPolicy; };

public Q_SLOTS:
  /**
   * Move the focus on the next folder (but do not select it)
   * Used by KMMainWidget actions for keyboard navigation.
   */
  void slotFocusNextFolder();

  /**
   * Move the focus on the prev folder (but do not select it)
   * Used by KMMainWidget actions for keyboard navigation.
   */
  void slotFocusPrevFolder();

  /**
   * Select the currently focused folder
   * Used by KMMainWidget actions for keyboard navigation.
   */
  void slotSelectFocusedFolder();

  /**
   * Clipboard management slot. Copy the current folder.
   */
  void slotCopyFolder();

  /**
   * Clipboard management slot. Cut the current folder.
   */
  void slotCutFolder();

  /**
   * Clipboard management slot. Paste the current folder.
   */
  void slotPasteFolder();

protected:

  /**
   * Return true if the specified item will accept the drop of
   * the currently dragged messages. Return false otherwise.
   */
  virtual bool acceptDropCopyOrMoveMessages( FolderViewItem *item );

  /**
   * enum used in dragMode()
   */
  enum DragMode {
    DragCopy,    ///< Copy the currently dragged items
    DragMove,    ///< Move the currently dragged items
    DragCancel   ///< Abort the drag operation
  };

  /**
   * Returns the dnd mode for the currently dropped item.
   * Either looks up in the options or shows a popup menu
   * that asks the user for what to do.
   */
  DragMode dragMode( bool alwaysAsk = false, bool canMove = true );

  /**
   * Read color options and set palette.
   */
  virtual void readColorConfig();

  /**
   * Checks if the local inbox should be hidden.
   */
  bool hideLocalInbox() const;

  /**
   * Handle drop of a MailList object.
   */
  void handleMailListDrop( QDropEvent *event, KMFolder *destination );

  /**
   * Recursively add a directory of folders to the tree by appending
   * them as children of the specified parent.
   */
  void addDirectory( bool createRoots, KMFolderDir *fdir, FolderViewItem *parent );

  /**
   * Called by the FolderViewItem slotFolderCountsChanged() in order
   * to trigger a lazy count update. The update is delayed up to ~500 msecs.
   */
  void triggerLazyUpdateCounts();

  /**
   * Triggers an immediate update of counts for the items marked as dirty.
   * The update fires after a 0 msecs timeout.
   */
  void triggerImmediateUpdateCounts();

  /**
   * Recursive part of reload()
   */
  void reloadInternal( QTreeWidgetItem *item );

  /**
   * Saves the state of all the items in the view to the main configuration file.
   */
  void saveItemStates();

  /**
   * Restores the state of all the items in the view from the main configuration file.
   */
  void restoreItemStates();

  /**
   * Updates the cut, copy and paste folder actions attacched to the main widget.
   */
  void updateCopyActions();

  /**
   * Handles new item activation: selects the item, makes it current
   * and notifies the manager of the change.
   * If keepSelection is false then the current selection is cleared
   * and only the specified item is selected. If keep selection is true
   * then the current selection set is preserved.
   * middleButton keeps track of the button that clicked the item: activation
   * with middle button may be handled differently by the users of this class.
   */
  void activateItem( FolderViewItem *fvi, bool keepSelection = false, bool middleButton = false );

  /**
   * Called by the FolderViewManager to notify us of current folder changes.
   * Sets the folder as current without notifying back the FolderViewManager.
   */
  void notifyFolderActivated( KMFolder *fld );

  /**
   * A helper used by both activateItem() and notifyFolderActivated().
   * Can be overridden in derived classes to add additional behaviour.
   */
  virtual void activateItemInternal( FolderViewItem *fvi, bool keepSelection, bool notifyManager, bool middleButton );

  /**
   * Resets the dynamic list of children for the specified item or for the current item
   * if fvi is 0. If startListing is true then also start
   * a new child folder listing operation.
   * This is meaningful only for IMAP folders.
   */
  void resetFolderList( FolderViewItem *fvi, bool startListing = true );

  /**
   * Moves or copies the folders specified in sources to the destination folder.
   */
  void moveOrCopyFolders( const QList<QPointer<KMFolder> > &sources, KMFolder* destination, bool move );

  /**
   * Internal helper for moveOrCopyFolders needed to manage DND correctly.
   * Returns true if the operation can be successfully initiated and false otherwise.
   */
  bool moveOrCopyFoldersInternal( const QList<QPointer<KMFolder> > &sources, KMFolder* destination, bool move );

  /**
   * Set focus on the specified item, but do not select it.
   * FIXME: With Qt4.4 this seems to be a bit broken.
   * @param it Item to focus: may be null (in that case this function turns to a no-op)
   */
  void focusItem( QTreeWidgetItem *item );

  /**
   * Check if the specified item contains a folder with unread
   * messages. If yes and confirm is true then show a dialog that asks
   * the user if he really wishes to move to the folder found.
   * If confirmation is obtained or it is not required then make the
   * specified item current.
   * Return true if the folder was finally selected and false otherwise.
   */
  bool selectFolderIfUnread( FolderViewItem *fvi, bool confirm );

  /**
   * Updates the sorting keys for the children of the specified item.
   * The children are assigned their current position as key.
   *
   * The keys are used in DND sorting.
   */
  void fixSortingKeysForChildren( QTreeWidgetItem *item );

  /**
   * Shared handler for message dragMove event. Make virtual
   * if you need to override it in a subclass.
   */
  void handleMessagesDragMoveEvent( QDragMoveEvent *e );

  /**
   * Shared handler for message drop event. Make virtual
   * if you need to override it in a subclass.
   */
  void handleMessagesDropEvent( QDropEvent *e );

  /**
   * Shared handler for folders dragMove event. This implementation
   * ignores the event: subclasses should provide their own.
   */
  virtual void handleFoldersDragMoveEvent( QDragMoveEvent *e );

  /**
   * Shared handler for folders drop event. This implementation
   * ignores the event: subclasses should provide their own.
   */
  virtual void handleFoldersDropEvent( QDropEvent *e );

  /**
   * Used to add a folder item to this tree based on the parameter data.
   * May return 0 instead of a folder, if the folder has to be filtered out.
   */
  virtual FolderViewItem * createItem(
      FolderViewItem *parent,
      const QString &label,
      KMFolder *folder,
      KPIM::FolderTreeWidgetItem::Protocol proto,
      KPIM::FolderTreeWidgetItem::FolderType type
    ) = 0;

  /**
   * Adds a set of account related actions for the specified item to the specified context menu.
   */
  virtual void fillContextMenuAccountRelatedActions( KMenu *menu, FolderViewItem *item, bool multiSelection );

  /**
   * Adds a set of message related actions for the specified item to the specified context menu.
   */
  virtual void fillContextMenuMessageRelatedActions( KMenu *menu, FolderViewItem *item, bool multiSelection );

  /**
   * Adds a set of folder tree structure related actions for the specified item to the specified context menu.
   */
  virtual void fillContextMenuTreeStructureRelatedActions( KMenu *menu, FolderViewItem *item, bool multiSelection );

  /**
   * Adds a set of folder view structure related actions for the specified item to the specified context menu.
   */
  virtual void fillContextMenuViewStructureRelatedActions( KMenu *menu, FolderViewItem *item, bool multiSelection );

  /**
   * Adds a set of folder service related actions for the specified item to the specified context menu.
   */
  virtual void fillContextMenuFolderServiceRelatedActions( KMenu *menu, FolderViewItem *item, bool multiSelection );

  /**
   * Adds actions to the context mneu when no item (i.e. whitespace in the listview) is clicked
   */
  virtual void fillContextMenuNoItem( KMenu *mneu );

  /**
   * Intercepts QHelpEvent for tooltip management
   */
  virtual bool event( QEvent *e );

  /**
   * Intercepts mouse press in order to remember the button pressed (and
   * use this information in the clicked() handler).
   */
  virtual void mousePressEvent( QMouseEvent *e );

  /**
   * Intercepts mouse releases in order to change the current folder
   * and do a couple of nasty things when the middle button is used.
   *
   * We're not using the pressed() since it may be triggered also before
   * a dnd action. We're not using currentItemChanged() since it may
   * be triggered during a reload. We're not using clicked() since
   * it doesn't contain mouse button information...
   */
  virtual void mouseReleaseEvent( QMouseEvent *e );

  /**
   * Show a contextual popup menu, if there is a current item...
   */
  virtual void contextMenuEvent( QContextMenuEvent *e );

  /**
   * Starts a drag operation for the currently selected items.
   */
  virtual void startDrag( Qt::DropActions supportedActions );

  /**
   * Handles drag enters. We accept messages and folders.
   */
  virtual void dragEnterEvent( QDragEnterEvent *e );

  /**
   * Handles drag move events. We (eventually) accept messages and folders.
   */
  virtual void dragMoveEvent( QDragMoveEvent *e );

  /**
   * Handles drop events. We (eventually) accept messages and folders.
   */
  virtual void dropEvent( QDropEvent *e );

  /**
   * DnD handling.
   */
  virtual void dragLeaveEvent( QDragLeaveEvent *e );

  /**
   * Reimplemented in order to allow for runtime icon size changes
   */
  virtual bool fillHeaderContextMenu( KMenu * menu, const QPoint &clickPoint );

  /**
   * Reimplemented in order to draw our custom drop indications
   */
  virtual void paintEvent( QPaintEvent *e );

  /**
   * Internal helper for drop our custom drop indicator
   */
  void setDropIndicatorData(
      FolderViewItem *moveTargetItem,
      FolderViewItem *sortReferenceItem,
      DropInsertPosition sortReferencePosition = AboveReference
    );

  /**
   * Builds the list of items to be carried in a drag operation.
   * If item is selected it's added to the list otherwise
   * the function is called recursively on the children.
   */
  void buildDragItemList( QTreeWidgetItem *item, QList< FolderViewItem * > &lDragItemList );

  /**
   * Returns the list of mime-types that this widget provides.
   * This is a must-override in order to get DND to work.
   * Nobody except Qt itself actually uses this function.
   */
  virtual QStringList mimeTypes() const;

  /**
   * Returns the list of supported drop actions.
   * This is a must-override in order to get DND to work.
   * Nobody except Qt itself actually uses this function.
   */
  virtual Qt::DropActions supportedDropActions() const;

  enum UpdateCountsResult
  {
    OperationCompletedDirty, ///< all the children counts were updated, some of the children were dirty
    OperationCompletedClean, ///< all the children counts were updated, none of the children were dirty
    OperationInterrupted     ///< operation interrupted due to a timeout
  };

  /**
   * Updates the counts for children of the specified item starting from the leafs.
   * attempts to run for a maximum of 100 msecs.
   */
  UpdateCountsResult updateCountsForChildren( QTreeWidgetItem *it, const QTime &tStart );

protected Q_SLOTS:

  /**
   * Resets all column sizes to some default values.
   * The size, unread and total column get 70 pixels, label column the rest
   */
  void setDefaultColumnSizes();

  /**
   * Connected to our own signal columnVisibilityChanged()
   * Used to trigger columnsChanged() which is handled by KMMainWindow
   * and to eventually trigger a reload.
   */
  void slotColumnVisibilityChanged( int logicalIndex );

  /**
   * Connected to the itemClicked() signal of the QTreeWidget, just forwards to
   * activateItem().
   */
  void slotItemClicked( QTreeWidgetItem *item, int column );

  /**
   * Connected to the header context menu: allows for changing the sorting policy
   */
  void slotHeaderContextMenuChangeSortingPolicy( bool );

  /**
   * Connected to the header context menu: allows for changing the icon size.
   */
  void slotHeaderContextMenuChangeIconSize( bool );

  /**
   * Connected to the header context menu: allows for changing the tooltip policy.
   */  
  void slotHeaderContextMenuChangeToolTipDisplayPolicy( bool );

  /**
   * Updates the counts of the items marked with "countsDirty()".
   * Also connected to the mUpdateCountsTimer timeout() signal.
   */
  void slotUpdateCounts();

  /**
   * Calls reload( false )
   */
  void slotReload();

  /**
   * Reload the folder tree (using a single shot timer)
   */
  void slotDelayedReload();

  /**
   * Called when a KMFolder is removed and we should remove the corresponding item.
   */
  void slotFolderRemoved( KMFolder *folder );

  /**
   * Adds the currently selected folders to the favorite folders view
   */
  void slotAddToFavorites();

  /**
   * Used to re-enable item dragging after a copy or move operation has finished.
   */
  void slotFolderMoveOrCopyOperationFinished();

  /**
   * Checks mail for the currently selected folder
   */
  void slotCheckMail();

  /**
   * Starts posting a new message to the currently selected folder's mailing list.
   */
  void slotNewMessageToMailingList();

  /**
   * Adds a child folder to the folder owned by the current item.
   */
  void slotAddChildFolder();

  /**
   * Calls resetFolderList( 0, true )
   */
  void slotResetFolderList();

  /**
   * Used internally to track account removals (in order to update the view)
   */
  void slotAccountRemoved( KMAccount * );

  /**
   * Used to dynamically update the tree for certain types of accounts
   */
  void slotFolderExpanded( QTreeWidgetItem *item );

  /**
   * Used to dynamically update the tree for certain types of accounts
   */
  void slotFolderCollapsed( QTreeWidgetItem *item );

Q_SIGNALS:
  /**
   * Emitted when the visibility of columns changes.
   */
  void columnsChanged();

  /**
   * Emitted when a new folder is selected in the view.
   * Folder may be 0 if the Root item has been selected.
   */
  void folderSelected( KMFolder *folder );

  /**
   * Emitted when a new folder with unread message is selected in the view.
   * Folder may be 0 if the Root item has been selected.
   * FIXME: What is the purpose of this ?
   */
  //void folderSelectedUnread( KMFolder *folder );

  /**
   * The icon of one of our folders has changed
   */
  void iconChanged( FolderViewItem * );

  /**
   * The name of one of our folders has changed
   */
  void nameChanged( FolderViewItem * );

  /**
   * Messages have been dropped onto a folder
   */
  void folderDrop(KMFolder*);

  /**
   * Messages have been dropped onto a folder with Ctrl
   */
  void folderDropCopy(KMFolder*);

private: // enums used internally
  /**
   * Used in a member variable to store the current copy/cut mode of the clipboard
   */
  enum FolderClipboardMode
  {
    CopyFolders,
    CutFolders
  };

private: // member variables

  QTimer * mUpdateCountsTimer;                      ///< Timer used to trigger a delayed updateCounts()
  QList< QPointer< KMFolder > > mFolderClipboard;   ///< Local clipboard of folders
  FolderClipboardMode mFolderClipboardMode;         ///< Are we cutting or just copying folders ?
  KMMainWidget *mMainWidget;                        ///< The main widget this instance is attacched to
  FolderViewManager *mManager;                      ///< The FolderViewManager we're attacched to
  QString mConfigPrefix;                            ///< Configuration prefix string for this view
  ToolTipDisplayPolicy mToolTipDisplayPolicy;       ///< The currently set tooltip display policy
  SortingPolicy mSortingPolicy;                     ///< The currently set sorting policy
  FolderViewItem *mDnDMoveTargetItem;               ///< Used to paint the custom drop target(s)
  FolderViewItem *mDnDSortReferenceItem;            ///< Used to paint the custom drop target(s)
  DropInsertPosition mDnDSortReferencePosition;     ///< Used to paint the custom drop target(s)
  bool mLastButtonPressedWasMiddle;                 ///< Used to find out the mouse button in the clicked() signal
};




/**
 * An item rappresenting a folder in a FolderView.
 * Usually contains an underlying "physical" KMFolder.
 */
class KMAIL_EXPORT FolderViewItem
  : public QObject,
    public KPIM::FolderTreeWidgetItem
{
  friend class FolderView;

  Q_OBJECT
public:

  /**
   * Construct a root item _with_ folder
   */
  FolderViewItem(
      FolderView *parent,
      const QString & name,
      KMFolder *folder,
      Protocol protocol,
      FolderType type
    );

  /**
   * Construct a child item
   */
  FolderViewItem(
      FolderViewItem *parent,
      const QString & name,
      KMFolder *folder,
      Protocol protocol,
      FolderType type
    );

private:

  KMFolder * mFolder; ///< The "physical" folder this item rappresents. May be NULL
  /**
   * Internal flags that can be set on each item.
   */
  enum Flags
  {
    CountsDirty = 1,                   ///< The counts of this item require an update
    OpenFolderForCountUpdate = 2       ///< When updating counts, open the folder (update cache)
  };

  /**
   * A combination of DirtyFlags
   */
  unsigned int mFlags;

  /**
   * Custom sorting key
   */
  int mDnDSortingKey;

public:
  /**
   * Returns the physical folder that this item is associated to. May be NULL
   */
  KMFolder * folder() const
    { return mFolder; };

  /**
   * Returns the protocol matching the specified folder. fld should not be null.
   */
  static KPIM::FolderTreeWidgetItem::Protocol protocolByFolder( KMFolder *fld );

  /**
   * Returns the type matching the specified folder. fld should not be null.
   */
  static KPIM::FolderTreeWidgetItem::FolderType folderTypeByFolder( KMFolder *fld );

  /**
   * Returns the name of the normal icon for this folder item
   */
  QString normalIcon() const;

  /**
   * Returns the name of the "unread" state icon for this folder item
   */
  QString unreadIcon() const;

  /**
   * Returns the name of the config group suitable for this item.
   * When the item has an associated folder then its configGroupName() is used.
   * When there is no folder then you either get a constant string for
   * well known "virtual" items or an empty string at all.
   * An empty string means that the item shouldn't permanently store the data
   * in the configuration.
   */
  QString configGroupName() const;

  /**
   * Updates the unread, total and size counts by looking in the associated KMFolder.
   * If the item has the OpenFolderForCountUpdate flag then the KMFolder is opened
   * in order to perform the update, otherwise cached values are read.
   * Internally clears the ( CountsDirty | OpenFolderForCountUpdate ) flags.
   * Returns true if some of the counts (children or own) have changed.
   */
  bool updateCounts();

  /**
   * Marks this item as requiring a counts update. If openFolderOnUpdate is true
   * then the next update will attempt to bypass the folder count cache
   * and get the actual values (possibly at the expense of computing time).
   */
  void setCountsDirty( bool openFolderOnUpdate )
    { mFlags |= openFolderOnUpdate ? CountsDirty : ( CountsDirty | OpenFolderForCountUpdate ); };

  /**
   * Returns true if this item is marked for a counts update.
   */
  bool countsDirty() const
    { return mFlags & CountsDirty; };

  /**
   * Calls setUnreadCount( unread ) and eventually updates the item
   * icon to either normalIcon() or unreadIcon().
   */
  void adjustUnreadCount( int newUnreadCount );

  /**
   * Sets the Dnd sorting key: used in drag and drop sorting
   */
  void setDndSortingKey( int customKey )
    { mDnDSortingKey = customKey; };

  /**
   * Returns the current Dnd sorting key.
   */
  int dndSortingKey() const
    { return mDnDSortingKey; };

  /**
   * Operator used for item sorting.
   */
  virtual bool operator < ( const QTreeWidgetItem &other ) const;

Q_SIGNALS:

  /**
   * Our icon changed
   */
  void iconChanged( FolderViewItem * );
  /**
   * Our name changed
   */
  void nameChanged( FolderViewItem * );

public Q_SLOTS:
  /**
   * Shows the dialog for the expiry properties for the KMFolder contained
   * in this item. If folder() is null then this function turns to a no-op.
   */
  void slotShowExpiryProperties();

protected:
  /**
   * Sets the folder associated to this item.
   * To be used only in the derived classes constructor.
   */
  void setFolder( KMFolder * folder )
    { mFolder = folder; };

protected Q_SLOTS:
  /**
   * Called when the underlying KMFolder icon changes.
   */
  void slotIconsChanged();

  /**
   * Called when the underlying KMFolder name changes.
   */
  void slotNameChanged();

  /**
   * Called when the underlying KMFolder 'no content' state changes
   */
  void slotNoContentChanged();

  /**
   * Connected to several signals of the underlying KMFolder.
   * It's called when unread count, total count or size of the folder changes.
   */
  void slotFolderCountsChanged( KMFolder *fld );

};

} // namespace KMail


#endif //!__KMAIL_FOLDERTREEWIDGETBASE_H__
