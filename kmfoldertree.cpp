// kmfoldertree.cpp

#include <stdlib.h>
#include <qfileinfo.h>
#include <qpixmap.h>
#include <kapp.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <qtimer.h>
#include <qpopupmenu.h>
#include <qpixmap.h>
#include <klocale.h>
#include <kglobal.h>

#include "kmglobal.h"
#include "kmdragdata.h"
#include "kmfoldermgr.h"
#include "kmfolderdir.h"
#include "kmfolder.h"
#include "kmmainwin.h"

#include <assert.h>

#include "kmfoldertree.moc"

//-----------------------------------------------------------------------------
KMFolderTree::KMFolderTree(QWidget *parent,const char *name):
  KMFolderTreeInherited(parent, name)
{
  initMetaObject();
  mUpdateTimer = NULL;

  setAcceptDrops(true);

  connect(this, SIGNAL(selectionChanged(QListViewItem*)),
	  this, SLOT(doClicked(QListViewItem*)));
  connect(folderMgr, SIGNAL(changed()),
	  this, SLOT(doFolderListChanged()));
  connect(this, SIGNAL(rightButtonPressed(QListViewItem*,const QPoint&,int)),
          this, SLOT(slotRightButtonPressed(QListViewItem*,const QPoint&,int)));
  connect(folderMgr, SIGNAL(unreadChanged(KMFolder*)),
	  this, SLOT(refresh(KMFolder*)));

  addColumn(i18n("Folders"));
  addColumn(i18n("Unread"));
  readConfig();

  // setAutoUpdate(TRUE);
  reload();
}


//-----------------------------------------------------------------------------
QPixmap KMFolderTree::folderTypePixmap(const QString aType)
{
  KIconLoader* loader = KGlobal::iconLoader();
  const char* fname = 0;

  if (aType == "dir") fname = "kmflddir";
  else if (aType == "In") fname = "kmfldin";
  else if (aType == "Out") fname = "kmfldout";
  else if (aType == "St") fname = "kmfldsent";
  else if (aType == "Tr") fname = "kmtrash";
  else fname = "kmfolder";

  return loader->loadIcon(fname);
}


//-----------------------------------------------------------------------------
void KMFolderTree::updateUnreadAll()
{
  KMFolderDir* fdir;
  KMFolder* folder;
  debug( "KMFolderTree::updateUnreadAll" );
  //  bool upd = autoUpdate();
  //  setAutoUpdate(FALSE);

  fdir = &folderMgr->dir();
  for (folder = (KMFolder*)fdir->first();
    folder != NULL;
    folder = (KMFolder*)fdir->next())
  {
    folder->open();
    folder->countUnread();
    folder->close();
  }
  //  setAutoUpdate(upd);
}

//-----------------------------------------------------------------------------
KMFolderTree::~KMFolderTree()
{
  disconnect(folderMgr, SIGNAL(changed()),
	     this, SLOT(doFolderListChanged()));
  disconnect(folderMgr, SIGNAL(unreadChanged(KMFolder*)),
	     this, SLOT(refresh(KMFolder*)));
  delete mUpdateTimer;
}


//-----------------------------------------------------------------------------
void KMFolderTree::readConfig()
{
  KConfig* conf = app->config();
  QCString key;
  int i, w, h;

  conf->setGroup("Geometry");
  key.sprintf("%s-width", name());
  w = conf->readNumEntry(key, 80);
  key.sprintf("%s-height", name());
  h = conf->readNumEntry(key, 120);
  resize(w, h);

  for (i=0; i<2; i++)
  {
    key.sprintf("Column-%d", i);
    w = conf->readNumEntry(key, -1);
    if (w >= 0) setColumnWidth(i, w);
  }
}


//-----------------------------------------------------------------------------
void KMFolderTree::writeConfig()
{
  KConfig* conf = app->config();
  QCString key;
  int i;

  conf->setGroup("Geometry");
  key.sprintf("%s-width", name());
  conf->writeEntry(key, size().width());
  key.sprintf("%s-height", name());
  conf->writeEntry(key, size().height());

  for (i=0; i<2; i++)
  {
    key.sprintf("Column-%d", i);
    conf->writeEntry(key, columnWidth(i));
  }
}


//-----------------------------------------------------------------------------
void KMFolderTree::setCurrentFolder(const KMFolder* aFolder)
{
  KMFolderTreeItem* it = findItem(aFolder);
  if (it)
  {
    setCurrentItem(it);
    setSelected(it, true);
  }
}


//-----------------------------------------------------------------------------
KMFolder* KMFolderTree::currentFolder() const
{
  KMFolderTreeItem* cur = (KMFolderTreeItem*)currentItem();
  if (!cur) return NULL;
  return cur->folder();
}


//-----------------------------------------------------------------------------
KMFolderTreeItem* KMFolderTree::appendItem(const QCString& aPath,
					   KMFolderTreeItem* aParent,
					   bool aBold, char sepChar)
{
  KMFolderTreeItem *item=NULL, *parentItem;
  const char *pos, *beg;
  QCString name;

  assert(aParent!=NULL);

  parentItem = aParent;
  beg = aPath;
  while (beg)
  {
    pos = strchr(beg, sepChar);
    if (pos) name = QCString(beg, pos-beg+1);
    else name = QCString(beg);

    item = (KMFolderTreeItem*)parentItem->firstChild();
    while (item)
    {
      if (QCString(item->text(0)) == name) break;
      item = (KMFolderTreeItem*)item->nextSibling();
    }
    if (!item)
    {
      item = new KMFolderTreeItem(parentItem, name);
      item->setPixmap(0, folderTypePixmap("dir"));
      item->setOpen(true);
      // if (aBold) item->setBold(true);
    }
    parentItem = item;

    if (!pos) break;
    beg = pos+1;
  }

  //  if (item== firstChild()) return NULL;
  return item;  
}


