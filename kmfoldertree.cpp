#include <stdlib.h>
#include <qfileinf.h>
#include <qpixmap.h>
#include <kapp.h>
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
  QString  kdir = app->kdedir();
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

  dict().insert("dir", new QPixmap(kdir+"/lib/pics/closed.xpm"));
  dict().insert("node", new QPixmap(kdir+"/lib/pics/green-bullet.xpm"));
  dict().insert("plain", new QPixmap(kdir+"/lib/pics/kmfolder.xpm"));
  dict().insert("fld", new QPixmap(kdir+"/lib/pics/kmfolder.xpm"));
  dict().insert("in", new QPixmap(kdir+"/lib/pics/kmfldin.xpm"));
  dict().insert("out", new QPixmap(kdir+"/lib/pics/kmfldout.xpm"));
  dict().insert("tr", new QPixmap(kdir+"/lib/pics/kmtrash.xpm"));

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
    str = indent.copy();

    if (folder->isDir()) str += "{dir} ";
    else if (folder->account()) str += "{in} ";
    else 
    {
      if (folder->name()=="trash") str += "{tr} ";
      else if (folder->name()=="outbox") str += "{out} ";
      else str += "{fld} ";
    }
    str += folder->name();
    str.detach();
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
