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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#ifndef __KMFOLDERTREE
#define __KMFOLDERTREE

#include "kmheaders.h"
#include "kmfolder.h"

#include <klocale.h>
#include <kpopupmenu.h>
#include <kfoldertree.h>
#include <kiconloader.h>
#include <kdepimmacros.h>

#include <qwidget.h>
#include <qtimer.h>
#include <qheader.h>

class QDropEvent;
class QPixmap;
class QPainter;
class KMFolderImap;
class KMFolderTree;
class KMMainWidget;
class KMAccount;
template <typename T> class QGuardedPtr;

class KDE_EXPORT KMFolderTreeItem : public QObject, public KFolderTreeItem
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

  QPixmap normalIcon(int size=16) const;
  QPixmap unreadIcon(int size=16) const;

  void setNeedsRepaint( bool value ) { mNeedsRepaint = value; }
  bool needsRepaint() const { return mNeedsRepaint; }

  /** associated folder */
  KMFolder* folder() const { return mFolder; }
  QListViewItem* parent() const { return KFolderTreeItem::parent(); }

  /** Adjust the unread count from the folder and update the
   * pixmaps accordingly. */
  void adjustUnreadCount( int newUnreadCount );

  /** dnd */
  virtual bool acceptDrag(QDropEvent* ) const;

signals:
  /** Our icon changed */
  void iconChanged( KMFolderTreeItem * );
  /** Our name changed */
  void nameChanged( KMFolderTreeItem * );

public slots:
  void properties();
  void assignShortcut();
  void slotRepaint();
  void slotShowExpiryProperties();
  void slotNameChanged() { emit nameChanged( this ); }

protected:
  void init();
  KMFolder* mFolder;
private:
  bool mNeedsRepaint;
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
  virtual QListViewItem* indexOfFolder( const KMFolder* folder ) const
  {
     if ( mFolderToItem.contains( folder ) )
       return mFolderToItem[ folder ];
     else
       return 0;
  }
  
  /** create a folderlist */
  void createFolderList( QStringList *str,
                         QValueList<QGuardedPtr<KMFolder> > *folders,
                         bool localFolders=true,
                         bool imapFolders=true,
                         bool dimapFolders=true,
                         bool searchFolders=false,
                         bool includeNoContent=true,
                         bool includeNoChildren=true );

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

  /** Set the checked/unchecked state of the unread and total column
   *  in the popup correctly */
  virtual void updatePopup() const;

  /** Returns the main widget that this widget is a child of. */
  KMMainWidget * mainWidget() const { return mMainWidget; }

  /** Select the folder and make sure it's visible */
  void showFolder( KMFolder* );

  void insertIntoFolderToItemMap( const KMFolder *folder, KMFolderTreeItem* item )
  {
    mFolderToItem.insert( folder, item );
  }

  void removeFromFolderToItemMap( const KMFolder *folder )
  {
    mFolderToItem.remove( folder );
  }

  /** Generate a popup menu that contains all folders that can have content */
  void folderToPopupMenu( bool move, QObject *receiver, KMMenuToFolder *, 
      QPopupMenu *menu, QListViewItem *start = 0 );
  
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
  void doFolderSelected(QListViewItem*);

  /**
   * Reset current folder and all childs
   * If no item is given we take the current one
   * If startListing is true a folder listing is started
   */
  void slotResetFolderList( QListViewItem* item = 0, bool startList = true );

protected slots:
  //  void slotRMB(int, int);
  /** called by the folder-manager when the list of folders changed */
  void doFolderListChanged();

  /** called, when a folder has been deleted */
  void slotFolderRemoved(KMFolder *);

  /** Updates the folder tree (delayed), causing a "blink" */
  void refresh();

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

  void slotContextMenuRequested( QListViewItem *, const QPoint & );

  /** Fires a new-mail-check of the account that is accociated with currentItem */
  void slotCheckMail();

  void slotNewMessageToMailingList();

protected:
  /** Catch palette changes */
  virtual bool event(QEvent *e);

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
  /** total column */
  QListViewItemIterator mUpdateIterator;

  /** popup for unread/total */
  KPopupMenu* mPopup;
  int mUnreadPop;
  int mTotalPop;

  /** show popup after D'n'D? */
  bool mShowPopupAfterDnD;
  KMMainWidget *mMainWidget;
  bool mReloading;
  QMap<const KMFolder*, KMFolderTreeItem*> mFolderToItem;
};

#endif