//-----------------------------------------------------------------------------
void KMFolderTree::reload(void)
{
  KMFolderTreeItem* rootItem;
  KMFolderTreeItem* item;
  KMFolderDir* fdir;
  KMFolder* fld;
  QString str;
  long numUnread;

  clear();
  rootItem = (KMFolderTreeItem*)new KMFolderTreeItem(this, i18n("Local Folders"));
  if (rootItem) rootItem->setOpen(true);

  fdir = &folderMgr->dir();
  for (fld = (KMFolder*)fdir->first(); fld; fld = (KMFolder*)fdir->next())
  {
    if (!fld->isDir())
    {
      fld->open();
      numUnread = fld->countUnread();
      fld->close();
    }

    item = appendItem(fld->name(), rootItem, numUnread>0);
    item->setFolder(fld);
    item->setPixmap(0, folderTypePixmap(fld->type()));
    if (!fld->isDir())
    {
      str.sprintf("%ld", numUnread);
      item->setText(1, str);
    }
  }
}


//-----------------------------------------------------------------------------
void KMFolderTree::refresh(KMFolder*)
{
  if (!mUpdateTimer)
  {
    mUpdateTimer = new QTimer(this);
    connect(mUpdateTimer, SIGNAL(timeout()), this, SLOT(delayedUpdate()));
  }
  mUpdateTimer->changeInterval(200);
}


//-----------------------------------------------------------------------------
KMFolderTreeItem* KMFolderTree::findItemRecursive(const KMFolder* aFolder,
						  KMFolderTreeItem* aItem) const 
{
  KMFolderTreeItem* item;
  KMFolderTreeItem* child;
  KMFolderTreeItem* found;

  for (item = (KMFolderTreeItem*)aItem; item;
       item = (KMFolderTreeItem*)item->nextSibling())
  {
    if (item->folder() == aFolder) return item;
    child = (KMFolderTreeItem*)item->firstChild();
    if (child)
    {
      found = findItemRecursive(aFolder, child);
      if (found) return found;
    }
  }
  return NULL;
}


//-----------------------------------------------------------------------------
KMFolderTreeItem* KMFolderTree::findItem(const KMFolder* aFolder) const 
{
  return findItemRecursive(aFolder, (KMFolderTreeItem*)firstChild());
}


//-----------------------------------------------------------------------------
void KMFolderTree::delayedUpdate()
{
  delayedUpdateRecursive((KMFolderTreeItem*)firstChild());
}


//-----------------------------------------------------------------------------
void KMFolderTree::delayedUpdateRecursive(KMFolderTreeItem* aItem)
{
  KMFolder* folder;
  KMFolderTreeItem* item;
  QString str;
  long unread;

  for (item = (KMFolderTreeItem*)aItem; item;
       item = (KMFolderTreeItem*)item->nextSibling())
  {
    folder = item->folder();
    if (folder)
    {
      folder->open();
      str.setNum(folder->countUnread());
      folder->close();
      item->setText(1, str);
    }
    else delayedUpdateRecursive((KMFolderTreeItem*)item->firstChild());
  }

  mUpdateTimer->stop();
}

//-----------------------------------------------------------------------------
void KMFolderTree::doFolderListChanged()
{
  //  QListViewItem* it = currentItem();
  debug("doFolderListChanged()");
  reload();
  //  if (idx >= 0 && idx < count()) setCurrentItem(idx);
}


//-----------------------------------------------------------------------------
void KMFolderTree::dropEvent(QDropEvent * event)
{
#if 0
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
#endif
}


//-----------------------------------------------------------------------------
void KMFolderTree::doClicked(QListViewItem* aItem)
{
  KMFolderTreeItem* it = (KMFolderTreeItem*)aItem;
  KMFolder* fld = NULL;

  if (!it) return;
  if (!(fld = it->folder())) return;

  debug("doClicked: %s\n", fld ? (const char*)fld->label() : "(none)");
  emit folderSelected(fld);
}


//-----------------------------------------------------------------------------
void KMFolderTree::resizeEvent(QResizeEvent* e)
{
  KConfig* conf = app->config();
  int i;
  QCString key;

  conf->setGroup("Geometry");
  conf->writeEntry(name(), size().width());

  for (i=0; i<2; i++)
  {
    key.sprintf("Column-%d", i);
    conf->writeEntry(key, columnWidth(i));
  }

  KMFolderTreeInherited::resizeEvent(e);
}


//-----------------------------------------------------------------------------
void KMFolderTree::slotRightButtonPressed(QListViewItem* aItem,
					  const QPoint& aPos, int)
{
  KMFolderTreeItem* it = (KMFolderTreeItem*)aItem;
  if (!it) return;

  setCurrentItem(it);
  setSelected(it, true);

  if (!topLevelWidget()) return; // safe bet
  QPopupMenu *folderMenu = ((KMMainWin*)topLevelWidget())->folderMenu();
  folderMenu->exec(aPos, 0);
}


//=============================================================================
//=============================================================================
KMFolderTreeItem::KMFolderTreeItem(KMFolderTreeItem* i, const QString& str):
  QListViewItem(i, str)
{
  mFolder = NULL;
}


//-----------------------------------------------------------------------------
KMFolderTreeItem::KMFolderTreeItem(KMFolderTree* t, const QString& str):
  QListViewItem(t, str)
{
  mFolder = NULL;
}

