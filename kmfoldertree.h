#ifndef __KMFOLDERTREE
#define __KMFOLDERTREE

#include <qwidget.h>
#include <qlistview.h>

class QDropEvent;
class QTimer;
class KMFolderTreeItem;
class KMFolder;
class QPoint;

#define KMFolderTreeInherited QListView
class KMFolderTree : public QListView
{
  Q_OBJECT
public:
  KMFolderTree(QWidget *parent=0, const char *name=0);
  virtual ~KMFolderTree();

  // Get/refresh the folder tree
  virtual void reload(void);

  /** Set current (selected) folder-node */
  virtual void setCurrentFolder(const KMFolder*);

  /** Returns folder-node of current item or NULL if none. */
  virtual KMFolder* currentFolder() const;

  /** Search for folder-node in list-view */
  virtual KMFolderTreeItem* findItem(const KMFolder*) const;

signals:
  void folderSelected(KMFolder*);

protected slots:
  void doClicked(QListViewItem*);

  /** called when right mouse button is pressed */
  void slotRightButtonPressed(QListViewItem*, const QPoint&, int);

  /** called by the folder-manager when the list of folders changed */
  void doFolderListChanged();

  /** called when a drop occurs. */
  void dropEvent(QDropEvent*);

  /** Updates the folder tree only if some folder label has changed */
  void refresh(KMFolder*);

  /** Executes delayed update of folder tree */
  void delayedUpdate();

protected:
  // Updates the number of unread messages for all folders
  virtual void updateUnreadAll( );

  virtual void resizeEvent(QResizeEvent*);

  /** Returns pixmap for given folder type */
  virtual QPixmap folderTypePixmap(const QString folderType);

  /** Search for folder-node in list-view -- recursive worker method. */
  virtual KMFolderTreeItem* findItemRecursive(const KMFolder*,
					      KMFolderTreeItem*) const;

  /** Append a new item to the list. Position in the list is searched
      according to the path given. If needed, directory nodes are
      created during the process. */
  virtual KMFolderTreeItem* appendItem(const QCString& path,
				       KMFolderTreeItem* aRoot,
				       bool bold=FALSE, char sepChar='/');

  /** Read config file and set geometry from it */
  virtual void writeConfig();

  /** Write geometry to config file */
  virtual void readConfig();

  void delayedUpdateRecursive(KMFolderTreeItem*);

protected:
  QTimer* mUpdateTimer;
};


//-----------------------------------------------------------------------------
class KMFolderTreeItem: public QListViewItem
{
public:
  KMFolderTreeItem(KMFolderTreeItem* i, const QString& str);
  KMFolderTreeItem(KMFolderTree* t, const QString& str);

  // Get/set folder node pointer
  KMFolder* folder(void) const { return mFolder; }
  void setFolder(KMFolder* f) { mFolder=f; }

protected:
  KMFolder* mFolder;
};


#endif
