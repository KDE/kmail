#ifndef __KMFOLDERTREE
#define __KMFOLDERTREE

#include <qwidget.h>
#include <qlistview.h>
#include "kmfolder.h"

class QDropEvent;
class QTimer;
class KMFolderTreeItem;

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
  virtual void setCurrentFolderNode(const KMFolderNode*);

  /** Returns folder-node of current item or NULL if none. */
  virtual KMFolderNode* currentFolderNode() const;

  /** Search for folder-node in list-view */
  virtual KMFolderTreeItem* findItem(const KMFolderNode*) const;

signals:
  void folderSelected(KMFolder*);

protected slots:
  void doClicked(QListViewItem*);

  void slotRMB(int, int);
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

  // Insert folder sorted by type and name
  virtual void inSort(KMFolder*);

  virtual void resizeEvent(QResizeEvent*);

  virtual KMFolderTreeItem* appendFolderNode(KMFolderNode* node, KMFolderTreeItem* parent=0);

  /** Returns pixmap for given folder type */
  virtual QPixmap folderTypePixmap(const QString folderType);

  /** Search for folder-node in list-view -- recursive worker method. */
  virtual KMFolderTreeItem* findItemRecursive(const KMFolderNode*,
					      KMFolderTreeItem*) const;
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
  KMFolderNode* node(void) const { return mNode; }
  void setNode(KMFolderNode* n) { mNode=n; }

  // Returns type of folder node or NULL if no folder node is set
  const char* type(void) const { return (mNode ? mNode->type() : 0); }

protected:
  KMFolderNode* mNode;
};


#endif
