/* This file is part of the KDE libraries

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#ifndef __KMFOLDERTREE
#define __KMFOLDERTREE

#include <qwidget.h>
#include <qtimer.h>
#include <qheader.h>

#include <klocale.h>
#include <kpopupmenu.h>
#include <kfoldertree.h>

#include "kmheaders.h"
#include "kmfolder.h"

class QDropEvent;
class QPixmap;
class QPainter;
class KMFolderImap;
class KMFolderTree;
class KMMainWidget;

class KMFolderTreeItem : public QObject, public KFolderTreeItem

{
  Q_OBJECT
public:
  /** Construct a root item _without_ folder */
  KMFolderTreeItem( KFolderTree *parent, QString name );

  /** Construct a root item _with_ folder */
  KMFolderTreeItem( KFolderTree *parent, QString name,
                    KMFolder* folder );

  /** Construct a child item */
  KMFolderTreeItem( KFolderTreeItem* parent, QString name,
                    KMFolder* folder );
  virtual ~KMFolderTreeItem();

  QPixmap* normalIcon() const
  { if ( mFolder && mFolder->useCustomIcons() ) return mNormalIcon; else return 0; }
  QPixmap* unreadIcon() const
  { if ( mFolder && mFolder->useCustomIcons() ) return mUnreadIcon; else return 0; }

  /** associated folder */
  KMFolder* folder() { return mFolder; }
  QListViewItem* parent() { return KFolderTreeItem::parent(); }

  /** dnd */
  virtual bool acceptDrag(QDropEvent* ) const;
public slots:
  void properties();
  void slotRepaint() { repaint(); }

protected:
  void init();
  void iconsFromPaths();
  KMFolder* mFolder;
private:
  /** Custom pixmaps to display in the tree, none by default */
  QPixmap *mNormalIcon;
  QPixmap *mUnreadIcon;
};

//==========================================================================

class KMFolderTree : public KFolderTree
{
  Q_OBJECT

public:
  KMFolderTree( KMMainWidget *mainWidget, QWidget *parent=0,
		const char *name=0 );

  /** Save config options */
  void writeConfig();

  /** Get/refresh the folder tree */
  virtual void reload(bool openFolders = false);

  /** Recusively add folders in a folder directory to a listview item. */
  virtual void addDirectory( KMFolderDir *fdir, KMFolderTreeItem* parent );

  /** Find index of given folder. Returns 0 if not found */
  virtual QListViewItem* indexOfFolder(const KMFolder*);

  /** Create a list of all folders */
  virtual void createFolderList(QStringList * str,
    QValueList<QGuardedPtr<KMFolder> > * folders);

  /** Create a list of all IMAP folders of a given account */
  void createImapFolderList(KMFolderImap *folder, QStringList *names,
    QStringList *urls, QStringList *mimeTypes);

  /** Read config options. */
  virtual void readConfig(void);

  /** Read color options and set palette. */
  void readColorConfig(void);

  /** Remove information about not existing folders from the config file */
  void cleanupConfigFile();

  /** Select the next folder with unread messages */
  void nextUnreadFolder(bool confirm);

  /** Check folder for unread messages (which isn't trash)*/
  bool checkUnreadFolder(KMFolderTreeItem* ftl, bool confirm);

  KMFolder *currentFolder() const;

  enum ColumnMode {unread=15, total=16};

  /** toggles the unread and total columns on/off */
  void toggleColumn(int column, bool openFolders = false);

signals:
  /** The selected folder has changed */
  void folderSelected(KMFolder*);

  /** The selected folder has changed to go to an unread message */
  void folderSelectedUnread( KMFolder * );

  /** Messages have been dropped onto a folder */
  void folderDrop(KMFolder*);

  /** Messages have been dropped onto a folder with Ctrl */
  void folderDropCopy(KMFolder*);

  /** unread/total column has changed */
  void columnsChanged();

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

  /** Remove all items associated with the given IMAP account */
  void slotAccountDeleted(KMFolderImap*);

  /** Select the item and switch to the folder */
  void doFolderSelected(QListViewItem*);

  /** autoscroll support */
  void startAutoScroll();
  void stopAutoScroll();

protected slots:
  //  void slotRMB(int, int);
  /** called by the folder-manager when the list of folders changed */
  void doFolderListChanged();

  /** called, when a folder has been deleted */
  void slotFolderRemoved(KMFolder *);

  /** Updates the folder tree (delayed), causing a "blink" */
  void refresh();

  /** Updates the folder tree only if some folder lable has changed */
  void refresh(KMFolder* folder, bool doUpdate);

  /** Create a child folder */
  void addChildFolder();

  /** Open a folder */
  void openFolder();

  /** Expand an IMAP folder */
  void slotFolderExpanded( QListViewItem * item );

  /** Tell the folder to refresh the contents on the next expansion */
  void slotFolderCollapsed( QListViewItem * item );

  /** Check if the new name is valid and confirm the new name */
  void slotRenameFolder( QListViewItem * item, int col, const QString& text);

  /** Update the total and unread columns (if available) */
  void slotUpdateCounts(KMFolder * folder);
  void slotUpdateCounts(KMFolderImap * folder, bool success = true);
  void slotUpdateOneCount();

  /** slots for the unread/total-popup */
  void slotToggleUnreadColumn();
  void slotToggleTotalColumn();

  void autoScroll();

  /** right and middle mouse button */
  void rightButtonPressed( QListViewItem *, const QPoint &, int);
  void mouseButtonPressed( int btn, QListViewItem *, const QPoint &, int);

  /** Fires a new-mail-check of the account that is accociated with currentItem */
  void slotCheckMail();

protected:
  /** Catch palette changes */
  virtual bool event(QEvent *e);

  virtual void paintEmptyArea( QPainter * p, const QRect & rect );

  /** Updates the number of unread messages for all folders */
  virtual void updateUnreadAll( );

  virtual void resizeEvent(QResizeEvent*);

  /** Read/Save open/close state indicator for an item in folderTree list view */
  bool readIsListViewItemOpen(KMFolderTreeItem *fti);
  void writeIsListViewItemOpen(KMFolderTreeItem *fti);

  QTimer mUpdateTimer;

  /** We need out own root, otherwise the @ref QListView will create
      its own root of type @ref QListViewItem, hence no overriding
      paintBranches and no backing pixmap */
  KMFolderTreeItem *root;

  /** Drag and drop methods */
  void contentsDragEnterEvent( QDragEnterEvent *e );
  void contentsDragMoveEvent( QDragMoveEvent *e );
  void contentsDragLeaveEvent( QDragLeaveEvent *e );
  void contentsDropEvent( QDropEvent *e );

  /** Drag and drop variables */
  QListViewItem *oldCurrent, *oldSelected;
  QListViewItem *dropItem;
  KMFolderTreeItem *mLastItem;
  QTimer autoopen_timer;

  // filter some rmb-events
  bool eventFilter(QObject*, QEvent*);

  /** open ancestors and ensure item is visible  */
  void prepareItem( KMFolderTreeItem* );

  /** connect all signals */
  void connectSignals();

private:
  QTimer autoscroll_timer;
  int autoscroll_time;
  int autoscroll_accel;

  /** total column */
  QListViewItemIterator mUpdateIterator;

  /** popup for unread/total */
  KPopupMenu* mPopup;
  int mUnreadPop;
  int mTotalPop;

  /** show popup after D'n'D? */
  bool mShowPopupAfterDnD;
  KMMainWidget *mMainWidget;
};

#endif
