#ifndef __KMFOLDERTREE
#define __KMFOLDERTREE

#include <qwidget.h>
#include <qlistview.h>
//#include <ktablistbox.h>
#include "kmfolder.h"

// Fixme! A temporary dependency
#include "kmheaders.h" // For KMHeaderToFolderDrag & KMPaintInfo

class QDropEvent;
class QTimer;
class QPixmap;
class KMFolderTreeItem;

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
  
  // Save config options
  void writeConfig();

  // Get/refresh the folder tree
  virtual void reload(void);

  // Recusively add folders in a folder directory to a listview item.
  virtual void addDirectory( KMFolderDir *fdir, QListViewItem* parent );

  // Find index of given folder. Returns -1 if not found
  virtual QListViewItem* indexOfFolder(const KMFolder*);

  /** Read config options. */
  virtual void readConfig(void);

signals:
  /* The selected folder has changed */
  void folderSelected(KMFolder*);

  /* Messages have been dropped onto a folder */
  void folderDrop(KMFolder*);

protected slots:
  void doFolderSelected(QListViewItem*);

  //  void slotRMB(int, int);
  /** called by the folder-manager when the list of folders changed */
  void doFolderListChanged();

  /** Updates the folder tree only if some folder lable has changed */
  void refresh(KMFolder*);

  /** Executes delayed update of folder tree */
  void delayedUpdate();

  /* Create a child folder */
  void addChildFolder();

  /* Open a folder */
  void openFolder();

protected:
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

  //Drag and drop variables
  QListViewItem *oldCurrent;
  QListViewItem *dropItem;
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
};

#endif
