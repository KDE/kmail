#ifndef __KMFOLDERTREE
#define __KMFOLDERTREE

#include <qwidget.h>
#include <ktablistbox.h>
#include "kmfolder.h"

class KDNDDropZone;

#define KMFolderTreeInherited KTabListBox
class KMFolderTree : public KTabListBox
{
  Q_OBJECT
public:
  KMFolderTree(QWidget *parent=0, const char *name=0);
  virtual ~KMFolderTree();

  // Get/refresh the folder tree
  virtual void reload(void);

  // Find index of given folder. Returns -1 if not found
  virtual int indexOfFolder(const KMFolder*) const;

signals:
  void folderSelected(KMFolder*);

protected slots:
  void doFolderSelected(int,int);

  /** called by the folder-manager when the list of folders changed */
  void doFolderListChanged();

  /** called when a drop occurs. */
  void doDropAction(KDNDDropZone*);

protected:
  // Insert folder sorted by type and name
  virtual void inSort(KMFolder*);

  virtual void resizeEvent(QResizeEvent*);

  KMFolderNodeList mList;
  KDNDDropZone* mDropZone;
};

#endif

