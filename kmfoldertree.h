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
class CryptPlugWrapperList;

class KMFolderTreeItem : public KFolderTreeItem
{
public:
  /** Construct a root item _without_ folder */
  KMFolderTreeItem( KFolderTree *parent,
                    QString name )
    : KFolderTreeItem( parent, name ), mFolder( 0 )
    {}

  /** Construct a root item _with_ folder */    
  KMFolderTreeItem( KFolderTree *parent, QString name,
                    KMFolder* folder )
    : KFolderTreeItem( parent, name, folder->protocol() ), mFolder( folder )
    {}
 
  /** Construct a child item */
  KMFolderTreeItem( KFolderTreeItem* parent, QString name,
                    KMFolder* folder )
    : KFolderTreeItem( parent, folder->protocol(), name ), mFolder( folder )
    {}

  /** gets the recursive unread-count */
  virtual int countUnreadRecursive();

  /** associated folder */
  KMFolder* folder() { return mFolder; }

  /** dnd */
  virtual bool acceptDrag(QDropEvent* ) const;

protected:
  KMFolder* mFolder;
};

//==========================================================================

class KMFolderTree : public KFolderTree
{
  Q_OBJECT

public:
  KMFolderTree( CryptPlugWrapperList * cryptPlugList,
                QWidget *parent=0, const char *name=0 );

  /** Save config options */
  void writeConfig();

  /** Get/refresh the folder tree */
  virtual void reload(bool openFolders = false);

  /** Recusively add folders in a folder directory to a listview item. */
  virtual void addDirectory( KMFolderDir *fdir, KMFolderTreeItem* parent );

  /** Find index of given folder. Returns -1 if not found */
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

  /** Updates the folder tree only if some folder lable has changed */
  void refresh(KMFolder*);

  /** Create a child folder */
  void addChildFolder();

  /** Open a folder */
  void openFolder();

  /** Expand an IMAP folder */
  void slotFolderExpanded( QListViewItem * item );

  /** Tell the folder to refresh the contents on the next expansion */
  void slotFolderCollapsed( QListViewItem * item );

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
  static QPixmap *pixDir, *pixNode, *pixPlain, *pixFld, *pixFull, *pixIn,
    *pixOut, *pixTr, *pixSent;

  /** We need out own root, otherwise the @ref QListView will create
      its own root of type @ref QListViewItem, hence no overriding
      paintBranches and no backing pixmap */
  KMFolderTreeItem *root;

  /** Drag and drop methods */
  void contentsDragEnterEvent( QDragEnterEvent *e );
  void contentsDragMoveEvent( QDragMoveEvent *e );
  void contentsDragLeaveEvent( QDragLeaveEvent *e );
  void contentsDropEvent( QDropEvent *e );

  /** Navigation/Selection methods */
  virtual void keyPressEvent( QKeyEvent * e );
  virtual void contentsMousePressEvent( QMouseEvent * e );
  virtual void contentsMouseReleaseEvent( QMouseEvent * e );
  virtual void contentsMouseMoveEvent( QMouseEvent* e );

  /** Drag and drop variables */
  QListViewItem *oldCurrent, *oldSelected;
  QListViewItem *dropItem;
  KMFolderTreeItem *mLastItem;
  QTimer autoopen_timer;

  // filter some rmb-events
  bool eventFilter(QObject*, QEvent*);

  virtual void drawContentsOffset( QPainter * p, int ox, int oy,
				   int cx, int cy, int cw, int ch );

  /** open ancestors and ensure item is visible  */
  void prepareItem( KMFolderTreeItem* );

  /** connect all signals */
  void connectSignals();

private:
  QTimer autoscroll_timer;
  int autoscroll_time;
  int autoscroll_accel;
  CryptPlugWrapperList * mCryptPlugList;

  /** total column */
  QListViewItemIterator mUpdateIterator;

  /** popup for unread/total */
  KPopupMenu* mPopup;
  int mUnreadPop;
  int mTotalPop;

  /** show popup after D'n'D? */
  bool mShowPopupAfterDnD;
};

#endif
