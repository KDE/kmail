// kmfoldertree.cpp

#include <stdlib.h>
#include <qfileinf.h>
#include <qpixmap.h>
#include <kapp.h>
#include <kiconloader.h>

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
  KIconLoader* loader = app->getIconLoader();
  static QPixmap pixDir, pixNode, pixPlain, pixFld, pixIn, pixOut, pixTr;
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

  pixDir   = loader->loadIcon("closed.xpm");
  pixNode  = loader->loadIcon("green-bullet.xp");
  pixPlain = loader->loadIcon("kmfolder.xpm");
  pixFld   = loader->loadIcon("kmfolder.xpm");
  pixIn    = loader->loadIcon("kmfldin.xpm");
  pixOut   = loader->loadIcon("kmfldout.xpm");
  pixTr    = loader->loadIcon("kmtrash.xpm");

  dict().insert("dir", &pixDir);
  dict().insert("node", &pixNode);
  dict().insert("plain", &pixPlain);
  dict().insert("fld", &pixFld);
  dict().insert("in", &pixIn);
  dict().insert("out", &pixOut);
  dict().insert("tr", &pixTr);

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
