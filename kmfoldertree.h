#ifndef __KMFOLDERTREE
#define __KMFOLDERTREE

#include <qwidget.h>
#include <qlistview.h>
#include <qtimer.h>
#include <klocale.h>
#include "kmfolder.h"

// Fixme! A temporary dependency
#include "kmheaders.h" // For KMHeaderToFolderDrag & KMPaintInfo

class QDropEvent;
class QPixmap;

class KMFolderTreeItem : public QListViewItem
{
 
public:
  KMFolder* folder;
  QString unread;
  KMPaintInfo *mPaintInfo;
  enum imapState { imapNoInformation=0, imapInProgress=1, imapFinished=2 };
  imapState mImapState;
 
  /* Construct the root item */
  KMFolderTreeItem( QListView *parent,
                    KMPaintInfo *aPaintInfo )
    : QListViewItem( parent, i18n("Mail") ),
      folder( 0 ),
      unread( QString::null ),
      mPaintInfo( aPaintInfo ),
      mImapState( imapNoInformation )
    {}
  KMFolderTreeItem( QListView *parent,
                    KMFolder* folder,
                    KMPaintInfo *aPaintInfo )
    : QListViewItem( parent, i18n("Mail") ),
      folder( folder ),
      unread( QString::null ),
      mPaintInfo( aPaintInfo ),
      mImapState( imapNoInformation )
    {}
 
  /* Construct a child item */
  KMFolderTreeItem( QListViewItem* parent,
                    KMFolder* folder,
                    KMPaintInfo *aPaintInfo )
    : QListViewItem( parent, (folder) ? folder->label() : QString::null ),
      folder( folder ),
      unread( QString::null ),
      mPaintInfo( aPaintInfo ),
      mImapState( imapNoInformation )
    {}
 
  virtual ~KMFolderTreeItem();
  void paintBranches( QPainter * p, const QColorGroup & cg,
                      int w, int y, int h, GUIStyle s )
  {
    QListViewItem::paintBranches( p, cg, w, y, h, s);
  }

  void paintCell( QPainter * p, const QColorGroup & cg,
                  int column, int width, int align ); 
  virtual QString key( int, bool ) const;
};


#define KMFolderTreeInherited QListView
class KMFolderTree : public QListView
{
  Q_OBJECT

protected:
  virtual void drawContentsOffset( QPainter * p, int ox, int oy,
				   int cx, int cy, int cw, int ch );

public:
  KMFolderTree(QWidget *parent=0, const char *name=0);
  virtual ~KMFolderTree();

  /** Save config options */
  void writeConfig();

  /** Get/refresh the folder tree */
  virtual void reload(void);

  /** Recusively add folders in a folder directory to a listview item. */
  virtual void addDirectory( KMFolderDir *fdir, QListViewItem* parent );

  /** Add an item for an Imap foler */
  void addImapChildFolder(KMFolderTreeItem *item, const QString& name,
    const QString& url, const QString& mimeType, bool noPrefix);

  /** Find index of given folder. Returns -1 if not found */
  virtual QListViewItem* indexOfFolder(const KMFolder*);

  /** Create a list of all folders */
  virtual void createFolderList(QStringList * str,
    QValueList<QGuardedPtr<KMFolder> > * folders);

  /** Read config options. */
  virtual void readConfig(void);

  /** Read color options and set palette. */
  void readColorConfig(void);

  /** Ensure that there is only one selected item */
  virtual void setSelected( QListViewItem *, bool );

  /** Remove information about not existing folders from the config file */
  void cleanupConfigFile();

signals:
  /* The selected folder has changed */
  void folderSelected(KMFolder*);

  /* Messages have been dropped onto a folder */
  void folderDrop(KMFolder*);

  /* Messages have been dropped onto a folder with Ctrl */
  void folderDropCopy(KMFolder*);

protected:
  /* open ancestors and ensure item is visible  */
  void prepareItem( KMFolderTreeItem* );

public slots:
  /* Select the next folder with unread messages */
  void nextUnreadFolder();
  /* Select the previous folder with unread messages */
  void prevUnreadFolder();
  /* Increment current folder */
  void incCurrentFolder();
  /* Decrement current folder */
  void decCurrentFolder();
  /* Select the current folder */
  void selectCurrentFolder();
  /** Executes delayed update of folder tree */
  void delayedUpdate();

protected slots:
  void doFolderSelected(QListViewItem*);

  //  void slotRMB(int, int);
  /** called by the folder-manager when the list of folders changed */
  void doFolderListChanged();

  /** Updates the folder tree only if some folder lable has changed */
  void refresh(KMFolder*);

  /* Create a child folder */
  void addChildFolder();

  /* Open a folder */
  void openFolder();

  /* Expand an IMAP folder */
  void slotFolderExpanded( QListViewItem * item );

  /* Delete all child items on collapse */
  void slotFolderCollapsed( QListViewItem * item );

protected:
  // Catch palette changes
  virtual bool event(QEvent *e);

  virtual void paintEmptyArea( QPainter * p, const QRect & rect );

  // Updates the number of unread messages for all folders
  virtual void updateUnreadAll( );

  virtual void resizeEvent(QResizeEvent*);

  // Read/Save open/close state indicator for an item in folderTree list view
  bool readIsListViewItemOpen(KMFolderTreeItem *fti);
  void writeIsListViewItemOpen(KMFolderTreeItem *fti);

  KMFolderNodeList mList;
  QTimer* mUpdateTimer;
  static QPixmap *pixDir, *pixNode, *pixPlain, *pixFld, *pixFull, *pixIn,
    *pixOut, *pixTr, *pixSent;

  // We need out own root, otherwise the QListView will create its own
  // root of type QListViewItem, hence no overriding paintBranches
  // and no backing pixmap
  QListViewItem *root;

  //Drag and drop methods
  void contentsDragEnterEvent( QDragEnterEvent *e );
  void contentsDragMoveEvent( QDragMoveEvent *e );
  void contentsDragLeaveEvent( QDragLeaveEvent *e );
  void contentsDropEvent( QDropEvent *e );

  // Navigation/Selection methods
  virtual void keyPressEvent( QKeyEvent * e );
  virtual void contentsMousePressEvent( QMouseEvent * e );
  virtual void contentsMouseReleaseEvent( QMouseEvent * e );
  virtual void contentsMouseMoveEvent( QMouseEvent* e );
  //Drag and drop variables
  QListViewItem *oldCurrent, *oldSelected;
  QListViewItem *dropItem;
  KMFolderTreeItem *mLastItem;
  QTimer autoopen_timer;
  KMPaintInfo mPaintInfo;

  // ########### The Trolls may move this Drag and drop stuff to QScrollView
private:
    QTimer autoscroll_timer;
    int autoscroll_time;
    int autoscroll_accel;
public slots:
    void startAutoScroll();
    void stopAutoScroll();
protected slots:
    void autoScroll();
    void rightButtonPressed( QListViewItem *, const QPoint &, int);
    void mouseButtonPressed( int btn, QListViewItem *, const QPoint &, int);
};

#endif
