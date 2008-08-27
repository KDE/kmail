/* -*- mode: C++ -*-
   This file is part of the KDE libraries

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#ifndef __KMFOLDERTREE
#define __KMFOLDERTREE

#include "kmail_export.h"
#include "foldertreebase.h"

#include <klocale.h>

#include <QWidget>
#include <QTimer>
#include <q3header.h>
#include <QPixmap>
#include <QDragLeaveEvent>
#include <QEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QList>
#include <QResizeEvent>
#include <QDragEnterEvent>
#include <QMouseEvent>
#include <Q3DragObject>
#include <QPointer>

class QDropEvent;
class QPixmap;
class QMenu;
class KMenu;
class KMFolder;
class KMFolderDir;
class KMFolderImap;
class KMFolderTree;
class KMAccount;
// duplication from kmcommands.h, to avoid the include
typedef QMap<QAction*,KMFolder*> KMMenuToFolder;
template <typename T> class QPointer;

using KPIM::KFolderTree;
using KPIM::KFolderTreeItem;

class KMAIL_EXPORT KMFolderTreeItem : public QObject, public KFolderTreeItem
{
  Q_OBJECT
public:
  /** Construct a root item _without_ folder */
  KMFolderTreeItem( KFolderTree *parent, const QString & name,
                    KFolderTreeItem::Protocol protocol=KFolderTreeItem::NONE );

  /** Construct a root item _with_ folder */
  KMFolderTreeItem( KFolderTree *parent, const QString & name,
                    KMFolder* folder );

  /** Construct a child item */
  KMFolderTreeItem( KFolderTreeItem* parent, const QString & name,
                    KMFolder* folder );
  virtual ~KMFolderTreeItem();

  QPixmap normalIcon( int size ) const;
  QPixmap unreadIcon( int size ) const;

  void setNeedsRepaint( bool value ) { mNeedsRepaint = value; }
  bool needsRepaint() const { return mNeedsRepaint; }

  /** associated folder */
  KMFolder* folder() const { return mFolder; }
  Q3ListViewItem* parent() const { return KFolderTreeItem::parent(); }

  /** Adjust the unread count from the folder and update the
   * pixmaps accordingly. */
  void adjustUnreadCount( int newUnreadCount );

  /** dnd */
  virtual bool acceptDrag(QDropEvent* e) const;

signals:
  /** Our icon changed */
  void iconChanged( KMFolderTreeItem * );
  /** Our name changed */
  void nameChanged( KMFolderTreeItem * );

public slots:
  void properties();
  void assignShortcut();
  void slotShowExpiryProperties();
  void slotIconsChanged();
  void slotNameChanged();
  void updateCount();

protected:
  void init();
  KMFolder* mFolder;
  /** Returns true when top-level/account icons should be used */
  virtual bool useTopLevelIcon() const { return depth() == 0; }
  /** Returns the icon size. */
  virtual int iconSize() const { return 16; }

private:
  bool mNeedsRepaint;
};

//==========================================================================

class KMFolderTree : public KMail::FolderTreeBase
{
  Q_OBJECT

public:
  explicit KMFolderTree( KMMainWidget *mainWidget, QWidget *parent=0,
			 const char *name=0 );

  /** Save config options */
  void writeConfig();

  /** Get/refresh the folder tree */
  virtual void reload(bool openFolders = false);

  /** Recusively add folders in a folder directory to a listview item. */
  virtual void addDirectory( KMFolderDir *fdir, KMFolderTreeItem* parent );

  /** create a folderlist */
  void createFolderList( QStringList *str,
                         QList<QPointer<KMFolder> > *folders,
                         bool localFolders=true,
                         bool imapFolders=true,
                         bool dimapFolders=true,
                         bool searchFolders=false,
                         bool includeNoContent=true,
                         bool includeNoChildren=true );

  /** Read config options. */
  virtual void readConfig(void);

  /** Remove information about not existing folders from the config file */
  void cleanupConfigFile();

  /** Select the next folder with unread messages */
  void nextUnreadFolder(bool confirm);

  /** Check folder for unread messages (which isn't trash)*/
  bool checkUnreadFolder(KMFolderTreeItem* ftl, bool confirm);

  KMFolder *currentFolder() const;

  QList<QPointer<KMFolder> > selectedFolders();

  enum ColumnMode {unread=15, total=16, foldersize=17};

  /** toggles the unread and total columns on/off */
  void toggleColumn(int column, bool openFolders = false);

  /** Set the checked/unchecked state of the unread and total column
   *  in the popup correctly */
  virtual void updatePopup() const;

  /** Select the folder and make sure it's visible */
  void showFolder( KMFolder* );

  /** Valid actions for the folderToPopup method */
  enum MenuAction {
    CopyMessage,
    MoveMessage,
    CopyFolder,
    MoveFolder
  };

  /** Generate a popup menu that contains all folders that can have content */
  void folderToPopupMenu( MenuAction action, QObject *receiver, KMMenuToFolder *,
      QMenu *menu, Q3ListViewItem *start = 0 );

signals:
  /** The selected folder has changed */
  void folderSelected(KMFolder*);

  /** The selected folder has changed to go to an unread message */
  void folderSelectedUnread( KMFolder * );

