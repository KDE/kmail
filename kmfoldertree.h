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

  // get/refresh the folder tree
  virtual void reload(void);

signals:
  void folderSelected(KMFolder*);

private slots:
 void doFolderSelected(int,int);

protected:
  virtual void resizeEvent(QResizeEvent*);

  KMFolderNodeList mList;
};

#endif

