#include <stdlib.h>
#include <qfileinf.h>
#include <qpixmap.h>
#include <kapp.h>
#include "util.h"
#include "kmglobal.h"
#include "kmfoldermgr.h"
#include "kmfolderdir.h"
#include "kmacctfolder.h"


#include "kmfoldertree.moc"

//-----------------------------------------------------------------------------
KMFolderTree::KMFolderTree(QWidget *parent,const char *name) : 
  KMFolderTreeInherited(parent, name, 1)
{
  KConfig* conf = app->getConfig();
  int width;

  initMetaObject();

  conf->setGroup("Geometry");
  width = conf->readNumEntry(name, 80);
  resize(width, size().height());

  connect(this, SIGNAL(highlighted(int,int)),
	  this, SLOT(doFolderSelected(int,int)));

  clearTableFlags();
  setTableFlags (Tbl_smoothVScrolling | Tbl_autoVScrollBar);

  setColumn(0, "Folders", 400, KTabListBox::MixedColumn);

  dict().insert("dir", new QPixmap("pics/kmdirclosed.xpm"));
  dict().insert("fld", new QPixmap("pics/flag.xpm"));
  dict().insert("in", new QPixmap("pics/bottom.xpm"));

  setAutoUpdate(TRUE);
  reload();
}


//-----------------------------------------------------------------------------
KMFolderTree::~KMFolderTree()
{
  KConfig* conf = app->getConfig();

  conf->setGroup("Geometry");
  conf->writeEntry(name(), size().width());
}


//-----------------------------------------------------------------------------
void KMFolderTree::reload(void)
{
  KMFolderDir* fdir;
  KMAcctFolder* folder;
  QString str;
  QString indent = "";
  bool upd = autoUpdate();

  setAutoUpdate(FALSE);

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
  setAutoUpdate(upd);
  if (upd) repaint();
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


//-----------------------------------------------------------------------------
void KMFolderTree::resizeEvent(QResizeEvent* e)
{
  KConfig* conf = app->getConfig();

  conf->setGroup("Geometry");
  conf->writeEntry(name(), size().width());

  KMFolderTreeInherited::resizeEvent(e);
}