  /** unread/total/size column has changed */
  void columnsChanged();

  /** an icon of one of our folders changed */
  void iconChanged( KMFolderTreeItem * );

  /** the name of one of our folders changed */
  void nameChanged( KMFolderTreeItem * );

public slots:
  /** Select the next folder with unread messages */
  void nextUnreadFolder();

  /** Select the previous folder with unread messages */
  void prevUnreadFolder();

  /** Increment current folder */
  void incCurrentFolder();

  /** Decrement current folder */
  void decCurrentFolder();

  /** Select the current folder */
  void selectCurrentFolder();

  /** Executes delayed update of folder tree */
  void delayedUpdate();

  /** Make sure the given account is not selected because it is gone */
  void slotAccountRemoved(KMAccount*);

  /** Select the item and switch to the folder */
  void doFolderSelected(Q3ListViewItem *qlvi, bool keepSelection = false);

  /**
   * Reset current folder and all children
   * If no item is given we take the current one
   * If startListing is true a folder listing is started
   */
  void slotResetFolderList( Q3ListViewItem* item = 0, bool startList = true );

  /** Create a child folder */
  void addChildFolder( KMFolder *folder = 0, QWidget * parent = 0 );

  /** Copies the currently selected folder. */
  void copyFolder();

  /** Cuts the currently selected folder. */
  void cutFolder();

  /** Pastes a previously copied/cutted folder below the currently selected folder. */
  void pasteFolder();

protected slots:
  //  void slotRMB(int, int);
  /** called by the folder-manager when the list of folders changed */
  void doFolderListChanged();

  /** called, when a folder has been deleted */
  void slotFolderRemoved(KMFolder *);

  /** called, when a folder has been moved or copied, successfully or not */
  void slotFolderMoveOrCopyOperationFinished();

  /** Updates the folder tree (delayed), causing a "blink" */
  void refresh();

  /** Open a folder */
  void openFolder();

  /** Expand an IMAP folder */
  void slotFolderExpanded( Q3ListViewItem * item );

  /** Tell the folder to refresh the contents on the next expansion */
  void slotFolderCollapsed( Q3ListViewItem * item );

  /** Check if the new name is valid and confirm the new name */
  void slotRenameFolder( Q3ListViewItem * item, int col, const QString& text);

  /** Update the total and unread columns but delayed */
  void slotUpdateCountsDelayed(KMFolder * folder);
  void slotUpdateCountTimeout();
  void slotUpdateOneCount();

  /** slots for the unread/total/size-popup */
  void slotToggleUnreadColumn();
  void slotToggleTotalColumn();
  void slotToggleSizeColumn();

  void slotContextMenuRequested( Q3ListViewItem *, const QPoint & );

  /** Fires a new-mail-check of the account that is accociated with currentItem */
  void slotCheckMail();

  void slotNewMessageToMailingList();

  /** For RMB move folder */
  virtual void moveSelectedToFolder( QAction* act );
  /** For RMB copy folder */
  virtual void copySelectedToFolder( QAction* act );

  /** Updates copy/cut/paste actions */
  void updateCopyActions();

protected:
  virtual void contentsMousePressEvent( QMouseEvent *e );
  virtual void contentsMouseReleaseEvent(QMouseEvent* me);

  /** Updates the number of unread messages for all folders */
  virtual void updateUnreadAll( );

  virtual void resizeEvent(QResizeEvent*);

  /** Read/Save open/close state indicator for an item in folderTree list view */
  bool readIsListViewItemOpen(KMFolderTreeItem *fti);
  void writeIsListViewItemOpen(KMFolderTreeItem *fti);

  QTimer mUpdateTimer;

  /** Drag and drop methods */
  void contentsDragEnterEvent( QDragEnterEvent *e );
  void contentsDragMoveEvent( QDragMoveEvent *e );
  void contentsDragLeaveEvent( QDragLeaveEvent *e );
  void contentsDropEvent( QDropEvent *e );
  virtual Q3DragObject* dragObject();

  /** Drag and drop variables */
  Q3ListViewItem *oldCurrent, *oldSelected;
  Q3ListViewItem *dropItem;
  QPointer<KMFolderTreeItem> mLastItem;
  QTimer autoopen_timer;

  // filter some rmb-events
  bool eventFilter(QObject*, QEvent*);

  /** open ancestors and ensure item is visible  */
  void prepareItem( KMFolderTreeItem* );

  /** connect all signals */
  void connectSignals();

  /** Move or copy the folder @p source to @p destination. */
  void moveOrCopyFolder( QList<QPointer<KMFolder> > sources, KMFolder* destination, bool move=false );

private slots:
  void slotAddToFavorites();
  void slotUnhideLocalInbox();

private:
  /** total column */
  Q3ListViewItemIterator mUpdateIterator;

  /** popup for unread/total */
  KMenu* mPopup;
  QAction *mUnreadAction,*mTotalAction, *mSizeAction;

  bool mReloading;
  QList<QPointer<KMFolder> > mCopySourceFolders;
  bool mCutFolder;

  QTimer *mUpdateCountTimer;
  QMap<QString,KMFolder*> mFolderToUpdateCount;

  /** Map menu id into a folder */
  KMMenuToFolder mMenuToFolder;
};

#endif
