// kmfoldertree.cpp

#include <stdlib.h>
#include <qfileinfo.h>
#include <qpixmap.h>
#include <kapp.h>
#include <kconfig.h>
#include <kiconloader.h>

#include "kmglobal.h"
#include "kmdragdata.h"
#include "kmfoldermgr.h"
#include "kmfolderdir.h"
#include "kmfolder.h"
#include <drag.h>
#include <klocale.h>

#include "kmfoldertree.moc"

//-----------------------------------------------------------------------------
KMFolderTree::KMFolderTree(QWidget *parent,const char *name) : 
  KMFolderTreeInherited(parent, name, 1), mList()
{
  KConfig* conf = app->getConfig();
  KIconLoader* loader = app->getIconLoader();
  static QPixmap pixDir, pixNode, pixPlain, pixFld, pixIn, pixOut, pixTr,
		 pixSent;
  static bool pixmapsLoaded = FALSE;
  int width;

  initMetaObject();

  mDropZone = new KDNDDropZone(this, DndRawData);
  connect(mDropZone, SIGNAL(dropAction(KDNDDropZone*)),
	  this, SLOT(doDropAction(KDNDDropZone*)));

  connect(this, SIGNAL(highlighted(int,int)),
	  this, SLOT(doFolderSelected(int,int)));
  connect(folderMgr, SIGNAL(changed()),
	  this, SLOT(doFolderListChanged()));

  conf->setGroup("Geometry");
  width = conf->readNumEntry(name, 80);
  resize(width, size().height());

  clearTableFlags();
  setTableFlags (Tbl_smoothVScrolling | Tbl_autoVScrollBar);

  setColumn(0, i18n("Folders"), 400, KTabListBox::MixedColumn);

  if (!pixmapsLoaded)
  {
    pixmapsLoaded = TRUE;

    pixDir   = loader->loadIcon("closed.xpm");
    pixNode  = loader->loadIcon("green-bullet.xpm");
    pixPlain = loader->loadIcon("kmfolder.xpm");
    pixFld   = loader->loadIcon("kmfolder.xpm");
    pixIn    = loader->loadIcon("kmfldin.xpm");
    pixOut   = loader->loadIcon("kmfldout.xpm");
    pixSent  = loader->loadIcon("kmfldsent.xpm");
    pixTr    = loader->loadIcon("kmtrash.xpm");
  }

  dict().insert("dir", &pixDir);
  dict().insert("node", &pixNode);
  dict().insert("plain", &pixPlain);
  dict().insert("Fld", &pixFld);
  dict().insert("In", &pixIn);
  dict().insert("Out", &pixOut);
  dict().insert("St", &pixSent);
  dict().insert("Tr", &pixTr);

  setAutoUpdate(TRUE);
  reload();
}


//-----------------------------------------------------------------------------
KMFolderTree::~KMFolderTree()
{
  KConfig* conf = app->getConfig();

  conf->setGroup("Geometry");
  conf->writeEntry(name(), size().width());

  disconnect(folderMgr, SIGNAL(changed()), this, SLOT(doFolderListChanged()));

  if (mDropZone) delete mDropZone;
}


//-----------------------------------------------------------------------------
void KMFolderTree::reload(void)
{
  KMFolderDir* fdir;
  KMFolder* folder;
  QString str;
  bool upd = autoUpdate();

  setAutoUpdate(FALSE);

  clear();
  mList.clear();

  fdir = &folderMgr->dir();

  for (folder = (KMFolder*)fdir->first(); 
       folder != NULL;
       folder = (KMFolder*)fdir->next())
  {
    inSort(folder);
  }
  setAutoUpdate(upd);
  if (upd) repaint();
}


//-----------------------------------------------------------------------------
void KMFolderTree::doFolderListChanged()
{
  uint idx = currentItem();
  debug("doFolderListChanged()");
  reload();
  if (idx >= 0 && idx < count()) setCurrentItem(idx);
}


//-----------------------------------------------------------------------------
void KMFolderTree::doDropAction(KDNDDropZone* aDropZone)
{
  KMFolder *toFld, *fromFld;
  KMDragData* dd;
  KMMessage* msg;
  QPoint pos;
  int i;
  
  if (aDropZone!=mDropZone) return;
  if (aDropZone->getDataType() != DndRawData) return; //sven

  dd = (KMDragData*)aDropZone->getData().ascii();
  if (!dd || sizeof(*dd)!=sizeof(KMDragData)) return;
  fromFld = dd->folder();

  pos.setY(aDropZone->getMouseY());
  pos = mapFromGlobal(pos);

  toFld = (KMFolder*)mList.at(findItem(pos.y()));
  if (!fromFld || !toFld || toFld->isDir()) return;

  fromFld->open();
  toFld->open();

  for (i=dd->to(); i>=dd->from(); i--)
  {
    msg = fromFld->take(i);
    if (msg) toFld->addMsg(msg);
  }

  fromFld->close();
  toFld->close();
}


//-----------------------------------------------------------------------------
void KMFolderTree::inSort(KMFolder* aFolder)
{
  KMFolder* cur;
  QString str;
  int i, cmp;

  for (i=0,cur=(KMFolder*)mList.first(); cur; cur=(KMFolder*)mList.next(),i++)
  {
    cmp = strcmp(aFolder->type(), cur->type());
    if (!cmp) cmp = stricmp(aFolder->label(), cur->label());
    if (cmp < 0) break;
  }

  str = QString("{") + aFolder->type() + "} " + aFolder->label();
  insertItem(str, i);
  mList.insert(i, aFolder);

  if (aFolder->isOpened() && aFolder->countUnread()>0)
     changeItemColor(darkRed, i);
}


//-----------------------------------------------------------------------------
void KMFolderTree::doFolderSelected(int index, int)
{
  KMFolder* folder;

  if (index < 0) return;

  folder = (KMFolder*)mList.at(index);
  if(folder)
  {
    if (folder->isDir()) 
    {
      debug("Folder `%s' is a directory -> ignoring it.",
	    (const char*)folder->name());
      emit folderSelected(NULL);
    }
    
    else emit folderSelected(folder);
  }
}


//-----------------------------------------------------------------------------
void KMFolderTree::resizeEvent(QResizeEvent* e)
{
  KConfig* conf = app->getConfig();

  conf->setGroup("Geometry");
  conf->writeEntry(name(), size().width());

  KMFolderTreeInherited::resizeEvent(e);
}


//-----------------------------------------------------------------------------
int KMFolderTree::indexOfFolder(const KMFolder* folder) const
{
  KMFolderNodeList* list = (KMFolderNodeList*)&mList;
  KMFolderNode* cur;
  int i;

  for (i=0, cur=list->first(); cur; cur=list->next())
  {
    if (cur == (KMFolderNode*)folder) return i;
    i++;
  }
  return -1;
}
