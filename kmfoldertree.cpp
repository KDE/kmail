#include <stdlib.h>
#include <qfileinf.h>
#include <qpixmap.h>

#include "util.h"
#include "kmglobal.h"
#include "kmfoldermgr.h"
#include "kmfolderdir.h"
#include "kmacctfolder.h"


#include "kmfoldertree.moc"

//-----------------------------------------------------------------------------
KMFolderTree::KMFolderTree(QWidget *parent=0,const char *name=0) : 
  KMFolderTreeInherited(parent, name, 1)
{
  connect(this, SIGNAL(highlighted(int,int)),
	  this, SLOT(doFolderSelected(int,int)));

  clearTableFlags();
  setTableFlags (Tbl_smoothVScrolling | Tbl_clipCellPainting);

  setColumn(0, "Folders", 80, KTabListBox::MixedColumn);

  dict().insert("dir", new QPixmap("pics/kmdirclosed.xpm"));
  dict().insert("fld", new QPixmap("pics/flag.xpm"));
  dict().insert("in", new QPixmap("pics/bottom.xpm"));

  reload();
}


//-----------------------------------------------------------------------------
void KMFolderTree::reload(void)
{
  KMFolderDir* fdir;
  KMAcctFolder* folder;
  QString str;
  QString indent = "";

  clear();
  mList.clear();

  fdir = &folderMgr->dir();

  for (folder = (KMAcctFolder*)fdir->first(); 
       folder != NULL;
       folder = (KMAcctFolder*)fdir->next())
  {
    if (folder->isDir()) str = indent+"{dir} "+folder->name();
    else if (folder->account()) str = indent+"{in} "+folder->name();
    else str = indent+"{fld} "+folder->name();
    insertItem(str);

    mList.append(folder);
  }
}

//-----------------------------------------------------------------------------
void KMFolderTree::doFolderSelected(int index, int)
{
  KMFolder* folder;

  printf("KMFolderTree::doFolderSelected(%d)\n", index);

  if (index < 0) return;

  folder = (KMFolder*)mList.at(index);
  if (!folder->isDir()) emit folderSelected(folder);
}
