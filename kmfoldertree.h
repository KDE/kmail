#ifndef __KMFOLDERTREE
#define __KMFOLDERTREE

#include <qwidget.h>
#include "ktablistbox.h"
#include "kmfolder.h"

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

private slots:
 void doFolderSelected(int,int);

protected:
  virtual void resizeEvent(QResizeEvent*);

  KMFolderNodeList mList;
};

#endif

